#pragma once

#include <stdint.h>

#define NUM_CONFIGURATION 6
typedef enum 
{
  BTN_1,
  BTN_2,
  BTN_3,
  BTN_4,
  BTN_5,
  BTN_6,
  ENC_CW,
  ENC_CCW,
  BTN_ENC,
  BTN_NUM,
}keyboard_button_t;

typedef enum 
{
  BTM_PRESS,
  BTM_RELEASE,
  BTM_CLICK,
}keyboard_button_keyboard_mode_t;


typedef enum 
{
  BUTTON_FUNCTION,
  BUTTON_NULL,
}keyboard_button_type;

typedef struct 
{
  keyboard_button_type type;
  void (*functionPointer)(keyboard_button_keyboard_mode_t mode);
}button_function_t;

typedef struct 
{
   button_function_t  button[BTN_NUM];
}keyboard_configuration_t;



// keyboard_press_button is called
void keyboard_press_button( keyboard_button_t button,  keyboard_button_keyboard_mode_t mode);

// keyboard setup 
void keyboard_setup(void);

// keyboard periodic update
void keyboard_update(void);

// keyboard encoder press to enter in menu
void keyboard_press_enc(keyboard_button_keyboard_mode_t mode);

// keyboard encoder rotation helpers
void keyboard_volume_up(keyboard_button_keyboard_mode_t mode);
void keyboard_volume_down(keyboard_button_keyboard_mode_t mode);

// keyboard menu scroll uo
void button_menu_up(keyboard_button_keyboard_mode_t mode);

// keyboard menu scroll down
void button_menu_down(keyboard_button_keyboard_mode_t mode);

// VS Code window navigation
void keyboard_vscode_next_window(keyboard_button_keyboard_mode_t mode);
void keyboard_vscode_prev_window(keyboard_button_keyboard_mode_t mode);

