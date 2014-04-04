/** \file
 * \brief Implementation of RStation class's graphics.
 */
#include "../RStation.h"
#include "Autonomous.h" // draw_healthbar
#include "draw/material.h"
#include "astrodraw.h"
#include "draw/WarDraw.h"
#include "draw/mqoadapt.h"
#include "draw/OpenGLState.h"
#include "glw/PopupMenu.h"
#include "Viewer.h"
#include "Game.h"
extern "C"{
#include <clib/suf/sufdraw.h>
#include <clib/suf/sufbin.h>
#include <clib/gl/gldraw.h>
}

Model *RStation::model = NULL;

void RStation::draw(wardraw_t *wd){
	if(!w)
		return;

	/* cull object */
/*	if(scepter_cull(pt, wd))
		return;*/
	wd->lightdraws++;

	draw_healthbar(this, wd, this->health / getMaxHealth(), 3., this->occupytime / 10., this->ru / RSTATION_MAX_RU);

	static OpenGLState::weak_ptr<bool> init;
	if(init == 0){
		model = LoadMQOModel("models/rstation.mqo");
		init.create(*openGLState);
	}

	if(model){
		static const double normal[3] = {0., 1., 0.};
		const double scale = RSTATION_SCALE;

		glPushMatrix();
		gldTranslate3dv(this->pos);
		gldScaled(scale);
		gldMultQuat(this->rot);
		glScalef(-1, 1, -1);
		DrawMQOPose(model, NULL);
		glPopMatrix();
	}
}

void RStation::drawtra(wardraw_t *wd){
	if(!wd->vw->gc->cullFrustum(this->pos, 35.)){
		int i, n = 3;
		static const avec3_t pos0[] = {
			{0, 20.5 * RSTATION_SCALE, 0},
			{4.71 * RSTATION_SCALE, 0, -33.7 * RSTATION_SCALE},
			{-4.71 * RSTATION_SCALE, 0, -33.7 * RSTATION_SCALE},
			{4.71 * RSTATION_SCALE, 0, 33.7 * RSTATION_SCALE},
			{-4.71 * RSTATION_SCALE, 0, 33.7 * RSTATION_SCALE},
			{33.7 * RSTATION_SCALE, 0, -4.71 * RSTATION_SCALE},
			{-33.7 * RSTATION_SCALE, 0, -4.71 * RSTATION_SCALE},
			{33.7 * RSTATION_SCALE, 0, 4.71 * RSTATION_SCALE},
			{-33.7 * RSTATION_SCALE, 0, 4.71 * RSTATION_SCALE},
		};
		avec3_t pos;
		double rad = .05;
		Mat4d mat;
		double t;
		GLubyte col[4] = {255, 31, 31, 255};
		transform(mat);

		/* color calculation of static navlights */
		t = fmod(game->player->gametime * .3, 2.);
		if(t < 1.){
			rad *= (t + 1.) / 2.;
			col[3] = GLubyte(col[3] * t);
		}
		else{
			rad *= (2. - t + 1.) / 2.;
			col[3] *= GLubyte(2. - t);
		}
		for(i = 0 ; i < numof(pos0); i++){
			mat4vp3(pos, mat, pos0[i]);
			gldSpriteGlow(pos, rad, col, wd->vw->irot);
		}
		for(i = 0; i < numof_rstation_hb; i++){
			glPushMatrix();
			gldTranslate3dv(this->pos);
			gldTranslate3dv(rstation_hb[i].org);
			Mat4d rot = rstation_hb[i].rot.tomat4();
			glMultMatrixd(rot);
			hitbox_draw(this, rstation_hb[i].sc);
			glPopMatrix();
		}
	}
}
