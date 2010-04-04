#include "glsl.h"
#include <clib/zip/UnZip.h>
#include <stdio.h>

PFNGLCREATESHADERPROC pglCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLUSEPROGRAMPROC pglUseProgram;
PFNGLGETUNIFORMLOCATIONPROC pglGetUniformLocation;
PFNGLUNIFORM1IPROC pglUniform1i;
PFNGLUNIFORM1FPROC pglUniform1f;
PFNGLUNIFORM4FVPROC pglUniform4fv;
PFNGLUNIFORMMATRIX3FVPROC pglUniformMatrix3fv;


int g_shader_enable = 1;


/* compile a shader with directly giving source text. */
int glsl_register_shader(GLuint shader, const char *src){
	GLint compiled;
	GLint logSize;
	if(shader == 0)
		return 0;
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH , &logSize);
	if (logSize > 1)
	{
		GLsizei length;
		char *buf;
		buf = malloc(logSize);
		glGetShaderInfoLog(shader, logSize, &length, buf);
		fprintf(stderr, "Shader Info Log\n%s\n", buf);
		free(buf);
    }
	return compiled;
}

/* load from a file. */
int glsl_load_shader(GLuint shader, const char *fname){
	FILE *fp;
	long size;
	int ret = 0;
	GLchar *buf;
	fp = fopen(fname, "rb");
	if(fp){
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		buf = malloc(size+1);
		if(!buf)
			goto finish;
		fread(buf, size, 1, fp);
		buf[size] = '\0';
	}
	else if(buf = ZipUnZip("rc.zip", fname, &size)){
		buf[size-1] = '\0';
	}
	else
		return 0;
	if(buf){
		ret = glsl_register_shader(shader, buf);
		if(fp)
			free(buf);
		else
			ZipFree(buf);
	}
finish:
	if(fp)
		fclose(fp);
	return ret;
}

static void printProgramInfoLog(   GLuint shader
)
{
    int logSize;
    int length;

    glGetProgramiv(shader, GL_INFO_LOG_LENGTH , &logSize);

    if (logSize > 1)
    {
		char *buf;
		buf = malloc(logSize);
		glGetProgramInfoLog(shader, logSize, &length, buf);
        fprintf(stderr, "Program Info Log\n%s\n", buf);
		free(buf);
    }
}

/* Link a shader program with given vertex shader and fragment shader. */
GLuint glsl_register_program(GLuint vtx, GLuint frg){
	GLuint prog;
	GLint linked;
    prog = glCreateProgram();

    glAttachShader(prog, vtx);
    glAttachShader(prog, frg);

    glDeleteShader(vtx);
    glDeleteShader(frg);

    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
	printProgramInfoLog(prog);
    if (linked == GL_FALSE)
        return 0;
	return prog;
}

static int glsl_registered = 0;

#define LoadGlExt(var,type) ((var)=(type)wglGetProcAddress(#var))

void glsl_register(){
	glsl_registered = 1;
	pglCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
	LoadGlExt(glCreateProgram, PFNGLCREATEPROGRAMPROC);
	LoadGlExt(glAttachShader, PFNGLATTACHSHADERPROC);
	LoadGlExt(glDeleteShader, PFNGLDELETESHADERPROC);
	LoadGlExt(glLinkProgram, PFNGLLINKPROGRAMPROC);
	LoadGlExt(glGetProgramiv, PFNGLGETPROGRAMIVPROC);
	LoadGlExt(glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC);
	pglUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	pglUniform1i = (void*)wglGetProcAddress("glUniform1i");
	pglUniform1f = (void*)wglGetProcAddress("glUniform1f");
	pglUniform4fv = (void*)wglGetProcAddress("pglUniform4fv");
	pglGetUniformLocation = (void*)wglGetProcAddress("glGetUniformLocation");
	pglUniformMatrix3fv = (void*)wglGetProcAddress("glUniformMatrix3fv");
	vrc_shader_enable(&g_shader_enable);
}

int vrc_shader_enable(int *pi){
	if(glsl_registered && !pglCreateShader)
		*pi = 0;
	return 0;
}
