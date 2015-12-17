/** \file 
 * \brief Implementation of ShadowMap class.
 */

#define NOMINMAX

#include "draw/OpenGLState.h"
#include "draw/ShadowMap.h"
#include "draw/ShaderBind.h"
#include "draw/HDR.h"

#include "glstack.h"
#include "glsl.h"

extern "C"{
#include <clib/timemeas.h>
#include <clib/c.h>
#include <clib/gl/gldraw.h>
#include <clib/gl/multitex.h>
#include <clib/gl/fbo.h>
}
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include <vector>
#include <iostream>
#include <algorithm>


void *operator new(size_t size, OpenGLState &o){
	void *ret = ::operator new(size);
	return ret;
}

void *OpenGLState::add(weak_ptr_base *wp){
	objs.insert(wp);
	return wp;
}

OpenGLState::~OpenGLState(){
	std::set<weak_ptr_base*>::iterator it = objs.begin();
	for(it; it != objs.end(); it++)
		(*it)->destroy();
}

OpenGLState *openGLState = new OpenGLState;



#define projection(e) glMatrixMode(GL_PROJECTION); e; glMatrixMode(GL_MODELVIEW);
#define texturemat(e) glMatrixMode(GL_TEXTURE); e; glMatrixMode(GL_MODELVIEW);


/// check FBO completeness
static bool checkFramebufferStatus()
{
    // check FBO status
    GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    switch(status)
    {
    case GL_FRAMEBUFFER_COMPLETE_EXT:
//        std::cout << "Framebuffer complete." << std::endl;
        return true;

    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
        std::cout << "[ERROR] Framebuffer incomplete: Attachment is NOT complete." << std::endl;
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
        std::cout << "[ERROR] Framebuffer incomplete: No image is attached to FBO." << std::endl;
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
        std::cout << "[ERROR] Framebuffer incomplete: Attached images have different dimensions." << std::endl;
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
        std::cout << "[ERROR] Framebuffer incomplete: Color attached images have different internal formats." << std::endl;
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
        std::cout << "[ERROR] Framebuffer incomplete: Draw buffer." << std::endl;
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
        std::cout << "[ERROR] Framebuffer incomplete: Read buffer." << std::endl;
        return false;

    case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
        std::cout << "[ERROR] Unsupported by FBO implementation." << std::endl;
        return false;

    default:
        std::cout << "[ERROR] Unknow error." << std::endl;
        return false;
    }
}


OpenGLState::weak_ptr<const ShaderBind*> g_currentShaderBind;
OpenGLState::weak_ptr<ShadowMap*> g_currentShadowMap;

ShaderBind::~ShaderBind(){
	if(shader)
		glDeleteProgram(shader);
}

void ShaderBind::getUniformLocations(){
	textureEnableLoc = glGetUniformLocation(shader, "textureEnable");
	texture2EnableLoc = glGetUniformLocation(shader, "texture2Enable");
	textureLoc = glGetUniformLocation(shader, "texture");
	texture2Loc = glGetUniformLocation(shader, "texture2");
	exposureLoc = glGetUniformLocation(shader, "exposure");
	tonemapLoc = glGetUniformLocation(shader, "tonemap");
}

void ShaderBind::use()const{
	glUseProgram(shader);
	useInt();
	g_currentShaderBind.create(*openGLState);
	*g_currentShaderBind = this;
}

void ShaderBind::enableTextures(bool texture1, bool texture2)const{
	// Reuse if deactivated
	if(g_currentShaderBind && *g_currentShaderBind != this)
		use();
	glUniform1i(textureEnableLoc, texture1);
	glUniform1i(texture2EnableLoc, texture2);
}

void ShaderBind::useInt()const{
	glUniform1i(textureEnableLoc, 1);
	glUniform1i(texture2EnableLoc, 1);
	glUniform1i(textureLoc, 0);
	glUniform1i(texture2Loc, 1);
	glUniform1f(exposureLoc, r_exposure);
	glUniform1i(tonemapLoc, r_tonemap);
}

