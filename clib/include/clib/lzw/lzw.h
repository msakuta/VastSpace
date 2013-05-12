#ifndef LZW_H
#define LZW_H
#include <stddef.h>

unsigned char *lzc(const unsigned char *in, size_t sz, size_t *retsz);
unsigned char *lzuc(const unsigned char *in, size_t sz, size_t *retsz);


#endif
