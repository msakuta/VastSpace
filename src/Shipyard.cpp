/** \file
 * \brief Implementation of Shipyard class non-drawing member functions.
 */
#include "Shipyard.h"
#include "judge.h"
#include "serial_util.h"
#include "Player.h"
#include "antiglut.h"
#include "sqadapt.h"
#include "btadapt.h"
extern "C"{
#include <clib/mathdef.h>
#include <clib/cfloat.h>
}
#include <sqstdio.h>
#ifndef DEDICATED
#include <gl/GL.h>
#endif

/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289
#define SQRT2P2 (M_SQRT2/2.)




#if 1
const unsigned ShipyardDocker::classid = registerClass("ShipyardDocker", Conster<ShipyardDocker>);

const char *ShipyardDocker::classname()const{return "ShipyardDocker";}

void ShipyardDocker::dock(Dockable *e){
	st::dock(e);
	if(getent()->dockingFrigate == e)
		getent()->dockingFrigate = NULL;
}

void ShipyardDocker::dockque(Dockable *e){
	QueryClassCommand com;
	com.ret = Docker::Fighter;
	e->command(&com);
	if(com.ret == Docker::Frigate){
		static_cast<Shipyard*>(this->e)->dockingFrigate = e;
	}
}


#define SCARRY_MAX_HEALTH 200000
#define SCARRY_MAX_SPEED .03
#define SCARRY_ACCELERATE .01
#define SCARRY_MAX_ANGLESPEED (.005 * M_PI)
#define SCARRY_ANGLEACCEL (.002 * M_PI)

double Shipyard::modelScale = 0.0010;
double Shipyard::hitRadius = 1.0;
double Shipyard::defaultMass = 5e9;

hardpoint_static *Shipyard::hardpoints = NULL/*[10] = {
	hardpoint_static(Vec3d(.100, .060, -.760), Quatd(0., 0., 0., 1.), "Turret 1", 0),
	hardpoint_static(Vec3d(.100, -.060, -.760), Quatd(0., 0., 1., 0.), "Turret 2", 0),
	hardpoint_static(Vec3d(-.180,  .180, .420), Quatd(0., 0., 0., 1.), "Turret 3", 0),
	hardpoint_static(Vec3d(-.180, -.180, .420), Quatd(0., 0., 1., 0.), "Turret 4", 0),
	hardpoint_static(Vec3d( .180,  .180, .420), Quatd(0., 0., 0., 1.), "Turret 5", 0),
	hardpoint_static(Vec3d( .180, -.180, .420), Quatd(0., 0., 1., 0.), "Turret 6", 0),
	hardpoint_static(Vec3d(-.180,  .180, -.290), Quatd(0., 0., 0., 1.), "Turret 7", 0),
	hardpoint_static(Vec3d(-.180, -.180, -.290), Quatd(0., 0., 1., 0.), "Turret 8", 0),
	hardpoint_static(Vec3d( .180,  .180, -.380), Quatd(0., 0., 0., 1.), "Turret 9", 0),
	hardpoint_static(Vec3d( .180, -.180, -.380), Quatd(0., 0., 1., 0.), "Turret 10", 0),
}*/;
int Shipyard::nhardpoints = 0;

Shipyard::Shipyard(WarField *w) : st(w), docker(new ShipyardDocker(this)), Builder(this->Entity::w){
	st::init();
	init();
//	for(int i = 0; i < nhardpoints; i++)
//		turrets[i] = NULL;
//		w->addent(turrets[i] = new MTurret(this, &hardpoints[i]));
}

void Shipyard::init(){
	static bool initialized = false;
	if(!initialized){
		sq_init(_SC("models/Shipyard.nut"),
				ModelScaleProcess(modelScale) <<=
				SingleDoubleProcess(hitRadius, "hitRadius", false) <<=
				MassProcess(defaultMass) <<=
				HitboxProcess(hitboxes) <<=
				DrawOverlayProcess(disp) <<=
				NavlightsProcess(navlights));
		initialized = true;
	}
	ru = 0.;
/*	if(!hardpoints){
		hardpoints = hardpoint_static::load("scarry.hb", nhardpoints);
	}
	turrets = new ArmBase*[nhardpoints];*/
	mass = defaultMass;
	doorphase[0] = 0.;
	doorphase[1] = 0.;
}

