# Mic Mute Bridge

Small Windows helper for the CH552G macropad.

It listens for `F24` and toggles the default capture device mute state through the Windows Core Audio API.
It also pushes the current mute state back to the MCU so the LEDs stay in sync.

## Build

This version is meant for MinGW-w64.

1. Make sure MinGW is installed under `D:\tools\mingw64`.
2. Run `build.bat`.

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
The 2-second hold still opens the config menu.

## MCU LED sync

- live mic: very low green on all three LEDs
- muted mic: very low yellow on all three LEDs
- the helper polls Windows for the actual mic state and pushes it back to the MCU
