/** \file
 * \brief Implementation of ReZEL class.
 */

#include "ReZEL.h"
#include "draw/effects.h"
#include "CoordSys.h"
#include "war.h"
//#include "arms.h"
#include "shield.h"
#include "Player.h"
#include "BeamProjectile.h"
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

double ReZEL::modelScale = 1./30000;
double ReZEL::defaultMass = 4e3;
double ReZEL::maxHealthValue = 100.;
GLuint ReZEL::overlayDisp = 0;
HSQOBJECT ReZEL::sqPopupMenu = sq_nullobj();
HSQOBJECT ReZEL::sqCockpitView = sq_nullobj();
StaticBindDouble ReZEL::rotationSpeed = (.3 * M_PI);
StaticBindDouble ReZEL::maxAngleSpeed = (M_PI * .5);
StaticBindDouble ReZEL::maxFuel = 300;
StaticBindDouble ReZEL::fuelRegenRate = 10.; ///< Units per second
StaticBindDouble ReZEL::cooldownTime = 1.;
StaticBindDouble ReZEL::reloadTime = 5.;
StaticBindDouble ReZEL::rifleDamage = 25.;
StaticBindInt ReZEL::rifleMagazineSize = 5;
StaticBindDouble ReZEL::shieldBeamCooldownTime = 1. / 3.;
StaticBindDouble ReZEL::shieldBeamReloadTime = 1.;
StaticBindDouble ReZEL::shieldBeamDamage = 10.;
StaticBindInt ReZEL::shieldBeamMagazineSize = 3;
StaticBindDouble ReZEL::vulcanCooldownTime = .1;
StaticBindDouble ReZEL::vulcanReloadTime = 5.;
StaticBindDouble ReZEL::vulcanDamage = 5.;
StaticBindInt ReZEL::vulcanMagazineSize = 20;




/* color sequences */
//extern const struct color_sequence cs_orangeburn, cs_shortburn;

// Height 20.5m
struct HitBoxList ReZEL::hitboxes;/*[] = {
	hitbox(Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(.005, .01025, .005)),
};
const int ReZEL::nhitboxes = numof(ReZEL::hitboxes);*/

struct HitBox ReZEL::waveRiderHitboxes[] = {
	hitbox(Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(.005, .005, .005)),
};
const int ReZEL::nWaveRiderHitboxes = numof(ReZEL::waveRiderHitboxes);


/*static const struct hitbox sceptor_hb[] = {
	hitbox(Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(.005, .002, .003)),
};*/

const char *ReZEL::idname()const{
	return "ReZEL";
}

const char *ReZEL::classname()const{
	return "ReZEL";
}


typedef std::map<gltestp::dstring, StaticBind *> StaticBindSet;

