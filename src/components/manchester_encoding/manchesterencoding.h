#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "../utils/binary-print.h"

uint8_t is_valid_manchester(uint8_t *input);
void encode_manchester_byte(uint8_t manchester_data[8], uint8_t input);
uint8_t decode_manchester_byte(uint8_t *input);

/**
 * @brief Byte to Manchester Encoding
 * Returns a 16-bit Manchester Encoded Short where 
 * 1 -> 01
 * 0 -> 10
 * 
 * @example 0x11111111 -> 0b0101010101010101
 *  
 * @param input Byte to encode
 * @return uint16_t Manchester Encoded Short
 */
uint16_t btm(uint8_t input);

/**
 * @brief Manchester Encoding to Byte
 * Returns a 16-bit Manchester Encoded Short where 
 * 1 -> 01
 * 0 -> 10
 * 
 * @example 0b0101010101010101 -> 0x11111111 
 *  
 * @param input 16 bit integer of manchester encoded 
 * @return uint8_t actual integer from
 */
uint8_t mtb(uint16_t input);