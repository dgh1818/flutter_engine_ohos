// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/entity/contents/filters/blend_filter_contents.h"

#include <array>
#include <memory>
#include <optional>

#include "impeller/base/strings.h"
#include "impeller/core/formats.h"
#include "impeller/entity/contents/anonymous_contents.h"
#include "impeller/entity/contents/content_context.h"
#include "impeller/entity/contents/contents.h"
#include "impeller/entity/contents/filters/color_filter_contents.h"
#include "impeller/entity/contents/filters/inputs/filter_input.h"
#include "impeller/entity/contents/solid_color_contents.h"
#include "impeller/entity/entity.h"
#include "impeller/geometry/color.h"
#include "impeller/renderer/render_pass.h"
#include "impeller/renderer/snapshot.h"

namespace impeller {

std::optional<BlendMode> InvertPorterDuffBlend(BlendMode blend_mode) {
  switch (blend_mode) {
    case BlendMode::kClear:
      return BlendMode::kClear;
    case BlendMode::kSource:
      return BlendMode::kDestination;
    case BlendMode::kDestination:
      return BlendMode::kSource;
    case BlendMode::kSourceOver:
      return BlendMode::kDestinationOver;
    case BlendMode::kDestinationOver:
      return BlendMode::kSourceOver;
    case BlendMode::kSourceIn:
      return BlendMode::kDestinationIn;
    case BlendMode::kDestinationIn:
      return BlendMode::kSourceIn;
    case BlendMode::kSourceOut:
      return BlendMode::kDestinationOut;
    case BlendMode::kDestinationOut:
      return BlendMode::kSourceOut;
    case BlendMode::kSourceATop:
      return BlendMode::kDestinationATop;
    case BlendMode::kDestinationATop:
      return BlendMode::kSourceATop;
    case BlendMode::kXor:
      return BlendMode::kXor;
    case BlendMode::kPlus:
      return BlendMode::kPlus;
    case BlendMode::kModulate:
      return BlendMode::kModulate;
    default:
      return std::nullopt;
  }
}

BlendFilterContents::BlendFilterContents() {
  SetBlendMode(BlendMode::kSourceOver);
}

BlendFilterContents::~BlendFilterContents() = default;

using PipelineProc = std::shared_ptr<Pipeline<PipelineDescriptor>> (
    ContentContext::*)(ContentContextOptions) const;

template <typename TPipeline>
static std::optional<Entity> AdvancedBlend(
    const FilterInput::Vector& inputs,
    const ContentContext& renderer,
    const Entity& entity,
    const Rect& coverage,
    BlendMode blend_mode,
    std::optional<Color> foreground_color,
    ColorFilterContents::AbsorbOpacity absorb_opacity,
    PipelineProc pipeline_proc,
    std::optional<Scalar> alpha) {
  using VS = typename TPipeline::VertexShader;
  using FS = typename TPipeline::FragmentShader;

  //----------------------------------------------------------------------------
  /// Handle inputs.
  ///

  const size_t total_inputs =
      inputs.size() + (foreground_color.has_value() ? 1 : 0);
  if (total_inputs < 2) {
    return std::nullopt;
  }

  auto dst_snapshot =
      inputs[0]->GetSnapshot("AdvancedBlend(Dst)", renderer, entity);
  if (!dst_snapshot.has_value()) {
    return std::nullopt;
  }
  auto maybe_dst_uvs = dst_snapshot->GetCoverageUVs(coverage);
  if (!maybe_dst_uvs.has_value()) {
    return std::nullopt;
  }
  auto dst_uvs = maybe_dst_uvs.value();

  std::optional<Snapshot> src_snapshot;
  std::array<Point, 4> src_uvs;
  if (!foreground_color.has_value()) {
    src_snapshot =
        inputs[1]->GetSnapshot("AdvancedBlend(Src)", renderer, entity);
    if (!src_snapshot.has_value()) {
      if (!dst_snapshot.has_value()) {
        return std::nullopt;
      }
      return Entity::FromSnapshot(dst_snapshot.value(), entity.GetBlendMode(),
                                  entity.GetClipDepth());
    }
    auto maybe_src_uvs = src_snapshot->GetCoverageUVs(coverage);
    if (!maybe_src_uvs.has_value()) {
      if (!dst_snapshot.has_value()) {
        return std::nullopt;
      }
      return Entity::FromSnapshot(dst_snapshot.value(), entity.GetBlendMode(),
                                  entity.GetClipDepth());
    }
    src_uvs = maybe_src_uvs.value();
  }

  Rect subpass_coverage = coverage;
  if (entity.GetContents()) {
    auto coverage_hint = entity.GetContents()->GetCoverageHint();

    if (coverage_hint.has_value()) {
      auto maybe_subpass_coverage =
          subpass_coverage.Intersection(*coverage_hint);
      if (!maybe_subpass_coverage.has_value()) {
        return std::nullopt;  // Nothing to render.
      }

      subpass_coverage = *maybe_subpass_coverage;
    }
  }

  //----------------------------------------------------------------------------
  /// Render to texture.
  ///

  ContentContext::SubpassCallback callback = [&](const ContentContext& renderer,
                                                 RenderPass& pass) {
    auto& host_buffer = renderer.GetTransientsBuffer();

    auto size = pass.GetRenderTargetSize();
    VertexBufferBuilder<typename VS::PerVertexData> vtx_builder;
    vtx_builder.AddVertices({
        {Point(0, 0), dst_uvs[0], src_uvs[0]},
        {Point(size.width, 0), dst_uvs[1], src_uvs[1]},
        {Point(0, size.height), dst_uvs[2], src_uvs[2]},
        {Point(size.width, size.height), dst_uvs[3], src_uvs[3]},
    });
    auto vtx_buffer = vtx_builder.CreateVertexBuffer(host_buffer);

    auto options = OptionsFromPass(pass);
    options.primitive_type = PrimitiveType::kTriangleStrip;
    options.blend_mode = BlendMode::kSource;
    std::shared_ptr<Pipeline<PipelineDescriptor>> pipeline =
        std::invoke(pipeline_proc, renderer, options);

#ifdef IMPELLER_DEBUG
    pass.SetCommandLabel(
        SPrintF("Advanced Blend Filter (%s)", BlendModeToString(blend_mode)));
#endif  // IMPELLER_DEBUG
    pass.SetVertexBuffer(std::move(vtx_buffer));
    pass.SetPipeline(pipeline);

    typename FS::BlendInfo blend_info;
    typename VS::FrameInfo frame_info;

    auto dst_sampler_descriptor = dst_snapshot->sampler_descriptor;
    if (renderer.GetDeviceCapabilities().SupportsDecalSamplerAddressMode()) {
      dst_sampler_descriptor.width_address_mode = SamplerAddressMode::kDecal;
      dst_sampler_descriptor.height_address_mode = SamplerAddressMode::kDecal;
    }
    const std::unique_ptr<const Sampler>& dst_sampler =
        renderer.GetContext()->GetSamplerLibrary()->GetSampler(
            dst_sampler_descriptor);
    FS::BindTextureSamplerDst(pass, dst_snapshot->texture, dst_sampler);
    frame_info.dst_y_coord_scale = dst_snapshot->texture->GetYCoordScale();
    blend_info.dst_input_alpha =
        absorb_opacity == ColorFilterContents::AbsorbOpacity::kYes
            ? dst_snapshot->opacity
            : 1.0;

    if (foreground_color.has_value()) {
      blend_info.color_factor = 1;
      blend_info.color = foreground_color.value();
      // This texture will not be sampled from due to the color factor. But
      // this is present so that validation doesn't trip on a missing
      // binding.
      FS::BindTextureSamplerSrc(pass, dst_snapshot->texture, dst_sampler);
    } else {
      auto src_sampler_descriptor = src_snapshot->sampler_descriptor;
      if (renderer.GetDeviceCapabilities().SupportsDecalSamplerAddressMode()) {
        src_sampler_descriptor.width_address_mode = SamplerAddressMode::kDecal;
        src_sampler_descriptor.height_address_mode = SamplerAddressMode::kDecal;
      }
      const std::unique_ptr<const Sampler>& src_sampler =
          renderer.GetContext()->GetSamplerLibrary()->GetSampler(
              src_sampler_descriptor);
      blend_info.color_factor = 0;
      blend_info.src_input_alpha = src_snapshot->opacity;
      FS::BindTextureSamplerSrc(pass, src_snapshot->texture, src_sampler);
      frame_info.src_y_coord_scale = src_snapshot->texture->GetYCoordScale();
    }
    auto blend_uniform = host_buffer.EmplaceUniform(blend_info);
    FS::BindBlendInfo(pass, blend_uniform);

    frame_info.mvp = pass.GetOrthographicTransform() *
                     Matrix::MakeTranslation(coverage.GetOrigin() -
                                             subpass_coverage.GetOrigin());

    auto uniform_view = host_buffer.EmplaceUniform(frame_info);
    VS::BindFrameInfo(pass, uniform_view);

    return pass.Draw().ok();
  };

  fml::StatusOr<RenderTarget> render_target = renderer.MakeSubpass(
      "Advanced Blend Filter", ISize(subpass_coverage.GetSize()), callback);
  if (!render_target.ok()) {
    return std::nullopt;
  }

  return Entity::FromSnapshot(
      Snapshot{
          .texture = render_target.value().GetRenderTargetTexture(),
          .transform = Matrix::MakeTranslation(subpass_coverage.GetOrigin()),
          // Since we absorbed the transform of the inputs and used the
          // respective snapshot sampling modes when blending, pass on
          // the default NN clamp sampler.
          .sampler_descriptor = {},
          .opacity = (absorb_opacity == ColorFilterContents::AbsorbOpacity::kYes
                          ? 1.0f
                          : dst_snapshot->opacity) *
                     alpha.value_or(1.0)},
      entity.GetBlendMode(), entity.GetClipDepth());
}

std::optional<Entity> BlendFilterContents::CreateForegroundAdvancedBlend(
    const std::shared_ptr<FilterInput>& input,
    const ContentContext& renderer,
    const Entity& entity,
    const Rect& coverage,
    Color foreground_color,
    BlendMode blend_mode,
    std::optional<Scalar> alpha,
    ColorFilterContents::AbsorbOpacity absorb_opacity) const {
  auto dst_snapshot =
      input->GetSnapshot("ForegroundAdvancedBlend", renderer, entity);
  if (!dst_snapshot.has_value()) {
    return std::nullopt;
  }

  RenderProc render_proc = [foreground_color, coverage, dst_snapshot,
                            blend_mode, alpha, absorb_opacity](
                               const ContentContext& renderer,
                               const Entity& entity, RenderPass& pass) -> bool {
    using VS = BlendScreenPipeline::VertexShader;
    using FS = BlendScreenPipeline::FragmentShader;

    auto& host_buffer = renderer.GetTransientsBuffer();

    auto maybe_dst_uvs = dst_snapshot->GetCoverageUVs(coverage);
    if (!maybe_dst_uvs.has_value()) {
      return false;
    }
    auto dst_uvs = maybe_dst_uvs.value();

    auto size = coverage.GetSize();
    auto origin = coverage.GetOrigin();
    VertexBufferBuilder<VS::PerVertexData> vtx_builder;
    vtx_builder.AddVertices({
        {origin, dst_uvs[0], dst_uvs[0]},
        {Point(origin.x + size.width, origin.y), dst_uvs[1], dst_uvs[1]},
        {Point(origin.x, origin.y + size.height), dst_uvs[2], dst_uvs[2]},
        {Point(origin.x + size.width, origin.y + size.height), dst_uvs[3],
         dst_uvs[3]},
    });
    auto vtx_buffer = vtx_builder.CreateVertexBuffer(host_buffer);

#ifdef IMPELLER_DEBUG
    pass.SetCommandLabel(SPrintF("Foreground Advanced Blend Filter (%s)",
                                 BlendModeToString(blend_mode)));
#endif  // IMPELLER_DEBUG
    pass.SetVertexBuffer(std::move(vtx_buffer));
    pass.SetStencilReference(entity.GetClipDepth());
    auto options = OptionsFromPass(pass);
    options.primitive_type = PrimitiveType::kTriangleStrip;

    switch (blend_mode) {
      case BlendMode::kScreen:
        pass.SetPipeline(renderer.GetBlendScreenPipeline(options));
        break;
      case BlendMode::kOverlay:
        pass.SetPipeline(renderer.GetBlendOverlayPipeline(options));
        break;
      case BlendMode::kDarken:
        pass.SetPipeline(renderer.GetBlendDarkenPipeline(options));
        break;
      case BlendMode::kLighten:
        pass.SetPipeline(renderer.GetBlendLightenPipeline(options));
        break;
      case BlendMode::kColorDodge:
        pass.SetPipeline(renderer.GetBlendColorDodgePipeline(options));
        break;
      case BlendMode::kColorBurn:
        pass.SetPipeline(renderer.GetBlendColorBurnPipeline(options));
        break;
      case BlendMode::kHardLight:
        pass.SetPipeline(renderer.GetBlendHardLightPipeline(options));
        break;
      case BlendMode::kSoftLight:
        pass.SetPipeline(renderer.GetBlendSoftLightPipeline(options));
        break;
      case BlendMode::kDifference:
        pass.SetPipeline(renderer.GetBlendDifferencePipeline(options));
        break;
      case BlendMode::kExclusion:
        pass.SetPipeline(renderer.GetBlendExclusionPipeline(options));
        break;
      case BlendMode::kMultiply:
        pass.SetPipeline(renderer.GetBlendMultiplyPipeline(options));
        break;
      case BlendMode::kHue:
        pass.SetPipeline(renderer.GetBlendHuePipeline(options));
        break;
      case BlendMode::kSaturation:
        pass.SetPipeline(renderer.GetBlendSaturationPipeline(options));
        break;
      case BlendMode::kColor:
        pass.SetPipeline(renderer.GetBlendColorPipeline(options));
        break;
      case BlendMode::kLuminosity:
        pass.SetPipeline(renderer.GetBlendLuminosityPipeline(options));
        break;
      default:
        return false;
    }

    FS::BlendInfo blend_info;
    VS::FrameInfo frame_info;

    auto dst_sampler_descriptor = dst_snapshot->sampler_descriptor;
    if (renderer.GetDeviceCapabilities().SupportsDecalSamplerAddressMode()) {
      dst_sampler_descriptor.width_address_mode = SamplerAddressMode::kDecal;
      dst_sampler_descriptor.height_address_mode = SamplerAddressMode::kDecal;
    }
    const std::unique_ptr<const Sampler>& dst_sampler =
        renderer.GetContext()->GetSamplerLibrary()->GetSampler(
            dst_sampler_descriptor);
    FS::BindTextureSamplerDst(pass, dst_snapshot->texture, dst_sampler);
    frame_info.dst_y_coord_scale = dst_snapshot->texture->GetYCoordScale();
    blend_info.dst_input_alpha =
        absorb_opacity == ColorFilterContents::AbsorbOpacity::kYes
            ? dst_snapshot->opacity * alpha.value_or(1.0)
            : 1.0;

    blend_info.color_factor = 1;
    blend_info.color = foreground_color;
    // This texture will not be sampled from due to the color factor. But
    // this is present so that validation doesn't trip on a missing
    // binding.
    FS::BindTextureSamplerSrc(pass, dst_snapshot->texture, dst_sampler);

    auto blend_uniform = host_buffer.EmplaceUniform(blend_info);
    FS::BindBlendInfo(pass, blend_uniform);

    frame_info.mvp = entity.GetShaderTransform(pass);

    auto uniform_view = host_buffer.EmplaceUniform(frame_info);
    VS::BindFrameInfo(pass, uniform_view);

    return pass.Draw().ok();
  };
  CoverageProc coverage_proc =
      [coverage](const Entity& entity) -> std::optional<Rect> {
    return coverage.TransformBounds(entity.GetTransform());
  };

  auto contents = AnonymousContents::Make(render_proc, coverage_proc);

  Entity sub_entity;
  sub_entity.SetContents(std::move(contents));
  sub_entity.SetClipDepth(entity.GetClipDepth());

  return sub_entity;
}

static std::optional<Entity> PipelineBlend(
    const FilterInput::Vector& inputs,
    const ContentContext& renderer,
    const Entity& entity,
    const Rect& coverage,
    BlendMode blend_mode,
    std::optional<Color> foreground_color,
    ColorFilterContents::AbsorbOpacity absorb_opacity,
    std::optional<Scalar> alpha) {
  using VS = BlendPipeline::VertexShader;
  using FS = BlendPipeline::FragmentShader;

  auto dst_snapshot =
      inputs[0]->GetSnapshot("PipelineBlend(Dst)", renderer, entity);
  if (!dst_snapshot.has_value()) {
    return std::nullopt;  // Nothing to render.
  }

  Rect subpass_coverage = coverage;
  if (entity.GetContents()) {
    auto coverage_hint = entity.GetContents()->GetCoverageHint();

    if (coverage_hint.has_value()) {
      auto maybe_subpass_coverage =
          subpass_coverage.Intersection(*coverage_hint);
      if (!maybe_subpass_coverage.has_value()) {
        return std::nullopt;  // Nothing to render.
      }

      subpass_coverage = *maybe_subpass_coverage;
    }
  }

  ContentContext::SubpassCallback callback = [&](const ContentContext& renderer,
                                                 RenderPass& pass) {
    auto& host_buffer = renderer.GetTransientsBuffer();

#ifdef IMPELLER_DEBUG
    pass.SetCommandLabel(
        SPrintF("Pipeline Blend Filter (%s)", BlendModeToString(blend_mode)));
#endif  // IMPELLER_DEBUG
    auto options = OptionsFromPass(pass);
    options.primitive_type = PrimitiveType::kTriangleStrip;

    auto add_blend_command = [&](std::optional<Snapshot> input) {
      if (!input.has_value()) {
        return false;
      }
      auto input_coverage = input->GetCoverage();
      if (!input_coverage.has_value()) {
        return false;
      }

      const std::unique_ptr<const Sampler>& sampler =
          renderer.GetContext()->GetSamplerLibrary()->GetSampler(
              input->sampler_descriptor);
      FS::BindTextureSamplerSrc(pass, input->texture, sampler);

      auto size = input->texture->GetSize();
      VertexBufferBuilder<VS::PerVertexData> vtx_builder;
      vtx_builder.AddVertices({
          {Point(0, 0), Point(0, 0)},
          {Point(size.width, 0), Point(1, 0)},
          {Point(0, size.height), Point(0, 1)},
          {Point(size.width, size.height), Point(1, 1)},
      });
      pass.SetVertexBuffer(vtx_builder.CreateVertexBuffer(host_buffer));

      VS::FrameInfo frame_info;
      frame_info.mvp = pass.GetOrthographicTransform() *
                       Matrix::MakeTranslation(-subpass_coverage.GetOrigin()) *
                       input->transform;
      frame_info.texture_sampler_y_coord_scale =
          input->texture->GetYCoordScale();

      FS::FragInfo frag_info;
      frag_info.input_alpha =
          absorb_opacity == ColorFilterContents::AbsorbOpacity::kYes
              ? input->opacity
              : 1.0;
      FS::BindFragInfo(pass, host_buffer.EmplaceUniform(frag_info));
      VS::BindFrameInfo(pass, host_buffer.EmplaceUniform(frame_info));

      return pass.Draw().ok();
    };

    // Draw the first texture using kSource.
    options.blend_mode = BlendMode::kSource;
    pass.SetPipeline(renderer.GetBlendPipeline(options));
    if (!add_blend_command(dst_snapshot)) {
      return true;
    }

    // Write subsequent textures using the selected blend mode.

    if (inputs.size() >= 2) {
      options.blend_mode = blend_mode;
      pass.SetPipeline(renderer.GetBlendPipeline(options));

      for (auto texture_i = inputs.begin() + 1; texture_i < inputs.end();
           texture_i++) {
        auto src_input = texture_i->get()->GetSnapshot("PipelineBlend(Src)",
                                                       renderer, entity);
        if (!add_blend_command(src_input)) {
          return true;
        }
      }
    }

    // If a foreground color is set, blend it in.

    if (foreground_color.has_value()) {
      auto contents = std::make_shared<SolidColorContents>();
      contents->SetGeometry(
          Geometry::MakeRect(Rect::MakeSize(pass.GetRenderTargetSize())));
      contents->SetColor(foreground_color.value());

      Entity foreground_entity;
      foreground_entity.SetBlendMode(blend_mode);
      foreground_entity.SetContents(contents);
      if (!foreground_entity.Render(renderer, pass)) {
        return false;
      }
    }

    return true;
  };

  fml::StatusOr<RenderTarget> render_target = renderer.MakeSubpass(
      "Pipeline Blend Filter", ISize(subpass_coverage.GetSize()), callback);

  if (!render_target.ok()) {
    return std::nullopt;
  }

  return Entity::FromSnapshot(
      Snapshot{
          .texture = render_target.value().GetRenderTargetTexture(),
          .transform = Matrix::MakeTranslation(subpass_coverage.GetOrigin()),
          // Since we absorbed the transform of the inputs and used the
          // respective snapshot sampling modes when blending, pass on
          // the default NN clamp sampler.
          .sampler_descriptor = {},
          .opacity = (absorb_opacity == ColorFilterContents::AbsorbOpacity::kYes
                          ? 1.0f
                          : dst_snapshot->opacity) *
                     alpha.value_or(1.0)},
      entity.GetBlendMode(), entity.GetClipDepth());
}

#define BLEND_CASE(mode)                                                      \
  case BlendMode::k##mode:                                                    \
    advanced_blend_proc_ =                                                    \
        [](const FilterInput::Vector& inputs, const ContentContext& renderer, \
           const Entity& entity, const Rect& coverage, BlendMode blend_mode,  \
           std::optional<Color> fg_color,                                     \
           ColorFilterContents::AbsorbOpacity absorb_opacity,                 \
           std::optional<Scalar> alpha) {                                     \
          PipelineProc p = &ContentContext::GetBlend##mode##Pipeline;         \
          return AdvancedBlend<Blend##mode##Pipeline>(                        \
              inputs, renderer, entity, coverage, blend_mode, fg_color,       \
              absorb_opacity, p, alpha);                                      \
        };                                                                    \
    break;

