#pragma once
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "../../main/constants.h"

uint8_t get_line_input(char* buf, size_t len, uint8_t* stop);