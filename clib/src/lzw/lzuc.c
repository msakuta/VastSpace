#ifdef _WIN32
#include <windows.h>
#endif
#include "clib/lzw/lzw.h"
/*#include <stdlib.h>*/

typedef struct{
	unsigned char v;
	short p;
} lztable;

static int initable(lztable *table, int *bits){
	int i;
	for(i = 0; i < 256; i++){
		table[i].v = i;
		table[i].p = -1;
	}
/*	table[i].v = 0;
	table[i].p = 0;
	i++;
	table[i].v = 0;
	table[i].p = i-1;
	i++;*/
	*bits = 8;
	return i;
}

static unsigned char last(lztable *table, int i){
	while(0 <= table[i].p)
		i = table[i].p;
	return i;
}

static void writestring(lztable table[], int buf, unsigned char **pret, size_t *osz, size_t *as){
	int j, n;
		for(j = buf, n = 1;; j = table[j].p, n++){
			if(table[j].p < 0)
				break;
		}
/*		printf("[%d] n = %d\n", buf, n);*/
		if(*as <= *osz + n)
			*pret = (unsigned char*)realloc(*pret, *as += n);
		*osz += n;
		for(j = buf;; j = table[j].p){
			--*osz;
/*			printf("(%d)%d ", j, table[j].v);*/
			(*pret)[*osz] = table[j].v;
			if(table[j].p < 0)
				break;
		}
		*osz += n;

}

static unsigned long readvl(const unsigned char *buf, long bp, long wid){
	unsigned val = 0, vbp = 0;
/*	printf("%d %d ", bp, wid);*/
	while(0 < wid){
		long bits = 8 - bp % 8;
		if(wid < bits) bits = wid;
		val |= ((buf[bp / 8] >> bp % 8) & ((1 << bits) - 1)) << vbp;
		vbp += bits;
		wid -= bits;
		bp += bits;
	}
/*	printf("%d\n", val);*/
	return val;
}

unsigned char *lzuc(const unsigned char *in, size_t sz, size_t *retsz){
	lztable table[4096];
	unsigned char *ret;
	size_t osz = 0, as;
	int i, n = 2;
	int buf, bp = 0, bits;
//	HLOCAL hl;

/*	printf("in size %d\n", sz);*/

	i = initable(table, &bits);

	/* uncompressed size varies, but unlikely reachs an effeciency of 1/20. */
	ret = malloc(as = sz * 20);
/*	ret = LocalLock(hl = LocalAlloc(LMEM_MOVEABLE, as = sz * 20));*/

/*	buf = *(short*)in;
	in += sizeof(short);
	sz -= sizeof(short);*/
	buf = readvl(in, bp, bits);
	bp += bits;

	while(bp + bits <= sz * 8){
		int pre;

		if(4096 <= i)
			i = initable(table, &bits);

/*		pre = *(short*)in;
		in += sizeof(short);
		sz -= sizeof(short);*/
		pre = readvl(in, bp, bits);
		bp += bits;

		for(bits = 0; i >> bits; bits++);

		table[i].v = last(table, pre);
		table[i].p = buf;
/*		printf("pre = %d, i = %d, v = %d, p = %d, bp = %d\n", pre, i, table[i].v, table[i].p, bp);*/
		i++;

		writestring(table, buf, &ret, &osz, &as);

		buf = pre;
	}

	writestring(table, buf, &ret, &osz, &as);

/*	printf("out size %d\n", osz);*/

	if(retsz)
		*retsz = osz;

	/* no use keeping extra working memory. */
	ret = (unsigned char*)realloc(ret, osz);
/*	ret = LocalReAlloc(ret, osz, 0);*/

	return ret;
}