SQInteger ReZEL::sqf_get(HSQUIRRELVM v){
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

SQInteger sqf_set(HSQUIRRELVM v){
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

SQInteger sqf_nexti(HSQUIRRELVM v){
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

HSQUIRRELVM ReZEL::sqvm;

StaticBindSet staticBind;

template<> void Entity::EntityRegister<ReZEL>::sq_defineInt(HSQUIRRELVM v){
	sqa::StackReserver sr(v);
	ReZEL::sqvm = v;
	sq_pushroottable(v); // ReZEL-class root
	sq_newclass(v, SQFalse); // class
	sq_pushstring(v, _SC("_get"), -1); // class "_get"
	sq_newclosure(v, ReZEL::sqf_get, 0); // class "_get" sqf_get
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

	staticBind["rotationSpeed"] = &ReZEL::rotationSpeed;
	staticBind["maxAngleSpeed"] = &ReZEL::maxAngleSpeed;
	staticBind["bulletSpeed"] = &ReZEL::bulletSpeed;
	staticBind["walkSpeed"] = &ReZEL::walkSpeed;
	staticBind["airMoveSpeed"] = &ReZEL::airMoveSpeed;
	staticBind["torqueAmount"] = &ReZEL::torqueAmount;
	staticBind["floorProximityDistance"] = &ReZEL::floorProximityDistance;
	staticBind["floorTouchDistance"] = &ReZEL::floorTouchDistance;
	staticBind["standUpTorque"] = &ReZEL::standUpTorque;
	staticBind["standUpFeedbackTorque"] = &ReZEL::standUpFeedbackTorque;
	staticBind["maxFuel"] = &ReZEL::maxFuel;
	staticBind["fuelRegenRate"] = &ReZEL::fuelRegenRate;
	staticBind["randomVibration"] = &ReZEL::randomVibration;
	staticBind["reloadTime"] = &ReZEL::reloadTime;
	staticBind["cooldownTime"] = &ReZEL::cooldownTime;
	staticBind["rifleDamage"] = &ReZEL::rifleDamage;
	staticBind["rifleMagazineSize"] = &ReZEL::rifleMagazineSize;
	staticBind["shieldBeamCooldownTime"] = &ReZEL::shieldBeamCooldownTime;
	staticBind["shieldBeamReloadTime"] = &ReZEL::shieldBeamReloadTime;
	staticBind["shieldBeamDamage"] = &ReZEL::shieldBeamDamage;
	staticBind["shieldBeamMagazineSize"] = &ReZEL::shieldBeamMagazineSize;
	staticBind["vulcanCooldownTime"] = &ReZEL::vulcanCooldownTime;
	staticBind["vulcanReloadTime"] = &ReZEL::vulcanReloadTime;
	staticBind["vulcanDamage"] = &ReZEL::vulcanDamage;
	staticBind["vulcanMagazineSize"] = &ReZEL::vulcanMagazineSize;

	sq_pushstring(v, _SC("set"), -1); // root ReZEL-class root class instance "set"
	sq_push(v, -2); // root ReZEL-class root class instance "set" instance
	sq_newslot(v, -6, SQTrue); // root ReZEL-class root class instance
}

const unsigned ReZEL::classid = registerClass("ReZEL", Conster<ReZEL>);
Entity::EntityRegister<ReZEL> ReZEL::entityRegister("ReZEL");

void ReZEL::serialize(SerializeContext &sc){
	st::serialize(sc);
}

void ReZEL::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
}

const char *ReZEL::dispname()const{
	return "ReZEL";
};

double ReZEL::getMaxHealth()const{
	return maxHealthValue;
}





ReZEL::ReZEL(Game *game) : 
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

ReZEL::ReZEL(WarField *aw) : st(aw)
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
	ReZEL *const p = this;
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

void ReZEL::init(){
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

		SqInit(game->sqvm, modPath() << _SC("models/ReZEL.nut"),
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

double ReZEL::motionInterpolateTime = 0.;
double ReZEL::motionInterpolateTimeAverage = 0.;
int ReZEL::motionInterpolateTimeAverageCount = 0;

void ReZEL::shootRifle(double dt){
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
		if(model->getBonePos("ReZEL_riflemuzzle", v[0], &gunpos)){
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

void ReZEL::shootShieldBeam(double dt){
#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
	static const struct color_node cnl_shortburn[] = {
		{0.1, COLOR32RGBA(255,127,255,255)},
		{0.15, COLOR32RGBA(255,63,255,191)},
		{0.25, COLOR32RGBA(255,31,255,0)},
	};
	static const struct color_sequence cs_shortburn = DEFINE_COLSEQ(cnl_shortburn, (COLOR32)-1, 0.5);

	Vec3d velo, gunpos, velo0(0., 0., -1.5);
	Mat4d mat;
	int i = 0;
	WeaponStatus &wst = weaponStatus[1];
	if(dt <= wst.cooldown)
		return;

	// Retrieve muzzle position from model, but not the velocity
	MotionPoseSet &v = motionInterpolate();
	if(model->getBonePos("ReZEL_shieldmuzzle", v[0], &gunpos)){
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
		BeamProjectile *pb;
		double phi, theta;
		pb = new BeamProjectile(this, 3, shieldBeamDamage, .005, Vec4<unsigned char>(255,31,255,255), cs_shortburn);
		w->addent(pb);
		pb->pos = mat.vp3(gunpos);
		pb->velo = mat.dvp3(aimRot().trans(velo0));
		pb->velo += this->velo;
		this->heat += .025;
	}
//	shootsound(pt, w, p->cooldown);
//	pt->shoots += 1;
	if(0 < --wst.magazine)
		wst.cooldown += shieldBeamCooldownTime/*1. / 3.*/;
	else
		reloadWeapon();
	this->muzzleFlash[1] = .3;
}

void ReZEL::shootVulcan(double dt){
	Vec3d velo, gunpos[2], velo0(0., 0., -1.);
	Mat4d mat;
	WeaponStatus &wst = weaponStatus[2];

	// Cannot shoot in waverider form
	if(dt <= wst.cooldown || 0 < fwaverider)
		return;

	// Retrieve muzzle position from model, but not the velocity
	{
		MotionPoseSet v = motionInterpolate();
		for(int i = 0; i < 2; i++) if(model->getBonePos(i ? "ReZEL_rvulcan" : "ReZEL_lvulcan", v[0], &gunpos[i])){
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


#if 1
static int space_collide_callback(const struct otjEnumHitSphereParam *param, Entity *pt){
	Entity *pt2 = (Entity*)param->hint;
	if(pt == pt2)
		return 0;
	const double &dt = param->dt;
	Vec3d dr = pt->pos - pt2->pos;
	double r = pt->getHitRadius(), r2 = pt2->getHitRadius();
	double sr = (r + r2) * (r + r2);
	if(r * 2. < r2){
		if(!pt2->tracehit(pt->pos, pt->velo, r, dt, NULL, NULL, NULL))
			return 0;
	}
	else if(r2 * 2. < r){
		if(!pt->tracehit(pt2->pos, pt2->velo, r2, dt, NULL, NULL, NULL))
			return 0;
	}

	return 1;
}

static void space_collide_reflect(Entity *pt, const Vec3d &netvelo, const Vec3d &n, double f){
	Vec3d dv = pt->velo - netvelo;
	if(dv.sp(n) < 0)
		pt->velo = -n * dv.sp(n) + netvelo;
	else
		pt->velo += n * f;
}

static void space_collide_resolve(Entity *pt, Entity *pt2, double dt){
	const double ff = .2;
	Vec3d dr = pt->pos - pt2->pos;
	double sd = dr.slen();
	double r = pt->getHitRadius(), r2 = pt2->getHitRadius();
	double sr = (r + r2) * (r + r2);
	double f = ff * dt / (/*sd */1) * (pt2->mass < pt->mass ? pt2->mass / pt->mass : 1.);
	Vec3d n;

	// If either one of concerned Entities are not solid, no hit check is needed.
	if(!pt->solid(pt2) || !pt2->solid(pt))
		return;

	// If bounding spheres are not intersecting each other, no resolving is performed.
	// Object tree should discard such cases, but that must be ensured here when the tree
	// is yet to be built.
	if(sr < sd)
		return;

	if(r * 2. < r2){
		if(!pt2->tracehit(pt->pos, pt->velo, r, dt, NULL, NULL, &n))
			return;
	}
	else if(r2 * 2. < r){
		if(!pt->tracehit(pt2->pos, pt2->velo, r2, dt, NULL, NULL, &n))
			return;
		n *= -1;
	}
	else{
		n = dr.norm();
	}
	if(1. < f) /* prevent oscillation */
		f = 1.;

	// Aquire momentum of center of mass
	Vec3d netmomentum = pt->velo * pt->mass + pt2->velo * pt2->mass;

	// Aquire velocity of netmomentum, which is exact velocity when the colliding object stick together.
	Vec3d netvelo = netmomentum / (pt->mass + pt2->mass);

	// terminate closing velocity component
	space_collide_reflect(pt, netvelo, n, f * pt2->mass / (pt->mass + pt2->mass));
	space_collide_reflect(pt2, netvelo, -n, f * pt->mass / (pt->mass + pt2->mass));
}

void space_collide(Entity *pt, WarSpace *w, double dt, Entity *collideignore, Entity *collideignore2){
	Entity *pt2;
	if(1 && w->otroot){
		Entity *iglist[3] = {pt, collideignore, collideignore2};
//		struct entity_static *igvft[1] = {&rstation_s};
		struct otjEnumHitSphereParam param;
		param.root = w->otroot;
		param.src = &pt->pos;
		param.dir = &pt->velo;
		param.dt = dt;
		param.rad = pt->getHitRadius();
		param.pos = NULL;
		param.norm = NULL;
		param.flags = OTJ_IGVFT | OTJ_CALLBACK;
		param.callback = space_collide_callback;
/*		param.hint = iglist;*/
		param.hint = pt;
		param.iglist = iglist;
		param.niglist = 3;
//		param.igvft = igvft;
//		param.nigvft = 1;
		if(pt2 = otjEnumPointHitSphere(&param)){
			space_collide_resolve(pt, pt2, dt);
		}
	}
	else{
		for(WarField::EntityList::iterator it2 = w->el.begin(); it2 != w->el.end(); ++it2){
			pt2 = *it2;
			if(pt2 != pt && pt2 != collideignore && pt2 != collideignore2 && pt2->w == pt->w){
				space_collide_resolve(pt, pt2, dt);
			}
		}
	}
}
#endif


Quatd ReZEL::aimRot()const{
	return Quatd::rotation(aimdir[1], 0, -1, 0) * Quatd::rotation(aimdir[0], -1, 0, 0);
}

btCompoundShape *ReZEL::shape = NULL;
btCompoundShape *ReZEL::waveRiderShape = NULL;

void ReZEL::enterField(WarField *target){
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
bool ReZEL::solid(const Entity *o)const{
	return !(task == Undock || task == Dock);
}

int ReZEL::takedamage(double damage, int hitpart){
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

double ReZEL::getHitRadius()const{
	return .01;
}

int ReZEL::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
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

double ReZEL::maxfuel()const{
	return maxFuel;
}

int ReZEL::getWeaponCount()const{
	return 4;
}

bool ReZEL::getWeaponParams(int weapon, WeaponParams &param)const{
	// All weapons have infinite ammo by default.
	param.maxAmmo = 0;
	switch(weapon){
	case 0:
		param.name = "Beam Rifle";
		param.coolDownTime = this->cooldownTime;
		param.reloadTime = this->reloadTime;
		param.magazineSize = rifleMagazineSize;
		return true;
	case 1:
		param.name = "Shield Beam";
		param.coolDownTime = shieldBeamCooldownTime;
		param.reloadTime = shieldBeamReloadTime;
		param.magazineSize = shieldBeamMagazineSize;
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
