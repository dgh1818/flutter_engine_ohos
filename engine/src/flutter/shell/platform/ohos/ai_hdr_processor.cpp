#include "flutter/shell/platform/ohos/ai_hdr_processor.h"

#include <unistd.h>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>

#include <multimedia/image_framework/image/image_common.h>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <multimedia/video_processing_engine/image_processing.h>
#include <native_buffer/native_buffer.h>
#include <native_color_space_manager/native_color_space_manager.h>

#include "fml/logging.h"

namespace ai_hdr {
namespace {

// The feature target: decode SDR pixels, hand them to the vendor VPE, get
// single-layer HDR back in a format the Flutter texture importer accepts.
constexpr ColorSpaceName kSourceColorSpace = SRGB;
constexpr ColorSpaceName kTargetColorSpace = BT2020_HLG;
constexpr int32_t kSourcePixelFormat = PIXEL_FORMAT_RGBA_8888;
constexpr int32_t kTargetPixelFormat = PIXEL_FORMAT_RGBA_1010102;
constexpr OH_Pixelmap_HdrMetadataType kSourceMetadataType =
    HDR_METADATA_TYPE_NONE;
constexpr OH_Pixelmap_HdrMetadataType kTargetMetadataType =
    HDR_METADATA_TYPE_ALTERNATE;

std::once_flag g_probe_once;
bool g_sdr_to_hlg_supported = false;

// Concurrent VPE conversion cap: 1.  The VPE service intermittently rejects
// calls under concurrent load (29200005/29200001 on identical inputs that
// succeed when retried), so conversions are serialized outright; callers
// queue on the slot instead.
constexpr int kMaxVpeInFlight = 1;
int g_vpe_in_flight = 0;
std::mutex g_vpe_mutex;
std::condition_variable g_vpe_cv;

// RAII slot: blocks the calling worker until a VPE slot is free.
class VpeSlot {
 public:
  VpeSlot() {
    std::unique_lock<std::mutex> lock(g_vpe_mutex);
    g_vpe_cv.wait(lock, [] { return g_vpe_in_flight < kMaxVpeInFlight; });
    g_vpe_in_flight++;
  }
  ~VpeSlot() {
    {
      std::lock_guard<std::mutex> lock(g_vpe_mutex);
      g_vpe_in_flight--;
    }
    g_vpe_cv.notify_all();
  }
  VpeSlot(const VpeSlot&) = delete;
  VpeSlot& operator=(const VpeSlot&) = delete;
};

ImageProcessing_ColorSpaceInfo MakeInfo(int32_t metadata_type,
                                        ColorSpaceName color_space,
                                        int32_t pixel_format) {
  ImageProcessing_ColorSpaceInfo info;
  info.metadataType = metadata_type;
  info.colorSpace = static_cast<int32_t>(color_space);
  info.pixelFormat = pixel_format;
  return info;
}

bool SetColorSpaceChecked(OH_PixelmapNative* pixelmap,
                          ColorSpaceName color_space,
                          const char* stage) {
  OH_NativeColorSpaceManager* manager =
      OH_NativeColorSpaceManager_CreateFromName(color_space);
  if (manager == nullptr) {
    FML_LOG(ERROR) << stage << ": create colorspace failed";
    return false;
  }
  const Image_ErrorCode err =
      OH_PixelmapNative_SetColorSpaceNative(pixelmap, manager);
  OH_NativeColorSpaceManager_Destroy(manager);
  FML_LOG(ERROR) << stage << ": SetColorSpace(" << static_cast<int>(color_space)
                 << ")->" << static_cast<int>(err);
  return err == IMAGE_SUCCESS;
}

bool SetMetadataTypeChecked(OH_PixelmapNative* pixelmap,
                            OH_Pixelmap_HdrMetadataType metadata_type,
                            const char* stage) {
  OH_Pixelmap_HdrMetadataValue value = {};
  value.type = metadata_type;
  const Image_ErrorCode err =
      OH_PixelmapNative_SetMetadata(pixelmap, HDR_METADATA_TYPE, &value);
  FML_LOG(ERROR) << stage << ": SetMetadata(" << static_cast<int>(metadata_type)
                 << ")->" << static_cast<int>(err);
  return err == IMAGE_SUCCESS;
}

bool HasDmaNativeBuffer(OH_PixelmapNative* pixelmap, const char* stage) {
  if (pixelmap == nullptr) {
    FML_LOG(ERROR) << stage << ": null pixelmap";
    return false;
  }
  OH_NativeBuffer* buffer = nullptr;
  const Image_ErrorCode err =
      OH_PixelmapNative_GetNativeBuffer(pixelmap, &buffer);
  const bool valid = err == IMAGE_SUCCESS && buffer != nullptr;
  FML_LOG(ERROR) << stage << ": GetNativeBuffer->" << static_cast<int>(err)
                 << " buffer=" << (buffer != nullptr);
  if (buffer != nullptr) {
    OH_NativeBuffer_Unreference(buffer);
  }
  return valid;
}

void RunProbe() {
  // Initialize the VPE environment exactly once per process and never
  // deinitialize it: repeated Initialize calls error out, and a Deinit breaks
  // every other user (MPF composition, AI HDR conversions). The old
  // per-operation init/deinit cycles also break once an init-once user (MPF)
  // leaves the environment permanently initialized.
  if (OH_ImageProcessing_InitializeEnvironment() != IMAGE_PROCESSING_SUCCESS) {
    FML_LOG(ERROR) << "AiHdrProbe: InitializeEnvironment failed";
  }

  // Query the exact documented SDR2HDR tuple. ImageProcessing_ColorSpaceInfo
  // stores an HdrMetadataType value here (NONE/BASE/GAINMAP/ALTERNATE), even
  // though one SDK header comment incorrectly links the field to the key
  // enum. The 7.0 Release capability tests use these value enums as well.
  auto sdr =
      MakeInfo(kSourceMetadataType, kSourceColorSpace, kSourcePixelFormat);
  auto hlg =
      MakeInfo(kTargetMetadataType, kTargetColorSpace, kTargetPixelFormat);

  bool primary = OH_ImageProcessing_IsColorSpaceConversionSupported(&sdr, &hlg);
  bool same_format =
      OH_ImageProcessing_IsColorSpaceConversionSupported(&sdr, &sdr);
  FML_LOG(ERROR) << "AiHdrProbe: sRGB/NONE/RGBA8888 -> "
                    "BT2020HLG/ALTERNATE/RGBA1010102 = "
                 << primary;
  FML_LOG(ERROR) << "AiHdrProbe: sRGB(RGBA8888)->sRGB(RGBA8888) = "
                 << same_format;
  g_sdr_to_hlg_supported = primary;
  FML_LOG(ERROR) << "AiHdrProbe: verdict sdr_to_hlg_supported = " << primary;

  // The capability table does not validate real PixelMaps. Exercise the
  // exact DMA/colorspace/metadata tuple with a deterministic textured image.
  // UNKNOWN/PROCESS_FAILED is inconclusive because vendor AI can decline
  // probe content; a hard contract failure still disables the feature.
  do {
    OH_Pixelmap_InitializationOptions* sopts = nullptr;
    if (OH_PixelmapInitializationOptions_Create(&sopts) != IMAGE_SUCCESS ||
        sopts == nullptr) {
      FML_LOG(ERROR) << "AiHdrSelfTest: src opts failed";
      g_sdr_to_hlg_supported = false;
      break;
    }
    OH_PixelmapInitializationOptions_SetWidth(sopts, 64);
    OH_PixelmapInitializationOptions_SetHeight(sopts, 64);
    OH_PixelmapInitializationOptions_SetPixelFormat(sopts,
                                                    PIXEL_FORMAT_RGBA_8888);
    OH_PixelmapInitializationOptions_SetAlphaType(sopts,
                                                  PIXELMAP_ALPHA_TYPE_OPAQUE);
    const size_t src_stride = 64 * 4;
    std::vector<uint8_t> src(src_stride * 64, 0);
    for (size_t y = 0; y < 64; y++) {
      for (size_t x = 0; x < 64; x++) {
        uint8_t* p = src.data() + src_stride * y + x * 4;
        const uint8_t horizontal = static_cast<uint8_t>((x * 255) / 63);
        const uint8_t vertical = static_cast<uint8_t>((y * 255) / 63);
        const uint8_t detail = (((x / 8) ^ (y / 8)) & 1) ? 224 : 32;
        p[0] = static_cast<uint8_t>((3 * horizontal + detail) / 4);
        p[1] = static_cast<uint8_t>((3 * vertical + (255 - detail)) / 4);
        p[2] = static_cast<uint8_t>((horizontal + vertical + detail) / 3);
        p[3] = 0xFF;
      }
    }
    OH_PixelmapNative* in = nullptr;
    Image_ErrorCode serr = OH_PixelmapNative_CreateEmptyPixelmapUsingAllocator(
        sopts, IMAGE_ALLOCATOR_MODE_DMA, &in);
    OH_PixelmapInitializationOptions_Release(sopts);
    if (serr != IMAGE_SUCCESS || in == nullptr) {
      FML_LOG(ERROR) << "AiHdrSelfTest: src create failed: "
                     << static_cast<int>(serr);
      g_sdr_to_hlg_supported = false;
      break;
    }
    Image_ErrorCode werr =
        OH_PixelmapNative_WritePixels(in, src.data(), src.size());
    FML_LOG(ERROR) << "AiHdrSelfTest: src write -> " << static_cast<int>(werr);

    OH_Pixelmap_InitializationOptions* topts = nullptr;
    if (OH_PixelmapInitializationOptions_Create(&topts) != IMAGE_SUCCESS ||
        topts == nullptr) {
      FML_LOG(ERROR) << "AiHdrSelfTest: dst opts failed";
      OH_PixelmapNative_Release(in);
      g_sdr_to_hlg_supported = false;
      break;
    }
    OH_PixelmapInitializationOptions_SetWidth(topts, 64);
    OH_PixelmapInitializationOptions_SetHeight(topts, 64);
    OH_PixelmapInitializationOptions_SetPixelFormat(topts,
                                                    PIXEL_FORMAT_RGBA_1010102);
    OH_PixelmapInitializationOptions_SetAlphaType(topts,
                                                  PIXELMAP_ALPHA_TYPE_OPAQUE);
    OH_PixelmapNative* out = nullptr;
    Image_ErrorCode terr = OH_PixelmapNative_CreateEmptyPixelmapUsingAllocator(
        topts, IMAGE_ALLOCATOR_MODE_DMA, &out);
    OH_PixelmapInitializationOptions_Release(topts);
    if (terr != IMAGE_SUCCESS || out == nullptr) {
      FML_LOG(ERROR) << "AiHdrSelfTest: dst create failed: "
                     << static_cast<int>(terr);
      OH_PixelmapNative_Release(in);
      g_sdr_to_hlg_supported = false;
      break;
    }

    const bool configured =
        HasDmaNativeBuffer(in, "AiHdrSelfTest.input") &&
        HasDmaNativeBuffer(out, "AiHdrSelfTest.output") &&
        SetColorSpaceChecked(in, kSourceColorSpace, "AiHdrSelfTest.input") &&
        SetMetadataTypeChecked(in, kSourceMetadataType,
                               "AiHdrSelfTest.input") &&
        SetColorSpaceChecked(out, kTargetColorSpace, "AiHdrSelfTest.output") &&
        SetMetadataTypeChecked(out, kTargetMetadataType,
                               "AiHdrSelfTest.output");
    FML_LOG(ERROR) << "AiHdrSelfTest: exact tuple "
                      "src={DMA,RGBA8888,SRGB,NONE} "
                      "dst={DMA,RGBA1010102,BT2020_HLG,ALTERNATE} configured="
                   << configured;
    if (!configured || werr != IMAGE_SUCCESS) {
      OH_PixelmapNative_Release(out);
      OH_PixelmapNative_Release(in);
      g_sdr_to_hlg_supported = false;
      break;
    }

    // The environment was initialized once by RunProbe; do not re-init here
    // (a repeated call errors out) and do not deinitialize afterwards.

    OH_ImageProcessing* proc = nullptr;
    ImageProcessing_ErrorCode cerr = OH_ImageProcessing_Create(
        &proc, IMAGE_PROCESSING_TYPE_COLOR_SPACE_CONVERSION);
    ImageProcessing_ErrorCode verr = IMAGE_PROCESSING_ERROR_INVALID_VALUE;
    if (cerr == IMAGE_PROCESSING_SUCCESS && proc != nullptr) {
      verr = OH_ImageProcessing_ConvertColorSpace(proc, in, out);
      FML_LOG(ERROR) << "AiHdrSelfTest: convert -> " << static_cast<int>(verr);
      OH_ImageProcessing_Destroy(proc);
    } else {
      FML_LOG(ERROR) << "AiHdrSelfTest: processor create failed: "
                     << static_cast<int>(cerr);
    }
    // Environment stays initialized for the rest of the process (init-once,
    // never deinit): a deinit here would break MPF composition and every
    // later AI HDR conversion.

    uint32_t ow = 0;
    uint32_t oh = 0;
    uint32_t ostride = 0;
    int32_t ofmt = 0;
    {
      OH_Pixelmap_ImageInfo* oi = nullptr;
      OH_PixelmapImageInfo_Create(&oi);
      if (oi != nullptr) {
        OH_PixelmapNative_GetImageInfo(out, oi);
        OH_PixelmapImageInfo_GetWidth(oi, &ow);
        OH_PixelmapImageInfo_GetHeight(oi, &oh);
        OH_PixelmapImageInfo_GetRowStride(oi, &ostride);
        OH_PixelmapImageInfo_GetPixelFormat(oi, &ofmt);
        OH_PixelmapImageInfo_Release(oi);
      }
    }
    FML_LOG(ERROR) << "AiHdrSelfTest: out " << ow << "x" << oh
                   << " stride=" << ostride << " fmt=" << ofmt;
    std::vector<uint8_t> buf;
    if (ostride >= 64 * 4 && oh == 64) {
      buf.resize(static_cast<size_t>(ostride) * oh, 0xEE);
    }
    size_t bsz = buf.size();
    Image_ErrorCode rerr =
        buf.empty() ? IMAGE_BAD_PARAMETER
                    : OH_PixelmapNative_ReadPixels(out, buf.data(), &bsz);
    FML_LOG(ERROR) << "AiHdrSelfTest: read -> " << static_cast<int>(rerr);
    uint32_t mid = 0;
    if (rerr == IMAGE_SUCCESS) {
      // ReadPixels strips allocator padding. Its output row pitch is the
      // packed RGBA byte width even though capacity was based on ostride.
      memcpy(&mid, buf.data() + static_cast<size_t>(64 * 4) * 32, 4);
    }
    char hex[16];
    snprintf(hex, sizeof(hex), "%08X", mid);
    FML_LOG(ERROR) << "AiHdrSelfTest: row32[0] = 0x" << hex;

    const bool self_test_pass =
        verr == IMAGE_PROCESSING_SUCCESS && rerr == IMAGE_SUCCESS && mid != 0;
    const bool inconclusive = verr == IMAGE_PROCESSING_ERROR_UNKNOWN ||
                              verr == IMAGE_PROCESSING_ERROR_PROCESS_FAILED;
    FML_LOG(ERROR) << "AiHdrSelfTest: pass=" << self_test_pass
                   << " inconclusive=" << inconclusive;
    if (!self_test_pass && !inconclusive) {
      g_sdr_to_hlg_supported = false;
    }
    FML_LOG(ERROR) << "AiHdrProbe: sdr_to_hlg_supported (exact self test) = "
                   << g_sdr_to_hlg_supported;
    OH_PixelmapNative_Release(out);
    OH_PixelmapNative_Release(in);
  } while (false);
}

}  // namespace

void ProbeAndLogCapabilities() {
  std::call_once(g_probe_once, RunProbe);
}

bool IsSdrToHlgSupported() {
  std::call_once(g_probe_once, RunProbe);
  return g_sdr_to_hlg_supported;
}

namespace {

constexpr uint32_t kAiHdrRgba8888Bytes = 4;
// DMA rows are 64-byte aligned; same constant as the MPF decoder.
constexpr uint32_t kAiHdrRowStrideAlignment = 64;

uint32_t AiHdrAlignTo(uint32_t value, uint32_t alignment) {
  return (value + alignment - 1) / alignment * alignment;
}

bool AiHdrGetPixelmapInfo(OH_PixelmapNative* pixelmap,
                          uint32_t* width,
                          uint32_t* height,
                          uint32_t* row_stride,
                          int32_t* pixel_format) {
  OH_Pixelmap_ImageInfo* info = nullptr;
  if (OH_PixelmapImageInfo_Create(&info) != IMAGE_SUCCESS || info == nullptr) {
    return false;
  }
  OH_PixelmapNative_GetImageInfo(pixelmap, info);
  OH_PixelmapImageInfo_GetWidth(info, width);
  OH_PixelmapImageInfo_GetHeight(info, height);
  OH_PixelmapImageInfo_GetRowStride(info, row_stride);
  OH_PixelmapImageInfo_GetPixelFormat(info, pixel_format);
  OH_PixelmapImageInfo_Release(info);
  return *width > 0 && *height > 0;
}

// Mirror of the MPF decoder CopyPixelmapToDmaRGBA8888: the VPE requires
// DMA-resident RGBA8888 inputs with a declared 64-aligned row stride.
OH_PixelmapNative* CopyPixelmapToDmaRGBA8888(OH_PixelmapNative* src) {
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t src_stride = 0;
  int32_t src_format = 0;
  if (!AiHdrGetPixelmapInfo(src, &width, &height, &src_stride, &src_format)) {
    FML_LOG(ERROR) << "AiHdr: DMA normalization: invalid source";
    return nullptr;
  }
  if (src_format != PIXEL_FORMAT_RGBA_8888) {
    FML_LOG(ERROR) << "AiHdr: DMA normalization: unexpected format "
                   << src_format;
    return nullptr;
  }
  const uint32_t tight_stride = width * kAiHdrRgba8888Bytes;
  if (src_stride < tight_stride) {
    FML_LOG(ERROR) << "AiHdr: DMA normalization: src_stride too small "
                   << src_stride << " < " << tight_stride;
    return nullptr;
  }

  const size_t tight_size = static_cast<size_t>(tight_stride) * height;
  // ReadPixels requires capacity for the PixelMap allocation, but for packed
  // RGB formats it writes rowDataSize_ bytes per row contiguously and strips
  // the DMA padding. Keep the physical stride for the capacity only.
  std::vector<uint8_t> src_buf(static_cast<size_t>(src_stride) * height);
  size_t src_size = src_buf.size();
  if (OH_PixelmapNative_ReadPixels(src, src_buf.data(), &src_size) !=
      IMAGE_SUCCESS) {
    FML_LOG(ERROR) << "AiHdr: DMA normalization: ReadPixels failed";
    return nullptr;
  }

  OH_Pixelmap_InitializationOptions* opt = nullptr;
  if (OH_PixelmapInitializationOptions_Create(&opt) != IMAGE_SUCCESS ||
      opt == nullptr) {
    FML_LOG(ERROR) << "AiHdr: DMA normalization: options create failed";
    return nullptr;
  }
  OH_PixelmapInitializationOptions_SetWidth(opt, width);
  OH_PixelmapInitializationOptions_SetHeight(opt, height);
  OH_PixelmapInitializationOptions_SetPixelFormat(opt, PIXEL_FORMAT_RGBA_8888);
  OH_PixelmapInitializationOptions_SetAlphaType(opt,
                                                PIXELMAP_ALPHA_TYPE_OPAQUE);
  const uint32_t desired_stride =
      AiHdrAlignTo(tight_stride, kAiHdrRowStrideAlignment);
  OH_PixelmapInitializationOptions_SetRowStride(
      opt, static_cast<int32_t>(desired_stride));

  OH_PixelmapNative* dst = nullptr;
  Image_ErrorCode err = OH_PixelmapNative_CreateEmptyPixelmapUsingAllocator(
      opt, IMAGE_ALLOCATOR_MODE_DMA, &dst);
  OH_PixelmapInitializationOptions_Release(opt);
  if (err != IMAGE_SUCCESS || dst == nullptr) {
    FML_LOG(ERROR) << "AiHdr: DMA normalization: create DMA pixelmap failed: "
                   << static_cast<int>(err);
    return nullptr;
  }

  uint32_t dst_width = 0;
  uint32_t dst_height = 0;
  uint32_t dst_stride = 0;
  int32_t dst_format = 0;
  if (!AiHdrGetPixelmapInfo(dst, &dst_width, &dst_height, &dst_stride,
                            &dst_format) ||
      dst_stride < tight_stride) {
    FML_LOG(ERROR) << "AiHdr: DMA normalization: dst_stride too small "
                   << dst_stride;
    OH_PixelmapNative_Release(dst);
    return nullptr;
  }
  if (dst_stride % kAiHdrRowStrideAlignment != 0) {
    FML_LOG(ERROR) << "AiHdr: DMA normalization: dst_stride not 64-aligned "
                   << dst_stride;
  }

  // WritePixels has the matching contract: it requires allocation-sized
  // capacity but consumes tightly packed source rows and inserts destination
  // DMA padding itself. A physically pitched staging buffer would therefore
  // copy padding into the next logical row.
  std::vector<uint8_t> dst_buf(static_cast<size_t>(dst_stride) * height, 0);
  memcpy(dst_buf.data(), src_buf.data(), tight_size);
  if (OH_PixelmapNative_WritePixels(dst, dst_buf.data(), dst_buf.size()) !=
      IMAGE_SUCCESS) {
    FML_LOG(ERROR) << "AiHdr: DMA normalization: WritePixels failed";
    OH_PixelmapNative_Release(dst);
    return nullptr;
  }
  FML_LOG(ERROR) << "AiHdr: input normalized to DMA " << width << "x" << height
                 << " stride=" << dst_stride;
  return dst;
}

}  // namespace

OH_PixelmapNative* ConvertSdrToHlg(OH_PixelmapNative* sdr, bool source_is_dma) {
  if (!g_sdr_to_hlg_supported) {
    return nullptr;
  }
  VpeSlot slot;

  // Same input contract as the MPF recipe: VPE consumes DMA-resident RGBA8888
  // with a 64-byte-aligned row stride.  Sources that are not DMA, not RGBA8888,
  // or whose decoded stride is not 64-aligned (odd / narrow widths decode to
  // unaligned rows, e.g. 1320x2848 -> 5280, 2723x897 -> 10892) are copied into
  // a fresh declared-stride DMA pixelmap first; the VPE rejects unaligned
  // inputs with 29200005.
  OH_PixelmapNative* dma_src = sdr;
  uint32_t src_w = 0;
  uint32_t src_h = 0;
  uint32_t src_stride = 0;
  int32_t src_format = 0;
  const bool src_aligned =
      AiHdrGetPixelmapInfo(sdr, &src_w, &src_h, &src_stride, &src_format) &&
      src_format == PIXEL_FORMAT_RGBA_8888 &&
      (src_stride % kAiHdrRowStrideAlignment) == 0;
  if (!source_is_dma || !src_aligned) {
    if (source_is_dma) {
      FML_LOG(ERROR) << "AiHdr: DMA input stride " << src_stride
                     << " not 64-aligned, normalizing";
    }
    dma_src = CopyPixelmapToDmaRGBA8888(sdr);
    if (dma_src == nullptr) {
      FML_LOG(ERROR) << "AiHdr: DMA input normalization failed";
      return nullptr;
    }
  }

  // Destination: same creation recipe as the MPF HDR output (RGBA1010102,
  // opaque, declared 64-aligned stride, DMA allocator).
  OH_Pixelmap_InitializationOptions* opts = nullptr;
  if (OH_PixelmapInitializationOptions_Create(&opts) != IMAGE_SUCCESS ||
      opts == nullptr) {
    FML_LOG(ERROR) << "AiHdr: init options create failed";
    if (dma_src != sdr) {
      OH_PixelmapNative_Release(dma_src);
    }
    return nullptr;
  }

  uint32_t w = 0;
  uint32_t h = 0;
  uint32_t info_stride = 0;
  int32_t info_format = 0;
  AiHdrGetPixelmapInfo(sdr, &w, &h, &info_stride, &info_format);
  OH_PixelmapInitializationOptions_SetWidth(opts, w);
  OH_PixelmapInitializationOptions_SetHeight(opts, h);
  OH_PixelmapInitializationOptions_SetPixelFormat(opts,
                                                  PIXEL_FORMAT_RGBA_1010102);
  OH_PixelmapInitializationOptions_SetAlphaType(opts,
                                                PIXELMAP_ALPHA_TYPE_OPAQUE);
  // No declared row stride: the DMA allocator picks the hardware layout and
  // may override a declared value (observed 1344 -> 1536); the actual stride
  // is read back below and used for every consumer.

  OH_PixelmapNative* hlg = nullptr;
  Image_ErrorCode perr = OH_PixelmapNative_CreateEmptyPixelmapUsingAllocator(
      opts, IMAGE_ALLOCATOR_MODE_DMA, &hlg);
  OH_PixelmapInitializationOptions_Release(opts);
  if (perr != IMAGE_SUCCESS || hlg == nullptr) {
    FML_LOG(ERROR) << "AiHdr: destination pixelmap create failed: "
                   << static_cast<int>(perr);
    if (dma_src != sdr) {
      OH_PixelmapNative_Release(dma_src);
    }
    return nullptr;
  }
  {
    uint32_t hw = 0;
    uint32_t hh = 0;
    uint32_t hstride = 0;
    int32_t hfmt = 0;
    AiHdrGetPixelmapInfo(hlg, &hw, &hh, &hstride, &hfmt);
    FML_LOG(ERROR) << "AiHdr: hlg actual " << hw << "x" << hh
                   << " stride=" << hstride << " fmt=" << hfmt;
  }

  // Label the destination color space before the conversion so the result is
  // consumed as BT.2020 HLG.
  OH_NativeColorSpaceManager* cs =
      OH_NativeColorSpaceManager_CreateFromName(BT2020_HLG);
  if (cs != nullptr) {
    OH_PixelmapNative_SetColorSpaceNative(hlg, cs);
    OH_NativeColorSpaceManager_Destroy(cs);
  }

  // Label the SDR input's colorspace before the conversion (the MPF decoder
  // labels its inputs too); an unlabeled input makes the VPE apply a wrong
  // conversion matrix and the result comes out false-colored.
  OH_NativeColorSpaceManager* cs_in =
      OH_NativeColorSpaceManager_CreateFromName(SRGB);
  if (cs_in != nullptr) {
    OH_PixelmapNative_SetColorSpaceNative(dma_src, cs_in);
    OH_NativeColorSpaceManager_Destroy(cs_in);
  }

  // Stamp the exact tuple the capability self test validated BEFORE the
  // conversion: src={DMA,RGBA8888,SRGB,NONE}
  // dst={DMA,RGBA1010102,BT2020_HLG,ALTERNATE}. The vendor algorithm reads
  // the pixelmaps' metadata (not just their color spaces) to classify the
  // conversion; without the ALTERNATE stamp on the destination the call comes
  // back UNSUPPORTED/PROCESS_FAILED on 7.0 Release tablets. ALTERNATE is the
  // documented SDR2HDR output metadata type, not an MPF-only value.
  if (!SetMetadataTypeChecked(dma_src, kSourceMetadataType, "AiHdr.input") ||
      !SetMetadataTypeChecked(hlg, kTargetMetadataType, "AiHdr.output")) {
    FML_LOG(ERROR) << "AiHdr: metadata stamp failed";
    OH_PixelmapNative_Release(hlg);
    if (dma_src != sdr) {
      OH_PixelmapNative_Release(dma_src);
    }
    return nullptr;
  }

  // The VPE environment is process-global and was initialized exactly once
  // by the capability probe; do not Initialize/Deinitialize per conversion
  // (repeated init errors out once the init-once MPF user has run, and a
  // deinit breaks every other user).
  bool converted = false;
  ImageProcessing_ErrorCode err = IMAGE_PROCESSING_SUCCESS;
  // The VPE service intermittently fails conversions under load; retry a
  // couple of times before giving up and falling back to SDR.
  constexpr int kConvertAttempts = 3;
  for (int attempt = 0; attempt < kConvertAttempts && !converted; ++attempt) {
    if (attempt > 0) {
      usleep(100 * 1000);
    }
    OH_ImageProcessing* processor = nullptr;
    err = OH_ImageProcessing_Create(
        &processor, IMAGE_PROCESSING_TYPE_COLOR_SPACE_CONVERSION);
    if (err == IMAGE_PROCESSING_SUCCESS && processor != nullptr) {
      err = OH_ImageProcessing_ConvertColorSpace(processor, dma_src, hlg);
      converted = (err == IMAGE_PROCESSING_SUCCESS);
      OH_ImageProcessing_Destroy(processor);
      if (!converted) {
        FML_LOG(ERROR) << "AiHdr: convert attempt " << attempt
                       << " failed: " << static_cast<int>(err);
      }
    } else {
      FML_LOG(ERROR) << "AiHdr: create conversion processor failed: "
                     << static_cast<int>(err);
    }
  }
  if (dma_src != sdr) {
    OH_PixelmapNative_Release(dma_src);
  }
  if (!converted) {
    FML_LOG(ERROR) << "AiHdr: ConvertColorSpace failed: "
                   << static_cast<int>(err);
    OH_PixelmapNative_Release(hlg);
    return nullptr;
  }

  FML_LOG(ERROR) << "AiHdr: SDR -> BT.2020 HLG conversion ok";
  return hlg;
}

}  // namespace ai_hdr
