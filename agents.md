# Agent Notes for `ch552g_mini_keyboard`

This repo is a CH552G USB macro keyboard firmware project with a Windows mic-mute bridge.

## Project Shape

- `ch552g_mini_keyboard.ino` is the firmware entrypoint.
- `src/` holds the firmware modules.
- `macropad_tools/` holds the Windows helper and configuration tool.
- `configuration.cpp` defines the keyboard profiles.
- `scripts/` contains the build and memory-map scripts.

## Behavior Overview

- There are five normal keyboard profiles plus a menu profile.
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
- Test = blue
- The default VS Code `BTN_1` macro sends the preview chord `Ctrl+K`, then `V`.
- The default VS Code `BTN_3` macro sends Copy Relative Path: `Ctrl+K`, then
  `Ctrl+Shift+C`.
- `macropad-config.exe bootloader` requests the CH552 USB bootloader from the
  running firmware. This is the preferred automated upload path: no physical
  button press is required. Export the macro configuration before flashing
  because an upload can erase DataFlash.
- The five profile sets use persistent two-chord macros; defaults live in
  `src/macro_config.c` and configuration traffic uses vendor HID report ID `6`.
- Board selection is compile-time: `scripts/build.ps1 -BoardVariant three_key`
  (default) or `six_key`. The six-key build uses the left vertical button column
  for BTN_1..BTN_6, and the build uses
  P1.5/SW2 as a startup/replug bootloader request. While running, P1.5 is also
  polled as BTN_6; a BTN_6 press does not request bootloader mode.
- Report ID `6` uses 8 data bytes plus its report ID; keep the USB endpoint
  packet size in `src/userUsbHidKeyboardMouse/USBconstant.h` at 9 bytes.
- `macropad_tools/common/` provides shared HID transport for both Windows
  executables. Keep mic-state feature report ID `5` unchanged.

## Firmware, Profiles, and Persistent Data

- `configuration.cpp` defines five user profiles plus the menu profile:
  Copy/paste (red), Google Meet (yellow), VS Code (green), and MS Teams web
  (cyan), and Test (blue). Do not remove the menu profile or alter the profile order unless
  explicitly requested.
- `src/keyboard.cpp` owns encoder actions and profile-menu state. A short
  encoder click sends `F24` for mic mute; a roughly one-second hold enters the
  profile menu. In menu mode, encoder rotation selects a profile and the
  release of the encoder chooses it.
- `src/macro_config.c` owns defaults, macro execution timing, DataFlash layout,
  and the single persistent configuration slot. Macro configuration survives
  power cycles but an upload can erase DataFlash. The slot starts at DataFlash
  address 0; the menu-selection byte is at address 127, with no reserved gap.
- `BTN_1`..`BTN_6` and `BTN_ENC` are logical controls; `BTN_4`..`BTN_6` exist for the
  six-key right-column shortcuts and is ignored by the three-key hardware.
  Board GPIO assignments belong in `configuration.h` / `src/board_config.h`; profile
  macro definitions belong in `configuration.cpp` and `src/macro_config.c`.
- Vendor HID report ID `6` transfers eight data bytes plus its report-ID byte.
  Its USB endpoint packet size must remain nine bytes in
  `src/userUsbHidKeyboardMouse/USBconstant.h`.
- Mic status is feature report ID `5`. It is supplied by the Windows bridge;
  firmware cannot independently know Windows microphone state.

## Windows Tools and Configuration Safety

- `macropad_tools\build\mic-mute-bridge.exe` listens for `F24`, toggles the
  Windows default microphone through Core Audio, and reports mic state to the
  keyboard LED.
- `macropad_tools\build\macropad-config.exe` exports, imports, resets, and
  requests bootloader mode for persistent macros. Export configuration before
  flashing and import it afterward if the DataFlash contents were erased.
- `macropad-config.exe board` queries the compile-time board ID embedded in the
  running firmware and prints `three_key` or `six_key`. This works only when an
  application firmware is running; a blank device or the ROM bootloader cannot
  identify its board automatically.
