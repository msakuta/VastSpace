#include <clib/zip/UniformLoader.h>
#include <clib/zip/UnZip.h>
#include <stdio.h>
#include <stdlib.h>

struct UniformLoader{
	int type;
	void *zipsrc;
	char *p;
	unsigned long sz;
	FILE *fp;
};

UniformLoader *ULfopen(const char *fname, unsigned flags){
	UniformLoader *ret;
	FILE *fp;
	char mode[3] = {'r', 't', '\0'};
	mode[1] = flags & UL_BINARY ? 'b' : 't';
	fp = fopen(fname, mode);
	if(!fp)
		return NULL;
	ret = (UniformLoader*)malloc(sizeof *ret);
	ret->type = 0;
	ret->fp = fp;
	return ret;
}

UniformLoader *ULzopen(const char *zipname, const char *entry, unsigned flags){
	UniformLoader *ret;
	void *pbuf;
	unsigned long fsize;

	/* If a normal file with the entry name exist, open that file in priority. */
	if(flags & UL_TRYFILE && (ret = ULfopen(entry, flags)))
		return ret;

	pbuf = ZipUnZip(zipname, entry, &fsize);
	if(!pbuf)
		return NULL;
	ret = (UniformLoader*)malloc(sizeof *ret);
	ret->type = 1;
	ret->p = (char*)(ret->zipsrc = pbuf);
	ret->sz = fsize;
	return ret;
}

void ULclose(UniformLoader *ul){
	if(!ul)
		return;
	switch(ul->type){
		case 0: fclose(ul->fp); break;
		case 1: ZipFree(ul->zipsrc); break;
	}
	free(ul);
}

char *ULfgets(char *buf, int sz, UniformLoader *in){
	if(in->type == 0)
		return fgets(buf, sz, in->fp);
	else{
		char *ret = buf;
		if(!in->sz)
			return NULL;
		for(; sz && in->sz; sz--, in->sz--){
			if(*in->p == '\r'){
				if(*(in->p+1) == '\n'){
					*buf = '\n';
					buf[!!sz] = '\0';
					in->p += 2;
					in->sz -= 2;
					return ret;
				}
				else
					goto normal_ret;
			}
			if(*in->p == '\n'){
	normal_ret:
				*buf++ = *in->p++;
				*buf = '\0';
				return ret;
			}
			*buf++ = *in->p++;
		}
		return ret;
	}
}
