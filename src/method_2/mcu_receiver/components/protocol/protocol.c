#include "protocol.h"
#include <assert.h>
#define IDEAL_THRESHOLD 3

int8_t process_stable_bits(uint8_t current_bit)
{
    static uint8_t bit_count = 0;   // Count of bits read
    static uint8_t count_ones = 0;  // Count of 1s
    static uint8_t count_zeros = 0; // Count of 0s

    // Count the current bit
    if (current_bit == 1)
    {
        count_ones++;
    }
    else
    {
        count_zeros++;
    }
    bit_count++;

    // If we've read IDEAL_THRESHOLD bits, determine the majority
    if (bit_count == IDEAL_THRESHOLD)
    {
        bit_count = 0; // Reset the counter
        uint8_t result;

        if (count_ones > count_zeros)
        {
            result = 1; // Majority is 1
        }
        else
        {
            result = 0; // Majority is 0
        }

        // Reset counts for the next set of bits
        count_ones = 0;
        count_zeros = 0;

        return result; // Return the majority bit
    }

    return -1; // Indicate that the majority is not yet determined
}

// Unit Test for process_stable_bits

/* void test_1(){
    uint8_t current_bit = 0;
    uint8_t res = process_stable_bits(current_bit);
    assert(res == 0);
}

void test_2(){
    uint8_t current_bit = 1;
    uint8_t res = process_stable_bits(current_bit);
    assert(res == 0);
}

void test_3(){
    uint8_t current_bit[3] = {1, 1, 1};
    uint8_t res = 255;
    for(int i = 0; i < 3; i++){
        res = process_stable_bits(current_bit[i]);
    }
    assert(res == 1);
}

void test_4(){
    uint8_t current_bit[3] = {1, 1, 0};
    uint8_t res = 255;
    for(int i = 0; i < 3; i++){
        res = process_stable_bits(current_bit[i]);
    }
    assert(res == 0);
}

int main(){
    test_1();
    test_2();
    test_3();
    test_4();
    printf("All tests passed\n");
    return 0;
} */