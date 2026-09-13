# Current Configuration

This document describes the shortcuts compiled into the current firmware
(`configuration.cpp` and `src/keyboard.cpp`). The selected profile is stored in
DataFlash and is restored after a power cycle.

## Profiles

| Profile | Menu LED (`LED_1`) | Button 1 | Button 2 | Button 3 | Encoder clockwise | Encoder counter-clockwise |
| --- | --- | --- | --- | --- | --- | --- |
| Copy / paste | Red | `Ctrl+C` — Copy | `Ctrl+V` — Paste | `Ctrl+Z` — Undo | System volume up | System volume down |
| Google Meet | Yellow | `Ctrl+D` — Toggle microphone | `Ctrl+E` — Toggle camera | `Ctrl+Alt+H` — Raise/lower hand | System volume up | System volume down |
| VS Code | Green | `Ctrl+K`, then `V` — Open preview to the side | `Ctrl+Shift+G`, then `G` — Open Source Control | `Ctrl+K`, then `Ctrl+Shift+C` — Copy Relative Path | `Alt+Tab` — Next window | `Alt+Shift+Tab` — Previous window |

In the VS Code profile, turning the encoder keeps `Alt` held for one second after
the latest turn, allowing repeated turns to cycle through windows before the
selection is confirmed.

## Encoder button

The encoder button behaves the same in every normal profile:

- Short click: sends `F24`. The Windows mic-mute bridge receives this and toggles
  the default microphone mute state.
- Hold for 1 second: enters profile-selection mode.
- Release after the long hold: selects the highlighted profile and saves it to
  DataFlash.

## Profile-selection mode

While in the menu, the three regular buttons do nothing.

- Clockwise: select the next profile (up to VS Code).
- Counter-clockwise: select the previous profile (down to Copy / paste).
- The middle LED shows the candidate profile color at higher brightness.
- The encoder short-click mute action is disabled while the menu is open.

## LED status

- `LED_0` (farthest from the encoder): always off.
- `LED_1` (middle): current profile color; indicates the selection while in the menu.
- `LED_2` (closest to the encoder): microphone state — solid green when live,
  slow-blinking yellow when muted.

## Other firmware shortcut

Pressing all three buttons and the encoder button together releases active keys
and enters the bootloader.
