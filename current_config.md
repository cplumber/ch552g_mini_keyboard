# Current Configuration

This document describes the current built-in macro defaults and fixed profile
behavior. The selected profile and imported macro configuration are stored in
DataFlash and are restored after a power cycle.

The three buttons in each of the four normal profiles are configurable macros. The mappings
below are the factory defaults; after the configuration firmware is flashed, they
can be changed without rebuilding firmware.

## Profiles

| Profile | Menu LED (`LED_1`) | Button 1 | Button 2 | Button 3 | Encoder clockwise | Encoder counter-clockwise |
| --- | --- | --- | --- | --- | --- | --- |
| Copy / paste | Red | `Ctrl+C` — Copy | `Ctrl+V` — Paste | `Ctrl+Z` — Undo | System volume up | System volume down |
| Google Meet | Yellow | `Ctrl+D` — Toggle microphone | `Ctrl+E` — Toggle camera | `Ctrl+Alt+H` — Raise/lower hand | System volume up | System volume down |
| VS Code | Green | `Ctrl+K`, then `V` — Open preview to the side | `Ctrl+Shift+G`, then `G` — Open Source Control | `Ctrl+K`, then `Ctrl+Shift+C` — Copy Relative Path | `Alt+Tab` — Next window | `Alt+Shift+Tab` — Previous window |
| MS Teams (web) | Cyan | `Ctrl+Shift+M` — Toggle mute | `Ctrl+Shift+K` — Raise/lower hand | `Alt+Shift+A` — Start audio call | System volume up | System volume down |

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

- Clockwise: select the next profile (up to MS Teams).
- Counter-clockwise: select the previous profile (down to Copy / paste).
- The middle LED shows the candidate profile color at higher brightness.
- The encoder short-click mute action is disabled while the menu is open.

## LED status

- `LED_0` (farthest from the encoder): always off.
- `LED_1` (middle): current profile color; indicates the selection while in the menu.
- `LED_2` (closest to the encoder): microphone state — solid green when live,
  slow-blinking yellow when muted.

For the six-key board, these logical LEDs are the confirmed left column:
`LED_2` top (strip pixel 2), `LED_1` middle (strip pixel 1), and `LED_0`
bottom (strip pixel 0). The right column is unused by the normal application:
top/middle/bottom are strip pixels 5/4/3 respectively.

## Other firmware shortcut

Pressing all three buttons and the encoder button together releases active keys
and enters the bootloader immediately. The raw four-button combination is
checked before any button macro is executed.

On the six-key board, hold SW2 while reconnecting USB for recovery. While the
firmware is running, pressing all three left buttons and the encoder button
together enters the bootloader immediately. `macropad-config.exe bootloader`
can request the same mode from Windows without pressing any physical buttons;
this is the preferred upload path. Firmware-triggered entry briefly flashes all
available LEDs low-intensity amber before jumping to the bootloader.

## Reassigning profile buttons

Build the Windows helpers with `macropad_tools\build.bat`, then use the separate
configuration tool:

```powershell
.\macropad_tools\build\macropad-config.exe export .\keyboard-config.json
.\macropad_tools\build\macropad-config.exe import .\keyboard-config.json
.\macropad_tools\build\macropad-config.exe reset
.\macropad_tools\build\macropad-config.exe bootloader
```

Each button supports one or two keyboard chords. Two-chord macros also preserve a
per-macro `between_chords_ms` delay: 50 ms for VS Code Preview and Copy Relative
Path, and 30 ms for Source Control. The Windows tool validates the complete JSON;
the device stages and checksum-verifies the update before activating it. A rejected
or interrupted import leaves the prior working configuration active.

Flashing can erase DataFlash. Run `export` before each firmware upload and
`import` the saved JSON after the new firmware starts.

The current mappings are included in `keyboard-config.example.json`. Start with
that file, or export the device defaults, rather than writing JSON yourself.

The Teams web shortcuts follow [Microsoft's current shortcut list](https://support.microsoft.com/en-gb/office/keyboard-shortcuts-for-microsoft-teams-2e8e2a70-e8d8-4a19-949b-4c36dd5292d2).
