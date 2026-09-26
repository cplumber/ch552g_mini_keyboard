# Firmware Optimization Notes

Current firmware is close to the flash limit, so feature removals and small logic
simplifications remain the safest wins. The notes below describe only the
current codebase and the remaining low-risk options.

Latest builds:

- Three-key: 13,956 / 14,336 bytes flash (97%) and 545 / 876 bytes RAM (62%).
- Six-key: 14,290 / 14,336 bytes flash (99%) and 554 / 876 bytes RAM (63%).

The six-key build has only 46 bytes of flash remaining, so future changes must
be measured against that variant.

## Biggest likely wins

1. Keep the USB/HID layer stable
- `src/userUsbHidKeyboardMouse/USBHIDKeyboardMouse.c` and `src/userUsbHidKeyboardMouse/USBhandler.c` are large, but they are core functionality.
- They already handle keyboard, mouse, consumer control, suspend/resume, and LED feedback.
- Avoid changing these first unless there is a clear bug.

The configurable-macro storage uses one 124-byte slot: thirty four-byte records
(five profiles × six buttons) plus a four-byte header. A power failure can invalidate the slot; firmware then
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
