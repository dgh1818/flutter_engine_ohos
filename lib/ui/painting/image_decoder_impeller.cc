// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/lib/ui/painting/image_decoder_impeller.h"

#include <memory>

#include "flutter/fml/closure.h"
#include "flutter/fml/make_copyable.h"
#include "flutter/fml/trace_event.h"
#include "flutter/impeller/core/allocator.h"
#include "flutter/impeller/core/texture.h"
#include "flutter/impeller/display_list/dl_image_impeller.h"
#include "flutter/impeller/renderer/command_buffer.h"
#include "flutter/impeller/renderer/context.h"
#include "flutter/lib/ui/painting/image_decoder_skia.h"
#include "impeller/base/strings.h"
#include "impeller/display_list/skia_conversions.h"
#include "impeller/geometry/size.h"
#include "third_party/skia/include/core/SkAlphaType.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/core/SkColorType.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkMallocPixelRef.h"
#include "third_party/skia/include/core/SkPixelRef.h"
#include "third_party/skia/include/core/SkPixmap.h"
#include "third_party/skia/include/core/SkPoint.h"
#include "third_party/skia/include/core/SkSamplingOptions.h"
#include "third_party/skia/include/core/SkSize.h"

namespace flutter {

class MallocDeviceBuffer : public impeller::DeviceBuffer {
 public:
  explicit MallocDeviceBuffer(impeller::DeviceBufferDescriptor desc)
      : impeller::DeviceBuffer(desc) {
    data_ = static_cast<uint8_t*>(malloc(desc.size));
  }

  ~MallocDeviceBuffer() override { free(data_); }

  bool SetLabel(const std::string& label) override { return true; }

  bool SetLabel(const std::string& label, impeller::Range range) override {
    return true;
  }

  uint8_t* OnGetContents() const override { return data_; }

  bool OnCopyHostBuffer(const uint8_t* source,
                        impeller::Range source_range,
                        size_t offset) override {
    memcpy(data_ + offset, source + source_range.offset, source_range.length);
    return true;
  }

 private:
  uint8_t* data_;

