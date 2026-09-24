#pragma once
#include <stdint.h>
#include "src/keyboard.h"

extern const keyboard_configuration_t configurations[NUM_CONFIGURATION];

// Discovered six-key + knob board wiring (hardware mapper, 2026-09-24).
// The six-key build consumes the left-column definitions for BTN_1..BTN_3;
// the right-column definitions remain recorded for future expansion.
#define SIX_KEY_BTN_1 16       // left/top: mapper g, P1.6
#define SIX_KEY_BTN_2 17       // left/middle: mapper h, P1.7
#define SIX_KEY_BTN_3 11       // left/bottom: mapper b, P1.1
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
// The mapper's physical layout is numbered as shown above. The verified strip
// order is pixel indices 0..5 = physical positions 3, 6, 1, 4, 2, 5.
#define SIX_KEY_LED_PIXEL_0_POSITION 3
#define SIX_KEY_LED_PIXEL_1_POSITION 6
#define SIX_KEY_LED_PIXEL_2_POSITION 1
#define SIX_KEY_LED_PIXEL_3_POSITION 4
#define SIX_KEY_LED_PIXEL_4_POSITION 2
#define SIX_KEY_LED_PIXEL_5_POSITION 5

// Logical indicators used by the main firmware on the six-key board.
// These are the verified physical left-column placements:
// LED_0 = bottom-left/off, LED_1 = middle-left/menu, LED_2 = top-left/mic.
#define SIX_KEY_LED_0_PIXEL 0
#define SIX_KEY_LED_1_PIXEL 5
#define SIX_KEY_LED_2_PIXEL 2
