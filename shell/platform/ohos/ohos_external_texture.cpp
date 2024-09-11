#include "ohos_external_texture.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <native_buffer/native_buffer.h>
#include <native_window/external_window.h>
#include <poll.h>
#include <cerrno>
#include <cstdint>
#include "include/core/SkM44.h"
#include "include/core/SkMatrix.h"

namespace flutter {

#define MAX_DELAYED_FRAMES 3

static int PixelMapToWindowFormat(PIXEL_FORMAT pixel_format) {
  switch (pixel_format) {
    case PIXEL_FORMAT_RGB_565:
      return NATIVEBUFFER_PIXEL_FMT_RGB_565;
    case PIXEL_FORMAT_RGBA_8888:
      return NATIVEBUFFER_PIXEL_FMT_RGBA_8888;
    case PIXEL_FORMAT_BGRA_8888:
      return NATIVEBUFFER_PIXEL_FMT_BGRA_8888;
    case PIXEL_FORMAT_RGB_888:
      return NATIVEBUFFER_PIXEL_FMT_RGB_888;
    case PIXEL_FORMAT_NV21:
      return NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP;
    case PIXEL_FORMAT_NV12:
      return NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP;
    case PIXEL_FORMAT_RGBA_1010102:
      return NATIVEBUFFER_PIXEL_FMT_RGBA_1010102;
    case PIXEL_FORMAT_YCBCR_P010:
      return NATIVEBUFFER_PIXEL_FMT_YCBCR_P010;
    case PIXEL_FORMAT_YCRCB_P010:
      return NATIVEBUFFER_PIXEL_FMT_YCRCB_P010;
    case PIXEL_FORMAT_ALPHA_8:
    case PIXEL_FORMAT_RGBA_F16:
    case PIXEL_FORMAT_UNKNOWN:
    default:
      // no support/unknow format: cannot copy
      return 0;
  }
  return 0;
}

static bool IsPixelMapYUVFormat(PIXEL_FORMAT format) {
  return format == PIXEL_FORMAT_NV21 || format == PIXEL_FORMAT_NV12 ||
         format == PIXEL_FORMAT_YCBCR_P010 || format == PIXEL_FORMAT_YCRCB_P010;
}

OHOSExternalTexture::OHOSExternalTexture(int64_t id,
                                         OH_OnFrameAvailableListener listener)
    : Texture(id), transform_(SkMatrix::I()), frame_listener_(listener) {
  native_image_source_ = OH_NativeImage_Create(0, GL_TEXTURE_EXTERNAL_OES);
  if (native_image_source_ == nullptr) {
    FML_LOG(ERROR) << "Error with OH_NativeImage_Create";
    return;
  }

  producer_nativewindow_ =
      OH_NativeImage_AcquireNativeWindow(native_image_source_);
  FML_LOG(INFO) << "OH_NativeImage_AcquireNativeWindow "
                << producer_nativewindow_;

  int ret = OH_NativeImage_SetOnFrameAvailableListener(native_image_source_,
                                                       frame_listener_);
  if (ret != 0) {
    FML_LOG(ERROR) << "Error with OH_NativeImage_SetOnFrameAvailableListener "
                   << ret;
  }
}

OHOSExternalTexture::~OHOSExternalTexture() {
  if (native_image_source_) {
    if (last_native_window_buffer_ != nullptr) {
      int ret = OH_NativeImage_ReleaseNativeWindowBuffer(
          native_image_source_, last_native_window_buffer_, last_fence_fd_);
      if (ret != 0) {
        FML_LOG(ERROR) << "~OHOSExternalTexture ReleaseNativeWindowBuffe(Get "
                          "Last) get err:"
                       << ret;
      }
    }
    FML_LOG(INFO) << "OH_NativeImage_Destroy " << native_image_source_;

    // producer_nativewindow_ will be destroy and
    // UnsetOnFrameAvailableListener will be invoked in OH_NativeImage_Destroy.
    OH_NativeImage_Destroy(&native_image_source_);
  }
  DestroyPixelMapBuffer();
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
    // draw_dl_image = old_dl_image_;
    // Temporary: Prevent freezes when using Vulkan to render camera data.
    if (last_native_window_buffer_) {
      OH_NativeBuffer* native_buffer;
      int ret = OH_NativeBuffer_FromNativeWindowBuffer(
          last_native_window_buffer_, &native_buffer);
      if (ret != 0) {
        FML_LOG(ERROR) << "OHOSExternalTextureGL get OH_NativeBuffer error:"
                       << ret;
      } else {
        // ensure buffer_id > 0 (may get seqNum = 0)
        uint32_t buffer_id = OH_NativeBuffer_GetSeqNum(native_buffer) + 1;
        draw_dl_image = CreateDlImage(context, bounds, buffer_id,
                                      last_native_window_buffer_);
      }
    }
  } else {
    // old_dl_image_ = draw_dl_image;
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
    if (last_fence_fd_ > 0) {
      close(last_fence_fd_);
    }
    SetGPUFence(&last_fence_fd_);
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
  FML_LOG(INFO) << " OHOSExternalTexture::MarkNewFrameAvailable avail-seq "
                << now_new_frame_seq_num_ << " paint-seq "
                << now_paint_frame_seq_num_;
  now_new_frame_seq_num_++;
  producer_has_frame_ = true;
  if (producer_nativewindow_ != nullptr) {
    int buffer_queue_size = 0;
    int ret = OH_NativeWindow_NativeWindowHandleOpt(
        producer_nativewindow_, GET_BUFFERQUEUE_SIZE, &buffer_queue_size);
    if (ret != 0) {
      return;
    }
    // Here we release the buffers in the buffer_queue to ensure there is always
    // space in the queue, preventing the producer side from stalling.
    int max_jank_frame = buffer_queue_size * 2 / 3;
    while (max_jank_frame > 1 &&
           now_new_frame_seq_num_ - now_paint_frame_seq_num_ > max_jank_frame) {
      OHNativeWindowBuffer* buffer = nullptr;
      int fence_fd;
      ret = OH_NativeImage_AcquireNativeWindowBuffer(native_image_source_,
                                                     &buffer, &fence_fd);
      if (buffer != nullptr && ret == 0) {
        FML_LOG(INFO) << "external_texture skip one frame(slow consumer): "
                      << buffer << " buffer_queue_size " << buffer_queue_size
                      << " max_jank_frame " << max_jank_frame;
        int ret = OH_NativeImage_ReleaseNativeWindowBuffer(native_image_source_,
                                                           buffer, fence_fd);
        if (ret != 0) {
          FML_LOG(ERROR) << "ReleaseNativeWindowBuffe get err:" << ret;
          OH_NativeWindow_DestroyNativeWindowBuffer(buffer);
          close(fence_fd);
        }
        now_paint_frame_seq_num_++;
      }
    }
  }
}

void OHOSExternalTexture::OnTextureUnregistered() {
  FML_LOG(INFO) << " OHOSExternalTexture::OnTextureUnregistered";
  // GPU resource must be release here (in raster thread).
  // Otherwise, gpu memory will leak.
  old_dl_image_.reset();
  image_lru_.Clear();
  if (last_fence_fd_ > 0) {
    close(last_fence_fd_);
    last_fence_fd_ = -1;
  }
  GPUResourceDestroy();
}

void OHOSExternalTexture::OnGrContextCreated() {
  FML_LOG(INFO) << " OHOSExternalTextureGL::OnGrContextCreated";
  state_ = AttachmentState::kUninitialized;
  // move SetOnFrame here to avoid MarkNewFrameAvailable being invoked when
  // rasterizer thread is in starting. Hit: MarkNewFrameAvailable will be
  // invoked in rasterizer thread.
  int ret = OH_NativeImage_SetOnFrameAvailableListener(native_image_source_,
                                                       frame_listener_);
  FML_LOG(INFO)
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
  FML_LOG(INFO) << "OH_NativeImage_GetSurfaceId " << producer_surface_id_;
  return producer_surface_id_;
}

uint64_t OHOSExternalTexture::GetProducerWindowId() {
  if (producer_nativewindow_ == nullptr) {
    producer_nativewindow_ =
        OH_NativeImage_AcquireNativeWindow(native_image_source_);
  }
  return (uint64_t)producer_nativewindow_;
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
  FML_LOG(INFO) << "SetPixelMapAsProducer";
  bool end_ret = true;
  if (!CreatePixelMapBuffer(pixelmap_info.width, pixelmap_info.height,
                            pixelmap_info.pixelFormat) ||
      !CopyDataToPixelMapBuffer(pixel_addr, pixelmap_info.width,
                                pixelmap_info.height, pixelmap_info.rowSize,
                                pixelmap_info.pixelFormat)) {
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
  if (!producer_has_frame_) {
    return pixelmap_buffer_;
  } else {
    if (pixelmap_buffer_ != nullptr) {
      DestroyPixelMapBuffer();
    }
  }
  OHNativeWindowBuffer* now_nw_buffer = nullptr;
  int ret = OH_NativeImage_AcquireNativeWindowBuffer(native_image_source_,
                                                     &now_nw_buffer, fence_fd);
  if (now_nw_buffer == nullptr || ret != 0) {
    return nullptr;
  }
  if (*fence_fd <= 0) {
    FML_DLOG(INFO) << "get not null native_window_buffer but invaild fence_fd: "
                   << *fence_fd;
  }

  if (last_native_window_buffer_ != nullptr) {
    ret = OH_NativeImage_ReleaseNativeWindowBuffer(
        native_image_source_, last_native_window_buffer_, last_fence_fd_);
    if (ret != 0) {
      FML_LOG(ERROR) << "OHOSExternalTexture ReleaseNativeWindowBuffe(Get "
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
    if (nw_buffer != nullptr && ret == 0) {
      FML_LOG(INFO) << "external_texture skip one frame: "
                    << last_native_window_buffer_ << " fence_fd "
                    << last_fence_fd_;
      int ret = OH_NativeImage_ReleaseNativeWindowBuffer(
          native_image_source_, last_native_window_buffer_, last_fence_fd_);
      if (ret != 0) {
        FML_LOG(ERROR) << "OHOSExternalTexture ReleaseNativeWindowBuffe(Get "
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

bool OHOSExternalTexture::SetProducerWindowSize(int width, int height) {
  if (producer_nativewindow_ == nullptr) {
    producer_nativewindow_ =
        OH_NativeImage_AcquireNativeWindow(native_image_source_);
    if (producer_nativewindow_ == nullptr) {
      FML_LOG(ERROR)
          << "OHOSExternalTexture OH_NativeImage_AcquireNativeWindow get null";
      return false;
    }
  }
  bool ret = SetWindowSize(producer_nativewindow_, width, height);
  if (ret) {
    producer_nativewindow_width_ = width;
    producer_nativewindow_height_ = height;
  }
  return ret;
}

bool OHOSExternalTexture::CreatePixelMapBuffer(int width,
                                               int height,
                                               int pixel_format) {
  int fence_fd = -1;
  DestroyPixelMapBuffer();

  int window_format = PixelMapToWindowFormat((PIXEL_FORMAT)pixel_format);

  if (width == 0 || height == 0 || window_format == 0) {
    return false;
  }

  OH_NativeImage* native_image =
      OH_NativeImage_Create(0, GL_TEXTURE_EXTERNAL_OES);
  if (native_image == nullptr) {
    FML_LOG(ERROR) << "Error with OH_NativeImage_Create";
    return false;
  }

  OHNativeWindow* native_window =
      OH_NativeImage_AcquireNativeWindow(native_image);

  if (native_window == nullptr ||
      !SetWindowSize(native_window, width, height) ||
      !SetWindowFormat(native_window, window_format) ||
      !SetNativeWindowCPUAccess(native_window, true)) {
    OH_NativeImage_Destroy(&native_image);
    pixelmap_buffer_ = nullptr;
    return false;
  }

  int ret = OH_NativeWindow_NativeWindowRequestBuffer(
      native_window, &pixelmap_buffer_, &fence_fd);
  if (ret != 0) {
    FML_LOG(ERROR) << "OHOSExternalTexture "
                      "OH_NativeWindow_NativeWindowRequestBuffer err:"
                   << ret;
    OH_NativeImage_Destroy(&native_image);
    pixelmap_buffer_ = nullptr;
    return false;
  }
  if (fence_fd > 0) {
    CPUWaitFence(fence_fd, -1);
    close(fence_fd);
  }
  pixelmap_native_image_ = native_image;

  return true;
}

void OHOSExternalTexture::DestroyPixelMapBuffer() {
  if (pixelmap_buffer_ != nullptr) {
    OHNativeWindow* native_window =
        OH_NativeImage_AcquireNativeWindow(pixelmap_native_image_);
    if (native_window != nullptr) {
      OH_NativeWindow_NativeWindowAbortBuffer(native_window, pixelmap_buffer_);
    } else {
      OH_NativeWindow_DestroyNativeWindowBuffer(pixelmap_buffer_);
    }
    OH_NativeImage_Destroy(&pixelmap_native_image_);
    pixelmap_buffer_ = nullptr;
    pixelmap_native_image_ = nullptr;
    FML_LOG(INFO) << "DestroyPixelMapBuffer";
  }
}

bool OHOSExternalTexture::SetWindowSize(OHNativeWindow* window,
                                        int width,
                                        int height) {
  int ret = OH_NativeWindow_NativeWindowHandleOpt(window, SET_BUFFER_GEOMETRY,
                                                  width, height);
  if (ret != 0) {
    FML_LOG(ERROR) << "SetWindowSize get err:" << ret << " window:" << window;
    return false;
  }
  return true;
}

bool OHOSExternalTexture::SetWindowFormat(OHNativeWindow* window, int format) {
  int ret = OH_NativeWindow_NativeWindowHandleOpt(window, SET_FORMAT, format);
  if (ret != 0) {
    int old_format;
    OH_NativeWindow_NativeWindowHandleOpt(window, GET_FORMAT, &old_format);
    FML_LOG(ERROR) << "window set format failed! ret:" << ret
                   << " old_format:" << old_format << " set_format:" << format;
    return false;
  }
  ret = OH_NativeWindow_NativeWindowHandleOpt(window, SET_STRIDE, 0x8);
  if (ret != 0) {
    FML_LOG(ERROR) << "NativeWindow set Stride failed:" << ret;
    return false;
  }
  return true;
}

bool OHOSExternalTexture::CPUWaitFence(int fence_fd, uint32_t timeout) {
  if (fence_fd <= 0) {
    return false;
  }
  struct pollfd poll_fd = {0};
  poll_fd.fd = fence_fd;
  poll_fd.events = POLLIN;

  int ret = -1;
  do {
    ret = poll(&poll_fd, 1, timeout);
  } while (ret == -1 && (errno == EINTR || errno == EAGAIN));

  if (ret == 0) {
    ret = -1;
    errno = ETIME;
  } else if (ret > 0) {
    ret = 0;
    if (poll_fd.revents & (POLLERR | POLLNVAL)) {
      ret = -1;
      errno = EINVAL;
    }
  }
  return ret < 0 ? -errno : 0;
}

bool OHOSExternalTexture::CopyDataToPixelMapBuffer(const unsigned char* src,
                                                   int width,
                                                   int height,
                                                   int stride,
                                                   int pixelmap_format) {
  if (src == nullptr || producer_nativewindow_ == nullptr ||
      pixelmap_buffer_ == nullptr) {
    return false;
  }
  OH_NativeBuffer_Config nativebuffer_config;

  // native_buffer ptr is convert from nativeWindowBuffer inner member, so it
  // don't need release
  OH_NativeBuffer* native_buffer;
  int ret =
      OH_NativeBuffer_FromNativeWindowBuffer(pixelmap_buffer_, &native_buffer);
  if (ret != 0) {
    FML_LOG(ERROR) << "OHOSExternalTextureGL get OH_NativeBuffer error:" << ret;
  }
  OH_NativeBuffer_GetConfig(native_buffer, &nativebuffer_config);
  if (nativebuffer_config.width != width ||
      nativebuffer_config.height != height) {
    FML_LOG(ERROR) << "OHOSExternalTextureGL "
                      "CopyDataToPixelMapBuffer size error: width "
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
  int real_height = height;
  if (IsPixelMapYUVFormat((PIXEL_FORMAT)pixelmap_format)) {
    // y is height, uv is height/2
    real_height = height + (height + 1) / 2;
  }
  for (int i = 0; i < real_height; i++) {
    memcpy(dst + i * nativebuffer_config.stride, src + i * stride,
           std::min(nativebuffer_config.stride, stride));
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

bool OHOSExternalTexture::SetNativeWindowCPUAccess(OHNativeWindow* window,
                                                   bool cpuAccess) {
  if (window == nullptr) {
    return false;
  }
  uint64_t usage = 0;
  int ret = OH_NativeWindow_NativeWindowHandleOpt(window, GET_USAGE, &usage);
  if (ret != 0) {
    FML_LOG(ERROR) << "OHOSExternalTexture "
                      "get window usage err:"
                   << ret << " window:" << window;
    return false;
  }
  usage |= NATIVEBUFFER_USAGE_HW_TEXTURE;
  if (cpuAccess) {
    usage |= NATIVEBUFFER_USAGE_CPU_READ;
    usage |= NATIVEBUFFER_USAGE_CPU_WRITE;
  } else {
    usage &= (~NATIVEBUFFER_USAGE_CPU_READ);
    usage &= (~NATIVEBUFFER_USAGE_CPU_WRITE);
  }
  ret = OH_NativeWindow_NativeWindowHandleOpt(window, SET_USAGE, usage);
  if (ret != 0) {
    FML_LOG(ERROR) << "OHOSExternalTexture "
                      "set window usage err:"
                   << ret << " window:" << window;
    return false;
  }
  return true;
}

void OHOSExternalTexture::GetNewTransformBound(SkM44& transform,
                                               SkRect& bounds) {
  if (pixelmap_buffer_ != nullptr) {
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