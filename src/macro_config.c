#include <Arduino.h>
#include <stdbool.h>
#include <string.h>

#include "macro_config.h"
#include "userUsbHidKeyboardMouse/USBHIDKeyboardMouse.h"

#define MACRO_PROFILE_COUNT 5
#define MACRO_BUTTON_COUNT 6
#define MACRO_CHORD_COUNT 2
#define MACRO_SLOT_MARKER 0xA5
/* Invalidate configurations written before the final profile ordering. */
#define MACRO_FORMAT_VERSION 7
#define MACRO_SLOT_HEADER_SIZE 4
#define MACRO_RECORD_SIZE 4
#define MACRO_FIELD_COUNT 6
#define MACRO_PAYLOAD_SIZE (MACRO_PROFILE_COUNT * MACRO_BUTTON_COUNT * MACRO_RECORD_SIZE)
#define MACRO_SLOT_SIZE (MACRO_SLOT_HEADER_SIZE + MACRO_PAYLOAD_SIZE)
#define MACRO_SLOT0_ADDRESS 0 /* use DataFlash from the first byte */

#define MACRO_MOD_CTRL 0x01
#define MACRO_MOD_SHIFT 0x02
#define MACRO_MOD_ALT 0x04
#define MACRO_MOD_GUI 0x08
#define MACRO_MOD_MASK (MACRO_MOD_CTRL | MACRO_MOD_SHIFT | MACRO_MOD_ALT | MACRO_MOD_GUI)

typedef struct {
  uint8_t modifiers;
  uint8_t key;
} macro_chord_t;

typedef struct {
  uint8_t length;
  macro_chord_t chord[MACRO_CHORD_COUNT];
  uint8_t inter_chord_delay_ms;
} button_macro_t;

static const button_macro_t defaults_s[MACRO_PROFILE_COUNT][MACRO_BUTTON_COUNT] = {
    {
        {1, {{MACRO_MOD_CTRL, 'c'}, {0, 0}}, 0},
        {1, {{MACRO_MOD_CTRL, 'v'}, {0, 0}}, 0},
        {1, {{MACRO_MOD_CTRL, 'z'}, {0, 0}}, 0},
        {0, {{0, 0}, {0, 0}}, 0},
        {0, {{0, 0}, {0, 0}}, 0},
        {0, {{0, 0}, {0, 0}}, 0},
    },
    {
        {1, {{MACRO_MOD_CTRL, 'd'}, {0, 0}}, 0},
        {1, {{MACRO_MOD_CTRL, 'e'}, {0, 0}}, 0},
        {1, {{MACRO_MOD_CTRL | MACRO_MOD_ALT, 'h'}, {0, 0}}, 0},
        {0, {{0, 0}, {0, 0}}, 0},
        {0, {{0, 0}, {0, 0}}, 0},
        {0, {{0, 0}, {0, 0}}, 0},
    },
    {
        {2, {{MACRO_MOD_CTRL, 'k'}, {0, 'v'}}, 50},
        {2, {{MACRO_MOD_CTRL | MACRO_MOD_SHIFT, 'g'}, {0, 'g'}}, 30},
        {2, {{MACRO_MOD_CTRL, 'k'}, {MACRO_MOD_CTRL | MACRO_MOD_SHIFT, 'c'}}, 50},
        {1, {{MACRO_MOD_CTRL, '`'}, {0, 0}}, 0},
        {0, {{0, 0}, {0, 0}}, 0},
        {0, {{0, 0}, {0, 0}}, 0},
    },
    {
        {1, {{MACRO_MOD_CTRL | MACRO_MOD_SHIFT, 'm'}, {0, 0}}, 0},
        {1, {{MACRO_MOD_CTRL | MACRO_MOD_SHIFT, 'k'}, {0, 0}}, 0},
        {1, {{MACRO_MOD_ALT | MACRO_MOD_SHIFT, 'a'}, {0, 0}}, 0},
        {0, {{0, 0}, {0, 0}}, 0},
        {0, {{0, 0}, {0, 0}}, 0},
        {0, {{0, 0}, {0, 0}}, 0},
    },
    {
        {0, {{0, 0}, {0, 0}}, 0},
        {0, {{0, 0}, {0, 0}}, 0},
        {0, {{0, 0}, {0, 0}}, 0},
        {0, {{0, 0}, {0, 0}}, 0},
        {0, {{0, 0}, {0, 0}}, 0},
        {0, {{0, 0}, {0, 0}}, 0},
    },
};

static uint8_t active_slot_s = 0xFF;
static uint8_t active_generation_s = 0;
static uint8_t staging_slot_s = 0xFF;
static bool update_in_progress_s = false;