struct ShaderList : public std::vector<GLuint>{
	~ShaderList(){
		for(iterator it = begin(); it != end(); ++it)
			glDeleteShader(*it);
	}
};

void ShadowMapShaderBind::build(){
	ShaderList shaders;
	GLuint s;
	shaders.push_back(s = glCreateShader(GL_VERTEX_SHADER));
	if(!glsl_load_shader(s, "shaders/normalShadowmap.vs"))
		return;
	shaders.push_back(s = glCreateShader(GL_FRAGMENT_SHADER));
	if(!glsl_load_shader(s, "shaders/normalShadowmap.fs"))
		return;
	shader = glsl_register_program(&shaders.front(), shaders.size());
	if(!shader)
		return;
}

void ShadowMapShaderBind::getUniformLocations(){
	ShaderBind::getUniformLocations();
	shadowmapLoc = glGetUniformLocation(shader, "shadowmap");
	shadowmap2Loc = glGetUniformLocation(shader, "shadowmap2");
	shadowmap3Loc = glGetUniformLocation(shader, "shadowmap3");
	shadowSlopeScaledBiasLoc = glGetUniformLocation(shader, "shadowSlopeScaledBias");
}

void ShadowMapShaderBind::useInt()const{
	ShaderBind::useInt();
	glUniform1i(shadowmapLoc, 2);
	glUniform1i(shadowmap2Loc, 3);
	glUniform1i(shadowmap3Loc, 4);
	glUniform1f(shadowSlopeScaledBiasLoc, shadowSlopeScaledBias);
}

void AdditiveShaderBind::build(){
	ShaderList shaders;
	GLuint s;
	shaders.push_back(s = glCreateShader(GL_VERTEX_SHADER));
	if(!glsl_load_shader(s, "shaders/additive.vs"))
		return;
	shaders.push_back(s = glCreateShader(GL_FRAGMENT_SHADER));
	if(!glsl_load_shader(s, "shaders/additive.fs"))
		return;
	shader = glsl_register_program(&shaders.front(), shaders.size());
	if(!shader)
		return;
}

void AdditiveShaderBind::getUniformLocations(){
	ShaderBind::getUniformLocations();
	intensityLoc = glGetUniformLocation(shader, "intensity");
}

void AdditiveShaderBind::useInt()const{
	ShaderBind::useInt();
	glUniform1f(glGetUniformLocation(shader, "intensity[0]"), 1.f);
	glUniform1f(glGetUniformLocation(shader, "intensity[1]"), 1.f);
	glUniform1f(glGetUniformLocation(shader, "intensity[2]"), 1.f);
}

void AdditiveShaderBind::setIntensity(const Vec3f &inten)const{
//	glUniform3fv(intensityLoc, 1, inten);
	static const char *strinten[3] = {"intensity[0]", "intensity[1]", "intensity[2]"};
	GLint intenloc[3];
	for(int i = 0; i < 3; i++){
		intenloc[i] = glGetUniformLocation(shader, strinten[i]);
		glUniform1f(intenloc[i], inten[i]);
	}
}

void AdditiveShadowMapShaderBind::build(){
	ShaderList shaders;
	GLuint s;
	shaders.push_back(s = glCreateShader(GL_VERTEX_SHADER));
	if(!glsl_load_shader(s, "shaders/additiveShadowmap.vs"))
		return;
	shaders.push_back(s = glCreateShader(GL_FRAGMENT_SHADER));
	if(!glsl_load_shader(s, "shaders/additiveShadowmap.fs"))
		return;
	shader = glsl_register_program(&shaders.front(), shaders.size());
	if(!shader)
		return;
}

void AdditiveShadowMapShaderBind::getUniformLocations(){
	ShadowMapShaderBind::getUniformLocations();
	AdditiveShaderBind::getUniformLocations();
}

void AdditiveShadowMapShaderBind::useInt()const{
	ShadowMapShaderBind::useInt();
	AdditiveShaderBind::useInt();
}


