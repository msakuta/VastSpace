#ifndef DRAW_SHADERBIND_H
#define DRAW_SHADERBIND_H
/** \file
 * \brief Definition of ShaderBind class tree.
 */

#include "export.h"
#include "OpenGLState.h"

#include "antiglut.h"
#ifdef _WIN32
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif
#include <cpplib/vec3.h>
#include <gl/GL.h>


/// \brief A class that binds shader object name with location indices.
///
/// The specification of location names are specific to this ShadowMap class's use.
struct EXPORT ShaderBind{
	GLuint shader;
	GLint textureEnableLoc;
	GLint texture2EnableLoc;
	GLint textureLoc;
	GLint texture2Loc;
	ShaderBind(GLuint shader = 0) : shader(shader),
		textureEnableLoc(-1),
		texture2EnableLoc(-1),
		textureLoc(-1),
		texture2Loc(-1){}
	~ShaderBind();

	virtual void getUniformLocations();
	void use()const;
	void enableTextures(bool texture1, bool texture2 = false)const;
protected:
	virtual void useInt()const;
};

EXPORT extern OpenGLState::weak_ptr<const ShaderBind*> g_currentShaderBind;

/// Binding of shadow mapping shader program.
struct EXPORT ShadowMapShaderBind : virtual ShaderBind{
	GLint shadowmapLoc;
	GLint shadowmap2Loc;
	GLint shadowmap3Loc;
	ShadowMapShaderBind(GLuint shader = 0) :
		ShaderBind(shader),
		shadowmapLoc(-1),
		shadowmap2Loc(-1),
		shadowmap3Loc(-1){}

	void build();
	void getUniformLocations();
protected:
	void useInt()const;
};

/// Binding of additive (brightness mapped) texturing shader program.
struct EXPORT AdditiveShaderBind : virtual ShaderBind{
	GLint intensityLoc;
	AdditiveShaderBind(GLuint shader = 0) : ShaderBind(0),
		intensityLoc(-1){}

	void build();
	void getUniformLocations();
	void setIntensity(const Vec3f&)const;
protected:
	void useInt()const;
};

/// Binding of additive and shadow mapped shader program.
struct AdditiveShadowMapShaderBind : ShadowMapShaderBind, AdditiveShaderBind{
	AdditiveShadowMapShaderBind(GLuint shader = 0) : ShadowMapShaderBind(shader), AdditiveShaderBind(shader){
	}

	void build();
	void getUniformLocations();
protected:
	void useInt()const;
};


#endif
