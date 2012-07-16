/** \file
 * \brief Implementation of LMissileTurret drawing methods.
 */
#include "LTurret.h"
#include "Viewer.h"
#include "draw/effects.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/mqoadapt.h"
extern "C"{
#include <clib/gl/gldraw.h>
}


void LMissileTurret::draw(WarDraw *wd){
	static OpenGLState::weak_ptr<bool> init;

	// Viewing volume culling
	if(wd->vw->gc->cullFrustum(pos, .03))
		return;
	// Scale too small culling
	if(fabs(wd->vw->gc->scale(pos)) * .03 < 2)
		return;
	static Model *model = NULL;
	static Motion *motions[1];
	if(!init){
		model = LoadMQOModel("models/missile_launcher.mqo");
		motions[0] = LoadMotion("models/missile_launcher_deploy.mot");
		init.create(*openGLState);
	}

	// Interpolate motion poses.
	MotionPose mp[1];
	motions[0]->interpolate(mp[0], deploy * 10.);

	glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_ENABLE_BIT);
	glPushMatrix();
	gldTranslate3dv(pos);
	gldMultQuat(rot);
	glRotated(deg_per_rad * this->py[1], 0., 1., 0.);
	gldScaled(bscale);
	glScalef(-1,1,-1);
	DrawMQOPose(model, mp);
	glPopMatrix();
	glPopAttrib();
}

void LMissileTurret::drawtra(WarDraw *wd){
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
