/* cs2x.c - Extension to Color Sequence in 2D */
#include "clib/colseq/cs2x.h"
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#define numof(a) (sizeof(a)/sizeof*(a))

typedef struct cs2x_bbox bbox;
typedef struct cs2x_vertex vert;
typedef struct cs2x_triangle trit; /* triangle */
typedef double ortt[4]; /* convex quadrangle */
typedef struct cs2x cs2xt;

void ColorSequence2xNormalize(cs2xt *cs){
	long i;
	if(cs->init) return;
	cs->init = 1;
	for(i = 0; i < cs->nt; i++){
		trit *t = &cs->t[i];
		vert *v0 = t->v[0], *v1, *v2;
		/* clockwise triangles must be reversed to ccw for just performance reasons,
		  which is enough. now get cross-(or vector-)product and see its sign. */
		if((t->v[1]->x - t->v[0]->x) * (t->v[2]->y - t->v[0]->y)
			- (t->v[1]->y - t->v[0]->y) * (t->v[2]->x - t->v[0]->x) < 0)
		{
			/* swapping v1 and v2 leads face direction to be reversed. */
			vert *tmp = t->v[1];
			t->v[1] = t->v[2];
			t->v[2] = tmp;
		}
		v1 = t->v[1];
		v2 = t->v[2];

		/* figure out bounding box. suspecious which is more efficient,
		  raw coding or iterations. */
		t->bb.l = v0->x;
		if(v1->x < t->bb.l) t->bb.l = v1->x;
		if(v2->x < t->bb.l) t->bb.l = v2->x;
		t->bb.t = v0->y;
		if(v1->y < t->bb.t) t->bb.t = v1->y;
		if(v2->y < t->bb.t) t->bb.t = v2->y;
		t->bb.r = v0->x;
		if(t->bb.r < v1->x) t->bb.r = v1->x;
		if(t->bb.r < v2->x) t->bb.r = v2->x;
		t->bb.b = v0->y;
		if(t->bb.b < v1->y) t->bb.b = v1->y;
		if(t->bb.b < v2->y) t->bb.b = v2->y;

		/* assume equivalence law is satisfied in color space. */
		t->homo = v0->c == v1->c && v1->c == v2->c;
	}
}
#define normalize ColorSequence2xNormalize

static int lefthand(const vert *v0, const vert *v1, double x, double y){
	return 0 < (v1->x - v0->x) * (y - v0->y) - (v1->y - v0->y) * (x - v0->x);
}

COLOR32 ColorSequence2x(cs2xt *cs, double x, double y){
	long i; /* triangle index */
	if(!cs->init)
		normalize(cs);
	for(i = 0; i < cs->nt; i++){
		trit *t = &cs->t[i];

		/* triangles whose bounding box doesn't include the point can be discarded */
		if(x < t->bb.l || t->bb.r < x || y < t->bb.t || t->bb.b < y)
			continue;

		/* if the point is looking in leftside from all edges, it's inside the region */
		if(lefthand(t->v[0], t->v[1], x, y)
			&& lefthand(t->v[1], t->v[2], x, y)
			&& lefthand(t->v[2], t->v[0], x, y))
		{
			/* as a special case, homogeneous triangle has constant color
			  over its area. */
			if(t->homo)
				return t->v[0]->c;

			{
			/* now we interpolate color at a point inside the triangle using
			  Gouraud Interpolation. */
			vert *v0 = cs->t->v[0], *v1 = cs->t->v[1], *v2 = cs->t->v[2];

#if 1 /* axe-cut */
			vert *vb = t->v[0], *vm, *vt;
#	if 0
			{ /* sort vertices in y, ascending */
				int j;
				if(t->v[1]->y < vb->y) vb = t->v[1];
				if(t->v[2]->y < vb->y) vb = t->v[2];
				for(vm = NULL, j = 0; j < 3; j++)
					if(vb != t->v[j] && (!vm || t->v[j]->y < vm->y)) vm = t->v[j];
				for(vt = NULL, j = 0; j < 3; j++)
					if(vb != t->v[j] && vm != t->v[j]) vt = t->v[j];
			}
#	endif

			/* divide vertices into groups whether over or under the horizontal scan line.
			  it came so that the order of vertices (i.e. direction of the face) is
			  unimportant.
			  btw, over means less y-coord value here, which doesn't affect the result. */
			{
				vert *up[2], *dn[2], **one, **two;
				int j, iup = 0, idn = 0;
				for(j = 0; j < 3; j++)
					if(t->v[j]->y < y) up[iup++] = t->v[j];
					else dn[idn++] = t->v[j];

				/* if a side includes just one vertex, it's alone */
				one = (iup == 1 ? up : dn);
				two = (iup == 1 ? dn : up);
				vt = one[0];
				vm = two[0];
				vb = two[1];
			}
			{
				double t = (y - vb->y) / (vt->y - vb->y), s = (y - vm->y) / (vt->y - vm->y);
				double q = vb->x + t * (vt->x - vb->x), r = vm->x + s * (vt->x - vm->x);
				double u = (x - q) / (r - q);
				return COLOR32MIX(COLOR32MIX(vb->c, vt->c, t), COLOR32MIX(vm->c, vt->c, s), u);
			}
/*			{
				double t = ((y - vb->y) * (vm->x - vb->x) - (x - vb->x) * (vm->y - vb->y)) / ((vt->y - vb->y) * (vm->x - vb->x) - (vm->y - vb->y) * (vt->x - vb->x));
				double s = ((x - vb->x) * (vt->y - vb->x) - (y - vb->y) * (vt->x - vb->x)) / ((vt->y - vb->y) * (vm->x - vb->x) - (vm->y - vb->y) * (vt->x - vb->x));
				double u = s + t;
				return COLOR32MIX(COLOR32MIX(vb->c, vt->c, t), COLOR32MIX(vb->c, vm->c, s), u);
			}*/
#else
			double di = (v1->y - v0->y) * (v2->x - v0->x) - (v1->x - v0->x) * (v2->y - v0->y);
			double f0 = (x - v1->x) * (v2->y - v1->y) - (y - v1->y) * (v2->x - v2->x) / di;
			double f1 = (y - v0->y) * (v2->y - v0->y) - (x - v0->x) * (v2->y - v0->y) / di;
			double f2 = (x - v1->x) * (v2->y - v1->y) - (y - v1->y) * (v1->x - v0->x) / di;
/*			return t->v[0]->c;*/
#define BREAD(C) f0 * COLOR32##C(v0->c) + f1 * COLOR32##C(v1->c) + f2 * COLOR32##C(v2->c)
			return COLOR32RGBA(BREAD(R), BREAD(G), BREAD(B), BREAD(A));
#undef BREAD
#endif
			}
		}
	}
	return cs->dc;
}

