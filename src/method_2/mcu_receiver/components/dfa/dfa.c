#include "dfa.h"

void reset_fsa(FSAState *fsa_state, uint8_t received_buffer[BYTE_MANCHESTER_SIZE], uint8_t *index)
{
    // Reset the index to 0
    *index = 0;
    *fsa_state = STATE_START;
    memset(received_buffer, 0, BYTE_MANCHESTER_SIZE);
}

void process_dfa(FSAState *fsa_state, uint8_t received_buffer[BYTE_MANCHESTER_SIZE], uint8_t *index, uint8_t bit)
{
    static uint8_t skip = 0;
    if (skip)
    {
        skip = 0;
        return;
    }

    switch (*fsa_state)
    {
    case STATE_START:
        if (bit == 1)
        {
            *fsa_state = STATE_1;
        }
        else
        {
            *fsa_state = STATE_START;
            reset_fsa(fsa_state, received_buffer, index);
        }
        break;
    case STATE_1:
        if (bit == 0)
        {
            *fsa_state = STATE_10;
        }
        else
            reset_fsa(fsa_state, received_buffer, index);
        break;
    case STATE_10:
        if (bit == 0)
        {
            *fsa_state = STATE_100;
        }
        else
        {
            skip = 1;
            reset_fsa(fsa_state, received_buffer, index);
        }
        break;
    case STATE_100:
        if (bit == 1)
        {
            *fsa_state = STATE_1001;
        }
        else
            reset_fsa(fsa_state, received_buffer, index);
        break;
    case STATE_1001:
        if (bit == 0)
        {
            *fsa_state = STATE_10010;
        }
        else
            reset_fsa(fsa_state, received_buffer, index);
        break;
    case STATE_10010:
        if (bit == 1)
        {
            *fsa_state = STATE_100101;
        }
        else
            reset_fsa(fsa_state, received_buffer, index);
        break;
    case STATE_100101:
        if (bit == 0)
        {
            *fsa_state = STATE_1001010;
        }
        else
            reset_fsa(fsa_state, received_buffer, index);
        break;
    case STATE_1001010:
        if (bit == 1)
        {
            *fsa_state = STATE_10010101;
        }
        else
            reset_fsa(fsa_state, received_buffer, index);
        break;
    case STATE_10010101:
        if (bit == 0)
        {
            *fsa_state = STATE_100101010;
        }
        else
            reset_fsa(fsa_state, received_buffer, index);
        break;
    case STATE_100101010:
        if (bit == 1)
        {
            *fsa_state = STATE_1001010101;
        }
        else
            reset_fsa(fsa_state, received_buffer, index);
        break;
    case STATE_1001010101:
        if (bit == 0)
        {
            *fsa_state = STATE_10010101010;
        }
        else
            reset_fsa(fsa_state, received_buffer, index);
        break;
    case STATE_10010101010:
        if (bit == 1)
        {
            *fsa_state = STATE_100101010101;
        }
        else
            reset_fsa(fsa_state, received_buffer, index);
        break;
    case STATE_100101010101:
        if (bit == 0)
        {
            *fsa_state = STATE_1001010101010;
        }
        else
            reset_fsa(fsa_state, received_buffer, index);
        break;
    case STATE_1001010101010:
        if (bit == 1)
        {
            *fsa_state = STATE_10010101010101;
        }
        else
            reset_fsa(fsa_state, received_buffer, index);
        break;
    case STATE_10010101010101:
        if (bit == 1)
        {
            *fsa_state = STATE_100101010101011;
        }
        else
            reset_fsa(fsa_state, received_buffer, index);
        break;

    case STATE_100101010101011:
        if (bit == 0)
        {
            *fsa_state = STATE_END;
        }
        else
            reset_fsa(fsa_state, received_buffer, index);
        break;
    case STATE_END:
        break;
    }
}