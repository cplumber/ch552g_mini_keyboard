# Plan: Profile-Aware Application and Browser-Tab Targeting

## Goal

Extend the Windows utility and firmware so that selecting a keyboard profile
selects and verifies the correct application window before that profile's
shortcuts are enabled.

The first supported use case is a profile for a web application: locate the first
matching browser window, select the tab running the requested site, and restore
that window as the shortcut target.

## Delivery order and host application

Do not create a second resident Windows application. Extend the existing mute
bridge and rename its executable to `macropad-companion.exe`; it becomes the
single background process for microphone synchronization and profile targeting.
Keep `macropad-config.exe` as the separate, short-lived configuration CLI.

Build and debug all target-selection behavior before changing firmware. Add a
host-only command for direct testing:

```powershell
macropad-companion.exe --target vs_code
macropad-companion.exe --target ms_teams
```

Each command loads `macropad-targets.json`, finds/activates its target, reports
the decision, and exits. It does not require a keyboard connection, a profile
notification, or a firmware rebuild. Only after these commands are dependable
will the resident companion listen for `PROFILE_CHANGED`; the firmware protocol
and one final device flash are the last implementation step.

Parse `--target` before creating the companion singleton, opening Core Audio, or
opening any HID handle. It must be a true standalone diagnostic mode, dependent
only on the target file and Windows window/browser APIs.

## Scope

- Add one small vendor-HID profile-state protocol; a firmware change is required.
- Keep microphone mute synchronization working as it does today.
- Add optional target metadata to each profile.
- Support ordinary desktop application windows first.
- Support Google Chrome browser tabs through a controlled discovery mechanism.
- For target-managed profiles, do not send shortcuts until the Windows utility
  has verified the target and marked the selected profile ready.
- Do not add a host round trip, polling delay, or extra delay between macro
  chords when a profile button is pressed.
- Preserve normal foreground-window behavior for profiles without a target.
- Defer Edge and Firefox support until after the first working Chrome release.

## Profile metadata

Add an optional `target` object to the host-side profile configuration. Existing
configuration files without this object remain valid.

```json
{
  "profiles": {
    "ms_teams": {
      "target": {
        "application": "chrome.exe",
        "browser": "chrome",
        "debug_port": 9222,
        "url_contains": "teams.microsoft.com",
        "tab_title_contains": "Microsoft Teams"
      }
    },
    "vs_code": {
      "target": {
        "application": "Code.exe",
        "window_title_contains": "Visual Studio Code"
      }
    }
  }
}
```

Recommended fields:

- `application`: executable image name, matched case-insensitively.
- `window_title_contains`: optional desktop-window title filter.
- `url_contains`: required browser-tab URL filter.
- `tab_title_contains`: required browser-tab title filter and browser-window
  title matcher.
- `browser`: required value `chrome` for the first version.
- `debug_port`: required loopback DevTools port for a browser target.

A profile with a `target` object is target-managed: its macros require bridge
authorization. A profile without one keeps the current standalone behavior.

Target metadata lives in a separate host-side file, for example
`macropad-targets.json`, beside `macropad_tools\build\macropad-companion.exe`.
It is deliberately separate from `keyboard-config.json`: keyboard macros belong
in DataFlash, while machine-specific window and browser details do not.
Resolve this default file from the companion executable's directory, never from
the caller's current working directory. An optional explicit target-file command
line argument may be added for diagnostics and tests.

Keep one explicit profile-index map in the companion that matches firmware:
`0=copy_paste`, `1=google_meet`, `2=vs_code`, `3=ms_teams`. The host-only
`--target <name>` command uses names; the final HID integration converts them
through this fixed map. Do not infer an index from JSON object order.

## Required profile-state protocol

The current firmware changes profiles locally and sends keyboard HID reports
directly. The Windows utility is not told which profile was selected, and it
cannot intercept a keyboard report that firmware has already sent. Therefore the
original “no firmware protocol change” and “suppress the shortcut” statements
are not feasible.

Add three compact vendor-HID messages:

- `PROFILE_CHANGED` input notification: emitted after profile selection is
  committed; contains the profile index and an eight-bit selection token.
