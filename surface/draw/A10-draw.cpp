/** \file
 * \brief Implementation of A-10 class graphics.
 */

#include "../A10.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/mqoadapt.h"
#include "glw/glwindow.h"
#include "glw/GLWchart.h"
#include "cmd.h"
#include "sqadapt.h"
#include "draw/effects.h"

extern "C"{
#include "clib/gl/gldraw.h"
#include <clib/timemeas.h>
}


Model *A10::model;

void A10::draw(WarDraw *wd){
	static Motion *lodMotion = nullptr;
	static Motion *aileronMotion = nullptr;
	static Motion *elevatorMotion = nullptr;
	static Motion *rudderMotion = nullptr;
	static Motion *gearMotion = nullptr;
	static Motion *airbrakeMotion = nullptr;

	timemeas_t tm;
	TimeMeasStart(&tm);

	static OpenGLState::weak_ptr<bool> init;
	if(!init){
		model = LoadMQOModel(modPath() << "models/A10.mqo");
		lodMotion = LoadMotion(modPath() << "models/A10-LOD.mot");
		aileronMotion = LoadMotion(modPath() << "models/F15-aileron.mot");
		elevatorMotion = LoadMotion(modPath() << "models/F15-elevator.mot");
		rudderMotion = LoadMotion(modPath() << "models/F15-rudder.mot");
		gearMotion = LoadMotion(modPath() << "models/F15-gear.mot");
		airbrakeMotion = LoadMotion(modPath() << "models/A10-airbrake.mot");

		init.create(*openGLState);
	}

	if(model){
		const double scale = modelScale;

		timemeas_t tm2;
		TimeMeasStart(&tm2);

		// TODO: Shadow map shape and real shape can diverge
		const double pixels = getHitRadius() * fabs(wd->vw->gc->scale(pos));
		MotionPose mp[6];
		lodMotion->interpolate(mp[0], pixels < 15 ? 0. : 10.);
		aileronMotion->interpolate(mp[1], aileron * 10. + 10.);
		elevatorMotion->interpolate(mp[2], elevator * 10. + 10.);
		rudderMotion->interpolate(mp[3], getRudder() * 10. + 10.);
		gearMotion->interpolate(mp[4], gearphase * 10.);
		airbrakeMotion->interpolate(mp[5], brakephase * 5.); // 10 = 90 degrees
		mp[0].next = &mp[1];
		mp[1].next = &mp[2];
		mp[2].next = &mp[3];
		mp[3].next = &mp[4];
		mp[4].next = &mp[5];

		if(!wd->shadowmapping)
			GLWchart::addSampleToCharts("posetime", TimeMeasLap(&tm2));

		glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT);
		glPushMatrix();
		{
			Mat4d mat;
			transform(mat);
			glMultMatrixd(mat);
		}

		glScaled(-scale, scale, -scale);
		DrawMQOPose(model, mp);
		glPopAttrib();

		glPopMatrix();
	}
	if(wd->shadowmapping)
		GLWchart::addSampleToCharts("A10time", TimeMeasLap(&tm));
	else
		GLWchart::addSampleToCharts("A10smtime", TimeMeasLap(&tm));
}

void A10::drawtra(WarDraw *wd){
	// Show navigation lights if turned on
	if(navlight){
		Mat4d mat;
		transform(mat);
		drawNavlights(wd, navlights, &mat);
	}

	if(debugWings)
		drawDebugWings();

}

