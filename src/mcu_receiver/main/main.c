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

#define UART_NUM UART_NUM_0 // Use UART1
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define BUF_SIZE (1024)

/*
 *  sync(1) -> Node 2 (assuming node 2 is not trying to send anything at the moment)
 *  Node 2 receives sync(1)
 *  Node 2 must first sync with sender (to get same timing)
 *  Node 1 receives sync(1)
 *  Node 1 sends ACK(1) -> Node 2
 *  Node 2 receives ACK(1)
 *  Messages can start flowing from Node 1 to Node 2
 *
 */

typedef enum
{
    UNDEFINED,
    RECEIVING,
    SENDING
} NodeState;

NodeState node_state = UNDEFINED;

ReceiverState receiver_state = NOT_SYNCED;
SenderState sender_state = NOT_SENDING;

char message_buffer[MESSAGE_SIZE] = "VLC MESSAGE :)";
uint8_t g_synced = 0;
uint8_t g_got_ack = 0;
uint8_t g_is_receiving = 0;

uint8_t g_sync_received = 0;

uint8_t g_me_to_them_synced = 0;
uint8_t g_them_to_me_synced = 0;

void setup();
void send_health_process();
void sender_process();
void receiver_process();

void app_main(void)
{
    setup();
    log(INFORMATION, "Setup complete");

    uint8_t manchester_encoded[BYTE_MANCHESTER_SIZE] = {0};
/*
         while (1) {
            const uint8_t flag = 0x7EEE;
            uint8_t data[BUF_SIZE];
            uint32_t len = uart_read_bytes(UART_NUM, data, BUF_SIZE, 100 / portTICK_PERIOD_MS);
            if (len > 0) {
                // Echo the received data back
                set_led(1);
                // log(INFORMATION, "Received %d bytes", len);
                uart_write_bytes(UART_NUM, (const char*) data, len);
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
            set_led(0);
        } */
    xTaskCreatePinnedToCore(receiver_process, "handle_reading_logic", 4096, NULL, 5, NULL, 0);
    /* xTaskCreatePinnedToCore(send_health_process, "handle_send_health", 4096, NULL, 5, NULL, 1); */
}

void setup()
{
    /*         // Configure UART parameters
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
            uart_driver_install(UART_NUM, BUF_SIZE * 2, BUF_SIZE, 0, NULL, 0); */

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
            printf("%d", manchester_encoded[i]);
            vTaskDelay(SAMPLING_RATE / portTICK_PERIOD_MS);
        }
        printf("\n");

        uint8_t decoded_byte = decode_manchester_byte(manchester_encoded);
        // print binary representation of decoded byte
        char binary[BYTE_SIZE];
        for (uint8_t i = 0; i < 8; i++)
        {
            binary[i] = (decoded_byte >> i) & 1;
        }
        log(INFORMATION, "Decoded byte: %d - Binary: %d%d%d%d%d%d%d%d", decoded_byte, binary[7], binary[6], binary[5], binary[4], binary[3], binary[2], binary[1], binary[0]);
        if(has_synced == 1)  {
            log(INFORMATION, "Synced");
            continue;
        }
        if (decoded_byte != SYNC_BYTE)
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
            has_synced = 1;
            
        }

        log(INFORMATION, "Decoded byte: %u", decoded_byte);

/*         // Assume message is being send
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
        } */

    }
}
