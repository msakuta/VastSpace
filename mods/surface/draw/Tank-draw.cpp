/** \file
 * \brief Implementation of Tank class's graphics.
 */
#include "../Tank.h"
#include "Viewer.h"
#include "draw/WarDraw.h"
#include "draw/material.h"
#include "draw/mqoadapt.h"
#include "draw/effects.h"
#include "draw/OpenGLState.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/gl/gldraw.h>
}


#define FORESPEED 0.004
#define ROTSPEED (.075 * M_PI)
#define GUNVARIANCE .005
#define INTOLERANCE (M_PI / 50.)
#define BULLETRANGE 3.
#define RELOADTIME 3.
#define RETHINKTIME .5
#define TANK_MAX_GIBS 30





void Tank::draw(WarDraw *wd){
	static Motion *turretYawMotion = NULL;
	static Motion *barrelPitchMotion = NULL;
	if(!w)
		return;

	/* cull object */
	if(wd->vw->gc->cullFrustum(pos, .007))
		return;
	double pixels = .005 * fabs(wd->vw->gc->scale(pos));
	if(pixels < 2)
		return;
	wd->lightdraws++;


	static OpenGLState::weak_ptr<bool> init;
	if(!init){
		model = LoadMQOModel(modPath() << _SC("models/type90.mqo"));
		turretYawMotion = LoadMotion(modPath() << "models/type90-turretyaw.mot");
		barrelPitchMotion = LoadMotion(modPath() << "models/type90-barrelpitch.mot");
		init.create(*openGLState);
	};

	if(model){
		// TODO: We'd like to apply the team colors to the model sometime.
//		GLfloat fv[4] = {.8f, .5f, 0.f, 1.f}, ambient[4] = {.4f, .25f, 0.f, 1.f};
//		if(race % 2)
//			fv[0] = 0., fv[2] = .8f, ambient[0] = 0., ambient[2] = .4f;

		glPushMatrix();
		{
			Mat4d mat;
			transform(mat);
			glMultMatrixd(mat);
		}

		/* center of gravity offset */
		glTranslated(0, -getLandOffset(), 0);

		MotionPose mp[2];
		turretYawMotion->interpolate(mp[0], fmod(turrety / M_PI * 20. + 20., 40.));
		barrelPitchMotion->interpolate(mp[1], barrelp / M_PI * 20. + 10.);
		mp[0].next = &mp[1];

		// Unlike most ModelEntities, Tank's model need not be rotated 180 degrees because
		// the model is made such way.
		glScaled(modelScale, modelScale, modelScale);
		DrawMQOPose(model, mp);
		glPopMatrix();
	}

}

void Tank::drawtra(wardraw_t *wd){

	if(muzzle & 1){
		Vec3d mpos = tankMuzzlePos();
		drawmuzzleflasha(mpos, wd->vw->pos, .007, wd->vw->irot);
		muzzle = 0;
	}

/*	avec3_t v;
	amat4_t mat;
	int i;
	tankrot(mat, pt);
	glPushMatrix();
	glMultMatrixd(mat);
	glBegin(GL_LINES);
	for(i = 0; i < numof(p->forces); i++){
		VECCPY(v, points[i]);
		VECSADD(v, p->forces[i], 1e-3);
		glColor4ub(255,0,0,255);
		glVertex3dv(points[i]);
		glVertex3dv(v);
		VECCPY(v, points[i]);
		VECSADD(v, p->normals[i], 1e-3);
		glColor4ub(0,0,255,255);
		glVertex3dv(points[i]);
		glVertex3dv(v);
	}
	glEnd();
	glPopMatrix();*/
}

#if 1
void Tank::gib_draw(const Teline3CallbackData *p, const Teline3DrawData *dd, void *private_data){
	if(!model)
		return;
	glPushMatrix();
	glTranslated(p->pos[0], p->pos[1], p->pos[2]);
	glScaled(modelScale, modelScale, modelScale);
	int i = int(private_data);
//	DrawSUFPoly(model->sufs[i], i, SUF_ATR, NULL);
	model->sufs[i]->draw(SUF_ATR, NULL);
	glPopMatrix();
}
#endif

