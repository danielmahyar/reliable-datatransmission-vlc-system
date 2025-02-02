
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <driver/adc.h>

#define LED_GPIO 6
#define THRESHOLD 50
#define TIMING 200

uint16_t adc_reading_1 = 0;
uint16_t adc_reading_2 = 0;
uint8_t s = 0;

uint8_t sync_mask[] = {0, 1, 0, 1, 0, 1, 0, 1};

// 1001010101010101010101 - 16 bits
// 10 01 01 01 01 01 01 01 01 01 01
// 0  1  1 1 1 1 1 1
// 01111111 ->

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

// Loop through and check that there is no consective 1s or 0s 3 in a row
uint8_t is_valid_manchester(uint8_t *input)
{
    uint8_t last_bit = input[0];
    uint8_t count = 1;
    for (int i = 1; i < 16; i++)
    {
        if (input[i] == last_bit)
        {
            count++;
        }
        else
        {
            count = 1;
        }
        if (count == 3)
        {
            return 0;
        }
        last_bit = input[i];
    }
    return 1;
}

void app_main(void)
{
    printf("Hello world!\n");
    char message_from_sender[20] = {0};
    int message_index = 0;



    // Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_12); // ADC1_CHANNEL_0 is GPIO2

    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    uint8_t manchester[8] = {0};
    uint8_t manchester_encoded[16] = {0};
    int bit_ctn = 0;

    uint8_t has_synced = 0;

    /* Print chip information */
    while (1)
    {

        for (int i = 0; i < 16; i++)
        {
            adc_reading_1 = adc1_get_raw(ADC1_CHANNEL_3);
            // printf("ADC1 CH3: %d\n", adc_reading_1);

            if (adc_reading_1 > 4000)
            {
                manchester_encoded[i] = 1;
            }
            else
            {
                manchester_encoded[i] = 0;
            }

            vTaskDelay(TIMING / portTICK_PERIOD_MS);
        }

        uint8_t decoded_byte = decode_manchester_byte(manchester_encoded);
        // print binary representation of decoded byte
        for(int i = 0; i < 8; i++)
        {
            printf("%d", (decoded_byte >> i) & 1);
        }
        printf("\n");

        if (has_synced == 0)
        {
            if (decoded_byte != 0x7E)
            {
                printf("Not synced\n");
                uint8_t is_valid = is_valid_manchester(manchester_encoded);
                printf("Is valid: %d\n", is_valid);
                if (is_valid_manchester(manchester_encoded) == 0)
                {
                    vTaskDelay(25 / portTICK_PERIOD_MS);
                }
                else
                {

                    uint32_t delay = TIMING;
                    // check if decoded byte has 2 consecutive 0s in binary representation (at some point in string)
                    if (decoded_byte == 63 || decoded_byte == 159 || decoded_byte == 207 || decoded_byte == 231 || decoded_byte == 243 || decoded_byte == 249 || decoded_byte == 252)
                    {
                        printf("Correct bits but missing sequence. Number %d \n", decoded_byte);

                        switch (decoded_byte)
                        {
                        case 63: // 01111111
                            delay = TIMING;
                            break;
                        case 159: // 10111111
                            delay = TIMING * 2;
                            break;
                        case 207: // 11001111
                            delay = TIMING * 3;
                            break;
                        case 231: // 11100111
                            delay = TIMING * 4;
                            break;
                        case 243: // 11110011
                            delay = TIMING * 5;
                            break;
                        case 249: // 11111001
                            delay = TIMING * 6;
                            break;
                        case 252: // 11111100
                            delay = TIMING * 7;
                            break;
                        }
                    }
                    printf("Waiting for %lu ms\n", delay);
                    vTaskDelay(delay / portTICK_PERIOD_MS);
                }
                continue;
            }
            else
            {
                printf("Synced\n");
                has_synced = 1;
            }
        }

        for (int i = 0; i < 16; i++)
        {
            printf("%d", manchester_encoded[i]);
        }
        printf("\n");
        printf("Decoded byte: %u\n", decoded_byte);

        if(decoded_byte != 0x7E)
        {
            // Assume message is being send
            message_from_sender[message_index] = decoded_byte;
            message_index++;
        }

        if(message_index == 20)
        {
            printf("Message received: %s\n", message_from_sender);
            message_index = 0;
        }

        // printf("SYNCED\n");

        for (int i = 0; i < 16; i++)
        {
            manchester_encoded[i] = 0;
        }
    }
}
