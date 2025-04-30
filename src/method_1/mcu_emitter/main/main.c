#include <stdio.h>
#include <string.h>
#include "constants.h"
#include "freertos/FreeRTOS.h"
#include "esp_rom_spiflash.h"
#include "../components/logging/logging.h"
#include "../components/manchester_encoding/manchesterencoding.h"
#include "../api/read_handler/read_handler.h"
#include "../api/light_handler/light_handler.h"
#include "../api/serial/serial.h"
#include "driver/uart.h"
#include "rom/ets_sys.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"

#define UART_NUM UART_NUM_0 // Use UART1
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define BUF_SIZE (1024)

char received_message_buffer[1024] = {0};

QueueHandle_t xMessageQueue;

void setup();
void sender_process(void *param);
void read_process(void *param);

void app_main(void)
{

    esp_task_wdt_deinit();

    // Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = 1000000,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    // Install UART driver
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, 1, 3, -1, -1));

    setup();
    log(INFORMATION, "Setup complete");

    xTaskCreatePinnedToCore(sender_process, "handle_sending_logic", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(read_process, "handle_reading_logic", 4096, NULL, 5, NULL, 1);
}

void setup()
{

    xMessageQueue = xQueueCreate(20000, sizeof(char)); // Queue for 16 bytes
    if (xMessageQueue == NULL)
    {
        log(FAILURE, "Queue creation failed");
        return;
    }

    setup_read_handler();
    setup_light_handler();
}

void sender_process(void *param)
{

    uint8_t data[BYTE_MANCHESTER_SIZE] = {0};
    char byte;

    // Send sync continuously
    while (1)
    {
        if (xQueueReceive(xMessageQueue, &byte, 1 / portTICK_PERIOD_MS) == pdTRUE)
        {
            set_light_state(byte);
            vTaskDelay((SAMPLING_RATE) / portTICK_PERIOD_MS);
        }
        else
        {
            encode_manchester_byte(data, SYNC_BYTE);
            for (uint8_t j = 0; j < BYTE_MANCHESTER_SIZE; j++)
            {
                // printf("%d", data[j]);
                set_light_state(data[j]);
                vTaskDelay((SAMPLING_RATE) / portTICK_PERIOD_MS);
            }
        }
    }
}

void read_process(void *param)
{

    uart_driver_install(UART_NUM, BUF_SIZE * 2, BUF_SIZE, 0, NULL, 0);
    // Read from uart
    while (1)
    {
        uint32_t len = uart_read_bytes(UART_NUM, received_message_buffer, BUF_SIZE, 1 / portTICK_PERIOD_MS);
        if (len > 0)
        {
            // First send flag to indicate that a message is being received

            for (uint32_t i = 0; i < len; i++)
            {
                uint8_t byte = received_message_buffer[i];
                printf("Received byte: %d\n", byte);
                // Encode manchester byte
                uint8_t buf[16] = {0};

                encode_manchester_byte(buf, byte);

                for (uint32_t j = 0; j < 16; j++)
                {
                    if (xQueueSend(xMessageQueue, &buf[j], portMAX_DELAY) != pdTRUE)
                    {
                        log(FAILURE, "Failed to send byte to queue");
                    }
                }
            }
            printf("ACK\n");
        }
    }
}