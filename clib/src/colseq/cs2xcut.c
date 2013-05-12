#include "clib/colseq/cs2xcut.h"
#include <stdlib.h>

#define MAX_CUTNODES 128

typedef struct cs2x_bbox bbox;
typedef struct cs2x_vertex vert;
typedef struct cs2x_triangle trit; /* triangle */
typedef double ortt[4]; /* convex quadrangle */
typedef struct cs2x cs2xt;

struct history{
	double y; /* intersection of cutline and the edge */
	double f; /* factor of point at edge */
	trit *t; /* cut triangle */
	int e; /* edge index of the triangle */
};

extern void ColorSequence2xNormalize(struct cs2x *); /* man! */

static int histcmp(const struct history *a, const struct history *b){
	if(b->y != a->y)
		return b->y < a->y;
	else{
		int c = b->t->v[(b->e+1)%3]->x - b->t->v[b->e]->x < a->t->v[(a->e+1)%3]->x - a->t->v[a->e]->x;
		return a->t != b->t ? c : !c;
	}
}

struct color_sequence *ColorSequence2xCut(struct cs2x *cs, double x, struct color_sequence *ret){
	struct history hist[MAX_CUTNODES];
	int i, n = 0, above = 0;
	struct color_node *nodes;
	if(!cs->init)
		ColorSequence2xNormalize(cs);
	for(i = 0; i < cs->nt; i++){
		int j;
		trit *t = &cs->t[i];

		/* triangles whose bounding box doesn't include the cutline can be discarded */
		if(x < t->bb.l || t->bb.r < x)
			continue;

		/* count intersecting triangle edges */
		for(j = 0; j < 3; j++){
			vert *v0 = t->v[j], *v1 = t->v[(j+1) % 3], *v0h, *v1h;
			double s;
			enum {left, right} slope;

			/* edges that are parallel to the cutline cannot calculate intersection */
			if(v1->x == v0->x)
				continue;

			/* keeping s value to be always measured from left to right in x axis makes
			  the y coordinate of intersecting point with the cutline calculate equivalent,
			  regardless of which direction the edge is. this effectively assures
			  consistency of adjacent but different colored triangles' order. */
			slope = v0->x < v1->x ? right : left;
			s = slope ? (x - v0->x) / (v1->x - v0->x) : (x - v1->x) / (v0->x - v1->x);

			/* skip duplicate color */
			if(0 <= s && s <= 1 && n < MAX_CUTNODES){
				int k;
				for(k = 0; k < n; k++){
					v0h = hist[k].t->v[hist[k].e];
					v1h = hist[k].t->v[(hist[k].e+1) % 3];
					if(v0h == v0 && v1h == v1 || v1h == v0 && v0h == v1)
						break;
				}
				if(k != n) continue;
				hist[n].t = t;
				hist[n].e = j;
				hist[n].y = slope ? s * (v1->y - v0->y) + v0->y : s * (v0->y - v1->y) + v1->y;
				hist[n].f = slope ? s : 1. - s;
				n++;
			}
		}
	}
	qsort(hist, n, sizeof *hist, histcmp);
	if(n == 0 || hist[0].y != 0){
		above = 1;
		n++;
	}
	ret = realloc(ret, sizeof(struct color_sequence) + n * sizeof(struct color_node));
	if(!ret)
		return NULL;
	if(above){
		nodes = &(ret->p = (struct color_node*)&ret[1])[1];
		nodes[-1].col = cs->dc;
		nodes[-1].t = n >= 2 ? hist[1].y : 1.;
	}
	else
		ret->p = nodes = (struct color_node*)&ret[1];
	ret->c = n;
	for(i = 0; i < n - above; i++){
/*		double f = (hist[i].y - hist[i].t->v[hist[i].e]->y) / (hist[i].t->v[(hist[i].e+1) % 3]->y - hist[i].t->v[hist[i].e]->y);*/
		COLOR32 *c1 = &hist[i].t->v[hist[i].e]->c, *c2 = &hist[i].t->v[(hist[i].e+1) % 3]->c;
		nodes[i].t = i != n-1 ? hist[i+1].y - hist[i].y : 0.;
		nodes[i].col = COLOR32MIX(*c1, *c2, hist[i].f);
	}
	ret->t = hist[i-1].y;
	ret->dc = cs->dc;
	return ret;
}