- `SET_PROFILE_READY` output command: contains the selected profile index and a
  ready flag, and echoes that selection token.
- `SET_TARGETING_MANAGED_MASK` output command: contains a four-bit mask naming
  profiles that have a `target` object; its response includes the currently
  selected profile index and token.

Transport these messages in a new vendor HID top-level collection, usage `3`,
with report ID `7`. Keep the existing configuration collection, usage `2` and
report ID `6`, exclusively for `macropad-config.exe`. This prevents the resident
companion and the configuration CLI from competing to read the same HID input
reports. The new collection reuses the existing nine-byte endpoint report size;
it does not add a USB endpoint.

Every profile-state input payload begins with a one-byte kind (`PROFILE_CHANGED`
or `MANAGED_MASK_RESPONSE`). The managed-mask response echoes a one-byte command
sequence. `SET_PROFILE_READY` is deliberately fire-and-forget: its matching
selection token is the firmware-side acceptance check, so an acknowledgement
would add no value. This lets the companion distinguish an asynchronous selection
event from the startup handshake response while both use report ID `7`.

Freeze the eight data-byte layouts now; all reserved bytes are zero and firmware
rejects a command with a nonzero reserved byte or an invalid enum/mask.

| Direction | Bytes `0..7` |
| --- | --- |
| output `SET_TARGETING_MANAGED_MASK` | command `1`, sequence, four-bit managed mask, `0`, `0`, `0`, `0`, `0` |
| output `SET_PROFILE_READY` | command `2`, profile index, ready (`0` or `1`), selection token, `0`, `0`, `0`, `0` |
| input `PROFILE_CHANGED` | kind `1`, profile index, selection token, `0`, `0`, `0`, `0`, `0` |
| input `MANAGED_MASK_RESPONSE` | kind `2`, sequence, status (`0` = accepted), current profile index, selection token, accepted managed mask, `0`, `0` |

Normal profile indexes are `0..3`; index `4` is accepted only as the current
menu profile in a managed-mask response. `PROFILE_CHANGED` and
`SET_PROFILE_READY` never accept index `4`. The managed-mask's upper four bits
must be zero.

Only one managed-mask request may be in flight. The output writer does not send
another one until it receives the response with that sequence or tears down the
channel; this makes the eight-bit sequence unambiguous across reconnects.
The host accepts the handshake only when the response sequence matches, status
is zero, and the returned accepted mask equals the requested four-bit mask.
Any mismatch follows the startup-failure path.

The host validates every incoming byte before acting. A malformed or unknown
report is a profile-state channel failure, not an event to ignore: invalidate
the target, best-effort clear the managed mask, and reconnect. A rejected
managed-mask response is the same targeting-channel startup failure path.
Targeting-channel failure must not terminate the combined companion or its
existing microphone mute synchronization.

At bridge startup, validate the target file first and compute one requested
managed mask. Use the validated mask when the file is valid, or zero when it is
missing/invalid. After opening the profile-state channel, send that one mask and
complete its response handshake; applying any mask clears stale readiness and
managed bits left by an earlier companion instance or crash. On a managed
profile selection, firmware clears that profile's ready bit before emitting
`PROFILE_CHANGED`. The bridge resolves and foregrounds the target asynchronously,
then sends `SET_PROFILE_READY(profile, true)` only after verification succeeds.
Firmware accepts a ready command only when its token matches the current profile
selection, so a delayed lookup for an older selection cannot re-enable keys.
Increment the token on every committed normal-profile selection, including a
reselection of the same profile; it is not merely a profile-index change counter.

When applying the managed mask, firmware clears the ready bits of all managed
profiles and returns the current profile index and token in the command response.
If that current profile is managed, the bridge immediately resolves it; if it is
unmanaged (or the menu profile), the bridge does nothing. This covers the case
where the bridge starts after a managed profile has already been selected,
without adding another HID message.

If that response and a following `PROFILE_CHANGED` describe the same
profile/token, the companion coalesces them into one resolver request. This can
happen when a selection occurs while the startup response waits for a busy
endpoint.

