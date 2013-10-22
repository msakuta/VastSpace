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

	if(debugWings)
		drawDebugWings();

#if 0
	const GLubyte red[4] = {255,31,31,255}, white[4] = {255,255,255,255};
	static const avec3_t
	flylights[3] = {
		{-160. * FLY_SCALE, 20. * FLY_SCALE, 10. * FLY_SCALE},
		{160. * FLY_SCALE, 20. * FLY_SCALE, 10. * FLY_SCALE},
		{0. * FLY_SCALE, 20. * FLY_SCALE, -130 * FLY_SCALE},
	};
	avec3_t valkielights[3] = {
		{-440. * .5, 50. * .5, 210. * .5},
		{440. * .5, 50. * .5, 210. * .5},
		{0., -12. * .5 * FLY_SCALE, -255. * .5 * FLY_SCALE},
	};
	avec3_t *navlights = pt->vft == &fly_s ? flylights : valkielights;
	avec3_t *wingtips = pt->vft == &fly_s ? flywingtips : valkiewingtips;
	static const GLubyte colors[3][4] = {
		{255,0,0,255},
		{0,255,0,255},
		{255,255,255,255},
	};
	double air;
	double scale = FLY_SCALE;
	avec3_t v0, v;
	GLdouble mat[16];
	fly_t *p = (fly_t*)pt;
	struct random_sequence rs;
	int i;
	if(!pt->active)
		return;

	if(fly_cull(pt, wd, NULL))
		return;

	init_rseq(&rs, (long)(wd->gametime * 1e5));

	tankrot(&mat, pt);

	air = wd->w->vft->atmospheric_pressure(wd->w, pt->pos);

	if(air && SONICSPEED * SONICSPEED < VECSLEN(pt->velo)){
		const double (*cuts)[2];
		int i, k, lumi = rseq(&rs) % ((int)(127 * (1. - 1. / (VECSLEN(pt->velo) / SONICSPEED / SONICSPEED))) + 1);
		COLOR32 cc = COLOR32RGBA(255, 255, 255, lumi), oc = COLOR32RGBA(255, 255, 255, 0);

		cuts = CircleCuts(8);
		glPushMatrix();
		glMultMatrixd(mat);
		glTranslated(0., 20. * FLY_SCALE, -160 * FLY_SCALE);
		glScaled(.01, .01, .01 * (VECLEN(pt->velo) / SONICSPEED - 1.));
		glBegin(GL_TRIANGLE_FAN);
		gldColor32(COLOR32MUL(cc, wd->ambient));
		glVertex3i(0, 0, 0);
		gldColor32(COLOR32MUL(oc, wd->ambient));
		for(i = 0; i <= 8; i++){
			k = i % 8;
			glVertex3d(cuts[k][0], cuts[k][1], 1.);
		}
		glEnd();
		glPopMatrix();
	}

#if 0
	if(p->afterburner) for(i = 0; i < (pt->vft == &fly_s ? 1 : 2); i++){
		double (*cuts)[2];
		int j;
		struct random_sequence rs;
		double x;
		init_rseq(&rs, *(long*)&wd->gametime);
		cuts = CircleCuts(8);
		glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		glEnable(GL_CULL_FACE);
		glPushMatrix();
		glMultMatrixd(mat);
		if(pt->vft == &fly_s)
			glTranslated(pt->vft == &fly_s ? 0. : (i * 105. * 2. - 105.) * .5 * FLY_SCALE, pt->vft == &fly_s ? FLY_SCALE * 20. : FLY_SCALE * .5 * -5., FLY_SCALE * (pt->vft == &fly_s ? 160 : 420. * .5));
		else
			glTranslated((i * 1.2 * 2. - 1.2) * .001, -.0002, .008);
		glScaled(.0005, .0005, .005);
		glBegin(GL_TRIANGLE_FAN);
		glColor4ub(255,127,0,rseq(&rs) % 64 + 128);
		x = (drseq(&rs) - .5) * .25;
		glVertex3d(x, (drseq(&rs) - .5) * .25, 1);
		for(j = 0; j <= 8; j++){
			int j1 = j % 8;
			glColor4ub(0,127,255,rseq(&rs) % 128 + 128);
			glVertex3d(cuts[j1][1], cuts[j1][0], 0);
		}
		glEnd();
		glPopMatrix();
		glPopAttrib();
	}
