/** \file
 * \brief Implementation of Apache's graphics.
 */
#define NOMINMAX
#include "../Apache.h"
#include "../Launcher.h"
#include "draw/WarDraw.h"
#include "draw/mqoadapt.h"
#include "draw/OpenGLState.h"
#include "draw/effects.h"
extern "C"{
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/gl/gldraw.h>
}


void Apache::draw(WarDraw *wd){
	/* cull object */
	if(wd->vw->gc->cullFrustum(this->pos, .03))
		return;
	double pixels = .015 * fabs(wd->vw->gc->scale(this->pos));
	if(pixels < 2)
		return;
	wd->lightdraws++;

	static Motion *rotorxMotion = NULL;
	static Motion *rotorzMotion = NULL;
	static Motion *rotorMotion = NULL;
	static Motion *tailRotorMotion = NULL;

	static OpenGLState::weak_ptr<bool> init;
	if(!init) do{
		FILE *fp;
		model = LoadMQOModel(modPath() << "models/apache.mqo");
		rotorxMotion = LoadMotion(modPath() << "models/apache-rotorx.mot");
		rotorzMotion = LoadMotion(modPath() << "models/apache-rotorz.mot");
		rotorMotion = LoadMotion(modPath() << "models/apache-rotor.mot");
		tailRotorMotion = LoadMotion(modPath() << "models/apache-tailrotor.mot");
		init.create(*openGLState);
	} while(0);

	if(!model)
		return;

	MotionPose mp[6];
	rotorxMotion->interpolate(mp[0], rotoraxis[0] / (M_PI / 2.) * 10. + 10.);
	rotorzMotion->interpolate(mp[1], rotoraxis[1] / (M_PI / 2.) * 10. + 10.);
	mp[0].next = &mp[1];
	rotorMotion->interpolate(mp[2], fmod(rotor, M_PI / 2.) / (M_PI / 2.) * 10.);
	mp[1].next = &mp[2];
	tailRotorMotion->interpolate(mp[3], fmod(tailrotor, M_PI) / (M_PI) * 20.);
	mp[2].next = &mp[3];
	gunMotion(&mp[4]);
	mp[3].next = &mp[4];

	glPushMatrix();

	Mat4d mat;
	transform(mat);
	glMultMatrixd(mat);

	glScaled(-modelScale, modelScale, -modelScale);

	DrawMQOPose(model, mp);

	glPopMatrix();

}

void Apache::drawtra(WarDraw *wd){
	Mat4d mat;
	transform(mat);
	if(muzzle){
		muzzle = 0;
		if(!model)
			return;
		static const Vec3d pvo(.0, -.00150, -.00320);
		Mat4d rmat = this->rot.tomat4().rotx(gun[0]).roty(gun[1]);
		MotionPose mp[2];
		gunMotion(mp);
		Vec3d pos;
		Quatd rot;
		if(model->getBonePos("apache_muzzle", mp[0], &pos, &rot)){
			mat.scalein(-modelScale, modelScale, -modelScale);
			drawmuzzleflash4(mat.vp3(pos), (this->rot.rotate(M_PI, 0, 1, 0) * rot).rotate(M_PI, 0, 1, 0).tomat4(),
				0.0025, mat4_u, &w->rs, wd->vw->pos);
		}
	}
}

