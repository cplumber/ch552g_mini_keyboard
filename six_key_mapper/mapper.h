#pragma once

// Configure all safe GPIO candidates as pulled-up inputs.
void mapper_setup_inputs(void);

// Arm one automatic ROM-bootloader window per application start.  The marker
// survives the ROM loader's reset/jump, preventing an endless bootloader loop.
bool mapper_should_enter_bootloader(void);
void mapper_clear_bootloader_marker(void);

// True when a candidate control (other than the LED-data candidate) is held.
// Used only during startup to request the ROM bootloader.
bool mapper_recovery_requested(void);

// Emit a lower-case letter when a candidate pin changes.  See README.md for
// the letter-to-pin table; a text editor on the host is the event monitor.
void mapper_update(void);