static uint8_t slot_address(uint8_t slot)
{
  (void)slot;
  return MACRO_SLOT0_ADDRESS;
}

static uint8_t record_address(uint8_t slot, uint8_t profile, uint8_t button)
{
  return slot_address(slot) + MACRO_SLOT_HEADER_SIZE +
         ((profile * MACRO_BUTTON_COUNT + button) * MACRO_RECORD_SIZE);
}

static uint8_t calculate_checksum(const uint8_t *data, uint8_t length)
{
  uint8_t value = 0x5A;
  uint8_t i;
  for (i = 0; i < length; ++i)
  {
    value = (uint8_t)((value << 1) | (value >> 7));
    value ^= data[i];
  }
  return value;
}

static bool valid_record(const button_macro_t *record)
{
  return record->length <= MACRO_CHORD_COUNT &&
         (record->chord[0].modifiers & ~MACRO_MOD_MASK) == 0 &&
         (record->chord[1].modifiers & ~MACRO_MOD_MASK) == 0 &&
         record->chord[0].key < 128 && record->chord[1].key < 128;
}

static void encode_record(const button_macro_t *record, uint8_t *bytes)
{
  bytes[0] = (uint8_t)((record->length & 0x03) |
                       ((record->chord[0].modifiers & 0x0F) << 2) |
                       ((record->chord[0].key & 0x03) << 6));
  bytes[1] = (uint8_t)((record->chord[0].key >> 2) & 0x1F) |
             (uint8_t)((record->chord[1].modifiers & 0x07) << 5);
  bytes[2] = (uint8_t)((record->chord[1].modifiers >> 3) & 0x01) |
             (uint8_t)((record->chord[1].key & 0x7F) << 1);
  bytes[3] = record->inter_chord_delay_ms;
}

static void decode_record(const uint8_t *bytes, button_macro_t *record)
{
  record->length = bytes[0] & 0x03;
  record->chord[0].modifiers = (bytes[0] >> 2) & 0x0F;
  record->chord[0].key = (uint8_t)(((bytes[0] >> 6) & 0x03) |
                                   ((bytes[1] & 0x1F) << 2));
  record->chord[1].modifiers = (uint8_t)((bytes[1] >> 5) |
                                         ((bytes[2] & 0x01) << 3));
  record->chord[1].key = (bytes[2] >> 1) & 0x7F;
  record->inter_chord_delay_ms = bytes[3];
}

static bool records_equal(const button_macro_t *left, const button_macro_t *right)
{
  return left->length == right->length &&
         left->chord[0].modifiers == right->chord[0].modifiers &&
         left->chord[0].key == right->chord[0].key &&
         left->chord[1].modifiers == right->chord[1].modifiers &&
         left->chord[1].key == right->chord[1].key &&
         left->inter_chord_delay_ms == right->inter_chord_delay_ms;
}

static void get_default_record(uint8_t profile, uint8_t button, button_macro_t *record)
{
  *record = defaults_s[profile][button];
}

static void read_record(uint8_t slot, uint8_t profile, uint8_t button, button_macro_t *record)
{
  uint8_t i;
  const uint8_t address = record_address(slot, profile, button);
  uint8_t bytes[MACRO_RECORD_SIZE];
  for (i = 0; i < MACRO_RECORD_SIZE; ++i)
  {
    bytes[i] = eeprom_read_byte(address + i);
  }
  decode_record(bytes, record);
}

static void write_record(uint8_t slot, uint8_t profile, uint8_t button, const button_macro_t *record)
{
  uint8_t i;
  const uint8_t address = record_address(slot, profile, button);
  uint8_t bytes[MACRO_RECORD_SIZE];
  encode_record(record, bytes);
  for (i = 0; i < MACRO_RECORD_SIZE; ++i)
  {
    eeprom_write_byte(address + i, bytes[i]);
  }
}

static uint8_t write_and_verify_staged_record(uint8_t profile, uint8_t button,
                                              const button_macro_t *record)
{
  button_macro_t verified;
  write_record(staging_slot_s, profile, button, record);
  read_record(staging_slot_s, profile, button, &verified);
  return valid_record(&verified) && records_equal(record, &verified)
             ? MACRO_CONFIG_STATUS_OK
             : MACRO_CONFIG_STATUS_VERIFY_FAILED;
}

static uint8_t payload_checksum(uint8_t slot)
{
  uint8_t bytes[MACRO_PAYLOAD_SIZE];
  uint8_t i;
  const uint8_t address = slot_address(slot) + MACRO_SLOT_HEADER_SIZE;
  for (i = 0; i < MACRO_PAYLOAD_SIZE; ++i)
  {
    bytes[i] = eeprom_read_byte(address + i);
  }
  return calculate_checksum(bytes, MACRO_PAYLOAD_SIZE);
}