static OpenGLState::weak_ptr<ShadowMapShaderBind> shaderBind;
static OpenGLState::weak_ptr<AdditiveShaderBind> additiveShaderBind;
static OpenGLState::weak_ptr<AdditiveShadowMapShaderBind> additiveShadowMapShaderBind;



#if 0
#include "ShadowHelper.h"
#else
void printFramebufferInfo(){}
#endif

GLint ShadowMap::shadowMapSize = 2048;
GLdouble ShadowMap::shadowMapScales[3] = {
	1. / 5., 1. / .75, 1. / .1
};

GLuint ShadowMap::fbo = 0; ///< Framebuffer object
//GLuint ShadowMap::rboId = 0; ///< Renderbuffer object
GLuint ShadowMap::to = 0; ///< Texture object
GLuint ShadowMap::depthTextures[3] = {0}; ///< Texture names for depth textures
static GLint additiveLoc = -1;

/// \brief Initializes shadow map textures
///
/// The parameters are interpreted only in the first invocation of this constructor and stored into static storage.
///
/// \param ashadowMapSize The size of shadow map texture in pixels.
/// \param ashadowMapCells The cell widths of shadow map regions. It have 3 LODs of textures, so you must specify the width for each LOD.
/// \param shadowOffset Factor of shadow offset correction to prevent shadow acne artifact. Unlike other parameters, it takes effect for each construction of the object.
/// \param cullFront Whether cull the front face instead of back face. It will reduce shadow acne but causes artifacts in concave junction.
/// \param shadowSlopeScaledBias Enable slope scaled bias technique to reduce shadow acne.
ShadowMap::ShadowMap(int ashadowMapSize, GLdouble (&ashadowMapCells)[3],
	double shadowOffset, bool cullFront, double shadowSlopeScaledBias)
	: shadowing(0), additive(false), shadowOffset(shadowOffset), cullFront(cullFront), shadowSlopeScaledBias(shadowSlopeScaledBias)
{
	if(FBOInit() && !fbo){
		int	gerr = glGetError();
		glGenFramebuffersEXT(1, &fbo);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

		// Make the texture size do not exceed the hardware's maximum.
		GLint maxsize;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxsize);
		shadowMapSize = std::min(ashadowMapSize, maxsize);
		for(int i = 0; i < numof(shadowMapScales); i++)
			shadowMapScales[i] = 1. / ashadowMapCells[i];

		// texture for depth
		glGenTextures(3, depthTextures);
		for(int i = 0; i < 3; i++){
			GLuint tod = depthTextures[i];
			glBindTexture(GL_TEXTURE_2D, tod);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, Vec4f(1., 1., 1., 1.));
	//		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // automatic mipmap generation included in OpenGL v1.4
			gerr = glGetError();
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// attach a renderbuffer to depth attachment point
//		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, rboId);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, depthTextures[0], 0);

		// These are necessary to complete framebuffer object.
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

        printFramebufferInfo();
		if(!checkFramebufferStatus()){
			glDeleteFramebuffersEXT(1, &fbo);
		}
		gerr = glGetError();

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}

	// Reset the shader bind object on the beginning of a frame, because some other shaders could be used
	// in background drawing which is outside of control of ShaderBind or ShadowMap classes.
	// It may cost a bit to free and allocate the memory for this variable, but it would cost more time
	// to generate a OpenGL error.
	g_currentShaderBind = NULL;
}

const ShaderBind *ShadowMap::getShader()const{
	return shaderBind;
}