  FML_DISALLOW_COPY_AND_ASSIGN(MallocDeviceBuffer);
};

#ifdef FML_OS_ANDROID
static constexpr bool kShouldUseMallocDeviceBuffer = true;
#else
static constexpr bool kShouldUseMallocDeviceBuffer = false;
#endif  // FML_OS_ANDROID

namespace {
/**
 *  Loads the gamut as a set of three points (triangle).
 */
void LoadGamut(SkPoint abc[3], const skcms_Matrix3x3& xyz) {
  // rx = rX / (rX + rY + rZ)
  // ry = rY / (rX + rY + rZ)
  // gx, gy, bx, and gy are calculated similarly.
  for (int index = 0; index < 3; index++) {
    float sum = xyz.vals[index][0] + xyz.vals[index][1] + xyz.vals[index][2];
    abc[index].fX = xyz.vals[index][0] / sum;
    abc[index].fY = xyz.vals[index][1] / sum;
  }
}

/**
 *  Calculates the area of the triangular gamut.
 */
float CalculateArea(SkPoint abc[3]) {
  const SkPoint& a = abc[0];
  const SkPoint& b = abc[1];
  const SkPoint& c = abc[2];
  return 0.5f * fabsf(a.fX * b.fY + b.fX * c.fY - a.fX * c.fY - c.fX * b.fY -
                      b.fX * a.fY);
}

// Note: This was calculated from SkColorSpace::MakeSRGB().
static constexpr float kSrgbGamutArea = 0.0982f;

// Source:
// https://source.chromium.org/chromium/_/skia/skia.git/+/393fb1ec80f41d8ad7d104921b6920e69749fda1:src/codec/SkAndroidCodec.cpp;l=67;drc=46572b4d445f41943059d0e377afc6d6748cd5ca;bpv=1;bpt=0
bool IsWideGamut(const SkColorSpace* color_space) {
  if (!color_space) {
    FML_DLOG(INFO) << "not wide gamut";
    return false;
  }
  FML_DLOG(INFO) << "is wide gamut";
  skcms_Matrix3x3 xyzd50;
  color_space->toXYZD50(&xyzd50);
  SkPoint rgb[3];
  LoadGamut(rgb, xyzd50);
  float area = CalculateArea(rgb);
  return area > kSrgbGamutArea;
}
}  // namespace

ImageDecoderImpeller::ImageDecoderImpeller(
    const TaskRunners& runners,
    std::shared_ptr<fml::ConcurrentTaskRunner> concurrent_task_runner,
    const fml::WeakPtr<IOManager>& io_manager,
    bool supports_wide_gamut,
    const std::shared_ptr<fml::SyncSwitch>& gpu_disabled_switch)
    : ImageDecoder(runners, std::move(concurrent_task_runner), io_manager),
      supports_wide_gamut_(supports_wide_gamut),
      gpu_disabled_switch_(gpu_disabled_switch) {
  std::promise<std::shared_ptr<impeller::Context>> context_promise;
  context_ = context_promise.get_future();
  runners_.GetIOTaskRunner()->PostTask(fml::MakeCopyable(
      [promise = std::move(context_promise), io_manager]() mutable {
        promise.set_value(io_manager ? io_manager->GetImpellerContext()
                                     : nullptr);
      }));
}

ImageDecoderImpeller::~ImageDecoderImpeller() = default;

static SkColorType ChooseCompatibleColorType(SkColorType type) {
  switch (type) {
    case kRGBA_F32_SkColorType:
      return kRGBA_F16_SkColorType;
    default:
      return kRGBA_8888_SkColorType;
  }
}

static SkAlphaType ChooseCompatibleAlphaType(SkAlphaType type) {
  return type;
}

DecompressResult ImageDecoderImpeller::DecompressTexture(
    ImageDescriptor* descriptor,
    SkISize target_size,
    impeller::ISize max_texture_size,
    bool supports_wide_gamut,
    const std::shared_ptr<impeller::Allocator>& allocator) {
  TRACE_EVENT0("impeller", __FUNCTION__);
  if (!descriptor) {
    std::string decode_error("Invalid descriptor (should never happen)");
    FML_DLOG(ERROR) << decode_error;
    return DecompressResult{.decode_error = decode_error};
  }

  target_size.set(std::min(static_cast<int32_t>(max_texture_size.width),
                           target_size.width()),
                  std::min(static_cast<int32_t>(max_texture_size.height),
                           target_size.height()));

  const SkISize source_size = descriptor->image_info().dimensions();
  auto decode_size = source_size;
  if (descriptor->is_compressed()) {
    decode_size = descriptor->get_scaled_dimensions(std::max(
        static_cast<float>(target_size.width()) / source_size.width(),
        static_cast<float>(target_size.height()) / source_size.height()));
  }

  //----------------------------------------------------------------------------
  /// 1. Decode the image.
  ///

  const auto base_image_info = descriptor->image_info();
  const bool is_wide_gamut =
      supports_wide_gamut ? IsWideGamut(base_image_info.colorSpace()) : false;
  SkAlphaType alpha_type =
      ChooseCompatibleAlphaType(base_image_info.alphaType());
  SkImageInfo image_info;
  if (is_wide_gamut ||
      base_image_info.colorType() == kRGBA_1010102_SkColorType) {
    SkColorType color_type = alpha_type == SkAlphaType::kPremul_SkAlphaType
                                 ? kRGBA_1010102_SkColorType
                                 : kRGBA_1010102_SkColorType;

    const skcms_Matrix3x3 dcip3_matrix = {{{1.2249f, -0.2247f, 0.000f},
                                           {-0.0420f, 1.0419f, 0.000f},
                                           {-0.0197f, -0.0786f, 1.0979f}}};

    const skcms_Matrix3x3 rec2020_matrix = {
        {{0.636958f, 0.144617f, 0.168881f},
         {0.262700f, 0.677998f, 0.059302f},
         {0.000000f, 0.028073f, 1.060985f}}};

    image_info =
        base_image_info.makeWH(decode_size.width(), decode_size.height())
            .makeColorType(color_type)
            .makeAlphaType(alpha_type)
            .makeColorSpace(
                SkColorSpace::MakeRGB(SkNamedTransferFn::kHLG, rec2020_matrix));

    FML_DLOG(INFO) << "is wide gamut";
    impeller::Context::hdr_ = 1;
  } else {
    image_info =
        base_image_info.makeWH(decode_size.width(), decode_size.height())
            .makeColorType(
                ChooseCompatibleColorType(base_image_info.colorType()))
            .makeAlphaType(alpha_type);

    FML_DLOG(INFO) << "not wide gamut";
    impeller::Context::hdr_ = 0;
  }

  const auto pixel_format =
      impeller::skia_conversions::ToPixelFormat(image_info.colorType());
  if (!pixel_format.has_value()) {
    std::string decode_error(impeller::SPrintF(
        "Codec pixel format is not supported (SkColorType=%d)",
        image_info.colorType()));
    FML_DLOG(ERROR) << decode_error;
    return DecompressResult{.decode_error = decode_error};
  }

  auto bitmap = std::make_shared<SkBitmap>();
  bitmap->setInfo(image_info);
  auto bitmap_allocator = std::make_shared<ImpellerAllocator>(allocator);

  if (descriptor->is_compressed()) {
    if (!bitmap->tryAllocPixels(bitmap_allocator.get())) {
      std::string decode_error(
          "Could not allocate intermediate for image decompression.");
      FML_DLOG(ERROR) << decode_error;
      return DecompressResult{.decode_error = decode_error};
    }
    // Decode the image into the image generator's closest supported size.
    if (!descriptor->get_pixels(bitmap->pixmap())) {
      std::string decode_error("Could not decompress image.");
      FML_DLOG(ERROR) << decode_error;
      return DecompressResult{.decode_error = decode_error};
    }
  } else {
    auto temp_bitmap = std::make_shared<SkBitmap>();
    temp_bitmap->setInfo(base_image_info);
    auto pixel_ref = SkMallocPixelRef::MakeWithData(
        base_image_info, descriptor->row_bytes(), descriptor->data());
    temp_bitmap->setPixelRef(pixel_ref, 0, 0);

    if (!bitmap->tryAllocPixels(bitmap_allocator.get())) {
      std::string decode_error(
          "Could not allocate intermediate for pixel conversion.");
      FML_DLOG(ERROR) << decode_error;
      return DecompressResult{.decode_error = decode_error};
    }
    temp_bitmap->readPixels(bitmap->pixmap());
    bitmap->setImmutable();
  }

  if (bitmap->dimensions() == target_size) {
    auto buffer = bitmap_allocator->GetDeviceBuffer();
    if (!buffer) {
      return DecompressResult{.decode_error = "Unable to get device buffer"};
    }
    return DecompressResult{.device_buffer = buffer,
                            .sk_bitmap = bitmap,
                            .image_info = bitmap->info()};
  }

  //----------------------------------------------------------------------------
  /// 2. If the decoded image isn't the requested target size, resize it.
  ///

  TRACE_EVENT0("impeller", "DecodeScale");
  const auto scaled_image_info = image_info.makeDimensions(target_size);

  auto scaled_bitmap = std::make_shared<SkBitmap>();
  auto scaled_allocator = std::make_shared<ImpellerAllocator>(allocator);
  scaled_bitmap->setInfo(scaled_image_info);
  if (!scaled_bitmap->tryAllocPixels(scaled_allocator.get())) {
    std::string decode_error(
        "Could not allocate scaled bitmap for image decompression.");
    FML_DLOG(ERROR) << decode_error;
    return DecompressResult{.decode_error = decode_error};
  }
  if (!bitmap->pixmap().scalePixels(
          scaled_bitmap->pixmap(),
          SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kNone))) {
    FML_LOG(ERROR) << "Could not scale decoded bitmap data.";
  }
  scaled_bitmap->setImmutable();

