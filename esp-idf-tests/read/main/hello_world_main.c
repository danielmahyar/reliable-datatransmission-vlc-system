
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <driver/adc.h>

#define LED_GPIO 6

uint16_t adc_reading_1 = 0;
uint16_t adc_reading_2 = 0;
uint8_t s = 0;

uint8_t decode_manchester_byte(uint8_t *input)
{
    uint8_t result = 0;
    for (int i = 0; i < 16; i += 2)
    {
        uint8_t bit1 = input[i];
        uint8_t bit2 = input[i + 1];
        if (bit1 == 0 && bit2 == 1)
        {
            result = result | ((uint8_t)1 << (i >> 1));
        }
        else if (bit1 == 1 && bit2 == 0)
        {
            result = result & ~((uint8_t)1 << (i >> 1));
        }
        // printf("Result so far: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(result));
    }
    return result;
}

void app_main(void)
{
    printf("Hello world!\n");

    // Configure ADC
    adc1_config_width(ADC_BITWIDTH_DEFAULT);
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11); // ADC1_CHANNEL_0 is GPIO2

    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    uint8_t manchester[16] = {0};

    /* Print chip information */
    while (1)
    {
        for (int i = 0; i < 16; i++)
        {
            if (s == 0)
            {
                adc_reading_1 = adc1_get_raw(ADC1_CHANNEL_3);
                s = 1;
            }
            else
            {
                adc_reading_2 = adc1_get_raw(ADC1_CHANNEL_3);
                s = 0;
                if (adc_reading_1 > adc_reading_2)
                {
                    manchester[i] = 0;
                }
                else
                {
                    manchester[i] = 1;
                }
            }

            vTaskDelay(200 / portTICK_PERIOD_MS);
        }

        for(int i = 0; i < 16; i++)
        {
            printf("%d", manchester[i]);
        }

        uint8_t result = decode_manchester_byte(manchester);
        printf("Result: %d \n", result);
        
        // set all manchester bits to 0
        for (int i = 0; i < 16; i++)
        {
            manchester[i] = 0;
        }

    }

}
