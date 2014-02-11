/** \file
 * \brief Implementation of Zeta Gundam class.
 */

#include "Zeta.h"
#include "draw/effects.h"
#include "CoordSys.h"
#include "war.h"
//#include "arms.h"
#include "shield.h"
#include "Player.h"
#include "BeamProjectile.h"
#include "ExplosiveBullet.h"
#include "judge.h"
#include "serial_util.h"
#include "Docker.h"
#include "draw/material.h"
#include "cmd.h"
#include "astrodraw.h"
#include "EntityCommand.h"
#include "btadapt.h"
#include "sqadapt.h"
#include "motion.h"
#include "glw/PopupMenu.h"
#include "tefpol3d.h"
#include "SqInitProcess-ex.h"
#include "audio/playSound.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/zip/UnZip.h>
#include <clib/timemeas.h>
}



/* color sequences */
extern const struct color_sequence cs_orangeburn, cs_shortburn;






/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289

double ZetaGundam::modelScale = 1./30000;
double ZetaGundam::defaultMass = 4e3;
double ZetaGundam::maxHealthValue = 100.;
GLuint ZetaGundam::overlayDisp = 0;
HSQOBJECT ZetaGundam::sqPopupMenu = sq_nullobj();
HSQOBJECT ZetaGundam::sqCockpitView = sq_nullobj();
StaticBindDouble ZetaGundam::rotationSpeed = (.3 * M_PI);
StaticBindDouble ZetaGundam::maxAngleSpeed = (M_PI * .5);
StaticBindDouble ZetaGundam::maxFuel = 300;
StaticBindDouble ZetaGundam::fuelRegenRate = 10.; ///< Units per second
StaticBindDouble ZetaGundam::cooldownTime = 1.;
StaticBindDouble ZetaGundam::reloadTime = 5.;
StaticBindDouble ZetaGundam::rifleDamage = 25.;
StaticBindInt ZetaGundam::rifleMagazineSize = 5;
StaticBindDouble ZetaGundam::grenadeCoolDownTime = 1.;
StaticBindDouble ZetaGundam::grenadeReloadTime = 5.;
StaticBindDouble ZetaGundam::grenadeDamage = 100.;
StaticBindInt ZetaGundam::grenadeMagazineSize = 2;
StaticBindDouble ZetaGundam::vulcanCooldownTime = .1;
StaticBindDouble ZetaGundam::vulcanReloadTime = 5.;
StaticBindDouble ZetaGundam::vulcanDamage = 5.;
StaticBindInt ZetaGundam::vulcanMagazineSize = 20;




/* color sequences */
//extern const struct color_sequence cs_orangeburn, cs_shortburn;

// Height 20.5m
struct HitBoxList ZetaGundam::hitboxes;/*[] = {
	hitbox(Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(.005, .01025, .005)),
};
const int ZetaGundam::nhitboxes = numof(ZetaGundam::hitboxes);*/

struct HitBox ZetaGundam::waveRiderHitboxes[] = {
	hitbox(Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(.005, .005, .005)),
};
const int ZetaGundam::nWaveRiderHitboxes = numof(ZetaGundam::waveRiderHitboxes);


/*static const struct hitbox sceptor_hb[] = {
	hitbox(Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(.005, .002, .003)),
};*/

const char *ZetaGundam::idname()const{
	return "ZetaGundam";
}

const char *ZetaGundam::classname()const{
	return "ZetaGundam";
}


typedef std::map<gltestp::dstring, StaticBind *> StaticBindSet;

SQInteger ZetaGundam::sqf_get(HSQUIRRELVM v){
	StaticBindSet *p;
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	if(SQ_FAILED(sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL)))
		return SQ_ERROR;
	StaticBindSet::iterator it = p->find(wcs);
	if(it != p->end()){
		it->second->push(v);
		return 1;
	}
	else
		return -1;
}

