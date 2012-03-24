/** \file
 * \brief Implementation of LMissileTurret drawing methods.
 */
#include "LTurret.h"
#include "Viewer.h"
#include "../Missile.h"
#include "draw/material.h"
#include "draw/effects.h"
#include "draw/WarDraw.h"
#include "draw/ShaderBind.h"
#include "glsl.h"
extern "C"{
#include <clib/gl/gldraw.h>
}


void LMissileTurret::draw(wardraw_t *wd){
	// Viewing volume culling
	if(wd->vw->gc->cullFrustum(pos, .03))
		return;
	// Scale too small culling
	if(fabs(wd->vw->gc->scale(pos)) * .03 < 2)
		return;
	static OpenGLState::weak_ptr<suf_t *> suf_turret = NULL, suf_barrel = NULL;
	static suftex_t *pst_turret, *pst_barrel;
	double scale;
	if(!suf_turret){
		suf_turret.create(*openGLState);
		*suf_turret = CallLoadSUF("models/missile_launcher.bin");
		if(*suf_turret){
			CacheSUFMaterials(*suf_turret);
			pst_turret = gltestp::AllocSUFTex(*suf_turret);
		}
	}
	if(!suf_barrel){
		suf_barrel.create(*openGLState);
		*suf_barrel = CallLoadSUF("models/missile_launcher_barrel.bin");
		if(*suf_barrel){
			CacheSUFMaterials(*suf_barrel);
			pst_barrel = gltestp::AllocSUFTex(*suf_barrel);
		}
	}

/*	static const GLfloat rotaxis2[16] = {
		-1,0,0,0,
		0,1,0,0,
		0,0,-1,0,
		0,0,0,1,
	};*/

	glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_ENABLE_BIT);
	glPushMatrix();
	gldTranslate3dv(pos);
	gldMultQuat(rot);
	glRotated(deg_per_rad * this->py[1], 0., 1., 0.);
	glPushMatrix();
	gldScaled(bscale);
//	glMultMatrixf(rotaxis2);
	glScalef(-1,1,-1);
	if(*suf_turret)
		DecalDrawSUF(*suf_turret, SUF_ATR, NULL, pst_turret, NULL, NULL);
	glPopMatrix();
	if(*suf_barrel){
		const Vec3d pos = Vec3d(0, 200, 0) * deploy;
		Vec3d joint = Vec3d(0, 120, 60);
		gldScaled(bscale);
		gldTranslate3dv(pos + joint);
		glRotated(deg_per_rad * this->py[0], 1., 0., 0.);
		gldTranslate3dv(-joint);
//		glMultMatrixf(rotaxis2);
		glScalef(-1,1,-1);
		DecalDrawSUF(*suf_barrel, SUF_ATR, NULL, pst_barrel, NULL, NULL);
	}
	glPopMatrix();
	glPopAttrib();
}

void LMissileTurret::drawtra(wardraw_t *wd){
	Entity *pb = base;
//	double bscale = 1;
	if(this->mf){
		struct random_sequence rs;
		Mat4d mat2, mat, rot;
		Vec3d const barrelpos = bscale * Vec3d(0, 200, 0) * deploy;
		Vec3d const joint = bscale * Vec3d(0, 120, 60);
		Vec3d pos, const muzzlepos = bscale * Vec3d(80. * (ammo % 3 - 1), (40. + 40. + 80. * (ammo / 3)), 0);
		init_rseq(&rs, (long)this ^ *(long*)&wd->vw->viewtime);
		this->transform(mat);
		mat2 = mat.roty(this->py[1]);
		mat2.translatein(barrelpos + joint);
		mat = mat2.rotx(this->py[0]);
		mat.translatein(-joint);
		pos = mat.vp3(muzzlepos);
		rot = mat;
		rot.vec3(3).clear();
//		drawmuzzleflash4(pos, rot, .005, wd->vw->irot, &rs, wd->vw->pos);

		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		glCallList(muzzle_texture());
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE); // Add blend
		float f = mf / .3 * 2, fi = 1. - mf / .3;
		glColor4f(f,f,f,1);
		gldTextureBeam(wd->vw->pos, pos, pos + rot.vp3(-vec3_001) * .050 * fi, .020 * fi);
		glPopAttrib();
	}
}
