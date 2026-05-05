#include <Arduino.h>
#include "userUsbHidKeyboardMouse/USBHIDKeyboardMouse.h"
#include "auto_mode.h"
#include "led.h"
#include "keyboard.h"
#include "../configuration.h"

#define MENU_CONF NUM_CONFIGURATION - 1
#define MENU_SELECTION_LAST 2
#define ENC_LONG_PRESS_MS 1000UL

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
static void keyboard_press_mode_1(keyboard_button_t button, keyboard_button_keyboard_mode_t mode);
static void keyboard_press_mode_2(keyboard_button_t button, keyboard_button_keyboard_mode_t mode);
static void keyboard_press_auto(keyboard_button_t button, keyboard_button_keyboard_mode_t mode);
static void keyboard_press_menu(keyboard_button_t button, keyboard_button_keyboard_mode_t mode);

// ===================================================================================
// create keyboard configuration
// ===================================================================================

const button_function_t button_function_null = {
    .type = BUTTON_NULL};

// ===================================================================================
// Menu section
// ===================================================================================

static void set_menu_led(void)
{
  int led_index = menu_mode_s % 3;
  int color_background = NEO_CYAN + (menu_mode_s / 3) * 32;

  led_set_mode(LED_FIX);
  led_set_color_hue((led_index == 0) ? NEO_RED : color_background,
                    (led_index == 1) ? NEO_RED : color_background,
                    (led_index == 2) ? NEO_RED : color_background);
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

void keyboard_meet_raise_hand(keyboard_button_keyboard_mode_t mode)
{
  if (mode == BTM_RELEASE)
  {
    return;
  }

  Keyboard_press(KEY_LEFT_CTRL);
  delay(10);
  Keyboard_press(KEY_LEFT_ALT);
  delay(10);
  Keyboard_press('h');
  delay(20);
  Keyboard_release('h');
  delay(10);
  Keyboard_release(KEY_LEFT_ALT);
  delay(10);
  Keyboard_release(KEY_LEFT_CTRL);
}

void keyboard_vscode_toggle_terminal(keyboard_button_keyboard_mode_t mode)
{
  if (mode == BTM_RELEASE)
  {
    return;
  }

  Keyboard_press(KEY_LEFT_CTRL);
  delay(10);
  Keyboard_press('`');
  delay(20);
  Keyboard_release('`');
  delay(10);
  Keyboard_release(KEY_LEFT_CTRL);
}

void keyboard_vscode_source_control(keyboard_button_keyboard_mode_t mode)
{
  if (mode == BTM_RELEASE)
  {
    return;
  }

  Keyboard_press(KEY_LEFT_CTRL);
  delay(10);
  Keyboard_press(KEY_LEFT_SHIFT);
  delay(10);
  Keyboard_press('g');
  delay(20);
  Keyboard_release('g');
  delay(10);
  Keyboard_release(KEY_LEFT_SHIFT);
  Keyboard_release(KEY_LEFT_CTRL);

  delay(30);

  Keyboard_press('g');
  delay(20);
  Keyboard_release('g');
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
      auto_set_cycle(button_function_null);
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

static void keyboard_run_key_sequence(button_sequence_t sequence, keyboard_button_keyboard_mode_t mode)
{
  if (mode == BTM_RELEASE)
  {
    return;
  }

  for (uint8_t i = 0; i < sequence.length; i++)
  {
    Keyboard_press(sequence.sequence[i]);
    delay(10);
    if (sequence.delay > 0)
    {
      Keyboard_release(sequence.sequence[i]);
      delay(sequence.delay);
    }
  }
  Keyboard_releaseAll();
}

static void keyboard_run_mouse_sequence(button_mouse_t sequence, keyboard_button_keyboard_mode_t mode)
{
  if (mode == BTM_RELEASE)
  {
    return;
  }
  if (sequence.keypress > 0)
  {
    Keyboard_press(sequence.keypress);
    delay(30);
  }
  for (uint8_t i = 0; i < sequence.length; i++)
  {
    switch (sequence.mouse_event_sequence[i].type)
    {
    case UP:
      Mouse_move(0, -sequence.mouse_event_sequence[i].value);
      break;
    case DOWN:
      Mouse_move(0, sequence.mouse_event_sequence[i].value);
      break;
    case LEFT:
      Mouse_move(-sequence.mouse_event_sequence[i].value, 0);
      break;
    case RIGH:
      Mouse_move(sequence.mouse_event_sequence[i].value, 0);
      break;
    case LEFT_CLICK:
      Mouse_click(MOUSE_LEFT);
      break;
    case RIGHT_CLICK:
      Mouse_click(MOUSE_RIGHT);
      break;
    case SCROLL_UP:
      Mouse_scroll(sequence.mouse_event_sequence[i].value);
      break;
    case SCROLL_DOWN:
      Mouse_scroll(-sequence.mouse_event_sequence[i].value);
      break;
    default:
      break;
    }
    if (sequence.keypress > 0)
    {
      Keyboard_releaseAll();
    }
    delay(sequence.delay);
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

  switch (configurations[current_mode_s].button[button].type)
  {
  case BUTTON_SEQUENCE:
    keyboard_run_key_sequence(configurations[current_mode_s].button[button].function.sequence, mode);
    break;
  case BUTTON_MOUSE:
    keyboard_run_mouse_sequence(configurations[current_mode_s].button[button].function.mouse, mode);
    break;
  case BUTTON_AUTO_KEYBOARD:
    if (mode == BTM_PRESS)
    {
      auto_set_cycle(configurations[current_mode_s].button[button]);
    }
    break;
  case BUTTON_AUTO_MOUSE:
    if (mode == BTM_PRESS)
    {
      auto_set_cycle(configurations[current_mode_s].button[button]);
    }
    break;
  case BUTTON_FUNCTION:
    configurations[current_mode_s].button[button].function.functionPointer(mode);
    break;
  case BUTTON_NULL:
    if (mode == BTM_PRESS)
    {
      auto_set_cycle(configurations[current_mode_s].button[button]);
    }
    break;
  default:
    break;
  }
}

void keyboard_setup()
{

}
