#ifndef LIB_CS2_H
#define LIB_CS2_H
/* Color Sequence 2 - Sequence of Color Sequence */

#include "color.h"
#include <stddef.h>

/* this structure is united in one memory storage thank struct hack technique,
  because it tend to be static data during application execution. */
struct cs2{
	int x_nodes, y_nodes;
	struct cs2_x{
		double dx;
		struct cs2_y{
			double dy;
			COLOR32 c;
		} y[1];
	} x[1]; /* variable-length */
};

extern COLOR32 ColorSequence2(const struct cs2*, double x, double y);
extern struct cs2 *NewColorSequence2(int x_nodes, int y_nodes);
extern void DelColorSequence2(struct cs2 *); /* practically, it's nothing more than synonym to free() because all data is stored in united storage. */

/* member referencing macros, probably it's easier to use if shortar aliases are defined
  such as CS2X or CS2NODE, for they are prefixed to keep name space clean. */
#define ColorSequence2X(cs,ix) ((struct cs2_x*)((char*)(cs)->x + \
	(ix) * (offsetof(struct cs2_x, y) + (cs)->y_nodes * sizeof(struct cs2_y))))
#define ColorSequence2Node(cs,ix,iy) ((struct cs2_y*)((char*)(cs)->x + \
	(ix) * (offsetof(struct cs2_x, y) + (cs)->y_nodes * sizeof(struct cs2_y)) + \
	offsetof(struct cs2_x, y) + (iy) * sizeof(struct cs2_y)))
#define ColorSequence2XofNode(cs,node) ((int)((char*)(node) - (char*)(cs)->x) / (offsetof(struct cs2_x, y) + (cs)->y_nodes * sizeof(struct cs2_y)))


/* use this macro to define pseudo-variable length structure to hardcode
  static color sequences. */
#define HACKED_CS2(ix,iy) struct{\
	int x_nodes, y_nodes;\
	struct hacked_cs2_x_##i{\
		double dx;\
		struct cs2_y y[iy];\
	} x[ix];\
}

#endif
