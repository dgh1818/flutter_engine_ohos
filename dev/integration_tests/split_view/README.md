# Split View Integration Tests

Integration tests for the OHOS split view (parallel vision / 分栏) feature.

## Overview

These tests verify end-to-end behavior of the split view feature on a real
OHOS device or emulator. They exercise the full widget tree with real
platform channels and screen dimensions.

### What is tested

| Area | Description |
|------|-------------|
| Home page visibility | Home page renders on initial load |
| Push detail page | Detail page content appears after navigation |
| Pop from detail | Popping detail restores home-only view |
| Fullscreen route | Fullscreen page covers entire screen |
| Navigation sequence | Multiple push/pop cycles maintain correct state |
| Native view | OHOS platform view (OhosView) renders in split view detail page |

## Running

### Prerequisites

- OHOS device or emulator with a wide screen (≥ 600 logical pixels, tablet or 2-in-1)
- Split view enabled in the app's config file
- Screen in landscape orientation (width > height)
- If the device is in portrait, all tests will fail with a clear
  "Device prerequisite not met" message. Rotate to landscape and re-run.

### Run on OHOS device

```bash
cd /path/to/flutter_sdk
flutter drive --driver=integration_test/split_view_test.dart \
  --target=integration_test/split_view_test.dart \
  --project-name=split_view \
  -d <device-id>
```

## Project structure

```
dev/integration_tests/split_view/
├── lib/
│   └── main.dart                        # Test app (home + detail + fullscreen pages)
├── integration_test/
│   └── split_view_test.dart             # Integration test cases
├── ohos/                                # OHOS platform project
│   ├── AppScope/
│   ├── build-profile.json5
│   ├── oh-package.json5
│   ├── package.json
│   ├── hvigorfile.ts
│   ├── hvigorconfig.ts
│   ├── hvigor/
│   └── entry/
│       ├── build-profile.json5
│       ├── hvigorfile.ts
│       └── src/main/
│           ├── module.json5
│           ├── ets/
│           │   ├── entryability/EntryAbility.ets
│           │   ├── entryability/NativeViewFactory.ets
│           │   ├── entryability/NativeViewPage.ets
│           │   ├── pages/Index.ets
│           │   └── plugins/GeneratedPluginRegistrant.ets
│           └── resources/
├── pubspec.yaml
├── analysis_options.yaml
└── README.md
```

## CI

This project is **not** executed by CI. It is intended for manual and
local verification of the split view feature on OHOS devices.
