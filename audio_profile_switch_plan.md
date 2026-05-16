# CH552G Audio Profile Switching Plan

## Summary
- Keep the encoder short-click path as the trigger for Windows audio control.
- Use a double-click on the encoder to advance to the next paired mic + speaker profile.
- Implement the switching logic in the Windows helper, with the firmware only sending the trigger and reflecting confirmed state.

## Implementation
- Update the Windows helper in `mic_mute_bridge/main.cpp` to recognize single-click vs double-click timing.
- Make single-click toggle the default microphone mute state.
- Make double-click switch both microphone and speaker defaults together.
- Load explicit profile pairs from an external JSON file next to the helper EXE.
- Apply the selected mic and speaker across the Windows console, multimedia, and communications roles.
- Add a tray balloon notification when the profile changes or the mic mute state changes.
- Keep the existing LED sync path, but use the helper as the source of truth for the mic state.

## Firmware
- Keep the short-click encoder trigger in the firmware.
- Remove any optimistic local mic-state flip that would conflict with double-click handling.
- Leave the long-press menu behavior unchanged.

## Config Shape
- Use a profile list with explicit microphone and speaker names.
- Treat profile names as user-facing labels for notifications.
- Keep the JSON external so device names can be edited without rebuilding the helper.

## Test Plan
- Verify a single click toggles mic mute only.
- Verify a double click advances to the next mic + speaker profile.
- Verify the tray balloon reflects the selected profile.
- Verify the LED state still matches the helper-confirmed mic state.
- Verify missing or invalid config entries fail safely.

## Assumptions
- Double-click is the preferred way to keep mic mute and device switching separate on the same encoder button.
- The helper remains the Windows-side owner of audio-device selection.
- The screenshot-like picker is not required; direct endpoint switching plus Windows notification feedback is the intended path.
