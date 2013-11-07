/** \file
 * \brief Implementation of F15 class graphics.
 */

#include "../F15.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/mqoadapt.h"
#include "glw/glwindow.h"
#include "glw/GLWchart.h"
#include "cmd.h"
#include "sqadapt.h"

extern "C"{
#include "clib/gl/gldraw.h"
#include <clib/timemeas.h>
}

#include "cpplib/CRC32.h"


Model *F15::model;


void F15::draw(WarDraw *wd){
	static Motion *lodMotion = nullptr;
	static Motion *aileronMotion = nullptr;
	static Motion *elevatorMotion = nullptr;
	static Motion *rudderMotion = nullptr;
	static Motion *gearMotion = nullptr;
	
	timemeas_t tm;
	TimeMeasStart(&tm);

	static OpenGLState::weak_ptr<bool> init;
	if(!init){
		model = LoadMQOModel(modPath() << "models/F15.mqo");
		lodMotion = LoadMotion(modPath() << "models/F15-LOD.mot");
		aileronMotion = LoadMotion(modPath() << "models/F15-aileron.mot");
		elevatorMotion = LoadMotion(modPath() << "models/F15-elevator.mot");
		rudderMotion = LoadMotion(modPath() << "models/F15-rudder.mot");
		gearMotion = LoadMotion(modPath() << "models/F15-gear.mot");

		init.create(*openGLState);
	}

	if(model){
		const double scale = modelScale;

		timemeas_t tm2;
		TimeMeasStart(&tm2);

		// TODO: Shadow map shape and real shape can diverge
		const double pixels = getHitRadius() * fabs(wd->vw->gc->scale(pos));
		MotionPose mp[5];
		lodMotion->interpolate(mp[0], pixels < 15 ? 0. : 10.);
		aileronMotion->interpolate(mp[1], aileron * 10. + 10.);
		elevatorMotion->interpolate(mp[2], elevator * 10. + 10.);
		rudderMotion->interpolate(mp[3], rudder * 10. + 10.);
		gearMotion->interpolate(mp[4], gearphase * 10.);
		mp[0].next = &mp[1];
		mp[1].next = &mp[2];
		mp[2].next = &mp[3];
		mp[3].next = &mp[4];

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
		GLWchart::addSampleToCharts("f15time", TimeMeasLap(&tm));
	else
		GLWchart::addSampleToCharts("f15smtime", TimeMeasLap(&tm));
}

void F15::drawtra(WarDraw *wd){
	Mat4d mat;
	transform(mat);

	// Show navigation lights if turned on
	if(navlight){
		drawNavlights(wd, navlights, &mat);
	}

	if(debugWings)
		drawDebugWings();

	if(afterburner){
		RandomSequence rs(crc32(&game->universe->global_time, sizeof game->universe->global_time));
		for(auto it : engineNozzles){
			double (*cuts)[2] = CircleCuts(8);
			glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
			glEnable(GL_CULL_FACE);
			glPushMatrix();
			glMultMatrixd(mat);
			gldTranslate3dv(it);
			glScaled(.0005, .0005, .005);
			glBegin(GL_TRIANGLE_FAN);
			glColor4ub(255,127,0, rs.next() % 64 + 128);
			double x = (rs.nextd() - .5) * .25;
			glVertex3d(x, (rs.nextd() - .5) * .25, 1);
			for(int j = 0; j <= 8; j++){
				int j1 = j % 8;
				glColor4ub(0,127,255, rs.next() % 128 + 128);
				glVertex3d(cuts[j1][1], cuts[j1][0], 0);
			}
			glEnd();
			glPopMatrix();
			glPopAttrib();
		}
	}

}

void F15::drawHUD(WarDraw *wd){

	timemeas_t tm;
	TimeMeasStart(&tm);
	st::drawHUD(wd);

	glLoadIdentity();

	glDisable(GL_LINE_SMOOTH);

#if 1
	// Show target marker if locked on
	drawTargetMarker(wd);

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

		if(afterburner)
			glColor4f(1,0,0,1);

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

		if(afterburner)
			glColor4f(1,1,1,1);

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
/*	printf("fly_drawHUD %lg\n", TimeMeasLap(&tm));*/
}

void F15::drawCockpit(WarDraw *wd){
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
		modelCockpit = LoadMQOModel(modPath() << "models/F15-cockpit.mqo");
	}


	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glClearDepth(1.);
	glClear(GL_DEPTH_BUFFER_BIT);

	glPushMatrix();

	glLoadMatrixd(wd->vw->rot);
	gldMultQuat(this->rot);

	glPushMatrix();

	gldTranslaten3dv(seat);
	glScaled(-modelScale, modelScale, -modelScale);
	DrawMQOPose(modelCockpit, nullptr);

	glPopMatrix();

	drawCockpitHUD(hudPos, hudSize, seat, gunDirection);

	glPushMatrix();

	glPopMatrix();

	glPopMatrix();

	glPopAttrib();
}

void F15::drawOverlay(WarDraw *){
	glCallList(overlayDisp);
}

