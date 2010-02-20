#ifndef CLIB_SUFVBO_H
#define CLIB_SUFVBO_H
#include "sufdraw.h"

typedef struct VBO{
	suf_t *suf;
	unsigned buffers[4];
	int np;
	unsigned short *indices;
} VBO;

struct VBO *CacheVBO(suf_t *suf);
void DrawVBO(const VBO *vbo, unsigned long flags, suftex_t *suft);

#endif
