#ifndef USER_USB_RAM
#error "Require USB RAM. Go Tools > USB Setting and pick the 2nd option in the dropdown list"
#endif

//lib include
#include "src/neo/neo.h"
#include "src/userUsbHidKeyboardMouse/USBHIDKeyboardMouse.h"
#include "src/board_config.h"

//app include
#include "src/buttons.h"
#include "src/encoder.h"
#include "src/keyboard.h"
#include "src/led.h"
#include "src/util.h"

// ===================================================================================
// Main section
// ============================================================================
// Initialize pins
void setup()
{

  // Initialize neopixels
  NEO_init();
  delay(10);
  NEO_clearAll();

#if BOARD_HAS_SIX_KEYS
  // SW2/P1.5 is the six-key board's hardware recovery input. The ROM loader
  // normally sees it before this code; this check also handles a running app
  // that is reset with SW2 held.
  pinMode(PIN_BOOT_SW2, INPUT_PULLUP);
  if (!digitalRead(PIN_BOOT_SW2))
  {
    Keyboard_releaseAll();
    delay(20);
    BOOT_with_indicator();
  }
#endif

  buttons_setup(PIN_BTN_1, PIN_BTN_2, PIN_BTN_3, PIN_BTN_ENC);
  keyboard_setup();
  encoder_setup(ENCODER_A, ENCODER_B);
  led_set_mode(LED_LOOP);
  USBInit();
  Keyboard_releaseAll();
}


//Main loop, read buttons
void loop()
{
  if (USBHID_bootloader_requested())
  {
    BOOT_with_indicator();
  }

  //task update
  buttons_update();
  keyboard_update();
  encoder_update();
  led_update();

  //debouncing
  delay(1); 
}