  auto buffer = scaled_allocator->GetDeviceBuffer();
  if (!buffer) {
    return DecompressResult{.decode_error = "Unable to get device buffer"};
  }
  return DecompressResult{.device_buffer = buffer,
                          .sk_bitmap = scaled_bitmap,
                          .image_info = scaled_bitmap->info()};
}

/// Only call this method if the GPU is available.
static std::pair<sk_sp<DlImage>, std::string> UnsafeUploadTextureToPrivate(
    const std::shared_ptr<impeller::Context>& context,
    const std::shared_ptr<impeller::DeviceBuffer>& buffer,
    const SkImageInfo& image_info) {
  const auto pixel_format =
      impeller::skia_conversions::ToPixelFormat(image_info.colorType());
  if (!pixel_format) {
    std::string decode_error(impeller::SPrintF(
        "Unsupported pixel format (SkColorType=%d)", image_info.colorType()));
    FML_DLOG(ERROR) << decode_error;
    return std::make_pair(nullptr, decode_error);
  }

  impeller::TextureDescriptor texture_descriptor;
  texture_descriptor.storage_mode = impeller::StorageMode::kDevicePrivate;
  texture_descriptor.format = pixel_format.value();
  texture_descriptor.size = {image_info.width(), image_info.height()};
  texture_descriptor.mip_count = texture_descriptor.size.MipCount();
  texture_descriptor.compression_type = impeller::CompressionType::kLossy;

  auto dest_texture =
      context->GetResourceAllocator()->CreateTexture(texture_descriptor);
  if (!dest_texture) {
    std::string decode_error("Could not create Impeller texture.");
    FML_DLOG(ERROR) << decode_error;
    return std::make_pair(nullptr, decode_error);
  }

  dest_texture->SetLabel(
      impeller::SPrintF("ui.Image(%p)", dest_texture.get()).c_str());

  auto command_buffer = context->CreateCommandBuffer();
  if (!command_buffer) {
    std::string decode_error(
        "Could not create command buffer for mipmap generation.");
    FML_DLOG(ERROR) << decode_error;
    return std::make_pair(nullptr, decode_error);
  }
  command_buffer->SetLabel("Mipmap Command Buffer");

  auto blit_pass = command_buffer->CreateBlitPass();
  if (!blit_pass) {
    std::string decode_error(
        "Could not create blit pass for mipmap generation.");
    FML_DLOG(ERROR) << decode_error;
    return std::make_pair(nullptr, decode_error);
  }
  blit_pass->SetLabel("Mipmap Blit Pass");
  blit_pass->AddCopy(impeller::DeviceBuffer::AsBufferView(buffer),
                     dest_texture);
  if (texture_descriptor.size.MipCount() > 1) {
    blit_pass->GenerateMipmap(dest_texture);
  }

  blit_pass->EncodeCommands(context->GetResourceAllocator());
  if (!context->GetCommandQueue()->Submit({command_buffer}).ok()) {
    std::string decode_error("Failed to submit blit pass command buffer.");
    FML_DLOG(ERROR) << decode_error;
    return std::make_pair(nullptr, decode_error);
  }

  context->DisposeThreadLocalCachedResources();

  return std::make_pair(
      impeller::DlImageImpeller::Make(std::move(dest_texture)), std::string());
}

