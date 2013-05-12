#ifndef DSTR_H
#define DSTR_H
/* dstr - dynamic string
  safe anti-fragmentation automatically-expanding string */
#include <stddef.h> /* size_t */

typedef struct dynamic_string{
	size_t size; /* doesn't include terminating null */
	char *s;
} dstr_t;

extern dstr_t dstr0;
#define dstr(ds) ((ds)->s)
#define dstrlen(ds) ((ds)->size)
char *dstralloc(dstr_t *dst, size_t sl);
#define dstrcmp(ds,cs) strncmp((ds)->s, cs, (ds)->size)
char *dstrncpy(dstr_t *dst, const char *src, size_t len);
char *dstrcpy(dstr_t *dst, const char *src);
char *dstrncat(dstr_t *dst, const char *src, size_t len);
char *dstrcat(dstr_t *dst, const char *src); /* concatenate another string */
char *dstrlong(dstr_t *dst, long src); /* concatenate a long value */
#define dstrfree(ds) (realloc((ds)->s, (ds)->size = 0), (ds)->s = NULL)

#endif
