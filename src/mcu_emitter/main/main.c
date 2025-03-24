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
#include "esp_timer.h"
#include "esp_task_wdt.h"


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

char message_buffer[MESSAGE_SIZE] = "VLC MESSAGE :)";
char received_message_buffer[1024] = {0};
uint32_t received_message_length = 0;
const uint8_t ACK_BYTE = 0x7E;

SemaphoreHandle_t xMutexBuffer;
SemaphoreHandle_t xMutexLength;

esp_timer_handle_t periodic_timer;

uint8_t buffer_index = 0;
uint8_t buffer[] = {1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0};
void IRAM_ATTR timer_isr(void* arg)
{
    set_light_state(buffer[buffer_index]);
    buffer_index++;
    if (buffer_index == 16)
    {
        buffer_index = 0;
    }
}

void setup();
void send_health_process();
void sender_process();
void read_process();


void app_main(void)
{
    esp_task_wdt_deinit();
    
    setup();
    log(INFORMATION, "Setup complete");

    xMutexBuffer = xSemaphoreCreateMutex();
    xMutexLength = xSemaphoreCreateMutex();
    if (xMutexBuffer == NULL || xMutexLength == NULL)
    {
        log(FAILURE, "Mutex creation failed");
        return;
    }

    // Create and start the periodic timer
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &timer_isr,
        .name = "periodic_timer",

        
    };
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000)); // 50 microseconds

    while(1){
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    // xTaskCreatePinnedToCore(sender_process, "handle_sending_logic", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(read_process, "handle_reading_logic", 4096, NULL, 5, NULL, 1);
/*     xTaskCreatePinnedToCore(send_health_process, "handle_send_health", 4096, NULL, 5, NULL, 1);
 */}

void setup()
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

    setup_read_handler();
    setup_light_handler();
}

void send_health_process()
{
    while (1)
    {
        vTaskDelay(PING_DELAY / portTICK_PERIOD_MS);
    }
}

void sender_process(void *param)
{

    uint8_t data[BYTE_MANCHESTER_SIZE] = {0};

    // Send sync continuously
    while (1)
    {
        xSemaphoreTake(xMutexLength, portMAX_DELAY);
        uint32_t length = received_message_length;
        xSemaphoreGive(xMutexLength);

        log(INFORMATION, "Received message length is %d", length);
        if (length > 0)
        {
            for (uint8_t i = 0; i < length; i++)
            {
                xSemaphoreTake(xMutexBuffer, portMAX_DELAY);
                char byte = received_message_buffer[i];
                xSemaphoreGive(xMutexBuffer);

                encode_manchester_byte(data, byte);
                // Send received message to the light
                log(SUCCESS, "Sending message: %d", byte);
                for (uint8_t j = 0; j < BYTE_MANCHESTER_SIZE; j++)
                {
                    set_light_state(data[j]);
                    vTaskDelay((SAMPLING_RATE) / portTICK_PERIOD_MS);
                }
            }

            xSemaphoreTake(xMutexLength, portMAX_DELAY);
            received_message_length = 0;
            xSemaphoreGive(xMutexLength);
            log(SUCCESS, "ACK");
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
            printf("\n");
        }
    }
}

void read_process(void *param)
{
    // Read from uart
    while (1)
    {

        uint32_t len = uart_read_bytes(UART_NUM, received_message_buffer, BUF_SIZE, 100 / portTICK_PERIOD_MS);
        if (len > 0)
        {
            xSemaphoreTake(xMutexBuffer, portMAX_DELAY);
            xSemaphoreTake(xMutexLength, portMAX_DELAY);
            received_message_length = len;
            xSemaphoreGive(xMutexLength);
            xSemaphoreGive(xMutexBuffer);

            log(SUCCESS, "Received %d bytes", len);
        }
    }
}