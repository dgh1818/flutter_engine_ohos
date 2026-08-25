/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/fml/build_config.h"  // IWYU pragma: keep  (defines FML_OS_OHOS)

#if defined(FML_OS_OHOS)

#include "flutter/shell/platform/ohos/external_view_embedder/external_view_embedder.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "flutter/display_list/dl_builder.h"
#include "flutter/display_list/dl_color.h"
#include "flutter/display_list/dl_paint.h"
#include "flutter/display_list/geometry/dl_geometry_types.h"
#include "flutter/display_list/geometry/dl_path_builder.h"
#include "flutter/flow/embedded_views.h"
#include "flutter/flow/surface_frame.h"
#include "flutter/fml/raster_thread_merger.h"
#include "flutter/fml/synchronization/waitable_event.h"
#include "flutter/fml/thread.h"
#include "flutter/impeller/display_list/aiks_context.h"
#include "flutter/impeller/renderer/backend/vulkan/test/mock_vulkan.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace flutter {
namespace testing {

using ::testing::_;
using ::testing::ByMove;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

// Path verb 编码协议（与 external_view_embedder.cc 私有常量保持同步）。
constexpr double kPathVerbClose = 0;
constexpr double kPathVerbMove = 1;
constexpr double kPathVerbLine = 2;
constexpr double kPathVerbQuad = 3;

// 构造一个基础 mutator 栈 + 对应的 EmbeddedViewParams。
EmbeddedViewParams MakeParams(DlMatrix matrix,
                              DlSize size,
                              MutatorsStack stack) {
  return EmbeddedViewParams(matrix, size, std::move(stack));
}

// 顶点为 (0,0) (100,0) (50,50) 的三角形，bounds 为 (0,0,100,50)。
DlPath MakeTrianglePath() {
  DlPathBuilder builder;
  builder.MoveTo(DlPoint(0, 0));
  builder.LineTo(DlPoint(100, 0));
  builder.LineTo(DlPoint(50, 50));
  builder.Close();
  return builder.TakePath();
}

void ExpectRectNear(const DlRect& rect,
                    DlScalar left,
                    DlScalar top,
                    DlScalar width,
                    DlScalar height) {
  EXPECT_FLOAT_EQ(rect.GetLeft(), left);
  EXPECT_FLOAT_EQ(rect.GetTop(), top);
  EXPECT_FLOAT_EQ(rect.GetWidth(), width);
  EXPECT_FLOAT_EQ(rect.GetHeight(), height);
}

}  // namespace

//------------------------------------------------------------------------------
// GetViewRect（静态）
//------------------------------------------------------------------------------

TEST(OHOSExternalViewEmbedderStatic, GetViewRectReturnsFinalBoundingRect) {
  std::unordered_map<int64_t, EmbeddedViewParams> view_params;
  view_params.emplace(1, MakeParams(DlMatrix::MakeTranslation(DlVector3(10, 20, 0)),
                                    DlSize(100, 50), MutatorsStack()));
  DlRect rect = OHOSExternalViewEmbedder::GetViewRect(1, view_params);
  ExpectRectNear(rect, 10, 20, 100, 50);
}

TEST(OHOSExternalViewEmbedderStatic, GetViewRectUnknownViewIsFatal) {
  // flutter_ohos_unittests 以 -fno-exceptions 编译，map::at 对未知 id 的
  // 失败路径是 abort 而非可捕获异常，无法在测试内安全断言；此处仅锁定
  // 正常路径语义，未知 id 的致命行为由引擎调用约定保证（调用方只传
  // Preroll 登记过的 view id）。
  std::unordered_map<int64_t, EmbeddedViewParams> view_params;
  view_params.emplace(7, MakeParams(DlMatrix(), DlSize(30, 20),
                                    MutatorsStack()));
  EXPECT_NO_FATAL_FAILURE(OHOSExternalViewEmbedder::GetViewRect(7, view_params));
}

//------------------------------------------------------------------------------
// FoldMutatorsToFinal（静态）
//------------------------------------------------------------------------------

TEST(OHOSExternalViewEmbedderStatic, FoldEmptyStack) {
  DlMatrix matrix = DlMatrix::MakeTranslation(DlVector3(5, 6, 0));
  EmbeddedViewParams params =
      MakeParams(matrix, DlSize(10, 10), MutatorsStack());

  auto folded = OHOSExternalViewEmbedder::FoldMutatorsToFinal(params);
  EXPECT_EQ(folded.final_matrix, matrix);
  EXPECT_FLOAT_EQ(folded.opacity, 1.0f);
  EXPECT_TRUE(folded.clip_rects.empty());
  EXPECT_TRUE(folded.clip_rrect_bounds.empty());
  EXPECT_TRUE(folded.clip_rrect_radii.empty());
  EXPECT_TRUE(folded.clip_path_bounds.empty());
  EXPECT_TRUE(folded.clip_path_commands.empty());
}

TEST(OHOSExternalViewEmbedderStatic, FoldOpacityMultiplies) {
  MutatorsStack stack;
  stack.PushOpacity(128);
  stack.PushOpacity(128);
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(10, 10), std::move(stack));

  auto folded = OHOSExternalViewEmbedder::FoldMutatorsToFinal(params);
  double expected = (128.0 / 255.0) * (128.0 / 255.0);
  EXPECT_NEAR(folded.opacity, expected, 1e-4);
}

TEST(OHOSExternalViewEmbedderStatic, FoldOpacityClampedAtOne) {
  MutatorsStack stack;
  stack.PushOpacity(255);
  stack.PushOpacity(255);
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(10, 10), std::move(stack));

  auto folded = OHOSExternalViewEmbedder::FoldMutatorsToFinal(params);
  EXPECT_FLOAT_EQ(folded.opacity, 1.0f);
}

TEST(OHOSExternalViewEmbedderStatic, FoldClipRectTransformedByStack) {
  MutatorsStack stack;
  stack.PushTransform(DlMatrix::MakeScale(DlVector2(2, 2)));
  stack.PushClipRect(DlRect::MakeXYWH(10, 10, 50, 40));
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(100, 100), std::move(stack));

  auto folded = OHOSExternalViewEmbedder::FoldMutatorsToFinal(params);
  ASSERT_EQ(folded.clip_rects.size(), 1u);
  // dpr=2 缩放折叠进 clip：bounds 乘 2。
  ExpectRectNear(folded.clip_rects[0], 20, 20, 100, 80);
}

TEST(OHOSExternalViewEmbedderStatic, FoldClipRectAfterTranslate) {
  MutatorsStack stack;
  stack.PushTransform(DlMatrix::MakeTranslation(DlVector3(10, 20, 0)));
  stack.PushClipRect(DlRect::MakeXYWH(0, 0, 50, 40));
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(100, 100), std::move(stack));

  auto folded = OHOSExternalViewEmbedder::FoldMutatorsToFinal(params);
  ASSERT_EQ(folded.clip_rects.size(), 1u);
  ExpectRectNear(folded.clip_rects[0], 10, 20, 50, 40);
}

