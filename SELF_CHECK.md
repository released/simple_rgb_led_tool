# SELF_CHECK

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
