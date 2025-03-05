#pragma once

// Hardware constants
#define LED_GPIO 12
#define THRESHOLD 4000
#define SAMPLING_RATE 300
#define NODE_ID 1

// Protocol constants
#define MESSAGE_SIZE 20
#define BYTE_SIZE 8
#define BYTE_MANCHESTER_SIZE 16
#define SYNC_BYTE 0x7E
#define SYNC_REPEAT 20
#define PING_DELAY 100