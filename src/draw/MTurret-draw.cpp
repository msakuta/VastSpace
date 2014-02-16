/** \file
 * \brief Implementation of MTurret drawing methods.
 */
#include "arms.h"
#include "Viewer.h"
#include "draw/material.h"
#include "draw/effects.h"
#include "draw/WarDraw.h"
#include "draw/ShaderBind.h"
#include "glsl.h"
extern "C"{
#include <clib/gl/gldraw.h>
}






#define STURRET_SCALE (.000005 * 3)
#define MTURRET_SCALE (.00005)
#define MTURRET_MAX_GIBS 30
#define MTURRET_BULLETSPEED 2.
#define MTURRET_VARIANCE (.001 * M_PI)
#define MTURRET_INTOLERANCE (M_PI / 20.)
#define MTURRETROTSPEED (.4*M_PI)
#define MTURRETMANUALROTSPEED (MTURRETROTSPEED * .5)

suf_t *MTurret::suf_turret = NULL;
suf_t *MTurret::suf_barrel = NULL;


void MTurret::draw(wardraw_t *wd){
	// Viewing volume culling
	if(wd->vw->gc->cullFrustum(pos, .03))
		return;
	// Scale too small culling
	if(fabs(wd->vw->gc->scale(pos)) * .03 < 2)
		return;
	MTurret *a = this;
	double scale;
	if(!suf_turret)
		suf_turret = CallLoadSUF("models/turretz1.bin");
	if(!suf_barrel)
		suf_barrel = CallLoadSUF("models/barrelz1.bin");

	if(suf_turret && suf_barrel){
		const double bscale = MTURRET_SCALE;
		static const GLfloat rotaxis2[16] = {
			-1,0,0,0,
			0,1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};

		if(const ShaderBind *sb = wd->getShaderBind())
			glUniform1i(sb->textureEnableLoc, 0);
		glPushMatrix();
		gldTranslate3dv(pos);
		gldMultQuat(rot);
		glRotated(deg_per_rad * a->py[1], 0., 1., 0.);
		glPushMatrix();
		gldScaled(bscale);
		glMultMatrixf(rotaxis2);
		DrawSUF(suf_turret, SUF_ATR, NULL);
		glPopMatrix();
/*		if(5 < scale)*/{
			const Vec3d pos(0., .00075, -0.0015);
			gldTranslate3dv(pos);
			glRotated(deg_per_rad * a->py[0], 1., 0., 0.);
			gldTranslate3dv(-pos);
			gldScaled(bscale);
			glMultMatrixf(rotaxis2);
			DrawSUF(suf_barrel, SUF_ATR, NULL);
		}
		glPopMatrix();
		if(const ShaderBind *sb = wd->getShaderBind())
			glUniform1i(sb->textureEnableLoc, 1);
	}
}

void MTurret::drawtra(wardraw_t *wd){
	Entity *pb = base;
	double bscale = MTURRET_SCALE, tscale = .00002;
	if(this->mf){
		struct random_sequence rs;
		Mat4d mat2, mat, rot;
		Vec3d pos, const pos0(0, 0, -.008);
		init_rseq(&rs, (long)this ^ *(long*)&wd->vw->viewtime);
		this->transform(mat);
		mat2 = mat.roty(this->py[1]);
		mat2.translatein(0., .001, -0.002);
		mat = mat2.rotx(this->py[0]);
		pos = mat.vp3(pos0);
		rot = mat;
		rot.vec3(3).clear();
		drawmuzzleflash4(pos, rot, .01, wd->vw->irot, &rs, wd->vw->pos);

		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		glCallList(muzzle_texture());
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE); // Add blend
		float f = mf / .1, fi = 1. - mf / .2;
		glColor4f(f,f,f,1);
		gldTextureBeam(wd->vw->pos, pos, pos + rot.vp3(-vec3_001) * .075 * fi, .02 * fi);
		glPopAttrib();
	}
}
