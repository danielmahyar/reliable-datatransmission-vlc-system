#pragma once
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include "../../main/constants.h"

typedef enum
{
    STATE_START,
    STATE_1,
    STATE_10,
    STATE_100,
    STATE_1001,
    STATE_10010,
    STATE_100101,
    STATE_1001010,
    STATE_10010101,
    STATE_100101010,
    STATE_1001010101,
    STATE_10010101010,
    STATE_100101010101,
    STATE_1001010101010,
    STATE_10010101010101,
    STATE_100101010101011,
    STATE_END
} FSAState;

void reset_fsa(FSAState *fsa_state, uint8_t received_buffer[BYTE_MANCHESTER_SIZE], uint8_t *index);
void process_dfa(FSAState *fsa_state, uint8_t received_buffer[BYTE_MANCHESTER_SIZE], uint8_t *index, uint8_t bit);