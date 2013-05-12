#ifndef MASALIB_CS2XCUT_H
#define MASALIB_CS2XCUT_H
#include "cs.h"
#include "cs2x.h"

/* cut and make a new mono-directional color sequence on that line. 
  It's the caller's responsibility to free returned object. */
extern struct color_sequence *ColorSequence2xCut(struct cs2x *, double x, struct color_sequence *pass);


#endif
