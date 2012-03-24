/** \file
 * \brief Implementation of GatlingTurret drawing methods.
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

#define GATLINGTURRET_SCALE (.00005)


void GatlingTurret::draw(wardraw_t *wd){
	// Viewing volume culling
	if(wd->vw->gc->cullFrustum(pos, .03))
		return;
	// Scale too small culling
	if(fabs(wd->vw->gc->scale(pos)) * .03 < 2)
		return;
	static suf_t *suf_turret = NULL;
	static suf_t *suf_barrel = NULL;
	static suf_t *suf_barrels = NULL;
	double scale;
	if(!suf_turret)
		suf_turret = CallLoadSUF("models/turretg1.bin");
	if(!suf_barrel)
		suf_barrel = CallLoadSUF("models/barrelg1.bin");
	if(!suf_barrels)
		suf_barrels = CallLoadSUF("models/barrelsg1.bin");

	{
		const double bscale = GATLINGTURRET_SCALE;
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
		glRotated(deg_per_rad * this->py[1], 0., 1., 0.);
		glPushMatrix();
		gldScaled(bscale);
		glMultMatrixf(rotaxis2);
		DrawSUF(suf_turret, SUF_ATR, NULL);
		glPopMatrix();
/*		if(5 < scale)*/{
			gldScaled(bscale);
			gldTranslate3dv(barrelpos);
			glRotated(deg_per_rad * this->py[0], 1., 0., 0.);
			gldTranslate3dv(-barrelpos);
			glMultMatrixf(rotaxis2);
			DrawSUF(suf_barrel, SUF_ATR, NULL);
			gldTranslate3dv(barrelpos);
			glRotated(barrelrot * deg_per_rad/*w->war_time() * 360. / reloadtime() / 3*/, 0, 0, 1);
			gldTranslate3dv(-barrelpos);
			DrawSUF(suf_barrels, SUF_ATR, NULL);
		}
		glPopMatrix();
		if(const ShaderBind *sb = wd->getShaderBind())
			glUniform1i(sb->textureEnableLoc, 1);
	}
}

void GatlingTurret::drawtra(wardraw_t *wd){
	Entity *pb = base;
	double bscale = GATLINGTURRET_SCALE;
	if(this->mf){
		struct random_sequence rs;
		Mat4d mat2, mat, rot;
		Vec3d pos, const muzzlepos(0, .003 * .5, -.0134 * .5);
		init_rseq(&rs, (long)this ^ *(long*)&wd->vw->viewtime);
		this->transform(mat);
		mat2 = mat.roty(this->py[1]);
		mat2.translatein(barrelpos * bscale);
		mat = mat2.rotx(this->py[0]);
		mat.translatein(-barrelpos * bscale);
		pos = mat.vp3(muzzlepos);
		rot = mat;
		rot.vec3(3).clear();
		drawmuzzleflash4(pos, rot, .005, wd->vw->irot, &rs, wd->vw->pos);

		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		glCallList(muzzle_texture());
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE); // Add blend
		float f = mf / .075 * 2, fi = 1. - mf / .075;
		glColor4f(f,f,f,1);
		gldTextureBeam(wd->vw->pos, pos, pos + rot.vp3(-vec3_001) * .03 * fi, .01 * fi);
		glPopAttrib();
	}
}
