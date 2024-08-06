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
    : Texture(id), transform_(SkMatrix::I()), frame_listener_(listener) {
  native_image_source_ = OH_NativeImage_Create(0, GL_TEXTURE_EXTERNAL_OES);
  if (native_image_source_ == nullptr) {
    FML_LOG(ERROR) << "Error with OH_NativeImage_Create";
    return;
  }
  int ret = OH_NativeImage_SetOnFrameAvailableListener(native_image_source_,
                                                       frame_listener_);
  if (ret != 0) {
    FML_LOG(ERROR) << "Error with OH_NativeImage_SetOnFrameAvailableListener "
                   << ret;
  }
}

OHOSExternalTexture::~OHOSExternalTexture() {
  if (native_image_source_) {
    if (producer_nativewindow_buffer_ != nullptr) {
      OH_NativeWindow_NativeWindowAbortBuffer(producer_nativewindow_,
                                              producer_nativewindow_buffer_);
    }
    // producer_nativewindow_ will be destroy and
    // UnsetOnFrameAvailableListener will be invoked in OH_NativeImage_Destroy.
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
  if (freeze ||
      (draw_dl_image = GetNextDrawImage(context, bounds)) == nullptr) {
    draw_dl_image = old_dl_image_;
  } else {
    old_dl_image_ = draw_dl_image;
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
      if (last_fence_fd_ > 0) {
        close(last_fence_fd_);
      }
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

void OHOSExternalTexture::MarkNewFrameAvailable() {
  // NOOP.
  FML_LOG(INFO) << " OHOSExternalTexture::MarkNewFrameAvailable-- "
                << now_new_frame_seq_num_ << " " << now_paint_frame_seq_num_;
  // new_frame_ready_ = true;
  now_new_frame_seq_num_++;
}

void OHOSExternalTexture::OnTextureUnregistered() {
  FML_LOG(INFO) << " OHOSExternalTexture::OnTextureUnregistered";
}

void OHOSExternalTexture::OnGrContextCreated() {
  FML_LOG(INFO) << " OHOSExternalTextureGL::OnGrContextCreated";
  state_ = AttachmentState::kUninitialized;
  // move SetOnFrame here to avoid MarkNewFrameAvailable being invoked when
  // rasterizer thread is in starting. Hit: MarkNewFrameAvailable will be
  // invoked in rasterizer thread.
  int ret = OH_NativeImage_SetOnFrameAvailableListener(native_image_source_,
                                                       frame_listener_);
  FML_LOG(ERROR)
      << "OnGrContextCreated OH_NativeImage_SetOnFrameAvailableListener ";
  if (ret != 0) {
    FML_LOG(ERROR) << "Error with OH_NativeImage_SetOnFrameAvailableListener "
                   << ret;
  }
}

void OHOSExternalTexture::OnGrContextDestroyed() {
  if (state_ == AttachmentState::kAttached) {
    // move UnsetOnFrame here to avoid MarkNewFrameAvailable being invoked when
    // rasterizer thread exit. Hit: MarkNewFrameAvailable will be invoked in
    // rasterizer thread.
    FML_LOG(ERROR)
        << "OnGrContextDestroyed OH_NativeImage_UnsetOnFrameAvailableListener ";
    int ret =
        OH_NativeImage_UnsetOnFrameAvailableListener(native_image_source_);
    if (ret != 0) {
      FML_LOG(ERROR)
          << "Error with OH_NativeImage_UnsetOnFrameAvailableListener " << ret;
    }
    old_dl_image_.reset();
    image_lru_.Clear();
    if (last_fence_fd_ > 0) {
      close(last_fence_fd_);
      last_fence_fd_ = -1;
    }
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
  FML_LOG(ERROR) << "OH_NativeImage_GetSurfaceId " << producer_surface_id_;
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
  unsigned char* pixel_addr = nullptr;
  ret = OH_PixelMap_AccessPixels(pixelMap, (void**)&pixel_addr);
  if (ret != IMAGE_RESULT_SUCCESS || pixel_addr == nullptr) {
    FML_LOG(ERROR) << "OHOSExternalTextureGL OH_PixelMap_AccessPixels err:"
                   << ret;
    return false;
  }
  FML_LOG(ERROR) << "SetPixelMapAsProducer";
  bool end_ret = true;
  if (!CreateProducerNativeBuffer(pixelmap_info.width, pixelmap_info.height) ||
      !CopyDataToNativeBuffer(pixel_addr, pixelmap_info.width,
                              pixelmap_info.height, pixelmap_info.rowSize)) {
    FML_LOG(ERROR) << "SetPixelMapAsProducer not ok";
    end_ret = false;
  }

  ret = OH_PixelMap_UnAccessPixels(pixelMap);
  if (ret != IMAGE_RESULT_SUCCESS) {
    FML_LOG(FATAL) << "OHOSExternalTextureGL OH_PixelMap_UnAccessPixels err:"
                   << ret;
    return false;
  }

  return end_ret;
}

OHNativeWindowBuffer* OHOSExternalTexture::GetConsumerNativeBuffer(
    int* fence_fd) {
  if (producer_nativewindow_buffer_ == nullptr) {
    OHNativeWindowBuffer* now_nw_buffer = nullptr;
    int ret = OH_NativeImage_AcquireNativeWindowBuffer(
        native_image_source_, &now_nw_buffer, fence_fd);
    if (now_nw_buffer == nullptr || ret != 0) {
      return nullptr;
    }
    if (*fence_fd <= 0) {
      FML_DLOG(INFO)
          << "get not null native_window_buffer but invaild fence_fd: "
          << *fence_fd;
    }

    if (last_native_window_buffer_ != nullptr) {
      ret = OH_NativeImage_ReleaseNativeWindowBuffer(
          native_image_source_, last_native_window_buffer_, last_fence_fd_);
      if (ret != 0) {
        FML_LOG(ERROR) << "OHOSExternalTexture ReleaseConsumerNativeBuffer(Get "
                          "Last) get err:"
                       << ret;
      }
    }
    last_native_window_buffer_ = now_nw_buffer;
    last_fence_fd_ = *fence_fd;
    now_paint_frame_seq_num_++;
    while (now_paint_frame_seq_num_ + MAX_DELAYED_FRAMES <
           now_new_frame_seq_num_) {
      OHNativeWindowBuffer* nw_buffer = nullptr;
      int ret = OH_NativeImage_AcquireNativeWindowBuffer(native_image_source_,
                                                         &nw_buffer, fence_fd);
      if (nw_buffer != nullptr || ret != 0) {
        FML_LOG(ERROR) << "external_texture skip one frame: "
                       << last_native_window_buffer_ << " fence_fd "
                       << last_fence_fd_;
        int ret = OH_NativeImage_ReleaseNativeWindowBuffer(
            native_image_source_, last_native_window_buffer_, last_fence_fd_);
        if (ret != 0) {
          FML_LOG(ERROR)
              << "OHOSExternalTexture ReleaseConsumerNativeBuffer(Get "
                 "Last) get err:"
              << ret;
        }
        last_native_window_buffer_ = nw_buffer;
        last_fence_fd_ = *fence_fd;
        now_nw_buffer = nw_buffer;
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
    // Note that *fence_fd has same fd and will be close in WaitGPUFence, so we
    // let last_fence_fd_ be -1.
    last_fence_fd_ = -1;
    return now_nw_buffer;
  } else {
    *fence_fd = -1;
    return producer_nativewindow_buffer_;
  }
}

void OHOSExternalTexture::ReleaseConsumerNativeBuffer(
    OHNativeWindowBuffer* buffer,
    int fence_fd) {
  if (producer_nativewindow_buffer_ == nullptr) {
    int ret = OH_NativeImage_ReleaseNativeWindowBuffer(native_image_source_,
                                                       buffer, fence_fd);
    if (ret != 0) {
      FML_LOG(ERROR)
          << "OHOSExternalTexture ReleaseConsumerNativeBuffer get err:" << ret;
    }
  } else {
    return;
  }
}

sk_sp<flutter::DlImage> OHOSExternalTexture::GetNextDrawImage(
    PaintContext& context,
    const SkRect& bounds) {
  int fence_fd = -1;
  OHNativeWindowBuffer* native_widnow_buffer =
      GetConsumerNativeBuffer(&fence_fd);
  if (native_widnow_buffer == nullptr) {
    return nullptr;
  }

  OH_NativeBuffer* native_buffer;
  int ret = OH_NativeBuffer_FromNativeWindowBuffer(native_widnow_buffer,
                                                   &native_buffer);
  if (ret != 0) {
    FML_LOG(ERROR) << "OHOSExternalTextureGL get OH_NativeBuffer error:" << ret;
  }
  // ensure buffer_id > 0 (may get seqNum = 0)
  uint32_t buffer_id = OH_NativeBuffer_GetSeqNum(native_buffer) + 1;

  auto ret_image = image_lru_.FindImage(buffer_id);
  if (ret_image == nullptr) {
    ret_image = CreateDlImage(context, bounds, buffer_id, native_widnow_buffer);
  }
  if (ret_image == nullptr) {
    // set last_fence_fd_ so it can be close later.
    last_fence_fd_ = fence_fd;
  } else {
    // let gpu wait for the nativebuffer end use
    // fence_fd will be close in WaitGPUFence.
    WaitGPUFence(fence_fd);
  }
  return ret_image;
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

  if (producer_nativewindow_buffer_ != nullptr) {
    OH_NativeWindow_NativeWindowAbortBuffer(producer_nativewindow_,
                                            producer_nativewindow_buffer_);
    producer_nativewindow_buffer_ = nullptr;
  }

  int fence_fd = -1;
  if ((ret = OH_NativeWindow_NativeWindowRequestBuffer(
           producer_nativewindow_, &producer_nativewindow_buffer_,
           &fence_fd)) != 0) {
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
  if (producer_nativewindow_buffer_ == nullptr || src == nullptr) {
    return false;
  }
  OH_NativeBuffer_Config nativebuffer_config;

  // native_buffer ptr is convert from nativeWindowBuffer inner member, so it
  // don't need release
  OH_NativeBuffer* native_buffer;
  int ret = OH_NativeBuffer_FromNativeWindowBuffer(
      producer_nativewindow_buffer_, &native_buffer);
  if (ret != 0) {
    FML_LOG(ERROR) << "OHOSExternalTextureGL get OH_NativeBuffer error:" << ret;
  }
  OH_NativeBuffer_GetConfig(native_buffer, &nativebuffer_config);
  if (nativebuffer_config.width != width ||
      nativebuffer_config.height != height) {
    FML_LOG(ERROR) << "OHOSExternalTextureGL "
                      "CopyDataToNativeBuffer size error: width "
                   << width << "->" << nativebuffer_config.width << " height "
                   << nativebuffer_config.height << "->" << height;
    return false;
  }

  unsigned char* dst = nullptr;
  ret = OH_NativeBuffer_Map(native_buffer, (void**)&dst);
  if (ret != 0 || dst == nullptr) {
    FML_LOG(ERROR) << "OHOSExternalTextureGL "
                      "OH_NativeBuffer_Map err:"
                   << ret;
    return false;
  }
  for (int i = 0; i < height; i++) {
    memcpy(dst + i * nativebuffer_config.stride, src + i * stride, width * 4);
  }
  ret = OH_NativeBuffer_Unmap(native_buffer);
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
    transform.setIdentity();
    return;
  }

  // TransformMatrixV2 performs a vertical flip by default.
  // This occurs because it uses the left-bottom corner as (0,0)
  // and the transformation follows the original texture order.
  // However, the canvas' (0,0) are located at the
  // left-top corner. Therefore, we need to flip it back to correct the
  // orientation.
  float matrix[16];
  OH_NativeImage_GetTransformMatrixV2(native_image_source_, matrix);
  // for (int i = 0; i < 4; i++) {
  //   FML_LOG(INFO) << matrix[i*4+0] << " " << matrix[i*4+1]
  //                 << " " << matrix[i*4+2] << " "
  //                 << matrix[i*4+3];
  // }

  SkM44 transform_origin = SkM44::ColMajor(matrix);
  // Note that SkM44's constructor parameters are in row-major order.
  // Note that SkM44's operate * is multiplied in row-major order so we use
  // postConcat. This operate is to do a flip-V and translate it to origin
  // place.
  SkM44 transform_end = transform_origin.preConcat(
      SkM44(1, 0, 0, 0, 0, -1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1));
  if (transform_end.rc(0, 0) == 0 && transform_end.rc(1, 1) == 0) {
    // it has flip 90/270 and has no flip-v/flip-h -> rotate 180 degree
    int dx = transform_end.rc(0, 3);
    int dy = transform_end.rc(1, 3);
    if ((dx ^ dy) == 1) {
      transform_end = transform_end.preConcat(
          SkM44(-1, 0, 0, 1, 0, -1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1));
    }
  }

  // The transformation matrix is used to transform texture coordinates.
  // The range of texture coordinates is (0,1), so translation transformations
  // are either 0 or 1. Now, we are transforming vertex coordinates, so the
  // translation range must be adjusted to the width and height of the vertex
  // range.
  transform_end.setRC(0, 3, transform_end.rc(0, 3) * bounds.width());
  transform_end.setRC(1, 3, transform_end.rc(1, 3) * bounds.height());
  // for (int i = 0; i < 4; i++) {
  //   FML_LOG(INFO) << transform_end.rc(0, i) << " " << transform_end.rc(1, i)
  //                 << " " << transform_end.rc(2, i) << " "
  //                 << transform_end.rc(3, i);
  // }

  // If a 90-degree rotation is applied, the width and height of the vertex
  // range need to be swapped.
  if (matrix[0] == 0 && matrix[5] == 0) {
    bounds.setWH(bounds.height(), bounds.width());
  }

  transform = transform_end;
  return;
}

}  // namespace flutter