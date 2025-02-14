#pragma once
#include <driver/gpio.h>
#include "../../main/constants.h"

/**
 * @brief Setup the LED
 * 
 */
void setup_led();

void set_led(uint8_t state);

