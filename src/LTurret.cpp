/** \file
 * \brief Implementation of LTurret and LMissileTurret.
 */
#include "LTurret.h"
#include "EntityRegister.h"
#include "Bullet.h"
#include "serial_util.h"
#include "Missile.h"
#include "SqInitProcess.h"
#include "Game.h"
extern "C"{
#include <clib/c.h>
#include <clib/mathdef.h>
#include <clib/cfloat.h>
}

double LTurret::modelScale = 1.;
double LTurret::hitRadius = 30.;
double LTurret::turretVariance = 0.001 * M_PI;
double LTurret::turretIntolerance = M_PI / 20.;
double LTurret::rotateSpeed = 0.4 * M_PI;
double LTurret::manualRotateSpeed = rotateSpeed * 0.5;
gltestp::dstring LTurret::modelFile = "models/lturret1.mqo";
gltestp::dstring LTurret::turretObjName = "lturret";
gltestp::dstring LTurret::barrelObjName = "lbarrel";
gltestp::dstring LTurret::muzzleObjName = "lmuzzle";

static const double lturret_range[2][2] = {-M_PI / 16., M_PI / 2, -M_PI, M_PI};


Entity::EntityRegisterNC<LTurret> LTurret::entityRegister("LTurret");
Entity::EntityStatic &LTurret::getStatic()const{return entityRegister;}

void LTurret::init(){
	static bool initialized = false;
	if(!initialized){
		SqInit(game->sqvm, _SC("models/LTurret.nut"),
			SingleDoubleProcess(modelScale, _SC("modelScale")) <<=
			SingleDoubleProcess(hitRadius, _SC("hitRadius")) <<=
			SingleDoubleProcess(turretVariance, _SC("turretVariance")) <<=
			SingleDoubleProcess(turretIntolerance, _SC("turretIntolerance")) <<=
			SingleDoubleProcess(rotateSpeed, _SC("rotateSpeed")) <<=
			SingleDoubleProcess(manualRotateSpeed, _SC("manualRotateSpeed")) <<=
			StringProcess(modelFile, _SC("modelFile")) <<=
			StringProcess(turretObjName, _SC("turretObjName")) <<=
			StringProcess(barrelObjName, _SC("barrelObjName")) <<=
			StringProcess(muzzleObjName, _SC("muzzleObjName"))
			);
		initialized = true;
	}
}

void LTurret::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << blowback;
	sc.o << blowbackspeed;
}

void LTurret::unserialize(UnserializeContext &sc){
	double orgblowbackspeed = blowbackspeed;
	st::unserialize(sc);
	sc.i >> blowback;
	sc.i >> blowbackspeed;

	// We can expect that only reason the blowbackspeed can increase is shooting the gun.
	if(orgblowbackspeed < blowbackspeed){
		Mat4d mat;
		shootTransform(mat);
		for(int i = 0; i < 2; i++){
			Vec3d lturret_ofs(5. * (i * 2 - 1), 0, -30.);
			Vec3d direction = -mat.vec3(2);
			shootEffect(mat.vp3(lturret_ofs), direction);
		}
	}
}


double LTurret::getHitRadius()const{return hitRadius;}

void LTurret::anim(double dt){
	st::anim(dt);
	blowback += blowbackspeed * dt;
	if(10. < blowback)
		blowbackspeed = 0;
	blowbackspeed *= exp(-dt);
	blowback *= exp(-dt);
}

void LTurret::clientUpdate(double dt){
	anim(dt);
}

float LTurret::getShootInterval()const{return 4.;}
float LTurret::getBulletLife()const{return 5.;}
double LTurret::getTurretVariance()const{return turretVariance;}
double LTurret::getTurretIntolerance()const{return turretIntolerance;}

