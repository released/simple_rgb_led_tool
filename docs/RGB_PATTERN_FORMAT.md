# RGB Pattern Format

## `.rgbpat`

Pattern files are plain text and start with:

```text
RGB_PATTERN_V1
```

Fields:

- `name`: user-facing pattern name.
- `led_count`: number of RGB LEDs, clamped by the tool to 1 through 64.
- `shape`: preview shape, one of `Line`, `Ring`, `Square`, or `UShape`.
- `start_index`: first physical LED index for the pattern path.
- `direction`: pattern path direction, `Forward` or `Reverse`.
- `path_mode`: preset endpoint behavior, `Loop` or `Bounce`.
- `frame N duration=MS`: starts one frame.
- `leds`: space-separated `RRGGBB` values for the current frame.

`start_index`, `direction`, and `path_mode` are optional for older files. Missing fields load as `0`, `Forward`, and `Loop`.

Example:

```text
RGB_PATTERN_V1
name=Example
led_count=4
shape=Line
start_index=0
direction=Forward
path_mode=Loop
frame 0 duration=100
leds=FF0000 00FF00 0000FF 000000
frame 1 duration=100
leds=000000 FF0000 00FF00 0000FF
```

## Generated C

The GUI exports a backend-specific source bundle. The filename prefix follows the selected `Output` backend:

- LLSI: `llsi_led_pattern_drv.*`, `llsi_led_pattern_player.*`
- SPI: `spi_led_pattern_drv.*`, `spi_led_pattern_player.*`
- SPI + PDMA: `spi_pdma_led_pattern_drv.*`, `spi_pdma_led_pattern_player.*`
- GPIO: `gpio_led_pattern_drv.*`, `gpio_led_pattern_player.*`

The generated driver owns the LED data buffer and pixel packing. The LLSI backend expects the BSP to provide:

```c
void LLSI_PDMA_Transfer(uint32_t *LLSIDataBuffer, uint32_t len);
void delay_ms(uint16_t ms);
```

The generated driver header defines `LED_NUM` from the GUI LED count. LLSI derives `LED_DATA_LEN` as:

```c
#define LED_DATA_LEN (((LED_NUM * 3u) + 3u) / 4u)
```

If the BSP already owns these macros, define them before including the generated driver header; otherwise the generated defaults are used.

For SPI, SPI + PDMA, and GPIO, `LED_DATA_LEN` is generated as:

```c
#define LED_DATA_LEN (LED_NUM * 3u)
```

SPI backends also define an expanded SPI transfer length:

```c
#define SPI_LED_PATTERN_SPI_DATA_LEN (LED_DATA_LEN * 8u)
#define SPI_PDMA_LED_PATTERN_SPI_DATA_LEN (LED_DATA_LEN * 8u)
```

## Backend Integration Hooks

- LLSI calls `LLSI_PDMA_Transfer()` and `delay_ms()`.
- SPI calls `SPI_LED_PATTERN_DRV_WriteByte()` by default. Define `SPI_LED_PATTERN_DRV_WRITE_BYTE(data)` to map it to a sample function or register macro.
- SPI + PDMA calls `SPI_PDMA_LED_PATTERN_DRV_Transfer()` by default. Define `SPI_PDMA_LED_PATTERN_DRV_TRANSFER(data, len)` to map it to `SPI_PDMA_Start(...)` for the target BSP.
- GPIO includes bit-bang send code in `gpio_led_pattern_drv.c`; the generated header defaults to MS51 P12-style macros and should be edited for the selected pin and clock.

In the current NUC1262 BSP, existing project-level definitions are in:

```text
SampleCode\Template\project_config.h
```

The player file declares:

- `llsi_led_pattern_color_t`
- `llsi_led_pattern_frame_t`
- `<symbol>_render_frame()`
- `<symbol>_play_once()`
- `<symbol>_step()`

The generated filenames use backend prefixes so future PWM support can use a separate prefix.

## Recommended Integration

For a quick demo, call:

```c
g_rgb_pattern_play_once();
```

from the BSP's LED effect loop.

For production firmware, avoid blocking the main loop. Use `<symbol>_step()` from a timer-driven state machine instead of repeatedly calling `<symbol>_play_once()`.

Reference files are under:

```text
demo_code\NUC1262_LLSI_RGB_PATTERN_PLAYER
```
