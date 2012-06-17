/** \file
 * \brief Implementation of LTurret drawing methods.
 */
#include "LTurret.h"
#include "Viewer.h"
#include "Bullet.h"
#include "draw/material.h"
#include "draw/effects.h"
#include "draw/WarDraw.h"
#include "draw/ShaderBind.h"
#include "glsl.h"
#include "draw/mqoadapt.h"
extern "C"{
#include <clib/gl/gldraw.h>
}


void LTurret::draw(wardraw_t *wd){
	static OpenGLState::weak_ptr<bool> init;

	// Viewing volume culling
	if(wd->vw->gc->cullFrustum(pos, .03))
		return;
	// Scale too small culling
	if(fabs(wd->vw->gc->scale(pos)) * .03 < 2)
		return;
	static Model *model = NULL;
	static Motion *motions[2];
	double scale;
	static int turretIndex = -1;
	static int barrelIndex = -1;
	if(!init){
		model = LoadMQOModel("models/lturret1.mqo");
		motions[0] = LoadMotion("models/lturret_pitch.mot");
		motions[1] = LoadMotion("models/lturret_blowback.mot");

		// Precache barrel model indices to avoid string match every frame and every instance.
		for(int i = 0; i < model->n; i++){
			if(model->bones[i]->name == "lturret")
				turretIndex = i;
			if(model->bones[i]->name == "lbarrel")
				barrelIndex = i;
		}
		init.create(*openGLState);
	}

	const double bscale = .001;

	if(const ShaderBind *sb = wd->getShaderBind())
		glUniform1i(sb->textureEnableLoc, 0);
	glPushMatrix();
	gldTranslate3dv(pos);
	gldMultQuat(rot);
	glRotated(deg_per_rad * this->py[1], 0., 1., 0.);
	gldScaled(bscale);
	glScalef(-1,1,-1);

#if 0
	// This method has an advantage in extensibility, but has more cost to calculate.
	MotionPose mp[2];
	motions[0]->interpolate(mp[0], (1. - py[0] / (M_PI / 2.)) * 10.);
	motions[1]->interpolate(mp[1], (blowback / 0.010) * 10.);
	mp[0].next = &mp[1];

	DrawMQOPose(model, mp);
	glPopMatrix();
#else
	// This method, which is closer to former method, has a limitation in extensibility
	// (you probably cannot just replace the model but also recompile to change the appearance of the model),
	// but has little overhead.
	if(0 <= turretIndex)
		DrawSUF(model->sufs[turretIndex], SUF_ATR, NULL);
	if(0 <= barrelIndex){
		// Bone joint must be defined.
		const Vec3d &pos = model->bones[barrelIndex]->joint;
		gldTranslate3dv(pos);
		glRotated(deg_per_rad * this->py[0], -1., 0., 0.);
		gldTranslate3dv(-pos);
		glTranslated(0, 0, -blowback / bscale);
		DrawSUF(model->sufs[barrelIndex], SUF_ATR, NULL);
	}
#endif
	glPopMatrix();
	if(const ShaderBind *sb = wd->getShaderBind())
		glUniform1i(sb->textureEnableLoc, 1);
}

void LTurret::drawtra(wardraw_t *wd){
	Entity *pb = base;
	double bscale = 1;
	if(this->mf) for(int i = 0; i < 2; i++){
		struct random_sequence rs;
		Mat4d mat2, mat, rot;
		Vec3d pos, const barrelpos(.005 * (i * 2 - 1), .005, -.0025), const muzzlepos(0, .0, -.030 + blowback);
		init_rseq(&rs, (long)this ^ *(long*)&wd->vw->viewtime);
		this->transform(mat);
		mat2 = mat.roty(this->py[1]);
		mat2.translatein(barrelpos * bscale);
		mat = mat2.rotx(this->py[0]);
//		mat.translatein(-barrelpos * bscale);
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
		gldTextureBeam(wd->vw->pos, pos, pos + rot.vp3(-vec3_001) * .200 * fi, .060 * fi);
		glPopAttrib();
	}
}

/// \brief Effect of shooting a bullet from the larget turret.
void LTurret::shootEffect(const Vec3d &bulletpos, const Vec3d &direction){
		if(WarSpace *ws = *w) for(int j = 0; j < 3; j++){
			Vec3d pos;
//			COLOR32 col = 0;
			pos[0] = .02 * (drseq(&w->rs) - .5);
			pos[1] = .02 * (drseq(&w->rs) - .5);
			pos[2] = .02 * (drseq(&w->rs) - .5);
/*			col |= COLOR32RGBA(rseq(&w->rs) % 32 + 127,0,0,0);
			col |= COLOR32RGBA(0,rseq(&w->rs) % 32 + 95,0,0);
			col |= COLOR32RGBA(0,0,rseq(&w->rs) % 32 + 80,0);
			col |= COLOR32RGBA(0,0,0,191);*/
			Vec3d smokevelo = direction * .02 * (drseq(&w->rs) + .5) + this->velo;
			smokevelo[0] += .02 * (drseq(&w->rs) - .5);
			smokevelo[1] += .02 * (drseq(&w->rs) - .5);
			smokevelo[2] += .02 * (drseq(&w->rs) - .5);
			static smokedraw_swirl_data sdata = {COLOR32RGBA(127,95,80,191), true};
			AddTelineCallback3D(ws->tell, pos + bulletpos, smokevelo, .015, quat_u, Vec3d(0, 0, 2. * M_PI * (drseq(&w->rs) - .5)),
				vec3_000, smokedraw_swirl, (void*)&sdata, TEL3_INVROTATE | TEL3_NOLINE, 1.5);
		}
		if(WarSpace *ws = *w) for(int j = 0; j < 15; j++){
			double triangle = drseq(&w->rs);
			triangle += drseq(&w->rs); // Triangular distributed random value
			Vec3d smokevelo = direction * .1 * triangle + this->velo;
			smokevelo[0] += .05 * (drseq(&w->rs) - .5);
			smokevelo[1] += .05 * (drseq(&w->rs) - .5);
			smokevelo[2] += .05 * (drseq(&w->rs) - .5);
			AddTelineCallback3D(ws->tell, bulletpos, smokevelo, .0004, quat_u, vec3_000,
				vec3_000, sparkdraw, NULL, TEL3_NOLINE, 0.5 + .25 * (drseq(&w->rs) - .5));
		}
}
