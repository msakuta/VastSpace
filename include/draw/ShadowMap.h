#ifndef DRAW_SHADOWMAP_H
#define DRAW_SHADOWMAP_H
/** \file
 * \brief Definition of ShadowMap class handling shadow map textures.
 */

#include "export.h"
#include "Viewer.h"
#include "OpenGLState.h"

#include "antiglut.h"
#ifdef _WIN32
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif
#include <GL/gl.h>

struct ShaderBind;
struct AdditiveShaderBind;

/// \brief A class to support shadow drawing with shadow mapping technique.
///
/// It depends on several OpenGL extensions.
class EXPORT ShadowMap{
	static GLint shadowMapSize;
	static GLdouble shadowMapScales[3];
	static GLuint fbo;
	static GLuint to;
	static GLuint depthTextures[3];
	double shadowOffset;
	double shadowSlopeScaledBias;

	/// Matrices to pass to shaders for texture transformation.
	GLfloat shadowMatrices[3][16];

	/// A variable indicating whether current rendering is a shadow map drawing pass.
	/// 0 : It's not shadow map pass; real screen rendering.
	/// >1 : A shadow map pass.  The number indicates which level of detail currently drawing.
	int shadowing;

	bool additive;
	bool cullFront;
public:
	class DrawCallback{
	public:
		virtual void drawShadowMaps(Viewer &vw2) = 0;
		virtual void draw(Viewer &vw2, GLuint shader, GLint textureLoc, GLint shadowmapLoc) = 0;
	};
	ShadowMap(int shadowMapSize, GLdouble (&shadowMapScales)[3], double shadowOffset, bool cullFront, double shadowSlopeScaleBias);
	void drawShadowMaps(Viewer &vw, const Vec3d &light, DrawCallback &drawcallback);
	const ShaderBind *getShader()const;
	bool isDrawingShadow()const{return !!shadowing;} ///< Returns whether the current drawing path is for a shadow map.
	int shadowLevel()const{return shadowing;} ///< Returns level of shadow map, 0 if not for shadow map path.
	void setAdditive(bool b);
	const AdditiveShaderBind *getAdditive()const;
	void enableShadows();
	void disableShadows();
	GLfloat getSlopeScaledBias()const;
	const GLfloat *getShadowMatrices()const{return *shadowMatrices;}
};

EXPORT extern OpenGLState::weak_ptr<ShadowMap*> g_currentShadowMap;

#endif