static SQInteger sqf_set(HSQUIRRELVM v){
	StaticBindSet *p;
	const SQChar *wcs;
	if(SQ_FAILED(sq_getstring(v, 2, &wcs)))
		return SQ_ERROR;
	if(SQ_FAILED(sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL)))
		return SQ_ERROR;
	StaticBindSet::iterator it = p->find(wcs);
	if(it != p->end()){
		it->second->set(v);
		return 1;
	}
	else
		return -1;
}

static SQInteger sqf_nexti(HSQUIRRELVM v){
	StaticBindSet *p;
	const SQChar *wcs;

	if(SQ_FAILED(sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL)))
		return SQ_ERROR;

	// First iteration
	if(sq_gettype(v, 2) == OT_NULL){
		sq_pushstring(v, p->begin()->first, -1);
		return 1;
	}

	// Next iteration
	if(SQ_FAILED(sq_getstring(v, 2, &wcs)))
		return SQ_ERROR;
	StaticBindSet::iterator it = p->find(wcs);
	if(it != p->end()){
		it++; // next
		if(it != p->end()){
			sq_pushstring(v, it->first, -1);
			return 1;
		}
	}
	return 0;
}

HSQUIRRELVM ZetaGundam::sqvm;

static StaticBindSet staticBind;

template<> void Entity::EntityRegister<ZetaGundam>::sq_defineInt(HSQUIRRELVM v){
	sqa::StackReserver sr(v);
	ZetaGundam::sqvm = v;
	sq_pushroottable(v); // ZetaGundam-class root
	sq_newclass(v, SQFalse); // class
	sq_pushstring(v, _SC("_get"), -1); // class "_get"
	sq_newclosure(v, ZetaGundam::sqf_get, 0); // class "_get" sqf_get
	sq_newslot(v, -3, SQFalse); // class
	sq_pushstring(v, _SC("_set"), -1); // class "_set"
	sq_newclosure(v, sqf_set, 0); // class "_set" sqf_set
	sq_newslot(v, -3, SQFalse); // class
	sq_pushstring(v, _SC("_nexti"), -1);
	sq_newclosure(v, sqf_nexti, 0);
	sq_newslot(v, -3, SQFalse);
	sq_pushstring(v, _SC("StaticBind"), -1); // class "StaticBind"
	sq_push(v, -2); // class "StaticBind" class
	sq_createslot(v, -4); // root class
	sq_createinstance(v, -1); // root class instance
	sq_setinstanceup(v, -1, SQUserPointer(&staticBind)); // root class instance

	staticBind["rotationSpeed"] = &ZetaGundam::rotationSpeed;
	staticBind["maxAngleSpeed"] = &ZetaGundam::maxAngleSpeed;
	staticBind["bulletSpeed"] = &ZetaGundam::bulletSpeed;
	staticBind["walkSpeed"] = &ZetaGundam::walkSpeed;
	staticBind["airMoveSpeed"] = &ZetaGundam::airMoveSpeed;
	staticBind["torqueAmount"] = &ZetaGundam::torqueAmount;
	staticBind["floorProximityDistance"] = &ZetaGundam::floorProximityDistance;
	staticBind["floorTouchDistance"] = &ZetaGundam::floorTouchDistance;
	staticBind["standUpTorque"] = &ZetaGundam::standUpTorque;
	staticBind["standUpFeedbackTorque"] = &ZetaGundam::standUpFeedbackTorque;
	staticBind["maxFuel"] = &ZetaGundam::maxFuel;
	staticBind["fuelRegenRate"] = &ZetaGundam::fuelRegenRate;
	staticBind["randomVibration"] = &ZetaGundam::randomVibration;
	staticBind["reloadTime"] = &ZetaGundam::reloadTime;
	staticBind["cooldownTime"] = &ZetaGundam::cooldownTime;
	staticBind["rifleDamage"] = &ZetaGundam::rifleDamage;
	staticBind["rifleMagazineSize"] = &ZetaGundam::rifleMagazineSize;
	staticBind["grenadeCoolDownTime"] = &ZetaGundam::grenadeCoolDownTime;
	staticBind["grenadeReloadTime"] = &ZetaGundam::grenadeReloadTime;
	staticBind["grenadeDamage"] = &ZetaGundam::grenadeDamage;
	staticBind["grenadeMagazineSize"] = &ZetaGundam::grenadeMagazineSize;
	staticBind["vulcanCooldownTime"] = &ZetaGundam::vulcanCooldownTime;
	staticBind["vulcanReloadTime"] = &ZetaGundam::vulcanReloadTime;
	staticBind["vulcanDamage"] = &ZetaGundam::vulcanDamage;
	staticBind["vulcanMagazineSize"] = &ZetaGundam::vulcanMagazineSize;

	sq_pushstring(v, _SC("set"), -1); // root ZetaGundam-class root class instance "set"
	sq_push(v, -2); // root ZetaGundam-class root class instance "set" instance
	sq_newslot(v, -6, SQTrue); // root ZetaGundam-class root class instance
}

