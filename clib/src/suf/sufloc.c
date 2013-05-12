#include "clib/suf/sufbin.h"
#include "clib/suf/suf.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

size_t DislocateSUF(void **buf, const suf_t *src){
	suf_t *ret;
	char *pos;
	size_t size;
	int i;

	/* first pass counts up all bytes necessary */
	size = sizeof *src;
	size += src->nv * sizeof *src->v;
	size += src->na * sizeof *src->a;
	for(i = 0; i < src->na; i++)
		size += (src->a[i].name ? strlen(src->a[i].name) + 1 : 0) + (src->a[i].colormap ? strlen(src->a[i].colormap) + 1 : 0);
	size += src->np * sizeof *src->p + sizeof(int) - 1;
	for(i = 0; i < src->np; i++){
		union suf_prim_t *prm = src->p[i];
		size += prm->t == suf_poly || prm->t == suf_shade ? offsetof(union suf_prim_t, p.v) + prm->p.n * sizeof(prm->p.v[0]) : offsetof(union suf_prim_t, uv.v) + prm->uv.n * sizeof(prm->uv.v[0]);
	}

	ret = malloc(size);
	memcpy(ret, src, sizeof *src);
	ret->v = (sufcoord(*)[3])(&ret[1]);
	memcpy(ret->v, src->v, ret->nv * sizeof *ret->v);
	ret->a = (struct suf_atr_t*)(&ret->v[ret->nv]);
	memcpy(ret->a, src->a, ret->na * sizeof *ret->a);
	pos = (char*)(&ret->a[ret->na]);
	for(i = 0; i < src->na; i++){

		/* strings never be at offset 0 so it's safe to make it identical to null pointer. */
		if(ret->a[i].name){
			ret->a[i].name = (char*)(pos - (char*)ret);
			strcpy(pos, src->a[i].name);
			pos += strlen(pos) + 1;
		}
		if(ret->a[i].colormap){
			ret->a[i].colormap = (char*)(pos - (char*)ret);
			strcpy(pos, src->a[i].colormap);
			pos += strlen(pos) + 1;
		}
	}

	/* align integer boundary to get faster processing */
	ret->p = (union suf_prim_t**)(((long)pos + sizeof(int) - 1) / 4 * 4);
	pos = (char*)&ret->p[ret->np];
	for(i = 0; i < src->np; i++){
		union suf_prim_t *prm = src->p[i];
		size_t s;
		s = prm->t == suf_poly || prm->t == suf_shade ? offsetof(union suf_prim_t, p.v) + prm->p.n * sizeof(prm->p.v[0]) : offsetof(union suf_prim_t, uv.v) + prm->uv.n * sizeof(prm->uv.v[0]);
		ret->p[i] = (union suf_prim_t*)(pos - (char*)ret);
		memcpy(pos, prm, s);
		pos += s;
	}
	ret->v = (sufcoord(*)[3])((char*)ret->v - (char*)ret);
	ret->a = (struct suf_atr_t*)((char*)ret->a - (char*)ret);
	ret->p = (union suf_prim_t**)((char*)ret->p - (char*)ret);
	*buf = (void*)ret;
	return pos - (char*)ret;
}

