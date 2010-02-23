#ifndef CLIB_SUFVBO_H
#define CLIB_SUFVBO_H
#include "sufdraw.h"

typedef struct VBO{
	suf_t *suf;
	unsigned (*atris)[4]; /* Attrubute Triangles */
	int *natris; /* Count of ... */
} VBO;

struct VBO *CacheVBO(suf_t *suf);
void DrawVBO(const VBO *vbo, unsigned long flags, suftex_t *suft);

#endif
