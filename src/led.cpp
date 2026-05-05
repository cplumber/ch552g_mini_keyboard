#include <Arduino.h>
#include "neo/neo.h"
#include "led.h"

// ===================================================================================
// Color section
// ============================================================================

static enum led_keyboard_mode_t led_mode_s = LED_LOOP;
static int color_hue_s[3] = {0, 0, 0}; // hue value: 0..191 color map
static int curretn_key_s = -1;         // current press key
static volatile uint8_t mic_muted_s = 0; // cached microphone state

void led_set_color_hue(uint8_t led0, uint8_t led1, uint8_t led2)
{
  color_hue_s[0] = led0;
  color_hue_s[1] = led1;
  color_hue_s[2] = led2;
}

void led_set_mode(enum led_keyboard_mode_t mode)
{
  led_mode_s = mode;
  switch (mode)
  {
  case LED_LOOP:
    color_hue_s[0] = NEO_RED;
    color_hue_s[1] = NEO_YEL;
    color_hue_s[2] = NEO_GREEN;
    break;
  }
}

void led_set_mic_mute_state(uint8_t muted)
{
  mic_muted_s = muted ? 1 : 0;
}

uint8_t led_get_mic_mute_state(void)
{
  return mic_muted_s;
}

// if in loop mode, change color to pressed key
void led_presskey(int key)
{
  curretn_key_s = key;
}

void led_update()
{
  if (led_mode_s == LED_LOOP)
  {
    uint8_t base_r = 0;
    uint8_t base_g = 0;
    uint8_t base_b = 0;

    if (mic_muted_s)
    {
      base_r = 2;
      base_g = 1;
    }
    else
    {
      base_g = 2;
    }

    for (int led = 0; led < 3; led++)
    {
      NEO_writeColor(led, base_r, base_g, base_b);
    }

    if (curretn_key_s >= 0 && curretn_key_s < 3)
    {
      NEO_writeColor(curretn_key_s, 1, 1, 1);
    }
  }
  else
  {
    for (int led = 0; led < 3; led++)
    {
      if (curretn_key_s == led)
      {
        NEO_writeColor(led, 255, 255, 255);
      }
      else
      {
        NEO_writeHue(led, color_hue_s[led], NEO_BRIGHT_KEYS);
      }
    }
  }

  NEO_update();
}