The bridge waits for the matching managed-mask response before treating the HID
channel as initialized. If that response is absent, malformed, or cannot be
read, it best-effort sends a zero managed mask and reports startup failure; it
must not remain running with profiles silently gated but no working reader. This
disables only profile targeting; the resident microphone bridge continues and
keeps retrying the profile-state channel. The
same best-effort clear applies when the profile-state reader fails after startup,
before the companion exits or reconnects.

On a device removal or profile-state read/write failure, invalidate the cached
target immediately and close both usage-`3` handles. While the resident
companion remains running, retry opening usage `3` at the existing low-frequency
device-retry cadence. A successful reconnect repeats the managed-mask handshake
and resolves the returned current managed profile. Device reconnection therefore
restores targeting without a companion restart; it never marks a profile ready
from stale pre-disconnect state.

For clean shutdown or an intentional reconnect, explicitly cancel the blocking
reader I/O (for example with `CancelSynchronousIo` on the reader thread) and
join that reader before closing/replacing its handle. Do not depend on
`CloseHandle` alone to wake a blocking `ReadFile`.

Firmware keeps the managed mask and readiness in RAM only. It starts with no
managed profiles, so the keyboard has its current standalone behavior until the
bridge actively enables targeting. On clean bridge exit, the bridge clears the
managed mask. If the bridge crashes, managed profiles stay safely blocked until
the bridge restarts or the USB device reconnects; no heartbeat is needed.

Clean companion shutdown is ordered: first set a stopping flag and invalidate
all bindings; then stop the resolver and event callbacks from enqueueing work;
then make the output writer discard queued ready commands and write a zero
managed mask; finally cancel/join the reader and close the handles. No later
ready command may be written after the zero mask. If the device is already gone,
the normal disconnect behavior applies instead.

If `macropad-targets.json` is missing or invalid, the companion must send only the
zero managed-mask reset (when the device channel is available), never enable any
target bits, and report the configuration error. The keyboard therefore returns
to its normal standalone mode rather than remaining blocked by stale state or a
bad host-side configuration; the microphone mute bridge remains operational.

For a managed profile, firmware checks its ready bit immediately before every
profile-local keyboard action. This covers the existing three-button macro and
the VS Code encoder actions that emit `Alt+Tab`; otherwise an unready VS Code
profile could still send a context-sensitive shortcut. The global microphone
command, menu navigation, and global volume controls remain independent. The
check is a branch only; it adds no `delay()` and preserves all current macro
timing unchanged.

Profile-state transport must be non-blocking. The existing `USB_EP1_send()`
waits up to 250 ms for a busy endpoint, so do not call it for report ID `7`.
Add a report-7 try-send path that returns immediately when `UpPoint1_Busy` is
set, and call it only from the normal update loop. If the endpoint is busy when
a profile is committed, firmware stores the latest profile index and selection
token in two pending bytes; newer selections replace older pending values.

The managed-mask command response also uses the deferred report-7 path. Keep
only a pending flag plus its sequence, status, and accepted mask; construct the
eight-byte report in the endpoint buffer when it is sent. Send that response
before a pending profile-change notification. This is the minimum extra RAM
required for the only request/response operation; `SET_PROFILE_READY` has no
reply. The normal update loop sends the queued response or latest notification
only after the endpoint is free. This keeps the final selected profile and
startup handshake reliable without waiting in the button or USB OUT paths or
adding host polling.

The queued managed-mask response keeps only its sequence, status, and accepted
mask. Fill its current-profile index and selection token immediately before it
is transmitted. If a profile changed while the endpoint was busy, the response
therefore describes that newest selection rather than causing a stale resolver
to run before the queued `PROFILE_CHANGED` event.

Firmware emits `PROFILE_CHANGED` only when the long-press menu is committed to
one of the four normal profiles, never while merely entering, navigating, or
leaving the menu. The companion validates every received index and ignores an
unknown index or the menu index. Firmware already clears a managed profile's
ready bit before it emits the notification, so the companion does not send a
redundant "not ready" command for that normal selection.

Entering the menu also discards any unsent `PROFILE_CHANGED` notification. A
queued profile event is therefore never delivered after the user has moved the
device back into menu mode, where no profile-local shortcut can run.

## Target selection flow