/// Actually draws shadow maps and the scene.
void ShadowMap::drawShadowMaps(Viewer &vw, const Vec3d &g_light, DrawCallback &drawcallback){
	vw.shadowmap = this;
	g_currentShadowMap.create(*openGLState);
	*g_currentShadowMap = this;
	if(fbo && glIsFrameBufferEXT(fbo)){
		Mat4d lightProjection[3];
		Mat4d lightModelView;
		GLdouble (&shadowCell)[3] = shadowMapScales/*{1. / 5., 1. / .75, 1. / .02}*/;

		bool shadowok = true;
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
		for(int i = 0; i < 1 + 2 * !!g_shader_enable; i++) if(checkFramebufferStatus()){
			GLpmmatrix pmm;
			projection((
				glLoadIdentity(),
				glOrtho(-1, 1, -1, 1, -100, 100),
				gldScaled(shadowCell[i])
	//				vw.frustum(g_warspace_near_clip, g_warspace_far_clip)
			));

			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, depthTextures[i], 0);

			glClearDepth(1.);
			glClearColor(0,0,0,1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			lightModelView = (g_light ? Quatd::direction(g_light).cnj() : quat_u).tomat4().translatein(-vw.pos);
			glLoadMatrixd(lightModelView);

			GLattrib gla(GL_POLYGON_BIT);
			glCullFace(cullFront ? GL_FRONT : GL_BACK);
			glEnable(GL_POLYGON_OFFSET_FILL);

			// This polygon offset prevents aliasing of two-sided polys.
//			glPolygonOffset(1., 1.);

			glViewport(0, 0, shadowMapSize, shadowMapSize);
			Viewer vw2 = vw;
			GLcull gc(vw.pos, vw.gc->getInvrot(), 1., -1, 1);
			vw2.gc = &gc;
	//			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, to, 0);
	//				cswardraw(&vw, const_cast<CoordSys*>(pl.cs), &CoordSys::draw);
			shadowing = i + 1; // Notify the callback implicitly that it's the shadow map texture pass.
			drawcallback.drawShadowMaps(vw2);
	//				war_draw(vw2, pl.cs, &WarField::draw).shadowmap();
	//			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, 0, 0);
			glGetDoublev(GL_PROJECTION_MATRIX, lightProjection[i]);
		}
		else{
			shadowok = false;
			break;
		}
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		glViewport(0, 0, vw.vp.w, vw.vp.h);
				
		if(shadowok){

			if(!g_shader_enable){
				glPushAttrib(GL_LIGHTING_BIT);
	//			glDisable(GL_LIGHT0);
				glLightfv(GL_LIGHT0, GL_DIFFUSE, Vec4f(.2,.2,.2,1.));
				shadowing = 0; // Notify the callback implicitly that it's the real scene pass.
				drawcallback.draw(vw, 0, 0, 0); // war_draw(vw, pl.cs, &WarField::draw);
				glPopAttrib();
			}
			else do{
				if(!shaderBind){
					shaderBind.create(*openGLState);
					shaderBind->build();
					shaderBind->getUniformLocations();
				}
				if(!additiveShadowMapShaderBind){
					additiveShadowMapShaderBind.create(*openGLState);
					additiveShadowMapShaderBind->build();
					additiveShadowMapShaderBind->getUniformLocations();
				}
			} while(0);

	//			glClear(GL_DEPTH_BUFFER_BIT);

			// Bias matrix from [-1, 1] to [0, 1]
			// Also adds offset z coord for reducing shadow acne.
			// Refer the URL about shadow acne.
			// http://msdn.microsoft.com/en-us/library/windows/desktop/ee416324(v=vs.85).aspx
			Mat4d biasMatrix(Vec4d(.5, .0, .0, .0),
							Vec4d(.0, .5, .0, .0),
							Vec4d(.0, .0, .5, .0),
							Vec4d(.5, .5, .5 - shadowOffset * 0.5 / shadowMapSize / 50., 1.));

			int numShadowTextures = 1;
			// If we can use GLSL, we use cascaded shadow maps, which uses multiple textures for shadow depth.
			if(g_shader_enable)
				numShadowTextures += 2;

			glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			for(int i = 0; i < numShadowTextures; i++){
				glActiveTextureARB(GL_TEXTURE2_ARB + i);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, depthTextures[i]);
				texturemat(glPushMatrix());

				//Enable shadow comparison
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);

				//Shadow comparison should be true (ie not in shadow) if r<=texture
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);

				//Shadow comparison should generate an INTENSITY result
				glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY);

				glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, Vec4f(1., 1., 1., 1.));

				Mat4d textureMatrix = (biasMatrix * lightProjection[i] * lightModelView);

				if(!g_shader_enable){
					textureMatrix = textureMatrix.transpose();
					glEnable(GL_TEXTURE_GEN_S);
					glEnable(GL_TEXTURE_GEN_T);
					glEnable(GL_TEXTURE_GEN_R);
					glEnable(GL_TEXTURE_GEN_Q);
					glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
					glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
					glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
					glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
					glTexGendv(GL_S, GL_EYE_PLANE, textureMatrix.vec4(0));
					glTexGendv(GL_T, GL_EYE_PLANE, textureMatrix.vec4(1));
					glTexGendv(GL_R, GL_EYE_PLANE, textureMatrix.vec4(2));
					glTexGendv(GL_Q, GL_EYE_PLANE, textureMatrix.vec4(3));

					//Set alpha test to discard false comparisons
					glAlphaFunc(GL_GEQUAL, 0.99f);
					glEnable(GL_ALPHA_TEST);
				}
				else{
					Mat4d itrans = vw.irot;
					itrans.vec3(3) = vw.pos;
					texturemat(glLoadMatrixd(textureMatrix * itrans));
					shaderBind->shadowSlopeScaledBias = shadowSlopeScaledBias * 0.5 / shadowMapSize / 50.;
					additiveShadowMapShaderBind->shadowSlopeScaledBias = shadowSlopeScaledBias * 0.5 / shadowMapSize / 50.;
					shaderBind->use();
					glDisable(GL_ALPHA_TEST);
				}
			}

			glActiveTextureARB(GL_TEXTURE0_ARB);

			glDepthFunc(GL_LEQUAL);

			shadowing = 0; // Notify the callback implicitly that it's the real scene pass.
			if(shaderBind)
				drawcallback.draw(vw, shaderBind->shader, shaderBind->textureLoc, shaderBind->shadowmapLoc);
			else
				drawcallback.draw(vw, 0, -1, -1);

			if(g_shader_enable)
				glUseProgram(0);

			// Disable texture units for shadow map to eliminate the inappropriate influence to the following drawings.
			for(int i = 0; i < numShadowTextures; i++){
				glActiveTextureARB(GL_TEXTURE2_ARB + i);
				texturemat(glPopMatrix());
				glBindTexture(GL_TEXTURE_2D, 0);
				glDisable(GL_TEXTURE_2D);
			}
			glActiveTextureARB(GL_TEXTURE0_ARB);

			glPopAttrib();
		}
	}
	vw.shadowmap = NULL;
	if(g_currentShadowMap)
		*g_currentShadowMap = NULL;
}


