# Implemented Design: Reassignable Profile Buttons Without Reflashing

## Goal

After one firmware update, let the user change the actions for `BTN_1`, `BTN_2`,
and `BTN_3` in each of the four normal profiles without rebuilding or reflashing
the keyboard firmware.

The settings must survive a power cycle. A small Windows command-line interface
will write them to the keyboard over its existing USB HID connection. An
incomplete, corrupt, or invalid import must never become active: the keyboard
must continue using its last known-good configuration.

## Scope

This intentionally supports only the twelve physical profile buttons:

| Profiles | Reassignable controls |
| --- | --- |
| Copy / paste, Google Meet, VS Code, MS Teams (web) | `BTN_1`, `BTN_2`, `BTN_3` |

The following remain firmware behavior, rather than user-configurable actions:

- Encoder clockwise/counter-clockwise actions.
- Encoder short click (`F24` mic-mute bridge toggle).
- Encoder long press and profile-selection menu.
- Profile colors, mic indicator, and bootloader shortcut.
- Mouse macros, automatic routines, arbitrary per-step delays, and a graphical editor.

Keeping this boundary makes the feature small enough for the CH552G and avoids
turning the macropad into a general-purpose macro platform.

## User Experience

The first version needs one normal firmware build and flash. After that, a user
edits a JSON file and imports it; no Arduino IDE, compilation, or bootloader mode
is required.

```powershell
# Save the current device settings to a readable file.
.\macropad-config.exe export .\keyboard-config.json

# Validate the file and write its mappings to the connected keyboard.
.\macropad-config.exe import .\keyboard-config.json

# Restore the firmware's built-in defaults.
.\macropad-config.exe reset

# Enter the CH552 USB bootloader for an automated upload.
.\macropad-config.exe bootloader
```

Example `keyboard-config.json`:

```json
{
  "version": 1,
  "profiles": {
    "copy_paste": {
      "BTN_1": {"chords": [["Ctrl", "C"]], "between_chords_ms": 0},
      "BTN_2": {"chords": [["Ctrl", "V"]], "between_chords_ms": 0},
      "BTN_3": {"chords": [["Ctrl", "Z"]], "between_chords_ms": 0}
    },
    "google_meet": {
      "BTN_1": {"chords": [["Ctrl", "D"]], "between_chords_ms": 0},
      "BTN_2": {"chords": [["Ctrl", "E"]], "between_chords_ms": 0},
      "BTN_3": {"chords": [["Ctrl", "Alt", "H"]], "between_chords_ms": 0}
    },
    "vs_code": {
      "BTN_1": {"chords": [["Ctrl", "K"], ["V"]], "between_chords_ms": 50},
      "BTN_2": {"chords": [["Ctrl", "Shift", "G"], ["G"]], "between_chords_ms": 30},
      "BTN_3": {"chords": [["Ctrl", "K"], ["Ctrl", "Shift", "C"]], "between_chords_ms": 50}
    },
    "ms_teams": {
      "BTN_1": {"chords": [["Ctrl", "Shift", "M"]], "between_chords_ms": 0},
      "BTN_2": {"chords": [["Ctrl", "Shift", "K"]], "between_chords_ms": 0},
      "BTN_3": {"chords": [["Alt", "Shift", "A"]], "between_chords_ms": 0}
    }
  }
}
```

Each inner array is one key chord. The outer array is the sequence of chords sent
by a button. Thus the VS Code Copy Relative Path action is represented as
`Ctrl+K`, then `Ctrl+Shift+C`, without needing a function called
`keyboard_vscode_copy_relative_path`.

## Firmware Design

### Generic macro record

Replace the per-button function pointer entries for the normal profile buttons
with a generic macro table. Each button allows up to two chord steps. Two steps
cover every current shortcut, including VS Code's chord-prefix commands, and do
not add an unnecessary general macro language.

```c
typedef struct {
  uint8_t modifiers; // Ctrl, Shift, Alt, GUI bit mask
  uint8_t key;       // existing Keyboard_press() key representation
} macro_chord_t;

typedef struct {
  uint8_t length;                  // 0 to 2
  macro_chord_t chord[2];
  uint8_t inter_chord_delay_ms;    // used only when length is 2
} button_macro_t;
```

