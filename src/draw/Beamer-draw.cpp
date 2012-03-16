#include "../Beamer.h"
#include "Player.h"
#include "Bullet.h"
#include "CoordSys.h"
#include "Viewer.h"
#include "cmd.h"
//#include "glwindow.h"
#include "judge.h"
#include "astrodef.h"
#include "stellar_file.h"
#include "astro_star.h"
#include "serial_util.h"
#include "draw/material.h"
//#include "sensor.h"
#include "motion.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/mqoadapt.h"
extern "C"{
#include "bitmap.h"
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/suf/sufbin.h>
#include <clib/suf/sufdraw.h>
#include <clib/suf/sufvbo.h>
#include <clib/GL/gldraw.h>
#include <clib/GL/cull.h>
#include <clib/GL/multitex.h>
#include <clib/wavsound.h>
#include <clib/zip/UnZip.h>
}
#include <assert.h>
#include <string.h>
#include <gl/glext.h>



/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289

#define MAX_SHIELD_AMOUNT 5000.
#define BEAMER_HEALTH 15000.
#define BEAMER_SCALE .0002
#define BEAMER_MAX_SPEED .1
#define BEAMER_ACCELERATE .05
#define BEAMER_MAX_ANGLESPEED .4
#define BEAMER_ANGLEACCEL .2
#define BEAMER_SHIELDRAD .09



suf_t *Beamer::sufbase = NULL;
const double Beamer::sufscale = BEAMER_SCALE;

void Beamer::draw(wardraw_t *wd){
	Beamer *const p = this;
//	static int init = 0;
//	static suftex_t *pst;
	static OpenGLState::weak_ptr<bool> init;
	static Model *model = NULL;
	if(!w)
		return;

	/* cull object */
	if(cull(wd))
		return;
//	wd->lightdraws++;

	draw_healthbar(this, wd, health / BEAMER_HEALTH, .1, shieldAmount / MAX_SHIELD_AMOUNT, capacitor / frigate_mn.capacity);

	if(!init) do{
//		suftexparam_t stp, stp2;
/*		beamer_s.sufbase = LZUC(lzw_assault);*/
/*		beamer_s.sufbase = LZUC(lzw_beamer);*/
		sufbase = CallLoadSUF("models/beamer.bin");
		if(!sufbase) break;
		model = LoadMQOModel("models/beamer.mqo");
		CacheSUFMaterials(sufbase);
//		cache_bridge();
//		pst = AllocSUFTex(sufbase);
/*		beamer_hb[1].rot[2] = sin(M_PI / 3.);
		beamer_hb[1].rot[3] = cos(M_PI / 3.);
		beamer_hb[2].rot[2] = sin(-M_PI / 3.);
		beamer_hb[2].rot[3] = cos(-M_PI / 3.);*/
		init.create(*openGLState);
	} while(0);
	if(sufbase){
		double scale = BEAMER_SCALE;
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

	drawCapitalBlast(wd, Vec3d(0,-0.003,.06), .01);

	drawShield(wd);

#if 1
	if(charge){
		int i;
		GLubyte azure[4] = {63,0,255,95}, bright[4] = {127,63,255,255};
		Vec3d muzzle, muzzle0(0., 0., -.100);
		double glowrad;
		muzzle = mat.vp3(muzzle0);
/*		quatrot(muzzle, pt->rot, muzzle0);
		VECADDIN(muzzle, pt->pos);*/

		if(0. < charge && charge < 4.){
			double beamrad;
			Vec3d end, end0(0., 0., -10.);
			end0[2] = -beamlen;
			end = mat.vp3(end0);
/*			quatrot(end, pt->rot, end0);
			VECADDIN(end, pt->pos);*/
			beamrad = (charge < .5 ? charge / .5 : 3.5 < charge ? (4. - charge) / .5 : 1.) * .005;
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
		glowrad = charge < 4. ? charge / 4. * .02 : (6. - charge) / 2. * .02;
		gldSpriteGlow(muzzle, glowrad * 3., azure, wd->vw->irot);
		gldSpriteGlow(muzzle, glowrad, bright, wd->vw->irot);
	}
#endif
}

