#ifndef SHADOWMAP_H
#define SHADOWMAP_H
/** \file
 * \brief Definition of ShadowMap class handling shadow map textures.
 */

#include "export.h"
#include "Viewer.h"

#include "antiglut.h"
#ifdef _WIN32
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif
#include <gl/GL.h>

/// \brief A class to support shadow drawing with shadow mapping technique.
///
/// It depends on several OpenGL extensions.
class EXPORT ShadowMap{
	static GLuint fbo;
	static GLuint to;
	static GLuint depthTextures[3];
	bool shadowing;
	bool additive;
public:
	class DrawCallback{
	public:
		virtual void drawShadowMaps(Viewer &vw2) = 0;
		virtual void draw(Viewer &vw2, GLuint shader, GLint textureLoc, GLint shadowmapLoc) = 0;
	};
	ShadowMap();
	void drawShadowMaps(Viewer &vw, const Vec3d &light, DrawCallback &drawcallback);
	GLuint getShader()const;
	bool isDrawingShadow()const{return shadowing;}
	void setAdditive(bool b);
	bool getAdditive()const;
};


#endif