std::pair<sk_sp<DlImage>, std::string>
ImageDecoderImpeller::UploadTextureToPrivate(
    const std::shared_ptr<impeller::Context>& context,
    const std::shared_ptr<impeller::DeviceBuffer>& buffer,
    const SkImageInfo& image_info,
    const std::shared_ptr<SkBitmap>& bitmap,
    const std::shared_ptr<fml::SyncSwitch>& gpu_disabled_switch) {
  TRACE_EVENT0("impeller", __FUNCTION__);
  if (!context) {
    return std::make_pair(nullptr, "No Impeller context is available");
  }
  if (!buffer) {
    return std::make_pair(nullptr, "No Impeller device buffer is available");
  }

  std::pair<sk_sp<DlImage>, std::string> result;
  gpu_disabled_switch->Execute(
      fml::SyncSwitch::Handlers()
          .SetIfFalse([&result, context, buffer, image_info] {
            result = UnsafeUploadTextureToPrivate(context, buffer, image_info);
          })
          .SetIfTrue([&result, context, bitmap, gpu_disabled_switch] {
            // create_mips is false because we already know the GPU is disabled.
            result =
                UploadTextureToStorage(context, bitmap, gpu_disabled_switch,
                                       impeller::StorageMode::kHostVisible,
                                       /*create_mips=*/false);
          }));
  return result;
}

