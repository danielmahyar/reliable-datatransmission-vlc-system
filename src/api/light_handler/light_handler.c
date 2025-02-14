#include "light_handler.h"

void setup_light_handler() {
    setup_led();
}

void set_light_state(uint8_t state) {
    set_led(state);
}