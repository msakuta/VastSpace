#include "../Battleship.h"
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


void Battleship::draw(wardraw_t *wd){
	static Model *model = NULL;
	static OpenGLState::weak_ptr<bool> init;

	draw_healthbar(this, wd, health / getMaxHealth(), 300., -1, capacitor / maxenergy());

	if(wd->vw->gc->cullFrustum(pos, getHitRadius()))
		return;

	struct TextureParams{
		Battleship *p;
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
	} tp = {this, wd};

	static int engineAtrIndex = -1;
	if(!init) do{
		model = LoadMQOModel("models/battleship.mqo");
		if(model && model->sufs[0]){
			for(int i = 0; i < model->tex[0]->n; i++){
				const char *colormap = model->sufs[0]->a[i].colormap;
				if(colormap && !strcmp(colormap, "engine2.bmp")){
					engineAtrIndex = i;
					model->tex[0]->a[i].onBeginTexture = TextureParams::onBeginTextureEngine;
					model->tex[0]->a[i].onEndTexture = TextureParams::onEndTextureEngine;
				}
			}
		}

		init.create(*openGLState);
	} while(0);

	if(model){
		if(model->tex[0]){
			if(0 <= engineAtrIndex){
				model->tex[0]->a[engineAtrIndex].onBeginTextureData = &tp;
				model->tex[0]->a[engineAtrIndex].onEndTextureData = &tp;
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

void Battleship::drawtra(wardraw_t *wd){
	st::drawtra(wd);
	if(wd->vw->gc->cullFrustum(pos, getHitRadius()))
		return;
	drawNavlights(wd, navlights);
	static const Vec3d engines[] = {
		Vec3d(0, 0, 0) * modelScale * 1.25,
		Vec3d(28, 22, 0) * modelScale * 1.25,
		Vec3d(-28, 22, 0) * modelScale * 1.25,
		Vec3d(28, -22, 0) * modelScale * 1.25,
		Vec3d(-28, -22, 0) * modelScale * 1.25,
	};
	for(int n = 0; n < 2; n++){
		for(int i = 0; i < numof(engines); i++){
			drawCapitalBlast(wd, engines[i] + Vec3d((2 * n - 1) * 145., 0, 240 * modelScale), 20. * 1.25);
		}
	}
//	drawShield(wd);
}

void Battleship::drawOverlay(wardraw_t *){
	glCallList(disp);
}

int Battleship::popupMenu(PopupMenu &list){
	int ret = st::popupMenu(list);
	list.append("Equipments...", 0, "armswindow");
	return ret;
}

