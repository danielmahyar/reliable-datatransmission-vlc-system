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
#define BUF_SIZE (1024)
#define SHARED_ARRAY_SIZE 100
#define BUFFER_SIZE 1024
#define MAX_SPACING 3 // Maximum spacing allowed between bits in "01" or "10" patterns

typedef enum
{
    UNDEFINED,
    RECEIVING,
    SENDING
} NodeState;

NodeState node_state = UNDEFINED;

ReceiverState receiver_state = NOT_SYNCED;
SenderState sender_state = NOT_SENDING;

esp_timer_handle_t periodic_timer;

void setup();
void send_health_process();
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
FSAState fsa_state = STATE_START;

void reset_fsa()
{
    printf("Resetting FSA\n");
    fsa_state = STATE_START;
    received_buffer_index = 0;
    memset(received_buffer, 0, 16);
}

void process_fsa(uint8_t bit)
{
    switch (fsa_state)
    {
    case STATE_START:
        if (bit == 1)
            fsa_state = STATE_1;
        break;
    case STATE_1:
        if (bit == 0)
            fsa_state = STATE_10;
        else
            reset_fsa();
        break;
    case STATE_10:
        if (bit == 0)
            fsa_state = STATE_100;
        else
            reset_fsa();
        break;
    case STATE_100:
        if (bit == 1)
            fsa_state = STATE_1001;
        else
            reset_fsa();
        break;
    case STATE_1001:
        if (bit == 0)
            fsa_state = STATE_10010;
        else
            reset_fsa();
        break;
    case STATE_10010:
        if (bit == 1)
            fsa_state = STATE_100101;
        else
            reset_fsa();
        break;
    case STATE_100101:
        if (bit == 0)
            fsa_state = STATE_1001010;
        else
            reset_fsa();
        break;
    case STATE_1001010:
        if (bit == 1)
            fsa_state = STATE_10010101;
        else
            reset_fsa();
        break;
    case STATE_10010101:
        if (bit == 0)
            fsa_state = STATE_100101010;
        else
            reset_fsa();
        break;
    case STATE_100101010:
        if (bit == 1)
            fsa_state = STATE_1001010101;
        else
            reset_fsa();
        break;
    case STATE_1001010101:
        if (bit == 0)
            fsa_state = STATE_10010101010;
        else
            reset_fsa();
        break;
    case STATE_10010101010:
        if (bit == 1)
            fsa_state = STATE_100101010101;
        else
            reset_fsa();
        break;
    case STATE_100101010101:
        if (bit == 0)
            fsa_state = STATE_1001010101010;
        else
            reset_fsa();
        break;
    case STATE_1001010101010:
        if (bit == 1)
            fsa_state = STATE_10010101010101;
        else
            reset_fsa();
        break;
    case STATE_10010101010101:
        if (bit == 1)
            fsa_state = STATE_100101010101011;
        else
            reset_fsa();
        break;

    case STATE_100101010101011:
        if (bit == 0)
            fsa_state = STATE_END;
        else
            reset_fsa();
        break;
    case STATE_END:
        // Already in the end state, do nothing
        break;
    }
}

QueueHandle_t timer_data_queue; // Queue to transfer data from ISR to thread

void timer_data_process()
{
    // Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = 115200,
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
            // Process the received bit
            printf("Processing bit: %d\n", received_bit);
            // Add further processing logic here if needed
        }
    }
}



void IRAM_ATTR timer_isr(void *arg)
{
    uint8_t current_bit = read_env_bit();

    // First check if the bit is stable or not

    uint8_t res = process_stable_bits(current_bit);

     if (res == 0)
    {
        return;
    } 

        // Else consider it a valid manchester bit
        received_buffer[received_buffer_index] = current_bit;
        received_buffer_index++;

    //printf("%d", current_bit);
    // Process the bit through the FSA
    process_fsa(current_bit);



    if (fsa_state == STATE_END)
    {
        for(int i = 0; i < 16; i++){
            printf("%d", received_buffer[i]);
        }
        printf("\n");
        printf("Got to the end of FSA\n");
        reset_fsa();
        // Send the bit to the queue
/*         BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(timer_data_queue, &current_bit, &xHigherPriorityTaskWoken);
        reset_fsa(); */
        
    }

    if (received_buffer_index == 16)
    {
        // Decode the buffer
        //printf("\n");

/*         uint8_t decoded_byte = decode_manchester_byte(received_buffer);
        if(decoded_byte == 0x7E){
            printf("Decoded byte: %d\n", decoded_byte);
            printf("End of message\n");
        }
        received_decoded_buffer[received_buffer_index] = decoded_byte;*/
        received_buffer_index = 0;
        memset(received_buffer, 0, 16); 
    }
}

