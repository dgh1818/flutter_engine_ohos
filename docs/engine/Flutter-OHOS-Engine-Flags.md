# Setting Flutter OHOS engine flags

You can set flags for the Flutter engine on OHOS in two different ways:

- Dynamically, via the Want used to start the ability (used by the Flutter
  tooling in debug/profile sessions)
- Statically, via the `metadata` array in the application's `module.json5`
  (per-build configuration)

See
`engine/src/flutter/shell/platform/ohos/flutter_embedding/flutter/src/main/ets/embedding/engine/FlutterEngineFlags.ets`
for the registry of flags that can be declared via `module.json5` metadata,
and see `engine/src/flutter/shell/common/switches.cc` for the list of all
supported flags across platforms.

**Note: If a flag is specified both via the Want and in `module.json5`
metadata, the Want value takes precedence at runtime.**

## Setting flags via module.json5 metadata

Declare engine flags in the module-level `metadata` array of `module.json5`.
Metadata keys are prefixed with `flutter.engine.` to avoid collisions with
other metadata keys:

```json5
{
  "module": {
    "name": "entry",
    // ...
    "metadata": [
      {
        "name": "flutter.engine.EnableDartProfiling",
        "value": "true"
      },
      {
        "name": "flutter.engine.EnableImpeller",
        "value": "false"
      }
    ]
  }
}
```

Use metadata when you want a fixed, reproducible baseline of engine flags for
your app across all launches — for example to enable profiling-related flags
in a performance-test build.

### Value rules

- **Boolean flags** (e.g. `flutter.engine.EnableDartProfiling`) are only
  enabled when the value is exactly `"true"`. Any other value, including
  malformed values, leaves the flag disabled.
- **Value flags** (e.g. `flutter.engine.EnableImpeller`,
  `flutter.engine.VMServicePort`) require a non-empty value; the value is
  appended verbatim to the engine argument.
- **Unknown metadata keys** are ignored.
- In multi-HAP apps, the metadata of every module is merged; if the same
  `flutter.engine.*` key is declared in more than one module, the value from
  the module appearing later in the bundle's module list wins, and a warning
  is logged.
- Metadata is re-read on each launch (once per FlutterLoader instance), as
  on Android.

### Release mode

Flags are classified as allowed or not allowed in release mode in the
`FlutterEngineFlags` registry, mirroring the Android classification
(flutter/flutter#182522). Flags that are not allowed in release mode — for
example `TraceSkia`, `DisableServiceAuthCodes`, `StartPaused`, or `DartFlags`
— are intended for development and profiling only; declaring them in a
release build is ignored and logs an error.

The same release whitelist applies to flags supplied via the Want: in a
release build they are removed from the launch arguments before the engine
starts. Launch arguments that are not part of the registry pass through
unchanged, matching Android (flutter/flutter#182557).

### Registry scope

The registry contains the flags that also exist in the Want channel
(`FlutterShellArgs`), plus a test-only flag.

### Interaction with embedding-internal defaults

Some switches interact with OHOS-specific build configuration:

- Impeller: when `buildinfo.json5` enables Impeller (and the device is not
  an emulator), the loader appends a bare `--enable-impeller` after the
  metadata args, which wins by last-wins. When `enable_impeller` is
  `false`, the loader pushes nothing and the metadata value decides.
- HCPP is enabled via `buildinfo.json5` (written by `flutter run` flags at
  build time) only; it is not in the registry and cannot be set via
  metadata.
- `--leak-vm` is hardcoded to `true` on OHOS and cannot be overridden via
  metadata.
