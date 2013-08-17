/** \file
 * \brief Implementation of Launcher's graphics.
 */
#define NOMINMAX
#include "../Launcher.h"
#include "../Apache.h"
#ifdef _WIN32 // Here is silly dependence of GL.h to Windows.h
//#include <Windows.h>
#endif
#include "draw/WarDraw.h"
#include "draw/mqoadapt.h"
#include "draw/OpenGLState.h"
#include "draw/effects.h"
#include "Viewer.h"
extern "C"{
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/gl/gldraw.h>
}





//-----------------------------------------------------------------------------
//	HydraRocket implementation
//-----------------------------------------------------------------------------

const double HydraRocket::modelScale = 1e-6;

void HydraRocket::draw(WarDraw *wd){
	/* cull object */
	if(wd->vw->gc->cullFrustum(this->pos, .003))
		return;
	double pixels = .015 * fabs(wd->vw->gc->scale(this->pos));
	if(pixels < 2)
		return;
	wd->lightdraws++;

	static Model *model = NULL;

	static OpenGLState::weak_ptr<bool> init;
	if(!init) do{
		model = LoadMQOModel(Launcher::modPath() << "models/hydra.mqo");
		init.create(*openGLState);
	} while(0);

	if(!model)
		return;

	glPushMatrix();

	Mat4d mat;
	transform(mat);
	glMultMatrixd(mat);

	glScaled(-modelScale, modelScale, -modelScale);

	DrawMQOPose(model, NULL);

	glPopMatrix();

}

void HydraRocket::drawtra(WarDraw *wd){
	double length = (this->velo.slen() * 5 * 5 + 20) * 0.0015;
	double width = 0.001;

	double f = 2. * this->velo.len() * length;
	double span = std::min(f, .1);
	length *= span / f;

	if(wd->vw->gc->cullFrustum(this->pos, span))
		return;
	double scale = fabs(wd->vw->gc->scale(this->pos));
	double pixels = span * scale;
	if(pixels < 2)
		return;

	double wpix = width * scale / 2.;

	if(0.1 < wpix){
		glColor4ub(191,127,95,255);
		gldGradBeam(wd->vw->pos, this->pos, this->pos + this->velo * -length, width, COLOR32RGBA(127,0,0,0));
	}
}

void HydraRocket::bulletDeathEffect(int hitground, const struct contact_info *ci){
	explosionEffect(ci);
}

//-----------------------------------------------------------------------------
//	HydraRocketLauncher implementation
//-----------------------------------------------------------------------------

void HydraRocketLauncher::draw(WarDraw *wd){
	/* cull object */
	if(wd->vw->gc->cullFrustum(this->pos, .003))
		return;
	double pixels = .015 * fabs(wd->vw->gc->scale(this->pos));
	if(pixels < 2)
		return;
	wd->lightdraws++;

	static Model *model = NULL;

	static OpenGLState::weak_ptr<bool> init;
	if(!init) do{
		model = LoadMQOModel(Launcher::modPath() << "models/hydralauncher.mqo");
		init.create(*openGLState);
	} while(0);

	if(!model)
		return;

	static const double modelScale = Apache::modelScale;

	glPushMatrix();

	Mat4d mat;
	transform(mat);
	glMultMatrixd(mat);

	glScaled(-modelScale, modelScale, -modelScale);

	DrawMQOPose(model, NULL);

	glPopMatrix();

}


//-----------------------------------------------------------------------------
//	Hellfire implementation
//-----------------------------------------------------------------------------

const double Hellfire::modelScale = 1e-5;

void Hellfire::draw(WarDraw *wd){
	/* cull object */
	if(wd->vw->gc->cullFrustum(this->pos, .003))
		return;
	double pixels = .015 * fabs(wd->vw->gc->scale(this->pos));
	if(pixels < 2)
		return;
	wd->lightdraws++;

	static Model *model = NULL;

	static OpenGLState::weak_ptr<bool> init;
	if(!init) do{
		model = LoadMQOModel(Launcher::modPath() << "models/hellfire.mqo");
		init.create(*openGLState);
	} while(0);

	if(!model)
		return;

	glPushMatrix();

	Mat4d mat;
	transform(mat);
	glMultMatrixd(mat);

	glScaled(-modelScale, modelScale, -modelScale);

	DrawMQOPose(model, NULL);

	glPopMatrix();

}

void Hellfire::drawtra(WarDraw *wd){
	double length = (this->velo.slen() * 5 * 5 + 20) * 0.0015;
	double width = 0.001;

	double f = 2. * this->velo.len() * length;
	double span = std::min(f, .1);
	length *= span / f;

	if(wd->vw->gc->cullFrustum(this->pos, span))
		return;
	double scale = fabs(wd->vw->gc->scale(this->pos));
	double pixels = span * scale;
	if(pixels < 2)
		return;

	double wpix = width * scale / 2.;

	if(0.1 < wpix){
		glColor4ub(191,127,95,255);
		gldGradBeam(wd->vw->pos, this->pos, this->pos + this->velo * -length, width, COLOR32RGBA(127,0,0,0));
	}
}

void Hellfire::bulletDeathEffect(int hitground, const struct contact_info *ci){
	explosionEffect(ci);
}

//-----------------------------------------------------------------------------
//	HellfireLauncher implementation
//-----------------------------------------------------------------------------

void HellfireLauncher::draw(WarDraw *wd){
	/* cull object */
	if(wd->vw->gc->cullFrustum(this->pos, .003))
		return;
	double pixels = .015 * fabs(wd->vw->gc->scale(this->pos));
	if(pixels < 2)
		return;
	wd->lightdraws++;

	static Model *model = NULL;

	static OpenGLState::weak_ptr<bool> init;
	if(!init) do{
		model = LoadMQOModel(Launcher::modPath() << "models/hellfire.mqo");
		init.create(*openGLState);
	} while(0);

	if(!model)
		return;

	static const double modelScale = Hellfire::modelScale;

	glPushMatrix();

	Mat4d mat;
	transform(mat);
	glMultMatrixd(mat);

	glScaled(-modelScale, modelScale, -modelScale);

	int count = 0;
	for(int ix = -1; ix <= 1; ix += 2) for(int iy = -1; iy <= 1; iy += 2){
		if(ammo <= count)
			break;
		glPushMatrix();
		glTranslated(ix * 20, iy * 20, 0);
		DrawMQOPose(model, NULL);
		glPopMatrix();
		count++;
	}

	glPopMatrix();

}