#if 0
static void tank_gib_draw(const Teline3CallbackData *pl, const Teline3DrawData *dd, void *pv){
	int i = (int)pv;
	gib_draw(pl, i < tank_s.sufbase->np ? tank_s.sufbase : tank_s.sufturret, TANK_SCALE, i < tank_s.sufbase->np ? i : i - tank_s.sufbase->np);
}
#endif

#if 0
void tank_drawHUD(entity_t *pt, warf_t *wf, wardraw_t *wd, const double irot[16], void (*gdraw)(void)){
	tank2_t *p = (tank2_t*)pt;
	base_drawHUD_target(pt, wf, wd, gdraw);
	base_drawHUD_map(pt, wf, wd, irot, gdraw);
	base_drawHUD(pt, wf, wd, gdraw);

	if(wf->pl->control != pt){
	}
	else{
		GLint vp[4];
		int w, h, m;
		double left, bottom;
/*		int tkills, tdeaths, ekills, edeaths;*/
/*		entity_t *pt2;*/
		double v;
		glGetIntegerv(GL_VIEWPORT, vp);
		w = vp[2], h = vp[3];
		m = w < h ? h : w;
		left = -(double)w / m;
		bottom = -(double)h / m;

		v = VECLEN(pt->velo);

		glPushMatrix();
		glLoadIdentity();
		glRasterPos3d(left + 20. / m, bottom + 60. / m, -1);
		gldprintf("%lg m/s", v * 1e3);
		glRasterPos3d(left + 20. / m, bottom + 80. / m, -1);
		gldprintf("%lg km/h", v * 1e3 * 3600. / 1000.);
		glRasterPos3d(-left - 500. / m, bottom + 60. / m, -1);
		gldprintf("%c 120mm Main Gun x %d", pt->weapon == 0 ? '>' : ' ', p->ammo[0]);
		glRasterPos3d(-left - 500. / m, bottom + 40. / m, -1);
		gldprintf("%c 7.62mm Type 74 x %d", pt->weapon == 1 ? '>' : ' ', p->ammo[1]);
		glRasterPos3d(-left - 500. / m, bottom + 20. / m, -1);
		gldprintf("%c 12.7mm M2 Browning x %d", pt->weapon == 2 ? '>' : ' ', p->ammo[2]);
		glRasterPos3d(-left - 500. / m, bottom + 0. / m, -1);
		gldprintf("%c 120mm Shotgun x %d", pt->weapon == 3 ? '>' : ' ', p->ammo[0]);
		glPopMatrix();
	}

}
#endif



