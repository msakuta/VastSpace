#ifndef CLIB_ZIP_UNIFORMLOADER_H
#define CLIB_ZIP_UNIFORMLOADER_H

#define UL_BINARY 1
#define UL_TRYFILE 2

typedef struct UniformLoader UniformLoader;

UniformLoader *ULfopen(const char *fname, unsigned flags);
UniformLoader *ULzopen(const char *zipname, const char *entry, unsigned flags);
void ULclose(UniformLoader *ul);
char *ULfgets(char *buf, int sz, UniformLoader *in);

#endif
