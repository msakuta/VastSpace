#include "clib/dstr.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define alunit 0x80 /* allocation unit */
#define alsize(s) ((((s) + alunit - 1) / alunit) * alunit) /* allocated size */

dstr_t dstr0 = {NULL, 0};

char *dstralloc(dstr_t *dst, size_t sl){
	assert(dst);
	if(alsize(dst->size) < sl){ /* expand if necessary */
		dst->s = realloc(dst->s, alsize(sl) + 1);
		if(dst->size && !dst->s){
			dst->size = 0;
			return dst->s = NULL;
		}
		else
			dst->s[sl] = '\0';
	}
	dst->size = sl;
	return dst->s;
}

/* safe anti-fragmentation automatically-expanding string copy */
char *dstrcpy(dstr_t *dst, const char *src){
	size_t sl;
	assert(dst && src);
	sl = strlen(src);
	if(alsize(dst->size) < sl){ /* expand if necessary */
		dst->s = realloc(dst->s, alsize(sl) + 1);
		if(dst->size && !dst->s){
			dst->size = 0;
			return dst->s = NULL;
		}
	}
	strcpy(dst->s, src);
	dst->size = sl;
	return dst->s;
}

/* safe anti-fragmentation automatically-expanding string concatenator */
char *dstrcat(dstr_t *dst, const char *src){
	return dstrncat(dst, src, strlen(src));
}

char *dstrncat(dstr_t *dst, const char *src, size_t len){
	size_t sl;
	if(!src) return dst->s;
	assert(dst && src);
	sl = strlen(src);
	if(len < sl)
		sl = len;
	if(!sl)
		return dst->s;
	if(alsize(dst->size) < dst->size + sl){ /* expand if necessary */
		dst->s = realloc(dst->s, alsize(dst->size + sl) + 1);
		if(dst->size && !dst->s){
			dst->size = 0;
			return dst->s = NULL;
		}
	}
	if(dst->size)
		strncat(dst->s, src, sl);
	else
		strncpy(dst->s, src, sl + 1);
	dst->size += sl;
	return dst->s;
}

/* safe replacement for sprintf */
char *dstrlong(dstr_t *dst, long src){
	char buf[1 + 8*sizeof src]; /* n-bit long's maximum string expression width is far less than 8*n, plus null */
	sprintf(buf, "%ld", src);
	return dstrcat(dst, buf);
}

