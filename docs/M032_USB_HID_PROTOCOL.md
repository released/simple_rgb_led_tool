# M032 RGB USB HID Protocol

The PC tool and firmware use VID `0416`, PID `0125`, and one 64-byte vendor
defined HID input/output report. Windows API buffers include a leading report
ID byte of zero; that byte is not part of the 64-byte protocol frame.

| Offset | Size | Meaning |
|---|---:|---|
| 0 | 1 | Magic `A5` |
| 1 | 1 | Protocol version `01` |
| 2 | 1 | Command |
| 3 | 1 | Sequence |
| 4 | 1 | Payload length, maximum 58 |
| 5 | 58 | Payload / zero padding |
| 63 | 1 | CRC-8, polynomial `07`, initial value 0 |

Commands:

- `01 GET_INFO`: no payload.
- `02 SET_CONFIG`: LED count, wire order (`0=GRB`, `1=RGB`), brightness.
- `03 SET_STATIC`: red, green, blue.
- `04 SET_PIXELS`: LED count followed by RGB triples, maximum 10 LEDs.
- `05 SET_EFFECT`: effect, red, green, blue, speed, brightness, direction.
- `06 STOP`: no payload and turns every LED off.

Effects are `0=Static`, `1=Breathing`, `2=Blink`, `3=Gradient`, `4=Flow`.
Direction is `0=Forward`, `1=Reverse`.

Implementation status:

- `SET_CONFIG`, `SET_STATIC`, `SET_PIXELS`, `SET_EFFECT`, and `STOP` are implemented.
- `GET_INFO` is reserved: firmware logs it on UART, but EP2 does not return an
  information report yet.
- Static, Breathing, Blink, and Gradient use the supplied RGB fields. Flow uses
  an internal rainbow color wheel and ignores RGB.
- Speed is used by Breathing, Blink, Gradient, and Flow. Direction is used only
  by Gradient and Flow.
- UART0 debug output is PB.12 RXD / PB.13 TXD at 115200 8-N-1. Valid commands,
  invalid frames, and unsupported command/length combinations are logged from
  the firmware main loop.