#endif

	if(pt->vft == &valkie_s){
		valkie_t *p = (valkie_t*)pt;
		double rot[numof(valkie_bonenames)][7];
		struct afterburner_hint hint;
		ysdnmv_t v, *dnmv;
		dnmv = valkie_boneset(p, rot, &v, 1);
		hint.ab = p->afterburner;
		hint.muzzle = FLY_RELOADTIME - .03 < p->cooldown || p->muzzle;
		hint.thrust = p->throttle;
		hint.gametime = wd->gametime;
		if(1 || p->navlight){
			glPushMatrix();
			glLoadIdentity();
			glRotated(180, 0, 1., 0);
			glScaled(-.001, .001, .001);
/*			glTranslated(0, p->batphase * -7, 0.);
			glTranslated(0, 0, 5. * p->wing[0] / (M_PI / 4.));*/
			TransYSDNM_V(dnm, dnmv, find_wingtips, &hint);
/*			TransYSDNM(dnm, valkie_bonenames, rot, numof(valkie_bonenames), NULL, 0, find_wingtips, &hint);*/
			glPopMatrix();
			for(i = 0; i < 3; i++)
				VECCPY(valkielights[i], hint.wingtips[i]);
		}
		if(hint.muzzle || p->afterburner){
/*			v.fcla = 1 << 9;
			v.cla[9] = p->batphase;*/
			v.bonenames = valkie_bonenames;
			v.bonerot = rot;
			v.bones = numof(valkie_bonenames);
			v.skipnames = NULL;
			v.skips = 0;
			glPushMatrix();
			glMultMatrixd(mat);
			glRotated(180, 0, 1., 0);
			glScaled(-.001, .001, .001);
/*			glTranslated(0, p->batphase * -7, 0.);
			glTranslated(0, 0, 5. * p->wing[0] / (M_PI / 4.));*/
			TransYSDNM_V(dnm, dnmv, draw_afterburner, &hint);
			glPopMatrix();
			p->muzzle = 0;
		}
	}


/*	{
		GLubyte col[4] = {255,127,127,255};
		glBegin(GL_LINES);
		glColor4ub(255,255,255,255);
		glVertex3dv(pt->pos);
		glVertex3dv(p->sight);
		glEnd();
		gldSpriteGlow(p->sight, .001, col, wd->irot);
	}*/

