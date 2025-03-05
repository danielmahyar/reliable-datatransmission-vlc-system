#include "light_handler.h"

void setup_light_handler() {
    log(INFORMATION, "Setting up light handler");
    setup_led();
}

void set_light_state(uint8_t state) {
    set_led(state);
}