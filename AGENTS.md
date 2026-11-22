# Flutter Project Rules

CRITICAL: You MUST follow these steps after modifying any Dart file. NEVER declare a task done without completing both steps.

After modifying any Dart file, you MUST:
1. Run `dart analyze --fatal-infos <files>` on every modified file. Fix ALL errors and warnings before proceeding.
2. Run `dart format <files>` on every modified file.

No exceptions. These steps are mandatory even for trivial changes.

CRITICAL: When adding new rules to `.agents/rules/` directory, you MUST also sync the rules to AGENTS.md.

Create a corresponding section in AGENTS.md for each rule file in `.agents/rules/` to maintain consistency between the rules directory and AGENTS.md.

## OHos ArkTS Native Public API Deletion

CRITICAL: When removing an `export` symbol that already exists in the repository from the ohos ArkTS native side, you MUST ask the user for confirmation before proceeding. Do NOT delete it without explicit user confirmation.

This rule does NOT apply to modifying an existing symbol's implementation (signature unchanged) or restructuring files not yet committed to the repository.

## OHOS Flutter Engine WSL-first build and synchronization

- Keep all 3.44.9 OHOS/HDR source changes on the existing WSL branch oh-3.44.9-dev-hdr; do not create task branches. Before merging any existing branch, compare its commit-only history and inspect the worktree and stashes; never assume changes were integrated.

CRITICAL: ALWAYS build the Engine with `./ohos -t release`, run from `/home/dgh18/engine_3.44.9/engine` (the directory containing the `ohos` script). Never build with raw `ninja`/`gn`/`autoninja` invocations or hand-picked out directories: the script pins the WSL OpenHarmony SDK, toolchain and release args, and artifacts from any other build path must be treated as untrusted and never installed or synced.

For OHOS/HDR Engine work, follow this order and keep the WSL checkout as the source of truth:

- Modify `/home/dgh18/engine_3.44.9` in Ubuntu WSL first. Do not make the initial Engine change in the Windows mirror or in a copied staging directory.
- Build the complete Engine from the WSL checkout with `./ohos -t release`, using only the WSL OpenHarmony SDK rooted at `/home/dgh18/ohos-sdk-linux-26.0.0.821/command-line-tools/sdk` (the installed API is under its `default` directory) and its matching WSL GN/Ninja/toolchain. Before every WSL HAR action, prepend `/home/dgh18/ohos-sdk-linux-26.0.0.821/command-line-tools/bin` and `/home/dgh18/ohos-sdk-linux-26.0.0.821/command-line-tools/hvigor/bin` to `PATH`, then run `hash -r` so `hvigorw` resolves to the WSL copy. Do not use the Windows SDK for any Engine or HAR action. A progress percentage is not a successful build: verify the final command exit status and every produced artifact's timestamp, size, and SHA256.
- On memory-limited hosts, set `FLUTTER_OHOS_BUILD_JOBS=3` before `./ohos -t release`; the script passes the limit to both target and host Ninja builds while retaining the same SDK and build outputs. Increase or omit the value only when memory allows.
- Do not copy any Engine artifact into the Windows FVM cache before the WSL build has completed successfully.
- After a successful WSL `./ohos -t release`, verify and record hashes before copying the matching build outputs into `F:\fvm\versions\custom_3.44.9\bin\cache`, including the OHOS Engine/HAR and any produced `dill`, `sky_engine`, or modified Dart framework artifacts. Copy only the corresponding files from that build; do not overwrite unrelated Flutter/Dart snapshots. Keep `F:\immich_ohos\mobile\ohos\har\flutter.har` synchronized for app builds when it is the selected app Engine artifact.
- Only after the WSL artifacts pass validation, synchronize the modified WSL source files (including this `AGENTS.md`) into `F:\flutter_engine_ohos_3.44.9` for future Windows host builds. Preserve unrelated Windows-side dirty changes; never reset the checkout or overwrite files outside the explicitly synchronized file set.
- After the modified WSL source files have been synchronized into F:\flutter_engine_ohos_3.44.9, compile the Windows host release with F:\flutter_engine_ohos_3.44.9\engine\src\flutter\bin\et.bat build --config host_release. This Windows host-tool build is separate from the WSL OHOS target build and must use the Windows host toolchain, not the DevEco OHOS SDK. Verify the exit status and output timestamps, sizes, and SHA256 before copying host artifacts.
- Before using a Windows gen_snapshot for OHOS ARM64 app AOT, validate both ELF e_machine=183 and snapshot features against the selected Engine: the verified 2026-11-04 runtime requires `arm64 ohos compressed-pointers`. Neither plain `host_release/gen_snapshot.exe` (x86-64) nor `gen_snapshot_product_linux_arm64.exe` (arm64 linux no-compressed-pointers) is compatible; `windows_simarm64` alone is insufficient. Until a matching Windows tool is available, compile the app kernel with the existing WSL `/home/dgh18/engine_3.44.9/engine/src/out/ohos_release_arm64/clang_x64/gen_snapshot`, then repackage/sign the app. Do not rebuild or replace the Engine/HAR for app-only changes. Record tool and output paths, timestamps, sizes, SHA256 and snapshot features.
- Never copy a generic Windows host_release/gen_snapshot.exe into the FVM OHOS artifact location as a phone AOT tool without first generating a fresh AOT from the current app.dill and verifying AArch64 ELF, `arm64 ohos compressed-pointers`, and compatibility with the exact selected Engine build. A tool being x64 describes where it runs, not the architecture it produces. Before installation, also verify the final HAP libflutter.so hash against the validated WSL Engine/HAR.
- Every artifact/source synchronization must record source and destination paths, timestamps, sizes, and SHA256. Run the app's FVM build and device installation/runtime validation as separate gates.
- Only after the Engine artifacts have been copied and verified may the Windows-side OpenHarmony SDK be used to compile `immich_ohos`; that Windows SDK step is for the app, not for the Engine.
- If the Engine build succeeds but FVM/app/device validation is incomplete, report the work as unverified rather than complete.

## BoringSSL OHOS patch tracking

- The 3.44.9 upstream `DEPS_ohos` hook applies `engine/src/flutter/third_party/boringssl/ohos_patches/0001-Add-eMudhra-kBlockList.patch` to the nested BoringSSL checkout and marks `crypto/x509/x509_vfy.cc` and `pki/verify_certificate_chain.cc` with Git `skip-worktree`. A clean `git status` in that checkout therefore does not prove its files match its commit.
- Before changing, syncing, or rebuilding that dependency, record its commit, both files' SHA256 and `git ls-files -v` flags; verify an applied patch with `git apply --reverse --check -p1 <absolute-patch-path>`. Preserve any existing local changes and flags, and do not reset or unhide those files blindly. The tracked patch and hook must remain available so a fresh `gclient sync` reproduces the build.
