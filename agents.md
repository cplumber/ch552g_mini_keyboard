# Agent Notes for `ch552g_mini_keyboard`

This repo is a CH552G USB macro keyboard firmware project with a Windows mic-mute bridge.

## Project Shape

- `ch552g_mini_keyboard.ino` is the firmware entrypoint.
- `src/` holds the firmware modules.
- `mic_mute_bridge/` holds the Windows helper that syncs mic mute state.
- `configuration.cpp` defines the keyboard profiles.
- `scripts/` contains the build and memory-map scripts.

## Behavior Overview

- There are three normal keyboard profiles plus a menu profile.
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
- VS Code `BTN_1` sends the preview chord `Ctrl+K`, then `V`.

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
- [`src/auto_mode.cpp`](src/auto_mode.cpp) for auto-sequence routines
- [`src/userUsbHidKeyboardMouse/USBhandler.c`](src/userUsbHidKeyboardMouse/USBhandler.c) and [`src/userUsbHidKeyboardMouse/USBHIDKeyboardMouse.c`](src/userUsbHidKeyboardMouse/USBHIDKeyboardMouse.c) for USB report handling
- [`mic_mute_bridge/main.cpp`](mic_mute_bridge/main.cpp) for Windows Core Audio mute sync
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

- `mic_mute_bridge\build.bat`

## Invariants

- Keep the third LED off unless the user explicitly asks to use it.
- Do not change the physical LED mapping without updating comments and docs.
- Preserve the existing encoder actions unless the task explicitly changes them.
- Keep the three user profiles plus menu profile behavior intact unless the task explicitly changes it.
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

When changing profile logic, update:

- `configuration.cpp`
- `src/keyboard.cpp`
- `readme.md`
- `agents.md`

When changing the bridge protocol, update:

- `mic_mute_bridge/main.cpp`
- `src/userUsbHidKeyboardMouse/USBhandler.c`
- `src/userUsbHidKeyboardMouse/USBHIDKeyboardMouse.c`
- `readme.md`
- `agents.md`