void Tank::deathEffects(){
	Teline3List *tell = w->getTeline3d();

	Vec3d gravity = w->accel(this->pos, this->velo);
	if(tell){
		int j;
		for(j = 0; j < 20; j++){
			Vec3d velo(
				.15 * (w->rs.nextd() - .5),
				.15 * (w->rs.nextd() - .5),
				.15 * (w->rs.nextd() - .5));
			AddTeline3D(tell, this->pos, velo, .0025, quat_u, vec3_000, gravity,
				j % 2 ? COLOR32RGBA(255,255,255,255) : COLOR32RGBA(255,191,63,255), TEL3_HEADFORWARD | TEL3_FADEEND | TEL3_REFLECT, 1. + .5 * w->rs.nextd());
		}
/*			for(j = 0; j < 10; j++){
			double velo[3];
			velo[0] = .025 * (drseq(&gsrs) - .5);
			velo[1] = .025 * (drseq(&gsrs));
			velo[2] = .025 * (drseq(&gsrs) - .5);
			AddTefpol3D(w->tepl, pt->pos, velo, gravity, &cs_firetrail,
				TEP3_REFLECT | TEP_THICK | TEP_ROUGH, 3. + 1.5 * drseq(&gsrs));
		}*/

		{/* explode shockwave thingie */
			AddTeline3D(tell, this->pos, vec3_000, .1, Quatd::rotation(M_PI * 0.5, 1, 0, 0), vec3_000, vec3_000, COLOR32RGBA(255,191,63,255), TEL3_EXPANDISK/*/TEL3_EXPANDTORUS*/ | TEL3_NOLINE, 1.);
#if 0 // Scorch mark should be rendered in ground's rendering pass.
			if(pt->pos[1] <= 0.) /* no scorch in midair */
				AddTeline3D(tell, pt->pos, NULL, .01, pyr, NULL, NULL, COLOR32RGBA(0,0,0,255), TEL3_STATICDISK | TEL3_NOLINE, 20.);
#endif
		}

		WarSpace *ws = *w;
		if(ws->gibs && model){
			int m = model->n;
			int n = m <= TANK_MAX_GIBS ? m : TANK_MAX_GIBS;
			int base = m <= TANK_MAX_GIBS ? 0 : w->rs.next() % m;
			for(int i = 0; i < n; i++){
				j = (base + i) % m;
				Vec3d velo(
					this->velo[0] + (w->rs.nextd() - .5) * .1,
					this->velo[1] + (w->rs.nextd() - .5) * .1,
					this->velo[2] + (w->rs.nextd() - .5) * .1);
				Vec3d omg(
					3. * 2 * M_PI * (w->rs.nextd() - .5),
					3. * 2 * M_PI * (w->rs.nextd() - .5),
					3. * 2 * M_PI * (w->rs.nextd() - .5));
				AddTelineCallback3D(ws->gibs, this->pos, velo, 0.01, quat_u, omg, gravity, gib_draw, (void*)j, TEL3_NOLINE | TEL3_REFLECT, 1.5 + w->rs.nextd());
			}
		}
	}
}


