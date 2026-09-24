# Six-key hardware mapper

This is a standalone, temporary firmware image for identifying the GPIO wiring
of a six-key CH552 keypad. It does not alter the normal three-key firmware or
write macro configuration into DataFlash.

## Recovery first

Build this sketch only with `bootloader_pin=p15`; `build.ps1` sets that option.
That preserves the expected `SW2` / P1.5-low-at-power-up bootloader selection
on the reference six-key PCB. Do not flash the regular project image, whose
default build selects `bootloader_pin=p36`, onto this board during discovery.

After a mapper flash, keep the physical SW2 method available:

1. Unplug the keypad.
2. Bridge the two `SW2` pads.
3. Plug in USB, wait about one second, then release the bridge.
4. The CH552 bootloader should enumerate (commonly `4348:55E0` or `1A86:55E0`).

The mapper also requests the bootloader if a candidate control is held during
startup. It also requests one automatic ROM-bootloader window on every
application start. The ROM loader controls its own timeout (normally about ten
seconds); the mapper cannot extend that timeout to an exact fifteen seconds.
The reset marker prevents an endless loop when the timeout expires. SW2 remains
the hardware fallback.

## LED discovery

The mapper assumes the LED data line is P3.4 and tests six pixel positions and
their RGB channels. It shows red, green, and blue on pixel 0, one color per
second, then repeats those three colors on pixels 1 through 5. The full cycle
is 18 seconds. If only three physical LEDs are installed, pixels 3–5 remain
dark; this does not damage the strip. The count and data pin are isolated in
`neo_config.h` for adjustment.

## Build

```powershell
powershell -File .\six_key_mapper\build.ps1
```

The output is `build\six_key_mapper\six_key_mapper.ino.hex`.

## Discovering controls

1. Enter bootloader with SW2 and flash the mapper image.
2. Replug normally.
3. Open Notepad or another plain text field, then press one physical control at
   a time. The mapper types a letter for every debounced low/high transition.
4. Record the letter(s) for each of the six keys and the encoder press.
5. Turn the encoder slowly one detent clockwise and counter-clockwise. The two
   alternating letters identify its A/B pins; their order distinguishes the
   direction.

| Typed letter | CH552 pin |
| --- | --- |
| `a`–`h` | P1.0–P1.7, respectively |
| `i`–`n` | P3.0–P3.5, respectively |

P3.4 (`m`) is included for discovery but is not used by the mapper's startup
recovery guard because it may instead be the LED data output. P3.6 and P3.7
are never probed because they are USB D+ and D-.

Once the pin map is recorded, use it to create the finished six-key firmware;
do not use this mapper as the daily keyboard firmware.
