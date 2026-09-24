#include <Arduino.h>
#include "util.h"
#include "board_config.h"
#include "neo/neo.h"

void BOOT_with_indicator(void)
{
  for (uint8_t pixel = 0; pixel < BOARD_NEO_COUNT; pixel++)
  {
    NEO_writeColor(pixel, 8, 3, 0);
  }
  NEO_update();
  delay(35);
  NEO_clearAll();
  NEO_update();
  delay(5);
  BOOT_now();
}


// ===================================================================================
// Move to internal Bootloader
// ===================================================================================
void BOOT_now(void)
{
  USB_CTRL = 0;
  EA = 0;
  TMOD = 0;
  __asm lcall #BOOT_LOAD_ADDR __endasm;
}
