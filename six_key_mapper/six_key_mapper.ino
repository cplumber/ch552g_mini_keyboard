#ifndef USER_USB_RAM
#error "Require USB RAM. Select USER CODE w/148B USB RAM."
#endif

#include "src/userUsbHidKeyboardMouse/USBHIDKeyboardMouse.h"
#include "src/util.h"
#include "mapper.h"
#include "mapper_led.h"

// This sketch is intentionally a separate, temporary hardware-discovery image.
// It must be built with bootloader_pin=p15 so the board's SW2/P1.5 recovery
// route is retained.
void setup()
{
  mapper_setup_inputs();
  mapper_led_setup();

  // Every application start gets one ROM bootloader window.  The ROM's own
  // timeout is hardware-defined (normally about 10 seconds); this sketch does
  // not pretend it can extend that timeout to an exact 15 seconds.
  if (mapper_should_enter_bootloader())
  {
    delay(10);
    BOOT_now();
    // Reaching here means the ROM loader returned without restarting us.
    mapper_clear_bootloader_marker();
  }

  // A held candidate switch is a convenient second recovery route.  SW2/P1.5
  // remains the primary ROM-level route and is checked before this sketch runs.
  delay(30);
  if (mapper_recovery_requested())
  {
    BOOT_now();
  }

  USBInit();
  Keyboard_releaseAll();
}

void loop()
{
  mapper_update();
  mapper_led_update();
  delay(1);
}
