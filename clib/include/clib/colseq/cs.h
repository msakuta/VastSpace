#ifndef MASALIB_COLSEQ_H
#define MASALIB_COLSEQ_H
#include "color.h"

#define COLSEQ_ALPHA 1 /* presence of alpha channel in bit 24-31 */
#define COLSEQ_RANDOM 2

struct color_node{
	double t; /* delay time of this node */
	COLOR32 col;
};

struct color_sequence{
	COLOR32 dc; /* default color */
	unsigned c; /* count of color nodes */
	const struct color_node *p; /* must be in a static storage, or live long enough */
	COLOR32 colrand; /* this color is the key for random color, set -1 to disable randomness */
	double t; /* total live length, used for fast processing */
	int flags;
};

extern COLOR32 ColorSequence(const struct color_sequence *cs, double time);


#endif
