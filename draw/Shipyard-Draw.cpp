/** \file
 * \brief Implementation of Shipyard classe's drawing methods.
 */
#include "Shipyard.h"
#include "judge.h"
#include "draw/material.h"
#include "../sceptor.h"
#include "draw/WarDraw.h"
#include "draw/mqoadapt.h"
#include "draw/OpenGLState.h"
extern "C"{
#include <clib/gl/gldraw.h>
}

#define texture(e) glMatrixMode(GL_TEXTURE); e; glMatrixMode(GL_MODELVIEW);


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
		model = LoadMQOModel("models/shipyard.mqo", SCARRY_SCALE);
/*		sufbase = CallLoadSUF("models/spacecarrier.bin");
		if(!sufbase) break;
		CallCacheBitmap("bridge.bmp", "bridge.bmp", NULL, NULL);
		CallCacheBitmap("beamer_panel.bmp", "beamer_panel.bmp", NULL, NULL);
		CallCacheBitmap("bricks.bmp", "bricks.bmp", NULL, NULL);
		CallCacheBitmap("runway.bmp", "runway.bmp", NULL, NULL);
		suftexparam_t stp;
		stp.flags = STP_MAGFIL | STP_MINFIL | STP_ENV;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		stp.env = GL_ADD;
		stp.mipmap = 0;
		CallCacheBitmap5("engine2.bmp", "engine2br.bmp", &stp, "engine2.bmp", NULL);
		pst = AllocSUFTex(sufbase);
		extern GLuint screentex;
		glNewList(pst->a[0].list, GL_COMPILE);
		glBindTexture(GL_TEXTURE_2D, screentex);
		glTexEnvi(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glDisable(GL_LIGHTING);
		glEndList();*/
		init.create(*openGLState);
	} while(0);
	if(model){
		static const double normal[3] = {0., 1., 0.};
		double scale = SCARRY_SCALE;
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
		glMultMatrixd(rotaxis);

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

		DrawMQOPose(model, NULL);

#if 0
		Mat4d modelview, proj;
		glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
		glGetDoublev(GL_PROJECTION_MATRIX, proj);
		Mat4d trans = proj * modelview;
		texture((glPushMatrix(),
			glScaled(1./2., 1./2., 1.),
			glTranslated(1, 1, 0),
			glMultMatrixd(trans)
		));
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGendv(GL_S, GL_EYE_PLANE, Vec4d(1,0,0,0));
		glEnable(GL_TEXTURE_GEN_S);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGendv(GL_T, GL_EYE_PLANE, Vec4d(0,1,0,0));
		glEnable(GL_TEXTURE_GEN_T);
		glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGeniv(GL_R, GL_EYE_PLANE, Vec4<int>(0,0,1,0));
		glEnable(GL_TEXTURE_GEN_R);
		glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGeniv(GL_Q, GL_EYE_PLANE, Vec4<int>(0,0,0,1));
		glEnable(GL_TEXTURE_GEN_Q);

		glPushMatrix();
		glScaled(scale, scale, scale);
		glMultMatrixd(rotaxis);
		DecalDrawSUF(sufbase, SUF_ATR, NULL, pst, NULL, NULL);
		glPopMatrix();

		texture((glPopMatrix()));
#endif
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
			{100 * SCARRY_SCALE, -360 * SCARRY_SCALE, -600 * SCARRY_SCALE},
			{100 * SCARRY_SCALE,  360 * SCARRY_SCALE, -600 * SCARRY_SCALE},
			{ 280 * SCARRY_SCALE,   20 * SCARRY_SCALE, 520 * SCARRY_SCALE},
			{ 280 * SCARRY_SCALE,  -20 * SCARRY_SCALE, 520 * SCARRY_SCALE},
			{-280 * SCARRY_SCALE,   20 * SCARRY_SCALE, 520 * SCARRY_SCALE},
			{-280 * SCARRY_SCALE,  -20 * SCARRY_SCALE, 520 * SCARRY_SCALE},
			{-280 * SCARRY_SCALE,   20 * SCARRY_SCALE, -300 * SCARRY_SCALE},
			{-280 * SCARRY_SCALE,  -20 * SCARRY_SCALE, -300 * SCARRY_SCALE},
			{ 280 * SCARRY_SCALE,   20 * SCARRY_SCALE, -480 * SCARRY_SCALE},
			{ 280 * SCARRY_SCALE,  -20 * SCARRY_SCALE, -480 * SCARRY_SCALE},
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
				pos0[0] = -160 * SCARRY_SCALE;
				pos0[1] = 20 * SCARRY_SCALE + .0025;
				pos0[2] = (i * -460 + (10 - i) * -960) * SCARRY_SCALE / 10;
				rad = .005 * (1. - fmod(i / 10. + t / 2., 1.));
				col[3] = 255/*rad * 255 / .01*/;
				mat4vp3(pos, mat, pos0);
				gldSpriteGlow(pos, rad, col, wd->vw->irot);
				pos0[0] = -40 * SCARRY_SCALE;
				mat4vp3(pos, mat, pos0);
				gldSpriteGlow(pos, rad, col, wd->vw->irot);
				pos0[1] = -20 * SCARRY_SCALE - .0025;
				mat4vp3(pos, mat, pos0);
				gldSpriteGlow(pos, rad, col, wd->vw->irot);
				pos0[0] = -160 * SCARRY_SCALE;
				mat4vp3(pos, mat, pos0);
				gldSpriteGlow(pos, rad, col, wd->vw->irot);
			}
		}

/*		for(i = 0; i < numof(p->turrets); i++)
			mturret_drawtra(&p->turrets[i], pt, wd);*/
	}

}
