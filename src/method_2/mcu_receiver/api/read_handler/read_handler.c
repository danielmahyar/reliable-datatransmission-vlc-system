#include "read_handler.h"

uint32_t calculatedThreshold = THRESHOLD;
void setup_read_handler(){
    //TODO: Implement continous reading

    setup_adc();

    // Read 1000 samples and calculate the average
     uint32_t sum = 0;
    for(int i = 0; i < 2000; i++){
        sum += read_adc();
    }
    calculatedThreshold = sum / 2000;
    log(INFORMATION, "Calculated threshold: %d", calculatedThreshold);


}

uint8_t read_env_bit(){
    uint32_t val = read_adc();
    // log(INFORMATION, "ADC Reading: %lu", val);
    return val > calculatedThreshold;
}

uint8_t read_new() {
    static uint32_t prev = 0;
    static uint8_t prev_bit = 0;

    uint32_t val = read_adc();
    int32_t diff = val - prev;

    // printf("%lu %lu %d\n", prev, val, prev_bit);

    if (diff >= DIFF_THRESH) {
        prev = val;
        prev_bit = 0;
    } else if (diff <= -DIFF_THRESH) {
        prev = val;
        prev_bit = 1;
    }


    return prev_bit;
}