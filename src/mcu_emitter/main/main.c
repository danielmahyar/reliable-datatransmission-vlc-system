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

    /*     while (1) {
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
    xTaskCreatePinnedToCore(sender_process, "handle_sending_logic", 4096, NULL, 5, NULL, 0);
/*     xTaskCreatePinnedToCore(send_health_process, "handle_send_health", 4096, NULL, 5, NULL, 1);
 */}

void setup()
{
    // Configure UART parameters
/*     uart_config_t uart_config = {
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

void sender_process()
{
    uint8_t data[BYTE_MANCHESTER_SIZE] = {0};

    encode_manchester_byte(data, SYNC_BYTE);
    // Send sync continuously
    while (1)
    {

        for (uint8_t j = 0; j < BYTE_MANCHESTER_SIZE; j++)
        {
            printf("%d", data[j]);
            set_light_state(data[j]);
            vTaskDelay((SAMPLING_RATE) / portTICK_PERIOD_MS);
        }
        printf("\n");
    }

    while (1)
    {
        log(SUCCESS, "Enter message to send: ");
        // uint8_t status = get_line_input(message_buffer, MESSAGE_SIZE, &g_is_receiving);
        log(SUCCESS, "Sending message: %s", message_buffer);


        for (uint8_t i = 0; i < MESSAGE_SIZE; i++)
        {
            encode_manchester_byte(data, message_buffer[i]);
            log(INFORMATION, "Sending character: %c (%d)", message_buffer[i], message_buffer[i]);
            for (uint8_t j = 0; j < BYTE_MANCHESTER_SIZE; j++)
            {
                set_light_state(data[j]);
                vTaskDelay((SAMPLING_RATE) / portTICK_PERIOD_MS);
            }
        }

    }
}
