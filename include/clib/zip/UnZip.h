#ifndef CLIB_ZIP_UNZIP_H
#define CLIB_ZIP_UNZIP_H

void *ZipUnZip(const char *fname, const char *ename, unsigned long *psize);
void ZipFree(void *pv);

#endif