Each record is six bytes. Twelve records therefore use 72 bytes. Store one complete
configuration slot in the 128 DataFlash bytes exposed by this CH552 Arduino core.
The slot contains a four-byte header and the 72-byte payload, for 76 bytes total.
Reserve the slot starting at address `8`; this keeps the existing profile-selection
byte at address `0` intact.

The stored `key` is deliberately **not** a raw HID usage. It uses the format
already accepted by `Keyboard_press()` and `Keyboard_release()`—printable ASCII
or the project's `KEY_*` constants. The Windows tool translates readable JSON
key names into this compact firmware format.

### One executor for every button

Add one `keyboard_run_macro()` routine. For each chord it:

1. Presses its modifier bits.
2. Presses and releases its key.
3. Uses a 10 ms pause between modifier/key state changes and a 20 ms key hold,
   matching the existing reliable shortcut timing.
4. Releases modifiers.
5. Waits for that macro's configured `between_chords_ms` delay before the next
   chord.

All current button actions fit this model, including Copy, Paste, Google Meet
Raise Hand, VS Code Preview, Source Control, and Copy Relative Path.

The existing special functions for normal profile buttons can then be removed:

- `keyboard_meet_raise_hand`
- `keyboard_vscode_preview`
- `keyboard_vscode_source_control`
- `keyboard_vscode_copy_relative_path`

Encoder helpers stay as functions because they implement fixed device behavior,
not configurable button macros.

### Defaults and DataFlash loading

Keep the built-in defaults in firmware as a `const` macro table. On startup:

1. Read the single configuration-slot header and payload.
2. Verify the slot magic, format version, generation counter, overall checksum,
   and every button record's length.
3. Load the fully valid slot into the active runtime table.
4. If the slot is invalid (for example, after power loss during an import), use
   the built-in defaults without writing them to DataFlash.

The active runtime table is only replaced after a complete slot passes all checks.
The current profile-selection byte at DataFlash address `0` remains independent
of the macro configuration.

### Saves with default fallback

Use the single DataFlash slot as the staging area. The active configuration remains
usable while an import is in progress; after power loss, an invalid slot causes the
firmware to use built-in defaults.

1. `BEGIN_UPDATE` marks the slot invalid and copies the active slot (or the
   built-in defaults) into it.
2. Each `SET_BUTTON` checks its profile, button index, and length of 0–2 before
   replacing only that staged record. The Windows CLI validates modifiers, keys,
   JSON structure, and all twelve mappings before it starts an import.
3. `COMMIT` reads and validates the entire staged slot, calculates its overall
   checksum, writes the header fields, and writes the single valid-marker byte
   last.
4. The firmware reads the committed slot back. Only a fully verified slot becomes
   the active runtime table.

If USB disconnects, the process exits, or a report is rejected before `COMMIT`,
the staged slot remains invalid and the last known-good slot stays active. If
power fails during the write, boot validation rejects the partial slot and uses the
built-in defaults. The next successful import repairs the slot.

## USB Protocol

The device already exposes vendor HID reports and the Windows bridge already uses
one-byte feature report ID `5` for microphone-state feedback. Do not reuse or
change it. Add a new, dedicated vendor report ID with both input and output
reports for configuration. It must be added to the HID report descriptor and
handled by the USB endpoint code.

This is not just a new command in the bridge: the current firmware accepts only
the existing one-byte reports and has no HID `GET_REPORT` handler. The new vendor
report avoids those limits and supplies responses for export and status.

Minimal operations:

| Operation | Purpose |
| --- | --- |
| `GET_BUTTON` | Return one saved/default-resolved button macro for export. |
| `BEGIN_UPDATE` | Create an invalid staging copy of the active/default configuration. |
| `SET_BUTTON` | Validate and update one button macro in the staging slot. |
| `SET_DELAY` | Update the staged inter-chord delay for one button. |
| `COMMIT` | Verify and atomically activate the complete staged configuration. |
| `ABORT` | Discard the staged slot without changing the active configuration. |
| `RESET_DEFAULTS` | Atomically replace the active configuration with built-in defaults. |
| `ENTER_BOOTLOADER` | Request the internal CH552 USB bootloader; requires a host confirmation value. |

