/** \file
 * \brief Implementation of GimbalTurret and MissileGimbalTurret classes.
 */
#include "GimbalTurret.h"
#include "EntityRegister.h"
#include "vastspace.h"
#include "serial_util.h"
#include "btadapt.h"
#include "sqadapt.h"
#include "Game.h"
#include "Bullet.h"
#include "Missile.h"
#include "BeamProjectile.h"
extern "C"{
#include <clib/cfloat.h>
#include <clib/mathdef.h>
}






GimbalTurret::GimbalTurret(WarField *aw) : st(aw){
	init();
}

void GimbalTurret::init(){
	static bool initialized = false;
	if(!initialized){
		sq_init(modPath() << _SC("models/GimbalTurret.nut"),
			ModelScaleProcess(modelScale) <<=
			MassProcess(defaultMass) <<=
			SingleDoubleProcess(maxHealthValue, "maxhealth", false) <<=
			DrawOverlayProcess(overlayDisp));
		initialized = true;
	}

	yaw = 0.;
	pitch = 0.;
	cooldown = 0.;
	muzzleFlash = 0.;
	health = getMaxHealth();
	mass = 2e6;
	deathEffectDone = false;
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

	// Update the dynamics body's parameters too if the values are sent from upstream Server.
	if(bbody){
		bbody->setCenterOfMassTransform(btTransform(btqc(rot), btvc(pos)));
		bbody->setAngularVelocity(btvc(omg));
		bbody->setLinearVelocity(btvc(velo));
		bbody->activate();
	}
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
		omg = btvc(bbody->getAngularVelocity());
	}

	Mat4d mat;
	transform(mat);

	findtarget(NULL, 0);

	/* forget about beaten enemy */
	if(enemy && (enemy->getHealth() <= 0. || enemy->w != w))
		enemy = NULL;

	if(0 < health){
		if(task != sship_moveto && enemy){
			Vec3d xh, dh, vh;
			Vec3d epos;
			estimate_pos(epos, enemy->pos, enemy->velo, this->pos, this->velo, bulletspeed(), w);
			Vec3d delta = epos - this->pos;

			Vec3d ldelta = mat.tdvp3(delta);
			double ldeltaLen = ldelta.len();
			double desiredYaw = rangein(-ldelta[0] / ldeltaLen * 20., -1, 1);
			yaw -= desiredYaw * dt * 20.;
			yaw -= floor(yaw / (2. * M_PI)) * 2. * M_PI;
			double desiredPitch = rangein(ldelta[1] / ldeltaLen * 20., -1, 1);
			pitch += desiredPitch * dt * 20.;
			pitch -= floor(pitch / (2. * M_PI)) * 2. * M_PI;

			if(fabs(desiredYaw) < shootPatience() && fabs(desiredPitch) < shootPatience())
				shoot(dt);

			if(bbody){
				btTransform tra = bbody->getWorldTransform();
				tra.setRotation(tra.getRotation()
					* btQuaternion(btVector3(0,1,0), desiredYaw * dt)
					* btQuaternion(btVector3(1,0,0), desiredPitch * dt));
				bbody->setWorldTransform(tra);
			}
		}

		if(cooldown < dt)
			cooldown = 0;
		else
			cooldown -= dt;
	}
	else{ // If health goes below 0, be dead in a instant.
		if(game->isServer()){
			delete this;
			return;
		}
		else
			deathEffect();
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


double GimbalTurret::modelScale = 0.1;
double GimbalTurret::defaultMass = 10000.;
double GimbalTurret::hitRadius = 10.;
double GimbalTurret::maxHealthValue = 1500.;
GLuint GimbalTurret::overlayDisp = 0;


Model *GimbalTurret::model = NULL;
Motion *GimbalTurret::motions[2] = {NULL};
Vec3d GimbalTurret::gunPos[2] = {
	Vec3d(50. * modelScale, 0. * modelScale, -120. * modelScale),
	Vec3d(-50. * modelScale, 0. * modelScale, -120. * modelScale)
};

Model *MissileGimbalTurret::model = NULL;


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

double GimbalTurret::getMaxHealth()const{return maxHealthValue;}

const Autonomous::ManeuverParams &GimbalTurret::getManeuve()const{
	static const ManeuverParams mn = {
		5., /* double accel; */
		10., /* double maxspeed; */
		50., /* double angleaccel; */
		.4, /* double maxanglespeed; */
		50000., /* double capacity; [MJ] */
		100., /* double capacitor_gen; [MW] */
	};
	return mn;
}


float GimbalTurret::reloadtime()const{
	return 0.3;
}

double GimbalTurret::bulletspeed()const{
	return 3000.;
}

float GimbalTurret::bulletlife()const{
	return 3.;
}

double GimbalTurret::bulletDamage()const{
	return 10.;
}

double GimbalTurret::shootPatience()const{
	return 0.5;
}

/// \brief The predicate to weigh precedence of Entities individually.
double GimbalTurret::findtargetproc(const Entity *)const{
	return 1.;
}

/// \brief Finds nearest or best effective target for shooting.
///
/// The algorithm was copied from MTurret.
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

		if(!(pt2->isTargettable() && pt2 != this && pt2->w == w && pt2->getHealth() > 0. && pt2->race != -1 && pt2->race != this->race))
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

/// \brief Try to shoot the guns, but can fail if not reloaded.
void GimbalTurret::shoot(double dt){

	if(dt <= cooldown)
		return;

	// Experimentally allow client game to create bullets.
	/*if(game->isServer())*/{
		for(int i = 0; i < 2; i++)
			createBullet(gunPos[i]);
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

Bullet *GimbalTurret::createBullet(const Vec3d &gunPos){
	Mat4d mat;
	transform(mat);
	Bullet *pb = new Bullet(this, bulletlife(), bulletDamage());
	Vec3d pos = mat.vp3(gunPos);
	Vec3d velo = mat.dvp3(Vec3d(0., 0., -bulletspeed())) + this->velo;
	pb->setPosition(&pos, &rot, &velo);
	w->addent(pb);
	return pb;
}

#ifdef DEDICATED
void GimbalTurret::draw(WarDraw *){}
void GimbalTurret::drawtra(wardraw_t *){}
void GimbalTurret::drawOverlay(wardraw_t *){}
Model *GimbalTurret::initModel(){return NULL;}
Model *MissileGimbalTurret::initModel(){return NULL;}
void GimbalTurret::deathEffect(){}
#endif


const char *MissileGimbalTurret::idname()const{
	return "MissileGimbalTurret";
}

const char *MissileGimbalTurret::classname()const{
	return "MissileGimbalTurret";
}

const unsigned MissileGimbalTurret::classid = registerClass("MissileGimbalTurret", Conster<MissileGimbalTurret>);
Entity::EntityRegister<MissileGimbalTurret> MissileGimbalTurret::entityRegister("MissileGimbalTurret");

/// \brief Creates a Missile instead of a Bullet.
Bullet *MissileGimbalTurret::createBullet(const Vec3d &gunPos){
	Mat4d mat;
	transform(mat);
	Missile *pb = new Missile(this, bulletlife(), bulletDamage(), enemy);
	Vec3d pos = mat.vp3(gunPos);
	Vec3d velo = mat.dvp3(Vec3d(0., 0., -bulletspeed())) + this->velo;
	pb->setPosition(&pos, &rot, &velo);
	w->addent(pb);
	return pb;
}




const char *BeamGimbalTurret::idname()const{
	return "BeamGimbalTurret";
}

const char *BeamGimbalTurret::classname()const{
	return "BeamGimbalTurret";
}

const unsigned BeamGimbalTurret::classid = registerClass("BeamGimbalTurret", Conster<BeamGimbalTurret>);
Entity::EntityRegister<BeamGimbalTurret> BeamGimbalTurret::entityRegister("BeamGimbalTurret");

/// \brief Creates a BeamProjectile instead of a Bullet.
Bullet *BeamGimbalTurret::createBullet(const Vec3d &gunPos){
	Mat4d mat;
	transform(mat);
	BeamProjectile *pb = new BeamProjectile(this, bulletlife(), bulletDamage());
	Vec3d pos = mat.vp3(gunPos);
	Vec3d velo = mat.dvp3(Vec3d(0., 0., -bulletspeed())) + this->velo;
	// Bullet head direction correction, copied from Defender.cpp.
	if(enemy){
		Vec3d epos;
		estimate_pos(epos, enemy->pos, enemy->velo, pos, this->velo, bulletspeed(), w);
		Vec3d dv = (epos - pos).normin();
		if(.95 < dv.sp(mat.dvp3(Vec3d(0,0,-1)))){
			velo = dv * bulletspeed();
		}
	}
	pb->setPosition(&pos, &rot, &velo);
	w->addent(pb);
	return pb;
}