static bool load_slot(uint8_t slot, uint8_t *generation)
{
  uint8_t profile;
  uint8_t button;
  button_macro_t record;
  const uint8_t address = slot_address(slot);
  if (eeprom_read_byte(address) != MACRO_SLOT_MARKER ||
      eeprom_read_byte(address + 1) != MACRO_FORMAT_VERSION)
  {
    return false;
  }

  *generation = eeprom_read_byte(address + 2);
  if (eeprom_read_byte(address + 3) != payload_checksum(slot))
  {
    return false;
  }

  for (profile = 0; profile < MACRO_PROFILE_COUNT; ++profile)
  {
    for (button = 0; button < MACRO_BUTTON_COUNT; ++button)
    {
      read_record(slot, profile, button, &record);
      if (!valid_record(&record))
      {
        return false;
      }
    }
  }
  return true;
}

static bool is_newer_generation(uint8_t candidate, uint8_t current)
{
  return (int8_t)(candidate - current) > 0;
}

static void read_active_record(uint8_t profile, uint8_t button, button_macro_t *record)
{
  if (active_slot_s == 0xFF)
  {
    get_default_record(profile, button, record);
  }
  else
  {
    read_record(active_slot_s, profile, button, record);
  }
}

static void press_modifiers(uint8_t modifiers)
{
  if (modifiers & MACRO_MOD_CTRL) {
    Keyboard_press(KEY_LEFT_CTRL);
    delay(10);
  }
  if (modifiers & MACRO_MOD_SHIFT) {
    Keyboard_press(KEY_LEFT_SHIFT);
    delay(10);
  }
  if (modifiers & MACRO_MOD_ALT) {
    Keyboard_press(KEY_LEFT_ALT);
    delay(10);
  }
  if (modifiers & MACRO_MOD_GUI) {
    Keyboard_press(KEY_LEFT_GUI);
    delay(10);
  }
}

static void release_modifiers(uint8_t modifiers)
{
  if (modifiers & MACRO_MOD_GUI) {
    Keyboard_release(KEY_LEFT_GUI);
    delay(10);
  }
  if (modifiers & MACRO_MOD_ALT) {
    Keyboard_release(KEY_LEFT_ALT);
    delay(10);
  }
  if (modifiers & MACRO_MOD_SHIFT) {
    Keyboard_release(KEY_LEFT_SHIFT);
    delay(10);
  }
  if (modifiers & MACRO_MOD_CTRL) {
    Keyboard_release(KEY_LEFT_CTRL);
    delay(10);
  }
}

void macro_config_setup(void)
{
  uint8_t generation = 0;
  if (load_slot(0, &generation))
  {
    active_slot_s = 0;
    active_generation_s = generation;
  }
  else
  {
    active_slot_s = 0xFF;
    active_generation_s = 0;
  }

  update_in_progress_s = false;
  staging_slot_s = 0;
}

void macro_config_run(uint8_t profile, uint8_t button, uint8_t mode)
{
  uint8_t i;
  button_macro_t macro;
  if (mode == 1 || profile >= MACRO_PROFILE_COUNT || button >= MACRO_BUTTON_COUNT)
  {
    return;
  }

  read_active_record(profile, button, &macro);
  for (i = 0; i < macro.length; ++i)
  {
    press_modifiers(macro.chord[i].modifiers);
    if (macro.chord[i].key != 0)
    {
      Keyboard_press(macro.chord[i].key);
      delay(20);
      Keyboard_release(macro.chord[i].key);
      delay(10);
    }
    release_modifiers(macro.chord[i].modifiers);
    if (i + 1 < macro.length)
    {
      delay(macro.inter_chord_delay_ms);
    }
  }
}

uint8_t macro_config_begin_update(void)
{
  uint8_t profile;
  uint8_t button;
  button_macro_t record;
  staging_slot_s = 0;
  eeprom_write_byte(slot_address(staging_slot_s), 0);

  for (profile = 0; profile < MACRO_PROFILE_COUNT; ++profile)
  {
    for (button = 0; button < MACRO_BUTTON_COUNT; ++button)
    {
      read_active_record(profile, button, &record);
      write_record(staging_slot_s, profile, button, &record);
    }
  }
  update_in_progress_s = true;
  return MACRO_CONFIG_STATUS_OK;
}

