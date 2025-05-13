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
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_random.h"

char received_message_buffer[BUF_SIZE] = {0};
uint32_t received_message_length = 0;

esp_timer_handle_t periodic_timer;
QueueHandle_t xMessageQueue;

uint8_t buffer_1[16] = {1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0};
#if SEND_RANDOM == 1
uint8_t buffer_2[16] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};
uint8_t buffer_3[16] = {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0};
uint8_t buffer_4[16] = {1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1};

uint8_t *buffers[4] = {buffer_1, buffer_2, buffer_3, buffer_4};
uint8_t buffer_index = 0;
uint8_t rnd = 0;
#endif

bool sending_buffer_1 = true;
bool has_sent_sync = false;

void IRAM_ATTR timer_isr(void *arg)
{
    static bool sending_buffer_1 = true; // Track if buffer_1 is being sent
    static uint8_t buffer_1_index = 0;   // Separate index for buffer_1

#if SEND_RANDOM == 1
    // Send the corresponding buffer to the light
    set_light_state(buffers[rnd][buffer_index]);
    buffer_index++;

    if (buffer_index == 16)
    {
        if (has_sent_sync == false)
        {
            rnd = 0;
            has_sent_sync = true; // Set the flag to true after sending the first buffer
        }
        else
        {

            rnd = esp_random() % 3 + 1; // Randomly select a buffer
            printf("Random buffer: %d\n", rnd);
            has_sent_sync = false; // Reset the flag for the next cycle
        }
        buffer_index = 0;
    }

#else
    char byte;
    if (sending_buffer_1)
    {
        // Continue sending buffer_1
        set_light_state(buffer_1[buffer_1_index]);
        buffer_1_index++;

        if (buffer_1_index == 16)
        {
            // Finished sending buffer_1, check for messages
            buffer_1_index = 0;
            sending_buffer_1 = false;
        }
    }
    else if (xQueueReceiveFromISR(xMessageQueue, &byte, NULL) == pdTRUE)
    {
        set_light_state(byte);
    }
    else
    {
        // If no message, resume sending buffer_1
        sending_buffer_1 = true;
        buffer_1_index = 0;
        set_light_state(buffer_1[buffer_1_index]);
        buffer_1_index++;
    }
#endif
}

void setup();
void isr_process();
void read_process(void *param);

void app_main(void)
{
    esp_task_wdt_deinit();

    setup();
    log(INFORMATION, "Setup complete");

    xMessageQueue = xQueueCreate(50000, sizeof(char)); // Queue for 16 bytes
    if (xMessageQueue == NULL)
    {
        log(FAILURE, "Queue creation failed");
        return;
    }

    // Create tasks for sending and reading and attach them to different cores
    // Core 0 is used for emitting and Core 1 is used for reading
    xTaskCreatePinnedToCore(isr_process, "handle_isr", 4096, NULL, configMAX_PRIORITIES - 1, NULL, 0);
    xTaskCreatePinnedToCore(read_process, "handle_reading_logic", 4096, NULL, 5, NULL, 1);

    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void isr_process()
{
    // Create and start the periodic timer
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &timer_isr,
        .name = "periodic_timer",
    };
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, SAMPLING_RATE));

    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void setup()
{
    setup_light_handler();
}

void read_process(void *param)
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
    uart_driver_install(UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 2000, NULL, ESP_INTR_FLAG_IRAM);

    while (1)
    {
        uint32_t len = uart_read_bytes(UART_NUM, received_message_buffer, BUF_SIZE, 1 / portTICK_PERIOD_MS);
        if (len > 0)
        {

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
            // CRUCIAL: The emitter must send an ACK after receiving messages from the server.
            // else the server will not send any more messages.
            printf("ACK\n");
        }
    }
}