const unsigned ZetaGundam::classid = registerClass("ZetaGundam", Conster<ZetaGundam>);
Entity::EntityRegister<ZetaGundam> ZetaGundam::entityRegister("ZetaGundam");

void ZetaGundam::serialize(SerializeContext &sc){
	st::serialize(sc);
}

void ZetaGundam::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
}

const char *ZetaGundam::dispname()const{
	return "ZetaGundam";
};

double ZetaGundam::getMaxHealth()const{
	return maxHealthValue;
}





ZetaGundam::ZetaGundam(Game *game) :
	st(game)
{
	init();
	muzzleFlash[0] = 0.;
	muzzleFlash[1] = 0.;
	for(int i = 0; i < numof(thrusterDirs); i++){
		thrusterDirs[i] = thrusterDir[i];
		thrusterPower[i] = 0.;
	}
}

ZetaGundam::ZetaGundam(WarField *aw) : st(aw)
{
	init();
	aimdir[0] = 0.;
	aimdir[1] = 0.;
	muzzleFlash[0] = 0.;
	muzzleFlash[1] = 0.;
	for(int i = 0; i < numof(thrusterDirs); i++){
		thrusterDirs[i] = thrusterDir[i];
		thrusterPower[i] = 0.;
	}
	ZetaGundam *const p = this;
//	EntityInit(ret, w, &SCEPTOR_s);
//	VECCPY(ret->pos, mother->st.st.pos);
	mass = defaultMass;
//	race = mother->st.st.race;
	health = getMaxHealth();
	p->aac.clear();
	memset(p->thrusts, 0, sizeof p->thrusts);
	p->throttle = .5;
	WarSpace *ws;
	if(w && (ws = (WarSpace*)w))
		p->pf = ws->tepl->addTefpolMovable(this->pos, this->velo, avec3_000, &cs_orangeburn, TEP3_THICK | TEP3_ROUGH, cs_orangeburn.t);
	else
		p->pf = NULL;
//	p->mother = mother;
	p->hitsound = -1;
	p->docked = false;
//	p->paradec = mother->paradec++;
	p->fcloak = 0.;
	p->cloak = 0;
	p->heat = 0.;
	integral[0] = integral[1] = 0.;
}