std::pair<sk_sp<DlImage>, std::string>
ImageDecoderImpeller::UploadTextureToStorage(
    const std::shared_ptr<impeller::Context>& context,
    std::shared_ptr<SkBitmap> bitmap,
    const std::shared_ptr<fml::SyncSwitch>& gpu_disabled_switch,
    impeller::StorageMode storage_mode,
    bool create_mips) {
  TRACE_EVENT0("impeller", __FUNCTION__);
  if (!context) {
    return std::make_pair(nullptr, "No Impeller context is available");
  }
  if (!bitmap) {
    return std::make_pair(nullptr, "No texture bitmap is available");
  }
  const auto image_info = bitmap->info();
  const auto pixel_format =
      impeller::skia_conversions::ToPixelFormat(image_info.colorType());
  if (!pixel_format) {
    std::string decode_error(impeller::SPrintF(
        "Unsupported pixel format (SkColorType=%d)", image_info.colorType()));
    FML_DLOG(ERROR) << decode_error;
    return std::make_pair(nullptr, decode_error);
  }

  impeller::TextureDescriptor texture_descriptor;
  texture_descriptor.storage_mode = storage_mode;
  texture_descriptor.format = pixel_format.value();
  texture_descriptor.size = {image_info.width(), image_info.height()};
  texture_descriptor.mip_count =
      create_mips ? texture_descriptor.size.MipCount() : 1;

  auto texture =
      context->GetResourceAllocator()->CreateTexture(texture_descriptor);
  if (!texture) {
    std::string decode_error("Could not create Impeller texture.");
    FML_DLOG(ERROR) << decode_error;
    return std::make_pair(nullptr, decode_error);
  }

  auto mapping = std::make_shared<fml::NonOwnedMapping>(
      reinterpret_cast<const uint8_t*>(bitmap->getAddr(0, 0)),  // data
      texture_descriptor.GetByteSizeOfBaseMipLevel(),           // size
      [bitmap](auto, auto) mutable { bitmap.reset(); }          // proc
  );

  if (!texture->SetContents(mapping)) {
    std::string decode_error("Could not copy contents into Impeller texture.");
    FML_DLOG(ERROR) << decode_error;
    return std::make_pair(nullptr, decode_error);
  }

  texture->SetLabel(impeller::SPrintF("ui.Image(%p)", texture.get()).c_str());

  if (texture_descriptor.mip_count > 1u && create_mips) {
    std::optional<std::string> decode_error;

    // The only platform that needs mipmapping unconditionally is GL.
    // GL based platforms never disable GPU access.
    // This is only really needed for iOS.
    gpu_disabled_switch->Execute(fml::SyncSwitch::Handlers().SetIfFalse(
        [context, &texture, &decode_error] {
          auto command_buffer = context->CreateCommandBuffer();
          if (!command_buffer) {
            decode_error =
                "Could not create command buffer for mipmap generation.";
            return;
          }
          command_buffer->SetLabel("Mipmap Command Buffer");

          auto blit_pass = command_buffer->CreateBlitPass();
          if (!blit_pass) {
            decode_error = "Could not create blit pass for mipmap generation.";
            return;
          }
          blit_pass->SetLabel("Mipmap Blit Pass");
          blit_pass->GenerateMipmap(texture);

          blit_pass->EncodeCommands(context->GetResourceAllocator());
          if (!context->GetCommandQueue()->Submit({command_buffer}).ok()) {
            decode_error = "Failed to submit blit pass command buffer.";
            return;
          }
          command_buffer->WaitUntilScheduled();
        }));
    if (decode_error.has_value()) {
      FML_DLOG(ERROR) << decode_error.value();
      return std::make_pair(nullptr, decode_error.value());
    }
  }
  context->DisposeThreadLocalCachedResources();

  return std::make_pair(impeller::DlImageImpeller::Make(std::move(texture)),
                        std::string());
}

