#include "ohos_external_texture.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <native_buffer/native_buffer.h>
#include <native_window/external_window.h>
#include "include/core/SkM44.h"
#include "include/core/SkMatrix.h"

namespace flutter {

#define MAX_DELAYED_FRAMES 3

OHOSExternalTexture::OHOSExternalTexture(int64_t id,
                                         OH_OnFrameAvailableListener listener)
    : Texture(id), transform_(SkMatrix::I()) {
  native_image_source_ = OH_NativeImage_Create(0, GL_TEXTURE_EXTERNAL_OES);
  if (native_image_source_ == nullptr) {
    FML_LOG(ERROR) << "Error with OH_NativeImage_Create";
    return;
  }

  int ret = OH_NativeImage_SetOnFrameAvailableListener(native_image_source_,
                                                       listener);
  if (ret != 0) {
    FML_LOG(ERROR) << "Error with OH_NativeImage_SetOnFrameAvailableListener "
                   << ret;
    return;
  }
}

OHOSExternalTexture::~OHOSExternalTexture() {
  if (native_image_source_) {
    if (producer_nativebuffer_ != nullptr) {
      OH_NativeWindow_NativeWindowAbortBuffer(producer_nativewindow_,
                                              producer_nativewindowbuffer_);
    }
    OH_NativeImage_Destroy(&native_image_source_);
  }
  return;
}

void OHOSExternalTexture::Paint(PaintContext& context,
                                const SkRect& bounds,
                                bool freeze,
                                DlImageSampling sampling) {
  if (state_ == AttachmentState::kDetached) {
    FML_LOG(INFO) << "paint state is kDetached";
    return;
  }

  sk_sp<flutter::DlImage> draw_dl_image;
  if (!freeze && new_frame_ready_) {
    draw_dl_image = GetNextDrawImage();
    new_frame_ready_ = false;
  } else {
    draw_dl_image = old_dl_image_;
  }
  if (draw_dl_image) {
    DlAutoCanvasRestore auto_restore(context.canvas, true);
    SkRect new_bounds = bounds;
    SkM44 new_transform;
    GetNewTransformBound(new_transform, new_bounds);
    context.canvas->Transform(new_transform);
    context.canvas->DrawImageRect(
        draw_dl_image,                                 // image
        SkRect::Make(draw_dl_image->bounds()),         // source rect
        new_bounds,                                    // destination rect
        sampling,                                      // sampling
        context.paint,                                 // paint
        flutter::DlCanvas::SrcRectConstraint::kStrict  // enforce edges
    );
    if (producer_nativewindow_buffer_ == nullptr) {
      SetGPUFence(&last_fence_fd_);
    }
    FML_LOG(INFO) << "Draw one dl image (" << draw_dl_image->bounds().width()
                  << "," << draw_dl_image->bounds().height() << ")->("
                  << bounds.width() << "," << bounds.height() << ")";
  } else {
    // ready for fix black background issue when external texture is not ready.
    // note: it may be incorrect because the background color should be set in
    // dart DlAutoCanvasRestore auto_restore(context.canvas, true); DlPaint
    // paint; paint.setColor(DlColor::kWhite());
    // context.canvas->DrawRect(bounds, paint);
    FML_LOG(INFO) << "No DlImage available for ImageExternalTexture to paint.";
  }
}

// Implementing flutter::Texture.
void OHOSExternalTexture::MarkNewFrameAvailable() {
  // NOOP.
  FML_LOG(INFO) << " OHOSExternalTexture::MarkNewFrameAvailable-- "
                << producer_nativewindow_width_ << " "
                << producer_nativewindow_height_;
  // new_frame_ready_ = true;
  now_new_frame_seq_num_++;
}

// Implementing flutter::Texture.
void OHOSExternalTexture::OnTextureUnregistered() {
  FML_LOG(INFO) << " OHOSExternalTextureGL::OnTextureUnregistered";
}

// Implementing flutter::ContextListener.
void OHOSExternalTexture::OnGrContextCreated() {
  FML_LOG(INFO) << " OHOSExternalTextureGL::OnGrContextCreated";
  state_ = AttachmentState::kUninitialized;
}

// Implementing flutter::ContextListener.
void OHOSExternalTexture::OnGrContextDestroyed() {
  if (state_ == AttachmentState::kAttached) {
    old_dl_image_.reset();
    // image_lru_.Clear();
    GPUResourceDestroy();
  }
  state_ = AttachmentState::kDetached;
}

uint64_t OHOSExternalTexture::GetProducerSurfaceId() {
  int ret =
      OH_NativeImage_GetSurfaceId(native_image_source_, &producer_surface_id_);
  if (ret != 0) {
    FML_LOG(ERROR) << "Error with OH_NativeImage_GetSurfaceId " << ret;
    return 0;
  }
  return producer_surface_id_;
}

bool OHOSExternalTexture::SetPixelMapAsProducer(NativePixelMap* pixelMap) {
  int32_t ret = -1;
  OhosPixelMapInfos pixelmap_info;
  ret = OH_PixelMap_GetImageInfo(pixelMap, &pixelmap_info);
  if (ret != 0) {
    FML_LOG(ERROR) << "OHOSExternalTextureGL OH_PixelMap_GetImageInfo err:"
                   << ret;
    return false;
  }
  char* pixel_addr = nullptr;
  ret = OH_PixelMap_AccessPixels(pixelMap, (void**)&pixel_addr);
  if (ret != IMAGE_RESULT_SUCCESS || pixel_addr == nullptr) {
    FML_DLOG(FATAL) << "OHOSExternalTextureGL OH_PixelMap_AccessPixels err:"
                    << ret;
    return false;
  }

  bool end_ret = true;
  if (!CreateProducerNativeBuffer(pixelmap_info.width, pixelmap_info.height) ||
      !CopyDataToNativeBuffer(pixel_addr, pixelmap_info.width,
                              pixelmap_info.height, pixelmap_info.rowSize)) {
    end_ret = false;
  }

  ret = OH_PixelMap_UnAccessPixels(pixelMap);
  if (ret != IMAGE_RESULT_SUCCESS) {
    FML_DLOG(FATAL) << "OHOSExternalTextureGL OH_PixelMap_UnAccessPixels err:"
                    << ret;
    return false;
  }

  return end_ret;
}

OH_NativeBuffer* OHOSExternalTexture::GetConsumerNativeBuffer(int* fence_fd) {
  if (producer_nativebuffer_ == nullptr) {
    OH_NativeBuffer* last_native_buffer =
        OH_NativeImage_AcquireConsumerNativeBuffer(native_image_source_,
                                                   fence_fd);
    if (last_native_buffer == nullptr) {
      return nullptr;
    }

    now_paint_frame_seq_num_++;
    int last_fence_fd = *fence_fd;
    while (now_paint_frame_seq_num_ + MAX_DELAYED_FRAMES <
           now_new_frame_seq_num_) {
      OHNativeWindowBuffer* nw_buffer =
          OH_NativeImage_AcquireConsumerNativeWindowBuffer(native_image_source_,
                                                           fence_fd);
      if (nw_buffer != nullptr) {
        FML_LOG(ERROR) << "external_texture skip one frame: " << nw_buffer
                       << " fence_fd " << last_fence_fd;
        int ret = OH_NativeImage_ReleaseConsumerNativeWindowBuffer(
            native_image_source_, last_nw_buffer, last_fence_fd);
        if (ret != 0) {
          FML_LOG(ERROR)
              << "OHOSExternalTexture ReleaseConsumerNativeBuffer(Get "
                 "Last) get err:"
              << ret;
        }
        last_nw_buffer = nw_buffer;
        last_fence_fd = *fence_fd;
        now_paint_frame_seq_num_++;
      } else {
        now_paint_frame_seq_num_ = (int64_t)now_new_frame_seq_num_;
        break;
      }
    }

    if (now_paint_frame_seq_num_ < now_new_frame_seq_num_ &&
        frame_listener_.onFrameAvailable != nullptr) {
      // Reschedule new frame (notify new texture in the next frame)
      now_new_frame_seq_num_--;
      frame_listener_.onFrameAvailable(frame_listener_.context);
    }
    *fence_fd = last_fence_fd;
    return last_native_buffer;
  } else {
    *fence_fd = -1;
    return producer_nativebuffer_;
  }
}

void OHOSExternalTexture::ReleaseConsumerNativeBuffer(OH_NativeBuffer* buffer,
                                                      int fence_fd) {
  if (producer_nativebuffer_ == nullptr) {
    int ret = OH_NativeImage_ReleaseConsumerNativeBuffer(native_image_source_,
                                                         buffer, fence_fd);
    if (ret != 0) {
      FML_LOG(ERROR)
          << "OHOSExternalTexture ReleaseConsumerNativeBuffer get err:" << ret;
    }
  } else {
    return;
  }
}

bool OHOSExternalTexture::CreateProducerNativeBuffer(int width, int height) {
  if (producer_nativewindow_ == nullptr) {
    producer_nativewindow_ =
        OH_NativeImage_AcquireNativeWindow(native_image_source_);
    if (producer_nativewindow_ == nullptr) {
      FML_LOG(ERROR)
          << "OHOSExternalTexture OH_NativeImage_AcquireNativeWindow get null";
      return false;
    }
  }

  // old producer_nativebuffer_ can use.
  if (producer_nativewindow_width_ == width &&
      producer_nativewindow_height_ == height) {
    return true;
  }

  // let's create one
  int code = SET_BUFFER_GEOMETRY;
  int ret = OH_NativeWindow_NativeWindowHandleOpt(
      producer_nativewindow_, SET_BUFFER_GEOMETRY, width, height);
  if (ret != 0) {
    FML_LOG(ERROR)
        << "OHOSExternalTextureGL OH_NativeWindow_NativeWindowHandleOpt "
           "set_buffer_size err:"
        << ret;
    return false;
  }

  if (producer_nativebuffer_ != nullptr) {
    OH_NativeWindow_NativeWindowAbortBuffer(producer_nativewindow_,
                                            producer_nativewindowbuffer_);
    producer_nativebuffer_ = nullptr;
    producer_nativewindowbuffer_ = nullptr;
  }

  int fence_fd = -1;
  if ((ret = OH_NativeWindow_NativeWindowRequestBuffer(
           producer_nativewindow_, &producer_nativewindowbuffer_, &fence_fd)) !=
          0 ||
      (ret = OH_NativeBuffer_FromNativeWindowBuffer(
           producer_nativewindowbuffer_, &producer_nativebuffer_)) != 0) {
    FML_LOG(ERROR) << "OHOSExternalTextureGL "
                      "OH_NativeWindow_NativeWindowRequestBuffer err:"
                   << ret;
    return false;
  }
  producer_nativewindow_width_ = width;
  producer_nativewindow_height_ = height;
  return true;
}

bool OHOSExternalTexture::CopyDataToNativeBuffer(const unsigned char* src,
                                                 int width,
                                                 int height,
                                                 int stride) {
  if (producer_nativebuffer_ == nullptr || src == nullptr) {
    return false;
  }
  OH_NativeBuffer_Config nativebuffer_config;
  OH_NativeBuffer_GetConfig(producer_nativebuffer_, &nativebuffer_config);
  if (nativebuffer_config.width != width ||
      nativebuffer_config.height != height) {
    FML_LOG(ERROR) << "OHOSExternalTextureGL "
                      "CopyDataToNativeBuffer size error: width "
                   << width << "->" << nativebuffer_config.width << " height "
                   << nativebuffer_config.height << "->" << height;
    return false;
  }

  char* dst = nullptr;
  int ret = OH_NativeBuffer_Map(producer_nativebuffer_, (void**)&dst);
  if (ret != 0 || dst == nullptr) {
    FML_LOG(ERROR) << "OHOSExternalTextureGL "
                      "OH_NativeBuffer_Map err:"
                   << ret;
    return false;
  }
  for (int i = 0; i < height; i++) {
    memcpy(dst + i * nativebuffer_config.stride, src + i * stride, width * 4);
  }
  ret = OH_NativeBuffer_Unmap(producer_nativebuffer_);
  if (ret != 0 || dst == nullptr) {
    FML_LOG(ERROR) << "OHOSExternalTextureGL "
                      "OH_NativeBuffer_Unmap err:"
                   << ret;
    return false;
  }

  return true;
}

void OHOSExternalTexture::GetNewTransformBound(SkM44& transform,
                                               SkRect& bounds) {
  if (producer_nativewindow_buffer_ != nullptr) {
    transform = transform.setIdentity();
    return;
  }
  // Ohos's NativeBuffer transform matrix operates on the data center point,
  // while the texture's (0,0) coordinate is not the center. Therefore, we first
  // translate (0,0) to the center point, apply the NativeBuffer transform,
  // and then translate it back. This sequence of steps ensures
  // the correct rotation. However, the canvas transform operates on the
  // vertices(not the texture), so we invert the transform to achieve the
  // opposite effect. In the end, rotating the vertices gives us the rotated
  // texture, but the vertices cannot change positions, so we use the original
  // transform to get the new bounds.
  float matrix[16];
  OH_NativeImage_GetTransformMatrix(native_image_source_, matrix);
  SkM44 transform_center =
      SkM44::Translate(bounds.centerX(), bounds.centerY(), 0);
  SkM44 transform_back =
      SkM44::Translate(-bounds.centerX(), -bounds.centerY(), 0);
  SkM44 transform_origin = SkM44::RowMajor(matrix);
  SkM44 transform_end = transform_center * transform_origin * transform_back;

  SkM44 transform_inverted;
  if (!transform_end.invert(&transform_inverted)) {
    FML_LOG(ERROR) << "Invalid (not invertable) transformation matrix";
  }

  transform = transform_inverted;
  transform_end.asM33().mapRect(&bounds);
  return;
}

}  // namespace flutter