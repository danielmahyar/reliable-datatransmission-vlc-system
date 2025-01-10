#include <stdio.h>
#include <inttypes.h>
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c \n"
#define BYTE_TO_BINARY(byte)         \
    ((byte) & 0x80 ? '1' : '0'),     \
        ((byte) & 0x40 ? '1' : '0'), \
        ((byte) & 0x20 ? '1' : '0'), \
        ((byte) & 0x10 ? '1' : '0'), \
        ((byte) & 0x08 ? '1' : '0'), \
        ((byte) & 0x04 ? '1' : '0'), \
        ((byte) & 0x02 ? '1' : '0'), \
        ((byte) & 0x01 ? '1' : '0')

/*

    Manchester Encoding
    1 -> 01
    0 -> 10

    Thinking we should send Little Endian Byte ordering

*/

void encode_manchester_byte(uint8_t *manchester_data, uint8_t input)
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

int main()
{

    uint8_t manchester_data[16] = {0};
    // uint8_t manchester_data2[64] = {0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0};
    uint8_t manchester_data2[64] = {0};

    encode_manchester_byte(manchester_data, 12);
    uint8_t result1 = decode_manchester_byte(manchester_data);
    printf("Result: %d\n", result1);

    encode_manchester_word(manchester_data2, 0xFF);
    uint32_t result2 = (uint32_t)decode_manchester_word(manchester_data2);
    printf("Result: %u\n", (uint32_t)result2);

    return 0;
}
