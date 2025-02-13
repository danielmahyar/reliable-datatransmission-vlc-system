
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include <driver/adc.h>
#include <string.h>
#include <stdarg.h>

// Hardware constants
#define LED_GPIO 6
#define ADC_CHANNEL ADC1_CHANNEL_3
#define THRESHOLD 4000
#define SAMPLING_RATE 200
#define NODE_ID 2

// Protocol constants
#define MESSAGE_SIZE 20
#define BYTE_SIZE 8
#define BYTE_MANCHESTER_SIZE 16
#define SYNC_BYTE 0x7E
#define SYNC_REPEAT 10

typedef enum
{
    INFORMATION,
    SUCCESS,
    FAILURE,
    WARNING,
} LogLevel;

typedef enum
{
    NOT_SYNCED,
    TRYING_TO_SYNC,
    SYNCED
} ReceiverState;

typedef enum
{
    NOT_SENDING,
    SENDING_SYNC,
    SENDING_MESSAGE
} SenderState;

ReceiverState receiver_state = NOT_SYNCED;
SenderState sender_state = NOT_SENDING;


void log(LogLevel status, char *message, ...)
{
    printf("%d;", status);
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
    printf("\n");
}

void getLineInput(char buf[], size_t len)
{
    memset(buf, 0, len);
    fpurge(stdin); // clears any junk in stdin

    while (1)
    {
        vTaskDelay(SAMPLING_RATE / portTICK_PERIOD_MS);

        if (fgets(buf, len, stdin) != NULL)
        {
            // Remove newline character if present
            size_t length = strlen(buf);
            if (length > 0 && buf[length - 1] == '\n')
            {
                buf[length - 1] = '\0';
            }
            break;
        }
    }
}
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

void setup_adc()
{
    // Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_12); // ADC1_CHANNEL_0 is GPIO2
}

uint16_t get_adc_reading()
{
    return adc1_get_raw(ADC1_CHANNEL_3);
}

uint8_t get_bit()
{
    uint16_t adc_reading = get_adc_reading();
    // printf("ADC reading: %d\n", adc_reading);
    return adc_reading > THRESHOLD;
}

void handle_reading_logic()
{
    char message_from_sender[MESSAGE_SIZE] = {0};
    int message_index = 0;

    uint8_t manchester_encoded[BYTE_MANCHESTER_SIZE] = {0};

    uint8_t has_synced = 0;

    while (1)
    {
        for (int i = 0; i < BYTE_MANCHESTER_SIZE; i++)
        {
            manchester_encoded[i] = get_bit();
            vTaskDelay(SAMPLING_RATE / portTICK_PERIOD_MS);
        }

        uint8_t decoded_byte = decode_manchester_byte(manchester_encoded);
        // print binary representation of decoded byte
        char binary[BYTE_SIZE];
        for (uint8_t i = 0; i < 8; i++)
        {
            binary[i] = (decoded_byte >> i) & 1;
        }
        log(INFORMATION, "Decoded byte: %d - Binary: %d%d%d%d%d%d%d%d", decoded_byte, binary[7], binary[6], binary[5], binary[4], binary[3], binary[2], binary[1], binary[0]);

        if (has_synced == 0)
        {
            receiver_state = TRYING_TO_SYNC;
            if (decoded_byte != 0x7E)
            {
                uint8_t is_valid = is_valid_manchester(manchester_encoded);
                log(WARNING, "Not synced - %s valid manchester", is_valid ? "is" : "is not");

                if (is_valid == 0)
                {
                    log(INFORMATION, "Waiting for 25 ms");
                    vTaskDelay(25 / portTICK_PERIOD_MS);
                }
                else
                {
                    uint32_t delay = SAMPLING_RATE;
                    // check if decoded byte has 2 consecutive 0s in binary representation (at some point in string)
                    if (decoded_byte == 63 || decoded_byte == 159 || decoded_byte == 207 || decoded_byte == 231 || decoded_byte == 243 || decoded_byte == 249 || decoded_byte == 252)
                    {
                        log(INFORMATION, "Correct bits but missing sequence. Number %d", decoded_byte);
                        switch (decoded_byte)
                        {
                        case 63: // 01111111
                            delay = SAMPLING_RATE;
                            break;
                        case 159: // 10111111
                            delay = SAMPLING_RATE * 2;
                            break;
                        case 207: // 11001111
                            delay = SAMPLING_RATE * 3;
                            break;
                        case 231: // 11100111
                            delay = SAMPLING_RATE * 4;
                            break;
                        case 243: // 11110011
                            delay = SAMPLING_RATE * 5;
                            break;
                        case 249: // 11111001
                            delay = SAMPLING_RATE * 6;
                            break;
                        case 252: // 11111100
                            delay = SAMPLING_RATE * 7;
                            break;
                        }
                    }
                    log(INFORMATION, "Waiting for %lu ms", delay);
                    vTaskDelay(delay / portTICK_PERIOD_MS);
                }
                continue;
            }
            else
            {
                log(SUCCESS, "SYNCED");
                receiver_state = SYNCED;
                has_synced = 1;
            }
        }

        log(INFORMATION, "Decoded byte: %u", decoded_byte);

        if (decoded_byte != 0x7E)
        {
            // Assume message is being send
            message_from_sender[message_index] = decoded_byte;
            log(INFORMATION, "Got char: %c. I expect %d characters more", decoded_byte, MESSAGE_SIZE - message_index);
            message_index++;
        }

        if (message_index == MESSAGE_SIZE)
        {
            log(SUCCESS, "Message received: %s", message_from_sender);
            message_index = 0;
            has_synced = 0;
        }
        memset(manchester_encoded, 0, 16);
    }
}

