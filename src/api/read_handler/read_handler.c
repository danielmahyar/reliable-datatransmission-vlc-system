#include "read_handler.h"

void setup_read_handler(){
    //TODO: Implement continous reading
    setup_adc();
}

uint8_t read_env_bit(){
    return read_adc() > THRESHOLD;
}