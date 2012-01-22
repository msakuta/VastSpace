#ifndef DRAW_EFFECTS_H
#define DRAW_EFFECTS_H
#include "export.h"
#include "tent3d.h"

// Damn silly Windows needs windows.h to be included before gl/gl.h.
#ifdef _WIN32
typedef unsigned int GLuint;
#else
#include <GL/gl.h>
#endif

EXPORT void sparkdraw(const struct tent3d_line_callback *p, const struct tent3d_line_drawdata *dd, void *private_data);
EXPORT void sparkspritedraw(const tent3d_line_callback *p, const tent3d_line_drawdata *dd, void *private_data);

EXPORT void smokedraw(const struct tent3d_line_callback *p, const struct tent3d_line_drawdata *dd, void *private_data);

struct smokedraw_swirl_data{
	COLOR32 col;
	bool expand;
};

EXPORT void smokedraw_swirl(const struct tent3d_line_callback *p, const struct tent3d_line_drawdata *dd, void *private_data);

EXPORT void firesmokedraw(const struct tent3d_line_callback *p, const struct tent3d_line_drawdata *dd, void *private_data);

EXPORT void debrigib(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv);

EXPORT void debrigib_multi(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv);

EXPORT GLuint muzzle_texture();

EXPORT void gldScrollTextureBeam(const Vec3d &view, const Vec3d &start, const Vec3d &end, double width, double offset);

#endif
