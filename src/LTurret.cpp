/** \file
 * \brief Implementation of LTurret and LMissileTurret.
 */
#include "LTurret.h"
#include "Bullet.h"
#include "serial_util.h"
#include "Missile.h"
extern "C"{
#include <clib/c.h>
#include <clib/mathdef.h>
#include <clib/cfloat.h>
}

#define LTURRET_VARIANCE (.001 * M_PI)
#define LTURRETROTSPEED (.4*M_PI)

static const double lturret_range[2][2] = {-M_PI / 16., M_PI / 2, -M_PI, M_PI};


const char *LTurret::classname()const{return "LTurret";}
const unsigned LTurret::classid = registerClass("LTurret", Conster<LTurret>);

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
			Vec3d lturret_ofs(.005 * (i * 2 - 1), 0, -0.030);
			Vec3d direction = -mat.vec3(2);
			shootEffect(mat.vp3(lturret_ofs), direction);
		}
	}
}


double LTurret::getHitRadius()const{return .03;}

void LTurret::anim(double dt){
	st::anim(dt);
	blowback += blowbackspeed * dt;
	if(.010 < blowback)
		blowbackspeed = 0;
	blowbackspeed *= exp(-dt);
	blowback *= exp(-dt);
}

void LTurret::clientUpdate(double dt){
	anim(dt);
}

float LTurret::getShootInterval()const{return 4.;}
float LTurret::getBulletLife()const{return 5.;}

/// \brief Returns transformation matrix and optional rotation quaternion indicating shooting bullet's orientation.
/// \param mat Filled with transformation matrix.
/// \param qrot Optional; can be NULL.
void LTurret::shootTransform(Mat4d &mat, Quatd *qrot)const{
	Mat4d mat2;
	base->transform(mat2);
	mat2.translatein(hp->pos);
	Mat4d rot = hp->rot.tomat4();
	mat = mat2 * rot;
	mat.translatein(0., .005, -0.0025);
	double yaw = this->py[1] + (drseq(&w->rs) - .5) * LTURRET_VARIANCE;
	double pitch = this->py[0] + (drseq(&w->rs) - .5) * LTURRET_VARIANCE;
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
		Vec3d lturret_ofs(.005 * (i * 2 - 1), 0, -0.030);
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
	blowbackspeed += .05;
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









LMissileTurret::LMissileTurret(Game *game) : st(game), deploy(0){
}

LMissileTurret::LMissileTurret(Entity *abase, const hardpoint_static *hp) : st(abase, hp), deploy(0){
	ammo = 6;
}

LMissileTurret::~LMissileTurret(){
}

const char *LMissileTurret::classname()const{return "LMissileTurret";}
const unsigned LMissileTurret::classid = registerClass("LMissileTurret", Conster<LMissileTurret>);
double LMissileTurret::getHitRadius()const{return .03;}

const double LMissileTurret::bscale = .0001 / 2.;

int LMissileTurret::wantsFollowTarget()const{
	return cooldown < 2. * getShootInterval();
}

void LMissileTurret::anim(double dt){
	st::anim(dt);
	if(target && cooldown < 2. * getShootInterval()){
		deploy = approach(deploy, 1., dt, 0.);
	}
	else{
		deploy = approach(deploy, 0., dt, 0.);
		py[0] = approach(py[0], 0., dt, 0.);
		py[0] = rangein(approach(py[0] + M_PI, 0. + M_PI, LTURRETROTSPEED * dt, 2 * M_PI) - M_PI, lturret_range[0][0], lturret_range[0][1]);
//		py[1] = approach(py[1], 0., dt, 0.);
	}
}

void LMissileTurret::clientUpdate(double dt){
	anim(dt);
}

double LMissileTurret::getBulletSpeed()const{return 1.;}
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
	mat.translatein(0., .01, 0.);
	double yaw = this->py[1] + (drseq(&w->rs) - .5) * LTURRET_VARIANCE;
	double pitch = this->py[0] + (drseq(&w->rs) - .5) * LTURRET_VARIANCE;
	const Vec3d barrelpos = bscale * Vec3d(0, 200, 0) * deploy;
	const Vec3d joint = bscale * Vec3d(0, 120, 60);
	mat2 = mat.roty(yaw);
	mat2.translatein(joint + barrelpos);
	mat = mat2.rotx(pitch);
	mat.translatein(-joint);
	mat.translatein(0., -.01, 0.);
	Quatd qrot = base->rot * hp->rot * Quatd(0, sin(yaw / 2.), 0, cos(yaw / 2.)) * Quatd(sin(pitch / 2.), 0, 0, cos(pitch / 2.));
	ammo--;
	{
		Vec3d lturret_ofs = bscale * Vec3d(80. * (ammo % 3 - 1), (40. + 40. + 80. * (ammo / 3)), 0);
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

