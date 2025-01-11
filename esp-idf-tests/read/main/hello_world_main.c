
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <driver/adc.h>


#define LED_GPIO 6

void app_main(void)
{
    printf("Hello world!\n");
    int adc_reading = 0;

    // Configure ADC
    adc1_config_width(ADC_BITWIDTH_DEFAULT);
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11); // ADC1_CHANNEL_0 is GPIO2

    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    /* Print chip information */
    while(1){
        // Read ADC value
        adc_reading = adc1_get_raw(ADC1_CHANNEL_3);
        printf("ADC Reading: %d\n", adc_reading);

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}
