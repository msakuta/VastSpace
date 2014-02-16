/** \file
 * \brief Implementation of SpacePlane's graphics.
 */
#include "../SpacePlane.h"
#include "../vastspace.h"
#include "draw/material.h"
#include "glstack.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/ShaderBind.h"
#include "draw/mqoadapt.h"



const double SpacePlane::sufscale = .0004;
static Model *model = NULL;

void SpacePlane::draw(WarDraw *wd){
	if(!w)
		return;

	/* cull object */
	if(cull(wd))
		return;

	draw_healthbar(this, wd, health / getMaxHealth(), .1, 0, capacitor / frigate_mn.capacity);

	struct TextureParams{
		SpacePlane *p;
		WarDraw *wd;
		static void onBeginTextureWindows(void *pv){
			TextureParams *p = (TextureParams*)pv;
			if(p && p->wd){
				p->wd->setAdditive(true);
				const AdditiveShaderBind *asb = p->wd->getAdditiveShaderBind();
				if(asb)
					asb->setIntensity(Vec3f(1,1,1));
			}
		}
		static void onEndTextureWindows(void *pv){
			TextureParams *p = (TextureParams*)pv;
			if(p && p->wd)
				p->wd->setAdditive(false);
		}
		static void onBeginTextureEngine(void *pv){
			TextureParams *p = (TextureParams*)pv;
			if(p && p->wd){
				p->wd->setAdditive(true);
				const AdditiveShaderBind *asb = p->wd->getAdditiveShaderBind();
				if(asb)
					asb->setIntensity(Vec3f(1. - (p->p->engineHeat - .6) * (p->p->engineHeat - .6) / (.6 * .6), p->p->engineHeat * 2, p->p->engineHeat * 3));
			}
		}
		static void onEndTextureEngine(void *pv){
			TextureParams *p = (TextureParams*)pv;
			if(p && p->wd)
				p->wd->setAdditive(false);
		}
	} tp = {this, wd};

	// The pointed type is not meaningful; it just indicates status of initialization by its presense.
	static OpenGLState::weak_ptr<bool> initialized = false;
	static int engineAtrIndex = -1, windowsAtrIndex = -1;

	if(!initialized){

		// Register brightness mapping texture
		TexParam stp;
		stp.flags = STP_MAGFIL | STP_MINFIL | STP_ENV;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		stp.env = GL_ADD;
		stp.mipmap = 0;
		stp.transparentColor = 0;
		AddMaterial(modPath() + "models/spaceplane.bmp", modPath() + "models/spaceplane_br.bmp", &stp, modPath() + "models/spaceplane.bmp", NULL);
		AddMaterial(modPath() + "models/engine2.bmp", modPath() + "models/engine2br.bmp", &stp, modPath() + "models/engine2.bmp", NULL);

		model = LoadMQOModel(modPath() + "models/spaceplane.mqo");

		if(model && model->sufs[0]){
			for(int i = 0; i < model->tex[0]->n; i++){
				if(!strcmp(model->sufs[0]->a[i].colormap, "engine2.bmp")){
					engineAtrIndex = i;
					model->tex[0]->a[i].onBeginTexture = TextureParams::onBeginTextureEngine;
					model->tex[0]->a[i].onEndTexture = TextureParams::onEndTextureEngine;
				}
				else if(!strcmp(model->sufs[0]->a[i].colormap, "spaceplane.bmp")){
					windowsAtrIndex = i;
					model->tex[0]->a[i].onBeginTexture = TextureParams::onBeginTextureWindows;
					model->tex[0]->a[i].onEndTexture = TextureParams::onEndTextureWindows;
				}
			}
		}

		initialized.create(*openGLState);
	}

	static int drawcount = 0;
	drawcount++;
	if(model){
		static const double normal[3] = {0., 1., 0.};
		double scale = sufscale;
		static const GLdouble rotaxis[16] = {
			-1,0,0,0,
			0,1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};
		Mat4d mat;

		if(model->tex[0]){
			if(0 <= engineAtrIndex){
				model->tex[0]->a[engineAtrIndex].onBeginTextureData = &tp;
				model->tex[0]->a[engineAtrIndex].onEndTextureData = &tp;
			}
			if(0 <= windowsAtrIndex){
				model->tex[0]->a[windowsAtrIndex].onBeginTextureData = &tp;
				model->tex[0]->a[windowsAtrIndex].onEndTextureData = &tp;
			}
		}

		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);

		GLattrib gla(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);

		glPushMatrix();
		glScaled(scale, scale, scale);
		glMultMatrixd(rotaxis);
		DrawMQOPose(model, NULL);
		glPopMatrix();

		glPopMatrix();

		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void SpacePlane::drawtra(wardraw_t *wd){
	st::drawtra(wd);
	drawCapitalBlast(wd, engines[0] + Vec3d(0,0,.01), .01);
	drawCapitalBlast(wd, engines[1] + Vec3d(0,0,.0075), .0075);
	drawCapitalBlast(wd, engines[2] + Vec3d(0,0,.0075), .0075);
}

void SpacePlane::drawOverlay(wardraw_t *){
	glScaled(10, 10, 1);
	glBegin(GL_LINE_LOOP);
	glVertex2d(-.10,  .00);
	glVertex2d(-.05, -.03);
	glVertex2d( .00, -.03);
	glVertex2d( .08, -.07);
	glVertex2d( .10, -.03);
	glVertex2d( .10,  .03);
	glVertex2d( .08,  .07);
	glVertex2d( .00,  .03);
	glVertex2d(-.05,  .03);
	glEnd();
}


Entity::Props SpacePlane::props()const{
	Props ret = st::props();
	ret.push_back(gltestp::dstring("People: ") << people);
	ret.push_back(gltestp::dstring("I am SpacePlane!"));
	return ret;
}
