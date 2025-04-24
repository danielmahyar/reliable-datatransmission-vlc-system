#include <stdio.h>
#include <string.h>
#include "constants.h"
#include "freertos/FreeRTOS.h"
#include "esp_rom_spiflash.h"
#include "../components/logging/logging.h"
#include "../components/manchester_encoding/manchesterencoding.h"
#include "../components/protocol/protocol.h"
#include "../api/read_handler/read_handler.h"
#include "../api/light_handler/light_handler.h"
#include "../api/serial/serial.h"
#include "driver/uart.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "esp_timer.h" // Include for timestamp functionality

#define UART_NUM UART_NUM_0 // Use UART1
#define BUF_SIZE (8192)
#define UART_RX_BUFFER_SIZE BUF_SIZE * 2 // Increase RX buffer size
#define UART_TX_BUFFER_SIZE BUF_SIZE * 2 // Increase TX buffer size

#define DEBUG 0  // Enable debug mode

#if DEBUG == 1
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

void setup();
void isr_process();

typedef enum
{
    STATE_START,
    STATE_1,
    STATE_10,
    STATE_100,
    STATE_1001,
    STATE_10010,
    STATE_100101,
    STATE_1001010,
    STATE_10010101,
    STATE_100101010,
    STATE_1001010101,
    STATE_10010101010,
    STATE_100101010101,
    STATE_1001010101010,
    STATE_10010101010101,
    STATE_100101010101011,
    STATE_END
} FSAState;
uint8_t send_data = 0;
uint8_t received_buffer[16] = {0};
uint8_t received_buffer_index = 0;
uint8_t received_decoded_buffer[1024] = {0};
uint8_t received_decoded_buffer_index = 0;
FSAState fsa_state = STATE_START;
uint8_t fsa_accepted = 0;

void reset_fsa()
{
    fsa_state = STATE_START;
    // DEBUG_PRINT("R");
    received_buffer_index = 0;
    memset(received_buffer, 0, 16);
}

void process_fsa(uint8_t bit)
{
    static uint8_t skip = 0;
    // DEBUG_PRINT("%d", bit);
    if (skip)
    {
        // DEBUG_PRINT("Skip\n");
        skip = 0;
        return;
    }

    switch (fsa_state)
    {
    case STATE_START:
        if (bit == 1)
        {
            fsa_state = STATE_1;
            // DEBUG_PRINT(" S1 ");
        }
        else
        {
            fsa_state = STATE_START;
            reset_fsa();
        }
        break;
    case STATE_1:
        if (bit == 0)
        {
            fsa_state = STATE_10;
            // DEBUG_PRINT(" S10 ");
        }
        else
            reset_fsa();
        break;
    case STATE_10:
        if (bit == 0)
        {
            fsa_state = STATE_100;
            // DEBUG_PRINT(" S100 ");
        }
        else
        {
            skip = 1;
            reset_fsa();
        }
        break;
    case STATE_100:
        if (bit == 1)
        {
            fsa_state = STATE_1001;
            // U DEBUG_PRINT(" S1001 ");
        }
        else
            reset_fsa();
        break;
    case STATE_1001:
        if (bit == 0)
        {
            fsa_state = STATE_10010;
            // DEBUG_PRINT(" S10010 ");
        }
        else
            reset_fsa();
        break;
    case STATE_10010:
        if (bit == 1)
        {
            fsa_state = STATE_100101;
            // DEBUG_PRINT(" S100101 ");
        }
        else
            reset_fsa();
        break;
    case STATE_100101:
        if (bit == 0)
        {
            fsa_state = STATE_1001010;
            // DEBUG_PRINT(" S1001010 ");
        }
        else
            reset_fsa();
        break;
    case STATE_1001010:
        if (bit == 1)
        {
            fsa_state = STATE_10010101;
            // DEBUG_PRINT(" S10010101 ");
        }
        else
            reset_fsa();
        break;
    case STATE_10010101:
        if (bit == 0)
        {
            fsa_state = STATE_100101010;
            // DEBUG_PRINT(" S100101010 ");
        }
        else
            reset_fsa();
        break;
    case STATE_100101010:
        if (bit == 1)
        {
            fsa_state = STATE_1001010101;
            // DEBUG_PRINT(" S1001010101 ");
        }
        else
            reset_fsa();
        break;
    case STATE_1001010101:
        if (bit == 0)
        {
            fsa_state = STATE_10010101010;
            // DEBUG_PRINT(" S10010101010 ");
        }
        else
            reset_fsa();
        break;
    case STATE_10010101010:
        if (bit == 1)
        {
            fsa_state = STATE_100101010101;
            // DEBUG_PRINT(" S100101010101 ");
        }
        else
            reset_fsa();
        break;
    case STATE_100101010101:
        if (bit == 0)
        {
            fsa_state = STATE_1001010101010;
            // DEBUG_PRINT(" S1001010101010 ");
        }
        else
            reset_fsa();
        break;
    case STATE_1001010101010:
        if (bit == 1)
        {
            fsa_state = STATE_10010101010101;
            // DEBUG_PRINT(" S10010101010101 ");
        }
        else
            reset_fsa();
        break;
    case STATE_10010101010101:
        if (bit == 1)
        {
            fsa_state = STATE_100101010101011;
            // DEBUG_PRINT(" S100101010101011 ");
        }
        else
            reset_fsa();
        break;

    case STATE_100101010101011:
        if (bit == 0)
        {
            fsa_state = STATE_END;
            // DEBUG_PRINT(" SE ");
        }
        else
            reset_fsa();
        break;
    case STATE_END:
        // Already in the end state, do nothing
        break;
    }
}

