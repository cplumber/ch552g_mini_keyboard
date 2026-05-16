#pragma once

#include <stdint.h>

// Key colors (hue value: 0..191)
#define NEO_RED 0    // red
#define NEO_YEL 32   // yellow
#define NEO_GREEN 64 // green
#define NEO_CYAN 96  // cyan
#define NEO_BLUE 128 // blue
#define NEO_MAG 160  // magenta
#define NEO_WHITE 191  // white
#define NEO_BRIGHT_KEYS 0

#define LED_0 0 // farthest from rotary switch, kept off
#define LED_1 1 // middle LED, menu/profile selection
#define LED_2 2 // closest to rotary switch, mic mute/live

enum led_keyboard_mode_t
{
  LED_LOOP,
  LED_MENU
};

// change led mode
void led_set_mode(enum led_keyboard_mode_t mode);

#ifdef __cplusplus
extern "C" {
#endif
// update the cached mic mute state for the normal LED mode
void led_set_mic_mute_state(uint8_t muted);

// set the selected profile indicator for menu mode
void led_set_menu_profile(uint8_t profile);

// blank LEDs while the USB bus is suspended
void led_set_usb_suspended(uint8_t suspended);

// optional C-callable accessor for the USB bridge
uint8_t led_get_mic_mute_state(void);
#ifdef __cplusplus
}
#endif

// update led task
void led_update();

//if in loop mode, change color to pressed key
void led_presskey(int key);

