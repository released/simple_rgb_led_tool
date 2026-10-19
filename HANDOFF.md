# HANDOFF

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