Shipyard::~Shipyard(){
	delete docker;
}

const char *Shipyard::idname()const{return "Shipyard";}
const char *Shipyard::classname()const{return "Shipyard";}

const unsigned Shipyard::classid = registerClass("Shipyard", Conster<Shipyard>);
Entity::EntityRegister<Shipyard> Shipyard::entityRegister("Shipyard");

void Shipyard::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << docker;
	sc.o << undockingFrigate;
	sc.o << dockingFrigate;
//	for(int i = 0; i < nhardpoints; i++)
//		sc.o << turrets[i];
}

void Shipyard::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> docker;
	sc.i >> undockingFrigate;
	sc.i >> dockingFrigate;

	// Update the dynamics body's parameters too.
	if(bbody){
		bbody->setCenterOfMassTransform(btTransform(btqc(rot), btvc(pos)));
		bbody->setAngularVelocity(btvc(omg));
		bbody->setLinearVelocity(btvc(velo));
	}

//	for(int i = 0; i < nhardpoints; i++)
//		sc.i >> turrets[i];
}

void Shipyard::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	st::dive(sc, method);
	docker->dive(sc, method);
}

const char *Shipyard::dispname()const{
	return "Shipyard";
};

double Shipyard::maxhealth()const{
	return 200000;
}

double Shipyard::getHitRadius()const{
	return hitRadius;
}

double Shipyard::maxenergy()const{
	return 5e8;
}

void Shipyard::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	static const Vec3d pos0[] = {
		Vec3d(.100, .22, -.610),
		Vec3d(.100, .10, -1.000),
		Vec3d(-.100, .10, 1.000),
	};
	if(seatid < numof(pos0)){
		pos = this->pos + this->rot.trans(pos0[seatid % numof(pos0)]);
		rot = this->rot;
	}
	else{
		Quatd trot = this->rot.rotate(Entity::w->war_time() * M_PI / 6., 0, 1, 0).rotate(sin(Entity::w->war_time() * M_PI / 23.) * M_PI / 3., 1, 0, 0);
		pos = this->pos + trot.trans(Vec3d(0, 0, 1.));
		rot = trot;
	}
}

