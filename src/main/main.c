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

ReceiverState receiver_state = NOT_SYNCED;
SenderState sender_state = NOT_SENDING;
char message_buffer[MESSAGE_SIZE] = "VLC MESSAGE :)";

void setup();
void send_health_process();
void sender_process();
void receiver_process();


void app_main(void)
{
    setup();
    log(INFORMATION, "Setup complete");

    xTaskCreate(receiver_process, "handle_reading_logic", 4096, NULL, 5, NULL);
    xTaskCreate(sender_process, "handle_sending_logic", 4096, NULL, 5, NULL);
    xTaskCreate(send_health_process, "handle_send_health", 4096, NULL, 5, NULL);
}

void setup(){
    setup_read_handler();
    setup_light_handler();
}

void send_health_process(){
    while(1) {
        printf("Sender;%d;%d\n", NODE_ID, sender_state);
        printf("Receiver;%d;%d\n", NODE_ID, receiver_state);
        vTaskDelay(PING_DELAY / portTICK_PERIOD_MS);
    }
}

void sender_process(){
    uint8_t data[BYTE_MANCHESTER_SIZE] = {0};
    uint8_t has_synced = 0;
    uint8_t has_user_input = 0;

    while (1)
    {
        if (has_user_input == 0)
        {
            log(SUCCESS, "Enter message to send: ");
            get_line_input(message_buffer, MESSAGE_SIZE);

            log(SUCCESS, "Sending message: %s", message_buffer);

            has_user_input = 1;
        }
        if (has_synced == 0)
        {
            sender_state = SENDING_SYNC;
            log(INFORMATION, "Sending sync bits [0111110]");
            encode_manchester_byte(data, SYNC_BYTE);

            // TODO: Should wait for ACK from receiver and not use SYNC_REPEAT
            for (int j = 0; j < SYNC_REPEAT; j++)
            {
                for (uint8_t i = 0; i < BYTE_MANCHESTER_SIZE; i++)
                {
                    set_light_state(data[i]);
                    vTaskDelay(SAMPLING_RATE / portTICK_PERIOD_MS);
                }
            }
            has_synced = 1;
            log(INFORMATION, "Done sending sync bits");
        }

        sender_state = SENDING_MESSAGE;
        for (uint8_t i = 0; i < MESSAGE_SIZE; i++)
        {
            encode_manchester_byte(data, message_buffer[i]);
            log(INFORMATION, "Sending character: %c (%d)", message_buffer[i], message_buffer[i]);
            for (uint8_t j = 0; j < BYTE_MANCHESTER_SIZE; j++)
            {
                gpio_set_level(LED_GPIO, data[j]);
                vTaskDelay(SAMPLING_RATE / portTICK_PERIOD_MS);
            }
        }
        log(INFORMATION, "Emitted full message. Resending");
        has_user_input = 0;
        has_synced = 0;
        sender_state = NOT_SENDING;
    }
}

void receiver_process(){
    char message_from_sender[MESSAGE_SIZE] = {0};
    int message_index = 0;

    uint8_t manchester_encoded[BYTE_MANCHESTER_SIZE] = {0};

    uint8_t has_synced = 0;

    while (1)
    {
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

        if (has_synced == 0)
        {
            receiver_state = TRYING_TO_SYNC;
            if (decoded_byte != 0x7E)
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
                receiver_state = SYNCED;
                has_synced = 1;
            }
        }

        log(INFORMATION, "Decoded byte: %u", decoded_byte);

        if (decoded_byte != 0x7E)
        {
            // Assume message is being send
            message_from_sender[message_index] = decoded_byte;
            log(INFORMATION, "Got char: %c. I expect %d characters more", decoded_byte, MESSAGE_SIZE - message_index);
            message_index++;
        }

        if (message_index == MESSAGE_SIZE)
        {
            log(SUCCESS, "Message received: %s", message_from_sender);
            message_index = 0;
            has_synced = 0;
        }
        memset(manchester_encoded, 0, 16);
    }
}