When a profile is selected:

1. Read the profile's target metadata.
2. Enumerate visible top-level windows with `EnumWindows`.
3. For each candidate, obtain the owning process with `GetWindowThreadProcessId`.
4. Compare the process image name with `application`.
5. Filter by title and normal application-window state: visible, not cloaked,
   and not an owned/tool window.
6. If the target is a browser tab, inspect the browser's tab list and find the
   tab whose URL/title matches the profile.
7. Restore the window if minimized and call `SetForegroundWindow`.
8. Confirm with `GetForegroundWindow` that the selected window is foreground,
   then set the profile ready bit.

Do not attempt privilege escalation or `AttachThreadInput` workarounds. The
companion and target must run at the same Windows integrity level. If Windows
refuses foreground activation—for example, because the target is elevated, on a
secure desktop, or otherwise unavailable—leave the profile unready and report
the reason.

For an ordinary application with multiple matching windows, “first” means the
topmost matching normal application window in Windows z-order. `EnumWindows`
provides this order. A normal candidate is visible, not `DWMWA_CLOAKED`, has no
owner, and lacks `WS_EX_TOOLWINDOW`; a title filter may narrow it further. Log
the selected window and every rejected candidate so the choice is diagnosable.

## Browser-tab targeting

Use a separate browser adapter rather than embedding browser-specific logic in
the generic window selector.

Initial adapter design:

- Implement Chrome first. The selected Chrome instance must be launched with a
  configured local debugging endpoint, such as `--remote-debugging-port=9222`.
  Current Chrome also requires a non-default `--user-data-dir` with that flag;
  use a dedicated profile for this feature and sign in to Teams there once.
  Edge is deferred until it has its own successful feasibility test.
- Query the browser's tab list through its local DevTools HTTP `/json/list`
  endpoint. First require `/json/version` to return a successful Chrome DevTools
  response (including a `Browser` value identifying Chrome); otherwise treat the
  configured port as unavailable.
- Consider only entries whose target type is `page`; ignore extensions, workers,
  DevTools, and browser-internal targets.
- Match both `url_contains` and `tab_title_contains`.
- Require a unique matching tab. If more than one tab matches, leave the profile
  unready and require a more specific URL filter.
- Activate the matching tab through the DevTools HTTP `/json/activate/<targetId>`
  endpoint and require its successful HTTP response. Subscribe to the matching
  browser-window name-change before that request (the resident hook already
  exists; `--target` installs a temporary equivalent), then first check
  immediately and otherwise wait event-driven for the activated tab title to
  appear. Use one short, bounded confirmation timeout; this happens only during
  profile selection and does not delay a macro button press. Find its visible
  `HWND` by process and activated-tab title, and bring it to foreground only when
  that match is unique; otherwise leave the profile unready.

DevTools browser window IDs are not Windows `HWND` values, so do not attempt to
cast or compare them. The first version deliberately supports browser targets
only when the activated tab produces a distinctive browser-window title.

The adapter's observable browser identity is the matched URL/title plus the
browser-window title. A later manual switch to a different tab that leaves the
browser-window title unchanged cannot be detected by the HTTP-only API and is
outside this version's guarantee, even if the tab URL changed. Target
configurations must therefore use the most specific stable URL filter available;
duplicate matches are always rejected at resolution time, and the user must
reselect the profile after an indistinguishable-tab change.
Do not describe HTTP-only targeting as an absolute guarantee of tab identity.

Use only loopback DevTools endpoints and store an explicit port for each browser
target. Do not scan the network or expose a remote-debugging port beyond the
local machine.

Use the Windows `WinHTTP` API for the three local GET requests: `/json/version`,
`/json/list`, and `/json/activate/<targetId>`. Open these requests with an
explicit direct/no-proxy setting; they must connect only to `127.0.0.1` and must
not inherit a system proxy. Do not add a WebSocket client, browser extension,
or third-party dependency in the first version. Window title and foreground
events provide the no-delay invalidation path.

Treat `<targetId>` as a URL path component and percent-encode it before the
activate request; do not assume DevTools target identifiers are always safe to
place verbatim in a URL.

