/** \file
 * \brief CRC32 reference implementation by Wikipedia with execution performance profiling
 */
#include "clib/crc32.h"

static uint32_t crc_table[256];
 
void clib_crc32_make_crc_table(void) {
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

uint32_t clib_crc32(const uint8_t *buf, size_t len){
	static int init = 0;
	if(!init)
		clib_crc32_make_crc_table();
	return clib_crc32_direct(buf, len);
}
 
uint32_t clib_crc32_direct(const uint8_t *buf, size_t len) {
    uint32_t c = 0xFFFFFFFF;
    size_t i = 0;
    for (; i < len; i++) {
        c = crc_table[(c ^ buf[i]) & 0xFF] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFF;
}

const uint32_t *clib_crc32_get_crc_table(void){
	return crc_table;
}
