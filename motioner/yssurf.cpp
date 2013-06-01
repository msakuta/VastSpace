#include "yssurf.h"
#include <clib/avec3.h>
#include <limits.h>
#include <stddef.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>


#define numof(a) (sizeof(a)/sizeof*(a))

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif


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

static sufindex add_vertex(suf_t *suf, const sufcoord v[3]){
	int i;
	for(i = 0; i < suf->nv; i++){
		sufcoord *va = suf->v[i];
		if(va[0] == v[0] && va[1] == v[1] && va[2] == v[2])
			return (sufindex)i;
	}
	if(SUFINDEX_MAX - 1 <= suf->nv)
		return SUFINDEX_MAX;
	suf->v = (sufcoord(*)[3])realloc(suf->v, ++suf->nv * sizeof *suf->v);
	VECCPY(suf->v[i], v);
	return (sufindex)i;
}

static union suf::suf_prim_t **add_poly(suf_t *suf){
	suf->p = (suf::suf_prim_t**)realloc(suf->p, (suf->np + 1) * sizeof *suf->p);
	suf->p[suf->np] = (suf::suf_prim_t*)malloc(sizeof(union suf::suf_prim_t));
	suf->p[suf->np]->p.n = 0;
	return &suf->p[suf->np++];
}

static void add_polyvert(struct suf::suf_prim_t::suf_poly_t **p, sufindex i, sufindex j){
	int n = *p ? (*p)->n + 1 : 1;
	assert((*p)->t == suf::suf_prim_t::suf_poly || (*p)->t == suf::suf_prim_t::suf_shade);
	*p = (suf::suf_prim_t::suf_poly_t*)realloc(*p, offsetof(struct suf::suf_prim_t::suf_poly_t, v) + n * sizeof (*p)->v);
	(*p)->v[n-1][0] = i;
	(*p)->v[n-1][1] = j;
	(*p)->n = n;
}

static void add_uvpolyvert(struct suf::suf_prim_t::suf_uvpoly_t **uv, sufindex i, sufindex j, sufindex k){
	int n = *uv ? (*uv)->n + 1 : 1;
	assert((*uv)->t == suf::suf_prim_t::suf_uvpoly || (*uv)->t == suf::suf_prim_t::suf_uvshade);
	*uv = (suf::suf_prim_t::suf_uvpoly_t*)realloc(*uv, offsetof(struct suf::suf_prim_t::suf_uvpoly_t, v) + n * sizeof (*uv)->v);
	(*uv)->v[n-1].p = i;
	(*uv)->v[n-1].n = j;
	(*uv)->v[n-1].t = k;
	(*uv)->n = n;
}