void ShadowMap::setAdditive(bool b){
	additive = b;
	if(additive){
		if(additiveShadowMapShaderBind)
			additiveShadowMapShaderBind->use();
	}
	else{
		if(shaderBind)
			shaderBind->use();
	}
}

const AdditiveShaderBind *ShadowMap::getAdditive()const{
	// The cast from OpenGLState::weak_ptr<AdditiveShadowMapShaderBind> to AdditiveShaderBind* induces
	// a cast to OpenGLState::weak_ptr<AdditiveShaderBind>, but we really want a
	// AdditiveShadowMapShaderBind*, so we make an explicit cast.
	return additive ? static_cast<AdditiveShadowMapShaderBind*>(additiveShadowMapShaderBind) : NULL;
}

/// Explicitly enable shadows when drawing real objects and shadowing is temporalily disabled.
void ShadowMap::enableShadows(){
	for(int i = 0; i < 1 + 2 * !!g_shader_enable; i++){
		glActiveTextureARB(GL_TEXTURE2_ARB + i);
//		glBindTexture(GL_TEXTURE_2D, 0);
		glEnable(GL_TEXTURE_2D);
	}
	glActiveTextureARB(GL_TEXTURE0_ARB);
}

/// Explicitly disable shadows when drawing real objects and shadowing is enabled.
void ShadowMap::disableShadows(){
	for(int i = 0; i < 1 + 2 * !!g_shader_enable; i++){
		glActiveTextureARB(GL_TEXTURE2_ARB + i);
//		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}
	glActiveTextureARB(GL_TEXTURE0_ARB);
}


