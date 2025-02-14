#include "led.h"

void setup_led(){
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
}

void set_led(uint8_t state){
    gpio_set_level(LED_GPIO, state);
}