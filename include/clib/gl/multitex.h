#ifndef CLIB_GL_MULTITEX_H
#define CLIB_GL_MULTITEX_H
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>

extern PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
extern PFNGLMULTITEXCOORD3DARBPROC glMultiTexCoord3dARB;
extern PFNGLMULTITEXCOORD3DVARBPROC glMultiTexCoord3dvARB;
extern PFNGLMULTITEXCOORD2DARBPROC glMultiTexCoord2dARB;
extern PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
extern PFNGLMULTITEXCOORD1FARBPROC glMultiTexCoord1fARB;

void MultiTextureInit();

#endif
