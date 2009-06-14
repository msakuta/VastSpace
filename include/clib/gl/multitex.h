#ifndef CLIB_GL_MULTITEX_H
#define CLIB_GL_MULTITEX_H
#include <GL/gl.h>
#include <GL/glext.h>

extern PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
extern PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
extern PFNGLMULTITEXCOORD1FARBPROC glMultiTexCoord1fARB;

void MultiTextureInit();

#endif
