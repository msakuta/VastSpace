#ifndef CLIB_CIRCUT_H
#define CLIB_CIRCUT_H

extern double (*CircleCuts(int n))[2];
extern double (*CircleCutsPartial(int n, int nmax))[2];
extern size_t CircleCutsMemory(void);

#endif
