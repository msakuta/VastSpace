#ifndef GLEXTCALL_H
#define GLEXTCALL_H
#ifdef _WIN32
#include <windows.h>
#endif
#include <gl/gl.h>
#include <gl/glext.h>

extern PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
extern PFNGLMULTITEXCOORD2DARBPROC glMultiTexCoord2dARB;
extern PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
extern PFNGLMULTITEXCOORD1FARBPROC glMultiTexCoord1fARB;

#endif
