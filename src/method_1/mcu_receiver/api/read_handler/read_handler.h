#pragma once
#include <inttypes.h>
#include "../../main/constants.h"
#include "../../hal/adc/adc.h"

/**
 * @brief Setup the read handler
 * 
 */
void setup_read_handler();


/**
 * @brief Read if there is currently light or not
 * 
 * @return uint8_t bit - 1 if the environment is dark, 0 if the environment is bright
 * 
 */
uint8_t read_env_bit();