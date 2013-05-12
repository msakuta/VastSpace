#ifndef LIB_CS2X_H
#define LIB_CS2X_H

#include "color.h"
#include <stdio.h>

/* the object is unimportant; they exists for performance reason
  this identifier is only for human readability */
#define TRIVIAL

struct cs2x_bbox{
	double l, t, r, b;
};

/* vertex */
struct cs2x_vertex{
	double x, y;
	COLOR32 c;
};

struct cs2x_triangle{
	struct cs2x_vertex *v[3]; /* vertex list */
	TRIVIAL struct cs2x_bbox bb; /* bounding box */
	TRIVIAL int homo; /* homogeneousity of vertex colors */
}; /* triangle */

struct cs2x{
	COLOR32 dc; /* default color */
	long nv;
	struct cs2x_vertex *v;
	long nt;
	struct cs2x_triangle *t;
	TRIVIAL int init;
/*	long no;
	ortt *o;*/
};

/* gets */
extern COLOR32 ColorSequence2x(struct cs2x*, double x, double y);
extern struct cs2x *NewColorSequence2x(COLOR32 default_color);
extern void DelColorSequence2x(struct cs2x *); /* practically, it's nothing more than synonym to free() because all data is stored in united storage. */

/* file i/o */
extern int ColorSequence2xSaveText(const struct cs2x*, FILE*);
extern char *ColorSequence2xSaveTextMemory(const struct cs2x*);
extern struct cs2x *ColorSequence2xLoad(FILE*);
extern struct cs2x *ColorSequence2xLoadMemory(const char*);

/* edits */
extern int ColorSequence2xAddVertex(struct cs2x*, double x, double y, COLOR32);
extern int ColorSequence2xAddTri(struct cs2x*, int v0, int v1, int v2);

#endif
