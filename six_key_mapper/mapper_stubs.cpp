#include <stdint.h>

#include "src/led.h"
#include "src/macro_config.h"

// The shared HID transport accepts the normal configuration reports.  The
// mapper deliberately makes them inert: it must never alter DataFlash or run
// an old three-key macro while it is identifying the six-key wiring.
void macro_config_setup(void) {}
void macro_config_run(uint8_t profile, uint8_t button, uint8_t mode)
{
  (void)profile;
  (void)button;
  (void)mode;
}
uint8_t macro_config_begin_update(void) { return MACRO_CONFIG_STATUS_BAD_COMMAND; }
uint8_t macro_config_set_button(uint8_t profile, uint8_t button, uint8_t length,
                                uint8_t modifiers1, uint8_t key1,
                                uint8_t modifiers2, uint8_t key2,
                                uint8_t inter_chord_delay_ms)
{
  (void)profile; (void)button; (void)length; (void)modifiers1;
  (void)key1; (void)modifiers2; (void)key2; (void)inter_chord_delay_ms;
  return MACRO_CONFIG_STATUS_BAD_COMMAND;
}
uint8_t macro_config_set_delay(uint8_t profile, uint8_t button, uint8_t delay_ms)
{
  (void)profile; (void)button; (void)delay_ms;
  return MACRO_CONFIG_STATUS_BAD_COMMAND;
}
uint8_t macro_config_commit(void) { return MACRO_CONFIG_STATUS_BAD_COMMAND; }
uint8_t macro_config_abort(void) { return MACRO_CONFIG_STATUS_OK; }
uint8_t macro_config_reset_defaults(void) { return MACRO_CONFIG_STATUS_BAD_COMMAND; }
uint8_t macro_config_get_field(uint8_t profile, uint8_t button, uint8_t field)
{
  (void)profile; (void)button; (void)field;
  return 0;
}
uint8_t macro_config_generation(void) { return 0; }

// USB suspend/mic reports are ignored in the mapper.  Keeping these symbols
// local avoids linking the normal LED renderer and its hardware assumptions.
void led_set_mic_mute_state(uint8_t muted) { (void)muted; }
void led_set_menu_profile(uint8_t profile) { (void)profile; }
void led_set_usb_suspended(uint8_t suspended) { (void)suspended; }
uint8_t led_get_mic_mute_state(void) { return 0; }
