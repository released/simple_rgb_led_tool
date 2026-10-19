# AGENTS.md

## Project Purpose

This project builds a standalone Windows MFC tool for RGB LED pattern editing.

- Users choose LED count and layout shape.
- Users preview RGB frames before exporting.
- Users can load/save `.rgbpat` pattern files.
- The tool generates C pattern code for NUC1262 LLSI RGB LED firmware.

## Local Rules

- Keep the layout aligned with the sibling simple tools:
  - `src\`
  - `scripts\`
  - `docs\`
  - `demo_code\`
  - root `README.md`, `AGENTS.md`, `SELF_CHECK.md`, `HANDOFF.md`
- Keep the PC app standalone. Do not add USB HID bridge behavior unless explicitly requested.
- Generated MCU C should stay C90-friendly where practical:
  - declare variables at the beginning of blocks
  - use English comments
  - avoid unrelated firmware refactors
- Preserve `.rgbpat` compatibility unless a versioned format change is documented.

## Verification Expectations

For PC GUI changes, build:

```bat
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_mfc.ps1 -Configuration Release -Platform x64
```

Expected output:

- `build\RgbLedTool.exe`

For generated MCU code, compile it inside the target BSP after adding the generated declarations to the firmware project.
