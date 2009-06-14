#include "clib/suf/sufbin.h"

suf_t *RelocateSUF(suf_t *src){
	int i;
	src->v = (sufcoord(*)[3])((char*)src + (long)src->v);
	src->a = (struct suf_atr_t*)((char*)src + (long)src->a);
	for(i = 0; i < src->na; i++){
		if(src->a[i].name)
			src->a[i].name = (char*)src + (long)src->a[i].name;
		if(src->a[i].colormap)
			src->a[i].colormap = (char*)src + (long)src->a[i].colormap;
	}
	src->p = (union suf_prim_t**)((char*)src + (long)src->p);
	for(i = 0; i < src->np; i++){
		src->p[i] = (union suf_prim_t*)((char*)src + (long)src->p[i]);
	}
	return src;
}
