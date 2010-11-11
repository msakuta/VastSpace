#ifndef SUFBIN_H
#define SUFBIN_H
#include "suf.h"

size_t DislocateSUF(void **buf, const suf_t *src);
suf_t *RelocateSUF(suf_t *src);

#define LZUC(lzw) RelocateSUF(lzuc(lzw, sizeof(lzw), NULL))

// In 64-bit CPU, dislocation and relocation do not work to serialize
// or unserialize a data.
suf_t *SerializeSUF(const void *src, size_t size);
suf_t *UnserializeSUF(const void *src, size_t size);

#endif
