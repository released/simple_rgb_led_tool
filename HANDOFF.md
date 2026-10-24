# HANDOFF

## Verified x64 EXE slimming 2026-10-25

- Canonical Release and isolated Debug builds, owned control/dialog/normal-close
  checks and five alternating performance pairs PASS; no real INI/device/clipboard.
- EXE 3228672 -> 1145856 bytes; required files 3228672 -> 1145856 bytes.
- SHA-256: A0024545E73638AEBA7BC2A8D5D141DBA48BE30A131636A3C08DB9E3E2AB69A7.
- Actual clean removed 29 compiler files; all 2 protected actual
  files unchanged by SHA. Nested fixtures/help/idempotence PASS.
- Raw sanitized samples/limits: docs/verification/exe-slimming-20261025.json.
  Physical devices, firmware, visual/DPI/clean-PC/Win32 acceptance NOT RUN.
- Ordinary publication commit/remote SHA: shared EXE_SLIMMING_PROGRESS.md.

## Rainbow Fill Animation Fix 2026-10-24

- Rainbow Fill now updates every pattern frame and preserves each LED's
  relative brightness across the animation.
- Breathing followed by Rainbow Fill therefore previews a rainbow breathing
  ring; it is no longer limited to one colored frame among single-color frames.
- An all-black pattern receives a full-brightness rainbow so the command always
  has a visible result.
- Release x64 build `1.0.0.042`: PASS, 0 warnings, 0 errors. Startup smoke:
  PASS; responsive main window and normal close.

## Applied Layout/Preset Status And Reset Color Fix 2026-10-24

- Renamed Apply Layout to Apply and Load Preset to Load.
- Added Current layout and Current preset status text based on the pattern
  actually applied/loaded, not merely the combo-box selection.
- Reset now forces LED selection to LED 0 before restoring frame RGB. This
  prevents a previously selected black LED from changing the preset primary
  color to 0,0,0 and producing an apparently colorless Breathing pattern.
- Release x64 output version `1.0.0.041`; build completion flag cleared,
  executable updated, startup smoke passed, and the app closed normally.
- Full visual sequence click-through: NOT RUN.

## Pattern Reset 2026-10-24

- Added Reset beside Play and Stop; the three buttons split the Pattern Editor
  bottom row.
- Reset stops the preview timer and restores Pattern / Code defaults without
  affecting Control-tab state or USB.
- Release x64 build: PASS, version `1.0.0.040`, 0 warnings, 0 errors.
- Reset click-through: NOT RUN.

## Pattern / Code Groups 2026-10-24

- Split the left column into Pattern Editor and Code Output groups.
- Moved C Symbol and Output backend into Code Output.
- Moved Copy Code and Save Code from the main-frame toolbar into Code Output;
  Copy writes the complete generated bundle to the clipboard and Save opens a
  folder picker and writes the source files.
- Release x64 build: PASS, version `1.0.0.039`, 0 warnings, 0 errors.
- Startup smoke: PASS; responsive main window and normal close.
- Visual click-through/file-dialog save: NOT RUN.

## Offline Preset Alignment 2026-10-24

- Pattern / Code remains an offline preview and code generator; no USB or
  target-specific MCU profile was added.
- Added Static, true Breathing, Blink, moving selected-color Gradient, and
  Flow (Rainbow). Legacy Rainbow Cycle remains accepted by the core loader.
- Renamed the misleading Breathing Blue preset to Blue Comet and made Static,
  Theater Chase, Color Wipe, Moving Dot, and the new effects use the RGB
  editor's selected primary color where applicable.
- Added a native `Choose color...` button for preset primary color selection.
- Added Previous/Next frame navigation, current/total status, draggable frame
  position and 0.25x through 4x preview speed. Speed is preview-only.
- Release x64 build: PASS, current version `1.0.0.036`, 0 warnings, 0 errors.
- Added persisted Color 2, Tail, Fade, Spacing and Repeat preset-generation
  controls. Current Release x64 build is `1.0.0.038`, 0 warnings, 0 errors.

## Pattern Preview Timing / Bottom Controls 2026-10-24

- Rainbow Cycle duration fix: editing `Duration(ms)` applies the clamped
  value to every frame, so preview and generated source no longer return to
  the preset's 80 ms duration after the first frame.
- Play and Stop are placed in a split row at the bottom of the left Pattern
  editor only; they do not occupy the Preview / Generated Code column.
- Release x64 build: PASS; duration change was built as `1.0.0.032`, and the
  corrected left-column button width was rebuilt as `1.0.0.033`; both builds
  completed with 0 warnings and 0 errors.
- Startup smoke: PASS; the window became responsive with title
  `Simple RGB LED Tool` and closed normally.
- Visual click-through: NOT RUN because the Windows computer-use native pipe
  was unavailable. Button geometry was source-reviewed but not screenshot
  verified.

