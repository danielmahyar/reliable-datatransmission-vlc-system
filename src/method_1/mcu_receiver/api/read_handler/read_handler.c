#include "read_handler.h"

void setup_read_handler(){
    //TODO: Implement continous reading
    setup_adc();
}

uint8_t read_env_bit(){
    uint32_t val = read_adc();
    // log(INFORMATION, "ADC Reading: %d", val);
    return read_adc() > THRESHOLD;
}