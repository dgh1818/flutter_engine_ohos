/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/external_view_embedder/external_view_embedder.h"

#include <algorithm>

#include "flutter/display_list/dl_color.h"
#include "flutter/display_list/geometry/dl_path.h"
#include "flutter/flow/view_slicer.h"
#include "flutter/fml/make_copyable.h"
#include "flutter/fml/trace_event.h"
#include "flutter/impeller/geometry/path_source.h"  // PathTransformer
#include "flutter/impeller/geometry/round_superellipse.h"  // RoundSuperellipsePathSource

namespace flutter {
namespace {
// kClipPath 的 path verb 编码,C++/ArkTS 双端共享协议。
// 每个 verb 后跟若干浮点参数(点坐标),参数个数见注释。
// 修改任一编码必须同步修改 ArkTS 端 PlatformViewsControllerHybrid 的 PathVerb。
constexpr int PATH_VERB_CLOSE = 0;  // 0 args
constexpr int PATH_VERB_MOVE = 1;   // 2 args: x, y
constexpr int PATH_VERB_LINE = 2;   // 2 args: x, y
constexpr int PATH_VERB_QUAD = 3;   // 4 args: cpx, cpy, x, y
constexpr int PATH_VERB_CUBIC = 4;  // 6 args: cp1x, cp1y, cp2x, cp2y, x, y

// 4x4 列主序矩阵 DlMatrix::m[16] 的语义化索引,用于日志/解析。
// m[0]=scaleX m[5]=scaleY m[12]=translateX m[13]=translateY。
constexpr int MATRIX_ENTRY_COUNT = 16;

// OnDisplayMutatorsHybrid 扁平 double 数组布局,C++/ArkTS 双端共享协议。
// 修改任一偏移必须同步修改 ArkTS 端 PlatformViewsControllerHybrid.onDisplayMutatorsHybrid。
//   [0]                      opacity
//   [1..1+MATRIX_ENTRY_COUNT) final_matrix m[0..15]
//   [next]                   clipRectsCount
//   [next, 4 each]           clipRects: x, y, w, h
//   [next]                   clipRRectCount
//   [next, 4 each]           clipRRect bounds: x, y, w, h
//   [next, 1 each]           clipRRect radii
//   [next]                   clipPathCount
//   [next, 5 each]           clipPath: bounds(4) + cmdLen(1)
//   [next, var]              clipPath commands (verb + args)
constexpr int RECT_FIELD_COUNT = 4;        // x, y, w, h
constexpr int PATH_HEADER_FIELD_COUNT = 5;  // bounds(4) + cmdLen(1)
constexpr int PATH_COMMAND_RESERVE_FACTOR = 16;  // 每 path 估算的 command 数上限

/// 接收 DlPath 的 verb 回调,把每个 point 经矩阵变换后累积成数值序列
/// (verb + 浮点参数)。用于把 kClipPath 的实际几何序列化下发到 ArkTS。
struct PathCommandsCollector : public impeller::PathReceiver {
  std::vector<double> commands;
  // Endpoint of the most recent verb; ConicTo needs it as the implicit
  // start point for degree-elevation to cubic.
  impeller::Point last_point_{};

