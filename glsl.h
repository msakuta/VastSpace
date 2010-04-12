#ifndef GLSL_H
#define GLSL_H
#ifdef _WIN32
#include <windows.h>
#endif
#include <gl/gl.h>
#include <gl/glext.h>

#ifdef __cplusplus
extern "C"{
#endif

extern PFNGLCREATESHADERPROC pglCreateShader;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLDELETESHADERPROC glDeleteShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLUSEPROGRAMPROC pglUseProgram;
extern PFNGLGETUNIFORMLOCATIONPROC pglGetUniformLocation;
extern PFNGLUNIFORM1IPROC pglUniform1i;
extern PFNGLUNIFORM1FPROC pglUniform1f;
extern PFNGLUNIFORM4FVPROC pglUniform4fv;
extern PFNGLUNIFORMMATRIX3FVPROC pglUniformMatrix3fv;
extern PFNGLGETATTRIBLOCATIONPROC pglGetAttribLocation;
extern PFNGLVERTEXATTRIB3DVPROC pglVertexAttrib3dv;

/* Hopefully link error will occur if forgot to include this header. */
#define glCreateShader(shader) (pglCreateShader ? pglCreateShader(shader) : 0)
#define glUseProgram(prog) (pglUseProgram ? pglUseProgram(prog) : 0)
#define glUniform1i(a,b) (pglUniform1i ? pglUniform1i(a,b) : 0)
#define glUniform1f(a,b) (pglUniform1f ? pglUniform1f(a,b) : 0)
#define glUniform4fv(a,c,b) (pglUniform4fv ? pglUniform4fv(a,c,b) : 0)
#define glGetUniformLocation(a,b) (pglGetUniformLocation ? pglGetUniformLocation(a,b) : 0)
#define glUniformMatrix3fv(a,b,c,d) (pglUniformMatrix3fv ? pglUniformMatrix3fv(a,b,c,d) : 0)
#define glGetAttribLocation(a,b) (pglGetAttribLocation ? pglGetAttribLocation(a,b) : 0)
#define glVertexAttrib3dv(a,b) (pglVertexAttrib3dv ? pglVertexAttrib3dv(a,b) : 0)

int glsl_register_shader(GLuint shader, const char *src);
int glsl_load_shader(GLuint shader, const char *fname);
GLuint glsl_register_program(GLuint vtx, GLuint frg);
void glsl_register();

int vrc_shader_enable(int *);
extern int g_shader_enable;

#ifdef __cplusplus
}
#endif

#endif