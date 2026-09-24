# USB Mini Keyboard with CH552G

## Introduction
This project began with the purchase of a compact USB keyboard with three keys from AliExpress, seemingly without any specific purpose in mind. The original software required for this keyboard was provided as a `.exe` file, which I downloaded from a Google Drive repository. Since I rebuilt the firmware myself, the board now behaves as a small CH552G macropad with a rotary encoder, a Windows mic-mute bridge, and profile selection stored in DataFlash.

I decided to open it and try to program it myself.

![Keyboard](img/keyboard.jpeg)

![Menu](img/key_menu.gif?raw=true)

Hold the encoder for 1 second to enter the menu, then rotate it to choose a profile.

In the normal profiles, encoder rotation controls system volume up/down. The menu profile uses the encoder for profile selection. Short encoder click toggles the Windows microphone mute through the bridge.


## What's Inside

The core of the board features a WCH CH552G microcontroller, three buttons, a rotary encoder, and three addressable LEDs. The firmware uses LED 2, the pixel closest to the rotary switch, for mic mute/live state, LED 1 for menu/profile selection, and leaves LED 0 off.

![Bottom](img/bottom.jpeg?raw=true)

## CH552G microcontroller

![CH552G](img/ch552g.png?raw=true)


## How to Build

This firmware uses the Arduino platform to simplify the build process. I built it on a MacBook Pro with an M1 chip.

1. Install the Arduino IDE.
2. Add support for CH552G:
   - Go to Preferences -> Additional Board Manager.
   - Add https://raw.githubusercontent.com/DeqingSun/ch55xduino/ch55xduino/package_ch55xduino_mcs51_index.json.
3. Open the project `ch552g_mini_keyboard.ino`.
   - In the Tools menu, select CH55xDuino board.
   - In Tools, select bootloader: P3.6 (D+) Pull up.
   - In Tools, select clock source: 16MHz (internal) 3.5V or 5V.
   - In Tools, select upload method: USB.
   - In Tools, select USB Setting: USER CODE w/148B USB RAM.
4. Compile the project.
5. Set the keyboard in bootloader mode (see below).
6. Flash the project. (*Original firmware will be completed lost*)

### Command-line build

If you want a repeatable build that is easy to tweak later, use [`scripts/build.ps1`](scripts/build.ps1).

```powershell
powershell -File .\scripts\build.ps1
```

The script automatically looks for the bundled `arduino-cli.exe` inside the Arduino IDE install, and it writes the build output to `build/CH55xDuino.mcs51.ch552/`.

Current built size: 12,810 / 14,336 bytes of flash (89%) and 467 / 876 bytes of RAM (53%).

If you want to change the board settings later, edit the default `-Fqbn` value at the top of the script or pass a new one on the command line.

```powershell
powershell -File .\scripts\build.ps1 -Fqbn 'CH55xDuino:mcs51:ch552:clock=16internal,usb_settings=user148,upload_method=usb,bootloader_pin=p36'
```

Useful script knobs:

- `-SketchPath`: path to the `.ino` file if the project moves.
- `-BuildPath`: where the `.hex`, `.elf`, and map files are written.
- `-Fqbn`: the CH55xDuino board/menu selection.
- `-ArduinoCliPath`: override the CLI path if Arduino IDE is installed somewhere unusual.

### Memory map report

After a build, use [`scripts/map-report.ps1`](scripts/map-report.ps1) to see where flash and RAM go.

```powershell
powershell -File .\scripts\map-report.ps1
```

The report reads `build/CH55xDuino.mcs51.ch552/ch552g_mini_keyboard.ino.map` and `build/CH55xDuino.mcs51.ch552/ch552g_mini_keyboard.ino.mem`, then prints:

- section totals
- top RAM symbols and modules
- top flash symbols and modules

For this project, `XSEG` and `XISEG` are the main external RAM sections that count toward the memory figure in the build output. `CSEG` and `CONST` are the big flash sections to watch when you want to trim program size.

## Setting up the Keyboard in Bootloader Mode

To enter bootloader mode, CH552G require connect pin P3.6 to vcc with a 10K pull-up resistor. To do this:
- Short the R12 on the bottom of the board and connect the board to your PC.
  ![Short](img/short.jpeg?raw=true)
- You can now proceed to flash the firmware.
- After the firmware is successfully flashed, reconnect USB while holding the encoder button, press all four buttons while the firmware is running, or use `macropad-config.exe bootloader`.

## CH552G Flashing (Pin 3 Boot + WCHISPTool)