uint8_t macro_config_set_button(uint8_t profile, uint8_t button, uint8_t length,
                                uint8_t modifiers_1, uint8_t key_1,
                                uint8_t modifiers_2, uint8_t key_2,
                                uint8_t inter_chord_delay_ms)
{
  button_macro_t record;
  if (!update_in_progress_s)
  {
    return MACRO_CONFIG_STATUS_NOT_UPDATING;
  }
  if (profile >= MACRO_PROFILE_COUNT || button >= MACRO_BUTTON_COUNT)
  {
    return MACRO_CONFIG_STATUS_BAD_ARGUMENT;
  }

  record.length = length;
  record.chord[0].modifiers = modifiers_1;
  record.chord[0].key = key_1;
  record.chord[1].modifiers = modifiers_2;
  record.chord[1].key = key_2;
  record.inter_chord_delay_ms = inter_chord_delay_ms;
  if (!valid_record(&record))
  {
    return MACRO_CONFIG_STATUS_BAD_ARGUMENT;
  }

  return write_and_verify_staged_record(profile, button, &record);
}

uint8_t macro_config_commit(void)
{
  button_macro_t verified;
  uint8_t profile;
  uint8_t button;
  uint8_t unused_generation = 0;
  uint8_t generation;
  uint8_t payload_sum;
  uint8_t base;
  if (!update_in_progress_s)
  {
    return MACRO_CONFIG_STATUS_NOT_UPDATING;
  }

  for (profile = 0; profile < MACRO_PROFILE_COUNT; ++profile)
  {
    for (button = 0; button < MACRO_BUTTON_COUNT; ++button)
    {
      read_record(staging_slot_s, profile, button, &verified);
      if (!valid_record(&verified))
      {
        return MACRO_CONFIG_STATUS_INVALID_CONFIG;
      }
    }
  }

  base = slot_address(staging_slot_s);
  generation = (uint8_t)(active_generation_s + 1);
  payload_sum = payload_checksum(staging_slot_s);
  eeprom_write_byte(base + 1, MACRO_FORMAT_VERSION);
  eeprom_write_byte(base + 2, generation);
  eeprom_write_byte(base + 3, payload_sum);
  eeprom_write_byte(base, MACRO_SLOT_MARKER);

  if (!load_slot(staging_slot_s, &unused_generation))
  {
    eeprom_write_byte(base, 0);
    return MACRO_CONFIG_STATUS_VERIFY_FAILED;
  }

  active_slot_s = staging_slot_s;
  active_generation_s = generation;
  update_in_progress_s = false;
  staging_slot_s = 0xFF;
  return MACRO_CONFIG_STATUS_OK;
}

uint8_t macro_config_set_delay(uint8_t profile, uint8_t button,
                               uint8_t inter_chord_delay_ms)
{
  button_macro_t record;
  if (!update_in_progress_s)
  {
    return MACRO_CONFIG_STATUS_NOT_UPDATING;
  }
  if (profile >= MACRO_PROFILE_COUNT || button >= MACRO_BUTTON_COUNT)
  {
    return MACRO_CONFIG_STATUS_BAD_ARGUMENT;
  }

  read_record(staging_slot_s, profile, button, &record);
  record.inter_chord_delay_ms = inter_chord_delay_ms;
  return write_and_verify_staged_record(profile, button, &record);
}

uint8_t macro_config_abort(void)
{
  if (update_in_progress_s)
  {
    eeprom_write_byte(slot_address(staging_slot_s), 0);
  }
  update_in_progress_s = false;
  staging_slot_s = 0xFF;
  return MACRO_CONFIG_STATUS_OK;
}

uint8_t macro_config_reset_defaults(void)
{
  uint8_t profile;
  uint8_t button;
  uint8_t status = macro_config_begin_update();
  if (status != MACRO_CONFIG_STATUS_OK)
  {
    return status;
  }
  for (profile = 0; profile < MACRO_PROFILE_COUNT; ++profile)
  {
    for (button = 0; button < MACRO_BUTTON_COUNT; ++button)
    {
      button_macro_t record;
      get_default_record(profile, button, &record);
      write_record(staging_slot_s, profile, button, &record);
    }
  }
  return macro_config_commit();
}

uint8_t macro_config_get_field(uint8_t profile, uint8_t button, uint8_t field)
{
  button_macro_t record;
  if (profile >= MACRO_PROFILE_COUNT || button >= MACRO_BUTTON_COUNT || field >= MACRO_FIELD_COUNT)
  {
    return 0;
  }
  read_active_record(profile, button, &record);
  switch (field)
  {
  case 0: return record.length;
  case 1: return record.chord[0].modifiers;
  case 2: return record.chord[0].key;
  case 3: return record.chord[1].modifiers;
  case 4: return record.chord[1].key;
  default: return record.inter_chord_delay_ms;
  }
}