#define SIGNATURE "#cs2x"

static size_t mwrite(const void *in, size_t size, size_t num, char **str){
	size_t all = num * size;
	size_t len;
	len = *str ? strlen(*str) : 0;
	*str = realloc(*str, len + all + 1);
	memcpy(&(*str)[len], in, all);
	(*str)[len + all] = '\0';
	return num;
}

static int mprintf(char **str, const char *fmt, ...){
	va_list ap;
	int ret;
	char buf[128]; /* TODO */
	va_start(ap, fmt);
	ret = vsprintf(buf, fmt, ap);
	va_end(ap);
	if(0 < ret){
		size_t len;
		len = *str ? strlen(*str) : 0;
		*str = realloc(*str, len + ret + 1);
		strcpy(&(*str)[len], buf);
	}
	return ret;
}

static int cs2x_save_text(const struct cs2x *cs, void *fp, size_t fwrite(const void*, size_t, size_t, void*), int fprintf(void*, const char*, ...)){
	int i;
	fwrite(SIGNATURE, 1, sizeof(SIGNATURE)-1, fp);
/*	printf(SIGNATURE);*/
	fprintf(fp, "\ndc %d %d %d %d\n", COLOR32R(cs->dc), COLOR32G(cs->dc), COLOR32B(cs->dc), COLOR32A(cs->dc));
	fprintf(fp, "v{\n");
	for(i = 0; i < cs->nv; i++){
		vert *v = &cs->v[i];
		fprintf(fp, "\t%.15lg %.15lg %d %d %d %d\n", v->x, v->y, COLOR32R(v->c), COLOR32G(v->c), COLOR32B(v->c), COLOR32A(v->c));
	}
	fprintf(fp, "}\nt{\n");
	for(i = 0; i < cs->nt; i++){
		trit *t = &cs->t[i];
		fprintf(fp, "\t%d %d %d\n", t->v[0] - cs->v, t->v[1] - cs->v, t->v[2] - cs->v);
	}
	fprintf(fp, "}\n");
	return 1;
}

int ColorSequence2xSaveText(const struct cs2x *cs, FILE *fp){
	return cs2x_save_text(cs, fp, fwrite, fprintf);
}

char *ColorSequence2xSaveTextMemory(const struct cs2x *cs){
	char *ret = NULL;
	cs2x_save_text(cs, &ret, mwrite, mprintf);
	return ret;
}

/* unite the storage and free working space */
static struct cs2x *unite_cs2x(struct cs2x *cs){
	struct cs2x *pcs;
	pcs = malloc(sizeof *cs + cs->nv * sizeof *cs->v + cs->nt * sizeof *cs->t);
	if(!pcs)
		return NULL;
	*pcs = *cs;
	pcs->init = 0;
	pcs->v = (struct cs2x_vertex*)((char*)pcs + sizeof *cs);
	pcs->t = (struct cs2x_triangle*)((char*)pcs->v + cs->nv * sizeof *cs->v);
	if(cs->v)
		memcpy(pcs->v, cs->v, cs->nv * sizeof *cs->v), free(cs->v);
	if(cs->t){
		int i, j;
		memcpy(pcs->t, cs->t, cs->nt * sizeof *cs->t);

		/* something like relocation. pointer is good faster than array index
		  except this problem */
		for(i = 0; i < pcs->nt; i++) for(j = 0; j < numof(pcs->t[i].v); j++){
			/* cs->v is freed, but it's pointer value is still valid, phew! */
			pcs->t[i].v[j] = &pcs->v[cs->t[i].v[j] - cs->v];
		}
		free(cs->t);
	}
	return pcs;
}

