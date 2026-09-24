#pragma once
#include <stdint.h>
#include "src/keyboard.h"

extern const keyboard_configuration_t configurations[NUM_CONFIGURATION];

// Discovered six-key + knob board wiring (hardware mapper, 2026-09-24).
// All six physical keys are confirmed. The main application uses BTN_1..BTN_3
// from the left column; the six-key build also exposes right/top/middle/bottom
// as BTN_4..BTN_6.
#define SIX_KEY_BTN_1 11       // CONFIRMED: left/top -> logical BTN_1, P1.1 (b)
#define SIX_KEY_BTN_2 17       // CONFIRMED: left/middle -> logical BTN_2, P1.7 (h)
#define SIX_KEY_BTN_3 16       // CONFIRMED: left/bottom -> logical BTN_3, P1.6 (g)
#define SIX_KEY_BTN_4 32       // CONFIRMED: right/top -> BTN_4, P3.2
#define SIX_KEY_BTN_5 14       // CONFIRMED: right/middle -> BTN_5, P1.4
#define SIX_KEY_BTN_6 15       // CONFIRMED: right/bottom -> BTN_6, P1.5 / SW2
#define SIX_KEY_ENCODER_A 30   // P3.0
#define SIX_KEY_ENCODER_B 31   // P3.1
#define SIX_KEY_ENCODER_PRESS 33 // P3.3
#define SIX_KEY_BOOT_PIN 15    // SW2 pulls P1.5 low at power-up

// All six LED positions are directly confirmed. With the knob at the top:
//   left/top=2, left/middle=1, left/bottom=0
//   right/top=5, right/middle=4, right/bottom=3
#define SIX_KEY_LEFT_BOTTOM_LED_PIXEL 0
#define SIX_KEY_LEFT_MIDDLE_LED_PIXEL 1
#define SIX_KEY_LEFT_TOP_LED_PIXEL 2
#define SIX_KEY_RIGHT_BOTTOM_LED_PIXEL 3
#define SIX_KEY_RIGHT_MIDDLE_LED_PIXEL 4
#define SIX_KEY_RIGHT_TOP_LED_PIXEL 5

// Logical indicators used by the main application.
#define SIX_KEY_LED_0_PIXEL SIX_KEY_LEFT_BOTTOM_LED_PIXEL // off
#define SIX_KEY_LED_1_PIXEL SIX_KEY_LEFT_MIDDLE_LED_PIXEL // menu/profile
#define SIX_KEY_LED_2_PIXEL SIX_KEY_LEFT_TOP_LED_PIXEL    // mic/live
