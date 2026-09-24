#pragma once
#include <stdint.h>
#include "src/keyboard.h"

extern const keyboard_configuration_t configurations[NUM_CONFIGURATION];

// Discovered six-key + knob board wiring (hardware mapper, 2026-09-24).
// The six-key build consumes the left-column definitions for BTN_1..BTN_3;
// the right-column definitions remain recorded for future expansion.
#define SIX_KEY_BTN_1 11       // CONFIRMED: left/top -> logical BTN_1, P1.1 (b)
#define SIX_KEY_BTN_2 17       // CONFIRMED: left/middle -> logical BTN_2, P1.7 (h)
#define SIX_KEY_BTN_3 16       // CONFIRMED: left/bottom -> logical BTN_3, P1.6 (g)
#define SIX_KEY_BTN_4 32       // right/top: mapper k, P3.2
#define SIX_KEY_BTN_5 14       // right/middle: mapper e, P1.4
#define SIX_KEY_BTN_6 15       // right/bottom: mapper f, P1.5 / SW2 line
#define SIX_KEY_ENCODER_A 30   // P3.0
#define SIX_KEY_ENCODER_B 31   // P3.1
#define SIX_KEY_ENCODER_PRESS 33 // P3.3
#define SIX_KEY_BOOT_PIN 15    // SW2 pulls P1.5 low at power-up

// Six-key LED strip order, recorded from the mapper test. Viewed from the top
// with the knob above the key grid, the physical layout is:
//       knob
//       3 6
//       2 5
//       1 4
// LED_0 (off): physical position not yet confirmed. Do not infer its location
// from the mapper drawing; record it only after a direct one-pixel test.
#define SIX_KEY_LED_0_PIXEL 0

// LED_1 (menu/profile): CONFIRMED to be the left-middle LED. Its pixel is 1.
#define SIX_KEY_LED_1_PIXEL 1

// LED_2 (mic/live): CONFIRMED to be the left-top LED. Its pixel is 2.
#define SIX_KEY_LED_2_PIXEL 2