void ZetaGundam::init(){
	st::init();
	static bool initialized = false;
	if(!initialized){
		// Load staticBind names from initialization script, too.
		// We could write down all StaticBind entries here, but we prefer methods that harder to mistake.
		// For instance, we can easily imagine a case that adding an entry in StaticBind list above and forget
		// to add corresponding entry here.
		std::vector<std::auto_ptr<SqInitProcess> > processes;
		for(StaticBindSet::iterator it = staticBind.begin(); it != staticBind.end(); ++it){
			if(StaticBindDouble *sbd = dynamic_cast<StaticBindDouble*>(it->second))
				processes.push_back(std::auto_ptr<SqInitProcess>(new SingleDoubleProcess(*sbd, it->first, false)));
		}

		// Link adjacent SqInitProcess entries to form a chain.
		for(int i = 0; i < processes.size()-1; ++i)
			processes[i+1]->chain(*processes[i]);

		SqInit(game->sqvm, modPath() << _SC("models/ZetaGundam.nut"),
			SingleDoubleProcess(modelScale, "modelScale") <<=
			SingleDoubleProcess(defaultMass, "mass") <<=
			SingleDoubleProcess(maxHealthValue, "maxhealth", false) <<=
			HitboxProcess(hitboxes) <<=
			DrawOverlayProcess(overlayDisp) <<=
			SqCallbackProcess(sqPopupMenu, "popupMenu") <<=
			SqCallbackProcess(sqCockpitView, "cockpitView") <<=
			*processes[0]); // Append dynamically constructed chain to the ordinary list.
		initialized = true;
	}
}

double ZetaGundam::motionInterpolateTime = 0.;
double ZetaGundam::motionInterpolateTimeAverage = 0.;
int ZetaGundam::motionInterpolateTimeAverageCount = 0;

void ZetaGundam::shootRifle(double dt){
	Vec3d velo, gunpos, velo0(0., 0., -bulletSpeed);
	Mat4d mat;
	int i = 0;
	WeaponStatus &wst = weaponStatus[0];
	if(dt <= wst.cooldown)
		return;

	// Retrieve muzzle position from model, but not the velocity
	{
		timemeas_t tm;
		TimeMeasStart(&tm);
		MotionPoseSet &v = motionInterpolate();

//		printf("motioninterp: %lg\n", TimeMeasLap(&tm));
		motionInterpolateTime = TimeMeasLap(&tm);
		if(model->getBonePos("riflemuzzle", v[0], &gunpos)){
			gunpos *= modelScale;
			gunpos[0] *= -1;
			gunpos[2] *= -1;
		}
		else
			gunpos = vec3_000;
//		YSDNM_MotionInterpolateFree(v);
		motionInterpolateFree(v);
//		delete pv;
	}

	transform(mat);
	{
		BeamProjectile *pb;
		double phi, theta;
		pb = new BeamProjectile(this, 3, rifleDamage);
		w->addent(pb);
		pb->pos = mat.vp3(gunpos);
		pb->velo = mat.dvp3(aimRot().trans(velo0));
		pb->velo += this->velo;
		this->heat += .025;
	}
#ifndef DEDICATED
	playSound3D(modPath() << "sound/beamrifle.ogg", this->pos, 1, 0.1, 0);
#endif
	if(0 < --wst.magazine)
		wst.cooldown += cooldownTime * (fuel <= 0 ? 3 : 1);
	else{
		reloadWeapon();
	}
	this->muzzleFlash[0] = .5;
}

void ZetaGundam::shootShieldBeam(double dt){
#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
	static const struct color_node cnl_shortburn[] = {
		{0.1, COLOR32RGBA(255,127,255,255)},
		{0.15, COLOR32RGBA(255,63,255,191)},
		{0.25, COLOR32RGBA(255,31,255,0)},
	};
	static const struct color_sequence cs_shortburn = DEFINE_COLSEQ(cnl_shortburn, (COLOR32)-1, 0.5);

	Vec3d velo, gunpos, velo0(0., 0., -0.25);
	Mat4d mat;
	int i = 0;
	WeaponStatus &wst = weaponStatus[1];
	if(dt <= wst.cooldown)
		return;

	// Retrieve muzzle position from model, but not the velocity
	MotionPoseSet &v = motionInterpolate();
	if(model->getBonePos("ZetaGundam_shieldmuzzle", v[0], &gunpos)){
		gunpos *= modelScale;
		gunpos[0] *= -1;
		gunpos[2] *= -1;
	}
	else
		gunpos = vec3_000;
//	YSDNM_MotionInterpolateFree(v);
//	delete pv;
	motionInterpolateFree(v);

	transform(mat);
	{
		ExplosiveBullet *pb;
		double phi, theta;
		pb = new ExplosiveBullet(this, 10, grenadeDamage, 0.050);
		w->addent(pb);
		pb->pos = mat.vp3(gunpos);
		pb->velo = mat.dvp3(aimRot().trans(velo0));
		pb->velo += this->velo;
		this->heat += .025;
	}
//	shootsound(pt, w, p->cooldown);
//	pt->shoots += 1;
	if(0 < --wst.magazine)
		wst.cooldown += grenadeCoolDownTime/*1. / 3.*/;
	else
		reloadWeapon();
	this->muzzleFlash[1] = .3;
}