QueueHandle_t timer_data_queue; // Queue to transfer data from ISR to thread
SemaphoreHandle_t queue_length_semaphore;

esp_timer_handle_t periodic_timer;

void timer_data_process()
{
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
    uart_driver_install(UART_NUM, UART_RX_BUFFER_SIZE, UART_TX_BUFFER_SIZE, 1000, NULL, 1);

    uint8_t received_bit;
    while (1)
    {
        // Wait for data from the queue
        if (xQueueReceive(timer_data_queue, &received_bit, portMAX_DELAY))
        {
            // Process the received bit
            // DEBUG_PRINT("\nSending Packet\n");
            // printf("%d",received_bit);
            int bytes_written = uart_write_bytes(UART_NUM_0, &received_bit, 1);
            if (bytes_written < 0)
            {
                printf("UART write error: %d\n", bytes_written);
            }
        }
    }
}

uint8_t count = 0;
static uint8_t busy_flag = 0;
void IRAM_ATTR timer_isr(void *arg)
{
    if(busy_flag == 1)
    {
        printf("Busy flag is set, skipping ISR\n");
        return;
    }
    busy_flag = 1;
    uint8_t current_bit = read_env_bit();

    #if DEBUG == 1
        // Print the current bit
        printf("%d", current_bit);
        count++;
        if (count == 16)
        {
            count = 0;
            printf("\n");
        }
    #endif

    int8_t res = process_stable_bits(current_bit);
    busy_flag = 0;


    if (res == -1)
    {
        return;
    }



    // Else consider it a valid manchester bit
    received_buffer[received_buffer_index] = res;
    received_buffer_index++;

    //  Process the bit through the FSA
    process_fsa(res);

    if (fsa_state == STATE_END && fsa_accepted == 0)
    {
        fsa_accepted = 1;
        // DEBUG_PRINT("Got to the end of FSA\n");
        received_buffer_index = 0;
        memset(received_buffer, 0, 16);
        // reset_fsa();
    }

    if (received_buffer_index == 16)
    {
        // Send the bit to the queue
        if (is_valid_manchester(received_buffer) == 0)
        {
            DEBUG_PRINT("Invalid Manchester encoding\n");

            received_buffer_index = 0;
            memset(received_buffer, 0, 16);

            received_decoded_buffer_index = 0;
            memset(received_decoded_buffer, 0, 1024);

            fsa_accepted = 0;
            reset_fsa();
            return;
        }
        uint8_t decoded_byte = decode_manchester_byte(received_buffer);

        //uart_write_bytes(UART_NUM_0, &decoded_byte, 1);
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(timer_data_queue, &decoded_byte, &xHigherPriorityTaskWoken);

        DEBUG_PRINT("Decoded byte: %d\n", decoded_byte);

        received_buffer_index = 0;
        memset(received_buffer, 0, 16);
    }
    // DEBUG_PRINT("E\n");
}

void app_main(void)
{
    esp_task_wdt_deinit();

    setup();

    log(INFORMATION, "Setup complete");

    // Create the queue for transferring data from ISR
    timer_data_queue = xQueueCreate(50000, sizeof(uint8_t));
    if (timer_data_queue == NULL)
    {
        log(FAILURE, "Failed to create queue");
        return;
    }

    // Create tasks on separate cores
    xTaskCreatePinnedToCore(isr_process, "handle_reading_logic", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(timer_data_process, "timer_data_process", 4096, NULL, 5, NULL, 1);
}

void setup()
{

    setup_read_handler();
    setup_light_handler();
}

void isr_process()
{

    // Create and start the periodic timer
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &timer_isr,
        .name = "periodic_timer",

    };
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 3333)); // 50 microseconds

    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