void APFSDS::draw(WarDraw *wd){
	static Model *model = NULL;

	/* cull object */
	if(wd->vw->gc->cullFrustum(pos, 0.005))
		return;
	double pixels = 2. * 0.005 * fabs(wd->vw->gc->scale(pos));
	if(pixels < .1)
		return;
	wd->lightdraws++;

	if(pixels < 2.){
		glPushAttrib(GL_POINT_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glDisable(GL_POINT_SMOOTH);
		glPointSize(pixels);
		glColor4ub(63,63,63,255);
		glBegin(GL_POINTS);
		glVertex3dv(pos);
		glEnd();
		glPopAttrib();
		return;
	}

	if(!model){
		model = LoadMQOModel(Tank::modPath() << _SC("models/APFSDS.mqo"));
	}
	if(model){
		glPushMatrix();
		gldTranslate3dv(pos);
		glMultMatrixd(Quatd::direction(velo).tomat4());
		gldScaled(.00001);
		DrawMQOPose(model, NULL);
		glPopMatrix();
	}
}

void APFSDS::drawtra(WarDraw *wd){
	/* cull object */
	if(wd->vw->gc->cullFrustum(pos, 0.005))
		return;
	double pixels = 2. * 0.005 * fabs(wd->vw->gc->scale(pos));
	if(pixels < .1)
		return;

	/* bullet glow */
	{
		Vec3d dv = (pos - wd->vw->pos).norm();
		Vec3d forward = velo.norm();
		GLubyte a[4] = {255,255,127, (GLubyte)(128. * (forward.sp(dv) + 1.) / 2.)};
		if(2 < a[3]){
			glPushAttrib(GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT);
			glDisable(GL_LIGHTING);
			glDepthMask(0);
			gldSpriteGlow(pos, .010, a, wd->vw->irot);
			glPopAttrib();
		}
	}
}










//-----------------------------------------------------------------------------
//    Implementation for M3Truck
//-----------------------------------------------------------------------------

void M3Truck::draw(WarDraw *wd){
	if(!w)
		return;

	/* cull object */
	if(wd->vw->gc->cullFrustum(pos, .007))
		return;
	double pixels = .005 * fabs(wd->vw->gc->scale(pos));
	if(pixels < 2)
		return;
	wd->lightdraws++;


	static Motion *steerMotion = NULL;
	static Motion *turretYawMotion = NULL;
	static Motion *turretPitchMotion = NULL;

	static OpenGLState::weak_ptr<bool> init;
	if(!init){
		model = LoadMQOModel(modPath() << _SC("models/m3truck1.mqo"));
		steerMotion = LoadMotion(modPath() << "models/m3truck-steer.mot");
		turretYawMotion = LoadMotion(modPath() << "models/m3truck-turretyaw.mot");
		turretPitchMotion = LoadMotion(modPath() << "models/m3truck-turretpitch.mot");
		init.create(*openGLState);
	};

	if(model){

		glPushMatrix();
		{
			Mat4d mat;
			transform(mat);
			glMultMatrixd(mat);
		}

		MotionPose mp[3];
		steerMotion->interpolate(mp[0], steer / getMaxSteeringAngle() * 10. + 10.);
		turretYawMotion->interpolate(mp[1], fmod(turrety / M_PI * 20. + 20., 40.));
		mp[0].next = &mp[1];
		turretPitchMotion->interpolate(mp[2], barrelp / M_PI * 20. + 10.);
		mp[1].next = &mp[2];

		/* center of gravity offset */
		glTranslated(0, -getLandOffset(), 0);

		// Unlike most ModelEntities, Tank's model need not be rotated 180 degrees because
		// the model is made such way.
		glScaled(-modelScale, modelScale, -modelScale);
		DrawMQOPose(model, mp);
		glPopMatrix();
	}

}


void M3Truck::drawCockpit(WarDraw *wd){
	if(!w)
		return;

	Player *player = game->player;
	if(player->getChaseCamera() != 0 || player->mover != player->cockpitview || player->mover != player->nextmover)
		return;

	static Model *cockpitModel = NULL;
	static Motion *handleMotion = NULL;

	static OpenGLState::weak_ptr<bool> init;
	if(!init){
		cockpitModel = LoadMQOModel(modPath() << _SC("models/m3truck1_cockpit.mqo"));
		handleMotion = LoadMotion(modPath() << "models/m3truck-handle.mot");
		init.create(*openGLState);
	};

	if(!cockpitModel)
		return;

	Vec3d seat = cameraPositions[0] + Vec3d(0, getLandOffset(), 0);

	MotionPose mp[1];
	handleMotion->interpolate(mp[0], steer / getMaxSteeringAngle() * 10. + 10.);

	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glClearDepth(1.);
	glClear(GL_DEPTH_BUFFER_BIT);

	glPushMatrix();

	glLoadMatrixd(wd->vw->rot);
	gldMultQuat(this->rot);
	gldTranslaten3dv(seat);

	glScaled(-modelScale, modelScale, -modelScale);
	DrawMQOPose(cockpitModel, mp);

	glPopMatrix();

	glPopAttrib();
}

void M3Truck::deathEffects(){
	Teline3List *tell = w->getTeline3d();

	Vec3d gravity = w->accel(this->pos, this->velo);
	if(tell){
		int j;
		for(j = 0; j < 20; j++){
			Vec3d velo(
				.15 * (w->rs.nextd() - .5),
				.15 * (w->rs.nextd() - .5),
				.15 * (w->rs.nextd() - .5));
			AddTeline3D(tell, this->pos, velo, .0025, quat_u, vec3_000, gravity,
				j % 2 ? COLOR32RGBA(255,255,255,255) : COLOR32RGBA(255,191,63,255), TEL3_HEADFORWARD | TEL3_FADEEND | TEL3_REFLECT, 1. + .5 * w->rs.nextd());
		}

		/* explode shockwave thingie */
		AddTeline3D(tell, this->pos, vec3_000, .1, Quatd::rotation(M_PI * 0.5, 1, 0, 0), vec3_000, vec3_000, COLOR32RGBA(255,191,63,255), TEL3_EXPANDISK/*/TEL3_EXPANDTORUS*/ | TEL3_NOLINE, 1.);
	}
}