void BlendFilterContents::SetBlendMode(BlendMode blend_mode) {
  if (blend_mode > Entity::kLastAdvancedBlendMode) {
    VALIDATION_LOG << "Invalid blend mode " << static_cast<int>(blend_mode)
                   << " assigned to BlendFilterContents.";
  }

  blend_mode_ = blend_mode;

  if (blend_mode > Entity::kLastPipelineBlendMode) {
    switch (blend_mode) {
      BLEND_CASE(Screen)
      BLEND_CASE(Overlay)
      BLEND_CASE(Darken)
      BLEND_CASE(Lighten)
      BLEND_CASE(ColorDodge)
      BLEND_CASE(ColorBurn)
      BLEND_CASE(HardLight)
      BLEND_CASE(SoftLight)
      BLEND_CASE(Difference)
      BLEND_CASE(Exclusion)
      BLEND_CASE(Multiply)
      BLEND_CASE(Hue)
      BLEND_CASE(Saturation)
      BLEND_CASE(Color)
      BLEND_CASE(Luminosity)
      default:
        FML_UNREACHABLE();
    }
  }
}

void BlendFilterContents::SetForegroundColor(std::optional<Color> color) {
  foreground_color_ = color;
}

std::optional<Entity> BlendFilterContents::RenderFilter(
    const FilterInput::Vector& inputs,
    const ContentContext& renderer,
    const Entity& entity,
    const Matrix& effect_transform,
    const Rect& coverage,
    const std::optional<Rect>& coverage_hint) const {
  if (inputs.empty()) {
    return std::nullopt;
  }

  if (inputs.size() == 1 && !foreground_color_.has_value()) {
    // Nothing to blend.
    return PipelineBlend(inputs, renderer, entity, coverage, BlendMode::kSource,
                         std::nullopt, GetAbsorbOpacity(), GetAlpha());
  }

  if (blend_mode_ <= Entity::kLastPipelineBlendMode) {
    return PipelineBlend(inputs, renderer, entity, coverage, blend_mode_,
                         foreground_color_, GetAbsorbOpacity(), GetAlpha());
  }

  if (blend_mode_ <= Entity::kLastAdvancedBlendMode) {
    if (inputs.size() == 1 && foreground_color_.has_value() &&
        GetAbsorbOpacity() == ColorFilterContents::AbsorbOpacity::kYes) {
      return CreateForegroundAdvancedBlend(
          inputs[0], renderer, entity, coverage, foreground_color_.value(),
          blend_mode_, GetAlpha(), GetAbsorbOpacity());
    }
    return advanced_blend_proc_(inputs, renderer, entity, coverage, blend_mode_,
                                foreground_color_, GetAbsorbOpacity(),
                                GetAlpha());
  }

  FML_UNREACHABLE();
  std::optional<Entity> ret;
  return ret;
}

}  // namespace impeller
