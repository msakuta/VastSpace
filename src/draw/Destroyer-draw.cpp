#include "../Destroyer.h"
#include "draw/material.h"
#include "effects.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/ShaderBind.h"
#include "glw/PopupMenu.h"
#include "draw/mqoadapt.h"
extern "C"{
#include <clib/gl/gldraw.h>
}


void Destroyer::draw(wardraw_t *wd){

	draw_healthbar(this, wd, getHealth() / getMaxHealth(), 300., -1, capacitor / maxenergy());

	if(wd->vw->gc->cullFrustum(pos, getHitRadius()))
		return;

	struct TextureParams{
		Destroyer *p;
		WarDraw *wd;
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
		static void onInitedTextureGradient(void *pv){
			TextureParams *p = (TextureParams*)pv;
			if(p->p->buildPhase != 1.){
				glAlphaFunc(GL_GEQUAL, (GLfloat)(1. - p->p->buildPhase));
				glEnable(GL_ALPHA_TEST);
				glDisable(GL_CULL_FACE);
			}
			else{
				glDisable(GL_ALPHA_TEST);
				glEnable(GL_CULL_FACE);
			}
			glDisable(GL_BLEND);
		}
	} tp = {this, wd};

	Model *model = getModel();

	if(model){
		// Assumption is that all bone sufs in a Model share the same texture set,
		// which is true if the Model is loaded from a MQO.
		for(int i = 0; i < model->n; i++) if(model->tex[i]){
			if(0 <= engineAtrIndex){
				model->tex[i]->a[engineAtrIndex].onBeginTexture = TextureParams::onBeginTextureEngine;
				model->tex[i]->a[engineAtrIndex].onBeginTextureData = &tp;
				model->tex[i]->a[engineAtrIndex].onEndTexture = TextureParams::onEndTextureEngine;
				model->tex[i]->a[engineAtrIndex].onEndTextureData = &tp;
			}
			if(0 <= gradientsAtrIndex){
				model->tex[i]->a[gradientsAtrIndex].onInitedTexture = TextureParams::onInitedTextureGradient;
				model->tex[i]->a[gradientsAtrIndex].onInitedTextureData = &tp;
			}
		}

		Mat4d mat;

		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT);
		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);

		glPushMatrix();
		gldScaled(modelScale);
		glScalef(-1,1,-1);
		DrawMQOPose(model, NULL);
		glPopMatrix();

		glPopMatrix();
		glPopAttrib();
	}
}

void Destroyer::drawtra(wardraw_t *wd){
	st::drawtra(wd);
	if(wd->vw->gc->cullFrustum(pos, getHitRadius()))
		return;
	drawNavlights(wd, navlights);
	static const Vec3d engines[] = {
		Vec3d(0, 0, 180) * modelScale,
		Vec3d(28, 22, 180) * modelScale,
		Vec3d(-28, 22, 180) * modelScale,
		Vec3d(28, -22, 180) * modelScale,
		Vec3d(-28, -22, 180) * modelScale,
	};
	for(int i = 0; i < numof(engines); i++)
		drawCapitalBlast(wd, engines[i], 20.);
//	drawShield(wd);
}

void Destroyer::drawOverlay(wardraw_t *){
	glCallList(disp);
}

int Destroyer::popupMenu(PopupMenu &list){
	int ret = st::popupMenu(list);
	list.append("Equipments...", 0, "armswindow");
	return ret;
}




void WireDestroyer::draw(wardraw_t *wd){
	static suf_t *sufbase;
	static suf_t *sufwheel;
	static suf_t *sufbit;
	static suftex_t *pst;
	static bool init = false;

	draw_healthbar(this, wd, health / getMaxHealth(), 300., -1, -1);

	if(!init) do{
		sufbase = CallLoadSUF("models/wiredestroyer0.bin");
		if(!sufbase) break;
		sufwheel = CallLoadSUF("models/wiredestroyer_wheel0.bin");
		if(!sufwheel) break;
		sufbit = CallLoadSUF("models/wirebit0.bin");
		suftexparam_t stp;
		stp.flags = STP_MAGFIL | STP_MINFIL | STP_ENV;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		stp.env = GL_ADD;
		stp.mipmap = 0;
		CallCacheBitmap5("engine2.bmp", "engine2br.bmp", &stp, "engine2.bmp", NULL);
		CacheSUFMaterials(sufbase);
		pst = AllocSUFTex(sufbase);
		init = true;
	} while(0);

	if(sufbase){
		static const double normal[3] = {0., 1., 0.};
		double scale = 1.;
		static const GLdouble rotaxis[16] = {
			-1,0,0,0,
			0,1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};
		Mat4d mat;

		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);

		if(!wd->vw->gc->cullFrustum(pos, getHitRadius())){

	#if 0
			for(int i = 0; i < nhitboxes; i++){
				Mat4d rot;
				glPushMatrix();
				gldTranslate3dv(hitboxes[i].org);
				rot = hitboxes[i].rot.tomat4();
				glMultMatrixd(rot);
				hitbox_draw(this, hitboxes[i].sc);
				glPopMatrix();
			}
	#endif

			glPushMatrix();
			gldScaled(scale);
			glMultMatrixd(rotaxis);
			DecalDrawSUF(sufbase, SUF_ATR, NULL, pst, NULL, NULL);
			glRotated(wirephase * deg_per_rad, 0, 0, -1);
			DecalDrawSUF(sufwheel, SUF_ATR, NULL, pst, NULL, NULL);
			glPopMatrix();
		}

		for(int i = 0; i < 2; i++){
			glPushMatrix();
			glRotated(wirephase * deg_per_rad, 0, 0, 1);
			glTranslated(wirelength * (i * 2 - 1), 0, 0);
			gldScaled(scale);
			glMultMatrixd(rotaxis);
			DrawSUF(sufbit, SUF_ATR, NULL);
			glPopMatrix();
		}

		glPopMatrix();
		glPopAttrib();
	}
}

void WireDestroyer::drawtra(wardraw_t *wd){
	Mat4d mat;
	transform(mat);
	for(int i = 0; i < 2; i++){
		Mat4d rot = mat.rotz(wirephase);
		glColor4f(1,.5,.5,1);
		gldBeam(wd->vw->pos, rot.vp3(Vec3d(70. * (i * 2 - 1), 0, 0)), rot.vp3(Vec3d(wirelength * (i * 2 - 1), 0, 0)), .01);
	}
}

