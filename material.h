#ifndef MATERIAL_H
#define MATERIAL_H
#ifdef __cplusplus
extern "C"{
#endif
#include <clib/suf/sufdraw.h>

void CacheSUFMaterials(const suf_t *suf);
extern unsigned CallCacheBitmap(const char *entry, const char *fname1, suftexparam_t *pstp, const char *fname2);
unsigned CallCacheBitmap5(const char *entry, const char *fname1, suftexparam_t *pstp1, const char *fname2, suftexparam_t *pstp2);
suf_t *CallLoadSUF(const char *fname);

#ifdef __cplusplus
}
#endif
#endif