void Apache::drawHUD(WarDraw *wd){
	st::drawHUD(wd);

	glPushMatrix();
	glLoadIdentity();

	// Show target marker if locked on
	if(isCockpitView(game->player->getChaseCamera()) && this->enemy){
		glPushMatrix();
		glLoadMatrixd(wd->vw->rot);
		Vec3d pos = (this->enemy->pos - this->pos).norm();
		gldTranslate3dv(pos);
		glMultMatrixd(wd->vw->irot);
		{
			Vec3d dp = this->enemy->pos - this->pos;
			double dist = dp.len();
			glRasterPos3d(.01, .02, 0.);
			if(dist < 1.)
				gldprintf("%.4g m", dist * 1000.);
			else
				gldprintf("%.4g km", dist);
			Vec3d dv = this->enemy->velo - this->velo;
			dist = dv.sp(dp) / dist;
			glRasterPos3d(.01, -.03, 0.);
			if(dist < 1.)
				gldprintf("%.4g m", dist * 1000.);
			else
				gldprintf("%.4g km", dist);
		}
		glBegin(GL_LINE_LOOP);
		glVertex3d(.05, .05, 0.);
		glVertex3d(-.05, .05, 0.);
		glVertex3d(-.05, -.05, 0.);
		glVertex3d(.05, -.05, 0.);
		glEnd();
		glBegin(GL_LINES);
		glVertex3d(.04, .0, 0.);
		glVertex3d(.02, .0, 0.);
		glVertex3d(-.04, .0, 0.);
		glVertex3d(-.02, .0, 0.);
		glVertex3d(.0, .04, 0.);
		glVertex3d(.0, .02, 0.);
		glVertex3d(.0, -.04, 0.);
		glVertex3d(.0, -.02, 0.);
		glEnd();
		glPopMatrix();
	}

	{
		GLint vp[4];
		int w, h, m, mi, i;
		double left, bottom;
		int k, ind[7] = {0, 1}, nind = 2;

		glGetIntegerv(GL_VIEWPORT, vp);
		w = vp[2], h = vp[3];
		m = w < h ? h : w;
		mi = w < h ? w : h;
		left = -(double)w / m;
		bottom = -(double)h / m;

		double velo = this->velo.len();
//		w->orientation(wf, &ort, pt->pos);
		Mat3d ort = mat3d_u();
		glRasterPos3d(left + 200. / m, -bottom - 100. / m, -1);
		gldprintf("%lg km/s", velo);
		glRasterPos3d(left + 200. / m, -bottom - 120. / m, -1);
		gldprintf("%lg kt", 1944. * velo);
		glRasterPos3d(left + 200. / m, -bottom - 140. / m, -1);
		gldprintf("climb %lg km/s", this->velo.sp(ort.vec3(1)));
		if(brk){
			glRasterPos3d(left + 200. / m, -bottom - 200. / m, -1);
			gldprintf("BRK");
		}

		glScaled(1./*(double)mi / w*/, 1./*(double)mi / h*/, (double)m / mi);

		/* throttle */
		glBegin(GL_LINE_LOOP);
		glVertex3d(-.8, -0.7, -1.);
		glVertex3d(-.8, -0.8, -1.);
		glVertex3d(-.78, -0.8, -1.);
		glVertex3d(-.78, -0.7, -1.);
		glEnd();
		glBegin(GL_LINES);
		glVertex3d(-.775, -0.8 + throttle * .1, -1.);
		glVertex3d(-.805, -0.8 + throttle * .1, -1.);
		/* main feather */
		glVertex3d(-.76, -0.8 + feather * .1, -1.);
		glVertex3d(-.75, -0.8 + feather * .1, -1.);
		glEnd();
		glBegin(GL_QUADS);
		glVertex3d(-.8, -0.8 + throttle * feather * .1, -1.);
		glVertex3d(-.8, -0.8, -1.);
		glVertex3d(-.78, -0.8, -1.);
		glVertex3d(-.78, -0.8 + throttle * feather * .1, -1.);
		glEnd();

		glTranslated(-.5, -.7, -1.);
		glScaled(.1, .1, 1.);

	}

	glPopMatrix();
}

void Apache::drawCockpit(WarDraw *wd){
	const Vec3d &seat = cockpitOfs;
	Player *player = game->player;
	if(!isCockpitView(player->getChaseCamera()) || player->mover != player->cockpitview || player->mover != player->nextmover)
		return;

	static Model *cockpitModel = NULL;

	static OpenGLState::weak_ptr<bool> init;
	if(!init){
		cockpitModel = LoadMQOModel(modPath() << "models/apache_int.mqo");
		init.create(*openGLState);
	}

	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glClearDepth(1.);
	glClear(GL_DEPTH_BUFFER_BIT);

	glPushMatrix();

	glLoadMatrixd(wd->vw->rot);
	gldMultQuat(this->rot);
	gldTranslaten3dv(seat);

	glScaled(-modelScale, modelScale, -modelScale);
	DrawMQOPose(cockpitModel, NULL);

	glPopMatrix();

	glPopAttrib();

}

void Apache::drawOverlay(WarDraw *){
	glCallList(overlayDisp);
}

void Apache::gunMotion(MotionPose *mp){
	static Motion *gunYawMotion = NULL;
	static Motion *gunPitchMotion = NULL;

	static OpenGLState::weak_ptr<bool> init;
	if(!init) do{
		gunYawMotion = LoadMotion(modPath() << "models/apache-gunyaw.mot");
		gunPitchMotion = LoadMotion(modPath() << "models/apache-gunpitch.mot");
		init.create(*openGLState);
	} while(0);

	gunYawMotion->interpolate(mp[0], gun[1] / (M_PI) * 20. + 10.);
	gunPitchMotion->interpolate(mp[1], -gun[0] / (M_PI) * 20. + 10.);
	mp[0].next = &mp[1];
}