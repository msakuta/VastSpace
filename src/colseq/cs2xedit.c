#include "clib/colseq/cs2x.h"
#include <stdlib.h>

int ColorSequence2xAddVertex(struct cs2x *cs, double x, double y, COLOR32 col){
	struct cs2x_vertex *ov = cs->v;
	cs->v = (struct cs2x_vertex*)realloc(cs->v, (cs->nv+1) * sizeof(struct cs2x_vertex));
	cs->v[cs->nv].c = cs->dc;
	cs->v[cs->nv].x = x;
	cs->v[cs->nv].y = y;
	cs->nv++;

	/* manual relocations */
	if(ov != cs->v){
		int i;
		for(i = 0; i < cs->nt; i++){
			cs->t[i].v[0] += cs->v - ov;
			cs->t[i].v[1] += cs->v - ov;
			cs->t[i].v[2] += cs->v - ov;
		}
	}
}

int ColorSequence2xAddTri(struct cs2x *cs, int v0, int v1, int v2){
}
