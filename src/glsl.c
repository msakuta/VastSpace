#include "glsl.h"
#include <clib/zip/UnZip.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

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
int g_shader_preprocess_out = 0;


/// @brief compile a shader with directly giving source text.
/// @param shader Allocated shader unit to bind the shader.
/// @param src Source text
/// @param srcFileNames A list of file names for outputting debug log (in logs/shader.log).
///        Since GLSL has no concept of file names, warning and error log messages are printed
///        with just the source string numbers.  A source string number is just a plain integer
///        that distinguishes sources, which is difficult to utilize for debugging by itself.
///        This function outputs a source file name table that maps the source string number
///        to the file name, which is given by this parameter.
///        Note that replacing source string numbers with file names in log messages is not
///        technically impossible but extremely difficult since different OpenGL driver vendors
///        have different message formats.  It's not worth trying.
/// @param numSrcFileNames Number of elements in srcFileNames array.
int glsl_register_shader(GLuint shader, const char *src, const char **srcFileNames, int numSrcFileNames){
	GLint compiled;
	GLint logSize;
	if(shader == 0)
		return 0;
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH , &logSize);
	
	if(logSize > 1){
		static int fileCleared = 0;
		FILE *fp;
		GLsizei length;
		char *buf;
		buf = (char*)malloc(logSize);
		if(!buf)
			return compiled;
		glGetShaderInfoLog(shader, logSize, &length, buf);

		// AMD Radeon HD 6870 and Intel HD graphics 4000 are confirmed but not necessarily the only
		// graphic processors that output log messages even if there are no errors or warnings.
		// This information is not much informative and takes much of console space
		// especially if there are many shaders, so output to console only if the compilation
		// itself failed.
		if(!compiled)
			fprintf(stderr, "Shader Info Log for %s\n%s\n", srcFileNames[0], buf);

		// Output to a log file regardless of whether the compilation succeeds or not.
		// It's useful for removing warnings if there are no errors.
		if(!fileCleared){
			// Clear the contents of the log file in the first invocation of this function
			// in an execution session in order to prevent the file from bloating endlessly.
			fp = fopen("logs/shader.log", "w");
			if(fp)
				fileCleared = 1;
		}
		else{
			// Append to the log file after the first output.
			fp = fopen("logs/shader.log", "a");
			fprintf(fp, "\n\n"); // Make some spaces from previous file entry
		}
		if(fp){
			int i;
			time_t timer;
			timer = time(NULL);
			fprintf(fp,
				"------------------------------------------\n"
				"  Shader compile info log\n");
			// Output the source file name table
			for(i = 0; i < numSrcFileNames; i++)
				fprintf(fp, "  File(%d): %s\n", i, srcFileNames[i]);
			fprintf(fp,
				"  Time: %s" // ctime() appends \n to the end of returned string
				"------------------------------------------\n"
				"%s", ctime(&timer), buf);
			fclose(fp);
		}
		free(buf);
	}
	return compiled;
}

/** @brief Preprocess a file to include #include directives.
 * @param buf A pointer to the pointer that holds source string as preprocessed output.  It can be realloc()-ed when
 *        there's a #include directive.
 * @param size A pointer to the variable that holds size of buf.  It can also be updated by #include interpretation.
 * @param scanStart Position in buf to start preprocessing.  The first invocation should pass 0 to this argument.
 *        It can be other values in recursive calls (which is triggered by #include interpretation).
 * @param scanEnd Position in buf to finish preprocessing.  The first invocation should pass *size to this argument.
 *        It can be other values in recursive calls (which is triggered by #include interpretation).
 * @param srcFileNames A pointer to the array of strings that holds source file names.  It can be realloc()-ed when
 *        #include directive is encountered.  Appended file names are malloc()-ed, so free()-ing them is responsibility
 *        of the caller.
 * @param numSrcFileNames A pointer to the variable that holds number of elements in srcFileNames array.
 *        It can also be updated by #include interpretation.
 *
 * The GLSL language specification omits #include because it have to support embedded processors
 * which could have no file system, but we want it for managing multiple shaders and split compiling.
 * Currently the sharp in a string cannot be correctly ignored.
 *
 * It is probably not worth noting that scanStart and scanEnd parameters are given as offset in buf instead of
 * pointers because buf can move around in memory address by realloc().
 */