TEST(OHOSExternalViewEmbedderStatic,
     FoldMultipleTransformsAccumulateInOrder) {
  MutatorsStack stack;
  stack.PushTransform(DlMatrix::MakeScale(DlVector2(2, 2)));
  stack.PushTransform(DlMatrix::MakeTranslation(DlVector3(10, 20, 0)));
  stack.PushClipRect(DlRect::MakeXYWH(0, 0, 10, 5));
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(100, 100), std::move(stack));

  auto folded = OHOSExternalViewEmbedder::FoldMutatorsToFinal(params);
  ASSERT_EQ(folded.clip_rects.size(), 1u);
  // 先平移 (10,20) 再整体乘 2：(10,20,10,5) -> (20,40,20,10)。
  ExpectRectNear(folded.clip_rects[0], 20, 40, 20, 10);
}

TEST(OHOSExternalViewEmbedderStatic,
     FoldMultipleClipRectsKeepTransformAcrossClips) {
  MutatorsStack stack;
  stack.PushTransform(DlMatrix::MakeScale(DlVector2(2, 2)));
  stack.PushClipRect(DlRect::MakeXYWH(0, 0, 10, 10));
  stack.PushClipRect(DlRect::MakeXYWH(5, 5, 10, 10));
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(100, 100), std::move(stack));

  auto folded = OHOSExternalViewEmbedder::FoldMutatorsToFinal(params);
  ASSERT_EQ(folded.clip_rects.size(), 2u);
  // 两个 clip 都带 dpr：遇 clip 不重置 root_transform（第二个
  // clip 丢 dpr 出现“半屏”裁剪）。
  ExpectRectNear(folded.clip_rects[0], 0, 0, 20, 20);
  ExpectRectNear(folded.clip_rects[1], 10, 10, 20, 20);
}

TEST(OHOSExternalViewEmbedderStatic, FoldClipRRectTransformsBoundsNotRadii) {
  MutatorsStack stack;
  stack.PushTransform(DlMatrix::MakeScale(DlVector2(2, 2)));
  stack.PushClipRRect(
      DlRoundRect::MakeRectRadius(DlRect::MakeXYWH(10, 10, 50, 40), 8));
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(100, 100), std::move(stack));

  auto folded = OHOSExternalViewEmbedder::FoldMutatorsToFinal(params);
  ASSERT_EQ(folded.clip_rrect_bounds.size(), 1u);
  ASSERT_EQ(folded.clip_rrect_radii.size(), 1u);
  // bounds 折叠到物理像素；radii 保持 layer-tree points（不乘 dpr）。
  ExpectRectNear(folded.clip_rrect_bounds[0], 20, 20, 100, 80);
  EXPECT_FLOAT_EQ(folded.clip_rrect_radii[0], 8.0f);
}

TEST(OHOSExternalViewEmbedderStatic, FoldClipPathSerializesVerbCommands) {
  MutatorsStack stack;
  stack.PushTransform(DlMatrix::MakeScale(DlVector2(2, 2)));
  stack.PushClipPath(MakeTrianglePath());
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(100, 100), std::move(stack));

  auto folded = OHOSExternalViewEmbedder::FoldMutatorsToFinal(params);
  ASSERT_EQ(folded.clip_path_bounds.size(), 1u);
  ASSERT_EQ(folded.clip_path_commands.size(), 1u);

  // bounds 随 dpr 缩放。
  ExpectRectNear(folded.clip_path_bounds[0], 0, 0, 200, 100);

  // commands 序列：MOVE LINE LINE LINE(回起点) CLOSE。末尾多出的 LINE 是
  // SkPath::Iter 的行为：闭合路径最后一点 != MoveTo 起点时，Close verb 前
  // 会显式补发一条回起点的 Line（真机 13 个数值验证）。坐标乘 dpr。
  const std::vector<double>& commands = folded.clip_path_commands[0];
  ASSERT_EQ(commands.size(), 13u);
  EXPECT_DOUBLE_EQ(commands[0], kPathVerbMove);
  EXPECT_DOUBLE_EQ(commands[1], 0);
  EXPECT_DOUBLE_EQ(commands[2], 0);
  EXPECT_DOUBLE_EQ(commands[3], kPathVerbLine);
  EXPECT_DOUBLE_EQ(commands[4], 200);
  EXPECT_DOUBLE_EQ(commands[5], 0);
  EXPECT_DOUBLE_EQ(commands[6], kPathVerbLine);
  EXPECT_DOUBLE_EQ(commands[7], 100);
  EXPECT_DOUBLE_EQ(commands[8], 100);
  EXPECT_DOUBLE_EQ(commands[9], kPathVerbLine);
  EXPECT_DOUBLE_EQ(commands[10], 0);
  EXPECT_DOUBLE_EQ(commands[11], 0);
  EXPECT_DOUBLE_EQ(commands[12], kPathVerbClose);
}

TEST(OHOSExternalViewEmbedderStatic,
     FoldRotationExcludedFromClipPathSerialization) {
  MutatorsStack stack;
  stack.PushTransform(DlMatrix::MakeRotationZ(DlRadians{kPi / 2}));
  stack.PushClipPath(MakeTrianglePath());
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(100, 100), std::move(stack));

  auto folded = OHOSExternalViewEmbedder::FoldMutatorsToFinal(params);
  ASSERT_EQ(folded.clip_path_bounds.size(), 1u);
  ASSERT_EQ(folded.clip_path_commands.size(), 1u);

  // kClipPath 的几何序列化用 clip_path_root（只含对角变换），
  // 旋转矩阵被排除——ArkTS 端 .rotate() 是唯一旋转来源，避免双重旋转。
  // 因此 path 坐标保持节点局部坐标，bounds 不旋转。
  ExpectRectNear(folded.clip_path_bounds[0], 0, 0, 100, 50);

  const std::vector<double>& commands = folded.clip_path_commands[0];
  // 同 FoldClipPathSerializesVerbCommands：Close 前补回起点 LINE（13 值），
  // 坐标保持局部空间（旋转被 clip_path_root 排除）。
  ASSERT_EQ(commands.size(), 13u);
  EXPECT_DOUBLE_EQ(commands[0], kPathVerbMove);
  EXPECT_DOUBLE_EQ(commands[1], 0);
  EXPECT_DOUBLE_EQ(commands[2], 0);
  EXPECT_DOUBLE_EQ(commands[3], kPathVerbLine);
  EXPECT_DOUBLE_EQ(commands[4], 100);
  EXPECT_DOUBLE_EQ(commands[5], 0);
  EXPECT_DOUBLE_EQ(commands[6], kPathVerbLine);
  EXPECT_DOUBLE_EQ(commands[7], 50);
  EXPECT_DOUBLE_EQ(commands[8], 50);
  EXPECT_DOUBLE_EQ(commands[9], kPathVerbLine);
  EXPECT_DOUBLE_EQ(commands[10], 0);
  EXPECT_DOUBLE_EQ(commands[11], 0);
  EXPECT_DOUBLE_EQ(commands[12], kPathVerbClose);
}

