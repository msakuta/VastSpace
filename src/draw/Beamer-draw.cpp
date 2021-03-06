/** \file
 * \brief Implementation of Beamer class's drawing methods.
 */
#include "../Beamer.h"
#include "draw/material.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/mqoadapt.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/GL/gldraw.h>
}



/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289

#define MAX_SHIELD_AMOUNT 5000.
#define BEAMER_HEALTH 15000.
#define BEAMER_MAX_SPEED 100.
#define BEAMER_ACCELERATE 50.
#define BEAMER_MAX_ANGLESPEED .4
#define BEAMER_ANGLEACCEL .2
#define BEAMER_SHIELDRAD 90.


void Beamer::cache_bridge(void){
	const gltestp::TexCacheBind *tcb;
	tcb = gltestp::FindTexture("bridge.bmp");
	if(!tcb)
		CallCacheBitmap("bridge.bmp", "bridge.bmp", NULL, NULL);
	tcb = gltestp::FindTexture("beamer_panel.bmp");
	if(!tcb)
		CallCacheBitmap("beamer_panel.bmp", "beamer_panel.bmp", NULL, NULL);
/*	if(!FindTexCache("bridge.bmp")){
		suftexparam_t stp;
		stp.bmi = lzuc(lzw_bridge, sizeof lzw_bridge, NULL);
		stp.env = GL_MODULATE;
		stp.mipmap = 0;
		stp.alphamap = 0;
		stp.magfil = GL_LINEAR;
		CacheSUFMTex("bridge.bmp", &stp, NULL);
		free(stp.bmi);
	}
	if(!FindTexCache("beamer_panel.bmp")){
		BITMAPINFO *bmi;
		bmi = lzuc(lzw_beamer_panel, sizeof lzw_beamer_panel, NULL);
		CacheSUFTex("beamer_panel.bmp", bmi, 0);
		free(bmi);
	}*/
}

//const double Beamer::sufscale = BEAMER_SCALE;

void Beamer::draw(wardraw_t *wd){
	Beamer *const p = this;
	static OpenGLState::weak_ptr<bool> init;
	static Model *model = NULL;
	if(!w)
		return;

	/* cull object */
	if(cull(wd))
		return;
//	wd->lightdraws++;

	draw_healthbar(this, wd, health / BEAMER_HEALTH, 100., shieldAmount / MAX_SHIELD_AMOUNT, capacitor / frigate_mn.capacity);

	if(!init) do{
		model = LoadMQOModel("models/beamer.mqo");
		init.create(*openGLState);
	} while(0);
	if(model){
		const double scale = modelScale;
		Mat4d mat;

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
	}
}

void Beamer::drawtra(wardraw_t *wd){
	st::drawtra(wd);
	Beamer *p = this;
	Mat4d mat;

/*	if(p->dock && p->undocktime == 0)
		return;*/

	transform(mat);

	drawCapitalBlast(wd, Vec3d(0,-3.,60.), 10.);

	drawShield(wd);

	drawNavlights(wd, navlights, &mat);

#if 1
	if(charge){
		int i;
		GLubyte azure[4] = {63,0,255,95}, bright[4] = {127,63,255,255};
		Vec3d muzzle, muzzle0(0., 0., -100.);
		double glowrad;
		muzzle = mat.vp3(muzzle0);
/*		quatrot(muzzle, pt->rot, muzzle0);
		VECADDIN(muzzle, pt->pos);*/

		if(0. < charge && charge < 4.){
			double beamrad;
			Vec3d end, end0(0., 0., -10000.);
			end0[2] = -beamlen;
			end = mat.vp3(end0);
/*			quatrot(end, pt->rot, end0);
			VECADDIN(end, pt->pos);*/
			beamrad = (charge < .5 ? charge / .5 : 3.5 < charge ? (4. - charge) / .5 : 1.) * 5.;
			for(i = 0; i < 5; i++){
				Vec3d p0, p1;
				p0 = muzzle * i / 5.;
				p0 += end * (5 - i) / 5.;
				p1 = muzzle * (i + 1) / 5.;
				p1 += end * (5 - i - 1) / 5.;
				glColor4ub(63,191,255, GLubyte(MIN(1., charge / 2.) * 255));
				gldBeam(wd->vw->pos, p0, p1, beamrad);
				glColor4ub(63,0,255, GLubyte(MIN(1., charge / 2.) * 95));
				gldBeam(wd->vw->pos, p0, p1, beamrad * 5.);
			}
#if 0
			for(i = 0; i < 3; i++){
				int j;
				avec3_t pos1, pos;
				struct random_sequence rs;
				struct gldBeamsData bd;
				init_rseq(&rs, (unsigned long)pt + i);
				bd.cc = 0;
				bd.solid = 0;
				glColor4ub(63,0,255, MIN(1., p->charge / 2.) * 95);
				VECCPY(pos1, pt->pos);
				pos1[0] += (drseq(&rs) - .5) * .5;
				pos1[1] += (drseq(&rs) - .5) * .5;
				pos1[2] += (drseq(&rs) - .5) * .5;
				init_rseq(&rs, *(unsigned long*)&wd->gametime + i);
				for(j = 0; j <= 64; j++){
					int c;
					avec3_t p1;
					for(c = 0; c < 3; c++)
						p1[c] = (muzzle[c] * j + end[c] * (64 - j)) / 64. + (drseq(&rs) - .5) * .05;
/*					glColor4ub(63,127,255, MIN(1., p->charge / 2.) * 191);*/
					gldBeams(&bd, wd->view, p1, beamrad * .5, COLOR32RGBA(63,127,255,MIN(1., p->charge / 2.) * 191));
				}
			}
#endif
		}

#if 0
		if(0) for(i = 0; i < 64; i++){
			int j;
			avec3_t pos1, pos;
			struct random_sequence rs;
			struct gldBeamsData bd;
			init_rseq(&rs, (unsigned long)pt + i);
			bd.cc = 0;
			bd.solid = 0;
			glColor4ub(63,0,255, MIN(1., p->charge / 2.) * 95);
			VECCPY(pos1, pt->pos);
			pos1[0] += (drseq(&rs) - .5) * .5;
			pos1[1] += (drseq(&rs) - .5) * .5;
			pos1[2] += (drseq(&rs) - .5) * .5;
			init_rseq(&rs, *(unsigned long*)&wd->gametime + i);
			for(j = 0; j < 16; j++){
				int c;
				for(c = 0; c < 3; c++)
					pos[c] = (pt->pos[c] * j + pos1[c] * (16 - j)) / 16. + (drseq(&rs) - .5) * .01;
				gldBeams(&bd, wd->view, pos, .001, COLOR32RGBA(255, j * 256 / 16, j * 64 / 16, MIN(1., p->charge / 2.) * j * 128 / 16));
			}
		}
#endif
		glowrad = charge < 4. ? charge / 4. * 20. : (6. - charge) / 2. * 20.;
		gldSpriteGlow(muzzle, glowrad * 3., azure, wd->vw->irot);
		gldSpriteGlow(muzzle, glowrad, bright, wd->vw->irot);
	}
#endif
}

void Beamer::drawOverlay(WarDraw *){
	glCallList(disp);
}
