#include "../F15.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/mqoadapt.h"
#include "glw/glwindow.h"

extern "C"{
#include "clib/gl/gldraw.h"
}


Model *F15::model;


void F15::draw(WarDraw *wd){
	static Motion *lodMotion = nullptr;
	static Motion *aileronMotion = nullptr;
	static Motion *elevatorMotion = nullptr;
	static Motion *rudderMotion = nullptr;
	static Motion *gearMotion = nullptr;
	
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
}

void F15::drawtra(WarDraw *wd){

	if(debugWings){
		// Show forces applied to each wings
		glBegin(GL_LINES);
		glColor4f(1,0,0,1);
		for(auto i = 0; i < getWings().size(); i++){
			if(i < force.size()){
				Vec3d org = this->pos + rot.trans(getWings()[i].pos);
				glVertex3dv(org);
				glVertex3dv(org + force[i] * 0.01);
			}
		}
		glEnd();

		// Show control surfaces directions
		glPushAttrib(GL_POLYGON_BIT);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glColor4f(0,0,1,1);
		for(auto it : getWings()){
			glPushMatrix();
			gldTranslate3dv(pos);

			gldMultQuat(rot);
			Quatd rot = quat_u;
			if(it.control == Wing::Control::Elevator)
				rot = Quatd::rotation(elevator * it.sensitivity, it.axis);
			else if(it.control == Wing::Control::Aileron)
				rot = Quatd::rotation(aileron * it.sensitivity, it.axis);
			else if(it.control == Wing::Control::Rudder)
				rot = Quatd::rotation(rudder * it.sensitivity, it.axis);

			// Obtain scales that makes cuboid with face areas proportional with
			// drag force perpendicular to the faces.
			//
			// Suppose $x,y,z$ are the each axis length.  We know that each side
			// face perpendicular to $x,y,z$ axes have area $X=yz, Y=zx, Z=xy$, respectively.
			// We can, for example, erase $y$ and $z$ from the equations to obtain
			// the length $x=\sqrt{YZ/X}$.
			//
			// Note that we cannot visualize sheer elements (other than diagonals) with
			// this technique.  I don't think doing so is needed.
			Vec3d sc;
			for(int i = 0; i < 3; i++) // Machine-code friendly loop logic
				sc[i] = ::sqrt((::fabs(it.aero[(i+1) % 3 * 4]) + ::fabs(it.aero[(i+2) % 3 * 4])) / ::fabs(it.aero[i * 4]));

			sc *= 0.0002; // Tweak visuals

			gldTranslate3dv(it.pos);
			gldMultQuat(rot);
			glScaled(sc[0], sc[1], sc[2]);

			// Draw cuboid-like wireframe to show each area along axes.
			glBegin(GL_QUADS);
			static const GLfloat arr[3][4] = {
				{-1, -1,  1, 1},
				{ 1, -1, -1, 1},
				{ 0,  0,  0, 0},
			};
			for(int i = 0; i < 3; i++){
				int i1 = (i + 1) % 3, i2 = (i + 2) % 3;
				for(int j = 0; j < 4; j++)
					glVertex3f(arr[i][j], arr[i1][j], arr[i2][j]);
			}
			glEnd();

			glPopMatrix();
		}
		glPopAttrib();
	}

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

void F15::drawHUD(WarDraw *wd){
	char buf[128];
/*	timemeas_t tm;*/
/*	glColor4ub(0,255,0,255);*/

/*	TimeMeasStart(&tm);*/

	st::drawHUD(wd);

	glLoadIdentity();

	glDisable(GL_LINE_SMOOTH);

#if 1
	if(game->player->getChaseCamera() == 0 && this->enemy){
		glPushMatrix();
		glLoadMatrixd(wd->vw->rot);
		Vec3d pos = (this->enemy->pos - this->pos).norm();
		gldTranslate3dv(pos);
		glMultMatrixd(wd->vw->irot);
		{
			Vec3d dp = this->enemy->pos - this->pos;
			double dist = dp.len();
			if(dist < 1.)
				sprintf(buf, "%.4g m", dist * 1000.);
			else
				sprintf(buf, "%.4g km", dist);
			glRasterPos3d(.01, .02, 0.);
			gldPutStrokeString(buf);
			Vec3d dv = this->enemy->velo - this->velo;
			dist = dv.sp(dp) / dist;
			if(dist < 1.)
				sprintf(buf, "%.4g m/s", dist * 1000.);
			else
				sprintf(buf, "%.4g km/s", dist);
			glRasterPos3d(.01, -.03, 0.);
			gldPutStrokeString(buf);
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

		if(game->player->getChaseCamera() == 0){
			Vec3d velo = this->velo.norm();
			glPushMatrix();
			glMultMatrixd(wd->vw->irot);
			gldTranslate3dv(velo);
			glMultMatrixd(wd->vw->rot);
			auto cuts = CircleCuts(16);
			glBegin(GL_LINE_LOOP);
			for(int i = 0; i < 16; i++)
				glVertex3d(cuts[i][0] * .02, cuts[i][1] * .02, 0.);
			glEnd();
			glPopMatrix();
		}

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

/*	printf("fly_drawHUD %lg\n", TimeMeasLap(&tm));*/
}

void F15::drawCockpit(WarDraw *wd){
	static bool init = false;
//	double scale = .0002 /*wd->pgc->znear / wd->pgc->zfar*/;
//	double sonear = scale * wd->vw->gc->getNear();
//	double wid = sonear * wd->vw->gc->getFov() * wd->vw->gc->getWidth / wd->pgc->res;
//	double hei = sonear * wd->vw->gc->getFov() * wd->vw->gc->height / wd->pgc->res;
	const Vec3d &seat = cameraPositions[0];
	Vec3d stick = Vec3d(0., 68., -270.) * modelScale / 2.;
	static Model *modelCockpit = NULL;
	Player *player = game->player;
	if(player->getChaseCamera() != 0 || player->mover != player->cockpitview || player->mover != player->nextmover)
		return;

	if(!init){
		init = true;
		modelCockpit = LoadMQOModel(modPath() << "models/F15-cockpit.mqo");
	}


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

	if(0. < health){
		static const GLfloat hudcolor[4] = {0, 1, 0, 1};
//		GLfloat mat_diffuse[] = { .5, .5, .5, .2 };
//		GLfloat mat_ambient_color[] = { 0.5, 0.5, 0.5, .2 };
		Mat4d m;
		double d, velo;
		int i;

		double air = w->atmosphericPressure(this->pos)/*exp(-pt->pos[1] / 10.)*/;

		glPushMatrix();
	/*	glRotated(-gunangle, 1., 0., 0.);*/
		glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_LINE_SMOOTH);
		glDepthMask(GL_FALSE);
		gldTranslate3dv(hudPos - seat);
		gldScaled(hudSize);
/*		gldScaled(.0002);
		glTranslated(0., 0., -1.);*/
		glGetDoublev(GL_MODELVIEW_MATRIX, m);

		glDisable(GL_LIGHTING);
/*		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_color);*/
		glColor4ub(0,127,255,255);
		glBegin(GL_LINE_LOOP);
		glVertex2d(-1., -1.);
		glVertex2d( 1., -1.);
		glVertex2d( 1.,  1.);
		glVertex2d(-1.,  1.);
		glEnd();

		/* crosshair */
		glColor4fv(hudcolor);
		glBegin(GL_LINES);
		glVertex2d(-.15, 0.);
		glVertex2d(-.05, 0.);
		glVertex2d( .15, 0.);
		glVertex2d( .05, 0.);
		glVertex2d(0., -.15);
		glVertex2d(0., -.05);
		glVertex2d(0., .15);
		glVertex2d(0., .05);
		glEnd();

		/* director */
/*		for(i = 0; i < 24; i++){
			d = fmod(-pt->pyr[1] + i * 2. * M_PI / 24. + M_PI / 2., M_PI * 2.) - M_PI / 2.;
			if(-.8 < d && d < .8){
				glBegin(GL_LINES);
				glVertex2d(d, .8);
				glVertex2d(d, i % 6 == 0 ? .7 : 0.75);
				glEnd();
				if(i % 6 == 0){
					int j, len;
					char buf[16];
					glPushMatrix();
					len = sprintf(buf, "%d", i / 6 * 90);
					glTranslated(d - (len - 1) * .03, .9, 0.);
					glScaled(.03, .05, .1);
					for(j = 0; buf[j]; j++){
						gldPolyChar(buf[j]);
						glTranslated(2., 0., 0.);
					}
					glPopMatrix();
				}
			}
		}*/

		/* pressure/height gauge */
		glBegin(GL_LINE_LOOP);
		glVertex2d(.92, -0.25);
		glVertex2d(.92, 0.25);
		glVertex2d(.9, 0.25);
		glVertex2d(.9, -.25);
		glEnd();
		glBegin(GL_LINES);
		glVertex2d(.92, -0.25 + air * .5);
		glVertex2d(.9, -0.25 + air * .5);
		glEnd();

		/* height gauge */
		{
			double height;
			char buf[16];
			height = -10. * log(air);
			glBegin(GL_LINE_LOOP);
			glVertex2d(.8, .0);
			glVertex2d(.9, 0.025);
			glVertex2d(.9, -0.025);
			glEnd();
			glBegin(GL_LINES);
			glVertex2d(.8, .3);
			glVertex2d(.8, height < .05 ? -height * .3 / .05 : -.3);
			for(d = MAX(.01 - fmod(height, .01), .05 - height) - .05; d < .05; d += .01){
				glVertex2d(.85, d * .3 / .05);
				glVertex2d(.8, d * .3 / .05);
			}
			glEnd();
			sprintf(buf, "%lg", height / 30.48e-5);
			glPushMatrix();
			glTranslated(.7, .7, 0.);
			glScaled(.015, .025, .1);
			gldPolyString(buf);
			glPopMatrix();
		}
/*		glRasterPos3d(.45, .5 + 20. / mi, -1.);
		gldprintf("%lg meters", pt->pos[1] * 1e3);
		glRasterPos3d(.45, .5 + 0. / mi, -1.);
		gldprintf("%lg feet", pt->pos[1] / 30.48e-5);*/

#if 1
		/* velocity */
		glPushMatrix();
		glTranslated(-.2, 0., 0.);
		glBegin(GL_LINES);
		velo = VECLEN(this->velo);
		glVertex2d(-.5, .3);
		glVertex2d(-.5, velo < .05 ? -velo * .3 / .05 : -.3);
		for(d = MAX(.01 - fmod(velo, .01), .05 - velo) - .05; d < .05; d += .01){
			glVertex2d(-.55, d * .3 / .05);
			glVertex2d(-.5, d * .3 / .05);
		}
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex2d(-.5, .0);
		glVertex2d(-.6, 0.025);
		glVertex2d(-.6, -0.025);
		glEnd();
		glPopMatrix();
		glPushMatrix();
		glTranslated(-.7, .7, 0.);
		glScaled(.015, .025, .1);
		gldPolyPrintf("M %lg", velo / .34);
		glPopMatrix();
		glPushMatrix();
		glTranslated(-.7, .7 - .08, 0.);
		glScaled(.015, .025, .1);
/*		gldPolyPrintf("%lg m/s", velo * 1e3);*/
		gldPolyPrintf("%lg", 1944. * velo);
		glPopMatrix();

		if(0 < health && !controller){
			const double fontSize = 0.005;
			glPushMatrix();
			glTranslated(-fontSize * glwGetSizeTextureString("AUTO PILOT", 12) / 2., 0, 0);
			glScaled(fontSize, -fontSize, .1);
			glwPutTextureString("AUTO PILOT", 12);
			glPopMatrix();
		}

		if(gear){
			glPushMatrix();
			glTranslated(-.7, -.7, 0.);
			glScaled(.005, -.005, .1);
			glwPutTextureString("GEAR", 12);
			glPopMatrix();
		}
#endif

		/* climb */
/*		glRotated(deg_per_rad * pt->pyr[2], 0., 0., 1.);
		for(i = -12; i <= 12; i++){
			d = 2. * (fmod(pt->pyr[0] + i * 2. * M_PI / 48. + M_PI / 2., M_PI * 2.) - M_PI / 2.);
			if(-.8 < d && d < .8){
				glBegin(GL_LINES);
				glVertex2d(-.4, d);
				glVertex2d(i % 6 == 0 ? -.5 : -0.45, d);
				glVertex2d(.4, d);
				glVertex2d(i % 6 == 0 ? .5 : 0.45, d);
				glEnd();
				if(i % 6 == 0){
					int j, len;
					char buf[16];
					glPushMatrix();
					len = sprintf(buf, "%d", i / 6 * 45);
					glTranslated(-.5 - len * .06, d, 0.);
					glScaled(.03, .05, .1);
					for(j = 0; buf[j]; j++){
						gldPolyChar(buf[j]);
						glTranslated(2., 0., 0.);
					}
					glPopMatrix();
				}
			}
		}*/

		glPopAttrib();
		glPopMatrix();
	}

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

void F15::drawOverlay(WarDraw *){
	glCallList(overlayDisp);
}

