#include <Arduino.h>
#include "userUsbHidKeyboardMouse/USBHIDKeyboardMouse.h"
#include "led.h"
#include "keyboard.h"
#include "macro_config.h"
#include "../configuration.h"

#define MENU_CONF NUM_CONFIGURATION - 1
#define MENU_SELECTION_LAST 3
#define ENC_LONG_PRESS_MS 1000UL
#define MENU_MODE_STORAGE_ADDR 0

static int current_mode_s = 0;                                       // current mode of keyboard
static int menu_mode_s = 0;                                          // during menu coice
static unsigned long enc_press_start_ms_s = 0;
static bool enc_long_press_active_s = false;
static bool enc_pressed_s = false;
static bool vscode_alt_held_s = false;
static unsigned long vscode_alt_release_ms_s = 0;
static const unsigned long VSCODE_ALT_HOLD_MS = 1000UL;


static void set_menu_led(void);
static void enter_menu(void);
static void save_menu_mode(void);
static uint8_t load_menu_mode(void);
static void keyboard_press_mode_1(keyboard_button_t button, keyboard_button_keyboard_mode_t mode);
static void keyboard_press_mode_2(keyboard_button_t button, keyboard_button_keyboard_mode_t mode);
static void keyboard_press_auto(keyboard_button_t button, keyboard_button_keyboard_mode_t mode);
static void keyboard_press_menu(keyboard_button_t button, keyboard_button_keyboard_mode_t mode);

// ===================================================================================
// Menu section
// ===================================================================================

static void set_menu_led(void)
{
  led_set_mode(LED_MENU);
  led_set_menu_profile((uint8_t)menu_mode_s);
}

static uint8_t load_menu_mode(void)
{
  uint8_t stored_mode = eeprom_read_byte(MENU_MODE_STORAGE_ADDR);
  if (stored_mode > MENU_SELECTION_LAST)
  {
    return 0;
  }
  return stored_mode;
}

static void save_menu_mode(void)
{
  if (menu_mode_s > MENU_SELECTION_LAST)
  {
    menu_mode_s = MENU_SELECTION_LAST;
  }

  eeprom_write_byte(MENU_MODE_STORAGE_ADDR, (uint8_t)menu_mode_s);
}

void keyboard_press_enc(keyboard_button_keyboard_mode_t mode)
{
  if (mode == BTM_PRESS)
  {
    enc_pressed_s = true;
    enc_long_press_active_s = false;
    enc_press_start_ms_s = millis();
  }
  if (mode == BTM_RELEASE)
  {
    if (enc_long_press_active_s)
    {
      current_mode_s = menu_mode_s;
      Keyboard_releaseAll();
      vscode_alt_held_s = false;
      save_menu_mode();
      led_set_mode(LED_LOOP);
    }
    else if (current_mode_s != MENU_CONF)
    {
      // Global short-click action: the Windows helper listens for F24.
      led_set_mic_mute_state(!led_get_mic_mute_state());
      Keyboard_write(KEY_F24);
    }
    enc_pressed_s = false;
  }
}

void keyboard_volume_up(keyboard_button_keyboard_mode_t mode)
{
  if (mode != BTM_RELEASE)
  {
    Consumer_click(CONSUMER_VOLUME_UP);
  }
}

void keyboard_volume_down(keyboard_button_keyboard_mode_t mode)
{
  if (mode != BTM_RELEASE)
  {
    Consumer_click(CONSUMER_VOLUME_DOWN);
  }
}

static void keyboard_vscode_switch_window(bool reverse)
{
  if (!vscode_alt_held_s)
  {
    Keyboard_press(KEY_LEFT_ALT);
    vscode_alt_held_s = true;
  }

  if (reverse)
  {
    Keyboard_press(KEY_LEFT_SHIFT);
  }

  Keyboard_write(KEY_TAB);

  if (reverse)
  {
    Keyboard_release(KEY_LEFT_SHIFT);
  }
}

void keyboard_vscode_next_window(keyboard_button_keyboard_mode_t mode)
{
  if (mode == BTM_RELEASE)
  {
    return;
  }

  vscode_alt_release_ms_s = millis() + VSCODE_ALT_HOLD_MS;
  keyboard_vscode_switch_window(false);
}

void keyboard_vscode_prev_window(keyboard_button_keyboard_mode_t mode)
{
  if (mode == BTM_RELEASE)
  {
    return;
  }

  vscode_alt_release_ms_s = millis() + VSCODE_ALT_HOLD_MS;
  keyboard_vscode_switch_window(true);
}

void keyboard_update(void)
{
  if (enc_pressed_s && !enc_long_press_active_s)
  {
    if ((millis() - enc_press_start_ms_s) >= ENC_LONG_PRESS_MS)
    {
      menu_mode_s = current_mode_s;
      current_mode_s = MENU_CONF;
      set_menu_led();
      enc_long_press_active_s = true;
    }
  }

  if (vscode_alt_held_s && (long)(millis() - vscode_alt_release_ms_s) >= 0)
  {
    Keyboard_release(KEY_LEFT_ALT);
    vscode_alt_held_s = false;
  }
}

void button_menu_up(keyboard_button_keyboard_mode_t mode)
{
  if (mode == BTM_CLICK)
  {
    if (menu_mode_s >= MENU_SELECTION_LAST)
    {
      menu_mode_s = MENU_SELECTION_LAST;
    }
    else
    {
      menu_mode_s++;
    }
    set_menu_led();
  }
}

void button_menu_down(keyboard_button_keyboard_mode_t mode)
{
  if (mode == BTM_CLICK)
  {
    if (menu_mode_s <= 0)
    {
      menu_mode_s = 0;
    }
    else
    {
      menu_mode_s--;
    }
    set_menu_led();
  }
}

void keyboard_press_button(keyboard_button_t button, keyboard_button_keyboard_mode_t mode)
{
  if (button >= BTN_1 && button <= BTN_3)
  {
    if (mode == BTM_PRESS)
    {
      led_presskey(button);
    }
    else
    {
      led_presskey(-1);
    }
  }

  if (button >= BTN_1 && button <= BTN_3 && current_mode_s != MENU_CONF)
  {
    macro_config_run((uint8_t)current_mode_s, (uint8_t)button, (uint8_t)mode);
    return;
  }

  switch (configurations[current_mode_s].button[button].type)
  {
  case BUTTON_FUNCTION:
    configurations[current_mode_s].button[button].functionPointer(mode);
    break;
  case BUTTON_NULL:
    break;
  default:
    break;
  }
}

void keyboard_setup()
{
  macro_config_setup();
  current_mode_s = load_menu_mode();
  menu_mode_s = current_mode_s;
  led_set_menu_profile((uint8_t)current_mode_s);
}
