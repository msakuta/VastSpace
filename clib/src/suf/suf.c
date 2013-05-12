#include "clib/suf/suf.h"
#include "clib/avec3.h"
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define AT_MAX 128

typedef struct{
	int n;
	void *t[AT_MAX];
} at_t;

/* allocation table alloc */
static void *atalloc(size_t size, at_t *at){
	void *ret;
	if(AT_MAX-1 <= at->n)
		return NULL;
	ret = malloc(size);
	if(ret) at->t[at->n++] = ret;
	return ret;
}

static void atfree(void *p, at_t *at){
	int i = 0;
	for(i = 0; i < at->n; i++) if(at->t[i] == p){
		free(at->t[i]);
		memmove(&at->t[i], &at->t[i+1], --at->n - i);
		return;
	}
	assert(0);
}

static void atfreeall(at_t *at){
	int i = 0;
	for(i = 0; i < at->n; i++)
		free(at->t[i]);
}


typedef struct{
	FILE *fp;
	size_t n;
	char *buf;
} FPOS;

static char *ftok(FPOS *fp){
	size_t i;
	int lc;
	char *s = fp->buf;
	for(i = 0; i < fp->n; i++){
		int c;
		c = getc(fp->fp);
		if(c == EOF || isspace(c)){
			if(i == 0 || isspace(lc)){
				if(c == EOF) return NULL;
				lc = c;
				continue;
			}
			*s = '\0';
			return fp->buf;
		}
		lc = c;
		*s++ = (char)c;
	}
	if(i == fp->n) /* overflow! */
		return NULL;
	return s;
}

static struct suf_atr_t *find_atr(suf_t *suf, const char *name){
	int i;
	for(i = 0; i < suf->na; i++) if(suf->a[i].name && !strcmp(suf->a[i].name, name))
		return &suf->a[i];
	return NULL;
}

static int readcolor(FPOS *fo, float f[4]){
	char *s;
	int i;
	s = ftok(fo);
	if(!s || strcmp(s, "(")) return 0;
	s = ftok(fo);
	if(!s) return 0;
	else if(!strcmp(s, "rgb")){
		s = ftok(fo);
		if(!s || strcmp(s, "(")) return 0;
		for(i = 0; i < 3; i++){
			s = ftok(fo);
			if(!s) return 0;
			if(f) f[i] = (float)atof(s);
		}
		if(f)
			f[3] = 1.F; /* no idea */
		s = ftok(fo);
		if(!s || strcmp(s, ")")) return 0;
	}
	else{
		float r;
		r = (float)atof(s);
		if(f){
			for(i = 0; i < 3; i++)
				f[i] = r;
			f[3] = 1.F; /* no idea */
		}
	}
	s = ftok(fo);
	if(!s || strcmp(s, ")")) return 0;
	return 1;
}

static int readcoord(FPOS *fo, float f[4]){
	char *s;
	int i;
	s = ftok(fo);
	if(!s || strcmp(s, "(")) return 0;
	for(i = 0; i < 3; i++){
		s = ftok(fo);
		if(!s) return 0;
		if(f) f[i] = (float)atof(s);
	}
	if(f) f[3] = 1.F; /* no idea */
	s = ftok(fo);
	if(!s || strcmp(s, ")")) return 0;
	return 1;
}

static int readrect(FPOS *fo, sufcoord f[4]){
	char *s;
	int i;
	s = ftok(fo);
	if(!s || strcmp(s, "(")) return 0;
	for(i = 0; i < 4; i++){
		s = ftok(fo);
		if(!s) return 0;
		if(f) f[i] = (sufcoord)atof(s);
	}
	s = ftok(fo);
	if(!s || strcmp(s, ")")) return 0;
	return 1;
}

