#include "clib/suf/sufbin.h"
#include <string.h>
#include <stddef.h>

suf_t *RelocateSUF(suf_t *src){
	int i;
	src->v = (sufcoord(*)[3])((char*)src + (long)src->v + sizeof(suf_t) - 6 * 4);
	src->a = (struct suf_atr_t*)((char*)src + (long)src->a + sizeof(suf_t) - 6 * 4);
	for(i = 0; i < src->na; i++){
		if(src->a[i].name)
			src->a[i].name = (char*)src + (long)src->a[i].name + sizeof(suf_t) - 6 * 4;
		if(src->a[i].colormap)
			src->a[i].colormap = (char*)src + (long)src->a[i].colormap + sizeof(suf_t) - 6 * 4;
	}
	src->p = (union suf_prim_t**)((char*)src + (long)src->p) + sizeof(suf_t) - 6 * 4;
	for(i = 0; i < src->np; i++){
		src->p[i] = (union suf_prim_t*)((char*)src + (long)src->p[i]) + sizeof(suf_t) - 6 * 4;
	}
	return src;
}

struct UnserializeSUFInput{
	const void *src;
	size_t size;
	size_t pos;
};

static size_t intRead(int *pp, struct UnserializeSUFInput *in){
	if(pp)
		*pp = *(int*)&((char*)in->src)[in->pos];
	in->pos += 4;
	return in->pos;
}

static size_t pointerRead(void **pp, struct UnserializeSUFInput *in){
	*pp = (void*)*(int*)&((char*)in->src)[in->pos];
	in->pos += 4; // Pointers are stored as int32 in a serialized file.
	return in->pos;
}

static size_t structRead(void *pp, struct UnserializeSUFInput *in, size_t sz){
	memcpy(pp, &((char*)in->src)[in->pos], sz);
	in->pos += sz;
	return in->pos;
}

suf_t *UnserializeSUF(const void *src, size_t size){
	suf_t suf;
	suf_t *ret;
	struct UnserializeSUFInput in;
	int i;
	size_t stroffset;
	size_t polyoffset;

	in.src = src;
	in.size = size;
	in.pos = 0;
	intRead(&suf.nv, &in);
	pointerRead(&suf.v, &in);
	intRead(&suf.na, &in);
	pointerRead(&suf.a, &in);
	intRead(&suf.np, &in);
	pointerRead(&suf.p, &in);
	{
		size_t strsize = 0;
		size_t polysize = 0;
		for(i = 0; i < suf.na; i++){
			const char *pstr = &((const char*)src)[*(unsigned*)&((const char*)src)[(size_t)suf.a + 52 * 4 * i]];
			if(pstr)
				strsize += strlen(pstr) + 1;
			pstr = &((const char*)src)[*(unsigned*)&((const char*)src)[(size_t)suf.a + 52 * 4 * i + 26 * 4]];
			if(pstr)
				strsize += strlen(pstr) + 1;
		}
		for(i = 0; i < suf.np; i++){
			union suf_prim_t *pprim = &((const char*)src)[*(unsigned*)&((const char*)src)[(size_t)suf.p + 4 * i]];
			polysize += pprim->t == suf_poly ?
				offsetof(struct suf_poly_t, v) + pprim->p.n * sizeof pprim->p.v :
				offsetof(struct suf_uvpoly_t, v) + pprim->uv.n * sizeof pprim->uv.v;
		}
		polyoffset = sizeof suf + suf.nv * sizeof*suf.v + suf.na * sizeof*suf.a + suf.np * sizeof*suf.p;
		stroffset = polyoffset + polysize;
		ret = malloc(stroffset + strsize);
	}
	*ret = suf;
	{
		ret->v = (sufcoord(*)[3])((char*)ret + (long)ret->v + sizeof(suf_t) - 6 * 4);
		memcpy(ret->v, &((char*)in.src)[in.pos], ret->nv * sizeof *ret->v);
	}

	// Attribute section includes strings which are allocated in the last part of the aggrigate.
	{
		ret->a = (struct suf_atr_t*)((char*)ret + (long)ret->a + sizeof(suf_t) - 6 * 4);
		in.pos = (size_t)suf.a;
		for(i = 0; i < ret->na; i++){
			pointerRead(&ret->a[i].name, &in);
			if(ret->a[i].name){
				const char *pstr = &((const char*)src)[(size_t)ret->a[i].name];
				ret->a[i].name = (char*)ret + stroffset;
				strcpy(ret->a[i].name, pstr);
				stroffset += strlen(pstr) + 1;
			}
			intRead(&ret->a[i].valid, &in);
			structRead(ret->a[i].col, &in, sizeof ret->a[i].col);
			structRead(ret->a[i].tra, &in, sizeof ret->a[i].tra);
			structRead(ret->a[i].amb, &in, sizeof ret->a[i].amb);
			structRead(ret->a[i].emi, &in, sizeof ret->a[i].emi);
			structRead(ret->a[i].dif, &in, sizeof ret->a[i].dif);
			structRead(ret->a[i].spc, &in, sizeof ret->a[i].spc);
			pointerRead(&ret->a[i].colormap, &in);
			if(ret->a[i].colormap){
				const char *pstr = &((const char*)src)[(size_t)ret->a[i].colormap];
				ret->a[i].colormap = (char*)ret + stroffset;
				strcpy(ret->a[i].colormap, pstr);
				stroffset += strlen(pstr) + 1;
			}
			intRead(NULL, &in); // Padding in serialized stream should never exist.
			structRead(ret->a[i].mapsize, &in, sizeof ret->a[i].mapsize);
			structRead(ret->a[i].mapview, &in, sizeof ret->a[i].mapview);
			structRead(ret->a[i].mapwind, &in, sizeof ret->a[i].mapwind);
		}
	}

	// Polygon section has variable-length elements, so we accumulate them.
	{
		ret->p = (union suf_prim_t**)((char*)ret + sizeof suf + suf.nv * sizeof*suf.v + suf.na * sizeof*suf.a);
		in.pos = (size_t)suf.p;
		for(i = 0; i < ret->np; i++){
			const union suf_prim_t *pprim;
			int srcoffset;
			size_t size;
			intRead(&srcoffset, &in);
			pprim = (const union suf_prim_t*)&((const char*)src)[srcoffset];
			ret->p[i] = (union suf_prim_t*)((char*)ret + polyoffset);
			size = pprim->t == suf_poly ?
				offsetof(struct suf_poly_t, v) + pprim->p.n * sizeof pprim->p.v :
				offsetof(struct suf_uvpoly_t, v) + pprim->uv.n * sizeof pprim->uv.v;
			memcpy(ret->p[i], pprim, size);
			polyoffset += size;
		}
	}
	return ret;
}