Use this as the fallback method if the default P3.6-to-VCC bootloader path does not work on your board.

### Setup

1. Open `WCHISPTool`.
2. Load `Object File1 -> firmware.hex`.
3. Leave the tool ready and waiting.

### Flashing

1. Short pin 3 to GND.
2. Plug USB while the pin is shorted.
3. Hold for about 1 second.
4. Release.
5. Immediately click `Download`.

### After flash

1. Unplug USB.
2. Plug it again without the pin short.

### Next uploads, no pin short

1. Hold the encoder button.
2. Plug USB.

The bootloader check lives in [`ch552g_mini_keyboard.ino`](ch552g_mini_keyboard.ino). When the encoder button is held during startup, the firmware flashes the three LEDs, then jumps into bootloader mode.



# Firmware feature

This firmware provides four profiles and a profile-selection menu. Long-press
the rotary encoder for 1 second, then rotate it to select a profile. Encoder actions,
profile colors, and menu behavior are compiled in; the twelve profile-button macros
are stored separately in DataFlash.

The three buttons in each normal profile are keyboard macros. Each macro supports
one or two sequential chords, with a configurable delay between two chords; the
encoder controls and menu behavior remain fixed.

![Menu](img/key_menu.gif?raw=true)

Current configuration

| Config | Menu LED color | BTN 1 | BTN 2 | BTN 3 | Encoder CW | Encoder CCW | Encoder press |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Copy / paste | Red | `Ctrl+C` | `Ctrl+V` | `Ctrl+Z` | `Volume up` | `Volume down` | Short click: mic mute, hold `1s`: menu |
| Google Meet | Yellow | `Ctrl+D` | `Ctrl+E` | `Ctrl+Alt+H` | `Volume up` | `Volume down` | Short click: mic mute, hold `1s`: menu |
| VS Code | Green | `Ctrl+K`, then `V` | `Ctrl+Shift+G`, then `G` | `Ctrl+K`, then `Ctrl+Shift+C` (Copy Relative Path) | `Alt+Tab` held for `1s` | `Alt+Shift+Tab` held for `1s` | Short click: mic mute, hold `1s`: menu |
| MS Teams (web) | Cyan | `Ctrl+Shift+M` | `Ctrl+Shift+K` | `Alt+Shift+A` | `Volume up` | `Volume down` | Short click: mic mute, hold `1s`: menu |

The menu profile uses the encoder to move through the four user profiles; the selected profile is saved in DataFlash, so it survives power cycles.

