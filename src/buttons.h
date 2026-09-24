#pragma once

#include <stdint.h>

//Button setup
void buttons_setup(uint8_t pin_b1, uint8_t pin_b2, uint8_t pin_b3, uint8_t pin_enc,
                   uint8_t pin_b4, uint8_t pin_b5, uint8_t pin_b6);

// Button update task
void buttons_update();