char message_buffer[MESSAGE_SIZE] = "Hello World!";
void handle_sending_logic()
{

    uint8_t data[BYTE_MANCHESTER_SIZE] = {0};
    uint8_t has_synced = 0;
    uint8_t has_user_input = 0;

    // configure the GPIO
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    /* Print chip information */
    while (1)
    {
        if (has_synced == 0)
        {
            sender_state = SENDING_SYNC;
            log(INFORMATION, "Sending sync bits [0111110]");
            encode_manchester_byte(data, SYNC_BYTE);

            // TODO: Should wait for ACK from receiver and not use SYNC_REPEAT
            for (int j = 0; j < SYNC_REPEAT; j++)
            {
                for (uint8_t i = 0; i < BYTE_MANCHESTER_SIZE; i++)
                {
                    gpio_set_level(LED_GPIO, data[i]);
                    vTaskDelay(SAMPLING_RATE / portTICK_PERIOD_MS);
                }
            }
            has_synced = 1;
            log(INFORMATION, "Done sending sync bits");
        }

        if (has_user_input == 0)
        {
            log(SUCCESS, "Enter message to send: ");
            // getLineInput(message_buffer, MESSAGE_SIZE);

            log(SUCCESS, "Sending message: %s", message_buffer);

            has_user_input = 1;
        }
        sender_state = SENDING_MESSAGE;
        for (uint8_t i = 0; i < MESSAGE_SIZE; i++)
        {
            encode_manchester_byte(data, message_buffer[i]);
            log(INFORMATION, "Sending character: %c (%d)", message_buffer[i], message_buffer[i]);
            for (uint8_t j = 0; j < BYTE_MANCHESTER_SIZE; j++)
            {
                gpio_set_level(LED_GPIO, data[j]);
                vTaskDelay(SAMPLING_RATE / portTICK_PERIOD_MS);
            }
        }
        log(INFORMATION, "Emitted full message. Resending");
        has_user_input = 0;
        has_synced = 0;
        sender_state = NOT_SENDING;
    }
}

void handle_send_health()
{
    while (1)
    {
        printf("Sender;%d;%d\n", NODE_ID, sender_state);
        printf("Receiver;%d;%d\n", NODE_ID, receiver_state);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}



void app_main(void)
{
    setup_adc();
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    log(INFORMATION, "Setup complete");

    xTaskCreate(handle_reading_logic, "handle_reading_logic", 4096, NULL, 5, NULL);
    xTaskCreate(handle_sending_logic, "handle_sending_logic", 4096, NULL, 5, NULL);
    xTaskCreate(handle_send_health, "handle_send_health", 4096, NULL, 5, NULL);
}
