#include "clib/lzw/lzw.h"

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

static int find(lztable *table, int tsz, const unsigned char *in, int n){
	int j;
	for(j = 0; j < tsz; j++){
		int l = 0;
		int k = j;
		do{
/*			printf("j = %d, l = %d, k = %d, in[-l] = %d, table[k].v = %d\n",
				j, l, k, in[-l], table[k].v);*/
/*			if(tsz == 500)
				printf("%d, %d, %d, %d\n", l, k, table[k].v, in[-l]);*/
			if(in[-l] != table[k].v){
				l = 0;
				break;
			}
			l++;
			if(table[k].p < 0){
				break;
			}
			k = table[k].p;
		} while(1);
		if(n == l){
			return j;
		}
	}
	return -1;
}

static void writevl(unsigned char *buf, size_t sz, long bp, long wid, unsigned long val){
	int byp, ebp;
	byp = bp / 8;
	ebp = bp + wid;
	val &= ((1 << wid) - 1);
/*	printf("%d %d %d\n", bp, wid, val);*/
	while(byp * 8 < ebp){
		buf[byp] = buf[byp] & ~(((1 << wid) - 1) << (bp % 8)) | (val << (bp % 8));
		val >>= (8 - bp % 8);
		wid -= 8 - bp % 8;
		bp += 8 - bp % 8;
		byp++;
	}
}


unsigned char *lzc(const unsigned char *in, size_t sz, size_t *retsz){
	lztable table[4096];
	unsigned char *ret;
	size_t osz = 0;
	int i, n = 2, nexi, bits;
	nexi = i = initable(table, &bits);
	ret = (unsigned char*)malloc(sz * 8);
/*	printf("sz = %d\n", sz);*/
	while(sz){
		int j, n, pre;
		j = *in;
		n = sz == 1 ? 1 : 2;
		do{
			int z;
			pre = j;
			j = find(table, i, &in[n-1], n);
/*			for(z = 0; z < n; z++)
				printf("%d ", in[z]);
			printf("j = %d, pre = %d\n", j, pre);*/
			n++;
			if(0 <= j && sz < n)
				break;
		} while(0 <= j);

		if(0 <= j){
/*			*(short*)&ret[osz] = j;
			osz += sizeof(short);*/
			writevl(ret, 0, osz, bits, j);
			osz += bits;
			break;
		}

		for(bits = 0; (i-1) >> bits; bits++);
/*		printf("i/bits %d %d\n", i, bits);*/

/*		*(short*)&ret[osz] = pre;
		osz += sizeof(short);*/
		writevl(ret, 0, osz, bits, pre);
		osz += bits;

		i = nexi;

		if(4096 <= nexi)
			nexi = i = initable(table, &bits);

/*		printf("table[%d]: v = %d, p = %d, sz = %d, osz = %d\n", i, in[n-2], pre, sz, osz);*/
		table[i].v = in[n-2];
		table[i].p = pre;

		nexi++;
		in += n-2;
		sz -= n-2;
	}
	*retsz = (osz + 7) / 8;
	return ret;
}

