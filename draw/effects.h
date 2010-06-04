#ifndef DRAW_EFFECTS_H
#define DRAW_EFFECTS_H
#include "../tent3d.h"

void smokedraw(const struct tent3d_line_callback *p, const struct tent3d_line_drawdata *dd, void *private_data);

void smokedraw_swirl(const struct tent3d_line_callback *p, const struct tent3d_line_drawdata *dd, void *private_data);

void debrigib(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv);

#endif