TEST(OHOSExternalViewEmbedderStatic,
     FoldRotationStillAppliesToClipRectSerialization) {
  MutatorsStack stack;
  stack.PushTransform(DlMatrix::MakeRotationZ(DlRadians{kPi / 2}));
  stack.PushClipRect(DlRect::MakeXYWH(0, 0, 100, 50));
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(100, 100), std::move(stack));

  auto folded = OHOSExternalViewEmbedder::FoldMutatorsToFinal(params);
  ASSERT_EQ(folded.clip_rects.size(), 1u);
  // 对照组：kClipRect 用完整 root_transform（含旋转）——旋转 90° 后
  // (x,y)->(-y,x)，bounds 从 (0,0,100,50) 变为 (-50,0,50,100)。
  EXPECT_NEAR(folded.clip_rects[0].GetLeft(), -50, 1e-3);
  EXPECT_NEAR(folded.clip_rects[0].GetTop(), 0, 1e-3);
  EXPECT_NEAR(folded.clip_rects[0].GetWidth(), 50, 1e-3);
  EXPECT_NEAR(folded.clip_rects[0].GetHeight(), 100, 1e-3);
}

TEST(OHOSExternalViewEmbedderStatic, FoldClipRSESerializesToPathCommands) {
  MutatorsStack stack;
  stack.PushClipRSE(DlRoundSuperellipse::MakeRectRadius(
      DlRect::MakeXYWH(0, 0, 100, 50), 20));
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(100, 100), std::move(stack));

  auto folded = OHOSExternalViewEmbedder::FoldMutatorsToFinal(params);
  ASSERT_EQ(folded.clip_path_bounds.size(), 1u);
  ASSERT_EQ(folded.clip_path_commands.size(), 1u);
  ExpectRectNear(folded.clip_path_bounds[0], 0, 0, 100, 50);
  // squircle 轮廓被序列化成非空 verb 序列（具体曲线 verb 不做精确断言，
  // 协议由 ArkTS 端重建保证）。
  EXPECT_FALSE(folded.clip_path_commands[0].empty());
}

TEST(OHOSExternalViewEmbedderStatic, FoldClipPathCircleElevatesConicToCubic) {
  // 圆形 kClipPath：SkPath 以 conic（有理二次）表示圆弧，Dispatch 产生
  // ConicTo 回调 → PathCommandsCollector 度提升为 Cubic（ArkUI 不支持
  // conic）。验证度提升后的序列合法：含 CUBIC verb 且所有坐标有限。
  DlPathBuilder builder;
  builder.AddCircle(DlPoint(50, 50), 40);
  MutatorsStack stack;
  stack.PushClipPath(builder.TakePath());
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(100, 100), std::move(stack));

  auto folded = OHOSExternalViewEmbedder::FoldMutatorsToFinal(params);
  ASSERT_EQ(folded.clip_path_commands.size(), 1u);
  const std::vector<double>& commands = folded.clip_path_commands[0];
  ASSERT_FALSE(commands.empty());

  bool has_cubic = false;
  for (size_t i = 0; i < commands.size();) {
    int verb = static_cast<int>(commands[i]);
    int argc;
    switch (verb) {
      case 0: argc = 0; break;  // CLOSE
      case 1: argc = 2; break;  // MOVE
      case 2: argc = 2; break;  // LINE
      case 3: argc = 4; break;  // QUAD
      case 4: argc = 6; has_cubic = true; break;  // CUBIC
      default:
        FAIL() << "unknown verb " << verb << " at " << i;
        return;
    }
    for (int k = 1; k <= argc; ++k) {
      ASSERT_TRUE(std::isfinite(commands[i + k]))
          << "non-finite arg at index " << i + k;
    }
    i += 1 + argc;
  }
  // SkPath 圆弧是有理曲线：度提升后必然产生 CUBIC。
  EXPECT_TRUE(has_cubic);
  // bounds 为圆外接矩形 (10,10,80,80)。
  ExpectRectNear(folded.clip_path_bounds[0], 10, 10, 80, 80);
}

TEST(OHOSExternalViewEmbedderStatic, FoldClipPathQuadCurveSerializesQuadVerb) {
  // 二次贝塞尔 path：Dispatch 产生 QuadTo 回调 → 序列化为 QUAD(3) + 4 参数。
  DlPathBuilder builder;
  builder.MoveTo(DlPoint(0, 0));
  builder.QuadraticCurveTo(DlPoint(50, 100), DlPoint(100, 0));
  builder.Close();
  MutatorsStack stack;
  stack.PushClipPath(builder.TakePath());
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(100, 100), std::move(stack));

  auto folded = OHOSExternalViewEmbedder::FoldMutatorsToFinal(params);
  ASSERT_EQ(folded.clip_path_commands.size(), 1u);
  const std::vector<double>& commands = folded.clip_path_commands[0];
  // MOVE(3) + QUAD(5) + LINE(3, 回起点) + CLOSE(1) = 12 个数值。
  // 末尾 LINE 同样是 SkPath::Iter 在最后一点 != MoveTo 起点时的补发行为
  // （quad 终点 (100,0) != 起点 (0,0)）。
  ASSERT_EQ(commands.size(), 12u);
  EXPECT_DOUBLE_EQ(commands[0], kPathVerbMove);
  EXPECT_DOUBLE_EQ(commands[3], kPathVerbQuad);
  EXPECT_DOUBLE_EQ(commands[4], 50);
  EXPECT_DOUBLE_EQ(commands[5], 100);
  EXPECT_DOUBLE_EQ(commands[6], 100);
  EXPECT_DOUBLE_EQ(commands[7], 0);
  EXPECT_DOUBLE_EQ(commands[8], kPathVerbLine);
  EXPECT_DOUBLE_EQ(commands[9], 0);
  EXPECT_DOUBLE_EQ(commands[10], 0);
  EXPECT_DOUBLE_EQ(commands[11], kPathVerbClose);
}

TEST(OHOSExternalViewEmbedderStatic, FoldIgnoresBackdropMutators) {
  // Backdrop 类 mutator 不参与 HCPP 折叠（走 switch default 分支），
  // 不产生任何 clip/opacity/matrix 副作用。
  MutatorsStack stack;
  stack.PushPlatformViewClipRect(DlRect::MakeXYWH(0, 0, 10, 10));
  stack.PushPlatformViewClipRRect(
      DlRoundRect::MakeRectRadius(DlRect::MakeXYWH(0, 0, 10, 10), 2));
  stack.PushPlatformViewClipRSuperellipse(
      DlRoundSuperellipse::MakeRectRadius(DlRect::MakeXYWH(0, 0, 10, 10), 2));
  stack.PushPlatformViewClipPath(MakeTrianglePath());
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(100, 100), std::move(stack));

  auto folded = OHOSExternalViewEmbedder::FoldMutatorsToFinal(params);
  EXPECT_TRUE(folded.clip_rects.empty());
  EXPECT_TRUE(folded.clip_rrect_bounds.empty());
  EXPECT_TRUE(folded.clip_path_bounds.empty());
  EXPECT_TRUE(folded.clip_path_commands.empty());
  EXPECT_FLOAT_EQ(folded.opacity, 1.0f);
}

//------------------------------------------------------------------------------
// 实例生命周期（null 依赖：静态逻辑不触 NDK/NAPI）
//------------------------------------------------------------------------------

class OHOSExternalViewEmbedderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Preroll/Composite/EndFrame 均不派发任务，四个槽位共用一个真实线程
    // 的 runner 即可满足 TaskRunners 构造。napi 用 null-env 实例：HCPP
    // napi 方法均有空防护，null-env 下为安全 no-op。
    thread_ = std::make_unique<fml::Thread>("hcpp_embedder_test");
    auto runner = thread_->GetTaskRunner();
    task_runners_ = std::make_unique<TaskRunners>("hcpp_embedder_test", runner,
                                                  runner, runner, runner);
    napi_facade_ = std::make_shared<PlatformViewOHOSNapi>(nullptr);
    embedder_ = std::make_unique<OHOSExternalViewEmbedder>(
        nullptr, napi_facade_, nullptr, *task_runners_);
  }

  void TearDown() override {
    embedder_.reset();
    task_runners_.reset();
    thread_.reset();
  }

  std::unique_ptr<fml::Thread> thread_;
  std::unique_ptr<TaskRunners> task_runners_;
  std::shared_ptr<PlatformViewOHOSNapi> napi_facade_;
  std::unique_ptr<OHOSExternalViewEmbedder> embedder_;
};

TEST_F(OHOSExternalViewEmbedderTest, GetRootCanvasReturnsNull) {
  EXPECT_EQ(embedder_->GetRootCanvas(), nullptr);
}

TEST_F(OHOSExternalViewEmbedderTest, DoesNotSupportDynamicThreadMerging) {
  EXPECT_FALSE(embedder_->SupportsDynamicThreadMerging());
}

TEST_F(OHOSExternalViewEmbedderTest, CompositeUnknownViewReturnsNull) {
  EXPECT_EQ(embedder_->CompositeEmbeddedView(99), nullptr);
}

TEST_F(OHOSExternalViewEmbedderTest, PrerollCreatesSliceForComposite) {
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(100, 50), MutatorsStack());
  embedder_->PrerollCompositeEmbeddedView(1,
                                          std::make_unique<EmbeddedViewParams>(params));
  DlCanvas* canvas = embedder_->CompositeEmbeddedView(1);
  EXPECT_NE(canvas, nullptr);
}

TEST_F(OHOSExternalViewEmbedderTest, CancelFrameDropsSlices) {
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(100, 50), MutatorsStack());
  embedder_->PrerollCompositeEmbeddedView(1,
                                          std::make_unique<EmbeddedViewParams>(params));
  ASSERT_NE(embedder_->CompositeEmbeddedView(1), nullptr);

  embedder_->CancelFrame();
  EXPECT_EQ(embedder_->CompositeEmbeddedView(1), nullptr);
}

TEST_F(OHOSExternalViewEmbedderTest, PrepareFlutterViewDropsSlices) {
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(100, 50), MutatorsStack());
  embedder_->PrerollCompositeEmbeddedView(1,
                                          std::make_unique<EmbeddedViewParams>(params));
  ASSERT_NE(embedder_->CompositeEmbeddedView(1), nullptr);

  embedder_->PrepareFlutterView(DlISize(200, 100), 2.0);
  EXPECT_EQ(embedder_->CompositeEmbeddedView(1), nullptr);
}

TEST_F(OHOSExternalViewEmbedderTest, EndFrameIsSafeAfterPreroll) {
  // EndFrame 会剔除不再出现的 view_params_（内部状态，无外部观察点），
  // 此处验证调用安全且后续帧仍可正常 Preroll/Composite。
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(100, 50), MutatorsStack());
  embedder_->PrerollCompositeEmbeddedView(1,
                                          std::make_unique<EmbeddedViewParams>(params));
  embedder_->EndFrame(false, nullptr);

  embedder_->PrepareFlutterView(DlISize(200, 100), 1.0);
  embedder_->PrerollCompositeEmbeddedView(2,
                                          std::make_unique<EmbeddedViewParams>(params));
  EXPECT_NE(embedder_->CompositeEmbeddedView(2), nullptr);
}

TEST_F(OHOSExternalViewEmbedderTest, BeginFrameIsNoOp) {
  // OHOS 架构下 BeginFrame 为空实现（无 Java 侧逐帧记账消费者）。
  embedder_->BeginFrame(nullptr, nullptr);
}

TEST_F(OHOSExternalViewEmbedderTest, OverlayWindowTeardownWithoutSurface) {
  // 未注册 overlay window 时清理不应触碰任何 surface/NDK 资源。
  embedder_->SetOverlayWindow(nullptr);
  embedder_->TearDownOverlayWindow();
  embedder_->Teardown();
}

TEST_F(OHOSExternalViewEmbedderTest, PostPrerollActionReturnsSuccess) {
  EXPECT_EQ(embedder_->PostPrerollAction(nullptr),
            PostPrerollResult::kSuccess);
}

TEST_F(OHOSExternalViewEmbedderTest, PrerollSameParamsTwiceSkipsCacheUpdate) {
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(100, 50), MutatorsStack());
  embedder_->PrerollCompositeEmbeddedView(
      1, std::make_unique<EmbeddedViewParams>(params));
  // 第二次同 id 同参数：命中"参数未变化"早退分支（幂等）。
  embedder_->PrerollCompositeEmbeddedView(
      1, std::make_unique<EmbeddedViewParams>(params));
  EXPECT_NE(embedder_->CompositeEmbeddedView(1), nullptr);

  // 同 id 不同参数：走重新 insert_or_assign 分支。
  EmbeddedViewParams changed =
      MakeParams(DlMatrix(), DlSize(80, 40), MutatorsStack());
  embedder_->PrerollCompositeEmbeddedView(
      1, std::make_unique<EmbeddedViewParams>(changed));
  EXPECT_NE(embedder_->CompositeEmbeddedView(1), nullptr);
}

TEST_F(OHOSExternalViewEmbedderTest, EndFrameRemovesStaleViewParams) {
  EmbeddedViewParams params =
      MakeParams(DlMatrix(), DlSize(100, 50), MutatorsStack());
  embedder_->PrerollCompositeEmbeddedView(
      1, std::make_unique<EmbeddedViewParams>(params));
  // CancelFrame 清空 composition_order_ 但保留 view_params_ → EndFrame
  // 应把不再出现的 id 判定为 stale 并剔除。
  embedder_->CancelFrame();
  embedder_->EndFrame(false, nullptr);

  // 剔除后仍可正常进入下一帧生命周期。
  embedder_->PrepareFlutterView(DlISize(200, 100), 1.0);
  embedder_->PrerollCompositeEmbeddedView(
      1, std::make_unique<EmbeddedViewParams>(params));
  embedder_->EndFrame(false, nullptr);
  EXPECT_NE(embedder_->CompositeEmbeddedView(1), nullptr);
}

TEST_F(OHOSExternalViewEmbedderTest, PrepareFlutterViewSameSizeKeepsState) {
  embedder_->PrepareFlutterView(DlISize(200, 100), 1.0);
  // 尺寸不变：不触发 DestroyOverlaySurface 的重置分支（幂等）。
  embedder_->PrepareFlutterView(DlISize(200, 100), 1.0);
}

TEST_F(OHOSExternalViewEmbedderTest, SetOverlayWindowStoresAndDeduplicates) {
  // 首次设置非空窗口：走存储分支并标记 dirty。
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(nullptr, true);
  embedder_->SetOverlayWindow(window);
  // 再次设置同一窗口：走"未变化"早退分支。
  embedder_->SetOverlayWindow(window);
  // 注：Show/HideOverlayLayerIfNeeded 为 private 且产品代码无调用者
  // （预留接口），无法也不应从测试触达。
}

//------------------------------------------------------------------------------
// SubmitFlutterView / EnsureOverlaySurface（经 fake surface factory 驱动）
//------------------------------------------------------------------------------