static const struct suf_atr_t atr0 = {
	NULL, /* char *name; */
	0, /*unsigned valid; /* validity flag of following values */
	{1.F, 1.F, 1.F, 1.F}, /*float col[4]; /* clamped */
	{0.F, 0.F, 0.F, 1.F}, /*float tra[4];*/
	{1.F, 1.F, 1.F, 1.F}, /*float amb[4]; /* ambient */
	{0.F, 0.F, 0.F, 1.F}, /*float emi[4]; /* emission */
	{1.F, 1.F, 1.F, 1.F}, /*float dif[4]; /* diffuse */
	{0.F, 0.F, 0.F, 1.F}, /*float spc[4]; /* specular lighting */
	NULL, /*char *colormap; /* texture color map name */
	{0, 0, 255, 255}, /*sufcoord mapsize[4];*/
	{0, 0, 255, 255}, /*sufcoord mapview[4];*/
	{0, 0, 255, 255}, /*sufcoord mapwind[4];*/
};

#define COLCPY(r,a) ((r)[0]=(a)[0],(r)[1]=(a)[1],(r)[2]=(a)[2],(r)[3]=(a)[3])
#define COLMULIN(r,a) ((r)[0]*=(a)[0],(r)[1]*=(a)[1],(r)[2]*=(a)[2],(r)[3]*=(a)[3])

static int load_atr(suf_t *suf, const char *path, at_t *at){
	char *fname;
	char buf[128], *s;
	FPOS fo;
	if(!suf || !path) return 0;
	s = strrchr(path, '.');
	fname = atalloc(s ? s - path + sizeof".atr" : strlen(path) + sizeof".atr", at);
	if(!fname) return 0;
	strcpy(fname, path);
	if(s)
		strcpy(&fname[s - path], ".atr");
	else
		strcat(fname, ".atr");
	fo.fp = fopen(fname, "r");
	if(!fo.fp) return 0;
	fo.n = sizeof buf;
	fo.buf = buf;

	do{
		s = ftok(&fo);
		if(!s) break;
		if(!strcmp(s, "atr")){
			struct suf_atr_t *atr = NULL;

			/* find attribute name, if the file contains attributes that are not
			  actually used, continue to parse but ignore it. */
			s = ftok(&fo);
			if(!s) return 0;
			atr = find_atr(suf, s);

			s = ftok(&fo);
			if(!s || strcmp(s, "{")) return 0;
			do{
				s = ftok(&fo);
				if(!s) return 0;

				if(!strcmp(s, "col")){
					if(!readcolor(&fo, atr ? atr->col : NULL))
						return 0;
					if(atr) atr->valid |= SUF_COL;
				}
				else if(!strcmp(s, "tra")){
					if(!readcolor(&fo, atr ? atr->tra : NULL))
						return 0;
					if(atr) atr->valid |= SUF_TRA;
				}
				else if(!strcmp(s, "amb")){
					if(!readcolor(&fo, atr ? atr->amb : NULL))
						return 0;
					if(atr) atr->valid |= SUF_AMB;
				}
				else if(!strcmp(s, "emi")){
					if(!readcolor(&fo, atr ? atr->emi : NULL))
						return 0;
					if(atr) atr->valid |= SUF_EMI;
				}
				else if(!strcmp(s, "dif")){
					if(!readcolor(&fo, atr ? atr->dif : NULL))
						return 0;
					if(atr) atr->valid |= SUF_DIF;
				}
				else if(!strcmp(s, "spc")){
					if(!readcoord(&fo, atr ? atr->spc : NULL))
						return 0;
					if(atr) atr->valid |= SUF_SPC;
				}
				else if(!strcmp(s, "colormap")){
					s = ftok(&fo);
					if(!s || strcmp(s, "(")) return 0;
					s = ftok(&fo);
					if(!s) return 0;
					if(atr) atr->colormap = strdup(s);
					s = ftok(&fo);
					if(!s || strcmp(s, ")")) return 0;
					if(atr) atr->valid |= SUF_TEX;
				}
				else if(!strcmp(s, "mapsize")){
					if(!readrect(&fo, atr ? atr->mapsize : NULL))
						return 0;
				}
				else if(!strcmp(s, "mapview")){
					if(!readrect(&fo, atr ? atr->mapview : NULL))
						return 0;
				}
				else if(!strcmp(s, "mapwind")){
					if(!readrect(&fo, atr ? atr->mapwind : NULL))
						return 0;
				}
				/* all other tokens are ignored! */
			} while(strcmp(s, "}"));

			/* the col value is specific to the suf file format, i.e. OpenGL
			  doesn't concern it. so we need to emulate the color effect manually,
			  by multiplying reflection factors. */
			if(atr && atr->valid & SUF_COL){
				if(atr->valid & SUF_AMB)
					COLMULIN(atr->amb, atr->col);
				else
					COLCPY(atr->amb, atr->col);
				if(atr->valid & SUF_DIF)
					COLMULIN(atr->dif, atr->col);
				else
					COLCPY(atr->dif, atr->col);
				if(atr->valid & SUF_EMI)
					COLMULIN(atr->emi, atr->col);
				else
					COLCPY(atr->emi, atr->col);

				/* transparency is so complex that needs more discussion */
				if(atr->valid & SUF_TRA)
					atr->amb[3] = atr->dif[3] = 1.f - atr->tra[0];

				atr->valid |= SUF_AMB | SUF_DIF;
			}
		}
	} while(1);

	fclose(fo.fp);

	/* if successive, be sure to free working space because the caller
	  won't free them. */
	atfree(fname, at);

	return 1;
}

