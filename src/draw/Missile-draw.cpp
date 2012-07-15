/** \brief Implemetation of Missile's drawing methods.
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


void Missile::draw(wardraw_t *wd){
	static suf_t *suf;
	static suftex_t *suft;
	static OpenGLState::weak_ptr<bool> init;

	if(wd->vw->gc->cullFrustum(pos, .01))
		return;
	double pixels = .005 * fabs(wd->vw->gc->scale(pos));
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
		gldScaled(1e-5);
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
		Vec3d v0(.0, .0, .00300);
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
		gldTextureGlow(end, .0020, col, wd->vw->irot);

		gldBeams(&bd, viewpos, end, .00001 * scale, COLOR32RGBA(3,127,191,0));
		v0[2] += .0003 * scale;
		end = mat.vp3(v0);
		gldBeams(&bd, viewpos, end, .00007 * scale, COLOR32RGBA(255,255,255,lumi));
		v0[2] += .0003 * scale;
		end = mat.vp3(v0);
		gldBeams(&bd, viewpos, end, .00009 * scale, COLOR32RGBA(255,127,63,lumi));
		v0[2] += .0003 * scale;
		end = mat.vp3(v0);
		gldBeams(&bd, viewpos, end, .00005 * scale, COLOR32RGBA(255,0,0,0));
	}
}

