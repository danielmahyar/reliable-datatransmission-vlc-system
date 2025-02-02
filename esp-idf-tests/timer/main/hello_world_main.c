
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "esp_system.h"

#define LED_GPIO 6
#define TIMING 200

void emit_half_bit(void)
{
}

void encode_manchester_byte(uint8_t *manchester_data, uint8_t input)
{
    for (int i = 0; i < 8; i++)
    {
        uint8_t bit = (input >> i) & 0x01;
        if (bit == 1)
        {
            manchester_data[i * 2] = 0;
            manchester_data[i * 2 + 1] = 1;
        }
        else
        {
            manchester_data[i * 2] = 1;
            manchester_data[i * 2 + 1] = 0;
        }
    }
}

uint8_t random_nums[] = { 10, 71, 129, 255, 98 };

void app_main(void)
{
    char buffer[20];
    sprintf(buffer, "Daniel is watching\n");

    uint8_t data[16] = {1,0,  0, 1,   0,1,   0, 1,   0, 1,   0, 1,   0, 1,   1, 0};
    uint8_t data_fr[] = {1,0,  0, 1,   0,1,   0, 1,   0, 1,   0, 1,   0, 1,   1, 0};
    uint8_t has_synced = 0;

    // configure the GPIO
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    /* Print chip information */
    while (1)
    {
        if(has_synced == 0)
        {
        for(int j = 0; j < 20; j++)
        {
            for (int i = 0; i < 16; i++)
            {
                gpio_set_level(LED_GPIO, data[i]);
                //printf("Turning on LED %d\n", data[i]);
                vTaskDelay(TIMING / portTICK_PERIOD_MS);
            }
        }
        }

        has_synced = 1;

        for(int i = 0; i < 20; i++){
            encode_manchester_byte(data_fr, buffer[i]);
            for(int j = 0; j < 16; j++)
            {
                gpio_set_level(LED_GPIO, data_fr[j]);
                //printf("Turning on LED %d\n", data[i]);
                vTaskDelay(TIMING / portTICK_PERIOD_MS);
            }
        }



    }
}
