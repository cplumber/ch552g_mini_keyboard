#pragma once



// Move to internal Bootloader
void BOOT_now(void);

// Briefly flash every configured LED low-intensity amber before bootloader entry.
void BOOT_with_indicator(void);
