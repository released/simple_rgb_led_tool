# Simple RGB LED Tool

Windows MFC GUI tool for live USB HID control, editing RGB LED patterns, and generating C code for MCU output to RGB LEDs.

The primary `Control` tab connects to the companion M032TD2AE USB HID
firmware. It controls 1-10 WS2812 (GRB) or LTST-E563CEGBW (RGB) LEDs,
brightness, static color, breathing, blink, gradient, and flow. The existing
editor and code generator are retained in the compact `Pattern / Code` tab.
While connected, the tool checks device presence every 500 ms. Unplugging the
USB cable closes the stale HID handle, changes Status to `Disconnected`, and
re-enables the Connect button without requiring a failed command first.

The Control tab provides numeric fields and sliders for Speed and RGB values,
plus `Choose color...` for the native Windows visual color picker. A selected
color immediately updates all three RGB fields and sliders. RGB values are
clamped to 0-255 and Speed to 1-255. Static, Breathing, Blink, and Gradient use
the selected RGB color. Gradient moves a brightness gradient of that color;
Flow uses its own rainbow color wheel and therefore disables the RGB controls,
including the color picker. Direction applies to Gradient and Flow.
Continuous effects map Speed 1-255 to approximately 17-1 seconds per cycle;
Blink maps it to approximately 1070-50 ms per on/off transition.
`Reset RGB / Speed` restores RGB to `255,80,0` and Speed to `128` without
changing the selected effect, direction, brightness, or strip configuration.

The portable `build\rgb_led_tool.ini` now stores the Control tab LED count,
wire order, brightness, effect, RGB, speed, and direction in `[CONTROL]`.
Settings are loaded at startup and saved by `Save INI` or automatically when
the tool closes. Invalid numeric values are clamped when loaded.

The tool can generate backend-specific source bundles for Nuvoton BSP projects
using LLSI, SPI, SPI + PDMA, or GPIO LED output.

## Layout And Safe Clean

Application source remains in `src`, automation in `scripts`, supporting guides
in `docs`, and delivery/runtime files in `build`. Existing firmware/vendor
trees retain their working paths. `scripts/clean_mfc.bat` removes allowlisted
compiler files only, rejects reparse-point traversal before deletion, and
retains EXE/INI/DLL/data/log/unknown files even inside nested output folders.
A folder name such as `tests` or `obj` is not permission to erase its contents.

## Current Scope

- Standalone PC GUI with native Windows HID access; no third-party driver or DLL.
- LED count from 1 to 64.
- Preview layouts:
  - Line
  - Ring
  - Square
  - U Shape
- Pattern editing:
  - path start LED and direction remap for current frames
  - loop/bounce motion for generated presets
  - frame duration
  - per-LED RGB color
  - fill all LEDs
  - rainbow fill
  - shift left/right
  - add, duplicate, and delete frames
- Presets:
  - Moving Dot
  - Rainbow Cycle
  - Theater Chase
  - Color Wipe Red
  - Breathing Blue
  - Alert Alternating
  - Solid Red/Green/Blue/White
  - Off
- Pattern file load/save with `.rgbpat`.
- Generated MCU source bundle with frame tables, a pattern player, and a backend-specific LED driver.
- Output backends:
  - LLSI
  - SPI
  - SPI + PDMA
  - GPIO

## Quick Start

1. Build or launch `build\RgbLedTool.exe`.
2. Set `LED Count` and `Shape`, then click `Apply Layout`.
3. Set `Start`, `Dir`, and `Motion` if a preset should begin from a specific LED or bounce at strip ends.
4. Pick `Output`: `LLSI`, `SPI`, `SPI + PDMA`, or `GPIO`.
5. Pick a `Preset` and click `Load Preset`, or edit frames manually.
6. Use `Play` to preview the frame sequence.
7. Use `Copy Code` to copy the full source bundle, or `Save Code` to choose an output folder.
8. Add the generated files to the BSP project. The filename prefix follows the selected backend:
   - `llsi_...`
   - `spi_...`
   - `spi_pdma_...`
   - `gpio_...`
9. The generated driver header defines `LED_NUM` from the GUI LED count and derives `LED_DATA_LEN`.
10. The BSP still provides backend hooks:
   - `LLSI_PDMA_Transfer()`
   - `delay()` / `delay_ms()`
   - SPI write or SPI PDMA transfer hook, when using SPI backends
   - GPIO pin macros, when using GPIO backend

If the BSP already owns these macros, define them before including the generated driver header; otherwise the generated defaults are used.

If a target BSP keeps project-wide LED definitions in a configuration header,
define those macros before including the generated driver header.

For LLSI, `LED_DATA_LEN` is the number of 32-bit words needed by RGB bytes. The generated formula is `((LED_NUM * 3u) + 3u) / 4u`. For 24 LEDs, this becomes `18`. For SPI/GPIO backends, `LED_DATA_LEN` is `LED_NUM * 3u` raw RGB bytes.

## Backend Hooks

- LLSI: generated code calls `LLSI_PDMA_Transfer(uint32_t *buffer, uint32_t len)` and `delay_ms()`.
- SPI: generated code expands each RGB data bit to `SPI_HIGH_CODE` / `SPI_LOW_CODE` bytes and calls `SPI_LED_PATTERN_DRV_WriteByte()` by default. Map `SPI_LED_PATTERN_DRV_WRITE_BYTE(data)` to an existing sample function such as `SPI_Send_Data(data)` if preferred.
- SPI + PDMA: generated code builds an expanded SPI buffer and calls `SPI_PDMA_LED_PATTERN_DRV_Transfer(uint8_t *data, uint16_t len)` by default. For the NUC1262 multi-port sample, map `SPI_PDMA_LED_PATTERN_DRV_TRANSFER(data, len)` to the target `SPI_PDMA_Start(...)` call.
- GPIO: generated code includes the bit-bang timing in `gpio_led_pattern_drv.c`. Defaults match the MS51 GPIO P12 sample style (`P12`, `P12_PUSHPULL_MODE`, `nop`, `clr_EA`, `set_EA`); edit the generated macros for the actual pin and clock timing.

## Build PC Tool

Supported PC platform:

- Windows 10/11 x64
- Visual Studio 2022 C++ toolchain with MFC support
- Native MFC executable

Build:

```bat
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_mfc.ps1 -Configuration Release -Platform x64
```

Output:

- `build\RgbLedTool.exe`
- `build\obj\x64\Release\...` intermediate build files

Clean:

```bat
scripts\clean_mfc.bat
```

## Pattern Files

`.rgbpat` is a small text format:

```text
RGB_PATTERN_V1
name=Moving Dot
led_count=24
shape=Line
start_index=12
direction=Reverse
path_mode=Bounce
frame 0 duration=75
leds=00B4FF 000000 ...
```

See `docs\RGB_PATTERN_FORMAT.md`.

## MCU Pattern Direction

The reviewed BSP `led_effect.c` is mostly function-driven and blocking. That is good for demos, but difficult for a GUI to generate safely. The recommended path is:

1. Generate backend-specific files with an explicit prefix such as `llsi_`, `spi_`, `spi_pdma_`, or `gpio_`.
2. Keep peripheral packing and transfer in `<backend>_led_pattern_drv.c`.
3. Move pattern frames into generated const data tables in `<backend>_led_pattern_player.c`.
4. Use the generated player to display one frame or step through a pattern.

See `docs\LED_EFFECT_REVIEW.md` and `demo_code\NUC1262_LLSI_RGB_PATTERN_PLAYER`.
