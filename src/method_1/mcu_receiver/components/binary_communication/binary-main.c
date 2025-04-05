#include <stdio.h>
#include <inttypes.h>

/*
Frame structure:
FLAG | TCP HEADER | DATA | FCS | FLAG | ...
where
FLAG = 01111110 = 0x7E
FCS  = 16-bit CRC (needs more thinking)
TCP HEADER = contains information about sequence (customized)

*/



int main(void) {

    // Bit stuffing: In case of 5 following "1"s we must provide a 0 at the end
    // 


    return 0;

}