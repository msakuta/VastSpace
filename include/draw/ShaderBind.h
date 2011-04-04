#ifndef DRAW_SHADERBIND_H
#define DRAW_SHADERBIND_H
/** \file
 * \brief Definition of ShaderBind class tree.
 */

#include "export.h"

#include "antiglut.h"
#ifdef _WIN32
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif
#include <gl/GL.h>


/// \brief A class that binds shader object name with location indices.
///
/// The specification of location names are specific to this ShadowMap class's use.
struct EXPORT ShaderBind{
	GLuint shader;
	GLint textureLoc;
	GLint texture2Loc;
	ShaderBind(GLuint shader = 0) : shader(shader),
		textureLoc(-1),
		texture2Loc(-1){}

	void getUniformLocations();
protected:
	void useInt();
};

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
	void use();
protected:
	void useInt();
};

/// Binding of additive (brightness mapped) texturing shader program.
struct EXPORT AdditiveShaderBind : virtual ShaderBind{
	GLint intensityLoc;
	AdditiveShaderBind(GLuint shader = 0) : ShaderBind(0),
		intensityLoc(-1){}

	void build();
	void getUniformLocations();
	void use();
	void setIntensity(GLfloat)const;
protected:
	void useInt();
};

/// Binding of additive and shadow mapped shader program.
struct AdditiveShadowMapShaderBind : ShadowMapShaderBind, AdditiveShaderBind{
	AdditiveShadowMapShaderBind(GLuint shader = 0) : ShadowMapShaderBind(shader), AdditiveShaderBind(shader){
	}

	void build();
	void getUniformLocations();
	void use();
protected:
	void useInt();
};


#endif
