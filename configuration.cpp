#include "configuration.h"
#include "src/userUsbHidKeyboardMouse/USBHIDKeyboardMouse.h"

const keyboard_configuration_t configurations[NUM_CONFIGURATION] = {
    {
        .button = { // Configuration copy paste keyboard
            [BTN_1] = {
                .type = BUTTON_NULL,
            },
            [BTN_2] = {
                .type = BUTTON_NULL,
            },
            [BTN_3] = {
                .type = BUTTON_NULL,
            },
            [ENC_CW] = { // volume up
                .type = BUTTON_FUNCTION,
                .functionPointer = keyboard_volume_up,
            },
            [ENC_CCW] = {   // volume down
                .type = BUTTON_FUNCTION,
                .functionPointer = keyboard_volume_down,
            },
            [BTN_ENC] = {
                .type = BUTTON_FUNCTION,
                .functionPointer = keyboard_press_enc,
            },
        }
    },
    {   // Configuration google meet
        .button = { // 
            [BTN_1] = {
                .type = BUTTON_NULL,
            },
            [BTN_2] = {
                .type = BUTTON_NULL,
            },
            [BTN_3] = {
                .type = BUTTON_NULL,
            },
            [ENC_CW] = {
                .type = BUTTON_FUNCTION,
                .functionPointer = keyboard_volume_up,
            },
            [ENC_CCW] = {
                .type = BUTTON_FUNCTION,
                .functionPointer = keyboard_volume_down,
            },
            [BTN_ENC] = {
                .type = BUTTON_FUNCTION,
                .functionPointer = keyboard_press_enc,
            },
        }
    },
    {   // Configuration VS Code
        .button = {
            [BTN_1] = {
                .type = BUTTON_NULL,
            },
            [BTN_2] = {
                .type = BUTTON_NULL,
            },
            [BTN_3] = {
                .type = BUTTON_NULL,
            },
            [ENC_CW] = {
                .type = BUTTON_FUNCTION,
                .functionPointer = keyboard_vscode_next_window,
            },
            [ENC_CCW] = {
                .type = BUTTON_FUNCTION,
                .functionPointer = keyboard_vscode_prev_window,
            },
            [BTN_ENC] = {
                .type = BUTTON_FUNCTION,
                .functionPointer = keyboard_press_enc,
            },
        }
    },
    {   // Configuration Microsoft Teams (web)
        .button = {
            [BTN_1] = {.type = BUTTON_NULL},
            [BTN_2] = {.type = BUTTON_NULL},
            [BTN_3] = {.type = BUTTON_NULL},
            [ENC_CW] = {.type = BUTTON_FUNCTION, .functionPointer = keyboard_volume_up},
            [ENC_CCW] = {.type = BUTTON_FUNCTION, .functionPointer = keyboard_volume_down},
            [BTN_ENC] = {.type = BUTTON_FUNCTION, .functionPointer = keyboard_press_enc},
        }
    },
    /*
    {   // Reserved slot
        .button = {
            [BTN_1] = {
                .type = BUTTON_NULL,
            },
            [BTN_2] = {
                .type = BUTTON_NULL,
            },
            [BTN_3] = {
                .type = BUTTON_NULL,
            },
            [ENC_CW] = {
                .type = BUTTON_FUNCTION,
                .functionPointer = keyboard_volume_up,
            },
            [ENC_CCW] = {
                .type = BUTTON_FUNCTION,
                .functionPointer = keyboard_volume_down,
            },
            [BTN_ENC] = {
                .type = BUTTON_FUNCTION,
                .functionPointer = keyboard_press_enc,
            },
        }
    },
    */
    {   //Menu configuration
        .button = { // Configurtion copy paste keyboard
            [BTN_1] = {
                .type = BUTTON_NULL,
            },
            [BTN_2] = {
                .type = BUTTON_NULL,
            },
            [BTN_3] = {
                .type = BUTTON_NULL,
            },
            [ENC_CW] = {
                .type = BUTTON_FUNCTION,
                .functionPointer = button_menu_up,
            },
            [ENC_CCW] = {
                .type = BUTTON_FUNCTION,
                .functionPointer = button_menu_down,
            },
            [BTN_ENC] = {
                .type = BUTTON_FUNCTION,
                .functionPointer = keyboard_press_enc,
            },
        }
    },
};