static sufindex add_vertex(suf_t *suf, const sufcoord v[3]){
	int i;
	for(i = 0; i < suf->nv; i++){
		sufcoord *va = suf->v[i];
		if(va[0] == v[0] && va[1] == v[1] && va[2] == v[2])
			return (sufindex)i;
	}
	if(SUFINDEX_MAX - 1 <= suf->nv)
		return SUFINDEX_MAX;
	suf->v = realloc(suf->v, ++suf->nv * sizeof *suf->v);
	VECCPY(suf->v[i], v);
	return (sufindex)i;
}

static union suf_prim_t **add_poly(suf_t *suf){
	suf->p = realloc(suf->p, (suf->np + 1) * sizeof *suf->p);
	suf->p[suf->np] = malloc(sizeof(union suf_prim_t));
/*	suf->p[suf->np]->n = 0;*/
	return &suf->p[suf->np++];
}

static void add_polyvert(struct suf_poly_t **p, sufindex i, sufindex j){
	int n = *p ? (*p)->n + 1 : 1;
	assert((*p)->t == suf_poly || (*p)->t == suf_shade);
	*p = realloc(*p, offsetof(struct suf_poly_t, v) + n * sizeof (*p)->v);
	(*p)->v[n-1][0] = i;
	(*p)->v[n-1][1] = j;
	(*p)->n = n;
}

static void add_uvpolyvert(struct suf_uvpoly_t **uv, sufindex i, sufindex j, sufindex k){
	int n = *uv ? (*uv)->n + 1 : 1;
	assert((*uv)->t == suf_uvpoly || (*uv)->t == suf_uvshade);
	*uv = realloc(*uv, offsetof(struct suf_uvpoly_t, v) + n * sizeof (*uv)->v);
	(*uv)->v[n-1].p = i;
	(*uv)->v[n-1].n = j;
	(*uv)->v[n-1].t = k;
	(*uv)->n = n;
}

/* the argument must be either full or relative path to existing file,
  for its path is used to search attribute files. */
