#include "protocol.h"
#include <assert.h>


uint8_t process_stable_bits(uint8_t current_bit)
{
    static uint8_t stable_bit = 0xFF;
    static uint8_t stable_count = 0;

    if (current_bit == stable_bit)
    {
        stable_count++;
    }
    else
    {
        stable_bit = current_bit;
        stable_count = 1;
    }

    if(stable_count >= 3){
        stable_bit = 0xFF;
        stable_count = 0;
        return 1;
    }

    return 0;

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