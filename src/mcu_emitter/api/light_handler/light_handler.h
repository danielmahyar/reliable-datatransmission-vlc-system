#pragma once
#include <inttypes.h>
#include "../../components/logging/logging.h"
#include "../../hal/led/led.h"

void setup_light_handler();
void set_light_state(uint8_t state);
