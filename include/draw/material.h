/** \file
 * \brief Defines material handling, OpenGL texture and lighting model.
 */
#ifndef MATERIAL_H
#define MATERIAL_H
#include "export.h"
#ifdef __cplusplus
extern "C"{
#endif
#include <clib/suf/sufdraw.h>
#ifdef __cplusplus
}
#endif

EXPORT void AddMaterial(const char *name, const char *texname1, suftexparam_t *stp1, const char *texname2, suftexparam_t *stp2);
EXPORT void CacheSUFMaterials(const suf_t *suf);
EXPORT extern unsigned CallCacheBitmap(const char *entry, const char *fname1, suftexparam_t *pstp, const char *fname2);
EXPORT unsigned CallCacheBitmap5(const char *entry, const char *fname1, suftexparam_t *pstp1, const char *fname2, suftexparam_t *pstp2);
EXPORT suf_t *CallLoadSUF(const char *fname);

#ifdef __cplusplus
//}
#endif
#endif