/*	glPushAttrib(GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(0);*/
	if(p->navlight){
		gldBegin();
		VECCPY(v0, navlights[0]);
/*		VECSCALE(v0, navlights[0], FLY_SCALE);*/
		mat4vp3(v, mat, v0);
	/*	gldSpriteGlow(v, .001, red, wd->irot);*/
		drawnavlight(&v, &wd->view, .0005, colors[0], &wd->irot, wd);
		VECCPY(v0, navlights[1]);
/*		VECSCALE(v0, navlights[1], FLY_SCALE);*/
		mat4vp3(v, mat, v0);
		drawnavlight(&v, &wd->view, .0005, colors[1], &wd->irot, wd);
		if(fmod(wd->gametime, 1.) < .1){
			VECCPY(v0, navlights[2]);
/*			VECSCALE(v0, navlights[2], FLY_SCALE);*/
			mat4vp3(v, mat, v0);
			drawnavlight(&v, &wd->view, .0005, &white, &wd->irot, wd);
		}
		gldEnd();
	}

	if(air && .1 < -VECSP(pt->velo, &mat[8])){
		avec3_t vh;
		VECNORM(vh, pt->velo);
		for(i = 0; i < 2; i++){
			struct random_sequence rs;
			struct gldBeamsData bd;
			avec3_t pos;
			int mag = 1 + 255 * (1. - .1 * .1 / VECSLEN(pt->velo));
			mat4vp3(v, mat, navlights[i]);
			init_rseq(&rs, (unsigned long)((v[0] * v[1] * 1e6)));
			bd.cc = bd.solid = 0;
			VECSADD(v, vh, -.0005);
			gldBeams(&bd, wd->view, v, .0001, COLOR32RGBA(255,255,255,0));
			VECSADD(v, vh, -.0015);
			gldBeams(&bd, wd->view, v, .0003, COLOR32RGBA(255,255,255, rseq(&rs) % mag));
			VECSADD(v, vh, -.0015);
			gldBeams(&bd, wd->view, v, .0002, COLOR32RGBA(255,255,255,rseq(&rs) % mag));
			VECSADD(v, vh, -.0015);
			gldBeams(&bd, wd->view, v, .00002, COLOR32RGBA(255,255,255,rseq(&rs) % mag));
		}
	}

	if(pt->vft != &valkie_s){
		fly_t *fly = ((fly_t*)pt);
		int i = 0;
		if(fly->muzzle){
			amat4_t ir, mat2;
/*			pyrmat(pt->pyr, &mat);*/
			do{
				avec3_t pos, gunpos;
	/*			MAT4TRANSLATE(mat, fly_guns[i][0], fly_guns[i][1], fly_guns[i][2]);*/
	/*			pyrimat(pt->pyr, &ir);
				MAT4MP(mat2, mat, ir);*/
/*				MAT4VP3(gunpos, mat, fly_guns[i]);
				VECADD(pos, pt->pos, gunpos);*/
				MAT4VP3(pos, mat, fly_guns[i]);
				drawmuzzleflash(&pos, &wd->view, .0025, &wd->irot);
			} while(!i++);
			fly->muzzle = 0;
		}
	}
/*	glPopAttrib();*/

#if 0
	{
		double (*cuts)[2];
		double f = ((struct entity_private_static*)pt->vft)->hitradius;
		int i;

		cuts = CircleCuts(32);
		glPushMatrix();
		glTranslated(pt->pos[0], pt->pos[1], pt->pos[2]);
		glMultMatrixd(wd->irot);
		glColor3ub(255,255,127);
		glBegin(GL_LINE_LOOP);
		for(i = 0; i < 32; i++)
			glVertex3d(cuts[i][0] * f, cuts[i][1] * f, 0.);
		glEnd();
		glPopMatrix();
	}
#endif
#endif
}

void A10::drawHUD(WarDraw *wd){
	char buf[128];
/*	timemeas_t tm;*/
/*	glColor4ub(0,255,0,255);*/

/*	TimeMeasStart(&tm);*/

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

/*		air = wf->vft->atmospheric_pressure(wf, &pt->pos)/*exp(-pt->pos[1] / 10.)*/;
/*		glRasterPos3d(left, bottom + 50. / m, -1.);
		gldprintf("atmospheric pressure: %lg height: %lg", exp(-pt->pos[1] / 10.), pt->pos[1] * 1e3);*/
/*		glRasterPos3d(left, bottom + 70. / m, -1.);
		gldprintf("throttle: %lg", ((fly_t*)pt)->throttle);*/

/*		if(((fly_t*)pt)->brk){
			glRasterPos3d(left, bottom + 110. / m, -1.);
			gldprintf("BRK");
		}*/

/*		if(((fly_t*)pt)->afterburner){
			glRasterPos3d(left, bottom + 150. / m, -1.);
			gldprintf("AB");
		}*/

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

/*		printf("flyHUD %lg\n", TimeMeasLap(&tm));*/

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

/*		printf("flyHUD %lg\n", TimeMeasLap(&tm));*/

		if(game->player->controlled != this){
//			amat4_t rot, rot2;
//			avec3_t pyr;
/*			MAT4TRANSPOSE(rot, wf->irot);
			quat2imat(rot2, pt->rot);*/
//			VECSCALE(pyr, wf->pl->pyr, -1);
//			pyrimat(pyr, rot);
/*			glMultMatrixd(rot2);*/
//			glMultMatrixd(rot);
/*			glRotated(deg_per_rad * wf->pl->pyr[2], 0., 0., 1.);
			glRotated(deg_per_rad * wf->pl->pyr[0], 1., 0., 0.);
			glRotated(deg_per_rad * wf->pl->pyr[1], 0., 1., 0.);*/
		}
		else{
/*			glMultMatrixd(wf->irot);*/
		}

/*		glPushMatrix();
		glRotated(-gunangle, 1., 0., 0.);
		glBegin(GL_LINES);
		glVertex3d(-.15, 0., -1.);
		glVertex3d(-.05, 0., -1.);
		glVertex3d( .15, 0., -1.);
		glVertex3d( .05, 0., -1.);
		glVertex3d(0., -.15, -1.);
		glVertex3d(0., -.05, -1.);
		glVertex3d(0., .15, -1.);
		glVertex3d(0., .05, -1.);
		glEnd();
		glPopMatrix();*/

/*		glRotated(deg_per_rad * pt->pyr[0], 1., 0., 0.);
		glRotated(deg_per_rad * pt->pyr[2], 0., 0., 1.);

		cuts = CircleCuts(64);

		glBegin(GL_LINES);
		glVertex3d(-.35, 0., -1.);
		glVertex3d(-.20, 0., -1.);
		glVertex3d( .20, 0., -1.);
		glVertex3d( .35, 0., -1.);
		for(i = 0; i < 16; i++){
			int k = i < 8 ? i : i - 8, sign = i < 8 ? 1 : -1;
			double y = sign * cuts[k][0];
			double z = -cuts[k][1];
			glVertex3d(-.35, y, z);
			glVertex3d(-.25, y, z);
			glVertex3d( .35, y, z);
			glVertex3d( .25, y, z);
		}
		glEnd();
*/
	}
	glPopMatrix();