void ZetaGundam::shootVulcan(double dt){
	Vec3d velo, gunpos[2], velo0(0., 0., -1.);
	Mat4d mat;
	WeaponStatus &wst = weaponStatus[2];

	// Cannot shoot in waverider form
	if(dt <= wst.cooldown || 0 < fwaverider)
		return;

	// Retrieve muzzle position from model, but not the velocity
	{
		MotionPoseSet v = motionInterpolate();
		for(int i = 0; i < 2; i++) if(model->getBonePos(i ? "rvulcan" : "lvulcan", v[0], &gunpos[i])){
			gunpos[i] *= modelScale;
			gunpos[i][0] *= -1;
			gunpos[i][2] *= -1;
		}
		else
			gunpos[i] = vec3_000;
//		YSDNM_MotionInterpolateFree(v)
//		delete pv;
		motionInterpolateFree(v);
	}

	transform(mat);
	for(int i = 0; i < 2; i++){
		Bullet *pb;
		double phi, theta;
		pb = new Bullet(this, 2, vulcanDamage);
		w->addent(pb);
		pb->pos = mat.vp3(gunpos[i]);
		pb->velo = mat.dvp3(aimRot().trans(velo0));
		pb->velo += this->velo;
		this->heat += .025;
	}

#ifndef DEDICATED
	// Prevent rapid fire from cluttering sound mixing buffer by limiting minimum interval of
	// addition of sound source.
	if(vulcanSoundCooldown < dt){
		playSound3D(modPath() << "sound/vulcan.ogg", this->pos, 0.5, 0.1, 0);
		vulcanSoundCooldown += 0.5;
	}
#endif

//	shootsound(pt, w, p->cooldown);
//	pt->shoots += 2;
	if(0 < --wst.magazine)
		wst.cooldown += vulcanCooldownTime;
	else
		reloadWeapon();
	this->muzzleFlash[2] = .1;
}





Quatd ZetaGundam::aimRot()const{
	return Quatd::rotation(aimdir[1], 0, -1, 0) * Quatd::rotation(aimdir[0], -1, 0, 0);
}

btCompoundShape *ZetaGundam::shape = NULL;
btCompoundShape *ZetaGundam::waveRiderShape = NULL;