- UID-capable firmware also reports the CH552G factory 40-bit chip UID. Prefix
  commands with `--uid HEX10` to select one keyboard when several are connected.
  Older firmware must be updated once while connected alone.
- `macropad-config.exe list` enumerates every connected UID-capable keyboard and
  prints its board type and UID.
- Build both Windows tools with `macropad_tools\build.bat`.
- Protocol changes require matching updates on both firmware and Windows sides;
  do not change report IDs, packet sizes, or command payloads in one place.

## Board Selection and Pin Ownership

- `three_key` is the default build. Its button pins are BTN_1=P1.1,
  BTN_2=P1.7, BTN_3=P1.6; encoder press=P3.3; A=P3.1; B=P3.0; NeoPixel=P3.4;
  and NeoPixel count=3.
- `six_key` is selected at compile time only. There is no reliable board-model
  detection before flashing, so choose the build variant deliberately.
- `src/board_config.h` is the board-selection boundary. Code outside that file
  should use logical names (`PIN_BTN_1`, `ENCODER_A`, `BOARD_NEO_COUNT`) rather
  than embedding raw pins.
- `src/neo/config.h` derives its count from `BOARD_NEO_COUNT`. The NeoPixel
  driver sends pixels in increasing index order; do not assume physical order
  without a recorded observation.

## LED Rules

- Use numbered LED defines only.
- `LED_0` = farthest from the rotary switch, off
- `LED_1` = middle LED, menu/profile indicator
- `LED_2` = closest to the rotary switch, mic mute/live indicator
- Keep the physical-location comments in `src/led.h` and do not rename these into semantic labels.

Mic indicator rules:

- live = solid green
- muted = slow blinking yellow

## Six-Key + Knob Board: Verified Working Map

This section overrides only the physical mapping for the `six_key` build;
normal application behavior remains the same as on the three-key board.

- Physical orientation is knob at the top, with keys in two vertical columns.
  The supported left column is top / middle / bottom. Right/top is six-key
  `BTN_4`..`BTN_6` for configurable macros.
- Select this target with `scripts/build.ps1 -BoardVariant six_key`. It supplies
  `-DBOARD_VARIANT_6KEY` and uses `bootloader_pin=p15`.
- The three supported controls are the left column. Their confirmed logical
  assignments are: left/top = `BTN_1` = P1.1, left/middle = `BTN_2` = P1.7,
  and left/bottom = `BTN_3` = P1.6. The encoder uses the same logical assignment
  as the three-key board: A=P3.1 and B=P3.0. These values live in `configuration.h`.
- P3.2/P1.4/P1.5 are confirmed right-top/middle/bottom inputs mapped to
  six-key `BTN_4`..`BTN_6`. P1.5 is also the SW2 recovery line; it is polled as
  BTN_6 only while the application is running, never as a boot request.
- All six LEDs are confirmed: left bottom/middle/top = pixels 0/1/2; right
  bottom/middle/top = pixels 3/4/5. The normal application uses only left
  bottom (`LED_0`, off), left middle (`LED_1`, menu), and left top (`LED_2`,
  mic/live).
- The encoder is A=P3.1, B=P3.0, press=P3.3, matching the three-key board.
- The NeoPixel data line is P3.4 and the six-key build sets the NeoPixel count
  to six. Do not change this count or data pin while mapping keys or LEDs.
- The immediate runtime bootloader combination is encoder press + all three
  left buttons. `buttons.cpp` checks the raw four inputs before any macro or
  other button handling, then calls the bootloader path immediately. It is not a
  startup-only behavior and must remain identical on both board variants.
- SW2/P1.5 is a startup/replug recovery method. While the application runs,
  the same line is BTN_6 and is treated as a normal configurable key.
- Normal LED behavior does not change: `LED_2` is solid green when the mic is
  live and blinks yellow when muted; `LED_1` shows the profile/menu color;
  `LED_0` remains off.