// |ImageDecoder|
void ImageDecoderImpeller::Decode(fml::RefPtr<ImageDescriptor> descriptor,
                                  uint32_t target_width,
                                  uint32_t target_height,
                                  const ImageResult& p_result) {
  FML_DCHECK(descriptor);
  FML_DCHECK(p_result);

  // Wrap the result callback so that it can be invoked from any thread.
  auto raw_descriptor = descriptor.get();
  raw_descriptor->AddRef();
  ImageResult result = [p_result,                               //
                        raw_descriptor,                         //
                        ui_runner = runners_.GetUITaskRunner()  //
  ](auto image, auto decode_error) {
    ui_runner->PostTask([raw_descriptor, p_result, image, decode_error]() {
      raw_descriptor->Release();
      p_result(std::move(image), decode_error);
    });
  };

  concurrent_task_runner_->PostTask(
      [raw_descriptor,                                            //
       context = context_.get(),                                  //
       target_size = SkISize::Make(target_width, target_height),  //
       io_runner = runners_.GetIOTaskRunner(),                    //
       result,
       supports_wide_gamut = supports_wide_gamut_,  //
       gpu_disabled_switch = gpu_disabled_switch_]() {
        if (!context) {
          result(nullptr, "No Impeller context is available");
          return;
        }
        auto max_size_supported =
            context->GetResourceAllocator()->GetMaxTextureSizeSupported();

        // Always decompress on the concurrent runner.
        auto bitmap_result = DecompressTexture(
            raw_descriptor, target_size, max_size_supported,
            supports_wide_gamut, context->GetResourceAllocator());
        if (!bitmap_result.device_buffer) {
          result(nullptr, bitmap_result.decode_error);
          return;
        }
        auto upload_texture_and_invoke_result = [result, context, bitmap_result,
                                                 gpu_disabled_switch]() {
          sk_sp<DlImage> image;
          std::string decode_error;
          if (!kShouldUseMallocDeviceBuffer &&
              context->GetCapabilities()->SupportsBufferToTextureBlits()) {
            std::tie(image, decode_error) = UploadTextureToPrivate(
                context, bitmap_result.device_buffer, bitmap_result.image_info,
                bitmap_result.sk_bitmap, gpu_disabled_switch);
            result(image, decode_error);
          } else {
            std::tie(image, decode_error) = UploadTextureToStorage(
                context, bitmap_result.sk_bitmap, gpu_disabled_switch,
                impeller::StorageMode::kDevicePrivate,
                /*create_mips=*/true);
            result(image, decode_error);
          }
        };
        // TODO(jonahwilliams):
        // https://github.com/flutter/flutter/issues/123058 Technically we
        // don't need to post tasks to the io runner, but without this
        // forced serialization we can end up overloading the GPU and/or
        // competing with raster workloads.
        io_runner->PostTask(upload_texture_and_invoke_result);
      });
}

ImpellerAllocator::ImpellerAllocator(
    std::shared_ptr<impeller::Allocator> allocator)
    : allocator_(std::move(allocator)) {}

std::shared_ptr<impeller::DeviceBuffer> ImpellerAllocator::GetDeviceBuffer()
    const {
  return buffer_;
}

bool ImpellerAllocator::allocPixelRef(SkBitmap* bitmap) {
  if (!bitmap) {
    return false;
  }
  const SkImageInfo& info = bitmap->info();
  if (kUnknown_SkColorType == info.colorType() || info.width() < 0 ||
      info.height() < 0 || !info.validRowBytes(bitmap->rowBytes())) {
    return false;
  }

  impeller::DeviceBufferDescriptor descriptor;
  descriptor.storage_mode = impeller::StorageMode::kHostVisible;
  descriptor.size = ((bitmap->height() - 1) * bitmap->rowBytes()) +
                    (bitmap->width() * bitmap->bytesPerPixel());

  std::shared_ptr<impeller::DeviceBuffer> device_buffer =
      kShouldUseMallocDeviceBuffer
          ? std::make_shared<MallocDeviceBuffer>(descriptor)
          : allocator_->CreateBuffer(descriptor);
  if (!device_buffer) {
    return false;
  }

  struct ImpellerPixelRef final : public SkPixelRef {
    ImpellerPixelRef(int w, int h, void* s, size_t r)
        : SkPixelRef(w, h, s, r) {}

    ~ImpellerPixelRef() override {}
  };

  auto pixel_ref = sk_sp<SkPixelRef>(
      new ImpellerPixelRef(info.width(), info.height(),
                           device_buffer->OnGetContents(), bitmap->rowBytes()));

  bitmap->setPixelRef(std::move(pixel_ref), 0, 0);
  buffer_ = std::move(device_buffer);
  return true;
}

}  // namespace flutter
