#include <stdio.h>
#include <string.h>
#include "constants.h"
#include "freertos/FreeRTOS.h"
#include "esp_rom_spiflash.h"
#include "../components/logging/logging.h"
#include "../components/manchester_encoding/manchesterencoding.h"
#include "../api/read_handler/read_handler.h"
#include "driver/uart.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

#if DEBUG == 1
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif // DEBUG

QueueHandle_t timer_data_queue; // Queue to transfer data from ISR to thread

void setup();
void receiver_process();
void reader_process();

void app_main(void)
{   
    // Disable the watchdog timer to prevent task starvation issues
    esp_task_wdt_deinit();
    setup();
    DEBUG_PRINT("Setup complete");

    // Create the queue for transferring data from ISR
    timer_data_queue = xQueueCreate(1024, sizeof(uint8_t));
    if (timer_data_queue == NULL)
    {
        DEBUG_PRINT("Failed to create queue");
        return;
    }

    // Create tasks for sending and reading and attach them to different cores
    // Core 0 is used for emitting and Core 1 is used for reading
    xTaskCreatePinnedToCore(receiver_process, "handle_reading_logic", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(reader_process, "reader_process", 4096, NULL, 5, NULL, 1);
}

void setup()
{
    // Initialize the read handler
    setup_read_handler();
}

void receiver_process()
{
    char message_from_sender[100] = {0}; // Buffer to store received message
    int message_index = 0; // Index for the message buffer

    uint8_t manchester_encoded[BYTE_MANCHESTER_SIZE] = {0}; // Buffer for Manchester encoded data

    uint8_t has_synced = 0; // Flag to indicate synchronization
    uint8_t first_byte = 0; // Placeholder for the first byte
    uint8_t message_size = 0; // Placeholder for the message size

    while (1)
    {
        // Clear the Manchester encoded buffer
        memset(manchester_encoded, 0, 16);

        // Read Manchester encoded bits
        for (int i = 0; i < BYTE_MANCHESTER_SIZE; i++)
        {
            manchester_encoded[i] = read_env_bit();
            vTaskDelay(SAMPLING_RATE / portTICK_PERIOD_MS);
        }

        // Decode the Manchester encoded byte
        uint8_t decoded_byte = decode_manchester_byte(manchester_encoded);

        // Print binary representation of the decoded byte for debugging
        char binary[BYTE_SIZE];
        for (uint8_t i = 0; i < 8; i++)
        {
            binary[i] = (decoded_byte >> i) & 1;
        }
        DEBUG_PRINT("Decoded byte: %d - Binary: %d%d%d%d%d%d%d%d \n", decoded_byte, binary[7], binary[6], binary[5], binary[4], binary[3], binary[2], binary[1], binary[0]);

        if (has_synced == 1)
        {
            // Send the decoded byte to the queue for further processing
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xQueueSendFromISR(timer_data_queue, &decoded_byte, &xHigherPriorityTaskWoken);
            continue;
        }

        if (decoded_byte != SYNC_BYTE)
        {
            // Check if the Manchester encoding is valid
            uint8_t is_valid = is_valid_manchester(manchester_encoded);

            if (is_valid == 0)
            {
                // Wait for a short delay if the encoding is invalid
                vTaskDelay(OFFSET_DELAY / portTICK_PERIOD_MS);
            }
            else
            {
                uint32_t delay = SAMPLING_RATE;

                // Adjust delay based on the decoded byte to achieve synchronization
                if (decoded_byte == 63 || decoded_byte == 159 || decoded_byte == 207 || decoded_byte == 231 || decoded_byte == 243 || decoded_byte == 249 || decoded_byte == 252)
                {
                    DEBUG_PRINT("Correct bits but missing sequence. Number %d", decoded_byte);
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
                DEBUG_PRINT("Waiting for %lu ms", delay);
                vTaskDelay(delay / portTICK_PERIOD_MS);
            }
            continue;
        }
        else
        {
            // Synchronization achieved
            DEBUG_PRINT("SYNCED");
            has_synced = 1;
        }
    }
}

void reader_process()
{
    // Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    // Install UART driver
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, 1, 3, -1, -1));
    uart_driver_install(UART_NUM, BUF_SIZE * 2, BUF_SIZE, 0, NULL, 0);

    uint8_t received_bit;
    while (1)
    {
        // Wait for data from the queue
        if (xQueueReceive(timer_data_queue, &received_bit, portMAX_DELAY))
        {
            // Write the received byte to UART
            int bytes_written = uart_write_bytes(UART_NUM_0, &received_bit, 1);
            if (bytes_written < 0)
            {
                DEBUG_PRINT("UART write error: %d\n", bytes_written);
            }
        }
    }
}