Give each local `WinHTTP` operation a short finite timeout (for example, one
second) and do not automatically retry. A timeout leaves the profile unready and
reports the failure; the next profile selection starts a fresh lookup. This keeps
the target resolver bounded and adds no delay to an already-ready macro press.

### Chrome setup for the first implementation

Chrome remote debugging must be enabled when Chrome starts. It cannot be enabled
later from Chrome settings, and current Chrome ignores the debugging-port flag
for its default user-data directory. Start one dedicated Chrome instance for
target-managed web applications:

```powershell
$chromeProfile = "$env:LOCALAPPDATA\macropad-browser-profiles\chrome"

& "$env:ProgramFiles\Google\Chrome\Application\chrome.exe" `
  --remote-debugging-port=9222 `
  --user-data-dir="$chromeProfile"
```

This creates a separate Chrome profile that can run alongside normal Chrome.
Sign in to Teams in this dedicated profile once, then use that window for the
MS Teams profile. Confirm that the local endpoint is available before testing
the companion:

```powershell
Invoke-RestMethod http://127.0.0.1:9222/json/version
```

The PowerShell backticks above must be the final characters on their lines.

The companion never starts, restarts, signs in to, or closes Chrome. Chrome is a
user-managed prerequisite: if its configured local endpoint is unavailable, the
companion reports that condition and leaves the profile unready.

Do not add UI Automation or simulated `Ctrl+Tab` fallback in the first version.
Neither provides sufficiently reliable tab identity. If the debugging endpoint
is unavailable, leave the managed profile unready and show a diagnostic.

Edge and Firefox are explicitly out of scope for the first release. If added
later, each needs its own feasibility test; do not add UI Automation or synthetic
tab navigation as a fallback.

The companion executable should provide a diagnostic command such as:

```powershell
macropad-companion.exe --targets
```

It should list detected processes, matching windows, browser tabs, and the reason
each candidate was accepted or rejected.

## Shortcut delivery rules

- On `PROFILE_CHANGED`, firmware has already cleared the selected managed
  profile's ready bit, and the target manager resolves and activates its target
  asynchronously.
- Firmware owns the final ready-bit check before every profile-local keyboard
  HID action. A permitted macro is sent immediately and its existing chord
  delays are unchanged.g1
- Cache the selected target handle/tab identity in the bridge for diagnostics.
- Install two narrow `SetWinEventHook` registrations, sharing one callback:
  one for `EVENT_SYSTEM_FOREGROUND` and one for `EVENT_OBJECT_NAMECHANGE`.
  These event values are not contiguous, so do not use one broad event range.
  If the foreground window leaves the managed target, clear its ready bit. For a
  browser target, a top-level window title change that no longer matches the
  activated tab title also clears readiness. Ignore child accessibility events;
  evaluate only `OBJID_WINDOW` for the stored target HWND. Register each hook as
  out-of-context with `WINEVENT_SKIPOWNPROCESS` and keep a normal Windows
  message loop running for its lifetime; Windows delivers these callbacks through
  that loop. The callback only filters and queues work, so it never blocks event
  delivery or performs HID I/O.
- If target activation fails, the selected managed profile remains unready and
  its buttons produce no macro.
- Profiles with no `target` object continue to use the current foreground window.
- The global microphone mute command remains independent of application targeting.

The input reader records the newest selected profile and token immediately and
invalidates any previous target binding, even while the bounded resolver is in a
browser request. Before a resolver foregrounds a window and before it queues
`SET_PROFILE_READY`, it checks that its profile/token is still the newest
selection. The output writer makes the same profile/token check immediately
before writing a ready command. If obsolete, the work is abandoned without
changing foreground focus or readiness. This prevents a slow lookup for an old
profile from stealing focus after the user selected a new one.

Each cached target binding and queued foreground/title event also carries the
profile, token, and target HWND observed at the time. The worker discards a
queued readiness-clear operation unless that exact binding is still current.
Thus a late event from an old target cannot clear readiness for a newer
selection of the same profile after the eight-bit token has advanced.