/// \brief Returns transformation matrix and optional rotation quaternion indicating shooting bullet's orientation.
/// \param mat Filled with transformation matrix.
/// \param qrot Optional; can be NULL.
void LTurret::shootTransform(Mat4d &mat, Quatd *qrot)const{
	Mat4d mat2;
	base->transform(mat2);
	mat2.translatein(hp->pos);
	Mat4d rot = hp->rot.tomat4();
	mat = mat2 * rot;
	mat.translatein(0., 5., -2.5);
	double yaw = this->py[1] + (drseq(&w->rs) - .5) * getTurretVariance();
	double pitch = this->py[0] + (drseq(&w->rs) - .5) * getTurretVariance();
	mat2 = mat.roty(yaw);
	mat = mat2.rotx(pitch);
	if(qrot)
		*qrot = base->rot * hp->rot * Quatd(0, sin(yaw / 2.), 0, cos(yaw / 2.)) * Quatd(sin(pitch / 2.), 0, 0, cos(pitch / 2.));
}

void LTurret::tryshoot(){
	if(ammo <= 0)
		return;

	// Do not actually shoot in the client.
	// Yes, we can shoot in the client.
/*	if(!game->isServer()){
		this->cooldown += getShootInterval();
		return;
	}*/

	Mat4d mat;
	Quatd qrot;
	shootTransform(mat, &qrot);
	for(int i = 0; i < 2; i++){
		Vec3d lturret_ofs(5. * (i * 2 - 1), 0, -30.);
		Vec3d direction = -mat.vec3(2);
		Bullet *pz;
		pz = new Bullet(base, getBulletLife(), 800.);
		w->addent(pz);
		pz->pos = mat.vp3(lturret_ofs);
		pz->velo = direction * getBulletSpeed() + this->velo;
		pz->rot = qrot;
		shootEffect(pz->pos, direction);
	}
	this->cooldown += getShootInterval();
	this->mf += .3;
	blowbackspeed += 50.;
	ammo -= 2;
}

// prefer bigger targets
double LTurret::findtargetproc(const Entity *pb, const hardpoint_static *hp, const Entity *pt2){
	return pt2->getHitRadius() / .01;
}

#ifdef DEDICATED
void LTurret::draw(WarDraw *){}
void LTurret::drawtra(WarDraw *){}
void LTurret::shootEffect(const Vec3d&, const Vec3d&){}
#endif










double LMissileTurret::modelScale = 0.1 / 2.;
double LMissileTurret::hitRadius = 30.;
double LMissileTurret::turretVariance = 0.001 * M_PI;
double LMissileTurret::turretIntolerance = M_PI / 20.;
double LMissileTurret::rotateSpeed = 0.4 * M_PI;
double LMissileTurret::manualRotateSpeed = rotateSpeed * 0.5;
gltestp::dstring LMissileTurret::modelFile = "models/missile_launcher.mqo";
gltestp::dstring LMissileTurret::deployMotionFile = "models/missile_launcher_deploy.mot";


LMissileTurret::LMissileTurret(Game *game) : st(game), deploy(0){
	init();
}

LMissileTurret::LMissileTurret(Entity *abase, const hardpoint_static *hp) : st(abase, hp), deploy(0){
	init();
	ammo = 6;
}

void LMissileTurret::init(){
	static bool initialized = false;
	if(!initialized){
		SqInit(game->sqvm, _SC("models/LMissileTurret.nut"),
			SingleDoubleProcess(modelScale, _SC("modelScale")) <<=
			SingleDoubleProcess(hitRadius, _SC("hitRadius")) <<=
			SingleDoubleProcess(turretVariance, _SC("turretVariance")) <<=
			SingleDoubleProcess(turretIntolerance, _SC("turretIntolerance")) <<=
			SingleDoubleProcess(rotateSpeed, _SC("rotateSpeed")) <<=
			SingleDoubleProcess(manualRotateSpeed, _SC("manualRotateSpeed")) <<=
			StringProcess(modelFile, _SC("modelFile")) <<=
			StringProcess(deployMotionFile, _SC("deployMotionFile"))
			);
		initialized = true;
	}
}

