/** \file
 * \brief CRC32 reference implementation by Wikipedia with execution performance profiling
 */
#include "clib/crc32.h"

uint32_t crc_table[256];
 
void make_crc_table(void) {
	uint32_t i = 0;
    for (; i < 256; i++) {
        uint32_t c = i;
        int j = 0;
        for (; j < 8; j++) {
            c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
        }
        crc_table[i] = c;
    }
}
 
uint32_t crc32(const uint8_t *buf, size_t len) {
    uint32_t c = 0xFFFFFFFF;
    size_t i = 0;
    for (; i < len; i++) {
        c = crc_table[(c ^ buf[i]) & 0xFF] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFF;
}

const uint32_t *get_crc_table(void){
	return crc_table;
}