static int preprocess_file(GLchar **buf, long *size, int scanStart, int scanEnd, char ***srcFileNames, int *numSrcFileNames){
	static const char *directive = "include";
	GLchar *scan = *buf + scanStart;
	int direcpos = 0;
	GLchar *direcStart = NULL;
	int lineComment = 0;
	GLchar *commentStart = NULL;
	int linenum = 0;
	int srcFileIdx = *numSrcFileNames - 1; // Remember this file's source file index before #include modifies numSrcFileNames
	int ret = 0;

	for(; *scan && scan - *buf < scanEnd; ++scan){
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

		if(*scan == '\n')
			++linenum;

		// Find the sharp indicating the beginning of a directive.
		// TODO: Ignore strings and block comments
		if(!direcStart){
			if(*scan == '#'){
				direcStart = scan;
				direcpos = 0;
			}
		}
		// Whitespaces between "#" and "include" are ignored.
		else if(*scan != '\n' && isspace(*scan) && direcpos == 0){
			// Do nothing
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
				*scan++ = '\0';
				// Even after the directive is processed, we must consume the stream until newline.
				while(*scan && *scan != '\n')
					++scan;
				// Calculate size of directive line.
				direcSize = scan - direcStart;

				// Try to open the included file.
				fp = fopen(fname, "rb");
				if(fp){
					fseek(fp, 0, SEEK_END);
					incSize = ftell(fp);
					fseek(fp, 0, SEEK_SET);
					buf2 = (GLchar*)malloc(incSize);
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
					GLchar linesDirective[256];
					long sizeLinesDirective;
					GLchar linesAfterDirective[256];
					long sizeLinesAfterDirective;

					// Append the file name to the source file name table
					*srcFileNames = (char**)realloc(*srcFileNames, ++*numSrcFileNames * sizeof *srcFileNames);
					(*srcFileNames)[*numSrcFileNames - 1] = (char *)malloc((strlen(fname) + 1) * sizeof(char));
					strcpy((*srcFileNames)[*numSrcFileNames - 1], fname);

					// Construct #line directives to make line numbers in log messages readable.
					// Unlike C or C++, the second argument of #line directive must be an integer (called source string number).
					// For details, see https://www.opengl.org/wiki/Core_Language_%28GLSL%29#.23line_directive
					sprintf(linesDirective, "#line %d %d\n", 0, *numSrcFileNames - 1);
					newSize += sizeLinesDirective = strlen(linesDirective);
					sprintf(linesAfterDirective, "#line %d %d\n", linenum, srcFileIdx);
					newSize += sizeLinesAfterDirective = strlen(linesAfterDirective);

					// Reallocate and possibly move the buffer.
					*buf = (GLchar*)realloc(*buf, newSize);
					// Move the later section of the original file to end of the included file.
					memmove(&(*buf)[direcOffset + sizeLinesDirective + incSize + sizeLinesAfterDirective], &(*buf)[scanOffset], *size - scanOffset);
					// Insert a #line directive before included file.
					memcpy(&(*buf)[direcOffset], linesDirective, sizeLinesDirective);
					// Embed the included file in the place of the directive.
					memcpy(&(*buf)[direcOffset + sizeLinesDirective], buf2, incSize);
					// Insert a #line directive after included file to restore original file name and line number.
					memcpy(&(*buf)[direcOffset + sizeLinesDirective + incSize], linesAfterDirective, sizeLinesAfterDirective);
					// Update the size.
					*size = newSize;
					// Free the appropriate buffer.
					if(fp)
						free(buf2);
					else
						ZipFree(buf2);

					// Recursively preprocess the included file. Since we need to restore line numbers and source string numbers of original file, we must have a stack
					// somewhere to remember line numbers in nested includes. The call stack is intuitive way to do this (although
					// number of parameters increases).
					preprocess_file(buf, size, direcOffset, direcOffset + sizeLinesDirective + incSize + sizeLinesAfterDirective, srcFileNames, numSrcFileNames);

					// Scan pointer should point to the reallocated buffer.
					scan = &(*buf)[direcOffset + sizeLinesDirective + incSize + sizeLinesAfterDirective];

					scanEnd += incSize - (scanOffset - direcOffset);

				}
				// Increment line number because the next iteration will consume the newline (\n) without counting.
				++linenum;
				// Reset the state.
				direcStart = NULL;
			}
		}
		else // Non-matched directive name can happen and should be ignored.
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
		char **srcFileNames;
		int numSrcFileNames = 1, i;
		int prepResult;
		long osize = size;

		srcFileNames = (char**)malloc(sizeof(*srcFileNames));
		srcFileNames[0] = (char*)fname;

		prepResult = preprocess_file(&buf, &size, 0, size, &srcFileNames, &numSrcFileNames);
		if(prepResult < 0){
			ret = 0;
			fprintf(stderr, "GLSL preprocessing error in \"%s\".\n", fname);
		}
		else{
			ret = glsl_register_shader(shader, buf, srcFileNames, numSrcFileNames);
			if(g_shader_preprocess_out && !ret && osize != size){
				FILE *pfp;
				pfp = fopen("logs/preprocessed.txt", "wb");
				if(pfp){
					fprintf(pfp, "%.*s\n", size, buf);
					fclose(pfp);
				}
			}
		}

		// Note that i starts with 1; the first element in the array is a const pointer to given
		// string which cannot be freed by this function.
		for(i = 1; i < numSrcFileNames; i++)
			free(srcFileNames[i]);
		free(srcFileNames);

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