void A10::drawHUD(WarDraw *wd){

	timemeas_t tm;
	TimeMeasStart(&tm);
	st::drawHUD(wd);

	glLoadIdentity();

	glDisable(GL_LINE_SMOOTH);

#if 1
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
				gldprintf("%.4g m/s", dist * 1000.);
			else
				gldprintf("%.4g km/s", dist);
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
		int w, h, m, mi;
		double left, bottom;
		glGetIntegerv(GL_VIEWPORT, vp);
		w = vp[2], h = vp[3];
		m = w < h ? h : w;
		mi = w < h ? w : h;
		left = -(double)w / m;
		bottom = -(double)h / m;

		glRasterPos3d(left, bottom + 130. / m, -1.);
		gldprintf("Missiles: %d", this->missiles);

		glPushMatrix();
		glScaled(1./*(double)mi / w*/, 1./*(double)mi / h*/, (double)m / mi);

		/* throttle */
		glBegin(GL_LINE_LOOP);
		glVertex3d(-.4, -0.7, -1.);
		glVertex3d(-.4, -0.8, -1.);
		glVertex3d(-.38, -0.8, -1.);
		glVertex3d(-.38, -0.7, -1.);
		glEnd();
		glBegin(GL_QUADS);
		glVertex3d(-.4, -0.8 + throttle * .1, -1.);
		glVertex3d(-.4, -0.8, -1.);
		glVertex3d(-.38, -0.8, -1.);
		glVertex3d(-.38, -0.8 + throttle * .1, -1.);
		glEnd();

		/* boundary of control surfaces */
		glBegin(GL_LINE_LOOP);
		glVertex3d(-.45, -.5, -1.);
		glVertex3d(-.45, -.7, -1.);
		glVertex3d(-.60, -.7, -1.);
		glVertex3d(-.60, -.5, -1.);
		glEnd();

		glBegin(GL_LINES);

		/* aileron */
		glVertex3d(-.45, aileron / M_PI * .2 - .6, -1.);
		glVertex3d(-.5, aileron / M_PI * .2 - .6, -1.);
		glVertex3d(-.55, -aileron / M_PI * .2 - .6, -1.);
		glVertex3d(-.6, -aileron / M_PI * .2 - .6, -1.);

		/* rudder */
		glVertex3d(rudder * 3. / M_PI * .1 - .525, -.7, -1.);
		glVertex3d(rudder * 3. / M_PI * .1 - .525, -.8, -1.);

		/* elevator */
		glVertex3d(-.5, -elevator / M_PI * .2 - .6, -1.);
		glVertex3d(-.55, -elevator / M_PI * .2 - .6, -1.);

		glEnd();

	}
	glPopMatrix();
#endif

	GLWchart::addSampleToCharts("tratime", TimeMeasLap(&tm));
}

void A10::drawCockpit(WarDraw *wd){
	static bool init = false;
	const Vec3d &seat = cameraPositions[0].pos;
	Vec3d stick = Vec3d(0., 68., -270.) * modelScale / 2.;
	static Model *modelCockpit = NULL;
	Player *player = game->player;
	int chasecam = player->getChaseCamera();

	// Chasecam == cameraPositions.size() means it's trying to follow launched missiles.
	if(!isCockpitView(chasecam) || player->mover != player->cockpitview || player->mover != player->nextmover)
		return;

	if(!init){
		init = true;
		modelCockpit = LoadMQOModel(modPath() << "models/A10-cockpit.mqo");
	}

	if(!modelCockpit)
		return;

	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glClearDepth(1.);
	glClear(GL_DEPTH_BUFFER_BIT);

	glPushMatrix();

	glLoadMatrixd(wd->vw->rot);
	gldMultQuat(this->rot);

	glPushMatrix();

	glMatrixMode (GL_PROJECTION);
	glMatrixMode (GL_MODELVIEW);

	gldTranslaten3dv(seat);
	glScaled(-modelScale, modelScale, -modelScale);
	DrawMQOPose(modelCockpit, nullptr);

	glPopMatrix();

	drawCockpitHUD(hudPos, hudSize, seat, gunDirection, weaponList);

	glPushMatrix();

	glPopMatrix();
#if 0
#endif

	glPopMatrix();

	glPopAttrib();
}

void A10::drawOverlay(WarDraw *){
	glCallList(overlayDisp);
}

struct SmokeTeline3 : Teline3{
	SmokeTeline3(const Teline3ConstructInfo &ci) : Teline3(ci){}
	bool update(double dt)override{
		if(!Teline3::update(dt))
			return false;
		velo *= exp(-dt);
		return true;
	}
	void draw(const Teline3DrawData &dd)const override{
		smokedraw(this, &dd, NULL);
	}
};

/// I really want to make this logic scipted, but the temporary entity library is not yet
/// exported API to Squirrel scripting.
SQInteger A10::sqf_gunFireEffect(HSQUIRRELVM v){
	Entity *e = sq_refobj(v, 1);
	if(!e || gunPositions.size() == 0)
		return 0;
	WarField *w = e->w;
	WarSpace *ws = *w;
	if(Teline3List *tell = ws->tell){
		if(w->rs.nextd() < 0.3){
			Teline3ConstructInfo ci = {
				e->pos + e->rot.trans(gunPositions.front()) + Vec3d(w->rs.nextGauss(), w->rs.nextGauss(), w->rs.nextGauss()) * 1.5e-3, // pos
				e->velo, // velo
				3e-3, // len
				quat_u, // rot
				vec3_000, // omg
				1.0 + w->rs.nextd(),
				w->accel(e->pos, e->velo), // gravity
				0, // flags
			};
			new(*tell) SmokeTeline3(ci);
		}
	}
	return 0;
}
