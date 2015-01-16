/** \file
 * \brief Defines a function to dynamically load OpenGL Vertex Buffer Object handling APIs
 */
#ifndef VBO_H
#define VBO_H

#include "export.h"

#ifdef _WIN32
#include <Windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#if defined(WIN32)
extern EXPORT PFNGLGENBUFFERSPROC glGenBuffers;
extern EXPORT PFNGLISBUFFERPROC glIsBuffer;
extern EXPORT PFNGLBINDBUFFERPROC glBindBuffer;
extern EXPORT PFNGLBUFFERDATAPROC glBufferData;
extern EXPORT PFNGLBUFFERSUBDATAPROC glBufferSubData;
extern EXPORT PFNGLMAPBUFFERPROC glMapBuffer;
extern EXPORT PFNGLUNMAPBUFFERPROC glUnmapBuffer;
extern EXPORT PFNGLDELETEBUFFERSPROC glDeleteBuffers;
#endif

bool vboInitBuffers();

#endif
