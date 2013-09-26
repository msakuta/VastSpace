/** \file
 * \brief Implements Attacker's drawing methods.
 */
#include "../Attacker.h"
#include "draw/material.h"
#include "effects.h"
#include "draw/WarDraw.h"
#include "draw/ShaderBind.h"
#include "draw/OpenGLState.h"
#include "draw/blackbody.h"
#include "draw/mqoadapt.h"
#include "motion.h"
extern "C"{
#include <clib/gl/gldraw.h>
#include <clib/suf/sufvbo.h>
}

struct AttackerTextureParams{
	Attacker *p;
	WarDraw *wd;
};

void Attacker::draw(wardraw_t *wd){
	static OpenGLState::weak_ptr<bool> init;
	static Model *model = NULL;

	draw_healthbar(this, wd, health / getMaxHealth(), .3, -1, capacitor / maxenergy());

	if(wd->vw->gc->cullFrustum(pos, getHitRadius()))
		return;

	struct TextureParams{
		Attacker *p;
		WarDraw *wd;
		static void onBeginTextureEngine(void *pv){
			TextureParams *p = (TextureParams*)pv;
			if(p && p->wd){
				p->wd->setAdditive(true);
				const AdditiveShaderBind *asb = p->wd->getAdditiveShaderBind();
				if(asb)
					asb->setIntensity(Vec4f(blackbodyEmit(p->p->engineHeat * 13000.), 1.));
			}
		}
		static void onEndTextureEngine(void *pv){
			TextureParams *p = (TextureParams*)pv;
			if(p && p->wd)
				p->wd->setAdditive(false);
		}
	} tp = {this, wd};

	if(!init) do{
		delete model;
		model = LoadMQOModel("models/attacker.mqo");

		if(model) for(int n = 0; n < model->n; n++) if(model->sufs[n] && model->tex[n]){
			MeshTex *tex = model->tex[n];
			for(int i = 0; i < tex->n; i++) if(!strcmp(model->sufs[n]->a[i].colormap, "attacker_engine.bmp")){
				tex->a[i].onBeginTexture = TextureParams::onBeginTextureEngine;
				tex->a[i].onEndTexture = TextureParams::onEndTextureEngine;
			}
		}

		init.create(*openGLState);
	} while(0);

	if(model){

		// This values could change every frame, so we assign them here.
		for(int n = 0; n < model->n; n++){
			MeshTex *pst = model->tex[n];
			for(int i = 0; i < pst->n; i++) if(pst->a[i].onBeginTexture){
				pst->a[i].onBeginTextureData = &tp;
				pst->a[i].onEndTextureData = &tp;
			}
		}

		double scale = modelScale;
		Mat4d mat;

		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);

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

		glScaled(-scale, scale, -scale);
		DrawMQOPose(model, NULL);

		glPopMatrix();
		glPopAttrib();
	}
}

void Attacker::drawtra(WarDraw *wd){
	st::drawtra(wd);
	if(wd->vw->gc->cullFrustum(pos, getHitRadius()))
		return;
	drawNavlights(wd, navlights);
}

void Attacker::drawOverlay(wardraw_t *){
	glCallList(overlayDisp);
}