void app_main(void)
{
    esp_task_wdt_deinit();

    setup();

    log(INFORMATION, "Setup complete");

    // Create the queue for transferring data from ISR
    timer_data_queue = xQueueCreate(1000, sizeof(uint8_t));
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

void send_health_process()
{
    while (1)
    {
        printf("Sender;%d;%d\n", NODE_ID, sender_state);
        printf("Receiver;%d;%d\n", NODE_ID, receiver_state);
        vTaskDelay(PING_DELAY / portTICK_PERIOD_MS);
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
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 250)); // 50 microseconds

    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// uint8_t current_bit = read_env_bit();

/*     printf("%d", current_bit);
    bit_read_count++;
    if (bit_read_count == 16)
    {
        printf("\n");
        bit_read_count = 0;
    } */

// Check if the current bit matches the stable bit
/* if (current_bit == stable_bit)
{
    stable_count++;
}
else
{
    // If the bit changes, reset the stable count
    stable_bit = current_bit;
    stable_count = 1;
}

// If the stable count reaches the threshold (3 readings), process the bit
if (stable_count >= 3)
{
    // Process the bit through the FSA
    process_fsa(current_bit);

    // Only add the bit to the buffer if it follows the FSA
    if (fsa_state != STATE_START && fsa_state != STATE_END)
    {
        printf("%d", current_bit);
        temp_buffer[bit_count] = stable_bit;
        bit_count++;
    }



    // Check if the FSA reached the end state
    if (fsa_state == STATE_END)
    {
        // log(SUCCESS, "Pattern 100101010101010110 detected!");

        reset_fsa(); // Reset the FSA for the next detection
    }

    stable_count = 0; // Reset stable count for the next bit
}

// Process when 16 bits are collected
if (bit_count == 16)
{
    uint8_t decoded_byte = decode_manchester_byte(temp_buffer);
    printf("\n");
    bit_count = 0;
    memset(temp_buffer, 0, 16);
} */

/*
void receiver_process()
{
    char message_from_sender[100] = {0};
    int message_index = 0;

    uint8_t manchester_encoded[BYTE_MANCHESTER_SIZE] = {0};

    uint8_t has_synced = 0;
    uint8_t first_byte = 0;
    uint8_t message_size = 0;

    while (1)
    {
        memset(manchester_encoded, 0, 16);
        for (int i = 0; i < BYTE_MANCHESTER_SIZE; i++)
        {
            manchester_encoded[i] = read_env_bit();
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
        if (has_synced == 1)
        {
            if (decoded_byte == 0x7E)
            {
                // Send buffer over UART
                uart_write_bytes(UART_NUM, uart_buffer, buffer_index);
                buffer_index = 0;                    // Reset buffer index
                memset(uart_buffer, 0, BUFFER_SIZE); // Clear buffer
            }
            else
            {
                // Add decoded byte to buffer
                if (buffer_index < BUFFER_SIZE)
                {
                    uart_buffer[buffer_index] = decoded_byte;
                    buffer_index++;
                }
                else
                {
                    log(WARNING, "Buffer overflow");
                }
            }
            continue;
        }
        if (decoded_byte != SYNC_BYTE)
        {
            uint8_t is_valid = is_valid_manchester(manchester_encoded);
            // log(WARNING, "Not synced - %s valid manchester", is_valid ? "is" : "is not");

            if (is_valid == 0)
            {
                // log(INFORMATION, "Waiting for 25 ms");
                vTaskDelay(OFFSET_DELAY / portTICK_PERIOD_MS);
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
            has_synced = 1;
        }

                 // Assume message is being send
                if (first_byte == 0)
                {
                    log(INFORMATION, "Expecting message size");
                    first_byte = 1;
                    message_index = 0;
                    message_size = decoded_byte;
                    memset(message_from_sender, 0, MESSAGE_SIZE);
                }

                message_from_sender[message_index] = decoded_byte;
                log(INFORMATION, "Got char: %c. I expect %d characters more", decoded_byte, MESSAGE_SIZE - message_index);
                message_index++;

                if (message_index == message_size + 1)
                {
                    log(SUCCESS, "Message received: %s", message_from_sender);
                    message_index = 0;
                    has_synced = 0;
                }
    }
}
*/