A target binding also has a host-side validity generation. A foreground/title
loss invalidates that binding before it queues `SET_PROFILE_READY(false)`. A
successful resolver tags its queued `SET_PROFILE_READY(true)` with the generation
it verified, and the output writer rechecks that generation immediately before
writing. It drops a stale ready command. This prevents an already-queued ready
command from re-enabling a target after a higher-priority focus-loss clear.

This design is intentionally not a per-key transaction. It reliably targets the
window when a profile is selected and blocks macros after that window loses
foreground status or its matching browser-tab title changes. A user returning to
the intended tab manually must reselect the profile to mark it ready again.

## Configuration validation

The Windows utility must reject:

- A target file whose root is not exactly the supported schema: `version: 1`
  and a `profiles` object.
- Duplicate JSON object members at any level, trailing non-JSON content, or a
  value with the wrong JSON type. Do not use a parser mode that silently keeps
  the last duplicate member.
- Unknown profile names or target fields.
- Empty `application` values.
- Empty supplied title or URL filters. A missing desktop title filter is valid;
  an explicitly empty one is not.
- A browser value other than `chrome`, or a browser target whose application is
  not `chrome.exe`.
- Missing or out-of-range `debug_port` values for browser targets. The companion
  always connects only to `127.0.0.1:<debug_port>`.
- Browser targets without both `url_contains` and `tab_title_contains`.
- `url_contains`, `tab_title_contains`, or `debug_port` on a non-browser target.
- Profiles whose target requirements cannot be satisfied by the installed
  adapters.

Validation occurs when the bridge starts and before any host-side target file is
accepted. Parse the file with a real standards-compliant JSON parser; do not
reuse the configuration CLI's substring-based parser for this stricter input.
Target metadata is not written into the keyboard's limited DataFlash.

Read the target file once at companion startup. Do not add a file watcher or
live reload in the first version; after editing it, restart the companion so the
complete new file is validated and applied as one configuration.

## Logging and diagnostics

Log these events with profile name and target details:

- Profile selected.
- Candidate windows found and rejected.
- Browser endpoint unavailable.
- Tab activated.
- Foreground activation failed.
- Target lost and shortcut suppressed.

Keep normal operation quiet; expose detailed output only through a `--verbose`
option or the `targets` diagnostic command.

## Implementation sequence

1. Rename the bridge source, executable, Startup shortcut, and documentation to
   `macropad-companion`; preserve its current microphone behavior. Rename its
   singleton/stop-event names and console title as well. As part of the same
   migration, stop any old `mic-mute-bridge.exe` process and remove its old
   Startup entry before starting the renamed companion; changing the mutex name
   alone would allow both binaries to run concurrently. Update
   `macropad_tools\build.bat` and any test/output names in the same change; keep
   `macropad-config.exe` unchanged.
2. Add a separate target-file parser and strict validation to the companion.
3. Implement generic process/window enumeration and foreground activation.
4. Add `--target <profile>` and verbose diagnostics; test VS Code with multiple
   windows without the keyboard connected. Repeat the activation test from the
   real background Startup-launched companion context; do not treat a
   foreground-console success as proof that `SetForegroundWindow` will work for
   the resident process.
5. Add the Chrome remote-debugging adapter; test Chrome tab discovery and
   activation with a dedicated browser profile.
6. Add target examples for VS Code and Microsoft Teams web.
7. Test multiple windows, minimized windows, missing tabs, ambiguous matches,
   browser failure, foreground changes, manual browser-tab changes, navigation
   that changes the tab title, elevated target windows, and USB disconnect/
   reconnect while the companion stays running. Also test a VS Code encoder
   action while unready, reselection of the same profile, a selection while a
   report-7 response is deferred, malformed report-7 input, and clean shutdown
   with queued ready work. Include duplicate browser tabs with identical URL/
   title and verify they remain unready. Verify invalid target configuration or
   a missing usage-`3` collection leaves F24 microphone synchronization working.
   Connect two identical macropads and verify profile targeting refuses the
   ambiguous device set rather than choosing one arbitrarily.
8. Only after host tests pass, add firmware profile-change notification, managed
   mask, per-profile RAM ready bits, and non-blocking pending notification.
