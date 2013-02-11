/** \file
 * \brief Implementation of GimbalTurret class
 */
#include "GimbalTurret.h"
#include "Player.h"
#include "Viewer.h"
#include "EntityCommand.h"
#include "cmd.h"
#include "judge.h"
#include "astrodef.h"
#include "stellar_file.h"
#include "astro_star.h"
#include "serial_util.h"
#include "draw/material.h"
#include "motion.h"
#include "btadapt.h"
#include "glstack.h"
#include "draw/WarDraw.h"
#include "sqadapt.h"
#include "draw/OpenGLState.h"
#include "draw/ShaderBind.h"
#include "draw/mqoadapt.h"
#include "glsl.h"
#include "tefpol3d.h"
#include "Game.h"
#include "Bullet.h"
#include "draw/effects.h"
extern "C"{
#include "bitmap.h"
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/suf/sufbin.h>
#include <clib/suf/sufdraw.h>
#include <clib/suf/sufvbo.h>
#include <clib/GL/gldraw.h>
#include <clib/GL/cull.h>
#include <clib/GL/multitex.h>
#include <clib/wavsound.h>
#include <clib/zip/UnZip.h>
#include <clib/colseq/cs.h>
}
#include <assert.h>
#include <string.h>
#include <gl/glext.h>
#include <iostream>






GimbalTurret::GimbalTurret(WarField *aw) : st(aw){
	init();
}

void GimbalTurret::init(){
	yaw = 0.;
	pitch = 0.;
	cooldown = 0.;
	muzzleFlash = 0.;
	health = maxhealth();
	mass = 2e6;
}

GimbalTurret::~GimbalTurret(){
}

const char *GimbalTurret::idname()const{
	return "GimbalTurret";
}

const char *GimbalTurret::classname()const{
	return "GimbalTurret";
}

const unsigned GimbalTurret::classid = registerClass("GimbalTurret", Conster<GimbalTurret>);
Entity::EntityRegister<GimbalTurret> GimbalTurret::entityRegister("GimbalTurret");

void GimbalTurret::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << yaw;
	sc.o << pitch;
	sc.o << cooldown;
}

void GimbalTurret::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> yaw;
	sc.i >> pitch;
	sc.i >> cooldown;
}

const char *GimbalTurret::dispname()const{
	return "GimbalTurret";
}


void GimbalTurret::anim(double dt){
	WarSpace *ws = *w;
	if(!ws){
		st::anim(dt);
		return;
	}

	if(bbody){
		const btTransform &tra = bbody->getCenterOfMassTransform();
		pos = btvc(tra.getOrigin());
		rot = btqc(tra.getRotation());
		velo = btvc(bbody->getLinearVelocity());
	}

	Mat4d mat;
	transform(mat);

	findtarget(NULL, 0);

	/* forget about beaten enemy */
	if(enemy && (enemy->health <= 0. || enemy->w != w))
		enemy = NULL;

	if(enemy){
		Vec3d xh, dh, vh;
		Vec3d epos;
		estimate_pos(epos, enemy->pos, enemy->velo, this->pos, this->velo, bulletspeed(), w);
		Vec3d delta = epos - this->pos;

		Vec3d ldelta = mat.tdvp3(delta);
		double ldeltaLen = ldelta.len();
		double desiredYaw = rangein(-ldelta[0] / ldeltaLen * 10., -1, 1);
		yaw -= desiredYaw * dt * 10.;
		yaw -= floor(yaw / (2. * M_PI)) * 2. * M_PI;
		double desiredPitch = rangein(ldelta[1] / ldeltaLen * 10., -1, 1);
		pitch += desiredPitch * dt * 10.;
		pitch -= floor(pitch / (2. * M_PI)) * 2. * M_PI;

		if(fabs(desiredYaw) < 0.5 && fabs(desiredPitch) < 0.5)
			shoot(dt);

		if(bbody){
			btTransform tra = bbody->getWorldTransform();
			tra.setRotation(tra.getRotation()
				* btQuaternion(btVector3(0,1,0), desiredYaw * dt)
				* btQuaternion(btVector3(1,0,0), desiredPitch * dt));
			bbody->setWorldTransform(tra);
		}
	}


	if(0 < health){
		if(cooldown < dt)
			cooldown = 0;
		else
			cooldown -= dt;
	}

	st::anim(dt);
}

void GimbalTurret::clientUpdate(double dt){
	anim(dt);

	if(muzzleFlash < dt)
		muzzleFlash = 0.;
	else
		muzzleFlash -= float(dt);

}

void GimbalTurret::postframe(){
	st::postframe();
}

void GimbalTurret::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	pos = this->rot.trans(Vec3d(0, 120, 50) * modelScale) + this->pos;
	rot = this->rot;
}

void GimbalTurret::enterField(WarField *target){
	WarSpace *ws = *target;
	if(ws && ws->bdw){
		buildBody();
		ws->bdw->addRigidBody(bbody, 1, ~2);
	}
}

bool GimbalTurret::buildBody(){
	if(!bbody){
		static btCompoundShape *shape = NULL;
		if(!shape){
			shape = new btCompoundShape();
			Vec3d sc = Vec3d(50, 40, 150) * modelScale;
			const Quatd rot = quat_u;
			const Vec3d pos = Vec3d(0,0,0);
			btBoxShape *box = new btBoxShape(btvc(sc));
			btTransform trans = btTransform(btqc(rot), btvc(pos));
			shape->addChildShape(trans, box);
			shape->addChildShape(btTransform(btqc(rot), btVector3(0, 0, 100) * modelScale), new btBoxShape(btVector3(150, 10, 50) * modelScale));
		}

		btTransform startTransform;
		startTransform.setIdentity();
		startTransform.setOrigin(btvc(pos));

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			shape->calculateLocalInertia(mass,localInertia);

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,shape,localInertia);
		bbody = new btRigidBody(rbInfo);
		return true;
	}
	return false;
}