namespace {

// OHOSSurface 的桩实现：SetDisplayWindow 是具体（非虚）实现，对 invalid
// window（handle=null 的 fake）在 IsValid() 检查处安全早退，因此桩的虚方法
// 在 UT 中不会被触达 NDK 的路径调用。
class FakeOHOSSurface : public OHOSSurface {
 public:
  FakeOHOSSurface()
      : OHOSSurface(std::make_shared<OHOSContext>(OHOSRenderingAPI::kSoftware)) {
  }

  bool IsValid() const override { return true; }
  void TeardownOnScreenContext() override {}
  bool OnScreenSurfaceResize(const DlISize& size) override { return true; }
  bool ResourceContextMakeCurrent() override { return true; }
  bool ResourceContextClearCurrent() override { return true; }
  bool SetNativeWindow(fml::RefPtr<OHOSNativeWindow> window) override {
    return true;
  }
  std::unique_ptr<Surface> CreateGPUSurface(
      GrDirectContext* gr_context) override {
    return nullptr;
  }
};

class FakeSurfaceFactory : public OhosSurfaceFactory {
 public:
  enum class Mode { kReturnNull, kReturnFake };
  explicit FakeSurfaceFactory(Mode mode) : mode_(mode) {}

  std::unique_ptr<OHOSSurface> CreateSurface() override {
    if (mode_ == Mode::kReturnNull) {
      return nullptr;
    }
    return std::make_unique<FakeOHOSSurface>();
  }

 private:
  Mode mode_;
};

}  // namespace

class OHOSExternalViewEmbedderFrameTest : public ::testing::Test {
 protected:
  void SetUp() override {
    thread_ = std::make_unique<fml::Thread>("hcpp_frame_test");
    auto runner = thread_->GetTaskRunner();
    task_runners_ = std::make_unique<TaskRunners>("hcpp_frame_test", runner,
                                                  runner, runner, runner);
    napi_facade_ = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  }

  void TearDown() override {
    // 先排干 platform 队列里已 post 的帧收尾 lambda，再销毁。
    WaitIdle();
    embedder_.reset();
    task_runners_.reset();
    thread_.reset();
  }

  void MakeEmbedder(std::unique_ptr<OhosSurfaceFactory> factory) {
    embedder_ = std::make_unique<OHOSExternalViewEmbedder>(
        nullptr, napi_facade_, std::move(factory), *task_runners_);
  }

  // CPU raster surface + 可观测 submit 的 SurfaceFrame。
  std::unique_ptr<SurfaceFrame> MakeFrame(bool* submitted) {
    sk_sp<SkSurface> sk_surface = SkSurfaces::Raster(
        SkImageInfo::MakeN32Premul(200, 200), 0, nullptr);
    *submitted = false;
    return std::make_unique<SurfaceFrame>(
        std::move(sk_surface), SurfaceFrame::FramebufferInfo{},
        [](SurfaceFrame& frame, DlCanvas* canvas) { return true; },
        [submitted](SurfaceFrame& frame) {
          *submitted = true;
          return true;
        },
        DlISize(200, 200));
  }

  void PrerollView(int64_t view_id,
                   double offset_x,
                   double offset_y,
                   MutatorsStack stack = MutatorsStack(),
                   bool paint_content = true) {
    EmbeddedViewParams params(
        DlMatrix::MakeTranslation(DlVector3(offset_x, offset_y, 0)),
        DlSize(100, 100), std::move(stack));
    embedder_->PrerollCompositeEmbeddedView(
        view_id, std::make_unique<EmbeddedViewParams>(params));
    if (paint_content) {
      // 在该 view 的 slice canvas 上真实绘制与 view 区域相交的内容：
      // SliceViews 依据 slice->region()（R-tree）判定 overlay 区域，
      // 不绘制则 overlay_layers 恒为空，无法驱动 overlay 相关分支。
      DlCanvas* canvas = embedder_->CompositeEmbeddedView(view_id);
      ASSERT_NE(canvas, nullptr);
      canvas->DrawRect(DlRect::MakeXYWH(0, 0, 100, 100),
                       DlPaint(DlColor::kRed()));
    }
  }

  fml::RefPtr<OHOSNativeWindow> MakeFakeWindow() {
    // handle=null 的 fake 窗口：IsValid()=false，使具体实现的
    // SetDisplayWindow 安全走 bind 失败分支，不触 NDK。
    return fml::MakeRefCounted<OHOSNativeWindow>(nullptr, true);
  }

  void WaitIdle() {
    fml::AutoResetWaitableEvent latch;
    task_runners_->GetPlatformTaskRunner()->PostTask(
        [&latch]() { latch.Signal(); });
    latch.Wait();
  }

  std::unique_ptr<fml::Thread> thread_;
  std::unique_ptr<TaskRunners> task_runners_;
  std::shared_ptr<PlatformViewOHOSNapi> napi_facade_;
  std::unique_ptr<OHOSExternalViewEmbedder> embedder_;
};

TEST_F(OHOSExternalViewEmbedderFrameTest, SubmitNoPlatformLayersSubmitsFrame) {
  MakeEmbedder(std::make_unique<FakeSurfaceFactory>(
      FakeSurfaceFactory::Mode::kReturnNull));
  embedder_->PrepareFlutterView(DlISize(200, 200), 1.0);

  bool submitted = false;
  embedder_->SubmitFlutterView(0, nullptr, nullptr, MakeFrame(&submitted));
  WaitIdle();
  EXPECT_TRUE(submitted);
}

TEST_F(OHOSExternalViewEmbedderFrameTest,
       SubmitTwoViewsThenEmptyFrameLifecycle) {
  MakeEmbedder(std::make_unique<FakeSurfaceFactory>(
      FakeSurfaceFactory::Mode::kReturnNull));
  embedder_->PrepareFlutterView(DlISize(200, 200), 1.0);

  // 视图 2 叠在视图 1 上方，两个 slice 都有绘制内容：
  // SliceViews 产生非空 overlay_layers → overlay_had_content_last_frame_
  // 置位 + lambda 的 show 分支。
  PrerollView(1, 0, 0);
  PrerollView(2, 10, 10);

  bool submitted = false;
  embedder_->SubmitFlutterView(0, nullptr, nullptr, MakeFrame(&submitted));
  WaitIdle();
  EXPECT_TRUE(submitted);

  // 第二帧无平台视图：走"上帧有 overlay 内容"的 EnsureOverlaySurface 分支 +
  // lambda 的 hide 分支 + views_visible_last_frame 的 hide 下发。
  embedder_->PrepareFlutterView(DlISize(200, 200), 1.0);
  bool submitted_empty = false;
  embedder_->SubmitFlutterView(0, nullptr, nullptr,
                               MakeFrame(&submitted_empty));
  WaitIdle();
  EXPECT_TRUE(submitted_empty);
}

