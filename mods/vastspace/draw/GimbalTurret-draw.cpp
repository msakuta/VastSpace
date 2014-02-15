/** \file
 * \brief Graphics implementation of GimbalTurret class
 */
#include "../GimbalTurret.h"
#include "Viewer.h"
#include "war.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/mqoadapt.h"
#include "draw/effects.h"
extern "C"{
#include <clib/GL/gldraw.h>
}







/// \brief Loads and initializes model.
/// \returns The Model object to be drawn if initialized or already initialized, NULL if something fail to initialize.
///
/// This process is completely for display, so defined in this GimbalTurret-draw.cpp.
Model *GimbalTurret::initModel(){
	static OpenGLState::weak_ptr<bool> init;

	if(!init){
		model = LoadMQOModel(modPath() << "models/GimbalTurret.mqo");
		init.create(*openGLState);
	}

	return model;
}

/// \brief Loads and initializes wheel rotation motions.
bool GimbalTurret::initMotions(){
	static OpenGLState::weak_ptr<bool> init;

	if(!init){
		motions[0] = LoadMotion(modPath() << "models/GimbalTurret_roty.mot");
		motions[1] = LoadMotion(modPath() << "models/GimbalTurret_rotx.mot");
		init.create(*openGLState);
	}

	return !!motions[0] && !!motions[1];
}

/// \brief Reimplements GimbalTurret::initModel() to load MissileGimbalTurret's model.
///
/// Note that motions are shared among the classes.
Model *MissileGimbalTurret::initModel(){
	static OpenGLState::weak_ptr<bool> init;

	if(!init){
		model = LoadMQOModel(modPath() << "models/MissileGimbalTurret.mqo");
		init.create(*openGLState);
	}

	return model;
}

void GimbalTurret::draw(WarDraw *wd){
	if(!w)
		return;

	/* cull object */
//	if(cull(wd))
//		return;

	draw_healthbar(this, wd, health / getMaxHealth(), getHitRadius(), 0, 0);

	Model *model = initModel();
	if(!model || !initMotions())
		return;

	MotionPose mp[2];
	motions[0]->interpolate(mp[0], yaw * 50. / 2. / M_PI);
	motions[1]->interpolate(mp[1], pitch * 50. / 2. / M_PI);
	mp[0].next = &mp[1];

	const double scale = modelScale;

	glPushMatrix();

	Mat4d mat;
	transform(mat);
	glMultMatrixd(mat);
	glScaled(-scale, scale, -scale);
//	glTranslated(0, 54.4, 0);
#if DNMMOT_PROFILE
	{
		timemeas_t tm;
		TimeMeasStart(&tm);
#endif
		DrawMQOPose(model, mp);
#if DNMMOT_PROFILE
		printf("motdraw %lg\n", TimeMeasLap(&tm));
	}
#endif
	glPopMatrix();

}

void GimbalTurret::drawtra(wardraw_t *wd){
	st::drawtra(wd);

	if(muzzleFlash) for(int i = 0; i < 2; i++){
		Vec3d pos = rot.trans(Vec3d(gunPos[i])) + this->pos;
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		glCallList(muzzle_texture());
/*		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glRotatef(-90, 0, 0, 1);
		glMatrixMode(GL_MODELVIEW);*/
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE); // Add blend
		double f = muzzleFlash / .1 * 2.;
		double fi = 1. - muzzleFlash / .1;
		glColor4f(f,f,f,1);
		gldTextureBeam(wd->vw->pos, pos, pos + rot.trans(-vec3_001) * .03 * fi, .01 * fi);
/*		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);*/
		glPopAttrib();
	}
}

void GimbalTurret::drawOverlay(wardraw_t *){
	glCallList(overlayDisp);
}

void GimbalTurret::deathEffect(){
	if(deathEffectDone)
		return;

	WarSpace *ws = *w;
	if(!ws)
		return;

	// smokes
	for(int i = 0; i < 16; i++){
		Vec3d pos;
		COLOR32 col = 0;
		pos[0] = .02 * (drseq(&w->rs) - .5);
		pos[1] = .02 * (drseq(&w->rs) - .5);
		pos[2] = .02 * (drseq(&w->rs) - .5);
		col |= COLOR32RGBA(rseq(&w->rs) % 32 + 127,0,0,0);
		col |= COLOR32RGBA(0,rseq(&w->rs) % 32 + 127,0,0);
		col |= COLOR32RGBA(0,0,rseq(&w->rs) % 32 + 127,0);
		col |= COLOR32RGBA(0,0,0,191);
		AddTelineCallback3D(ws->getTeline3d(), pos + this->pos, pos / 1. + velo / 2., .02, quat_u, vec3_000, vec3_000, ::smokedraw, (void*)col, TEL3_INVROTATE | TEL3_NOLINE, 5.);
	}

	// explode shockwave
	AddTeline3D(ws->getTeline3d(), this->pos, vec3_000, .3, quat_u, vec3_000, vec3_000, COLOR32RGBA(255,255,255,127), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_INVROTATE, .5);

	// Prevent multiple effects for a single object.
	deathEffectDone = true;
}
