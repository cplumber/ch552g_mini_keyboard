#include "configuration.h"
#include "src/userUsbHidKeyboardMouse/USBHIDKeyboardMouse.h"

const keyboard_configuration_t configurations[NUM_CONFIGURATION] = {
    {
        .button = { // Configuration copy paste keyboard
            [BTN_1] = {
                .type = BUTTON_SEQUENCE,
                .function.sequence = {
                    .sequence = {KEY_LEFT_CTRL, 'c'}, // ctrl + c
                    .length = 2,            // Lengh of sequence
                    .delay = 0             // no delay
                }
            },
            [BTN_2] = {
                .type = BUTTON_SEQUENCE,
                .function.sequence = {
                    .sequence = {KEY_LEFT_CTRL, 'v'}, // ctrl + v
                    .length = 2,            // Lengh of sequence
                    .delay = 0             // no delay
                }
            },
            [BTN_3] = {
                .type = BUTTON_SEQUENCE,
                .function.sequence = {
                    .sequence = {KEY_LEFT_CTRL, 'z'}, // ctrl + z
                    .length = 2,            // Lengh of sequence
                    .delay = 0             // no delay
                }
            },
            [ENC_CW] = { // volume up
                .type = BUTTON_FUNCTION,
                .function.functionPointer = keyboard_volume_up,
            },
            [ENC_CCW] = {   // volume down
                .type = BUTTON_FUNCTION,
                .function.functionPointer = keyboard_volume_down,
            },
            [BTN_ENC] = {
                .type = BUTTON_FUNCTION,
                .function.functionPointer = keyboard_press_enc,
            },
        }
    },
    {   // Configuration google meet
        .button = { // 
            [BTN_1] = {
                .type = BUTTON_SEQUENCE,
                .function.sequence = {
                    .sequence = {KEY_LEFT_CTRL, 'd'}, // mute / unmute
                    .length = 2,            // Lengh of sequence
                    .delay = 0             // no delay
                }
            },
            [BTN_2] = {
                .type = BUTTON_SEQUENCE,
                .function.sequence = {
                    .sequence = {KEY_LEFT_CTRL, 'e'}, // camera on / off
                    .length = 2,            // Lengh of sequence
                    .delay = 0             // no delay
                }
            },
            [BTN_3] = {
                .type = BUTTON_FUNCTION,
                .function.functionPointer = keyboard_meet_raise_hand,
            },
            [ENC_CW] = {
                .type = BUTTON_FUNCTION,
                .function.functionPointer = keyboard_volume_up,
            },
            [ENC_CCW] = {
                .type = BUTTON_FUNCTION,
                .function.functionPointer = keyboard_volume_down,
            },
            [BTN_ENC] = {
                .type = BUTTON_FUNCTION,
                .function.functionPointer = keyboard_press_enc,
            },
        }
    },
    {   // Configuration VS Code
        .button = {
            [BTN_1] = {
                .type = BUTTON_FUNCTION,
                .function.functionPointer = keyboard_vscode_preview,
            },
            [BTN_2] = {
                .type = BUTTON_FUNCTION,
                .function.functionPointer = keyboard_vscode_source_control,
            },
            [BTN_3] = {
                .type = BUTTON_FUNCTION,
                .function.functionPointer = keyboard_vscode_toggle_terminal,
            },
            [ENC_CW] = {
                .type = BUTTON_FUNCTION,
                .function.functionPointer = keyboard_vscode_next_window,
            },
            [ENC_CCW] = {
                .type = BUTTON_FUNCTION,
                .function.functionPointer = keyboard_vscode_prev_window,
            },
            [BTN_ENC] = {
                .type = BUTTON_FUNCTION,
                .function.functionPointer = keyboard_press_enc,
            },
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
                .function.functionPointer = keyboard_volume_up,
            },
            [ENC_CCW] = {
                .type = BUTTON_FUNCTION,
                .function.functionPointer = keyboard_volume_down,
            },
            [BTN_ENC] = {
                .type = BUTTON_FUNCTION,
                .function.functionPointer = keyboard_press_enc,
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
                .function.functionPointer = button_menu_up,
            },
            [ENC_CCW] = {
                .type = BUTTON_FUNCTION,
                .function.functionPointer = button_menu_down,
            },
            [BTN_ENC] = {
                .type = BUTTON_FUNCTION,
                .function.functionPointer = keyboard_press_enc,
            },
        }
    },
};