9. Add the dedicated vendor HID profile-state collection (usage `3`, report ID
    `7`). Extend `macropad_tools\common\macropad_hid` with shared usage-`3`
    discovery and separate reader/writer handle support; do not duplicate SetupAPI
    enumeration in the companion. Leave the configuration CLI on its usage-`2`
    API unchanged.
   Update the report descriptor in
   `src\userUsbHidKeyboardMouse\USBconstant.c`, parse usage-`3` output and
   schedule usage-`3` input in
   `src\userUsbHidKeyboardMouse\USBHIDKeyboardMouse.c`, and keep all report-7
   paths within the existing endpoint-1 buffers.
10. Wire `PROFILE_CHANGED` to the already tested target resolver plus
    `SET_PROFILE_READY`. In `keyboard.cpp`, gate both `macro_config_run()` and
    the VS Code encoder functions on the selected profile's ready bit; leave
    microphone, menu, and volume actions ungated.
    Call the report-7 deferred-drain routine from the regular `loop()` in
    `ch552g_mini_keyboard.ino`, after `buttons_update()` and `keyboard_update()`
    but before encoder/LED work; it must return immediately when endpoint 1 is
    busy and must never add a delay.
11. Update `readme.md`, `current_config.md`, `agents.md`,
    `key_reassignment_plan.md`, and `macropad_tools\README.md`. Search all
    maintained documentation for stale `mic-mute-bridge`/`mic_mute_bridge`
    names and paths; update them to `macropad-companion`/`macropad_tools`,
    including the dedicated-Chrome launch command, target-file location/schema,
    diagnostics, and profile-to-LED mapping. Preserve old names only when they
    are intentionally historical references.
12. Rebuild firmware and check the CH552 flash/RAM report, then perform the one
    final firmware flash.

The companion uses one blocking input reader, one bounded target-resolver worker,
and one serialized output writer for the usage-`3` HID channel. It opens one
shared read handle for profile-state input reports and one shared write handle
for output commands. Only the reader performs reads; only the output writer
performs writes. The reader and window-event callback can enqueue a
high-priority `SET_PROFILE_READY(false)` command directly to that writer, while
the resolver performs window/WinHTTP work separately and queues a successful
`SET_PROFILE_READY(true)` only after verification. Thus a slow lookup never
holds up a readiness-clear command, writes never race, and the bounded resolver
does not run on the event callback or HID writer.

Usage-`3` HID discovery must be unambiguous. Enumerate all matching VID/PID
profile-state collections and require exactly one device; never select the first
matching interface. If multiple devices are present, disable targeting and
report the ambiguity while leaving microphone synchronization operational.

## Reliability boundary

The ready gate prevents macros until targeting has completed, and foreground/title
events clear readiness after a target changes. Windows cannot atomically combine
`SetForegroundWindow` with a later physical keyboard report, so a tiny external
focus-change race remains theoretically possible. Eliminating that race requires
a host round trip for every macro press, which would add latency and was
explicitly rejected. The selected design is the no-delay, practical-reliability
option.

## Acceptance criteria

- Selecting a managed desktop-app profile focuses the first visible matching
  window in z-order.
- The same desktop activation succeeds (or reports the documented foreground
  restriction) when initiated by the background resident companion, not only by
  the foreground diagnostic command.
- Selecting a managed browser profile activates one uniquely matching Teams/web
  tab.
- A target-managed keyboard action adds no host communication or delay on
  button press; its existing macro timing is unchanged.
- An ambiguous, missing, or unverified target leaves the selected profile
  unready, so it produces no profile-local shortcut.
- A VS Code encoder shortcut is also suppressed while its target is unready;
  global mic and volume actions remain available.
- Leaving a managed desktop target clears readiness before later macros run.
- Changing the title of a managed browser window to another tab clears readiness
  before later macros run.
- Duplicate or indistinguishable browser-tab matches never become ready.
- Existing mute synchronization and unconfigured-profile behavior are unchanged.
- Missing/invalid target configuration or a usage-`3` HID failure does not stop
  microphone mute synchronization.
- Reselecting the same profile invalidates its previous ready state and cannot
  accept an older ready command or deferred notification.
- The feature requires one firmware rebuild/flash, but no macro DataFlash format
  change.
