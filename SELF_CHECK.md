# SELF_CHECK

## Rainbow Fill Animation Fix 2026-10-24

- Static check: PASS; Rainbow Fill transforms every frame and scales the
  rainbow by the original per-LED intensity relative to the pattern peak.
- Release x64 build `1.0.0.042`: PASS, 0 warnings, 0 errors.
- Startup smoke: PASS; responsive main window and normal close.
- Breathing/Rainbow Fill visual click-through: NOT RUN.

## Applied Layout/Preset Status And Reset Color Fix 2026-10-24

- Build artifact: PASS, version `1.0.0.041`; MSBuild unsuccessful flag absent.
- Startup smoke: PASS.
- Static check: PASS; Apply/Load status derives from the active pattern.
- Static check: PASS; Reset selects LED 0 before and after default pattern load,
  then refreshes RGB fields, preserving the default orange primary color.
- Reset/Breathing/Rainbow Fill visual click-through: NOT RUN.

## Pattern Reset 2026-10-24

- Release x64 build: PASS, version `1.0.0.040`, 0 warnings, 0 errors.
- Static check: PASS; Reset kills the preview timer, clears playing state,
  loads default Pattern / Code state, restores 1x preview speed, and refreshes
  playback controls.
- Reset click-through: NOT RUN.

## Pattern / Code Groups 2026-10-24

- Release x64 build: PASS, version `1.0.0.039`, 0 warnings, 0 errors.
- Startup smoke: PASS.
- Static check: PASS; C Symbol, Output, Copy Code and Save Code are owned and
  laid out by the Code Output group; duplicate main-frame Copy/Save controls
  and handlers were removed.
- Clipboard and folder-picker click-through: NOT RUN.

## Offline Preset Alignment 2026-10-24

- Release x64 build: PASS, current version `1.0.0.038`, 0 warnings, 0 errors.
- Static check: PASS; Pattern / Code includes Static, Breathing, Blink,
  Gradient, and Flow as offline frame generators and does not invoke USB.
- Static check: PASS; old Rainbow Cycle and Breathing Blue names remain
  accepted by the core for persisted-state compatibility.
- Native color-picker click-through: NOT RUN; Windows computer-use native pipe
  unavailable.
- Preview navigation static check: PASS; Previous/Next, slider and status use
  the current frame, while preview-speed scaling does not mutate durations.
- Preset-parameter static check: PASS; Color 2, Tail, Fade, Spacing and Repeat
  are passed only when Load Preset regenerates frames and persist through INI.

## Pattern Preview Timing / Bottom Controls 2026-10-24

- Release x64 build: PASS, output `build\RgbLedTool.exe`, current version
  `1.0.0.033`, 0 warnings, 0 errors.
- Startup smoke: PASS; responsive main window and normal close.
- Duration propagation static check: PASS; user edits update all frames before
  preview playback and generated-code refresh.
- Bottom-row layout static check: PASS; Play and Stop split only the left
  Pattern editor width and do not reduce or overlap the generated-code view.
- Visual click-through: NOT RUN; Windows computer-use native pipe unavailable.

## Layout / Clean Audit 2026-10-20

- Existing source layout already conforms; no application/protocol/firmware restructuring or dependencies were introduced.
- Replaced broad directory deletion with standalone `scripts/clean_intermediates.ps1`; batch compatibility remains.
- Build: PASS: scripts/build_mfc.ps1 -Configuration Release -Platform x64 -CloseRunningTool $false; 0 warnings/errors; normal generated version 1.0.0.031.
- Runtime: PASS: isolated captioned main frame/startup/normal close, exit 0; no connection/write.
- PASS: shared verifier `-CleanBatch scripts/clean_mfc.bat -CleanActualProject -CheckNestedDelivery -CheckReparseRejection`; 17 protected fixture build files plus source retained, object fixture removed, repeat-clean and pre-deletion junction rejection PASS. 2 actual protected build files unchanged (SHA-256).
- The initial expanded audit classified MSBuild `obj/*.recipe` compiler metadata as protected and reported FAIL; the classification was corrected against Microsoft.CppBuild.targets (DesktopDeployRecipeFile), then the audit passed. No delivery EXE was deleted.
- NOT RUN: physical hardware/firmware workflows, complete GUI/DPI, new Debug acceptance. Publication only updates already tracked deliverables; private INIs/data/logs and newly built test EXEs are excluded.
- Final commit/push state: workspace `#READ_ME_FIRST#/PROJECT_LAYOUT_MIGRATION.md`.

