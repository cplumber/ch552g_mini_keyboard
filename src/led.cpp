#include <Arduino.h>
#include "neo/neo.h"
#include "led.h"

static enum led_keyboard_mode_t led_mode_s = LED_LOOP;
static uint8_t menu_profile_s = 0;
static uint8_t auto_hue_s = NEO_GREEN;
static volatile uint8_t mic_muted_s = 0; // cached microphone state
static volatile uint8_t usb_suspended_s = 0;
static unsigned long mic_blink_last_toggle_ms_s = 0;
static uint8_t mic_blink_visible_s = 1;

static const uint8_t menu_palette_s[] = {
    NEO_RED,
    NEO_YEL,
    NEO_GREEN,
    NEO_CYAN,
    NEO_BLUE,
    NEO_MAG,
    NEO_WHITE,
};

static const uint8_t menu_rgb_s[][3] = {
    {1, 0, 0}, // red
    {1, 1, 0}, // yellow
    {0, 1, 0}, // green
    {0, 1, 1}, // cyan
    {0, 0, 1}, // blue
    {1, 0, 1}, // magenta
    {1, 1, 1}, // white
};

static const uint8_t menu_brightness_s = 32;
static const uint8_t selected_brightness_s = 1;

static void set_pixel_off(uint8_t pixel)
{
  NEO_writeColor(pixel, 0, 0, 0);
}

static void write_menu_rgb(uint8_t index, uint8_t brightness)
{
  NEO_writeColor(LED_1,
                 menu_rgb_s[index][0] * brightness,
                 menu_rgb_s[index][1] * brightness,
                 menu_rgb_s[index][2] * brightness);
}

static void render_mic_pixel(void)
{
  if (mic_muted_s)
  {
    const unsigned long now = millis();
    if ((unsigned long)(now - mic_blink_last_toggle_ms_s) >= 500UL)
    {
      mic_blink_visible_s = !mic_blink_visible_s;
      mic_blink_last_toggle_ms_s = now;
    }

    if (mic_blink_visible_s)
    {
      NEO_writeColor(LED_2, 2, 1, 0);
    }
    else
    {
      set_pixel_off(LED_2);
    }
    return;
  }

  mic_blink_visible_s = 1;
  mic_blink_last_toggle_ms_s = millis();
  NEO_writeColor(LED_2, 0, 2, 0);
}

static void render_menu_pixel(void)
{
  if (led_mode_s == LED_MENU || led_mode_s == LED_LOOP)
  {
    const uint8_t index = menu_profile_s % (sizeof(menu_palette_s) / sizeof(menu_palette_s[0]));
    write_menu_rgb(index, led_mode_s == LED_MENU ? menu_brightness_s : selected_brightness_s);
    return;
  }

  if (led_mode_s == LED_AUTO)
  {
    NEO_writeHue(LED_1, auto_hue_s, NEO_BRIGHT_KEYS);
    return;
  }

  set_pixel_off(LED_1);
}

void led_set_mode(enum led_keyboard_mode_t mode)
{
  led_mode_s = mode;
}

void led_set_auto_hue(uint8_t hue)
{
  auto_hue_s = hue;
}

void led_set_mic_mute_state(uint8_t muted)
{
  mic_muted_s = muted ? 1 : 0;
  mic_blink_visible_s = 1;
  mic_blink_last_toggle_ms_s = millis();
}

void led_set_menu_profile(uint8_t profile)
{
  menu_profile_s = profile;
}

void led_set_usb_suspended(uint8_t suspended)
{
  usb_suspended_s = suspended ? 1 : 0;
}

uint8_t led_get_mic_mute_state(void)
{
  return mic_muted_s;
}

void led_presskey(int key)
{
  (void)key;
}

void led_update()
{
  if (usb_suspended_s)
  {
    for (uint8_t led = 0; led < 3; led++)
    {
      set_pixel_off(led);
    }
    NEO_update();
    return;
  }

  render_mic_pixel();
  render_menu_pixel();
  set_pixel_off(LED_0);

  NEO_update();
}