suf_t *LoadSUF(const char *fname){
	FILE *fp;
	suf_t *ret;
	char buf[128], *s = NULL, *name = NULL;
	sufindex atr = USHRT_MAX; /* current attribute index */
	FPOS fo;

	fp = fopen(fname, "r");
	if(!fp)
		return NULL;

	fo.fp = fp, fo.n = sizeof buf, fo.buf = buf;

	/* checking signatures */
	s = ftok(&fo);
	if(!s || strcmp(s, "obj")) return NULL;
	s = ftok(&fo);
	if(!s || strcmp(s, "suf")) return NULL;

	/* obtain model name */
	do{
		s = ftok(&fo);
		if(!s) return NULL;
		if(name) continue;
		name = malloc(strlen(s) + 1);
		strcpy(name, s);
	} while(strcmp(s, "{"));

	ret = malloc(sizeof *ret);
	ret->nv = 0;
	ret->v = NULL;
	ret->na = 0;
	ret->a = NULL;
	ret->np = 0;
	ret->p = NULL;

	do{
		s = ftok(&fo);
		if(!s) return NULL;
		if(!strcmp(s, "atr")){
			struct suf_atr_t *patr;
			s = ftok(&fo);
			if(!s) return NULL;
			patr = find_atr(ret, s);
			if(patr)
				atr = (sufindex)(patr - ret->a);
			else{
				atr = ret->na;
				ret->a = realloc(ret->a, (ret->na+1) * sizeof *ret->a);
				if(!ret->a) return NULL;

				/* initialization is necessary after all */
				memcpy(&ret->a[ret->na], &atr0, sizeof atr0);

				ret->a[ret->na].name = malloc(strlen(s) + 1);
				strcpy(ret->a[ret->na].name, s);
				ret->a[ret->na].valid = 0;
				ret->na++;
			}
		}
		else if(!strcmp(s, "prim")){
			int i, j;
			enum suf_elemtype type;
			union suf_prim_t **sp;
			s = ftok(&fo);
			if(!s) return NULL;
			if(!strcmp(s, "poly"))
				type = suf_poly;
			else if(!strcmp(s, "shade"))
				type = suf_shade;
			else if(!strcmp(s, "uvpoly"))
				type = suf_uvpoly;
			else if(!strcmp(s, "uvshade"))
				type = suf_uvshade;
			else continue;

			s = ftok(&fo);
			if(!s || strcmp(s, "(")) return NULL;
			sp = add_poly(ret);
			(*sp)->t = type;

			(*sp)->p.atr = atr;
			switch(type){
				case suf_poly:
				case suf_uvpoly:
				case suf_uvshade:
				(*sp)->uv.n = 0;
				for(i = 0;; i++){
					sufcoord v[3];
					for(j = 0; j < 3; j++){
						s = ftok(&fo);
						if(!s) return NULL;
						if(!strcmp(s, ")")) goto poly_end;
						v[j] = (sufcoord)atof(s);
					}
					if(type == suf_uvpoly || type == suf_uvshade){
						sufcoord uv[3];
						sufindex n;
						if(type == suf_uvshade){
							sufcoord norm[3] = {0};
							for(j = 0; j < 3; j++){
								s = ftok(&fo);
								if(!s) return NULL;
								if(!strcmp(s, ")"))
									break;
								norm[j] = atof(s);
							}

							/* and so do shaded surfaces (a polygon modeler may not normalize it) */
							VECNORMIN(norm);

							n = add_vertex(ret, norm);
						}
						else
							n = SUFINDEX_MAX;

						uv[0] = (sufcoord)atof(ftok(&fo));
						uv[1] = (sufcoord)atof(ftok(&fo));
						uv[2] = 0;

						add_uvpolyvert(sp, add_vertex(ret, v), n, add_vertex(ret, uv));
					}
					else{
						add_polyvert(sp, add_vertex(ret, v), SUFINDEX_MAX);
					}
				}
poly_end:
				if(type != suf_uvshade && 3 <= i){
					unsigned short a, b, c, vvi;
					double vv[3];

					/* there's an occasion that three consequative vertices lie on a line,
					  so we need to find the valid triplet from all possible cases.
					  note that polygon normal is calculated the same direction,
					  as long as the polygon is convex. */
					for(a = 0; a < i; a++) for(b = a+1; b < i; b++) for(c = b+1; c < i; c++){
						double *v0, *v1, *v2;
						double v01[3], v02[3];
						if(type == suf_uvpoly){
							struct suf_uvpoly_t *uv = &(*sp)->uv;
							v0 = ret->v[uv->v[a].p];
							v1 = ret->v[uv->v[b].p];
							v2 = ret->v[uv->v[c].p];
						}
						else{
							struct suf_poly_t *p = &(*sp)->p;
							v0 = ret->v[p->v[a][0]];
							v1 = ret->v[p->v[b][0]];
							v2 = ret->v[p->v[c][0]];
						}
						VECSUB(v01, v0, v1);
						VECSUB(v02, v0, v2);
						VECVP(vv, v01, v02);
						if(vv[0] != 0. || vv[1] != 0. || vv[2] != 0.)
							goto abc_break;
					}
abc_break:

					/* polygon on a line */
					if(a == i){
						sufcoord nullvec[3];
						VECNULL(nullvec);
						vvi = add_vertex(ret, nullvec);
						if(type == suf_uvpoly) for(j = 0; j < i; j++)
							(*sp)->uv.v[j].p = vvi;
						else for(j = 0; j < i; j++)
							(*sp)->p.v[j][1] = vvi;
						break;
					}

					/* this normalization isn't necessary, but
					  it would improve memory space efficiency, for
					  normal vectors tend to point the same direction
					  with different length. */
					VECNORMIN(vv);

					vvi = add_vertex(ret, vv);
					if(type == suf_uvpoly) for(j = 0; j < i; j++)
						(*sp)->uv.v[j].n = vvi;
					else for(j = 0; j < i; j++)
						(*sp)->p.v[j][1] = vvi;
				}
				break;

				case suf_shade:
				(*sp)->p.n = 0;
				while(1){
					double v[2][3], len;
					for(i = 0; i < 2; i++) for(j = 0; j < 3; j++){
						s = ftok(&fo);
						if(!s) return NULL;
						if(!strcmp(s, ")")) goto shade_end;
						v[i][j] = atof(s);
					}

					/* and so do shaded surfaces (a polygon modeler may not normalize it) */
					len = VECLEN(v[1]);
					if(len)
						VECSCALEIN(v[1], 1. / len);

					add_polyvert(sp, add_vertex(ret, v[0]), add_vertex(ret, v[1]));
				}
shade_end:		break;

				default:;
			}
		}
	} while(strcmp(s, "}"));

	/* now that reading attributes, no need to read from suf file anymore. */
	fclose(fp);

	/* prepare allocation table for auto-freeing */
	{
		at_t at = {0};
		if(!load_atr(ret, fname, &at))
			atfreeall(&at);
	}

	return ret;
}


/* count up total memory usage for this suf object */
size_t SizeofSUF(const suf_t *suf){
	size_t ret = 0;
	int i;
	if(!suf) return 0;
	ret += suf->nv * sizeof *suf->v;
	ret += suf->na * sizeof suf->a;
	for(i = 0; i < suf->na; i++) if(suf->a[i].name)
		ret += strlen(suf->a[i].name) + 1;
	for(i = 0; i < suf->np; i++) if(suf->p[i]) if(suf->p[i]->t == suf_poly || suf->p[i]->t == suf_shade)
		ret += offsetof(struct suf_poly_t, v) + suf->p[i]->p.n * sizeof *suf->p[i]->p.v;
	else
		ret += offsetof(struct suf_uvpoly_t, v) + suf->p[i]->uv.n * sizeof *suf->p[i]->uv.v;
	return ret;
}

void FreeSUF(suf_t *suf){
	if(!suf) return;
	if(suf->v) free(suf->v);
	if(suf->a){
		int i;
		for(i = 0; i < suf->na; i++)
			free(suf->a[i].name);
		free(suf->a);
	}
	if(suf->p){
		int i;
		for(i = 0; i < suf->np; i++)
			free(suf->p[i]);
		free(suf->p);
	}
	free(suf);
};
