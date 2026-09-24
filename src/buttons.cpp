#include <Arduino.h>

#include "userUsbHidKeyboardMouse/USBHIDKeyboardMouse.h"
#include "keyboard.h"
#include "util.h"
#include "buttons.h"
#include "board_config.h"

// Button pins
static uint8_t pin_btn_1_s;
static uint8_t pin_btn_2_s;
static uint8_t pin_btn_3_s;
static uint8_t pin_btn_enc_s;
static uint8_t pin_btn_4_s;
static uint8_t pin_btn_5_s;
static uint8_t pin_btn_6_s;

// Previous button states
static bool bt1ActiveState_s = false;
static bool bt2ActiveState_s = false;
static bool bt3ActiveState_s = false;
static bool btEncActiveState_s = false;
static bool bt4ActiveState_s = false;
static bool bt5ActiveState_s = false;
static bool bt6ActiveState_s = false;

// Current button states
static bool bt1Active_s  = false;
static bool bt2Active_s  = false;
static bool bt3Active_s  = false;
static bool btEncActive_s  = false;
static bool bt4Active_s  = false;
static bool bt5Active_s  = false;
static bool bt6Active_s  = false;

// Button setup
void buttons_setup(uint8_t pin_b1, uint8_t pin_b2, uint8_t pin_b3, uint8_t pin_enc,
                   uint8_t pin_b4, uint8_t pin_b5, uint8_t pin_b6)
{
    pinMode(pin_b1, INPUT_PULLUP);
    pinMode(pin_b2, INPUT_PULLUP);
    pinMode(pin_b3, INPUT_PULLUP);
    pinMode(pin_enc, INPUT_PULLUP);
#if BOARD_HAS_SIX_KEYS
    pinMode(pin_b4, INPUT_PULLUP);
    pinMode(pin_b5, INPUT_PULLUP);
    pinMode(pin_b6, INPUT_PULLUP);
#endif
    pin_btn_1_s = pin_b1;
    pin_btn_2_s = pin_b2;
    pin_btn_3_s = pin_b3;
    pin_btn_enc_s = pin_enc;
    pin_btn_4_s = pin_b4;
    pin_btn_5_s = pin_b5;
    pin_btn_6_s = pin_b6;
}

void buttons_update(void)
{
    // Read the button states, default PULL HIGH (aka LOW Activate)
    bt1Active_s  = !digitalRead(pin_btn_1_s);
    bt2Active_s  = !digitalRead(pin_btn_2_s);
    bt3Active_s  = !digitalRead(pin_btn_3_s);
    btEncActive_s  = !digitalRead(pin_btn_enc_s);
#if BOARD_HAS_SIX_KEYS
    bt4Active_s = !digitalRead(pin_btn_4_s);
    bt5Active_s = !digitalRead(pin_btn_5_s);
    bt6Active_s = !digitalRead(pin_btn_6_s);
#endif

    // Check this before any button macro can introduce a delay. This is the
    // immediate runtime bootloader combination for both board variants.
    if (btEncActive_s && bt1Active_s && bt2Active_s && bt3Active_s)
    {
        Keyboard_releaseAll();
        delay(20);
        BOOT_with_indicator();
    }

    if (bt1ActiveState_s != bt1Active_s)
    {
        bt1ActiveState_s = bt1Active_s;
        if (bt1Active_s)  
        {
            keyboard_press_button(BTN_1, BTM_PRESS);
        }
        else
        {
            keyboard_press_button(BTN_1, BTM_RELEASE);
        }
    }

    // Button 2
    if (bt2ActiveState_s != bt2Active_s)
    {
        bt2ActiveState_s = bt2Active_s;
        if (bt2Active_s)  
        {
            keyboard_press_button(BTN_2, BTM_PRESS);
        }
        else
        {
            keyboard_press_button(BTN_2, BTM_RELEASE);
        }
    }

    // Button 3
    if (bt3ActiveState_s != bt3Active_s)
    {
        bt3ActiveState_s = bt3Active_s;
        if (bt3Active_s)  
        {
            keyboard_press_button(BTN_3, BTM_PRESS);
        }
        else
        {
            keyboard_press_button(BTN_3, BTM_RELEASE);
        }
    }

    // Button Encoder
    if (btEncActiveState_s != btEncActive_s)
    {
        btEncActiveState_s = btEncActive_s;
        if (btEncActive_s)  
        {
            keyboard_press_button(BTN_ENC, BTM_PRESS);
        }
        else
        {
            keyboard_press_button(BTN_ENC, BTM_RELEASE);
        }
    }

#if BOARD_HAS_SIX_KEYS
    if (bt4ActiveState_s != bt4Active_s)
    {
        bt4ActiveState_s = bt4Active_s;
        keyboard_press_button(BTN_4, bt4Active_s ? BTM_PRESS : BTM_RELEASE);
    }
    if (bt5ActiveState_s != bt5Active_s)
    {
        bt5ActiveState_s = bt5Active_s;
        keyboard_press_button(BTN_5, bt5Active_s ? BTM_PRESS : BTM_RELEASE);
    }
    if (bt6ActiveState_s != bt6Active_s)
    {
        bt6ActiveState_s = bt6Active_s;
        keyboard_press_button(BTN_6, bt6Active_s ? BTM_PRESS : BTM_RELEASE);
    }
#endif
}
