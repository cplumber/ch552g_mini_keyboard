# USB Mini Keyboard with CH552G

## Introduction
This project began with the purchase of a compact USB keyboard with three keys from AliExpress, seemingly without any specific purpose in mind. The original software required for this keyboard was provided as a .exe file, which I downloaded from a Google Drive repository. However, since I don't have access to a Windows PC to test it, I'm quite certain that most antivirus software would not approve of it.

I decided to open it and try to program it myself.

![Keyboard](img/keyboard.jpeg)

![Menu](img/key_menu.gif?raw=true)

Change keyboard config by holding the encoder for 1 second, then rotating it

In the normal configs, encoder rotation controls system volume up/down. The menu config still uses the encoder for config selection.
Short encoder click toggles the mic mute through the Windows bridge.


## What's Inside

The core of the board features a wch-ic CH552G microcontroller, three buttons, a rotary encoder, and three addressable LEDs.

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

## Setting up the Keyboard in Bootloader Mode

To enter bootloader mode, CH552G require connect pin P3.6 to vcc with a 10K pull-up resistor. To do this:
- Short the R12 on the bottom of the board and connect the board to your PC.
  ![Short](img/short.jpeg?raw=true)
- You can now proceed to flash the firmware.
- Once the firmware is successfully flashed, to *return to bootloader mode, reconnect the USB interface while either pressing the encoder button or in running mode simultaneously press all the buttons*.

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

```C
  // Go in bootloader more if connected with encoder button pressed
  if (!digitalRead(PIN_BTN_ENC))
  {
    NEO_writeHue(0, NEO_CYAN, NEO_BRIGHT_KEYS); // set led1 to cyan
    NEO_writeHue(1, NEO_BLUE, NEO_BRIGHT_KEYS); // set led2 to blue
    NEO_writeHue(2, NEO_MAG, NEO_BRIGHT_KEYS); //  set led3 to magenta
    NEO_update();                              // update pixels
    BOOT_now();     // jump to bootloader
  }
```



# Firmware feature

This firmware can set up the keyboard in different configurations.
Edit configuration.cpp to change it
To change configuration, long-press the rotary encoder for 1 second, then rotate it.

On configurtion is possible to send keyboard or mous event and setup automatic cycle rutine

![Menu](img/key_menu.gif?raw=true)

Current configuration

| Config | BTN 1 | BTN 2 | BTN 3 | Encoder CW | Encoder CCW | Encoder press |
| --- | --- | --- | --- | --- | --- | --- |
| Copy / paste | `Ctrl+C` | `Ctrl+V` | `Ctrl+Z` | `Volume up` | `Volume down` | Short click: mic mute, hold `1s`: menu |
| Google Meet | `Ctrl+D` | `Ctrl+E` | `Ctrl+Alt+H` | `Volume up` | `Volume down` | Short click: mic mute, hold `1s`: menu |
| VS Code | `Ctrl+Shift+E` | `Ctrl+Shift+G`, then `G` | `Ctrl+\`` | `Alt+Tab` held for `1s` | `Alt+Shift+Tab` held for `1s` | Short click: mic mute, hold `1s`: menu |

The menu config uses the encoder to move through the first 3 configs; the rows above are the active normal working configs.

## Pinout

- BUTTON 1: P16
- BUTTON 2: P17
- BUTTON 3: P11
- BUTTON R: P33
- ENCODER A: P31
- ENCODER B: P30
- LED: P34

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

A separate C++ helper lives in [`mic_mute_bridge/`](mic_mute_bridge/). It listens for `F24` and toggles the default Windows microphone mute state through Core Audio. The helper also sends the current mic state back to the MCU so the LEDs show very low green when the mic is live and very low yellow when muted. The encoder short click emits `F24`; the 2-second hold still opens the menu.


# License
![license.png](https://i.creativecommons.org/l/by-sa/3.0/88x31.png)

This work is licensed under Creative Commons Attribution-ShareAlike 3.0 Unported License. 
(http://creativecommons.org/licenses/by-sa/3.0/)
