// Copyright note: fork-only module for the immich OHOS HDR line.
// Licensed under the same LICENSE_KHZG terms as the OHOS embedding.

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_AI_HDR_PROCESSOR_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_AI_HDR_PROCESSOR_H_

struct OH_PixelmapNative;

namespace ai_hdr {

// Converts one SDR PixelMap to BT.2020 HLG (RGBA1010102) using the vendor
// VPE, mirroring the MPF decoder recipe: DMA-resident RGBA8888 input with a
// declared 64-aligned row stride (share-memory sources are copied into one),
// and an HLG-labelled RGBA1010102 DMA destination. The process-global VPE
// environment remains initialized after the one-shot probe. `source_is_dma`
// must reflect whether `sdr` is DMA resident. Returns a new PixelMap the
// caller owns, or nullptr on failure.
OH_PixelmapNative* ConvertSdrToHlg(OH_PixelmapNative* sdr, bool source_is_dma);

// One-shot capability probe. Queries the vendor VPE for the SDR -> BT.2020
// HLG color space conversion combinations this feature relies on and logs
// the result once per process under the "AiHdrProbe" tag. Safe to call from
// any thread; the query itself is a cheap table lookup.
void ProbeAndLogCapabilities();

// Cached result of the sRGB -> BT.2020 HLG (RGBA8888 -> RGBA1010102)
// capability query. Runs the probe on first use.
bool IsSdrToHlgSupported();

}  // namespace ai_hdr

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_AI_HDR_PROCESSOR_H_
