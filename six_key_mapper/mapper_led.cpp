#include <Arduino.h>

#include "src/neo/neo.h"
#include "mapper_led.h"

static uint8_t active_pixel_s = 0;
static uint8_t active_color_s = 0;
static unsigned long last_pixel_change_ms_s = 0;

static void write_test_color(uint8_t pixel, uint8_t color)
{
  if (color == 0)
  {
    NEO_writeColor(pixel, 16, 0, 0); // red
  }
  else if (color == 1)
  {
    NEO_writeColor(pixel, 0, 16, 0); // green
  }
  else
  {
    NEO_writeColor(pixel, 0, 0, 16); // blue
  }
}

void mapper_led_setup(void)
{
  NEO_init();
  NEO_clearAll();
  write_test_color(0, 0);
  NEO_update();
  last_pixel_change_ms_s = millis();
}

void mapper_led_update(void)
{
  const unsigned long now = millis();
  if ((unsigned long)(now - last_pixel_change_ms_s) < 1000UL)
  {
    return;
  }

  last_pixel_change_ms_s = now;
  NEO_clearPixel(active_pixel_s);
  active_color_s++;
  if (active_color_s >= 3)
  {
    active_color_s = 0;
    active_pixel_s++;
    if (active_pixel_s >= NEO_COUNT)
    {
      active_pixel_s = 0;
    }
  }
  write_test_color(active_pixel_s, active_color_s);
  NEO_update();
}
