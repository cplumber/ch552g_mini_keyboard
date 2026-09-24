#pragma once

#include "../configuration.h"

// Board selection is a compile-time property. Select it through
// scripts/build.ps1 -BoardVariant; do not try to infer it at runtime.
#if defined(BOARD_VARIANT_6KEY)

#define BOARD_VARIANT_NAME "six-key"
#define BOARD_HAS_SIX_KEYS 1

// Six-key board: left column is the supported three-button column.
#define PIN_BTN_1 SIX_KEY_BTN_1
#define PIN_BTN_2 SIX_KEY_BTN_2
#define PIN_BTN_3 SIX_KEY_BTN_3
#define PIN_BTN_ENC SIX_KEY_ENCODER_PRESS
#define ENCODER_A SIX_KEY_ENCODER_A
#define ENCODER_B SIX_KEY_ENCODER_B
#define PIN_BOOT_SW2 SIX_KEY_BOOT_PIN
#define PIN_NEO P34        // P3.4
#define BOARD_NEO_COUNT 6

#else

#define BOARD_VARIANT_NAME "three-key"
#define BOARD_HAS_SIX_KEYS 0

#define PIN_BTN_1 11       // P1.1
#define PIN_BTN_2 17       // P1.7
#define PIN_BTN_3 16       // P1.6
#define PIN_BTN_ENC 33     // P3.3
#define ENCODER_A 31       // P3.1
#define ENCODER_B 30       // P3.0
#define PIN_NEO P34        // P3.4
#define BOARD_NEO_COUNT 3

#endif