static suf_t *load_yssuf(FILE *fp, int lines){
	suf_t *ret;
	sufindex atr = USHRT_MAX; /* current attribute index */
	char buf[128], *s = NULL, *name = NULL;
	FPOS fo;
	int i, line = 0;
	fo.fp = fp, fo.n = sizeof buf, fo.buf = buf;
	/* checking signatures */
	s = ftok(&fo);
	if(!s || strcmp(s, "SURF")) return NULL;

	ret = (suf_t*)malloc(sizeof *ret);
	ret->nv = 0;
	ret->v = NULL;
	ret->na = 0;
	ret->a = NULL;
	ret->np = 0;
	ret->p = NULL;

	ret->a = (suf::suf_atr_t*)malloc(sizeof *ret->a);
	for(i = 0; i < 4; i++)
		ret->a[0].col[i] = 1.f;
	for(i = 0; i < 4; i++)
		ret->a[0].dif[i] = 1.f;
	for(i = 0; i < 4; i++)
		ret->a[0].amb[i] = 1.f;
	for(i = 0; i < 4; i++)
		ret->a[0].tra[i] = 0.f;
	for(i = 0; i < 4; i++)
		ret->a[0].emi[i] = 0.f;
	for(i = 0; i < 4; i++)
		ret->a[0].spc[i] = .4f;
	ret->a[0].colormap = NULL;
	ret->a[0].name = "def";
	for(i = 0; i < 4; i++)
		ret->a[0].valid = SUF_COL | SUF_DIF | SUF_AMB;
	ret->na = 1;
	atr = 0;

	do{
		s = fgets(buf, sizeof buf, fp);
		s = strtok(s, " \t\n");
		if(lines <= line++)
			return ret;
		if(!s) return ret;
		if(!strcmp(s, "V")){ /* add vertex */
			int j;
			ret->v = (sufcoord (*)[3])realloc(ret->v, (ret->nv + 1) * sizeof *ret->v);
			for(j = 0; j < 3; j++){
				s = strtok(NULL, " \t\n");
				ret->v[ret->nv][j] = atof(s);
			}
			ret->nv++;
/*			s = ftok(&fo);
			if(strcmp("R")){
				fprintf(stderr, "??????\n");
			}*/
		}
		else if(!strcmp(s, "F")){
			int i, j;
			enum suf::suf_prim_t::suf_elemtype type = suf::suf_prim_t::suf_poly;
			sufindex norm;
			union suf::suf_prim_t **sp;
			sp = add_poly(ret);
			(*sp)->t = suf::suf_prim_t::suf_poly;
			(*sp)->p.atr = atr;

			do{
				s = fgets(buf, sizeof buf, fp);
				line++;
				if(!s) return NULL;
				if(s[0] == 'C'){ /* color? */
					float col[4];
					char *ss;
					ss = &s[2];
					for(i = 0; i < 3; i++){
						col[i] = strtod(ss, &ss) / 256.;
					}
					col[3] = 1.;
					for(i = 0; i < ret->na; i++) if(!memcmp(ret->a[i].col, col, 3 * sizeof(float))){
						(*sp)->p.atr = atr = i;
						break;
					}
					if(i == ret->na){
						ret->a = (suf::suf_atr_t*)realloc(ret->a, (ret->na + 1) * sizeof *ret->a);
						for(i = 0; i < 4; i++)
							ret->a[ret->na].col[i] = col[i];
						for(i = 0; i < 4; i++)
							ret->a[ret->na].dif[i] = col[i];
						for(i = 0; i < 4; i++)
							ret->a[ret->na].amb[i] = col[i];
						for(i = 0; i < 4; i++)
							ret->a[ret->na].tra[i] = 0.f;
						for(i = 0; i < 4; i++)
							ret->a[ret->na].emi[i] = 0.f;
						for(i = 0; i < 4; i++)
							ret->a[ret->na].spc[i] = .4f;
						ret->a[ret->na].colormap = NULL;
						ret->a[ret->na].name = "def";
						for(i = 0; i < 4; i++)
							ret->a[ret->na].valid = SUF_COL | SUF_DIF | SUF_AMB;
						(*sp)->p.atr = atr = ret->na;
						ret->na++;
					}
				}
				else if(s[0] == 'N'){  /* normal vector? */
/*					char *ss;
					sufcoord v[3];
					v[0] = strtod(&s[2], &ss);
					v[1] = strtod(ss, &ss);
					v[2] = strtod(ss, &ss);
					norm = add_vertex(ret, v);*/
				}
				else if(s[0] == 'V'){
					int v;
					char *ss;
					strtok(s, " \t\n");
					for(i = 0; i < 10 && (ss = strtok(NULL, " \t\n")); i++){
						v = atol(ss);
						add_polyvert((suf::suf_prim_t::suf_poly_t**)sp, v, 0);
					}
					if(type != suf::suf_prim_t::suf_uvshade && 3 <= i){
						unsigned short a, b, c, vvi;
						double vv[3];

						/* there's an occasion that three consequative vertices lie on a line,
						so we need to find the valid triplet from all possible cases.
						note that polygon normal is calculated the same direction,
						as long as the polygon is convex. */
						for(a = 0; a < i; a++) for(b = a+1; b < i; b++) for(c = b+1; c < i; c++){
							double *v0, *v1, *v2;
							double v01[3], v02[3];
							if(type == suf::suf_prim_t::suf_uvpoly){
								struct suf::suf_prim_t::suf_uvpoly_t *uv = &(*sp)->uv;
								v0 = ret->v[uv->v[a].p];
								v1 = ret->v[uv->v[b].p];
								v2 = ret->v[uv->v[c].p];
							}
							else{
								struct suf::suf_prim_t::suf_poly_t *p = &(*sp)->p;
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
							if(type == suf::suf_prim_t::suf_uvpoly) for(j = 0; j < i; j++)
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
						if(type == suf::suf_prim_t::suf_uvpoly) for(j = 0; j < i; j++)
							(*sp)->uv.v[j].n = vvi;
						else for(j = 0; j < i; j++)
							(*sp)->p.v[j][1] = vvi;
					}
				}
				else if(s[0] == 'E'){
					break;
				}
			} while (1);
		}
		else if(!strcmp(s, "E")){
			break;
		}
	} while(1);

	return ret;
}

suf_t *LoadYSSUF(const char *fname){
	suf_t *ret;
	FILE *fp;

	fp = fopen(fname, "r");
	if(!fp)
		return NULL;

	ret = load_yssuf(fp, 100000);

	/* now that reading attributes, no need to read from suf file anymore. */
	fclose(fp);

	/* prepare allocation table for auto-freeing */
/*	{
		at_t at = {0};
		if(!load_atr(ret, fname, &at))
			atfreeall(&at);
	}*/

	return ret;
}

ysdnm_t *LoadYSDNM(const char *fname){
	ysdnm_t *ret;
	FILE *fp;
	char buf[128], *s = NULL, *name = NULL;

	fp = fopen(fname, "r");
	if(!fp)
		return NULL;

	ret = (ysdnm_t*)malloc(sizeof *ret);
	ret->np = 0;
	ret->p = NULL;
	ret->ns = 0;
	ret->s = NULL;

	if(!fgets(buf, sizeof buf, fp) || strcmp(buf, "DYNAMODEL\n"))
		return NULL;
	if(!fgets(buf, sizeof buf, fp) || strcmp(buf, "DNMVER 1\n"))
		return NULL;
	do{
		int lines;
		s = fgets(buf, sizeof buf, fp);
		if(!s) return NULL;
		if(!strncmp(s, "PCK ", 4)){
			s = strtok(s, " \t\n");
			s = strtok(NULL, " \t\n");
			ret->p = (ysdnm_pck*)realloc(ret->p, (ret->np + 1) * sizeof *ret->p);
			ret->p[ret->np].name = (char *)malloc(strlen(s) + 1);
			strcpy(ret->p[ret->np].name, s);
			lines = atoi(strtok(NULL, " \t\n"));
			ret->p[ret->np].suf = load_yssuf(fp, lines);
			ret->np++;
		}
		else if(!strncmp(s, "SRF ", 4)){
			struct ysdnm_srf *srf;
			int i, nch = 0;
			s = strtok(&s[4], "\n\"");
			ret->s = (ysdnm_srf**)realloc(ret->s, (ret->ns + 1) * sizeof *ret->s);
			srf = ret->s[ret->ns] = (ysdnm_srf*)malloc(sizeof *ret->s[ret->ns]);
			srf->name = (char *)malloc(strlen(s) + 1);
			srf->cla = 0;
			srf->nst = 0;
			srf->nch = 0;
			strcpy(srf->name, s);
			do{
				s = fgets(buf, sizeof buf, fp);
				if(!s) return NULL;
				else if(!strncmp(s, "FIL ", 4)){
					s = strtok(s, " \t\n");
					s = strtok(NULL, " \t\n");
					srf->pck = NULL;
					for(i = 0; i < ret->np; i++) if(!strcmp(s, ret->p[i].name)){
						srf->pck = &ret->p[i];
						break;
					}
				}
				else if(!strncmp(s, "CLA ", 4)){
					s = strtok(s, " \t\n");
					s = strtok(NULL, " \t\n");
					srf->cla = atoi(s);
				}
				else if(!strncmp(s, "STA ", 4)){/* really not know what this means */
					s = strtok(s, " \t\n");
					for(i = 0; i < 7 && (s = strtok(NULL, " \t\n")); i++) if(srf->nst < numof(srf->sta)){
						if(i < 6)
							srf->sta[srf->nst][i] = (i < 3 ? atof(s) : atof(s) * 180 / 32768.);
						else
							srf->stav[srf->nst] = !!atoi(s);
					}
					if(srf->nst < numof(srf->sta))
						srf->nst++;
				}
				else if(!strncmp(s, "POS ", 4)){
					s = strtok(s, " \t\n");
					for(i = 0; i < 3; i++){
						s = strtok(NULL, " \t\n");
						srf->pos[i] = atof(s);
					}
				}
				else if(!strncmp(s, "NCH ", 4)){
					s = strtok(s, " \t\n");
					s = strtok(NULL, " \t\n");
					srf->nch = atoi(s);
					srf = ret->s[ret->ns] = (ysdnm_srf*)realloc(srf, offsetof(struct ysdnm_srf, cld) + srf->nch * sizeof *srf->cld);
				}
				else if(!strncmp(s, "CLD ", 4)){
					s = strtok(&s[4], "\"\t\n");
					srf->cld[nch].name = (char*)malloc(strlen(s) + 1);
					strcpy(srf->cld[nch].name, s);
					srf->cld[nch].srf = NULL;
					nch++;
				}
				else if(!strncmp(s, "END", 3)){
					break;
				}
			} while(1);
			ret->ns++;
		}
		else if(!strncmp(s, "END", 3)){
			break;
		}
	} while(1);

	/* now that reading attributes, no need to read from suf file anymore. */
	fclose(fp);

	/* prepare allocation table for auto-freeing */
/*	{
		at_t at = {0};
		if(!load_atr(ret, fname, &at))
			atfreeall(&at);
	}*/

	return ret;
}

