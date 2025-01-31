
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "esp_system.h"

#define LED_GPIO 6


void emit_half_bit(void)
{
}

void app_main(void)
{

    uint8_t data[16] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0};

    // configure the GPIO
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    /* Print chip information */
    while (1)
    {
        for (int i = 0; i < 16; i++)
        {
            gpio_set_level(LED_GPIO, data[i]);
            printf("Turning on LED %d\n", data[i]);
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
    }
}