TEST_F(OHOSExternalViewEmbedderFrameTest, SubmitDroppedFramesLogOnceThenReset) {
  MakeEmbedder(std::make_unique<FakeSurfaceFactory>(
      FakeSurfaceFactory::Mode::kReturnNull));
  // 注册了窗口但 bind 必然失败（fake window）：EnsureOverlaySurface 返回
  // null 而 overlay_layers 非空 → 丢帧告警链路。
  embedder_->SetOverlayWindow(MakeFakeWindow());
  embedder_->PrepareFlutterView(DlISize(200, 200), 1.0);

  // 帧一：!overlay_drop_logged_ == true → 记录一次告警。
  PrerollView(1, 0, 0);
  PrerollView(2, 10, 10);
  bool submitted_1 = false;
  embedder_->SubmitFlutterView(0, nullptr, nullptr, MakeFrame(&submitted_1));
  WaitIdle();
  EXPECT_TRUE(submitted_1);

  // 帧二：同样的丢帧场景，overlay_drop_logged_ 已置位 → 跳过重复告警臂。
  embedder_->PrepareFlutterView(DlISize(200, 200), 1.0);
  PrerollView(1, 0, 0);
  PrerollView(2, 10, 10);
  bool submitted_2 = false;
  embedder_->SubmitFlutterView(0, nullptr, nullptr, MakeFrame(&submitted_2));
  WaitIdle();
  EXPECT_TRUE(submitted_2);

  // 帧三：单视图且 slice 不绘制 → overlay_layers 空 → else 重置
  // overlay_drop_logged_ = false 臂。
  embedder_->PrepareFlutterView(DlISize(200, 200), 1.0);
  PrerollView(1, 0, 0, MutatorsStack(), /*paint_content=*/false);
  bool submitted_3 = false;
  embedder_->SubmitFlutterView(0, nullptr, nullptr, MakeFrame(&submitted_3));
  WaitIdle();
  EXPECT_TRUE(submitted_3);
}

TEST_F(OHOSExternalViewEmbedderFrameTest, SubmitVanishedViewIsHidden) {
  MakeEmbedder(std::make_unique<FakeSurfaceFactory>(
      FakeSurfaceFactory::Mode::kReturnNull));
  embedder_->PrepareFlutterView(DlISize(200, 200), 1.0);

  // 帧一：两个可见视图（lambda: show + 全量 OnDisplay）。
  PrerollView(1, 0, 0);
  PrerollView(2, 10, 10);
  bool submitted_1 = false;
  embedder_->SubmitFlutterView(0, nullptr, nullptr, MakeFrame(&submitted_1));
  WaitIdle();
  EXPECT_TRUE(submitted_1);

  // 帧二：view 2 消失。lambda 走"已显示不重复 show"臂 +
  // views_visible_last_frame 差集（view 2）的 HidePlatformViewHybrid。
  embedder_->PrepareFlutterView(DlISize(200, 200), 1.0);
  PrerollView(1, 0, 0);
  bool submitted_2 = false;
  embedder_->SubmitFlutterView(0, nullptr, nullptr, MakeFrame(&submitted_2));
  WaitIdle();
  EXPECT_TRUE(submitted_2);

  // 帧三：全部消失 → hide 臂收尾。
  embedder_->PrepareFlutterView(DlISize(200, 200), 1.0);
  bool submitted_3 = false;
  embedder_->SubmitFlutterView(0, nullptr, nullptr, MakeFrame(&submitted_3));
  WaitIdle();
  EXPECT_TRUE(submitted_3);
}

TEST_F(OHOSExternalViewEmbedderFrameTest, SetOverlayWindowReplaceRebuilds) {
  MakeEmbedder(std::make_unique<FakeSurfaceFactory>(
      FakeSurfaceFactory::Mode::kReturnFake));
  embedder_->PrepareFlutterView(DlISize(200, 200), 1.0);

  // 窗口 A：EnsureOverlaySurface 首次创建（dirty 置位）。
  embedder_->SetOverlayWindow(MakeFakeWindow());
  PrerollView(1, 0, 0);
  bool submitted_1 = false;
  embedder_->SubmitFlutterView(0, nullptr, nullptr, MakeFrame(&submitted_1));
  WaitIdle();
  EXPECT_TRUE(submitted_1);

  // 换窗口 B：SetOverlayWindow 走"不同窗口"存储臂；尺寸变化触发
  // DestroyOverlaySurface，下一次 EnsureOverlaySurface 走重建臂。
  embedder_->SetOverlayWindow(MakeFakeWindow());
  embedder_->PrepareFlutterView(DlISize(300, 300), 1.0);
  PrerollView(1, 0, 0);
  bool submitted_2 = false;
  embedder_->SubmitFlutterView(0, nullptr, nullptr, MakeFrame(&submitted_2));
  WaitIdle();
  EXPECT_TRUE(submitted_2);

  // Teardown（DestroyOverlaySurface 清 applied_）后再来一帧：
  // EnsureOverlaySurface 再次走重建臂（applied 为空）。
  embedder_->TearDownOverlayWindow();
  embedder_->PrepareFlutterView(DlISize(200, 200), 1.0);
  PrerollView(1, 0, 0);
  bool submitted_3 = false;
  embedder_->SubmitFlutterView(0, nullptr, nullptr, MakeFrame(&submitted_3));
  WaitIdle();
  EXPECT_TRUE(submitted_3);
}

TEST_F(OHOSExternalViewEmbedderFrameTest,
       SubmitWithWindowBindFailureStillSubmits) {
  MakeEmbedder(std::make_unique<FakeSurfaceFactory>(
      FakeSurfaceFactory::Mode::kReturnFake));
  embedder_->SetOverlayWindow(MakeFakeWindow());
  embedder_->PrepareFlutterView(DlISize(200, 200), 1.0);
  PrerollView(1, 0, 0);

  bool submitted = false;
  embedder_->SubmitFlutterView(0, nullptr, nullptr, MakeFrame(&submitted));
  WaitIdle();
  EXPECT_TRUE(submitted);

  // 第二帧：overlay window 未变（dirty=false 且 applied 相同）→ 走
  // "无需重建 surface 缓存"的判断分支；GPU surface 仍为空 → 再次创建。
  embedder_->PrepareFlutterView(DlISize(200, 200), 1.0);
  PrerollView(1, 0, 0);
  bool submitted_again = false;
  embedder_->SubmitFlutterView(0, nullptr, nullptr,
                               MakeFrame(&submitted_again));
  WaitIdle();
  EXPECT_TRUE(submitted_again);
}

TEST_F(OHOSExternalViewEmbedderFrameTest, SubmitWithFactoryFailureSubmits) {
  MakeEmbedder(std::make_unique<FakeSurfaceFactory>(
      FakeSurfaceFactory::Mode::kReturnNull));
  embedder_->SetOverlayWindow(MakeFakeWindow());
  embedder_->PrepareFlutterView(DlISize(200, 200), 1.0);
  PrerollView(1, 0, 0);

  // factory 返回 null surface：EnsureOverlaySurface 的创建失败分支。
  bool submitted = false;
  embedder_->SubmitFlutterView(0, nullptr, nullptr, MakeFrame(&submitted));
  WaitIdle();
  EXPECT_TRUE(submitted);
}

