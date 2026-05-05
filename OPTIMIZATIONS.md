# Firmware Optimization Notes

Current firmware is very close to the flash limit, so the safest wins are feature removals and small logic simplifications. This note collects options only. Nothing here is implemented yet.

## Biggest likely wins

1. Remove dead auto-mode support
- `src/auto_mode.cpp` is still compiled, but none of the active configs use `BUTTON_AUTO_KEYBOARD` or `BUTTON_AUTO_MOUSE`.
- If auto-repeat is no longer needed, this is the cleanest code-size win.
- Menu exit currently uses `auto_set_cycle(button_function_null)` only as a reset path, so that reset could be replaced with a tiny local helper.

2. Replace menu division/modulo
- `src/keyboard.cpp` still uses `menu_mode_s % 3` and `menu_mode_s / 3` in the menu LED logic.
- On this toolchain, division and modulo can pull in helper routines.
- A small branch or lookup-table version should keep the same behavior with less code.

3. Keep the USB/HID layer stable
- `src/userUsbHidKeyboardMouse/USBHIDKeyboardMouse.c` and `src/userUsbHidKeyboardMouse/USBhandler.c` are large, but they are core functionality.
- They already handle keyboard, mouse, consumer control, suspend/resume, and LED feedback.
- Avoid changing these first unless there is a clear bug.

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