## Layout / Clean Audit 2026-10-20

- Existing source layout already conforms; no application/protocol/firmware restructuring or dependencies were introduced.
- Replaced broad directory deletion with standalone `scripts/clean_intermediates.ps1`; batch compatibility remains.
- Build: PASS: scripts/build_mfc.ps1 -Configuration Release -Platform x64 -CloseRunningTool $false; 0 warnings/errors; normal generated version 1.0.0.031.
- Runtime: PASS: isolated captioned main frame/startup/normal close, exit 0; no connection/write.
- PASS: shared verifier `-CleanBatch scripts/clean_mfc.bat -CleanActualProject -CheckNestedDelivery -CheckReparseRejection`; 17 protected fixture build files plus source retained, object fixture removed, repeat-clean and pre-deletion junction rejection PASS. 2 actual protected build files unchanged (SHA-256).
- The initial expanded audit classified MSBuild `obj/*.recipe` compiler metadata as protected and reported FAIL; the classification was corrected against Microsoft.CppBuild.targets (DesktopDeployRecipeFile), then the audit passed. No delivery EXE was deleted.
- NOT RUN: physical hardware/firmware workflows, complete GUI/DPI, new Debug acceptance. Publication only updates already tracked deliverables; private INIs/data/logs and newly built test EXEs are excluded.
- Final commit/push state: workspace `#READ_ME_FIRST#/PROJECT_LAYOUT_MIGRATION.md`.

## Current State

- The project is a standalone MFC RGB LED controller and pattern editor.
- The repository uses `src`, `scripts`, `docs`, and `demo_code` directories.
- The primary Control tab can connect to VID 0416 / PID 0125 and send live strip configuration and effect commands.
- Connected HID device presence is polled every 500 ms; cable removal closes the stale handle and updates the UI to `Disconnected`.
- Control-tab Wire order, Effect, and Direction combo boxes reserve a full drop-list height and support keyboard tab navigation.
- Control-tab Speed and RGB values have synchronized numeric fields/sliders with clamped ranges. Effect selection enables only relevant inputs and explains color behavior.
- Control-tab `Choose color...` opens the native full visual color picker and synchronizes the selected color to all RGB numeric fields/sliders.
- Control-tab `Reset RGB / Speed` restores RGB 255/80/0 and Speed 128 while preserving Effect, Direction, brightness, and strip configuration.
- The portable INI now persists and restores Control-tab LED count, wire order, brightness, effect, RGB, speed, and direction through a `[CONTROL]` section.
- Firmware Gradient now animates the selected RGB color as a moving brightness gradient; Flow remains an independent rainbow effect.
- M032 firmware logs validated USB commands and decoded parameters on UART0. `GET_INFO` is documented as reserved/no-response; the protocol contract is synchronized in `docs\M032_USB_HID_PROTOCOL.md` and the firmware `USB_HID_PROTOCOL.md`.
- The GUI can preview, edit, save/load, and generate MCU source bundles in the Pattern / Code tab.
- Pattern / Code `Duration(ms)` now applies a manually entered interval to all
  frames, preventing multi-frame presets such as Rainbow Cycle from returning
  to their original 80 ms interval during playback. Play and Stop split the
  bottom row of the left Pattern editor without entering the preview column.
- The NUC1262 BSP `led_effect.c` was reviewed, but not modified.

## Key Files

- PC entry point and shell:
  - `src\app.*`
  - `src\main.*`
  - `src\main_frame.*`
- RGB LED UI:
  - `src\ui\rgb_led_tab.*`
- Pattern model and generation:
  - `src\core\rgb_pattern.*`
  - `src\core\app_state.*`
  - `src\core\text_utils.*`
- Docs:
  - `docs\RGB_PATTERN_FORMAT.md`
  - `docs\LED_EFFECT_REVIEW.md`
- MCU renderer sample:
  - `demo_code\NUC1262_LLSI_RGB_PATTERN_PLAYER\llsi_led_pattern_drv.*`
  - `demo_code\NUC1262_LLSI_RGB_PATTERN_PLAYER\llsi_led_pattern_player.*`

## Build

Use:

```bat
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_mfc.ps1 -Configuration Release -Platform x64
```

Current verification status:

- Debug and Release x64 builds pass with 0 warnings and 0 errors.
- Startup smoke testing reaches a responsive `Simple RGB LED Tool` window and closes normally.
- Startup state restoration is guarded until child controls have valid window handles.
- Linker PDB and other compiler intermediates are written under `build\obj`;
  the delivery root contains the executable and portable INI.

## Remaining Hardware Check

M032 USB/SPI/PDMA/UART behavior and physical LED timing still require validation
on the target board. Visual click-through of combo boxes and the native color
dialog should be included in release testing at supported DPI settings.
