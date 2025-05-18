#pragma once

// Hardware constants
#define LED_GPIO 12
#define SAMPLING_RATE 10000
#define NODE_ID 1

#define UART_NUM UART_NUM_0 // Use UART1
#define UART_BAUD_RATE 1000000
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define BUF_SIZE (8192)
#define SEND_RANDOM 0

// Protocol constants
#define MESSAGE_SIZE 20
#define BYTE_SIZE 8
#define BYTE_MANCHESTER_SIZE 16
#define SYNC_BYTE 0x7E
#define SYNC_REPEAT 20
#define PING_DELAY 100