#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  MACRO_CONFIG_CMD_GET_BUTTON = 1,
  MACRO_CONFIG_CMD_BEGIN_UPDATE = 2,
  MACRO_CONFIG_CMD_SET_BUTTON = 3,
  MACRO_CONFIG_CMD_COMMIT = 4,
  MACRO_CONFIG_CMD_ABORT = 5,
  MACRO_CONFIG_CMD_RESET_DEFAULTS = 6,
  MACRO_CONFIG_CMD_ENTER_BOOTLOADER = 7,
  MACRO_CONFIG_CMD_SET_DELAY = 8,
  MACRO_CONFIG_CMD_GET_BOARD_ID = 9,
};

enum {
  MACRO_CONFIG_STATUS_OK = 0,
  MACRO_CONFIG_STATUS_BAD_COMMAND = 1,
  MACRO_CONFIG_STATUS_BAD_ARGUMENT = 2,
  MACRO_CONFIG_STATUS_NOT_UPDATING = 3,
  MACRO_CONFIG_STATUS_INVALID_CONFIG = 4,
  MACRO_CONFIG_STATUS_VERIFY_FAILED = 5,
};

void macro_config_setup(void);
void macro_config_run(uint8_t profile, uint8_t button, uint8_t mode);
uint8_t macro_config_begin_update(void);
uint8_t macro_config_set_button(uint8_t profile, uint8_t button, uint8_t length,
                                uint8_t modifiers_1, uint8_t key_1,
                                uint8_t modifiers_2, uint8_t key_2,
                                uint8_t inter_chord_delay_ms);
uint8_t macro_config_set_delay(uint8_t profile, uint8_t button,
                               uint8_t inter_chord_delay_ms);
uint8_t macro_config_commit(void);
uint8_t macro_config_abort(void);
uint8_t macro_config_reset_defaults(void);
uint8_t macro_config_get_field(uint8_t profile, uint8_t button, uint8_t field);
uint8_t macro_config_generation(void);

#ifdef __cplusplus
}
#endif
