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

For OHOS/HDR Engine work, follow this order and keep the WSL checkout as the source of truth:

- Modify `/home/dgh18/engine_3.44.9` in Ubuntu WSL first. Do not make the initial Engine change in the Windows mirror or in a copied staging directory.
- Build the complete Engine from the WSL checkout with `./ohos -t release`, using only the WSL OpenHarmony SDK rooted at `/home/dgh18/ohos-sdk-linux-26.0.0.821/command-line-tools/sdk` (the installed API is under its `default` directory) and its matching WSL GN/Ninja/toolchain. Before every WSL HAR action, prepend `/home/dgh18/ohos-sdk-linux-26.0.0.821/command-line-tools/bin` and `/home/dgh18/ohos-sdk-linux-26.0.0.821/command-line-tools/hvigor/bin` to `PATH`, then run `hash -r` so `hvigorw` resolves to the WSL copy. Do not use the Windows SDK for any Engine or HAR action. A progress percentage is not a successful build: verify the final command exit status and every produced artifact's timestamp, size, and SHA256.
- Do not copy any Engine artifact into the Windows FVM cache before the WSL build has completed successfully.
- After a successful WSL `./ohos -t release`, verify and record hashes before copying the matching build outputs into `F:\fvm\versions\custom_3.44.9\bin\cache`, including the OHOS Engine/HAR and any produced `dill`, `sky_engine`, or modified Dart framework artifacts. Copy only the corresponding files from that build; do not overwrite unrelated Flutter/Dart snapshots. Keep `F:\immich_ohos\mobile\ohos\har\flutter.har` synchronized for app builds when it is the selected app Engine artifact.
- Only after the WSL artifacts pass validation, synchronize the modified WSL source files (including this `AGENTS.md`) into `F:\flutter_engine_ohos_3.44.9` for future Windows host builds. Preserve unrelated Windows-side dirty changes; never reset the checkout or overwrite files outside the explicitly synchronized file set.
- Every artifact/source synchronization must record source and destination paths, timestamps, sizes, and SHA256. Run the app's FVM build and device installation/runtime validation as separate gates.
- Only after the Engine artifacts have been copied and verified may the Windows-side OpenHarmony SDK be used to compile `immich_ohos`; that Windows SDK step is for the app, not for the Engine.
- If the Engine build succeeds but FVM/app/device validation is incomplete, report the work as unverified rather than complete.
