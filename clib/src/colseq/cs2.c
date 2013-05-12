#include "clib/colseq/cs2.h"
#include <stddef.h>
#include <stdlib.h>

#define CS2X ColorSequence2X
#define CS2NODE ColorSequence2Node

/* probably it's better not to try understanding this macro */
#define COLOR32MIX(c1,c2,f) ((COLOR32)(((c1)>>24)*(f)+((c2)>>24)*(1-(f)))<<24|(COLOR32)(((c1)>>16&0xff)*(f)+((c2)>>16&0xff)*(1-(f)))<<16|(COLOR32)(((c1)>>8&0xff)*(f)+((c2)>>8&0xff)*(1-(f)))<<8|(COLOR32)(((c1)&0xff)*(f)+((c2)&0xff)*(1-(f))))
/* wonderous
#define COLOR32MIX4(c1,c2,c3,c4,f12,f34) ((COLOR32)(((c1)>>24)*(f12)+((c2)>>24)*(1-(f12))+((c3)>>24)*(f34)+((c4)>>24)*(1-(f34))/2)<<24|\
	(COLOR32)(((c1)>>16)*(f12)+((c2)>>16)*(1-(f12))+((c3)>>16)*(f34)+((c4)>>16)*(1-(f34))/2)<<16|\
	(COLOR32)(((c1)>>8)*(f12)+((c2)>>8)*(1-(f12))+((c3)>>8)*(f34)+((c4)>>8)*(1-(f34))/2)<<8|\
	(COLOR32)((c1)*(f12)+(c2)*(1-(f12))+(c3)*(f34)+(c4)*(1-(f34))/2)) */
#define COLOR32MIX4(c1,c2,c3,c4,f1,f2) ((COLOR32)((((c1)>>24)*(f1)+((c2)>>24)*(1-(f1)))*(f2)+(((c3)>>24)*(f1)+((c4)>>24)*(1-(f1)))*(1-(f2)))<<24|\
	(COLOR32)((((c1)>>16&0xff)*(f1)+((c2)>>16&0xff)*(1-(f1)))*(f2)+(((c3)>>16&0xff)*(f1)+((c4)>>16&0xff)*(1-(f1)))*(1-(f2)))<<16|\
	(COLOR32)((((c1)>>8&0xff)*(f1)+((c2)>>8&0xff)*(1-(f1)))*(f2)+(((c3)>>8&0xff)*(f1)+((c4)>>8&0xff)*(1-(f1)))*(1-(f2)))<<8|\
	(COLOR32)((((c1)&0xff)*(f1)+((c2)&0xff)*(1-(f1)))*(f2)+(((c3)&0xff)*(f1)+((c4)&0xff)*(1-(f1)))*(1-(f2))))

/* retrieve 4 component 8 bit encoded color from 2D color map, with interpolated values */
COLOR32 ColorSequence2(const struct cs2 *cs, double x, double y){
	double ptx;
	int ix, iy;
	struct cs2_x *xn = CS2X(cs, 0);
	size_t sx = offsetof(struct cs2_x, y) + cs->y_nodes * sizeof(struct cs2_y);
	for(ix = 0, ptx = x; ix < cs->x_nodes && 0 < x; ptx = x, x -= (xn = CS2X(cs, ix))->dx, ix++);
	if(ix == cs->x_nodes) return 0;
	ix = 0 < ix ? ix-1 : 0;
	{
		struct cs2_y black = {0, 0};
		struct cs2_y *y00, *y01, *y10, *y11;
		double ty0, ty1, Y0, Y1;

		/* this loop is repeated at least a time with an exception of cs->y_nodes being not positive,
		  so don't worry about the pointers to be unassigned. */
		for(iy = 0, ty0 = ty1 = 0; iy < cs->y_nodes; iy++){
			y00 = CS2NODE(cs, ix, iy), y01 = iy == cs->y_nodes - 1 ? &black : CS2NODE(cs, ix, iy+1),
				y10 = ix == cs->x_nodes - 1 ? &black : CS2NODE(cs, ix + 1, iy),
				y11 = ix == cs->x_nodes - 1 || iy == cs->y_nodes - 1 ? &black : CS2NODE(cs, ix+1, iy+1);
			Y0 = y - ((ty1 - ty0) / xn->dx * ptx + ty0);
			ty0 += y00->dy;
			ty1 += y10->dy;
			Y1 = ((ty1 - ty0) / xn->dx * ptx + ty0) - y;
			if(0 <= Y1){
				break;
			}
		}
		if(iy == cs->y_nodes) return 255;
		iy = 0 < iy ? iy-1 : 0;
		{
			double fx = ptx / xn->dx, fy = Y0 / (Y0 + Y1);
	/*		COLOR32 c0 = COLOR32MIX(y10->c, y00->c, fx), c1 = COLOR32MIX(y11->c, y01->c, fx);
	/		return fy <= 1 ? COLOR32MIX(c1, c0, fy) : 0;*/
			return fy <= 1 ? COLOR32MIX4(y11->c, y01->c, y10->c, y00->c, fx, fy) : 0;
		}
	}
}

/* storage allocator and initializer, call it to create dynamically defined color sequences. */
struct cs2 *NewColorSequence2(int ix, int iy){
	struct cs2 *ret;
	ret = malloc((ix) * (offsetof(struct cs2_x, y) + iy * sizeof(struct cs2_y)));
	ret->x_nodes = ix;
	ret->y_nodes = iy;
	return ret;
}

void DelColorSequence2(struct cs2 *cs){
	free(cs);
}
