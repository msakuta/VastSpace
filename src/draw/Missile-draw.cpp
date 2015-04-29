/** \file
 * \brief Implemetation of Missile's drawing methods.
 */
#include "Missile.h"
#include "Viewer.h"
#include "draw/material.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/suf/sufdraw.h>
#include <clib/gl/gldraw.h>
}


#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_firetrail[] = {
	{0.05, COLOR32RGBA(255, 255, 212, 0)},
	{0.05, COLOR32RGBA(255, 191, 191, 255)},
	{0.4, COLOR32RGBA(111, 111, 111, 255)},
	{0.3, COLOR32RGBA(63, 63, 63, 127)},
};
const struct color_sequence cs_firetrail = DEFINE_COLSEQ(cnl_firetrail, (COLOR32)-1, .8);


/// \brief Initializes Tefpol object.
void Missile::initFpol(){
	if(WarSpace *ws = *w){
		static GLuint tex = 0;
		if(!tex){
			CallCacheBitmap("perlin.jpg", "textures/perlin.jpg", NULL, NULL);
			const gltestp::TexCacheBind *tcb = gltestp::FindTexture("perlin.jpg");
			if(tcb)
				tex = tcb->getList();
		}
		pf = ws->tepl->addTefpolMovable(pos, velo, vec3_000, &cs_firetrail, TEP3_THICK | TEP3_ROUGH, cs_firetrail.t);
		if(pf)
			pf->setTexture(tex);
	}
	else
		pf = NULL;
}

/// \brief Updates position of Tefpol object.
void Missile::updateFpol(){
	if(pf)
		pf->move(pos, avec3_000, cs_firetrail.t, 0/*pf->docked*/);
}

void Missile::draw(wardraw_t *wd){
	static suf_t *suf;
	static suftex_t *suft;
	static OpenGLState::weak_ptr<bool> init;

	if(wd->vw->gc->cullFrustum(pos, 10.))
		return;
	double pixels = 5. * fabs(wd->vw->gc->scale(pos));
	if(pixels < 5)
		return;

	if(!init) do{
		FILE *fp;
		suf = CallLoadSUF("models/missile.bin");
		if(!suf)
			break;
		CallCacheBitmap("missile_body.bmp", "models/missile_body.bmp", NULL, NULL);
		suft = gltestp::AllocSUFTex(suf);
		init.create(*openGLState);
	} while(0);
	if(suf){
		static const double normal[3] = {0., 1., 0.};
		double x;
		double pyr[3];
		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

		glPushMatrix();
		gldTranslate3dv(this->pos);
		gldMultQuat(this->rot);
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
		gldScaled(modelScale);
		glScalef(-1, 1, -1);
		DecalDrawSUF(suf, SUF_ATR, NULL, suft, NULL, NULL);
		glPopMatrix();

		glPopAttrib();
	}
}

void Missile::drawtra(wardraw_t *wd){
	Mat4d mat = rot.tomat4();
	mat.vec3(3) += pos;

	/* burning trail */
	if(/*p->target &&*/ 0. < fuel){
		struct gldBeamsData bd;
		Vec3d v0(.0, .0, 3.);
		const Vec3d &viewpos = wd->vw->pos;
		int lumi;
		struct random_sequence rs;
		double viewdist;
		const double scale = 10.;
		bd.cc = bd.solid = 0;
		init_rseq(&rs, (long)(wd->vw->viewtime * 1e6) + (unsigned long)this);
		lumi = rseq(&rs) % 256;
		Vec3d end = mat.vp3(v0);

		viewdist = (end - viewpos).len();
//		drawglow(end, wd->irot, .0015, COLOR32RGBA(255,255,255, 127 / (1. + viewdist)));
		glColor4ub(255,255,255,255);
		Vec4<GLubyte> col(255,255,255, 127 / (1. + viewdist));
		gldTextureGlow(end, 2., col, wd->vw->irot);

		gldBeams(&bd, viewpos, end, .01 * scale, COLOR32RGBA(3,127,191,0));
		v0[2] += .3 * scale;
		end = mat.vp3(v0);
		gldBeams(&bd, viewpos, end, .07 * scale, COLOR32RGBA(255,255,255,lumi));
		v0[2] += .3 * scale;
		end = mat.vp3(v0);
		gldBeams(&bd, viewpos, end, .09 * scale, COLOR32RGBA(255,127,63,lumi));
		v0[2] += .3 * scale;
		end = mat.vp3(v0);
		gldBeams(&bd, viewpos, end, .05 * scale, COLOR32RGBA(255,0,0,0));
	}
}