The USB encoding should use compact numeric key values (in the existing
`Keyboard_press()` format) and modifier bits. JSON names such as `"Ctrl"` and
`"C"` exist only in the Windows tool. This keeps device flash/RAM use low and
makes invalid host input easy to reject.

Report ID `6` uses 8 data bytes plus the report ID, so it fits the existing 9-byte
endpoint size. `SET_BUTTON` carries the five chord fields; `SET_DELAY` carries the
separate inter-chord delay. Keeping reports within 9 bytes preserves Windows HID
enumeration on this device.

## Windows Programs and Shared HID Code

Use two separate executables. This keeps the background microphone helper focused
on microphone synchronization and makes configuration a short-lived, explicit
command-line action.

```text
macropad_tools/
  common/
    macropad_hid.h
    macropad_hid.cpp       # Device discovery and HID report transport
  main.cpp                 # F24 hotkey, Core Audio, and mic-state feedback
  config_tool/
    main.cpp               # JSON import, export, and reset commands
  build.bat
  build/
    mic-mute-bridge.exe
    macropad-config.exe
```

Build `common/macropad_hid.cpp` as a small static library, then link it into both
executables. A static library is preferable to a DLL here: it avoids a separate
runtime dependency and retains the current single-folder deployment experience.

### `mic-mute-bridge.exe`

This remains the long-running helper. It listens for `F24`, toggles the Windows
default microphone, and sends the microphone state using the existing report ID
`5`. It does not parse JSON or modify button mappings.

### `macropad-config.exe`

This is a short-lived command-line program:

1. Opens the shared HID device through `macropad_hid`.
2. Reads or writes the new configuration vendor reports.
3. Converts between the compact device format and JSON.
4. Validates JSON, profile/button names, chord count, modifier names, and key
   names before sending anything to the keyboard.
5. Prints useful errors for an absent device, invalid JSON, or rejected macro.

The bridge already opens the HID device with read/write sharing enabled, so the
configuration tool can communicate with the keyboard while the bridge is running.

## Implementation Order

1. Define the two-chord macro structs, defaults, DataFlash addresses, and
   complete-slot validation rules.
2. Implement `keyboard_run_macro()` and replace normal profile button handlers
   with the generic macro dispatcher.
3. Verify current defaults reproduce every existing button shortcut.
4. Add single-slot DataFlash load, staging, validation, commit, and default fallback.
   reset handling.
5. Add the configuration HID input/output report descriptor and firmware report
   handlers.
6. Extract HID device discovery and report transport into `common/macropad_hid`.
7. Add `macropad-config.exe` with `export`, `import`, `reset`, and `bootloader` commands.
8. Update `build.bat` to build both self-contained executables.
9. Add a sample JSON configuration and update `readme.md` and
   `current_config.md`.
10. Build firmware and both Windows executables, then manually test import/export,
   simultaneous bridge/config-tool access, unplug and
   reconnect persistence, invalid JSON rejection, and reset-to-defaults.

## Acceptance Checks

- Current default behavior works exactly as it does before this change.
- Changing VS Code `BTN_3` in JSON changes its action without a firmware rebuild
  or flash.
- A two-chord shortcut such as `Ctrl+K`, then `Ctrl+Shift+C` works reliably.
- The selected profile continues to persist independently of macro settings.
- Invalid JSON, malformed HID data, rejected key values, an interrupted import,
  or a power loss during import leaves the last known-good configuration active.
- A configuration becomes active only after the whole slot passes device-side
  validation and read-back verification.
- The mic-mute bridge keeps working with its existing report ID and behavior.
- `macropad-config.exe` can import, export, and reset mappings while the
  mic-mute bridge is running.
- `macropad-config.exe bootloader` enters the CH552 bootloader without a physical
  button; configuration is exported before flashing and restored afterward.
