#ifndef SUFBIN_H
#define SUFBIN_H
#include "suf.h"

size_t DislocateSUF(void **buf, const suf_t *src);
suf_t *RelocateSUF(suf_t *src);

#define LZUC(lzw) RelocateSUF(lzuc(lzw, sizeof(lzw), NULL))

#endif