#endif

	GLWchart::addSampleToCharts("tratime", TimeMeasLap(&tm));
/*	printf("fly_drawHUD %lg\n", TimeMeasLap(&tm));*/
}

void A10::drawCockpit(WarDraw *wd){
	static bool init = false;
//	double scale = .0002 /*wd->pgc->znear / wd->pgc->zfar*/;
//	double sonear = scale * wd->vw->gc->getNear();
//	double wid = sonear * wd->vw->gc->getFov() * wd->vw->gc->getWidth / wd->pgc->res;
//	double hei = sonear * wd->vw->gc->getFov() * wd->vw->gc->height / wd->pgc->res;
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
/*	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBegin(GL_QUADS);
	glVertex3d(.5, .5, -1.);
	glVertex3d(-.5, .5, -1.);
	glVertex3d(-.5, -.5, -1.);
	glVertex3d(.5, -.5, -1.);
	glEnd();*/


	glPushMatrix();

	glLoadMatrixd(wd->vw->rot);
	gldMultQuat(this->rot);

	glPushMatrix();

	glMatrixMode (GL_PROJECTION);
/*	glPushMatrix();
	glLoadIdentity();
	glFrustum (-wid, wid,
		-hei, hei,
		sonear, wd->pgc->znear * 5.);*/
	glMatrixMode (GL_MODELVIEW);

/*	if(pt->vft == &valkie_s && valcockpit){
		glScaled(.00001, .00001, -.00001);
		glTranslated(0, - 1., -3.5);
		glPushAttrib(GL_POLYGON_BIT);
		glFrontFace(GL_CW);
		DrawYSDNM(valcockpit, NULL, NULL, 0, NULL, 0);
		glPopAttrib();
	}
	else*/{
		gldTranslaten3dv(seat);
		glScaled(-modelScale, modelScale, -modelScale);
		DrawMQOPose(modelCockpit, nullptr);
	}

	glPopMatrix();

	drawCockpitHUD(hudPos, hudSize, seat, gunDirection, weaponList);

	glPushMatrix();

/*	gldTranslate3dv(stick);
	glRotated(deg_per_rad * p->elevator, 1., 0., 0.);
	glRotated(deg_per_rad * p->aileron[0], 0., 0., -1.);
	gldTranslaten3dv(stick);
	gldScaled(FLY_SCALE / 2.);

	DrawSUF(sufstick, SUF_ATR, &g_gldcache);*/

/*	glMatrixMode (GL_PROJECTION);
	glPopMatrix();
	glMatrixMode (GL_MODELVIEW);*/

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