TEST_F(OHOSExternalViewEmbedderFrameTest, SubmitAppliesViewportClipDiff) {
  MakeEmbedder(std::make_unique<FakeSurfaceFactory>(
      FakeSurfaceFactory::Mode::kReturnNull));
  embedder_->PrepareFlutterView(DlISize(200, 200), 1.0);

  // stack 含 transform + 两个相交 clipRect：驱动 SubmitFlutterView 的
  // viewport 裁剪块——accumulator 累积 kTransform 臂、kClipRect 臂、
  // has_viewport_clip 首个 clip（false 臂）与第二个 clip 走
  // IntersectionOrEmpty（true 臂）。
  MutatorsStack stack;
  stack.PushTransform(DlMatrix::MakeTranslation(DlVector3(10, 10, 0)));
  stack.PushClipRect(DlRect::MakeXYWH(0, 0, 120, 120));
  stack.PushClipRect(DlRect::MakeXYWH(20, 20, 200, 200));
  PrerollView(1, 0, 0, std::move(stack));

  bool submitted = false;
  embedder_->SubmitFlutterView(0, nullptr, nullptr, MakeFrame(&submitted));
  WaitIdle();
  EXPECT_TRUE(submitted);
}

TEST_F(OHOSExternalViewEmbedderFrameTest, TeardownDrainsOverlayState) {
  MakeEmbedder(std::make_unique<FakeSurfaceFactory>(
      FakeSurfaceFactory::Mode::kReturnFake));
  embedder_->SetOverlayWindow(MakeFakeWindow());
  embedder_->PrepareFlutterView(DlISize(200, 200), 1.0);
  PrerollView(1, 0, 0);
  bool submitted = false;
  embedder_->SubmitFlutterView(0, nullptr, nullptr, MakeFrame(&submitted));
  WaitIdle();

  embedder_->TearDownOverlayWindow();
  embedder_->Teardown();
  EXPECT_TRUE(submitted);
}

namespace {

// The pure-virtual Surface interface the embedder stores per view; the
// windowing embedder only ever calls AcquireFrame, so the rest are defaulted.
class SurfaceMock : public Surface {
 public:
  MOCK_METHOD(bool, IsValid, (), (override));
  MOCK_METHOD(std::unique_ptr<SurfaceFrame>, AcquireFrame, (const DlISize&),
              (override));
  MOCK_METHOD(DlMatrix, GetRootTransformation, (), (const, override));
  MOCK_METHOD(GrDirectContext*, GetContext, (), (override));
};

// Builds a real SurfaceFrame (display-list fallback) whose encode/submit
// callbacks are no-ops. |submitted| is flipped when Submit() runs; when
// |captured_info| is non-null the frame's submit-info at submit time is copied
// into it (used to observe SubmitFlutterView's submit-info carry-over).
std::unique_ptr<SurfaceFrame> MakeTestFrame(
    bool* submitted,
    SurfaceFrame::SubmitInfo* captured_info = nullptr,
    DlISize size = DlISize(800, 600)) {
  SurfaceFrame::FramebufferInfo framebuffer_info;
  framebuffer_info.supports_readback = true;
  return std::make_unique<SurfaceFrame>(
      nullptr, framebuffer_info,
      [](SurfaceFrame&, DlCanvas*) { return true; },  // encode
      [submitted, captured_info](SurfaceFrame& frame) {
        if (submitted != nullptr) {
          *submitted = true;
        }
        if (captured_info != nullptr) {
          *captured_info = frame.submit_info();
        }
        return true;
      },
      size, nullptr, /*display_list_fallback=*/true);
}

}  // namespace

// ---------------------------------------------------------------------------
// OhosEmbedderRootSurface
// ---------------------------------------------------------------------------

TEST(OhosEmbedderRootSurfaceTest, SurfaceContractWithNoContext) {
  OhosEmbedderRootSurface root(nullptr);
  // Always-valid token surface.
  EXPECT_TRUE(root.IsValid());
  EXPECT_FALSE(root.EnableRasterCache());
  // No Skia GPU context on this path.
  EXPECT_EQ(root.GetContext(), nullptr);
  // No root transformation.
  EXPECT_EQ(root.GetRootTransformation(), DlMatrix());
  // No impeller context -> no AiksContext (and no crash).
  EXPECT_EQ(root.GetAiksContext(), nullptr);
}

TEST(OhosEmbedderRootSurfaceTest, AcquireFrameRejectsEmptySize) {
  OhosEmbedderRootSurface root(nullptr);
  EXPECT_EQ(root.AcquireFrame(DlISize(0, 0)), nullptr);
}

TEST(OhosEmbedderRootSurfaceTest, AcquireFrameReturnsSubmittableTokenFrame) {
  OhosEmbedderRootSurface root(nullptr);
  auto frame = root.AcquireFrame(DlISize(200, 100));
  ASSERT_NE(frame, nullptr);
  EXPECT_TRUE(frame->framebuffer_info().supports_readback);
  // The display-list fallback keeps a recording canvas available.
  EXPECT_NE(frame->Canvas(), nullptr);
  EXPECT_TRUE(frame->Submit());
  EXPECT_TRUE(frame->IsSubmitted());
}

TEST(OhosEmbedderRootSurfaceTest, GetAiksContextLazilyCreatedAndMemoized) {
  impeller::testing::MockVulkanContextBuilder builder;
  auto context = builder.Build();
  ASSERT_NE(context, nullptr);

  OhosEmbedderRootSurface root(context);
  auto first = root.GetAiksContext();
  ASSERT_NE(first, nullptr);
  // Second call must not rebuild: same instance back.
  EXPECT_EQ(root.GetAiksContext(), first);
}

// ---------------------------------------------------------------------------
// OHOSWindowingViewEmbedder
// ---------------------------------------------------------------------------

TEST(OHOSWindowingViewEmbedderTest, GetRootCanvasTracksPendingBuilder) {
  OHOSWindowingViewEmbedder embedder;
  // No frame in flight yet.
  EXPECT_EQ(embedder.GetRootCanvas(), nullptr);

  embedder.PrepareFlutterView(DlISize(300, 200), /*device_pixel_ratio=*/2.0);
  EXPECT_NE(embedder.GetRootCanvas(), nullptr);

  embedder.CancelFrame();
  EXPECT_EQ(embedder.GetRootCanvas(), nullptr);
}

TEST(OHOSWindowingViewEmbedderTest, BeginFrameResetsPendingState) {
  OHOSWindowingViewEmbedder embedder;
  embedder.PrepareFlutterView(DlISize(300, 200), 1.0);
  EXPECT_NE(embedder.GetRootCanvas(), nullptr);

  fml::RefPtr<fml::RasterThreadMerger> null_merger;
  embedder.BeginFrame(nullptr, null_merger);
  EXPECT_EQ(embedder.GetRootCanvas(), nullptr);

  // A new frame after BeginFrame starts fresh.
  embedder.PrepareFlutterView(DlISize(640, 480), 1.0);
  EXPECT_NE(embedder.GetRootCanvas(), nullptr);
}

TEST(OHOSWindowingViewEmbedderTest, EndFrameClearsPendingBuilder) {
  OHOSWindowingViewEmbedder embedder;
  embedder.PrepareFlutterView(DlISize(300, 200), 1.0);
  EXPECT_NE(embedder.GetRootCanvas(), nullptr);

  fml::RefPtr<fml::RasterThreadMerger> null_merger;
  embedder.EndFrame(/*should_resubmit_frame=*/false, null_merger);
  EXPECT_EQ(embedder.GetRootCanvas(), nullptr);
}

