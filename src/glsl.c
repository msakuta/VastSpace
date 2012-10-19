#include "glsl.h"
#include <clib/zip/UnZip.h>
#include <stdio.h>
#include <assert.h>

PFNGLCREATESHADERPROC pglCreateShader;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
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
PFNGLUNIFORM3FVPROC pglUniform3fv;
PFNGLUNIFORM4FVPROC pglUniform4fv;
PFNGLUNIFORMMATRIX3FVPROC pglUniformMatrix3fv;
PFNGLGETATTRIBLOCATIONPROC pglGetAttribLocation;
PFNGLVERTEXATTRIB3DVPROC pglVertexAttrib3dv;


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
	
	// AMD Radeon HD 6870 outputs log messages when the compilation succeeds.
	// This information is not much informative and takes much of console space
	// especially if there are many shaders.
	if (!compiled && logSize > 1)
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

/** Preprocess a file to include #include directives.
 * Currently the sharp in a string cannot be correctly ignored. */
static int preprocess_file(GLchar **buf, long *size){
	static const char *directive = "include";
	GLchar *scan = *buf;
	int direcpos = 0;
	GLchar *direcStart = NULL;
	int lineComment = 0;
	GLchar *commentStart = NULL;
	int ret = 0;

	for(; *scan; ++scan){
		// Ignore line comment
		if(*scan == '/'){
			++lineComment;
			if(2 == lineComment){
				while(*scan && *scan != '\n')
					++scan;
				// Reset state
				lineComment = 0;
			}
		}
		else
			lineComment = 0;

		// Find the sharp indicating the beginning of a directive.
		// TODO: Ignore strings and block comments
		if(!direcStart){
			if(*scan == '#'){
				direcStart = scan;
				direcpos = 0;
			}
		}
		else if(*scan == directive[direcpos]){
			// Scan the directive name
			++direcpos;

			// If directive name matches "include", proceed with file inclusion.
			if(directive[direcpos] == '\0'){
				FILE *fp;
				GLchar *fname;
				GLchar *buf2;
				long direcSize;
				long incSize;

				// Scan until the first delimiter denoting beginning of the file name.
				while(*scan != '"' && *scan != '<')
					++scan;
				// Remember the beginning of the file name.
				fname = ++scan;
				// Scan until the end.
				while(*scan != '"' && *scan != '>')
					++scan;
				// Null terminate the string to interpret the file name as a C string.
				*scan = '\0';
				// Even after the directive is processed, we must consume the stream until newline.
				while(*scan != '\n')
					++scan;
				// Calculate size of directive line.
				direcSize = scan - direcStart;

				// Try to open the included file.
				fp = fopen(fname, "rb");
				if(fp){
					fseek(fp, 0, SEEK_END);
					incSize = ftell(fp);
					fseek(fp, 0, SEEK_SET);
					buf2 = malloc(incSize);
					if(!buf2)
						ret = -1;
					else if(1 != fread(buf2, incSize, 1, fp))
						ret = -1;
					fclose(fp);
					if(ret < 0)
						return ret;
				}
				else if(buf2 = ZipUnZip("rc.zip", fname, &incSize)){
				}
				else{
					fprintf(stderr, "GLSL preprocessor error: include file \"%s\" is not found.\n", fname);
					return -1;
				}
				{
					// Remember sizes before reallocating the buffer, because it can move the buffer on the memory.
					long direcOffset = direcStart - *buf;
					long scanOffset = scan - *buf;
					long newSize = *size + incSize - (scanOffset - direcOffset);

					// Reallocate and possibly move the buffer.
					*buf = realloc(*buf, newSize);
					// Move the later section of the original file to end of the included file.
					memmove(&(*buf)[direcOffset + incSize], &(*buf)[scanOffset], *size - scanOffset);
					// Ember the included file in the place of the directive.
					memcpy(&(*buf)[direcOffset], buf2, incSize);
					// Update the size.
					*size = newSize;
					// Free the appropriate buffer.
					if(fp)
						free(buf2);
					else
						ZipFree(buf2);
					// Scan pointer should point to the reallocated buffer.
					scan = &(*buf)[direcOffset];
				}
				// Reset the state.
				direcStart = NULL;
			}
		}
		else if(direcpos != 0){
			// Non-matched directive name; shouldn't happen
			// TODO: Helpful error messages.
			assert(0);
			direcStart = NULL;
		}
		else if(*scan == '\n')
			direcStart = NULL;
	}
	return ret;
}

/** Load a GLSL source from a file.
 * If the file contains #include preprocessor directives, they will be expanded. */
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
		if(buf){
			fread(buf, size, 1, fp);
			buf[size++] = '\0';
		}
		fclose(fp);
	}
	else if(buf = ZipUnZip("rc.zip", fname, &size)){
		buf[size-1] = '\0';
	}
	else
		return 0;
	if(buf){
		int prepResult;
		prepResult = preprocess_file(&buf, &size);
		if(prepResult < 0){
			ret = 0;
			fprintf(stderr, "GLSL preprocessing error in \"%s\".\n", fname);
		}
		else
			ret = glsl_register_shader(shader, buf);
		if(fp)
			free(buf);
		else
			ZipFree(buf);
	}
	return ret;
}

static void printProgramInfoLog(GLuint shader){
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
GLuint glsl_register_program(const GLuint *shaders, int nshaders){
	GLuint prog;
	GLint linked;
	int i;
    prog = glCreateProgram();

	for(i = 0; i < nshaders; i++){
	    glAttachShader(prog, shaders[i]);
//	    glDeleteShader(shaders[i]); // Is this really necessary? The caller should be responsible for destructing them.
	}

    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);

	// AMD Radeon HD 6870 outputs log messages when the linking succeeds.
	// This information is not much informative and takes much of console space
	// especially if there are many shaders.
	if (linked == GL_FALSE){
		printProgramInfoLog(prog);
        return 0;
	}
	return prog;
}

static int glsl_registered = 0;

#define LoadGlExt(var,type) ((var)=(type)wglGetProcAddress(#var))

void glsl_register(){
	glsl_registered = 1;
	pglCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
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
	pglUniform3fv = (void*)wglGetProcAddress("glUniform3fv");
	pglUniform4fv = (void*)wglGetProcAddress("glUniform4fv");
	pglGetUniformLocation = (void*)wglGetProcAddress("glGetUniformLocation");
	pglUniformMatrix3fv = (void*)wglGetProcAddress("glUniformMatrix3fv");
	pglGetAttribLocation = (void*)wglGetProcAddress("glGetAttribLocation");
	pglVertexAttrib3dv = (void*)wglGetProcAddress("glVertexAttrib3dv");
	vrc_shader_enable(&g_shader_enable);
}

int vrc_shader_enable(int *pi){
	if(glsl_registered && !pglCreateShader)
		*pi = 0;
	return 0;
}
