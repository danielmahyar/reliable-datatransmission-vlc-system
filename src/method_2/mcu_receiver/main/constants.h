#pragma once

// Hardware constants
#define LED_GPIO 12
#define THRESHOLD 3800 
#define SAMPLING_RATE 333
#define DEBUG 1  // Enable debug mode

#define UART_BAUD_RATE 1000000
#define UART_NUM UART_NUM_0 // Use UART1
#define BUF_SIZE (8192)
#define UART_RX_BUFFER_SIZE BUF_SIZE * 2 // Increase RX buffer size
#define UART_TX_BUFFER_SIZE BUF_SIZE * 2 // Increase TX buffer size
#define RX_BUFFER_SIZE 8 // Define UART_NUM_0 for UART1

// Protocol constants
#define BYTE_SIZE 8
#define BYTE_MANCHESTER_SIZE 16
#define SYNC_BYTE 0x7E