Entity::EntityRegisterNC<LMissileTurret> LMissileTurret::entityRegister("LMissileTurret");

double LMissileTurret::getHitRadius()const{return hitRadius;}


int LMissileTurret::wantsFollowTarget()const{
	return 2 * (cooldown < 2. * getShootInterval());
}

void LMissileTurret::anim(double dt){
	st::anim(dt);
	if(target && cooldown < 2. * getShootInterval()){
		deploy = approach(deploy, 1., dt, 0.);
	}
	else{
		deploy = approach(deploy, 0., dt, 0.);
		py[0] = approach(py[0], 0., dt, 0.);
		py[0] = rangein(approach(py[0] + M_PI, 0. + M_PI, getRotateSpeed() * dt, 2 * M_PI) - M_PI, lturret_range[0][0], lturret_range[0][1]);
//		py[1] = approach(py[1], 0., dt, 0.);
	}
}

void LMissileTurret::clientUpdate(double dt){
	anim(dt);
}

double LMissileTurret::getBulletSpeed()const{return 1000.;}
float LMissileTurret::getShootInterval()const{return .5;}

void LMissileTurret::tryshoot(){
	if(ammo <= 0)
		return;
	static const Vec3d forward(0., 0., -1.);
	Mat4d mat2;
	base->transform(mat2);
	mat2.translatein(hp->pos);
	Mat4d rot = hp->rot.tomat4();
	Mat4d mat = mat2 * rot;
	mat.translatein(0., 10., 0.);
	double yaw = this->py[1] + (drseq(&w->rs) - .5) * getTurretVariance();
	double pitch = this->py[0] + (drseq(&w->rs) - .5) * getTurretVariance();
	const Vec3d barrelpos = modelScale * Vec3d(0, 200, 0) * deploy;
	const Vec3d joint = modelScale * Vec3d(0, 120, 60);
	mat2 = mat.roty(yaw);
	mat2.translatein(joint + barrelpos);
	mat = mat2.rotx(pitch);
	mat.translatein(-joint);
	mat.translatein(0., -10., 0.);
	Quatd qrot = base->rot * hp->rot * Quatd(0, sin(yaw / 2.), 0, cos(yaw / 2.)) * Quatd(sin(pitch / 2.), 0, 0, cos(pitch / 2.));
	ammo--;
	{
		Vec3d lturret_ofs = modelScale * Vec3d(80. * (ammo % 3 - 1), (40. + 40. + 80. * (ammo / 3)), 0);
		Bullet *pz;
		pz = new Missile(base, 15., 500., target);
		w->addent(pz);
		pz->pos = mat.vp3(lturret_ofs);
		pz->velo = mat.dvp3(forward) * .1*getBulletSpeed() + this->velo;
		pz->rot = qrot;
	}
	targets[numof(targets) - ammo - 1] = target;
	if(!ammo){
		this->cooldown += 17.;
		ammo = 6;
	}
	else
		this->cooldown += getShootInterval();
	this->mf += .3;
}

double LMissileTurret::findtargetproc(const Entity *pb, const hardpoint_static *hp, const Entity *pt2){
	double accumdamage = 0.;
	Missile::TargetMap::iterator it = Missile::targetmap().find(pt2);
	if(it != Missile::targetmap().end()){
		ObservableSet<Missile> &theMap = it->second;
		for(ObservableSet<Missile>::iterator pm = theMap.begin(); pm != theMap.end(); ++pm){
			accumdamage += (*pm)->getDamage();
			if(pt2->getHealth() < accumdamage)
				return 0.;
		}
	}
	return 1. / pt2->getHitRadius(); // precede small objects that conventional guns can hardly hit.
}

#ifdef DEDICATED
void LMissileTurret::draw(WarDraw *){}
void LMissileTurret::drawtra(WarDraw *){}
#endif

