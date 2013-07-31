/** \file
 * \brief Implementation of Apache's graphics.
 */
#include "../Apache.h"
#include "draw/WarDraw.h"
#include "draw/mqoadapt.h"
#include "draw/OpenGLState.h"
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

	static Model *model = NULL;
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

	MotionPose mp[4];
	rotorxMotion->interpolate(mp[0], rotoraxis[0] / (M_PI / 2.) * 10. + 10.);
	rotorzMotion->interpolate(mp[1], rotoraxis[1] / (M_PI / 2.) * 10. + 10.);
	mp[0].next = &mp[1];
	rotorMotion->interpolate(mp[2], fmod(rotor, M_PI / 2.) / (M_PI / 2.) * 10.);
	mp[1].next = &mp[2];
	tailRotorMotion->interpolate(mp[3], fmod(tailrotor, M_PI) / (M_PI) * 20.);
	mp[2].next = &mp[3];

	glPushMatrix();

	Mat4d mat;
	transform(mat);
	glMultMatrixd(mat);

	glScaled(-modelScale, modelScale, -modelScale);

	DrawMQOPose(model, mp);

	glPopMatrix();

}

void Apache::drawHUD(WarDraw *wd){
	st::drawHUD(wd);

	glPushMatrix();
	glLoadIdentity();

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
	if(player->getChaseCamera() != 0 || player->mover != player->cockpitview || player->mover != player->nextmover)
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