  void MoveTo(const impeller::Point& p, bool /*will_be_closed*/) override {
    commands.push_back(PATH_VERB_MOVE);
    commands.push_back(p.x);
    commands.push_back(p.y);
    last_point_ = p;
  }
  void LineTo(const impeller::Point& p) override {
    commands.push_back(PATH_VERB_LINE);
    commands.push_back(p.x);
    commands.push_back(p.y);
    last_point_ = p;
  }
  void QuadTo(const impeller::Point& cp, const impeller::Point& p) override {
    commands.push_back(PATH_VERB_QUAD);
    commands.push_back(cp.x);
    commands.push_back(cp.y);
    commands.push_back(p.x);
    commands.push_back(p.y);
    last_point_ = p;
  }
  bool ConicTo(const impeller::Point& cp, const impeller::Point& p,
               impeller::Scalar weight) override {
    // ArkUI Path 不支持 Conic(有理二次贝塞尔)。
    // 度提升为 Cubic: Q1=(P0+2w·Cp)/(1+2w), Q2=(2w·Cp+P)/(1+2w),
    // 然后丢弃接近 1 的有理三次权重。误差从 QuadTo 的 ~6% 降至 ~2%
    // (90° 弧, weight≈0.707)。ArkTS 侧已支持 PATH_VERB_CUBIC。
    float denom = 1.0f + 2.0f * weight;
    float s = 2.0f * weight / denom;  // control-point blend factor
    float t = 1.0f / denom;           // endpoint blend factor
    impeller::Point c1 = {last_point_.x * t + cp.x * s,
                          last_point_.y * t + cp.y * s};
    impeller::Point c2 = {cp.x * s + p.x * t,
                          cp.y * s + p.y * t};
    CubicTo(c1, c2, p);  // updates last_point_
    return true;
  }
  void CubicTo(const impeller::Point& cp1, const impeller::Point& cp2,
               const impeller::Point& p) override {
    commands.push_back(PATH_VERB_CUBIC);
    commands.push_back(cp1.x);
    commands.push_back(cp1.y);
    commands.push_back(cp2.x);
    commands.push_back(cp2.y);
    commands.push_back(p.x);
    commands.push_back(p.y);
    last_point_ = p;
  }
  void Close() override { commands.push_back(PATH_VERB_CLOSE); }
};

/// Check whether a 2D affine matrix's linear part is diagonal (no rotation or
/// shear). Scale and translate matrices are diagonal; rotation composites like
/// T(c)·R(θ)·T(−c) are not (their off-diagonal elements are ±sinθ).
///
/// Used to decide whether a kTransform mutator should be accumulated into
/// |clip_path_root| — the "rotation-free" transform used for kClipPath/kClipRSE
/// geometry serialization.  By excluding rotation-bearing mutators, the path
/// coordinates stay in the node's local (unrotated) space, and ArkTS .rotate()
/// is the sole source of on-screen rotation — preventing the double-rotation
/// deformation (§28).
bool IsDiagonal2D(const DlMatrix& m) {
  // Column-major: off-diagonal 2D elements are m[1] (row 1, col 0)
  // and m[4] (row 0, col 1).
  return std::abs(m.m[1]) < 1e-6f && std::abs(m.m[4]) < 1e-6f;
}
}  // namespace


OHOSExternalViewEmbedder::OHOSExternalViewEmbedder(
    const std::shared_ptr<OHOSContext>& ohos_context,
    const std::shared_ptr<PlatformViewOHOSNapi>& napi_facade,
    const std::shared_ptr<OhosSurfaceFactory>& surface_factory,
    const TaskRunners& task_runners)
    : ExternalViewEmbedder(),
      ohos_context_(ohos_context),
      napi_facade_(napi_facade),
      surface_factory_(surface_factory),
      task_runners_(task_runners) {}

OHOSExternalViewEmbedder::~OHOSExternalViewEmbedder() = default;

void OHOSExternalViewEmbedder::PrerollCompositeEmbeddedView(
    int64_t view_id,
    std::unique_ptr<EmbeddedViewParams> params) {
  // 为平台视图创建一个独立的绘制切片（slice）
  DlRect view_bounds = DlRect::MakeSize(frame_size_);
  std::unique_ptr<EmbedderViewSlice> view =
      std::make_unique<DisplayListEmbedderViewSlice>(view_bounds);
  slices_.insert_or_assign(view_id, std::move(view));
  // 记录合成顺序（Z 序）
  composition_order_.push_back(view_id);
  // 缓存视图参数（尺寸、位置、mutator 栈），仅变化时更新
  if (view_params_.count(view_id) == 1 &&
      view_params_.at(view_id) == *params.get()) {
    return;
  }
  view_params_.insert_or_assign(view_id, EmbeddedViewParams(*params.get()));
}

DlCanvas* OHOSExternalViewEmbedder::CompositeEmbeddedView(int64_t view_id) {
  if (slices_.count(view_id) == 1) {
    return slices_.at(view_id)->canvas();
  }
  return nullptr;
}

DlRect OHOSExternalViewEmbedder::GetViewRect(
    int64_t view_id,
    const std::unordered_map<int64_t, EmbeddedViewParams>& view_params) {
  const EmbeddedViewParams& params = view_params.at(view_id);
  return params.finalBoundingRect();
}

OHOSExternalViewEmbedder::FoldedMutators
OHOSExternalViewEmbedder::FoldMutatorsToFinal(
    const EmbeddedViewParams& params) {
  FoldedMutators out;
  DlMatrix root_transform;
  // clip_path_root: 只累积对角 2D(无旋转/剪切)的 kTransform。
  // 用于 kClipPath/kClipRSE 的 path 几何序列化——确保 path 坐标不含旋转分量,
  // 旋转仅由 ArkTS .rotate(angleDeg) 施加一次,避免双重旋转导致多边形变形(§28)。
  DlMatrix clip_path_root;
  // Flutter 在 layer tree 里,平台视图上方的每一层(TransformLayer / OpacityLayer / ClipRectLayer)
  // 会给这个视图叠一个 mutator。这些 mutator构成一个栈(MutatorsStack),按从下到上的顺序排列。
  const MutatorsStack& stack = params.mutatorsStack();
  for (auto it = stack.Begin(); it != stack.End(); ++it) {
    const std::shared_ptr<Mutator>& mutator = *it;
    switch (mutator->GetType()) {
      case MutatorType::kTransform: {
        const DlMatrix& m = mutator->GetMatrix();
        root_transform = root_transform * m;
        // 只累积无旋转的变换(scale/translate,如栈底 dpr)到 clip_path_root。
        // 旋转复合(如 T(c)·R(θ)·T(−c))的 2×2 部分非对角 → 跳过。
        bool diagonal = IsDiagonal2D(m);
        if (diagonal) {
          clip_path_root = clip_path_root * m;
        }
        break;
      }
      case MutatorType::kOpacity: {
        out.opacity *= std::clamp(mutator->GetAlphaFloat(), 0.0f, 1.0f);
        break;
      }
      case MutatorType::kClipRect: {
        const DlRect& raw = mutator->GetRect();
        // 用 root_transform（含栈底 dpr + 全部 kTransform）折叠成最终 px，
        // 保证每个 clip 都带 dpr——修复 reset 丢 dpr 导致"半屏"裁剪的根因。
        DlRect transformed = raw.TransformBounds(root_transform);
        out.clip_rects.push_back(transformed);
        break;
      }
      case MutatorType::kClipRRect: {
        const DlRoundRect& rrect = mutator->GetRRect();
        // 用 root_transform（含栈底 dpr + 全部 kTransform，clip 不清空它）折叠成
        // 最终 px，与 kClipRect 一致——修复多 clip 串行时"每个 clip 只用它外侧的
        // 变换、遇到 clip 就重置"的旧算法丢 dpr 的根因。radii 不经 TransformBounds
        // （见坐标空间铁律，rrect.GetRadii() 返回 layer-tree points/vp），本就不丢
        // dpr，无需改。
        out.clip_rrect_bounds.push_back(
            rrect.GetBounds().TransformBounds(root_transform));
        // Store the corner radius (top_left.x; all 4 corners are equal
        // in the common case; otherwise degrades to the top-left corner).
        out.clip_rrect_radii.push_back(rrect.GetRadii().top_left.width);
        break;
      }
      case MutatorType::kClipRSE: {
        // 从 mutator 取出椭圆(squircle)几何,包装成一个可被 Dispatch 遍历的 Path 源
        // 目的是把椭圆的曲线轮廓精确序列化成 Path，供 ArkTS 端重建 ArkUI Path。
        const DlRoundSuperellipse& rse = mutator->GetRSE();
        impeller::RoundSuperellipsePathSource source(rse);
        // 去旋转:clip_path_root 只含 scale/translate(dpr 等),不含旋转。
        // 旋转由 ArkTS .rotate() 单独施加;若 path 也预旋转会导致双重旋转 → 变形(§28)。
        out.clip_path_bounds.push_back(
            rse.GetBounds().TransformBounds(clip_path_root));
        PathCommandsCollector collector;
        impeller::PathTransformer transformer(collector, clip_path_root);
        source.Dispatch(transformer);
        out.clip_path_commands.push_back(std::move(collector.commands));
        break;
      }
      case MutatorType::kClipPath: {
        const DlPath& path = mutator->GetPath();
        // 序列化实际路径几何:collect 成 verb+args 数值序列。
        // ArkTS 端据此重建 ArkUI Path commands(支持三角形/多边形等任意形状)。
        // 去旋转:clip_path_root 只含对角(scale/translate)变换,无旋转。
        // 旋转由 ArkTS .rotate(angleDeg) 单独施加;若 path 也预旋转,
        // ArkTS .rotate() 会二次旋转 → clip 与 content 角度偏差 → 多边形变形(§28)。
        // clip_path_root 不 reset(同 root_transform 语义):多 clip 串行时不丢 dpr。
        out.clip_path_bounds.push_back(
            path.GetBounds().TransformBounds(clip_path_root));
        PathCommandsCollector collector;
        impeller::PathTransformer transformer(collector, clip_path_root);
        path.Dispatch(transformer);
        out.clip_path_commands.push_back(std::move(collector.commands));
        break;
      }
      default: {
        break;
      }
    }
  }

  out.final_matrix = params.transformMatrix();
  out.opacity = std::clamp(out.opacity, 0.0f, 1.0f);
  return out;
}

bool OHOSExternalViewEmbedder::FrameHasPlatformLayers() {
  return !composition_order_.empty();
}

Surface* OHOSExternalViewEmbedder::EnsureOverlaySurface(
    GrDirectContext* context) {
  fml::RefPtr<OHOSNativeWindow> target;
  {
    std::lock_guard<std::mutex> lock(overlay_mutex_);
    target = overlay_window_;
  }

  if (!target) {
    return nullptr;
  }

  if (overlay_window_dirty_.exchange(false) ||
      target != applied_overlay_window_) {
    overlay_gpu_surface_.reset();
    overlay_ohos_surface_.reset();
    applied_overlay_window_ = target;
  }

  if (overlay_gpu_surface_) {
    return overlay_gpu_surface_.get();
  }

  overlay_ohos_surface_ = surface_factory_->CreateSurface();
  if (!overlay_ohos_surface_ ||
      !overlay_ohos_surface_->SetDisplayWindow(applied_overlay_window_)) {
    FML_LOG(ERROR) << "HCPP: failed to bind overlay native window.";
    overlay_ohos_surface_.reset();
    return nullptr;
  }
  // 将一个 OHOS 原生窗口（OHNativeWindow）包装成 Flutter 引擎可以绘图的 Surface 对象
  overlay_gpu_surface_ = overlay_ohos_surface_->CreateGPUSurface(context);
  FML_LOG(INFO) << "HCPP embedder: overlay surface created="
                << (overlay_gpu_surface_ != nullptr);
  return overlay_gpu_surface_.get();
}

void OHOSExternalViewEmbedder::SubmitFlutterView(
    int64_t flutter_view_id,
    GrDirectContext* context,
    const std::shared_ptr<impeller::AiksContext>& aiks_context,
    std::unique_ptr<SurfaceFrame> frame) {

  if (!FrameHasPlatformLayers()) {
    frame->Submit();
    if (overlay_had_content_last_frame_) {
      Surface* overlay_surface = EnsureOverlaySurface(context);
      if (overlay_surface != nullptr) {
        std::unique_ptr<SurfaceFrame> overlay_frame =
            overlay_surface->AcquireFrame(frame_size_);
        if (overlay_frame != nullptr) {
          overlay_frame->Canvas()->Clear(DlColor::kTransparent());
          overlay_frame->set_submit_info({.frame_boundary = false});
          overlay_frame->Submit();
        }
      }
      overlay_had_content_last_frame_ = false;
    }
    task_runners_.GetPlatformTaskRunner()->PostTask(fml::MakeCopyable(
        [overlay_layer_is_shown = overlay_layer_is_shown_,
         napi_facade = napi_facade_,
         views_visible_last_frame = views_visible_last_frame_]() {
          if (overlay_layer_is_shown->load()) {
            napi_facade->HideOverlaySurfaceHybrid();
            overlay_layer_is_shown->store(false);
          }
          for (int64_t view_id : views_visible_last_frame) {
            napi_facade->HidePlatformViewHybrid(view_id);
          }
          napi_facade->OnEndFrameHybrid();
        }));
    views_visible_last_frame_.clear();
    return;
  }

  std::unordered_map<int64_t, DlRect> view_rects;
  for (int64_t platform_id : composition_order_) {
    view_rects[platform_id] = GetViewRect(platform_id, view_params_);
  }

  // 每个 PV 各自从自己的 stack 算各自的可见区
  for (auto& kv : view_rects) {
    const auto& params = view_params_.at(kv.first);
    const MutatorsStack& stack = params.mutatorsStack();
    DlMatrix accumulator;
    bool has_viewport_clip = false;
    DlRect viewport_clip;
    for (auto it = stack.Begin(); it != stack.End(); ++it) {
      if ((*it)->GetType() == MutatorType::kTransform) {
        accumulator = accumulator * (*it)->GetMatrix();
      } else if ((*it)->GetType() == MutatorType::kClipRect) {
        DlRect cr = (*it)->GetRect().TransformBounds(accumulator);
        viewport_clip = has_viewport_clip
                            ? viewport_clip.IntersectionOrEmpty(cr)
                            : cr;
        has_viewport_clip = true;
      }
    }
    if (has_viewport_clip) {
      kv.second = kv.second.IntersectionOrEmpty(viewport_clip);
    }
  }

  // Clip the background canvas ("punch holes") and compute the overlay regions.
  std::unordered_map<int64_t, DlRect> overlay_layers = SliceViews(
      frame->Canvas(), composition_order_, slices_, view_rects);

  bool overlay_has_content = false;
  Surface* overlay_surface = EnsureOverlaySurface(context);
  if (overlay_surface == nullptr && !overlay_layers.empty()) {
    // Background holes were punched above, so the Flutter content that
    // belongs above the platform view(s) is silently lost until the overlay
    // surface arrives. Error once per drop window (not per frame).
    if (!overlay_drop_logged_) {
      overlay_drop_logged_ = true;
      FML_LOG(ERROR) << "HCPP overlay surface unavailable; dropping "
                     << overlay_layers.size()
                     << " overlay region(s) above platform view(s).";
    }
  } else {
    overlay_drop_logged_ = false;
  }

  const bool overlay_has_content_this_frame = !overlay_layers.empty();
  const bool need_clear = overlay_has_content_this_frame ||
                          overlay_had_content_last_frame_;
  if (overlay_surface != nullptr && need_clear) {
    std::unique_ptr<SurfaceFrame> overlay_frame =
        overlay_surface->AcquireFrame(frame_size_);
    if (overlay_frame != nullptr) {
      overlay_frame->Canvas()->Clear(DlColor::kTransparent());
      for (size_t i = 0; i < composition_order_.size(); i++) {
        int64_t view_id = composition_order_[i];
        auto overlay = overlay_layers.find(view_id);
        if (overlay == overlay_layers.end()) {
          continue;
        }

        DlCanvas* overlay_canvas = overlay_frame->Canvas();
        int restore_count = overlay_canvas->GetSaveCount();
        overlay_canvas->Save();
        overlay_canvas->ClipRect(overlay->second);

        for (size_t j = i + 1; j < composition_order_.size(); j++) {
          DlRect view_rect = view_rects[composition_order_[j]];
          overlay_canvas->ClipRect(view_rect, DlClipOp::kDifference);
        }

        slices_.at(view_id)->render_into(overlay_canvas);
        overlay_canvas->RestoreToCount(restore_count);
        overlay_has_content = true;
      }
      overlay_frame->set_submit_info({.frame_boundary = false});
      overlay_frame->Submit();
    }
  }

  overlay_had_content_last_frame_ = overlay_has_content_this_frame;

  // Track whether the overlay regions were actually RENDERED this frame (vs
  // merely computed). When the overlay surface is unavailable the regions
  // were dropped, so their touch Block rects must not be published either —
  // a published-but-invisible HitTestMode.Block swallows touches aimed at
  // the native content the user actually sees (finding: overlay rect vs
  // dropped pixels divergence).
  const bool overlay_rendered = overlay_has_content;

  // Always submit, even for passes that were not painted: the Vulkan
  // AcquireFrame eagerly holds a swapchain image that only Submit returns
  // (see PostPrerollAction note). Skipping the submit leaks it.
  frame->Submit();

  task_runners_.GetPlatformTaskRunner()->PostTask(fml::MakeCopyable(
      [overlay_layer_is_shown = overlay_layer_is_shown_,
       composition_order = composition_order_,
       view_params = view_params_, napi_facade = napi_facade_,
       device_pixel_ratio = device_pixel_ratio_,
       views_visible_last_frame = views_visible_last_frame_,
       overlay_has_content,
       overlay_rendered,
       overlay_layers = overlay_layers]() mutable -> void {
        if (overlay_has_content) {
          if (!overlay_layer_is_shown->load()) {
            napi_facade->ShowOverlaySurfaceHybrid();
            overlay_layer_is_shown->store(true);
          }
        } else {
          if (overlay_layer_is_shown->load()) {
            napi_facade->HideOverlaySurfaceHybrid();
            overlay_layer_is_shown->store(false);
          }
        }

        for (int64_t view_id : composition_order) {
          DlRect view_rect = GetViewRect(view_id, view_params);
          const EmbeddedViewParams& params = view_params.at(view_id);

          napi_facade->OnDisplayPlatformViewHybrid(
              view_id,
              view_rect.GetX(),
              view_rect.GetY(),
              view_rect.GetWidth(),
              view_rect.GetHeight(),
              params.sizePoints().width * device_pixel_ratio,
              params.sizePoints().height * device_pixel_ratio);

          FoldedMutators folded = FoldMutatorsToFinal(params);
          std::vector<double> data;
          // 头部 = opacity(1) + matrix(16) + clipRectsCount(1) = 18;
          // +2 = clipRRectCount + clipPathCount。
          const size_t kMutatorsHeaderSize =
              1 + MATRIX_ENTRY_COUNT + 1;  // opacity + matrix + clipRectsCount
          data.reserve(kMutatorsHeaderSize +
                       RECT_FIELD_COUNT * folded.clip_rects.size() +
                       RECT_FIELD_COUNT * folded.clip_rrect_bounds.size() +
                       folded.clip_rrect_radii.size() + 2 +
                       PATH_HEADER_FIELD_COUNT * folded.clip_path_bounds.size() +
                       folded.clip_path_commands.size() * PATH_COMMAND_RESERVE_FACTOR);
          data.push_back(folded.opacity);
          for (int i = 0; i < MATRIX_ENTRY_COUNT; ++i) {
            data.push_back(folded.final_matrix.m[i]);
          }
          data.push_back(static_cast<double>(folded.clip_rects.size()));
          for (const DlRect& c : folded.clip_rects) {
            data.push_back(c.GetX());
            data.push_back(c.GetY());
            data.push_back(c.GetWidth());
            data.push_back(c.GetHeight());
          }
          data.push_back(static_cast<double>(folded.clip_rrect_bounds.size()));
          for (const DlRect& c : folded.clip_rrect_bounds) {
            data.push_back(c.GetX());
            data.push_back(c.GetY());
            data.push_back(c.GetWidth());
            data.push_back(c.GetHeight());
          }

          for (float r : folded.clip_rrect_radii) {
            data.push_back(r);
          }
          // clipPath: count + 每个 path 的 bounds(4值) + cmdLen + commands(变长)。
          // bounds 是 path 外接矩形(px),供 ArkTS 算 pathWidth/Height;
          // verb 是"路径操作码"——描述一条路径(path)由哪些基本绘图动作
          // (移动到、画线到、画曲线到、闭合)组成,每个 verb 后跟若干个点坐标作为参数
          // ArkTS 端按 cmdLen 读出 commands,再按 verb 编码解析每个 verb 的参数个数,重建 SVG commands。
          data.push_back(static_cast<double>(folded.clip_path_bounds.size()));
          for (size_t i = 0; i < folded.clip_path_bounds.size(); ++i) {
            const DlRect& b = folded.clip_path_bounds[i];
            data.push_back(b.GetX());
            data.push_back(b.GetY());
            data.push_back(b.GetWidth());
            data.push_back(b.GetHeight());
            const auto& cmds = folded.clip_path_commands[i];
            data.push_back(static_cast<double>(cmds.size()));
            for (double v : cmds) {
              data.push_back(v);
            }
          }
          napi_facade->OnDisplayMutatorsHybrid(view_id, data);
          views_visible_last_frame.erase(view_id);
        }

        for (int64_t view_id : views_visible_last_frame) {
          napi_facade->HidePlatformViewHybrid(view_id);
        }

        // 下发 SliceViews 算出的全部 overlay rect。仅在 overlay 像素确实渲染
        // 进 overlay surface 时下发：surface 不可用（窗口未注册/被销毁）时本帧
        // 区域已被丢弃，若仍下发，ArkTS 侧会挂出对应 HitTestMode.Block，
        // 在用户实际看到原生内容/破洞的区域吞触摸。丢弃帧不下发 →
        // onEndFrameHybrid flush 空 rects → Block 卸载，触摸直达下层。
        if (overlay_rendered) {
          for (const auto& kv : overlay_layers) {
            const DlRect& r = kv.second;
            napi_facade->OnDisplayOverlayHybrid(kv.first, r.GetX(), r.GetY(),
                                           r.GetWidth(), r.GetHeight());
          }
        }

        napi_facade->OnEndFrameHybrid();
      }));

  views_visible_last_frame_.clear();
  views_visible_last_frame_.insert(composition_order_.begin(),
                                   composition_order_.end());
}

PostPrerollResult OHOSExternalViewEmbedder::PostPrerollAction(
    const fml::RefPtr<fml::RasterThreadMerger>& raster_thread_merger) {
  return PostPrerollResult::kSuccess;
}

DlCanvas* OHOSExternalViewEmbedder::GetRootCanvas() {
  return nullptr;
}

void OHOSExternalViewEmbedder::Reset() {
  composition_order_.clear();
  slices_.clear();
}

// Android 的 BeginFrame 通知 Java 重置逐帧记账集合（ImageView 池 + PV attach 差集管理）；
// OHOS 的架构（overlay XComponent + 常驻 DISPLAY 节点 + C++自管可见性差集）没有这两个消费者
void OHOSExternalViewEmbedder::BeginFrame(
    GrDirectContext* context,
    const fml::RefPtr<fml::RasterThreadMerger>& raster_thread_merger) {}

void OHOSExternalViewEmbedder::PrepareFlutterView(DlISize frame_size,
                                                  double device_pixel_ratio) {
  Reset();

  if (frame_size_ != frame_size) {
    DestroyOverlaySurface();
  }
  frame_size_ = frame_size;
  device_pixel_ratio_ = device_pixel_ratio;
}

void OHOSExternalViewEmbedder::CancelFrame() {
  Reset();
}

void OHOSExternalViewEmbedder::EndFrame(
    bool should_resubmit_frame,
    const fml::RefPtr<fml::RasterThreadMerger>& raster_thread_merger) {
  std::vector<int64_t> stale_ids;
  for (const auto& kv : view_params_) {
    if (std::find(composition_order_.begin(), composition_order_.end(),
                  kv.first) == composition_order_.end()) {
      stale_ids.push_back(kv.first);
    }
  }
  for (int64_t id : stale_ids) {
    view_params_.erase(id);
  }
}

bool OHOSExternalViewEmbedder::SupportsDynamicThreadMerging() {
  return false;
}

void OHOSExternalViewEmbedder::Teardown() {
  DestroyOverlaySurface();
}

void OHOSExternalViewEmbedder::SetOverlayWindow(
    fml::RefPtr<OHOSNativeWindow> overlay_window) {
  std::lock_guard<std::mutex> lock(overlay_mutex_);
  if (overlay_window_ == overlay_window) {
    return;
  }
  overlay_window_ = std::move(overlay_window);
  overlay_window_dirty_.store(true);
}

void OHOSExternalViewEmbedder::TearDownOverlayWindow() {
  SetOverlayWindow(nullptr);
  DestroyOverlaySurface();
}

void OHOSExternalViewEmbedder::DestroyOverlaySurface() {
  overlay_gpu_surface_.reset();
  overlay_ohos_surface_.reset();
  applied_overlay_window_ = nullptr;
  overlay_layer_is_shown_->store(false);
  overlay_had_content_last_frame_ = false;
}

void OHOSExternalViewEmbedder::ShowOverlayLayerIfNeeded() {
  if (!overlay_layer_is_shown_->load()) {
    napi_facade_->ShowOverlaySurfaceHybrid();
    overlay_layer_is_shown_->store(true);
  }
}

void OHOSExternalViewEmbedder::HideOverlayLayerIfNeeded() {
  if (overlay_layer_is_shown_->load()) {
    napi_facade_->HideOverlaySurfaceHybrid();
    overlay_layer_is_shown_->store(false);
  }
}

}  // namespace flutter
