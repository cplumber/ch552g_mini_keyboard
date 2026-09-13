# Firmware Optimization Notes

Current firmware is close to the flash limit, so feature removals and small logic
simplifications remain the safest wins. Auto-mode support was removed to make
room for persistent, transaction-safe profile-button configuration.

Latest build: 12,810 / 14,336 bytes flash (89%) and 467 / 876 bytes RAM (53%).
Keep at least the remaining 1,526 bytes of flash available for future changes.

## Biggest likely wins

1. Auto-mode support (implemented)
- `src/auto_mode.cpp` and its unused automatic keyboard/mouse routines were
  removed.

2. Replace menu division/modulo
- `src/keyboard.cpp` still uses `menu_mode_s % 3` and `menu_mode_s / 3` in the menu LED logic.
- On this toolchain, division and modulo can pull in helper routines.
- A small branch or lookup-table version should keep the same behavior with less code.

3. Keep the USB/HID layer stable
- `src/userUsbHidKeyboardMouse/USBHIDKeyboardMouse.c` and `src/userUsbHidKeyboardMouse/USBhandler.c` are large, but they are core functionality.
- They already handle keyboard, mouse, consumer control, suspend/resume, and LED feedback.
- Avoid changing these first unless there is a clear bug.

The configurable-macro storage uses one 76-byte slot: twelve six-byte records
plus a four-byte header. A power failure can invalidate the slot; firmware then
uses built-in defaults, and the next successful import repairs it. Do not add
firmware-side JSON parsing or broad macro validation; that
belongs in `macropad-config.exe`.

## Smaller possible cleanups

- Review helper delays only if behavior allows it.
  - These usually affect responsiveness more than flash size.
  - They are not the first thing to trim if the goal is code-space.
- Keep button shortcut data in `configuration.cpp` compact and avoid adding new helper paths unless a profile really needs them.
- Prefer one-shot helpers over generic sequence engines when a shortcut is only used once.

## What not to optimize first

- Encoder scanning logic
- Google Meet shortcuts
- VS Code shortcuts
- suspend/resume LED off
- Windows bridge behavior

Those are already part of the working user-facing behavior. The safest approach is to remove unused legacy features before touching active ones.