bool Shipyard::buildBody(){
	if(!bbody){
		static btCompoundShape *shape = NULL;
		if(!shape){
			shape = new btCompoundShape();
			// Assign dummy value if the initialize file is not available.
			if(hitboxes.empty())
				hitboxes.push_back(hitbox(Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(.3, .2, .500)));
			for(int i = 0; i < hitboxes.size(); i++){
				const Vec3d &sc = hitboxes[i].sc;
				const Quatd &rot = hitboxes[i].rot;
				const Vec3d &pos = hitboxes[i].org;
				btBoxShape *box = new btBoxShape(btvc(sc));
				btTransform trans = btTransform(btqc(rot), btvc(pos));
				shape->addChildShape(trans, box);
			}
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
//		rbInfo.m_linearDamping = .5;
//		rbInfo.m_angularDamping = .25;
		bbody = new btRigidBody(rbInfo);

//		bbody->setSleepingThresholds(.0001, .0001);

		//add the body to the dynamics world
//		ws->bdw->addRigidBody(bbody);
		return true;
	}
	return false;
}


short Shipyard::bbodyGroup()const{
	return 2;
}

short Shipyard::bbodyMask()const{
	return ~2;
}

std::vector<hitbox> *Shipyard::getTraceHitBoxes()const{
	return &hitboxes;
}

void Shipyard::anim(double dt){
	st::anim(dt);
	docker->anim(dt);
//	Builder::anim(dt);
//	for(int i = 0; i < nhardpoints; i++) if(turrets[i])
//		turrets[i]->align();
	clientUpdate(dt);
}

void Shipyard::clientUpdate(double dt){
	if(undockingFrigate){
		double threshdist = .5 + undockingFrigate->getHitRadius();
		const Vec3d &udpos = undockingFrigate->pos;
		Vec3d delta = udpos - (pos + rot.trans(Vec3d(0.1, 0, -0.35)));
		if(threshdist * threshdist < delta.slen())
			undockingFrigate = NULL;
		else
			doorphase[0] = approach(doorphase[0], 1., dt * .5, 0.);
	}
	else
		doorphase[0] = approach(doorphase[0], 0., dt * 0.5, 0.);

	if(dockingFrigate){
		if(dockingFrigate->w != Entity::w)
			dockingFrigate = NULL;
		else
			doorphase[1] = approach(doorphase[1], 1., dt * .5, 0.);
	}
	else
		doorphase[1] = approach(doorphase[1], 0., dt * .5, 0.);
}

Entity::Props Shipyard::props()const{
	Props ret = st::props();
//	ret.push_back(cpplib::dstring("?: "));
	return ret;
}

int Shipyard::armsCount()const{
	return nhardpoints;
}

ArmBase *Shipyard::armsGet(int i){
	return turrets[i];
}

double Shipyard::getRU()const{return ru;}
Builder *Shipyard::getBuilderInt(){return this;}
Docker *Shipyard::getDockerInt(){return docker;}


const Shipyard::ManeuverParams Shipyard::mymn = {
	SCARRY_ACCELERATE, /* double accel; */
	SCARRY_MAX_SPEED, /* double maxspeed; */
	SCARRY_ANGLEACCEL, /* double angleaccel; */
	SCARRY_MAX_ANGLESPEED, /* double maxanglespeed; */
	5e8, /* double capacity; */
	1e6, /* double capacitor_gen; */
};

const Warpable::ManeuverParams &Shipyard::getManeuve()const{return mymn;}

HitBoxList Shipyard::hitboxes;
GLuint Shipyard::disp = 0;
std::vector<Shipyard::Navlight> Shipyard::navlights;

void Shipyard::doneBuild(Entity *e){
	Entity::Dockable *d = e->toDockable();
	if(d)
		docker->dock(d);
	else{
		e->w = this->Entity::w;
		this->Entity::w->addent(e);
		e->pos = pos + rot.trans(Vec3d(-.10, 0.05, 0));
		e->velo = velo;
		e->rot = rot;
	}
/*	e->pos = pos + rot.trans(Vec3d(-.15, 0.05, 0));
	e->velo = velo;
	e->rot = rot;
	static_cast<Dockable*>(e)->undock(this);*/
}

bool ShipyardDocker::undock(Entity::Dockable *pe){
	if(st::undock(pe)){
		QueryClassCommand com;
		com.ret = Docker::Fighter;
		pe->command(&com);
		Vec3d dockPos;
		if(com.ret == Docker::Fighter)
			dockPos = Vec3d(-.10, 0.05, 0);
		else if(com.ret == Docker::Frigate){
			dockPos = Vec3d(.10, 0.0, -.350);
			static_cast<Shipyard*>(e)->undockingFrigate = pe;
		}
		else
			dockPos = Vec3d(1.0, 0.0, 0);
		Vec3d pos = e->pos + e->rot.trans(dockPos);
		pe->setPosition(&pos, &e->rot, &e->velo, &e->omg);
		return true;
	}
	return false;
}

Vec3d ShipyardDocker::getPortPos(Dockable *e)const{
	QueryClassCommand com;
	com.ret = Fighter;
	e->command(&com);
	if(com.ret == Fighter)
		return Vec3d(-100. * SCARRY_SCALE, -50. * SCARRY_SCALE, -0.350);
	else if(com.ret == Frigate)
		return Vec3d(100. * SCARRY_SCALE, 0, 0.350);
}

Quatd ShipyardDocker::getPortRot(Dockable *)const{
	return quat_u;
}
#endif

#ifndef _WIN32
int Shipyard::popupMenu(PopupMenu &list){return st::popupMenu(list);}
void Shipyard::draw(WarDraw*){}
void Shipyard::drawtra(WarDraw*){}
void Shipyard::drawOverlay(wardraw_t *){}
#endif
