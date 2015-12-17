#ifndef GLSL_H
#define GLSL_H
#include "export.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>

#ifdef __cplusplus
extern "C"{
#endif

EXPORT extern PFNGLCREATESHADERPROC pglCreateShader;
EXPORT extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
EXPORT extern PFNGLSHADERSOURCEPROC glShaderSource;
EXPORT extern PFNGLCOMPILESHADERPROC glCompileShader;
EXPORT extern PFNGLGETSHADERIVPROC glGetShaderiv;
EXPORT extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
EXPORT extern PFNGLCREATEPROGRAMPROC glCreateProgram;
EXPORT extern PFNGLATTACHSHADERPROC glAttachShader;
EXPORT extern PFNGLDELETESHADERPROC glDeleteShader;
EXPORT extern PFNGLLINKPROGRAMPROC glLinkProgram;
EXPORT extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
EXPORT extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
EXPORT extern PFNGLUSEPROGRAMPROC pglUseProgram;
EXPORT extern PFNGLGETUNIFORMLOCATIONPROC pglGetUniformLocation;
EXPORT extern PFNGLUNIFORM1IPROC pglUniform1i;
EXPORT extern PFNGLUNIFORM1FPROC pglUniform1f;
EXPORT extern PFNGLUNIFORM3FVPROC pglUniform3fv;
EXPORT extern PFNGLUNIFORM4FVPROC pglUniform4fv;
EXPORT extern PFNGLUNIFORMMATRIX3FVPROC pglUniformMatrix3fv;
EXPORT extern PFNGLUNIFORMMATRIX4FVPROC pglUniformMatrix4fv;
EXPORT extern PFNGLGETATTRIBLOCATIONPROC pglGetAttribLocation;
EXPORT extern PFNGLVERTEXATTRIB3DVPROC pglVertexAttrib3dv;

/* Hopefully link error will occur if forgot to include this header. */
#define glCreateShader(shader) (pglCreateShader ? pglCreateShader(shader) : 0)
#define glUseProgram(prog) (pglUseProgram ? pglUseProgram(prog) : 0)
#define glUniform1i(a,b) (pglUniform1i ? pglUniform1i(a,b) : 0)
#define glUniform1f(a,b) (pglUniform1f ? pglUniform1f(a,b) : 0)
#define glUniform3fv(a,c,b) (pglUniform3fv ? pglUniform3fv(a,c,b) : 0)
#define glUniform4fv(a,c,b) (pglUniform4fv ? pglUniform4fv(a,c,b) : 0)
#define glGetUniformLocation(a,b) (pglGetUniformLocation ? pglGetUniformLocation(a,b) : -1)
#define glUniformMatrix3fv(a,b,c,d) (pglUniformMatrix3fv ? pglUniformMatrix3fv(a,b,c,d) : 0)
#define glUniformMatrix4fv(a,b,c,d) (pglUniformMatrix4fv ? pglUniformMatrix4fv(a,b,c,d) : 0)
#define glGetAttribLocation(a,b) (pglGetAttribLocation ? pglGetAttribLocation(a,b) : -1)
#define glVertexAttrib3dv(a,b) (pglVertexAttrib3dv ? pglVertexAttrib3dv(a,b) : 0)

EXPORT int glsl_register_shader(GLuint shader, const char *src, const char **srcFileNames, int numSrcFileNames);
EXPORT int glsl_load_shader(GLuint shader, const char *fname);
EXPORT GLuint glsl_register_program(const GLuint *shaders, int nshaders);
EXPORT void glsl_register();

EXPORT int vrc_shader_enable(int *);
EXPORT extern int g_shader_enable;
EXPORT extern int g_shader_preprocess_out;

#ifdef __cplusplus
}
#endif

#endif
