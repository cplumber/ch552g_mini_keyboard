# Agent Notes for `ch552g_mini_keyboard`

This repo is a CH552G USB macro keyboard firmware project with a Windows mic-mute bridge.

## Project Shape

- `ch552g_mini_keyboard.ino` is the firmware entrypoint.
- `src/` holds the firmware modules.
- `macropad_tools/` holds the Windows helper and configuration tool.
- `configuration.cpp` defines the keyboard profiles.
- `scripts/` contains the build and memory-map scripts.

## Behavior Overview

- There are four normal keyboard profiles plus a menu profile.
- Short encoder click toggles Windows microphone mute through the bridge.
- Long encoder press enters profile selection mode.
- Mic mute/live state is shown on the LED closest to the rotary switch.
- Menu/profile selection is shown on the middle LED.
- The remaining LED stays off.
- Profile selection is persisted in DataFlash so it survives power cycles.
- Menu/profile colors map to the current profiles:
  - Copy / paste = red
  - Google Meet = yellow
  - VS Code = green
  - MS Teams (web) = cyan
- The default VS Code `BTN_1` macro sends the preview chord `Ctrl+K`, then `V`.
- The default VS Code `BTN_3` macro sends Copy Relative Path: `Ctrl+K`, then
  `Ctrl+Shift+C`.
- `macropad-config.exe bootloader` requests the CH552 USB bootloader for an
  automated upload. Export the macro configuration before flashing because an
  upload can erase DataFlash.
- The four profile sets use persistent two-chord macros; defaults live in
  `src/macro_config.c` and configuration traffic uses vendor HID report ID `6`.
- Board selection is compile-time: `scripts/build.ps1 -BoardVariant three_key`
  (default) or `six_key`. The six-key build uses the left vertical button column
  for BTN_1..BTN_3, keeps the knob behavior, ignores the right column, and uses
  P1.5/SW2 as a startup/replug bootloader request. P1.5 is shared with the
  right-bottom switch, so it must not be polled while the application runs.
- Report ID `6` uses 8 data bytes plus its report ID; keep the USB endpoint
  packet size in `src/userUsbHidKeyboardMouse/USBconstant.h` at 9 bytes.
- `macropad_tools/common/` provides shared HID transport for both Windows
  executables. Keep mic-state feature report ID `5` unchanged.

## LED Rules

- Use numbered LED defines only.
- `LED_0` = farthest from the rotary switch, off
- `LED_1` = middle LED, menu/profile indicator
- `LED_2` = closest to the rotary switch, mic mute/live indicator
- Keep the physical-location comments in `src/led.h` and do not rename these into semantic labels.

Mic indicator rules:

- live = solid green
- muted = slow blinking yellow

## Important Files

- [`src/led.cpp`](src/led.cpp) for LED rendering, menu colors, and blink timing
- [`src/keyboard.cpp`](src/keyboard.cpp) for menu selection and encoder logic
- [`src/macro_config.c`](src/macro_config.c) for default button macros, execution
  timing, and single-slot DataFlash configuration
- [`src/userUsbHidKeyboardMouse/USBhandler.c`](src/userUsbHidKeyboardMouse/USBhandler.c) and [`src/userUsbHidKeyboardMouse/USBHIDKeyboardMouse.c`](src/userUsbHidKeyboardMouse/USBHIDKeyboardMouse.c) for USB report handling
- [`macropad_tools/main.cpp`](macropad_tools/main.cpp) for Windows Core Audio mute sync
- [`configuration.cpp`](configuration.cpp) for profile definitions
- [`readme.md`](readme.md) for user-facing behavior and wiring notes

## Build And Test

Build firmware:

```powershell
powershell -File .\scripts\build.ps1
```

Build output:

- `build/CH55xDuino.mcs51.ch552/ch552g_mini_keyboard.ino.hex`

Memory map report:

```powershell
powershell -File .\scripts\map-report.ps1
```

Bridge build:

- `macropad_tools\build.bat`

Build outputs:

- Firmware: 12,810 / 14,336 bytes flash (89%); 467 / 876 bytes RAM (53%).
- Windows: `macropad_tools\build\mic-mute-bridge.exe` and
  `macropad_tools\build\macropad-config.exe`.

## Invariants

- Keep the third LED off unless the user explicitly asks to use it.
- Do not change the physical LED mapping without updating comments and docs.
- Preserve the existing encoder actions unless the task explicitly changes them.
- Keep the four user profiles plus menu profile behavior intact unless the task explicitly changes it.
- Avoid reverting user edits in unrelated files.

## Common Gotchas

- The NeoPixel count is configured in `src/neo/config.h`.
- The menu profile is part of the same firmware configuration array, but it is not a normal user profile.
- The Windows helper is what keeps the mic LED honest; firmware alone does not know the OS mute state.

## If You Change Behavior

When changing LED behavior, update:

- `src/led.cpp`
- `src/led.h`
- `readme.md`
- `agents.md`
- `src/macro_config.h` when adding configuration commands
- `macropad_tools/common/macropad_hid.*` and `macropad_tools/config_tool/main.cpp`

When changing profile logic, update:

- `configuration.cpp`
- `src/keyboard.cpp`
- `readme.md`
- `agents.md`

When changing the bridge protocol, update:

- `macropad_tools/main.cpp`
- `src/userUsbHidKeyboardMouse/USBhandler.c`
- `src/userUsbHidKeyboardMouse/USBHIDKeyboardMouse.c`
- `readme.md`
- `agents.md`