#define BUS 64
static char *fgetline2(FILE *fp, char *s){
	char *c, *p;
	size_t sl = 0;
	if(!fp) return NULL;
	s = realloc(s, BUS + 1);
	p = s;
	while(fgets(p, BUS + 1, fp)){
		p[BUS] = '\0';
		if(c = strchr(p, '\n')){
			*c = '\0';
			return s;
		}
		s = (char*)realloc(s, (sl += BUS) + BUS + 1);
		p = &s[sl];
	}
	if(sl)
		return s;
	realloc(s, 0); /* free */
	return NULL;
}
#undef BUS

struct cs2x *ColorSequence2xLoad(FILE *fp){
	cs2xt *cs;
	int mode = 0;
	char *s = NULL;
	char buf[sizeof SIGNATURE];
	fread(buf, 1, sizeof SIGNATURE-1, fp);
	if(memcmp(buf, SIGNATURE, sizeof buf-1))
		return NULL;
	cs = malloc(sizeof*cs);
	if(!cs) return NULL;
	cs->nv = 0;
	cs->v = NULL;
	cs->nt = 0;
	cs->t = NULL;
	while(s = fgetline2(fp, s)) switch(mode){
		case 0:
			if(strncmp(s, "dc ", sizeof"dc "-1)){
				int r, g, b, a;
				sscanf(&s[sizeof"dc "-1], "%d %d %d", &r, &g, &b, &a);
				cs->dc = COLOR32RGB(r,g,b);
			}
			if(s[0] == 'v')
				mode = 1;
			else if(s[0] == 't')
				mode = 2;
			break;
		case 1:
			if(s[0] == '}')
				mode = 0;
			else{
				int r, g, b, a;
				cs->v = realloc(cs->v, (cs->nv+1) * sizeof*cs->v);
				sscanf(s, "%lg %lg %d %d %d %d", &cs->v[cs->nv].x, &cs->v[cs->nv].y, &r, &g, &b, &a);
				cs->v[cs->nv++].c = COLOR32RGBA(r,g,b,a);
			}
			break;
		case 2:
			if(s[0] == '}')
				mode = 0;
			else{
				int a, b, c;
				cs->t = realloc(cs->t, (cs->nt+1) * sizeof*cs->t);
				sscanf(s, "%d %d %d", &a, &b, &c);
				cs->t[cs->nt].v[0] = &cs->v[a];
				cs->t[cs->nt].v[1] = &cs->v[b];
				cs->t[cs->nt].v[2] = &cs->v[c];
				cs->nt++;
			}
			break;
	}
	return unite_cs2x(cs);
}

static const char *mgetline(const char *src, const char *s){
	const char *ret;
	if(!s)
		return src;
	ret = s;
	do{
		ret = strpbrk(ret, "\r\n");
	} while(ret && (ret++, *ret == '\r' || *ret == '\n'));
	return ret;
}

struct cs2x *ColorSequence2xLoadMemory(const char *src){
	cs2xt cs;
	int mode = 0;
	const char *s = NULL;
	char buf[sizeof SIGNATURE];
	if(memcmp(src, SIGNATURE, sizeof buf-1))
		return NULL;
	cs.nv = 0;
	cs.v = NULL;
	cs.nt = 0;
	cs.t = NULL;
	while(s = mgetline(src, s)) switch(mode){
		case 0:
			if(!strncmp(s, "dc ", sizeof"dc "-1)){
				int r, g, b, a;
				sscanf(&s[sizeof"dc "-1], "%d %d %d %d", &r, &g, &b, &a);
				cs.dc = COLOR32RGB(r,g,b);
			}
			else if(s[0] == 'v')
				mode = 1;
			else if(s[0] == 't')
				mode = 2;
			break;
		case 1:
			if(s[0] == '}')
				mode = 0;
			else{
				int r, g, b, a;
				cs.v = realloc(cs.v, (cs.nv+1) * sizeof*cs.v);
				sscanf(s, "%lg %lg %d %d %d %d", &cs.v[cs.nv].x, &cs.v[cs.nv].y, &r, &g, &b, &a);
				cs.v[cs.nv++].c = COLOR32RGB(r,g,b);
			}
			break;
		case 2:
			if(s[0] == '}')
				mode = 0;
			else{
				int a, b, c;
				cs.t = realloc(cs.t, (cs.nt+1) * sizeof*cs.t);
				sscanf(s, "%d %d %d", &a, &b, &c);
				cs.t[cs.nt].v[0] = &cs.v[a];
				cs.t[cs.nt].v[1] = &cs.v[b];
				cs.t[cs.nt].v[2] = &cs.v[c];
				cs.nt++;
			}
			break;
	}
	return unite_cs2x(&cs);
}
