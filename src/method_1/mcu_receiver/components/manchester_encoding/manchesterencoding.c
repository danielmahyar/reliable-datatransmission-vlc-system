#include "./manchesterencoding.h"

/*
    Manchester Encoding
    1 -> 01
    0 -> 10
*/

int main()
{
    uint8_t byte = 0xFF;
    const uint16_t res = btm(byte);

    PRINT_SHORT_BINARY(res);
    printf("\n");
    const uint8_t res2 = mtb(res);
    PRINT_BYTE_BINARY(res2);

    // uint8_t manchester_data[16] = {0};
    // // uint8_t manchester_data2[64] = {0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0};
    // uint8_t manchester_data2[64] = {0};

    // TIME_FUNCTION(encode_manchester_byte(manchester_data, 12));
    // uint8_t result1 = decode_manchester_byte(manchester_data);
    // printf("Result: %d\n", result1);

    // encode_manchester_word(manchester_data2, 0xFF);
    // uint32_t result2 = (uint32_t)decode_manchester_word(manchester_data2);
    // printf("Result: %u\n", (uint32_t)result2);

    return 0;
}

// Loop through and check that there is no consective 1s or 0s 3 in a row
uint8_t is_valid_manchester(uint8_t *input)
{
    uint8_t last_bit = input[0];
    uint8_t count = 1;
    for (int i = 1; i < 16; i++)
    {
        if (input[i] == last_bit)
        {
            count++;
        }
        else
        {
            count = 1;
        }
        if (count == 3)
        {
            return 0;
        }
        last_bit = input[i];
    }
    return 1;
}

uint16_t btm(uint8_t input)
{
    uint16_t result = 0;
    for (int i = 0; i < 8; i++)
    {
        uint8_t bit = (input >> i) & 0x01;
        if (bit == 1)
        {
            result |= (0x01 << (i * 2));
        }
        else
        {
            result |= (0x02 << (i * 2));
        }
    }
    return result;
}

uint8_t mtb(uint16_t input) {
    uint8_t result = 0;
    for(uint8_t i = 0; i < 16; i+=2) {
        uint8_t bits = (input >> i) & 0x03;
        if(bits == 0x01) {
            result |= (0x01 << (i >> 1));
        } else if(bits == 0x02) {
            result &= ~(0x01 << (i >> 1));
        }
    }
    return result;
}


void encode_manchester_byte(uint8_t manchester_data[8], uint8_t input)
{
    for (int i = 0; i < 8; i++)
    {
        uint8_t bit = (input >> i) & 0x01;
        if (bit == 1)
        {
            manchester_data[i * 2] = 0;
            manchester_data[i * 2 + 1] = 1;
        }
        else
        {
            manchester_data[i * 2] = 1;
            manchester_data[i * 2 + 1] = 0;
        }
    }
}

void encode_manchester_word(uint8_t *manchester_data, uint32_t input)
{
    // Little Endian encoding
    for (int i = 0; i < 4; i++)
    {
        encode_manchester_byte(manchester_data + i * 8, (uint8_t)((input >> (i * 8)) & 0xFF));
    }
}

uint8_t decode_manchester_byte(uint8_t *input)
{
    uint8_t result = 0;
    for (int i = 0; i < 16; i += 2)
    {
        uint8_t bit1 = input[i];
        uint8_t bit2 = input[i + 1];
        if (bit1 == 0 && bit2 == 1)
        {
            result = result | ((uint8_t)1 << (i >> 1));
        }
        else if (bit1 == 1 && bit2 == 0)
        {
            result = result & ~((uint8_t)1 << (i >> 1));
        }
        // printf("Result so far: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(result));
    }
    return result;
}

uint32_t decode_manchester_word(uint8_t *input)
{
    // Use the decode_manchester_byte function to decode the word
    uint32_t result = 0;
    for (int i = 0; i < 4; i++)
    {
        uint32_t byte = (uint32_t)decode_manchester_byte(input + i * 8);
        result |= (uint32_t)byte << (i * 8);
    }
    return result;
}



