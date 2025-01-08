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

uint8_t decode_manchester_byte(uint8_t *input)
{
    uint8_t result = 0;
    for (int i = 0; i < 16; i += 2)
    {
        uint8_t bit1 = input[i];
        uint8_t bit2 = input[i + 1];
        if (bit1 == 0 && bit2 == 1)
        {
            // Now we know we recieved a 1
            // Place a 1 in the ith bit position in result
            result = result | ((uint8_t)1 << (i >> 1));
            // printf("1");
        }
        else if (bit1 == 1 && bit2 == 0)
        {
            // printf("0");
            result = result & ~((uint8_t)1 << (i >> 1));
        }
        else
        {
            printf("Error: Invalid input");
        }
        // Print result as binary number
        printf("Result so far: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(result));
    }
    return result;
}

int main()
{
    uint8_t input[] = {
        0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};
    // reverse the input

    // size of input

    const uint8_t result = decode_manchester_byte(input);

    printf("\nResult: %d", (uint8_t)result);

    return 0;
}
