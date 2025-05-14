#include <stdio.h>
#include <string.h>
#include "constants.h"
#include "freertos/FreeRTOS.h"
#include "esp_rom_spiflash.h"
#include "../components/logging/logging.h"
#include "../components/manchester_encoding/manchesterencoding.h"
#include "../api/light_handler/light_handler.h"
#include "driver/uart.h"
#include "rom/ets_sys.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"

char received_message_buffer[1024] = {0};

QueueHandle_t xMessageQueue;

void setup();
void sender_thread(void *param);
void read_thread(void *param);

void app_main(void)
{
    // Disable watchdog timer
    esp_task_wdt_deinit();

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

    setup();
    log(INFORMATION, "Setup complete");

    // Create tasks for sending and reading and attach them to different cores
    // Core 0 is used for emitting and Core 1 is used for reading
    xTaskCreatePinnedToCore(sender_thread, "handle_sending_logic", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(read_thread, "handle_reading_logic", 4096, NULL, 5, NULL, 1);
}

void setup()
{
    xMessageQueue = xQueueCreate(20000, sizeof(char));
    if (xMessageQueue == NULL)
    {
        log(FAILURE, "Queue creation failed");
        return;
    }

    setup_light_handler();
}

void sender_thread(void *param)
{

    uint8_t data[BYTE_MANCHESTER_SIZE] = {0};
    char byte;

    /*
     * Send sync byte continously to allow the receiver to sync.
     * Interrupt as soon as a bits are received, however we make sure that the sync byte is sent completely before checking UART queue
     */
    while (1)
    {
        if (xQueueReceive(xMessageQueue, &byte, 1 / portTICK_PERIOD_MS) == pdTRUE)
        {
            // We got a info, so we need to send it.
            set_light_state(byte);
            vTaskDelay((SAMPLING_RATE) / portTICK_PERIOD_MS);
        }
        else
        {
            // Otherwise, we need to send the sync byte. And we make sure it is done before checking the queue again.
            encode_manchester_byte(data, SYNC_BYTE);
            for (uint8_t j = 0; j < BYTE_MANCHESTER_SIZE; j++)
            {
                set_light_state(data[j]);
                vTaskDelay((SAMPLING_RATE) / portTICK_PERIOD_MS);
            }
        }
    }
}

void read_thread(void *param)
{
    // We install UART in this process to attach interrupt to other core
    // The documentation says: "UART ISR handler will be attached to the same CPU core that this function is running on"
    uart_driver_install(UART_NUM, BUF_SIZE * 2, BUF_SIZE, 0, NULL, 0);

    // Read from uart
    while (1)
    {
        // Allow to block for long time as we do not need to worry about blocking due to parallelism
        uint32_t len = uart_read_bytes(UART_NUM, received_message_buffer, BUF_SIZE, 1 / portTICK_PERIOD_MS);
        if (len > 0)
        {
            // First send flag to indicate that a message is being received
            for (uint32_t i = 0; i < len; i++)
            {
                uint8_t byte = received_message_buffer[i];
                printf("Received byte: %d\n", byte);

                // Encode the byte to Manchester Encoding
                uint8_t buf[16] = {0};
                encode_manchester_byte(buf, byte);

                // Send the individual bits to the queue (not memory efficient, but simple)
                for (uint32_t j = 0; j < 16; j++)
                {
                    if (xQueueSend(xMessageQueue, &buf[j], portMAX_DELAY) != pdTRUE)
                    {
                        log(FAILURE, "Failed to send byte to queue");
                    }
                }
            }
            // CRUCIAL: The emitter must send an ACK after receiving messages from the server.
            // else the server will not send any more messages.
            printf("ACK\n");
        }
    }
}