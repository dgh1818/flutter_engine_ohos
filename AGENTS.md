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