void ZetaGundam::enterField(WarField *target){
	WarSpace *ws = *target;
	if(ws && ws->bdw){
		if(!bbody){
			if(!shape){
				shape = new btCompoundShape();
				for(int i = 0; i < hitboxes.size(); i++){
					const Vec3d &sc = hitboxes[i].sc;
					const Quatd &rot = hitboxes[i].rot;
					const Vec3d &pos = hitboxes[i].org;
					btBoxShape *box = new btBoxShape(btvc(sc));
					btTransform trans = btTransform(btqc(rot), btvc(pos));
					shape->addChildShape(trans, box);
				}
			}
			if(!waveRiderShape){
				waveRiderShape = new btCompoundShape();
				for(int i = 0; i < nWaveRiderHitboxes; i++){
					const Vec3d &sc = waveRiderHitboxes[i].sc;
					const Quatd &rot = waveRiderHitboxes[i].rot;
					const Vec3d &pos = waveRiderHitboxes[i].org;
					btBoxShape *box = new btBoxShape(btvc(sc));
					btTransform trans = btTransform(btqc(rot), btvc(pos));
					waveRiderShape->addChildShape(trans, box);
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
	//		rbInfo.m_angularDamping = .5;
			bbody = new btRigidBody(rbInfo);

	//		bbody->setSleepingThresholds(.0001, .0001);
		}

		//add the body to the dynamics world
		ws->bdw->addRigidBody(bbody, 1, ~2);
	}
}

// Docking and undocking will never stack.
bool ZetaGundam::solid(const Entity *o)const{
	return !(task == Undock || task == Dock);
}

int ZetaGundam::takedamage(double damage, int hitpart){
	int ret = 1;
	Teline3List *tell = w->getTeline3d();
	if(this->health < 0.)
		return 1;
//	this->hitsound = playWave3D("hit.wav", this->pos, game->player->getpos(), vec3_000, 1., .01, w->realtime);
	if(0 < health && health - damage <= 0){
		int i;
		ret = 0;
/*		effectDeath(w, pt);*/
		if(tell) for(i = 0; i < 32; i++){
			double pos[3], velo[3];
			velo[0] = drseq(&w->rs) - .5;
			velo[1] = drseq(&w->rs) - .5;
			velo[2] = drseq(&w->rs) - .5;
			VECNORMIN(velo);
			VECCPY(pos, this->pos);
			VECSCALEIN(velo, .1);
			VECSADD(pos, velo, .1);
			AddTeline3D(tell, pos, velo, .005, quat_u, vec3_000, w->accel(this->pos, this->velo), COLOR32RGBA(255, 31, 0, 255), TEL3_HEADFORWARD | TEL3_THICK | TEL3_FADEEND | TEL3_REFLECT, 1.5 + drseq(&w->rs));
		}
/*		((SCEPTOR_t*)pt)->pf = AddTefpolMovable3D(w->tepl, pt->pos, pt->velo, nullvec3, &cs_firetrail, TEP3_THICKER | TEP3_ROUGH, cs_firetrail.t);*/
//		((SCEPTOR_t*)pt)->hitsound = playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
		health = -2.; // Death animation timer
//		pt->deaths++;
	}
	else
		health -= damage;
	return ret;
}

double ZetaGundam::getHitRadius()const{
	return .01;
}

int ZetaGundam::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	double sc[3];
	double best = dt, retf;
	int reti = 0, n;
	for(n = 0; n < hitboxes.size(); n++){
		Vec3d org;
		Quatd rot;
		org = this->rot.itrans(hitboxes[n].org) + this->pos;
		rot = this->rot * hitboxes[n].rot;
		for(int j = 0; j < 3; j++)
			sc[j] = hitboxes[n].sc[j] + rad;
		if((jHitBox(org, sc, rot, src, dir, 0., best, &retf, retp, retn)) && (retf < best)){
			best = retf;
			if(ret) *ret = retf;
			reti = n + 1;
		}
	}
	return reti;
}

double ZetaGundam::maxfuel()const{
	return maxFuel;
}

int ZetaGundam::getWeaponCount()const{
	return 4;
}

bool ZetaGundam::getWeaponParams(int weapon, WeaponParams &param)const{
	// All weapons have infinite ammo by default.
	param.maxAmmo = 0;
	switch(weapon){
	case 0:
		param.name = "Beam Rifle";
		param.coolDownTime = cooldownTime;
		param.reloadTime = reloadTime;
		param.magazineSize = rifleMagazineSize;
		return true;
	case 1:
		param.name = "Grenade Launcher";
		param.coolDownTime = grenadeCoolDownTime;
		param.reloadTime = grenadeReloadTime;
		param.magazineSize = grenadeMagazineSize;
		return true;
	case 2:
		param.name = "Vulcan";
		param.coolDownTime = vulcanCooldownTime;
		param.reloadTime = vulcanReloadTime;
		param.magazineSize = vulcanMagazineSize;
		return true;
	case 3:
		param.name = "Beam Sabre";
		param.coolDownTime = 1;
		param.reloadTime = 1;
		param.magazineSize = 0;
		return true;
	}
	return false;
}
