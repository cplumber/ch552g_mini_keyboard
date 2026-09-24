# Mic Mute Bridge

Windows helpers for the CH552G macropad.

It listens for `F24` and toggles the default capture device mute state through the Windows Core Audio API.
It also pushes the current mute state back to the MCU so the LEDs stay in sync.
The build also produces `macropad-config.exe`, which imports, exports, resets,
and backs up persistent profile-button mappings; it can also request bootloader
mode.

## Build

This version is meant for MinGW-w64.

1. Make sure MinGW is installed under `D:\tools\mingw64`.
2. Run `build.bat`.

Build output:

- `build\mic-mute-bridge.exe`
- `build\macropad-config.exe`

## Button configuration

```bat
build\macropad-config.exe export ..\keyboard-config.json
build\macropad-config.exe import ..\keyboard-config.json
build\macropad-config.exe reset
build\macropad-config.exe bootloader
build\macropad-config.exe board
```

Use an exported JSON file as the import template. The configuration tool validates
the complete JSON before starting an import. The device stages and checksum-verifies
the update before activation, so an invalid or interrupted import does not replace
the working mapping.

`bootloader` requests the firmware's internal CH552 bootloader. It is intended as
the handoff just before an automated uploader runs; it does not itself flash a file.
Export the configuration before uploading and import it after the new firmware
starts, because flashing can erase DataFlash.

`board` prints the board ID embedded in the running application firmware:
`three_key` or `six_key`. It cannot identify a blank device or the ROM
bootloader; select the build variant manually in those cases.

UID-capable firmware also prints the CH552G factory 40-bit UID, for example
`six_key uid=1234ABCDEF`. Add `--uid HEX10` before the command to select one
keyboard when multiple boards are connected. Each existing board needs one
UID-capable firmware upload while connected alone before this selection works.

## CLI test

Toggle the mic once and exit:

```bat
build\mic-mute-bridge.exe --toggle
```

`--once` is the same as `--toggle`.

Hide the console window:

```bat
build\mic-mute-bridge.exe --windowless
```

Stop all running bridge instances and exit:

```bat
build\mic-mute-bridge.exe --stop-all
```

The bridge only allows one running instance at a time.
`--stop-all` signals the running instance to exit cleanly.

You can combine flags:

```bat
build\mic-mute-bridge.exe --windowless --toggle
```

## Trigger

The firmware emits `F24` on the knob short-click.
The 1-second hold still opens the config menu.

## MCU LED sync

- live mic: solid green on LED 2 (closest to the encoder)
- muted mic: slow-blinking yellow on LED 2
- the helper polls Windows for the actual mic state and pushes it back to the MCU
