#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "../utils/binary-print.h"

/**
 * @brief Validate Manchester Encoding
 * Returns 1 if the Manchester Encoding is valid, 0 otherwise. 
 * Essentially checks that there are no 3 consecutive 1s or 0s in the input buffer.
 * 
 * @param input buffer of 16 bits representing the Manchester Encoding
 * @return uint8_t 1 if valid, 0 if not
 */
uint8_t is_valid_manchester(uint8_t *input);

/**
 * @brief Byte to Manchester Encoding
 * Modifies the input buffer to contain the Manchester Encoding of the input byte. 
 * 
 * 1 -> 01
 * 
 * 0 -> 10
 * 
 * Uses LSB first encoding.
 * @param input Byte to encode
 */
void encode_manchester_byte(uint8_t manchester_data[8], uint8_t input);

/**
 * @brief Manchester Encoding to Byte
 * Returns the actual byte from the Manchester Encoded buffer.
 * If the buffer contains invalid sequences like 00 or 11, then it parse the bit as 0 regardless of what.
 * 
 * @param input 16 byte buffer of manchester encoded data 
 * @return uint8_t actual integer from manchester encoded data
 */
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