#include "clib/colseq/cs.h"
/*#include <windows.h>*/
#include <assert.h>
#include <stdlib.h> /* rand?? */

typedef unsigned char byte;
typedef struct color_node colnode_t;
typedef struct color_sequence colseq_t;

/* retrieve color from a color sequence at a specified elapsed time. */
COLOR32 ColorSequence(const struct color_sequence *cs, double time){
	unsigned j;
	COLOR32 dstc;
	byte srcc[4];
	assert(cs->c != 0);
	if(time < 0 || cs->t < time) return cs->dc;
	for(j = 0; j < cs->c && time >= 0; j++) time -= cs->p[j].t;
	j--;
	/* by default, disappear into black. This behavior can be changed by appending 0
	  timed color node at the end of the sequence. */
	dstc = j == cs->c-1 ? cs->dc : cs->p[j+1].col;
	time = cs->p[j].t ? (time + cs->p[j].t) / cs->p[j].t : 0.5;
	if(time > 1.) time = 1.;
	if(cs->flags & COLSEQ_RANDOM && cs->p[j].col == cs->colrand)
		srcc[0] = (byte)rand(), srcc[1] = (byte)rand() & 0xff, srcc[2] = (byte)rand() & 0xff, srcc[3] = 0xff;
	else
		srcc[0] = COLOR32R(cs->p[j].col),
		srcc[1] = COLOR32G(cs->p[j].col),
		srcc[2] = COLOR32B(cs->p[j].col),
		srcc[3] = COLOR32A(cs->p[j].col);
	return COLOR32RGBA((srcc[0] * (1.-time) + COLOR32R(dstc) * time),
			(srcc[1] * (1.-time) + COLOR32G(dstc) * time),
			(srcc[2] * (1.-time) + COLOR32B(dstc) * time),
			(srcc[3] * (1.-time) + COLOR32A(dstc) * time));
}
