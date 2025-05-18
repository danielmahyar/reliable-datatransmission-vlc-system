#include <stdio.h>
#include <string.h>
#include "constants.h"
#include "freertos/FreeRTOS.h"
#include "esp_rom_spiflash.h"
#include "../components/logging/logging.h"
#include "../components/manchester_encoding/manchesterencoding.h"
#include "../api/read_handler/read_handler.h"
#include "../components/dfa/dfa.h"
#include "driver/uart.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "../components/protocol/protocol.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "esp_timer.h" // Include for timestamp functionality


#if DEBUG == 1
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

void setup();
// Task/process to setup the ISR on a specific core
void isr_process();

uint8_t received_buffer[BYTE_MANCHESTER_SIZE] = {0};
uint8_t received_buffer_index = 0;
FSAState fsa_state = STATE_START;

QueueHandle_t timer_data_queue; // Queue to transfer data from ISR to thread
SemaphoreHandle_t queue_length_semaphore;

esp_timer_handle_t periodic_timer;


void uart_process()
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
    uart_driver_install(UART_NUM, UART_RX_BUFFER_SIZE, UART_TX_BUFFER_SIZE, 5000, NULL, 1);

    uint8_t received_bit;
    while (1)
    {
        // Wait for data from the queue
        if (xQueueReceive(timer_data_queue, &received_bit, portMAX_DELAY))
        {
            // Process the received bit
            int bytes_written = uart_write_bytes(UART_NUM_0, &received_bit, 1);
            if (bytes_written < 0)
            {
                printf("UART write error: %d\n", bytes_written);
            }
        }
    }
}

uint8_t fsa_accepted = 0;
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
    // #if DEBUG == 1
    //     // Print the current bit
    //     printf("%d", current_bit);
    //     count++;
    //     if (count == BYTE_MANCHESTER_SIZE)
    //     {
    //         count = 0;
    //         printf("\n");
    //     }
    // #endif

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
    process_dfa(&fsa_state, received_buffer, &received_buffer_index, res);

    if (fsa_state == STATE_END && fsa_accepted == 0)
    {
        uint8_t decoded_bytee = decode_manchester_byte(received_buffer);
        // DEBUG_PRINT("Flag byte: %d\n", decoded_bytee);    
        fsa_accepted = 1;
        received_buffer_index = 0;
        memset(received_buffer, 0, BYTE_MANCHESTER_SIZE);
    }

    // Only reach this point if the DFA is in the end state
    if (received_buffer_index == BYTE_MANCHESTER_SIZE)
    {
        // Send the bit to the queue
        if (is_valid_manchester(received_buffer) == 0)
        {
            DEBUG_PRINT("Invalid Manchester encoding\n");

            received_buffer_index = 0;
            memset(received_buffer, 0, BYTE_MANCHESTER_SIZE);

            reset_fsa(&fsa_state, received_buffer, &received_buffer_index);

            return;
        } 
        uint8_t decoded_byte = decode_manchester_byte(received_buffer);

        // Fun experiment: Tried to send the decoded byte to UART, but it blocks the ISR which is not good
        // Therefore, it will mess up the timing of the ISR
        //uart_write_bytes(UART_NUM_0, &decoded_byte, 1);
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(timer_data_queue, &decoded_byte, &xHigherPriorityTaskWoken);

        DEBUG_PRINT("Decoded byte: %d\n", decoded_byte);

        received_buffer_index = 0;
        memset(received_buffer, 0, BYTE_MANCHESTER_SIZE);     

        // reset_fsa(&fsa_state, received_buffer, &received_buffer_index);
        return;
    } 
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

    // Create tasks for sending and reading and attach them to different cores
    // Core 0 is used for emitting and Core 1 is used for reading
    xTaskCreatePinnedToCore(isr_process, "handle_reading_logic", 4096, NULL,  configMAX_PRIORITIES - 1, NULL, 0);
    xTaskCreatePinnedToCore(uart_process, "uart_process", 4096, NULL, 5, NULL, 1);
}

void setup()
{

    setup_read_handler();
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