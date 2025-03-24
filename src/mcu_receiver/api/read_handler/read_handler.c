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