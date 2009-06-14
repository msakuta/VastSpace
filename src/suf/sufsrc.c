/* sufsrc.c - a suf format to C source file converter
  use this to compile the surface models into the executable.
  things like static objects are good to implement by the source code.
  note that you cannot overwrite the compiled model without recompiling,
  which can be bothering to have some customize fun. */

#include "clib/suf/suf.h"
#include <stdlib.h>
#include <string.h>

static const char *elemtype(enum suf_elemtype t){
	switch(t){
		case suf_poly: return "suf_poly";
		case suf_uvpoly: return "suf_uvpoly";
		case suf_shade: return "suf_shade";
		case suf_uvshade: return "suf_uvshade";
		default: return "?";
	}
}

static char *hackadd(char *hacked[], int *hackeds, int add){
	if(*hackeds <= add){
		*hacked = realloc(*hacked, (add + 1) * sizeof*hacked);
		memset(&(*hacked)[*hackeds], 0, (add + 1 - *hackeds) * sizeof*hacked);
		*hackeds = add + 1;
	}
	return *hacked;
}

void SourceSUF(const suf_t *suf, FILE *fp, const char *name){
	char *hacked = NULL;
	int hackeds = 0;
	int i;
	if(!suf || !fp) return;

	/* the include directry is subject of discussion */
	fprintf(fp, "#include <clib/suf/suf.h>\n");

	/* vertex section */
	fprintf(fp, "static sufcoord suf_%s_vertex[%d][3] = {\n", name, suf->nv);
	for(i = 0; i < suf->nv; i++)
		fprintf(fp, "\t{%lg, %lg, %lg},\n", (double)suf->v[i][0], (double)suf->v[i][1], (double)suf->v[i][2]);
	fprintf(fp, "};\n\n"
		
	/* attribute section */
	"static struct suf_atr_t suf_%s_attribute[%d] = {\n", name, suf->na);
	for(i = 0; i < suf->na; i++){
		struct suf_atr_t *a = &suf->a[i];
		fprintf(fp, "\t{"
			"\"%s\", %d, "
			"{%fF, %fF, %fF, %fF}, "
			"{%fF, %fF, %fF, %fF}, "
			"{%fF, %fF, %fF, %fF}, "
			"{%fF, %fF, %fF, %fF}, "
			"{%fF, %fF, %fF, %fF}, "
			"{%fF, %fF, %fF, %fF}, ",
		a->name,
		a->valid,
		a->col[0], a->col[1], a->col[2], a->col[3],
		a->tra[0], a->tra[1], a->tra[2], a->tra[3],
		a->amb[0], a->amb[1], a->amb[2], a->amb[3],
		a->emi[0], a->emi[1], a->emi[2], a->emi[3],
		a->dif[0], a->dif[1], a->dif[2], a->dif[3],
		a->spc[0], a->spc[1], a->spc[2], a->spc[3]);
		if(a->valid & SUF_TEX && a->colormap)
			fprintf(fp, "\"%s\", "
			"{%fF, %fF, %fF, %fF}, "
			"{%fF, %fF, %fF, %fF}, "
			"{%fF, %fF, %fF, %fF}},\n",
			a->colormap,
			a->mapsize[0], a->mapsize[1], a->mapsize[2], a->mapsize[3],
			a->mapview[0], a->mapview[1], a->mapview[2], a->mapview[3],
			a->mapwind[0], a->mapwind[1], a->mapwind[2], a->mapwind[3]);
		else
			fprintf(fp, "NULL},\n");
	}
	fprintf(fp, "};\n\n");

	/* polygon section */
	for(i = 0; i < suf->np; i++){
		if(suf->p[i]->t == suf_uvpoly || suf->p[i]->t == suf_uvshade){
			struct suf_uvpoly_t *uv = &suf->p[i]->uv;
			int j;
			if(uv->n < hackeds && hacked[uv->n] & 0x2)
				fprintf(fp, "static struct sufuvpolyhack_%s_%d", name, uv->n);
			else{
				fprintf(fp, "static SUFSRCUVPOLYHACK(%s,%d)", name, uv->n);
				hackadd(&hacked, &hackeds, uv->n)[uv->n] |= 0x2;
			}
			fprintf(fp, " suf_%s_prim%d = {\n"
				"%s, %d, %hu,{\n", name, i, elemtype(suf->p[i]->t), uv->n, uv->atr);
			for(j = 0; j < uv->n; j++)
				fprintf(fp, "{%hu, %hu, %hu},\n", uv->v[j].p, uv->v[j].n, uv->v[j].t);
			fprintf(fp,"}};\n");
		}
		else{
			struct suf_poly_t *p = &suf->p[i]->p;
			int j;
			fprintf(fp, p->n < hackeds && hacked[p->n] & 0x1 ?
				"static struct sufpolyhack_%s_%d" :
				(hackadd(&hacked, &hackeds, p->n)[p->n] |= 0x1, 
					"static SUFSRCPOLYHACK(%s,%d)"), name, p->n);
			fprintf(fp, " suf_%s_prim%d = {\n"
				"%s, %d, %hu,\n", name, i, elemtype(suf->p[i]->t), p->n, p->atr);
			for(j = 0; j < p->n; j++)
				fprintf(fp, "%hu, %hu,\n", p->v[j][0], p->v[j][1]);
			fprintf(fp,"};\n");
		}
	}

	/* polygon list object */
	fprintf(fp, "static union suf_prim_t *suf_%s_primlist[%d] = {\n",
		name, suf->np);
	for(i = 0; i < suf->np; i++){
		fprintf(fp, i == suf->np - 1 ? "\t(union suf_prim_t*)&suf_%s_prim%d\n};\n\n" : "\t(union suf_prim_t*)&suf_%s_prim%d,\n", name, i);
	}

	/* suf object */
	fprintf(fp, "SUF_LINKAGE suf_t suf_%s = {\n"
		"\t%d,\n"
		"\tsuf_%s_vertex,\n"
		"\t%d,\n"
		"\tsuf_%s_attribute,\n"
		"\t%d,\n"
		"\tsuf_%s_primlist\n"
		"};",
		name,
		suf->nv,
		name,
		suf->na,
		name,
		suf->np,
		name);

	realloc(hacked, 0);
}
