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
}

void GimbalTurret::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
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

	/* forget about beaten enemy */
	if(enemy && (enemy->health <= 0. || enemy->w != w))
		enemy = NULL;

	Mat4d mat;
	transform(mat);

	if(0 < health){
		Entity *collideignore = NULL;
	}
	else{
		this->w = NULL;
		return;
	}

	st::anim(dt);
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



/// \brief Loads and initializes model and motions.
/// \returns True if initialized or already initialized, false if something fail to initialize.
///
/// This process is completely for display, so defined in this Sldier-draw.cpp.
bool GimbalTurret::initModel(){
	static OpenGLState::weak_ptr<bool> init;

	if(!init){
		model = LoadMQOModel(modPath() << "models/GimbalTurret.mqo");
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

//	draw_healthbar(this, wd, health / maxhealth(), .1, 0, 0);

	if(!initModel())
		return;

	const double scale = modelScale;

	glPushMatrix();

	Mat4d mat;
	transform(mat);
	glMultMatrixd(mat);
	glScaled(-scale, scale, -scale);
	glTranslated(0, 54.4, 0);
#if DNMMOT_PROFILE
	{
		timemeas_t tm;
		TimeMeasStart(&tm);
#endif
		DrawMQOPose(model, NULL);
#if DNMMOT_PROFILE
		printf("motdraw %lg\n", TimeMeasLap(&tm));
	}
#endif
	glPopMatrix();

}

void GimbalTurret::drawtra(wardraw_t *wd){
	st::drawtra(wd);
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



