#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// MTU = 65535 bytes
// ESP32 MAX RAM SIZE = 520000 bytes

typedef struct
{
    uint8_t SYNC;
    uint8_t ACK;
    uint8_t FIN;
    uint8_t RST;
    uint8_t PSH;
    uint8_t URG;
    uint8_t ECE;
    uint8_t NS;
    // 3 bits reserved
    uint8_t RES[3];
} TCPFlags;

// Do not use source and port
typedef struct
{
    uint32_t seq_num;
    uint32_t ack_num;
    TCPFlags flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_pointer;
} TCPHeader;

typedef struct {
    uint8_t *data;
    TCPHeader header;
} TCPFrame;

void construct_flags(TCPFlags *flags, uint8_t SYNC, uint8_t ACK, uint8_t FIN, uint8_t RST, uint8_t PSH, uint8_t URG, uint8_t ECE, uint8_t NS)
{
    flags->SYNC = SYNC;
    flags->ACK = ACK;
    flags->FIN = FIN;
    flags->RST = RST;
    flags->PSH = PSH;
    flags->URG = URG;
    flags->ECE = ECE;
    flags->NS = NS;
}

uint8_t get_flags(TCPFlags* flags)
{
    return (flags->SYNC << 7) | (flags->ACK << 6) | (flags->FIN << 5) | (flags->RST << 4) | (flags->PSH << 3) | (flags->URG << 2) | (flags->ECE << 1) | flags->NS;
}

void get_flags_from_byte(TCPFlags* flags, uint8_t byte)
{
    TCPFlags flags;
    flags->SYNC = (byte >> 7) & 1;
    flags->ACK = (byte >> 6) & 1;
    flags->FIN = (byte >> 5) & 1;
    flags->RST = (byte >> 4) & 1;
    flags->PSH = (byte >> 3) & 1;
    flags->URG = (byte >> 2) & 1;
    flags->ECE = (byte >> 1) & 1;
    flags->NS = byte & 1;
    return flags;
}

void construct_frame(uint8_t *frame, uint32_t seq_num, uint32_t ack_num, TCPFlags flags, uint16_t window_size, uint16_t checksum, uint16_t urgent_pointer, uint8_t *data, uint16_t data_len)
{
    TCPHeader header;
    header.seq_num = seq_num;
    header.ack_num = ack_num;
    header.flags = flags;
    header.window_size = window_size;
    header.checksum = checksum;
    header.urgent_pointer = urgent_pointer;

    memcpy(frame, &header, sizeof(TCPHeader));
    memcpy(frame + sizeof(TCPHeader), data, data_len);
}



int main()
{


    uint32_t frame_start = 0, frame_end = 0;
    uint8_t window_size = 4;

    FILE *fp = fopen("image.png", "r");

    

    
    if (fp == NULL)
    {
        printf("Error! opening file");
        exit(1);
    }

    return 0;
}