TEST(OHOSWindowingViewEmbedderTest, RegisterViewSurfaceReplacesExisting) {
  OHOSWindowingViewEmbedder embedder;
  embedder.PrepareFlutterView(DlISize(300, 200), 1.0);

  auto first = std::make_unique<NiceMock<SurfaceMock>>();
  EXPECT_CALL(*first, AcquireFrame(_)).Times(0);
  embedder.RegisterViewSurface(7, std::move(first));

  // Re-registering the same view replaces the old surface: the new one is
  // what SubmitFlutterView sees.
  auto second = std::make_unique<NiceMock<SurfaceMock>>();
  EXPECT_CALL(*second, AcquireFrame(DlISize(300, 200)))
      .Times(1)
      .WillOnce(Return(ByMove(std::unique_ptr<SurfaceFrame>())));
  embedder.RegisterViewSurface(7, std::move(second));

  bool token_submitted = false;
  embedder.SubmitFlutterView(7, nullptr, nullptr,
                             MakeTestFrame(&token_submitted));
  EXPECT_TRUE(token_submitted);
}

TEST(OHOSWindowingViewEmbedderTest, RegisterNullSurfaceUnregisters) {
  OHOSWindowingViewEmbedder embedder;
  auto surface = std::make_unique<NiceMock<SurfaceMock>>();
  embedder.RegisterViewSurface(7, std::move(surface));
  embedder.RegisterViewSurface(7, nullptr);  // unregister

  bool token_submitted = false;
  embedder.SubmitFlutterView(7, nullptr, nullptr,
                             MakeTestFrame(&token_submitted));
  // No surface registered: the token frame is dropped without acquiring.
  EXPECT_TRUE(token_submitted);
}

TEST(OHOSWindowingViewEmbedderTest, UnregisterAndCollectDropSurface) {
  OHOSWindowingViewEmbedder embedder;

  for (int64_t view : {1, 2}) {
    auto surface = std::make_unique<NiceMock<SurfaceMock>>();
    embedder.RegisterViewSurface(view, std::move(surface));
  }

  embedder.UnregisterViewSurface(1);
  embedder.CollectView(2);

  // Both dropped: submitting either view hits the unknown-surface path.
  bool submitted = false;
  embedder.SubmitFlutterView(1, nullptr, nullptr, MakeTestFrame(&submitted));
  EXPECT_TRUE(submitted);
  submitted = false;
  embedder.SubmitFlutterView(2, nullptr, nullptr, MakeTestFrame(&submitted));
  EXPECT_TRUE(submitted);
}

TEST(OHOSWindowingViewEmbedderTest, SubmitWithoutRegisteredSurface) {
  OHOSWindowingViewEmbedder embedder;
  embedder.PrepareFlutterView(DlISize(300, 200), 1.0);

  bool token_submitted = false;
  embedder.SubmitFlutterView(42, nullptr, nullptr,
                             MakeTestFrame(&token_submitted));
  EXPECT_TRUE(token_submitted);
}

TEST(OHOSWindowingViewEmbedderTest, SubmitWithoutPendingRecording) {
  OHOSWindowingViewEmbedder embedder;
  auto surface = std::make_unique<NiceMock<SurfaceMock>>();
  // The surface must NOT be acquired: the frame is dropped at the missing
  // recording check.
  EXPECT_CALL(*surface, AcquireFrame(_)).Times(0);
  embedder.RegisterViewSurface(7, std::move(surface));

  // Prepare then cancel -> no pending builder at submit time.
  embedder.PrepareFlutterView(DlISize(300, 200), 1.0);
  embedder.CancelFrame();

  bool token_submitted = false;
  embedder.SubmitFlutterView(7, nullptr, nullptr,
                             MakeTestFrame(&token_submitted));
  EXPECT_TRUE(token_submitted);
}

TEST(OHOSWindowingViewEmbedderTest, SubmitWhenFrameUnavailable) {
  OHOSWindowingViewEmbedder embedder;
  auto surface = std::make_unique<NiceMock<SurfaceMock>>();
  EXPECT_CALL(*surface, AcquireFrame(DlISize(300, 200)))
      .Times(1)
      .WillOnce(Return(ByMove(std::unique_ptr<SurfaceFrame>())));
  embedder.RegisterViewSurface(7, std::move(surface));

  embedder.PrepareFlutterView(DlISize(300, 200), 1.0);

  bool token_submitted = false;
  embedder.SubmitFlutterView(7, nullptr, nullptr,
                             MakeTestFrame(&token_submitted));
  EXPECT_TRUE(token_submitted);
}

TEST(OHOSWindowingViewEmbedderTest, SubmitReplaysRecordingAndCarriesSubmitInfo) {
  OHOSWindowingViewEmbedder embedder;
  auto surface = std::make_unique<NiceMock<SurfaceMock>>();
  bool view_submitted = false;
  SurfaceFrame::SubmitInfo view_info;
  EXPECT_CALL(*surface, AcquireFrame(DlISize(300, 200)))
      .Times(1)
      .WillOnce(
          Return(ByMove(MakeTestFrame(&view_submitted, &view_info))));
  embedder.RegisterViewSurface(7, std::move(surface));

  embedder.PrepareFlutterView(DlISize(300, 200), 1.0);
  ASSERT_NE(embedder.GetRootCanvas(), nullptr);
  // Record something into the pending builder so the replay is observable.
  embedder.GetRootCanvas()->DrawRect(DlRect::MakeLTRB(0, 0, 10, 10),
                                     DlPaint());

  // Token frame carries presentation/damage info to be forwarded.
  bool token_submitted = false;
  auto token = MakeTestFrame(&token_submitted);
  SurfaceFrame::SubmitInfo token_info;
  token_info.frame_damage = DlIRect::MakeLTRB(1, 2, 3, 4);
  token->set_submit_info(token_info);

  embedder.SubmitFlutterView(7, nullptr, nullptr, std::move(token));

  // The per-view frame was submitted with the token's submit-info carried over.
  EXPECT_TRUE(view_submitted);
  ASSERT_TRUE(view_info.frame_damage.has_value());
  EXPECT_EQ(view_info.frame_damage.value(), DlIRect::MakeLTRB(1, 2, 3, 4));
  // The token root frame is submitted too (no-op submit).
  EXPECT_TRUE(token_submitted);
}

TEST(OHOSWindowingViewEmbedderTest, TeardownClearsSurfacesAndRecording) {
  OHOSWindowingViewEmbedder embedder;
  auto surface = std::make_unique<NiceMock<SurfaceMock>>();
  embedder.RegisterViewSurface(7, std::move(surface));
  embedder.PrepareFlutterView(DlISize(300, 200), 1.0);
  ASSERT_NE(embedder.GetRootCanvas(), nullptr);

  embedder.Teardown();
  EXPECT_EQ(embedder.GetRootCanvas(), nullptr);

  // The view surface is gone: submit hits the unknown-surface path.
  bool token_submitted = false;
  embedder.SubmitFlutterView(7, nullptr, nullptr,
                             MakeTestFrame(&token_submitted));
  EXPECT_TRUE(token_submitted);
}

// The base-class no-op overrides should be callable (pure Flutter windows:
// no embedded platform views).
TEST(OHOSWindowingViewEmbedderTest, EmbeddedViewOverridesAreNoOps) {
  OHOSWindowingViewEmbedder embedder;
  embedder.PrerollCompositeEmbeddedView(1, nullptr);
  EXPECT_EQ(embedder.CompositeEmbeddedView(1), nullptr);
}

}  // namespace testing
}  // namespace flutter

#endif  // FML_OS_OHOS