## Static Checks

- Confirm the Control tab uses native Windows HID for VID 0416 / PID 0125.
- Confirm unplugging the connected USB device changes Status to `Disconnected` within approximately one second and logs one removal message.
- Confirm the USB protocol document marks `GET_INFO` reserved/no-response and lists exact payload lengths for commands `0x01..0x06`.
- Confirm Wire order, Effect, and Direction combo boxes expand and can be reached with keyboard Tab navigation.
- Confirm Red/Green/Blue clamp to 0-255 and Speed clamps to 1-255, with edit/slider values synchronized.
- Confirm `Choose color...` opens the visual color picker and updates all RGB edits/sliders.
- Confirm `Reset RGB / Speed` restores RGB to 255/80/0 and Speed to 128 without changing Effect or Direction.
- Confirm `[CONTROL]` in the portable INI saves and restores LED count, wire order, brightness, effect, RGB, speed, and direction.
- Confirm Static enables RGB only; Breathing/Blink enable RGB+Speed; Gradient enables RGB+Speed+Direction; Flow enables Speed+Direction and disables RGB.
- Confirm Pattern / Code retains the existing editor and generator.
- Confirm editing Pattern / Code `Duration(ms)` updates every frame, and that
  Play uses the new interval across a complete multi-frame preset cycle.
- Confirm Play and Stop share the bottom row of the left Pattern editor without
  entering or overlapping the Preview / Generated Code column.
- Confirm supported LED count is clamped to 1 through 64.
- Confirm shape choices are Line, Ring, Square, and U Shape.
- Confirm preset path choices support Start, Forward/Reverse, and Loop/Bounce.
- Confirm `.rgbpat` files use `RGB_PATTERN_V1`.
- Confirm generated MCU code declares backend-prefixed driver/player files for LLSI, SPI, SPI + PDMA, and GPIO.
- Confirm generated MCU driver header defines `LED_NUM` and `LED_DATA_LEN`.
- Confirm GPIO generated code keeps pin write and timing macros inside `gpio_led_pattern_drv.h`.
- Confirm generated MCU code uses English comments.
- Confirm BSP review is documented without modifying unrelated BSP code.

## Build Checks

- PC GUI Release x64 build: PASS, output `build\RgbLedTool.exe`, 0 warnings, 0 errors.
- PC GUI Debug x64 build: PASS, 0 warnings, 0 errors.
- Debug and Release startup smoke: PASS; both reached the responsive `Simple RGB LED Tool` main window for three seconds and were then closed.
- Combo-box layout static check: PASS; Control-tab Wire order/Effect/Direction reserve 200/180/120-pixel drop-list heights, and all Pattern/Code combo boxes reserve 120-220 pixels.
- Control-tab drop-down click-through: MANUAL; verify with a visible desktop at 100% and 125% DPI.
- Visual color-picker click-through: MANUAL; verify selection, cancel, and RGB/slider synchronization.
- Control-tab INI state static check: PASS; load/save wiring and `[CONTROL]` serialization cover all requested fields with numeric clamping on load.

## Firmware Checks

- Generated code syntax inside target BSPs: NOT RUN. Requires selecting/exporting a backend and adding it to the target Keil/GCC firmware project.
- Hardware LLSI/SPI/SPI+PDMA/GPIO validation: NOT RUN. Requires target boards and RGB LED strip/ring.
- M032 USB/SPI/PDMA/UART hardware validation: NOT RUN. Requires M032TD2AE target and LEDs.
- UART command-log hardware capture: NOT RUN; requires the target board and PB.13 UART connection.

## Known Limits

- The generated `play_once()` renderer is blocking because it calls the generated backend driver delay wrapper.
- For production UI-controlled firmware, use the non-blocking step architecture described in `docs\LED_EFFECT_REVIEW.md`.
