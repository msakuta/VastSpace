#ifndef DRAW_EFFECTS_H
#define DRAW_EFFECTS_H
#include "../tent3d.h"

// Damn silly Windows needs windows.h to be included before gl/gl.h.
#ifdef _WIN32
typedef unsigned int GLuint;
#else
#include <gl/gl.h>
#endif

void sparkdraw(const struct tent3d_line_callback *p, const struct tent3d_line_drawdata *dd, void *private_data);
void sparkspritedraw(const tent3d_line_callback *p, const tent3d_line_drawdata *dd, void *private_data);

void smokedraw(const struct tent3d_line_callback *p, const struct tent3d_line_drawdata *dd, void *private_data);

struct smokedraw_swirl_data{
	COLOR32 col;
	bool expand;
};

void smokedraw_swirl(const struct tent3d_line_callback *p, const struct tent3d_line_drawdata *dd, void *private_data);

void debrigib(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv);

void debrigib_multi(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv);

GLuint muzzle_texture();

#endif