The MS Teams (web) defaults use Microsoft's listed web shortcuts: `Ctrl+Shift+M`
for mute, `Ctrl+Shift+K` for raise/lower hand, and `Alt+Shift+A` for an audio
call. See [Microsoft's Teams shortcut list](https://support.microsoft.com/en-gb/office/keyboard-shortcuts-for-microsoft-teams-2e8e2a70-e8d8-4a19-949b-4c36dd5292d2).

### Reassigning profile buttons

After flashing the configuration-capable firmware once, change the twelve profile
buttons without rebuilding or reflashing. Build the Windows tools in
`macropad_tools\` and use `macropad-config.exe`:

```powershell
cd macropad_tools
build.bat
build\macropad-config.exe export ..\keyboard-config.json
build\macropad-config.exe import ..\keyboard-config.json
build\macropad-config.exe reset
build\macropad-config.exe bootloader
```

[`keyboard-config.example.json`](keyboard-config.example.json) shows the default
format. `export` creates an editable JSON template. The Windows tool validates the
complete JSON before starting an import; the keyboard stages and checksum-verifies
the update before activation. Bad input, disconnects, or an interrupted import
retain the last known-good mapping.

`bootloader` tells the running firmware to enter the CH552 USB bootloader, so a
script can begin an upload without holding any physical buttons. Export the
configuration before flashing and import it again afterward: an upload can erase
DataFlash.

JSON accepts up to two chords per button. Chord items may contain `Ctrl`, `Shift`,
`Alt`, `GUI`, and one printable ASCII key. `between_chords_ms` must be `0` for a
one-chord macro and is `0`–`255` for two chords. The firmware preserves the 10 ms
state-change delay and 20 ms key hold for every chord.

### Automated build and upload

After this firmware is installed, an upload can be performed without physical
button input. The upload may erase DataFlash, so retain and restore the macro
backup:

```powershell
.\macropad_tools\build\macropad-config.exe export .\keyboard-config-backup.json
powershell -File .\scripts\build.ps1
.\macropad_tools\build\macropad-config.exe bootloader

& 'C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe' upload `
  --fqbn 'CH55xDuino:mcs51:ch552:clock=16internal,usb_settings=user148,upload_method=usb,bootloader_pin=p36' `
  --build-path .\build\CH55xDuino.mcs51.ch552 `
  .\ch552g_mini_keyboard.ino

.\macropad_tools\build\macropad-config.exe import .\keyboard-config-backup.json
```

## Pinout

- BUTTON 1: P16
- BUTTON 2: P17
- BUTTON 3: P11
- BUTTON R: P33
- ENCODER A: P31
- ENCODER B: P30
- LED strip data: P34
- LED 0: farthest from rotary switch, unused/off
- LED 1: middle LED, menu/profile indicator
- LED 2: closest to rotary switch, mic mute/live indicator

### Six-key + knob board variant

The six-key variant was mapped with the standalone hardware mapper on
2026-09-24. The map is recorded in [`configuration.h`](configuration.h) and
is selected by the `six_key` build variant:

| Control | CH552 pin |
| --- | --- |
| Left/top (supported BTN_1) | P1.6 |
| Left/middle (supported BTN_2) | P1.7 |
| Left/bottom (supported BTN_3) | P1.1 |
| Right/top (unused) | P3.2 |
| Right/middle (unused) | P1.4 |
| Right/bottom / SW2 line (unused) | P1.5 |
| Encoder A / B | P3.0 / P3.1 |
| Encoder press | P3.3 |

The mapper found six responding LED positions. Viewed from the top with the
knob above the key grid, their physical arrangement is:

```text
    knob
3 6
2 5
1 4
```

The verified addressable strip order is pixel `0..5` = physical positions
`3, 6, 2, 5, 1, 4`. The six-key configuration header records this mapping.
The red/green/blue test confirms that all six pixels respond as independent RGB
channels.

Encoder rotation produced `ijij` left and `jiji` right in the mapper, confirming
the A/B order above. Keep the SW2/P1.5 boot path available when flashing the
six-key firmware. Build variants are selected explicitly because the board
cannot be reliably identified by the CH552 before flashing:

```powershell
powershell -File .\scripts\build.ps1 -BoardVariant three_key
powershell -File .\scripts\build.ps1 -BoardVariant six_key -BuildPath .\build\CH55xDuino.mcs51.ch552.six_key
```

SW2/P1.5 is held during startup/replug; because it shares the right-bottom
switch line, the application does not poll it while running.
Its three existing logical LED indicators use the left-column LED positions:
mic/live is top-left (pixel 2), menu is middle-left (pixel 4), and the remaining
logical LED is bottom-left (pixel 0, kept off).

## Additional resources

Here are the resources I used for reprogramming the firmware:

- [How to Program a Really Cheap Microcontroller](https://hackaday.com/2019/02/17/how-to-program-a-really-cheap-microcontroller/#more-345535)
- [RGB Macropad Custom Firmware](https://hackaday.io/project/189914-rgb-macropad-custom-firmware)
- [CH552G Macropad Plus](https://oshwlab.com/wagiminator/ch552g-macropad-plus)
- [ch554_sdcc on GitHub](https://github.com/Blinkinlabs/ch554_sdcc)
- [ch55xduino on GitHub](https://github.com/DeqingSun/ch55xduino)
- [CH552G Product Page](https://www.esclabs.in/product/ch552g-8-bit-usb-device-microcontroller/)
- [LCSC Product Page](https://www.lcsc.com/product-detail/Microcontroller-Units-MCUs-MPUs-SOCs_WCH-Jiangsu-Qin-Heng-CH552G_C111292.html?utm_source=digipart&utm_medium=cpc&utm_campaign=CH552G)
- [CH552G Datasheet](http://www.wch-ic.com/downloads/file/309.html)


## Windows Mic Mute Bridge

`macropad_tools/` contains two Windows programs. `mic-mute-bridge.exe` listens
for `F24`, toggles the default Windows microphone through Core Audio, and sends
the current mic state back to the MCU. `macropad-config.exe` imports, exports,
and resets button macros, and can request bootloader mode. The encoder short click
emits `F24`; a 1-second hold opens the menu.


# License
![license.png](https://i.creativecommons.org/l/by-sa/3.0/88x31.png)

This work is licensed under Creative Commons Attribution-ShareAlike 3.0 Unported License. 
(http://creativecommons.org/licenses/by-sa/3.0/)
