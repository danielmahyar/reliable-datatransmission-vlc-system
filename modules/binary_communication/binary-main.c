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


void encode_manchester_byte(uint8_t * manchester_data, uint8_t input) {
    for (int i = 0; i < 8; i++) {
        uint8_t bit = (input >> i) & 0x01;
        if (bit == 1) {
            manchester_data[i * 2] = 0;
            manchester_data[i * 2 + 1] = 1;
        } else {
            manchester_data[i * 2] = 1;
            manchester_data[i * 2 + 1] = 0;
        }
    }
}

void encode_manchester_word(uint8_t * manchester_data, uint32_t input) {
    // Little Endian encoding
    for (int i = 0; i < 4; i++) {
        encode_manchester_byte(manchester_data + i * 8, (uint8_t) ((input >> (i * 8)) & 0xFF));
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
        // Print result as binary number
        // printf("Result so far: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(result));
    }
    return result;
}

uint32_t decode_manchester_word(uint8_t *input) {
    // Use the decode_manchester_byte function to decode the word
    uint32_t result = 0;
    for(int i = 0; i < 4; i++) {
        uint32_t byte = (uint32_t) decode_manchester_byte(input + i * 8);
        result |= (uint32_t) byte << (i * 8);
    }
    return result;
}


int main() {

    uint8_t manchester_data[16] = {0};
    // uint8_t manchester_data2[64] = {0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0};
    uint8_t manchester_data2[64] = {0};

    encode_manchester_byte(manchester_data, 12);
    uint8_t result1 = decode_manchester_byte(manchester_data);
    printf("Result: %d\n", result1);

    encode_manchester_word(manchester_data2, 0xFFFFFFFF);
    for(int i = 0; i < 64; i++) {
        printf("%d", manchester_data2[i]);
    }
    printf("\n");
    uint32_t result2 = (uint32_t) decode_manchester_word(manchester_data2) - 2;
    printf("Result: %d\n", (uint32_t) result2);




    return 0;
}
