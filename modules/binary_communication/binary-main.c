#include <stdio.h>
#include <inttypes.h>

/*

Make a TCP kind of communication between two devices.
The flow would be:

1) Connect
2) Connect ACK 
3) Data transmission (Sliding Window and Stop-and-Wait)
4) Disconnect
5) Disconnect ACK


Frame structure:
FLAG | DATA | FCS | FLAG | ...

where
FLAG = 01111110 = 0x7E
FCS  = 16-bit CRC (needs more thinking)

*/

int sliding_window_start = 0;
int sliding_window_end = 0;
typedef enum
{
    CONNECT,
    CONNECT_ACK,
    ACK,
    DISCONNECT,
    DISCONNECT_ACK
} comm_state_t;

int main(void) {
    comm_state_t state = CONNECT;
    while (1) {
        switch (state) {
            case CONNECT:
                printf("CONNECT\n");
                state = CONNECT_ACK;
                break;
            case CONNECT_ACK:
                printf("CONNECT_ACK\n");
                state = ACK;
                break;
            case ACK:
                printf("ACK\n");
                break;
            case DISCONNECT:
                printf("DISCONNECT\n");
                state = DISCONNECT_ACK;
                break;
            case DISCONNECT_ACK:
                printf("DISCONNECT_ACK\n");
                state = CONNECT;
                break;
        }
    }
    return 0;

}