/** \file
 * \brief Implementation of Shipyard classe's drawing methods.
 */
#include "Shipyard.h"
#include "judge.h"
#include "draw/material.h"
#include "../Sceptor.h"
#include "draw/WarDraw.h"
#include "draw/mqoadapt.h"
#include "draw/OpenGLState.h"
#include "glw/popup.h"
extern "C"{
#include <clib/gl/gldraw.h>
}

#define texture(e) glMatrixMode(GL_TEXTURE); e; glMatrixMode(GL_MODELVIEW);


int Shipyard::popupMenu(PopupMenu &list){
	int ret = st::popupMenu(list);
//	list.append("Build Window", 'b', "buildmenu");
	list.append("Dock Window", 'd', "dockmenu");
	return ret;
}

Motion *Shipyard::motions[2];

void Shipyard::draw(wardraw_t *wd){
	Shipyard *const p = this;
	static Model *model = NULL;
	static OpenGLState::weak_ptr<bool> init;
	static suftex_t *pst;
	static suf_t *sufbase = NULL;
	if(!Entity::w)
		return;

	/* cull object */
/*	if(beamer_cull(this, wd))
		return;*/
//	wd->lightdraws++;

	draw_healthbar(this, wd, health / maxhealth(), hitradius(), -1., capacitor / maxenergy());

	if(wd->vw->gc->cullFrustum(pos, hitradius()))
		return;
#if 1
	if(!init) do{
		model = LoadMQOModel("models/shipyard.mqo");
		motions[0] = LoadMotion("models/shipyard_door.mot");
		motions[1] = LoadMotion("models/shipyard_door2.mot");
		init.create(*openGLState);
	} while(0);
	if(model){
		MotionPose mp[2];
		motions[0]->interpolate(mp[0], doorphase[0] * 10.);
		motions[1]->interpolate(mp[1], doorphase[1] * 10.);
		mp[0].next = &mp[1];

		double scale = SCARRY_SCALE;
		Mat4d mat;

		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);
		gldScaled(scale);
		glScalef(-1, 1, -1);

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

		DrawMQOPose(model, mp);

		glPopMatrix();
		glPopAttrib();
	}
#endif
}

void Shipyard::drawtra(wardraw_t *wd){
	st::drawtra(wd);
	Shipyard *p = this;
	Shipyard *pt = this;
	Mat4d mat;
	Vec3d pa, pb, pa0(.01, 0, 0), pb0(-.01, 0, 0);
	double scale;

/*	if(scarry_cull(pt, wd))
		return;*/

	if(wd->vw->gc->cullFrustum(pos, hitradius()))
		return;

	scale = fabs(wd->vw->gc->scale(this->pos));

	transform(mat);

	pa = pt->rot.trans(pa0);
	pa += pt->pos;
	pb = pt->rot.trans(pb0);
	pb += pt->pos;
	glColor4ub(255,255,9,255);
	glBegin(GL_LINES);
	glVertex3dv(pa);
	glVertex3dv(pb);
	glEnd();

	{
		int i;
		static const avec3_t lights[] = {
			{0, 520 * SCARRY_SCALE, 220 * SCARRY_SCALE},
			{0, -520 * SCARRY_SCALE, 220 * SCARRY_SCALE},
			{140 * SCARRY_SCALE, 370 * SCARRY_SCALE, 220 * SCARRY_SCALE},
			{-140 * SCARRY_SCALE, 370 * SCARRY_SCALE, 220 * SCARRY_SCALE},
			{140 * SCARRY_SCALE, -370 * SCARRY_SCALE, 220 * SCARRY_SCALE},
			{-140 * SCARRY_SCALE, -370 * SCARRY_SCALE, 220 * SCARRY_SCALE},
		};
		avec3_t pos;
		double rad = .01;
		double t;
		GLubyte col[4] = {255, 31, 31, 255};
		random_sequence rs;
		init_rseq(&rs, (unsigned long)this);

		/* color calculation of static navlights */
		t = fmod(wd->vw->viewtime + drseq(&rs) * 2., 2.);
		if(t < 1.){
			rad *= (t + 1.) / 2.;
			col[3] *= t;
		}
		else{
			rad *= (2. - t + 1.) / 2.;
			col[3] *= 2. - t;
		}
		for(i = 0 ; i < numof(lights); i++){
			mat4vp3(pos, mat, lights[i]);
			gldSpriteGlow(pos, rad, col, wd->vw->irot);
		}

		/* runway lights */
		if(1 < scale * .01){
			col[0] = 0;
			col[1] = 191;
			col[2] = 255;
			for(i = 0 ; i <= 10; i++){
				avec3_t pos0;
				pos0[0] = 180 * SCARRY_SCALE;
				pos0[1] = 60 * SCARRY_SCALE;
				pos0[2] = (i * -460 + (10 - i) * -960) * SCARRY_SCALE / 10;
				rad = .005 * (1. - fmod(i / 10. + t / 2., 1.));
				col[3] = 255/*rad * 255 / .01*/;
				mat4vp3(pos, mat, pos0);
				gldSpriteGlow(pos, rad, col, wd->vw->irot);
				pos0[0] = 20 * SCARRY_SCALE;
				mat4vp3(pos, mat, pos0);
				gldSpriteGlow(pos, rad, col, wd->vw->irot);
				pos0[1] = -60 * SCARRY_SCALE;
				mat4vp3(pos, mat, pos0);
				gldSpriteGlow(pos, rad, col, wd->vw->irot);
				pos0[0] = 180 * SCARRY_SCALE;
				mat4vp3(pos, mat, pos0);
				gldSpriteGlow(pos, rad, col, wd->vw->irot);
			}
		}

/*		for(i = 0; i < numof(p->turrets); i++)
			mturret_drawtra(&p->turrets[i], pt, wd);*/
	}

}