/// if we are transitting WarField or being destroyed, trailing tefpols should be marked for deleting.
void GimbalTurret::leaveField(WarField *w){
	st::leaveField(w);
}


double GimbalTurret::modelScale = 0.0001;
double GimbalTurret::hitRadius = 0.01;


Model *GimbalTurret::model = NULL;
Motion *GimbalTurret::motions[2] = {NULL};
Vec3d GimbalTurret::gunPos[2] = {
	Vec3d(50. * modelScale, 0. * modelScale, -120. * modelScale),
	Vec3d(-50. * modelScale, 0. * modelScale, -120. * modelScale)
};




/// \brief Loads and initializes model and motions.
/// \returns True if initialized or already initialized, false if something fail to initialize.
///
/// This process is completely for display, so defined in this Sldier-draw.cpp.
bool GimbalTurret::initModel(){
	static OpenGLState::weak_ptr<bool> init;

	if(!init){
		model = LoadMQOModel(modPath() << "models/GimbalTurret.mqo");
		motions[0] = LoadMotion(modPath() << "models/GimbalTurret_roty.mot");
		motions[1] = LoadMotion(modPath() << "models/GimbalTurret_rotx.mot");
		init.create(*openGLState);
	}

	return !!model;
}

void GimbalTurret::draw(WarDraw *wd){
	if(!w)
		return;

	/* cull object */
//	if(cull(wd))
//		return;

//	draw_healthbar(this, wd, health / maxhealth(), .1, 0, 0);

	if(!initModel())
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
	glScaled(10, 10, 1);
	glBegin(GL_LINE_LOOP);
	glVertex2d(-.10,  .00);
	glVertex2d(-.05, -.03);
	glVertex2d( .00, -.03);
	glVertex2d( .08, -.07);
	glVertex2d( .10, -.03);
	glVertex2d( .10,  .03);
	glVertex2d( .08,  .07);
	glVertex2d( .00,  .03);
	glVertex2d(-.05,  .03);
	glEnd();
}


bool GimbalTurret::dock(Docker *d){
//	d->e->command(&TransportPeopleCommand(people));
	return true;
}

bool GimbalTurret::undock(Docker *d){
//	task = sship_undock;
//	mother = d;
//	d->e->command(&TransportPeopleCommand(-people));
	return true;
}

double GimbalTurret::maxhealth()const{return 15000.;}

float GimbalTurret::reloadtime()const{
	return .15;
}

double GimbalTurret::bulletspeed()const{
	return 4.;
}

float GimbalTurret::bulletlife()const{
	return 1.;
}

double GimbalTurret::findtargetproc(const Entity *)const{
	return 1.;
}


void GimbalTurret::findtarget(const Entity *ignore_list[], int nignore_list){
	double bulletrange = bulletspeed() * bulletlife(); /* sense range */
	double best = bulletrange * bulletrange;
	static const Vec3d right(1., 0., 0.), left(-1., 0., 0.);
	Entity *closest = NULL;

	// Obtain reverse transformation matrix to the turret's local coordinate system.
	Mat4d mat2 = this->rot.cnj().tomat4().translatein(-this->pos);

	// Quit chasing target that is out of shooting range.
	if(enemy && best < (enemy->pos - pos).slen())
		enemy = NULL;

	for(WarField::EntityList::iterator it = w->el.begin(); it != w->el.end(); it++) if(*it){
		Entity *pt2 = *it;
		Vec3d delta, ldelta;
		double theta, phi, f;

		f = findtargetproc(pt2);
		if(f == 0.)
			continue;

		if(!(pt2->isTargettable() && pt2 != this && pt2->w == w && pt2->health > 0. && pt2->race != -1 && pt2->race != this->race))
			continue;

/*		if(!entity_visible(pb, pt2))
			continue;*/

		// Do not passively attack Resource Station that can be captured instead of being destroyed.
		if(!strcmp(pt2->classname(), "RStation"))
			continue;

		ldelta = mat2.vp3(pt2->pos);
		phi = -atan2(ldelta[2], ldelta[0]);
		theta = atan2(ldelta[1], sqrt(ldelta[0] * ldelta[0] + ldelta[2] * ldelta[2]));

		double sdist = (pt2->pos - this->pos).slen();
		if(bulletrange * bulletrange < sdist)
			continue;

		// Weigh already targetted enemy to keep shooting single enemy.
		sdist *= (pt2 == enemy ? .25 * .25 : 1.) / f;
		if(sdist < best){
			best = sdist;
			closest = pt2;
		}
	}
	if(closest)
		enemy = closest;
}

void GimbalTurret::shoot(double dt){

	if(dt <= cooldown)
		return;

	if(game->isServer()){
		Vec3d velo, gunpos, velo0(0., 0., -bulletspeed());
		Mat4d mat;
		int i = 0;
		transform(mat);
		do{
			Bullet *pb;
			pb = new Bullet(this, bulletlife(), 20.);
			w->addent(pb);
			pb->pos = mat.vp3(gunPos[i]);
			pb->velo = mat.dvp3(velo0);
			pb->velo += this->velo;
		} while(!i++);
	}
//	shootsound(pt, w, p->cooldown);
//	pt->shoots += 2;
//	if(0 < --magazine)
		this->cooldown += reloadtime()/* * (fuel <= 0 ? 3 : 1)*/;
//	else{
//		magazine = SCEPTOR_MAGAZINE;
//		this->cooldown += SCEPTOR_RELOADTIME;
//	}
	this->muzzleFlash = .1;
}