- Any firmware-triggered bootloader entry briefly flashes every configured
  NeoPixel low-intensity amber, then jumps immediately to the bootloader.
- The six-key VS Code `BTN_4` default macro is `Ctrl+\`` (toggle integrated
  terminal). `BTN_4`..`BTN_6` can be configured independently in every profile;
  their other defaults are empty, and they are absent on the three-key board.
- The authoritative logical LED pixels are `SIX_KEY_LED_0_PIXEL`,
  `SIX_KEY_LED_1_PIXEL`, and `SIX_KEY_LED_2_PIXEL` in `configuration.h`.
  `src/led.h` must reference these definitions; do not hardcode alternate
  pixel values there.
- All physical mappings are now confirmed. Do not alter a GPIO or LED pixel
  definition without an explicit request and a corresponding documentation update.
- Before a firmware upload, build the selected board variant into a fresh
  `build\...` directory. The `build\` directory is generated output and may be
  removed completely with `git clean -fdX -- build`; never delete source files,
  `configuration.h`, or user documents as part of cleanup.
- Upload the six-key build with the six-key FQBN (`bootloader_pin=p15`). Enter
  ROM bootloader without touching the board by running
  `macropad-config.exe bootloader` while the application is running. SW2 held
  during reconnect and the immediate left-three-buttons + encoder combination
  remain physical fallback methods.
- `app_window_targeting_plan.md` is a user file, not generated output; do not
  stage, delete, or alter it unless the user explicitly requests it.

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

Build the default three-key firmware:

```powershell
powershell -File .\scripts\build.ps1
```

Build the six-key firmware:

```powershell
powershell -File .\scripts\build.ps1 -BoardVariant six_key -BuildPath .\build\six_key
```

The six-key FQBN must use `bootloader_pin=p15`; the three-key FQBN uses
`bootloader_pin=p36`. `scripts/build.ps1` selects these automatically.

Firmware build output:

- `build/CH55xDuino.mcs51.ch552/ch552g_mini_keyboard.ino.hex`

For a clean firmware build, use a new `build\...` output directory or remove
only generated output with `git clean -fdX -- build`. Never clean source files,
DataFlash backups, user documents, or `app_window_targeting_plan.md`.

Upload the already-built six-key image only after the user asks for upload:

```powershell
& 'C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe' upload `
  --input-dir .\build\six_key `
  --fqbn 'CH55xDuino:mcs51:ch552:clock=16internal,usb_settings=user148,upload_method=usb,bootloader_pin=p15'
```

An upload is complete only when the programmer reports both `Write complete!!!`
and `Verify complete!!!`. Do not claim the board runs a changed mapping merely
because source code compiled; it must have been uploaded and verified.

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

## Documentation and Change Discipline

- `agents.md` is the engineering handoff and must retain every confirmed board
  fact, build rule, bootloader behavior, protocol invariant, and user-file
  boundary. Update it whenever a confirmed behavior changes.
- `readme.md` is user-facing flashing and wiring documentation. Keep it aligned
  with the board variants and recovery paths.
- `current_config.md` documents current macro/profile behavior; update it when
  defaults, profile behavior, or bootloader shortcuts change.
- `configuration.h` is the source of truth for six-key physical GPIO and the
  authoritative logical LED pixel definitions. Do not leave a second hardcoded
  mapping elsewhere.
- Before editing hardware mapping, state the exact one requested change. Make
  that change only, preserve all unrelated values, rebuild, and upload only if
  explicitly requested.
- Do not stage or commit `app_window_targeting_plan.md` unless explicitly asked.
  Do not include generated `build\` files in commits.

## Invariants

- Keep the third LED off unless the user explicitly asks to use it.
- Do not change the physical LED mapping without updating comments and docs.
- Preserve the existing encoder actions unless the task explicitly changes them.
- Keep the five user profiles plus menu profile behavior intact unless the task explicitly changes it.
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
