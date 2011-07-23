#include "draw/effects.h"
#include "ReZEL.h"
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
#include "glw/popup.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/wavsound.h>
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

//#define SCEPTOR_SCALE 1./10000
StaticBindDouble ReZEL::deathSmokeFreq = 20.; ///< Smokes per second
StaticBindDouble ReZEL::rotationSpeed = (.3 * M_PI);
StaticBindDouble ReZEL::maxAngleSpeed = (M_PI * .5);
StaticBindDouble ReZEL::bulletSpeed = 2.;
StaticBindDouble ReZEL::walkSpeed = .03;
StaticBindDouble ReZEL::airMoveSpeed = .2;
StaticBindDouble ReZEL::torqueAmount = .1;
StaticBindDouble ReZEL::floorProximityDistance = .05;
StaticBindDouble ReZEL::floorTouchDistance = .02;
StaticBindDouble ReZEL::standUpTorque = (6e-4 * 5e4);
StaticBindDouble ReZEL::standUpFeedbackTorque = 1e1;
StaticBindDouble ReZEL::maxFuel = 300;
StaticBindDouble ReZEL::fuelRegenRate = 10.; ///< Units per second
StaticBindDouble ReZEL::randomVibration = .15;
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


const Vec3d ReZEL::thrusterDir[7] = {
	Vec3d(0, -1, 0),
	Vec3d(0, -1, 0),
	Vec3d(0, -1, 0),
	Vec3d(1, 0, 0),
	Vec3d(-1, 0, 0),
	Vec3d(0, -1, 0),
	Vec3d(0, -1, 0),
};


Entity::Dockable *ReZEL::toDockable(){return this;}

/* color sequences */
//extern const struct color_sequence cs_orangeburn, cs_shortburn;

// Height 20.5m
struct hitbox ReZEL::hitboxes[] = {
	hitbox(Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(.005, .01025, .005)),
};
const int ReZEL::nhitboxes = numof(ReZEL::hitboxes);

struct hitbox ReZEL::waveRiderHitboxes[] = {
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

template<> bool Entity::EntityRegister<ReZEL>::sq_define(HSQUIRRELVM v){
	sqa::StackReserver sr(v);
	ReZEL::sqvm = v;
	sq_newclass(v, SQFalse); // class
	sq_pushstring(v, _SC("_get"), -1); // class "_get"
	sq_newclosure(v, ReZEL::sqf_get, 0); // class "_get" sqf_get
	sq_newslot(v, -3, SQFalse); // class
	sq_pushstring(v, _SC("_set"), -1); // class "_get"
	sq_newclosure(v, sqf_set, 0); // class "_get" sqf_get
	sq_newslot(v, -3, SQFalse); // class
	sq_pushstring(v, _SC("_nexti"), -1);
	sq_newclosure(v, sqf_nexti, 0);
	sq_newslot(v, -3, SQFalse);
	sq_pushstring(v, _SC("StaticBind"), -1); // class "StaticBind"
	sq_push(v, -2); // class "StaticBind" class
	sq_createslot(v, -4); // class
	sq_createinstance(v, -1); // class instance
	sq_setinstanceup(v, -1, SQUserPointer(&staticBind)); // class instance

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

	sq_pushstring(v, sq_classname(), -1);
	sq_pushstring(v, ReZEL::st::entityRegister.sq_classname(), -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, SQUserPointer(m_classid));
	sq_pushstring(v, _SC("set"), -1);
	sq_push(v, -4);
	sq_newslot(v, -3, SQTrue);
	sq_createslot(v, 1);
	return true;
}

const unsigned ReZEL::classid = registerClass("ReZEL", Conster<ReZEL>);
Entity::EntityRegister<ReZEL> ReZEL::entityRegister("ReZEL");

void ReZEL::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << aac; /* angular acceleration */
	sc.o << throttle;
	sc.o << fuel;
	sc.o << cooldown;
	sc.o << dest;
	sc.o << fcloak;
	sc.o << waverider;
	sc.o << fwaverider;
	sc.o << weapon;
	sc.o << magazine;
	sc.o << submagazine;
	sc.o << heat;
	sc.o << mother; // Mother ship
//	sc.o << hitsound;
	sc.o << paradec;
	sc.o << aimdir[0];
	sc.o << aimdir[1];
	sc.o << (int)task;
	sc.o << docked << returning << away << cloak << forcedEnemy;
	sc.o << stabilizer;
	sc.o << formPrev;
}

void ReZEL::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> aac; /* angular acceleration */
	sc.i >> throttle;
	sc.i >> fuel;
	sc.i >> cooldown;
	sc.i >> dest;
	sc.i >> fcloak;
	sc.i >> waverider;
	sc.i >> fwaverider;
	sc.i >> weapon;
	sc.i >> magazine;
	sc.i >> submagazine;
	sc.i >> heat;
	sc.i >> mother; // Mother ship
//	sc.i >> hitsound;
	sc.i >> paradec;
	sc.i >> aimdir[0];
	sc.i >> aimdir[1];
	sc.i >> (int&)task;
	sc.i >> docked >> returning >> away >> cloak >> forcedEnemy;
	sc.i >> stabilizer;
	sc.i >> formPrev;

	// Re-create temporary entities if flying in a WarSpace. If environment is a WarField, don't restore.
	WarSpace *ws;
	if(w && (ws = (WarSpace*)w))
		pf = AddTefpolMovable3D(ws->tepl, this->pos, this->velo, avec3_000, &cs_orangeburn, TEP3_THICK | TEP3_ROUGH, cs_orangeburn.t);
	else
		pf = NULL;
}

const char *ReZEL::dispname()const{
	return "ReZEL";
};

double ReZEL::maxhealth()const{
	return 100.;
}





ReZEL::ReZEL() : 
	mother(NULL),
	paradec(-1),
	vulcancooldown(0.f),
	vulcanmag(vulcanMagazineSize),
	twist(0.f),
	pitch(0.f),
	freload(0.f),
	fsabre(0.f),
	fonfeet(0.f),
	walkphase(0.f)
{
	muzzleFlash[0] = 0.;
	muzzleFlash[1] = 0.;
	for(int i = 0; i < numof(thrusterDirs); i++){
		thrusterDirs[i] = thrusterDir[i];
		thrusterPower[i] = 0.;
	}
}

ReZEL::ReZEL(WarField *aw) : st(aw),
	mother(NULL),
	task(Auto),
	standingUp(false),
	fuel(maxfuel()),
	reverser(0),
	waverider(false),
	fwaverider(0.),
	weapon(0),
	fweapon(0.),
	vulcancooldown(0.f),
	vulcanmag(vulcanMagazineSize),
	submagazine(3),
	twist(0.f),
	pitch(0.f),
	freload(0.f),
	paradec(-1),
	forcedEnemy(false),
	formPrev(NULL),
	evelo(vec3_000),
	attitude(Passive),
	fsabre(0.f),
	fonfeet(0.f),
	walkphase(0.f),
	stabilizer(true)
{
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
	mass = 4e3;
//	race = mother->st.st.race;
	health = maxhealth();
	p->aac.clear();
	memset(p->thrusts, 0, sizeof p->thrusts);
	p->throttle = .5;
	p->cooldown = 1.;
	WarSpace *ws;
	if(w && (ws = (WarSpace*)w))
		p->pf = AddTefpolMovable3D(ws->tepl, this->pos, this->velo, avec3_000, &cs_orangeburn, TEP3_THICK | TEP3_ROUGH, cs_orangeburn.t);
	else
		p->pf = NULL;
//	p->mother = mother;
	p->hitsound = -1;
	p->docked = false;
//	p->paradec = mother->paradec++;
	p->magazine = rifleMagazineSize;
	p->fcloak = 0.;
	p->cloak = 0;
	p->heat = 0.;
	integral[0] = integral[1] = 0.;
}

void ReZEL::cockpitView(Vec3d &pos, Quatd &q, int seatid)const{
//	Player *ppl = w->pl;
	Vec3d ofs;
	static const Vec3d src[4] = {
		Vec3d(0., .001, -.002),
		Vec3d(0., .018,  .035),
		Vec3d(0., .018,  .035),
		Vec3d(0.008, .007,  .013),
	};
	Mat4d mat;
	Player *pl = w->getPlayer();
	seatid = (seatid + 4) % 4;
	if(seatid == 2 && enemy && enemy->w == w && pl){
		q = this->rot * Quatd::direction(this->rot.cnj().trans(this->pos - enemy->pos));
		ofs = q.trans(Vec3d(src[seatid][0], src[seatid][1], src[seatid][2] / pl->fov)); // Trackback if zoomed
	}
	else{
		q = this->rot * aimRot();
		ofs = q.trans(src[seatid]);
	}
	pos = this->pos + ofs;
}

void ReZEL::control(const input_t *inputs, double dt){
	if(!w || health <= 0.)
		return;
	this->inputs = *inputs;
	if(inputs->change & PL_RCLICK)
		weapon = (weapon + 1) % 4;
	if(inputs->change & PL_MWU)
		weapon = (weapon + 1) % 4;
	else if(inputs->change & PL_MWD)
		weapon = (weapon - 1 + 4) % 4;
}

int ReZEL::popupMenu(PopupMenu &list){
	int ret = st::popupMenu(list);
	list.append("Transform to Waverider", 0, "sq \"foreachselectedents(function(e){e.command(\\\"Transform\\\", 1);})\"");
	list.append("Transform to Fighter", 0, "sq \"foreachselectedents(function(e){e.command(\\\"Transform\\\", 0);})\"");
	list.appendSeparator();
	list.append("Arm Beam Rifle", 0, "sq \"foreachselectedents(function(e){e.command(\\\"Weapon\\\", 0);})\"");
	list.append("Arm Shield Beam", 0, "sq \"foreachselectedents(function(e){e.command(\\\"Weapon\\\", 1);})\"");
	list.append("Arm Vulcan", 0, "sq \"foreachselectedents(function(e){e.command(\\\"Weapon\\\", 2);})\"");
	list.append("Arm Beam Sabre", 0, "sq \"foreachselectedents(function(e){e.command(\\\"Weapon\\\", 3);})\"");
	list.appendSeparator();
	list.append("Turn on Stabilizer", 0, "sq \"foreachselectedents(function(e){e.command(\\\"Stabilizer\\\", 1);})\"");
	list.append("Turn off Stabilizer", 0, "sq \"foreachselectedents(function(e){e.command(\\\"Stabilizer\\\", 0);})\"");
/*	list.append(sqa_translate("Dock"), 0, "dock")
		.append(sqa_translate("Military Parade Formation"), 0, "parade_formation")
		.append(sqa_translate("Cloak"), 0, "cloak")
		.append(sqa_translate("Delta Formation"), 0, "delta_formation");*/
	return ret;
}

Entity::Props ReZEL::props()const{
	Props ret = st::props();
	ret.push_back(gltestp::dstring("Task: ") << task);
	ret.push_back(gltestp::dstring("Fuel: ") << fuel << '/' << maxfuel());
	ret.push_back(gltestp::dstring("Throttle: ") << throttle);
	ret.push_back(gltestp::dstring("Stabilizer: ") << stabilizer);
	return ret;
}

bool ReZEL::undock(Docker *d){
	st::undock(d);
	task = Undock;
	mother = d;
	if(this->pf)
		ImmobilizeTefpol3D(this->pf);
	WarSpace *ws;
	if(w && w->getTefpol3d())
		this->pf = AddTefpolMovable3D(w->getTefpol3d(), this->pos, this->velo, avec3_000, &cs_orangeburn, TEP3_THICK | TEP3_ROUGH, cs_orangeburn.t);
	d->baycool += 1.;
	return true;
}

/*
void cmd_cloak(int argc, char *argv[]){
	extern struct player *ppl;
	if(ppl->cs->w){
		entity_t *pt;
		for(pt = ppl->selected; pt; pt = pt->selectnext) if(pt->vft == &SCEPTOR_s){
			((SCEPTOR_t*)pt)->cloak = !((SCEPTOR_t*)pt)->cloak;
		}
	}
}
*/

double ReZEL::motionInterpolateTime = 0.;

void ReZEL::shootRifle(double dt){
	Vec3d velo, gunpos, velo0(0., 0., -bulletSpeed);
	Mat4d mat;
	int i = 0;
	if(dt <= cooldown)
		return;

	// Retrieve muzzle position from model, but not the velocity
	{
		double motion_time[numof(motions)];
		getMotionTime(&motion_time);
		timemeas_t tm;
		TimeMeasStart(&tm);
		ysdnm_var *v = YSDNM_MotionInterpolate(motions, motion_time, numof(motions));
//		printf("motioninterp: %lg\n", TimeMeasLap(&tm));
		motionInterpolateTime = TimeMeasLap(&tm);
		if(model->getBonePos("ReZEL_riflemuzzle", *v, &gunpos)){
			gunpos *= sufscale;
			gunpos[0] *= -1;
			gunpos[2] *= -1;
		}
		else
			gunpos = vec3_000;
		YSDNM_MotionInterpolateFree(v);
	}

	transform(mat);
	{
		BeamProjectile *pb;
		double phi, theta;
		pb = new BeamProjectile(this, 5, rifleDamage);
		w->addent(pb);
		pb->pos = mat.vp3(gunpos);
		pb->velo = mat.dvp3(aimRot().trans(velo0));
		pb->velo += this->velo;
		pb->life = 3.;
		this->heat += .025;
	}
//	shootsound(pt, w, p->cooldown);
//	pt->shoots += 2;
	if(0 < --magazine)
		this->cooldown += cooldownTime * (fuel <= 0 ? 3 : 1);
	else{
		magazine = rifleMagazineSize;
		this->cooldown += reloadTime;
		this->freload = reloadTime;
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
	if(dt <= cooldown)
		return;

	// Retrieve muzzle position from model, but not the velocity
	double motion_time[numof(motions)];
	getMotionTime(&motion_time);
	ysdnm_var *v = YSDNM_MotionInterpolate(motions, motion_time, numof(motions));
	if(model->getBonePos("ReZEL_shieldmuzzle", *v, &gunpos)){
		gunpos *= sufscale;
		gunpos[0] *= -1;
		gunpos[2] *= -1;
	}
	else
		gunpos = vec3_000;
	YSDNM_MotionInterpolateFree(v);

	transform(mat);
	{
		BeamProjectile *pb;
		double phi, theta;
		pb = new BeamProjectile(this, 5, shieldBeamDamage, .005, Vec4<unsigned char>(255,31,255,255), cs_shortburn);
		w->addent(pb);
		pb->pos = mat.vp3(gunpos);
		pb->velo = mat.dvp3(aimRot().trans(velo0));
		pb->velo += this->velo;
		pb->life = 3.;
		this->heat += .025;
	}
//	shootsound(pt, w, p->cooldown);
//	pt->shoots += 1;
	if(0 < --submagazine)
		this->cooldown += shieldBeamCooldownTime/*1. / 3.*/;
	else{
		submagazine = shieldBeamMagazineSize;
		this->cooldown += shieldBeamReloadTime;
	}
	this->muzzleFlash[1] = .3;
}

void ReZEL::shootVulcan(double dt){
	Vec3d velo, gunpos[2], velo0(0., 0., -1.);
	Mat4d mat;

	// Cannot shoot in waverider form
	if(dt <= vulcancooldown || 0 < fwaverider)
		return;

	// Retrieve muzzle position from model, but not the velocity
	{
		double motion_time[numof(motions)];
		getMotionTime(&motion_time);
		ysdnm_var *v = YSDNM_MotionInterpolate(motions, motion_time, numof(motions));
		for(int i = 0; i < 2; i++) if(model->getBonePos(i ? "ReZEL_rvulcan" : "ReZEL_lvulcan", *v, &gunpos[i])){
			gunpos[i] *= sufscale;
			gunpos[i][0] *= -1;
			gunpos[i][2] *= -1;
		}
		else
			gunpos[i] = vec3_000;
		YSDNM_MotionInterpolateFree(v);
	}

	transform(mat);
	for(int i = 0; i < 2; i++){
		Bullet *pb;
		double phi, theta;
		pb = new Bullet(this, 5, vulcanDamage);
		w->addent(pb);
		pb->pos = mat.vp3(gunpos[i]);
		pb->velo = mat.dvp3(aimRot().trans(velo0));
		pb->velo += this->velo;
		pb->life = 2.;
		this->heat += .025;
	}
//	shootsound(pt, w, p->cooldown);
//	pt->shoots += 2;
	if(0 < --vulcanmag)
		this->vulcancooldown += vulcanCooldownTime;
	else{
		vulcanmag = vulcanMagazineSize;
		this->vulcancooldown += vulcanReloadTime;
	}
	this->muzzleFlash[2] = .1;
}


// find the nearest enemy
bool ReZEL::findEnemy(){
	if(forcedEnemy)
		return !!enemy;
	Entity *pt2, *closest = NULL;
	double best = 1e2 * 1e2;
	for(pt2 = w->el; pt2; pt2 = pt2->next){

		if(!(pt2->isTargettable() && pt2 != this && pt2->w == w && pt2->health > 0. && pt2->race != -1 && pt2->race != this->race))
			continue;

/*		if(!entity_visible(pb, pt2))
			continue;*/

		double sdist = (pt2->pos - this->pos).slen();
		if(sdist < best){
			best = sdist;
			closest = pt2;
		}
	}
	if(closest){
		enemy = closest;
		integral[0] = integral[1] = 0.f;
		evelo = vec3_000;
	}
	return !!closest;
}

#if 1
static int space_collide_callback(const struct otjEnumHitSphereParam *param, Entity *pt){
	Entity *pt2 = (Entity*)param->hint;
	if(pt == pt2)
		return 0;
	const double &dt = param->dt;
	Vec3d dr = pt->pos - pt2->pos;
	double r = pt->hitradius(), r2 = pt2->hitradius();
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
	double r = pt->hitradius(), r2 = pt2->hitradius();
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
		param.rad = pt->hitradius();
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
	else for(pt2 = w->el; pt2; pt2 = pt2->next) if(pt2 != pt && pt2 != collideignore && pt2 != collideignore2 && pt2->w == pt->w){
		space_collide_resolve(pt, pt2, dt);
	}
}
#endif



void ReZEL::steerArrival(double dt, const Vec3d &atarget, const Vec3d &targetvelo, double speedfactor, double minspeed){
	Vec3d target(atarget);
	Vec3d rdr = target - this->pos; // real dr
	Vec3d rdrn = rdr.norm();
	Vec3d dv = targetvelo - this->velo;
	Vec3d dvLinear = rdrn.sp(dv) * rdrn;
	Vec3d dvPlanar = dv - dvLinear;
	double dist = rdr.len();
	if(rdrn.sp(dv) < 0 && DBL_EPSILON < dvLinear.slen()) // estimate only when closing
		target += dvPlanar * dist / dvLinear.len();
	Vec3d dr = this->pos - target;
	this->throttle = rangein((dr.len() - dv.len() * 5.5) * speedfactor + minspeed, -1, 1);
	this->omg = 3 * this->rot.trans(vec3_001).vp(dr.norm());
	if(.9 < dr.norm().sp(-this->rot.trans(vec3_001)))
		this->omg += this->rot.trans(vec3_010);
	if(rotationSpeed * rotationSpeed < this->omg.slen())
		this->omg.normin().scalein(rotationSpeed);
	bbody->setAngularVelocity(btvc(this->omg));
	dr.normin();
	Vec3d sidevelo = velo - dr * dr.sp(velo);
	bbody->applyCentralForce(btvc(-sidevelo * mass));
//	this->rot = this->rot.quatrotquat(this->omg * dt);
}

// Find mother if has none
Entity *ReZEL::findMother(){
	Entity *pm = NULL;
	double best = 1e10 * 1e10, sl;
	for(Entity *e = w->entlist(); e; e = e->next) if(e->race == race && e->getDocker() && (sl = (e->pos - this->pos).slen()) < best){
		mother = e->getDocker();
		pm = mother->e;
		best = sl;
	}
	return pm;
}

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
				for(int i = 0; i < nhitboxes; i++){
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

void ReZEL::anim(double dt){
	WarField *oldw = w;
	Entity *pt = this;
	ReZEL *const pf = this;
	ReZEL *const p = this;
//	scarry_t *const mother = pf->mother;
	Entity *pm = mother && mother->e ? mother->e : NULL;
	Mat4d mat, imat;

	if(Docker *docker = *w){
		fuel = min(fuel + dt * 20., maxfuel()); // it takes 6 seconds to fully refuel
		health = min(health + dt * 100., maxhealth()); // it takes 7 seconds to be fully repaired
		if(fuel == maxfuel() && health == maxhealth() && !docker->remainDocked)
			docker->postUndock(this);
		return;
	}

	WarSpace *ws = *w;

	// Transit to another CoordSys when its border is reached.
	// Exceptions should be made to prevent greatly remote CoordSys to be transited, because more remote
	// results more floating point errors.
	{
		Vec3d pos;
		const CoordSys *cs = w->cs->belongcs(pos, this->pos);
		if(cs != w->cs){
			dest = cs->tocs(dest, w->cs);
			transit_cs(const_cast<CoordSys*>(cs));

			// The later calculation should be done in destination CoordSys, which is not here.
			return;
		}
	}

	// Do not follow remote mothership. Probably you'd better finding another mothership.
	if(mother != w && pm && pm->w != w){
		mother = NULL;
		pm = NULL;
	}

	if(bbody){
		const btTransform &tra = bbody->getCenterOfMassTransform();
		this->pos = btvc(tra.getOrigin());
		this->rot = btqc(tra.getRotation());
		this->velo = btvc(bbody->getLinearVelocity());
		this->omg = btvc(bbody->getAngularVelocity());
	}

/*	if(!mother){
		if(p->pf){
			ImmobilizeTefpol3D(p->pf);
			p->pf = NULL;
		}
		pt->active = 0;
		return;
	}*/

#if 1
	if(pf->pf)
		MoveTefpol3D(pf->pf, pt->pos + pt->rot.trans(Vec3d(0,0,.005)), avec3_000, cs_orangeburn.t, 0/*pf->docked*/);
#endif

#if 0
	if(pf->docked){
		if(pm->enemy && mother->baycool == 0.){
			static int counter = 0;
			static const avec3_t pos0[2] = {{-70. * SCARRY_SCALE, 30. * SCARRY_SCALE, 0.}, {-130. * SCARRY_SCALE, 30. * SCARRY_SCALE, 0.},};
			QUATCPY(pt->rot, pm->rot);
			quatrot(pt->pos, pm->rot, pos0[counter++ % 2]);
			VECADDIN(pt->pos, pm->pos);
			VECCPY(pt->velo, pm->velo);
			pf->docked = pf->returning = 0;
			p->task = SCEPTOR_undock;
			mother->baycool = SCARRY_BAYCOOL;
		}
		else{
			VECCPY(pt->pos, pm->pos);
			pt->pos[1] += .1;
			VECCPY(pt->velo, pm->velo);
			pt->race = pm->race;
			return;
		}
	}
#endif

	/* forget about beaten enemy */
	if(pt->enemy && pt->enemy->health <= 0.)
		pt->enemy = NULL;

	transform(mat);
//	quat2imat(imat, mat);

#if 0
	if(mother->st.enemy && 0 < pt->health && VECSLEN(pt->velo) < .1 * .1){
		double acc[3];
		double len;
		avec3_t acc0 = {0., 0., -0.025};
		amat4_t mat;
		pyrmat(pt->pyr, mat);
		MAT4VP3(acc, mat, acc0);
		len = drseq(&w->rs) + .5;
		VECSCALEIN(acc, len);
		VECSADD(pt->velo, acc, dt);
	/*	VECSADD(pt->velo, w->gravity, dt);*/
		VECSADD(pt->pos, pt->velo, dt);
		movesound3d(pf->hitsound, pt->pos);
		return;
	}
#endif

	{
		int i;
		for(i = 0; i < 2; i++){
			if(p->thrusts[0][i] < dt * 2.)
				p->thrusts[0][i] = 0.;
			else
				p->thrusts[0][i] -= dt * 2.;
		}
	}

//	if(pm)
//		pt->race = pm->race;

	bool floorTouched = false;
	bool floorProximity = false;

	if(0 < pt->health){
//		double oldyaw = pt->pyr[1];
		bool controlled = controller;
		int parking = 0;
		Entity *collideignore = NULL;

		// Default is to collide
		if(bbody)
			bbody->getBroadphaseProxy()->m_collisionFilterMask |= 2;

		// Clear inputs when not controlled
		if(!controlled)
			pt->inputs.press = 0;

//		btRigidBody *worldBody = ws->worldBody();
		double elevation = 0; // Cosine of angle of elevation relative to local acceleration
		Vec3d accel = w->accel(pos, vec3_000);
		btVector3 btaccel = btVector3(btvc(accel));
		btVector3 btdown = btaccel.normalized();

		// Check if we are touching the ground.
		if(!btaccel.isZero()) do{
			const btVector3 &btpos = bbody->getWorldTransform().getOrigin();
			const btVector3 &from = btpos;
			const btVector3 &btvelo = bbody->getLinearVelocity();
			btScalar closingSpeed = btvelo.dot(btdown);
			const btVector3 to = btpos + btaccel.normalized() * (closingSpeed < floorProximityDistance ? floorProximityDistance : closingSpeed);

			btCollisionWorld::ClosestRayResultCallback rayCallback(from, to);

			ws->bdw->rayTest(from, to, rayCallback);

			if(rayCallback.hasHit()){
				btRigidBody *body = btRigidBody::upcast(rayCallback.m_collisionObject);
				if(body && body->hasContactResponse() && body->isStaticObject()){
					floorProximity = true;
					btScalar hitDistance = (rayCallback.m_hitPointWorld - from).dot(btaccel.normalized());
					if(hitDistance < floorTouchDistance)
						floorTouched = true;
				}
			}

			// Query all contact pairs to find out if this object and the world's floor face is in contact.
			if(!floorTouched){
				int numManifolds = ws->bdw->getDispatcher()->getNumManifolds();
				for (int i=0;i<numManifolds;i++)
				{
					btPersistentManifold* contactManifold =  ws->bdw->getDispatcher()->getManifoldByIndexInternal(i);
					btCollisionObject* obA = static_cast<btCollisionObject*>(contactManifold->getBody0());
					btCollisionObject* obB = static_cast<btCollisionObject*>(contactManifold->getBody1());

					if(obA != bbody && obB != bbody)
						continue;
				
					int numContacts = contactManifold->getNumContacts();
					for (int j=0;j<numContacts;j++)
					{
						btManifoldPoint& pt = contactManifold->getContactPoint(j);
						if (pt.getDistance()<0.f)
						{
							const btVector3& ptA = pt.getPositionWorldOnA();
							const btVector3& ptB = pt.getPositionWorldOnB();
							const btVector3& normalOnB = pt.m_normalWorldOnB;

							// Assume static objects to be parts of the world.
							if(obA->isStaticObject() && obB == bbody && 0 < normalOnB.dot(btaccel) || obA == bbody && obB->isStaticObject() && normalOnB.dot(btaccel) < 0){
								floorTouched = true;
								break;
							}
						}
					}
				}
			}

			if(floorProximity){

				// If some acceleration, no matter gravitational or artificial, is present, we ought to stand againt it.
				if(DBL_EPSILON < accel.slen()){
					Vec3d downDir = accel.norm();
					elevation = rot.trans(Vec3d(0,0,-1)).sp(downDir);
					double uprightness = rot.trans(Vec3d(0,1,0)).sp(downDir);

					// Try to stand up no matter how you are upright.
					if(-.95 < uprightness)
						standingUp = true;
				}
			}
		} while(0);

		bbody->setFriction(floorTouched ? 0.05 : .5);

		if(standingUp && controlled){
			if(!btaccel.isZero()){
				const btVector3 &btOmega = bbody->getAngularVelocity();

				// Allow rotation around vertical axis by ignoring vertical component of the angular velocity.
				btVector3 lateralOmega = btOmega - btdown.dot(btOmega) * btdown;
				const btVector3 up = bbody->getWorldTransform().getBasis().getColumn(1);

				// We need to invert the tensor to determine which torque is required to get the head up.
				btMatrix3x3 inertiaTensor = bbody->getInvInertiaTensorWorld().inverse();
				bbody->applyTorque(btvc(-up.cross(btdown) * inertiaTensor * standUpTorque) - lateralOmega * inertiaTensor * standUpFeedbackTorque);

				// Directly modifying rotation transformation should be more stable (but cannot handle collisions, etc. correctly),
				// but the fact is that it didn't work.
//				bbody->setWorldTransform(btTransform(bbody->getWorldTransform().getRotation() * btQuaternion(-up.cross(btdown), dt), bbody->getWorldTransform().getOrigin()));

				// Quit standing up only if we're really upright and stable.
				if(up.dot(btdown) < -.99 && lateralOmega.length2() < .05 * .05)
					standingUp = false;
			}
			else
				standingUp = false;
		}

		if(false/*pf->docked*/);
		else if(p->task == Undockque){
			// Avoid collision with mother if in a progress of undocking.
			if(bbody)
				bbody->getBroadphaseProxy()->m_collisionFilterMask &= ~2;
/*			if(pm->w != w){
				if(p->pf){
					ImmobilizeTefpol3D(p->pf);
					p->pf = NULL;
				}
				pt->active = 0;
				return;
			}
			else if(p->mother->baycool == 0.){
				static const avec3_t pos0[2] = {{-70. * SCARRY_SCALE, 30. * SCARRY_SCALE, 0.}, {-130. * SCARRY_SCALE, 30. * SCARRY_SCALE, 0.},};
				QUATCPY(pt->rot, pm->rot);
				quatrot(pt->pos, pm->rot, pos0[mother->undockc++ % 2]);
				VECADDIN(pt->pos, pm->pos);
				VECCPY(pt->velo, pm->velo);
				p->task = SCEPTOR_undock;
				p->mother->baycool = SCARRY_BAYCOOL;
				if(p->pf)
					ImmobilizeTefpol3D(p->pf);
				p->pf = AddTefpolMovable3D(w->tepl, pt->pos, pt->velo, avec3_000, &cs_shortburn, TEP3_THICK | TEP3_ROUGH, cs_shortburn.t);
			}
			else
				return;*/
		}
/*		else if(w->pl->control == pt){
		}*/
/*		else if(pf->returning){
			avec3_t delta;
			VECSUB(delta, pt->pos, mother->st.pos);
			pt->pyr[1] = approach(pt->pyr[1], fmod(atan2(-delta[0], delta[2]) + 2 * M_PI, 2 * M_PI), SCEPTOR_ROTSPEED * dt, 2 * M_PI);
			pt->pyr[0] = approach(pt->pyr[0], atan2(delta[1], sqrt(delta[0] * delta[0] + delta[2] * delta[2])), SCEPTOR_ROTSPEED * dt, 2 * M_PI);
			pt->pyr[2] = approach(pt->pyr[2] + M_PI, (pt->pyr[1] - oldyaw) / dt + M_PI, SCEPTOR_ROLLSPEED * dt, 2 * M_PI) - M_PI;
			if(VECSDIST(pt->pos, mother->st.pos) < 1. * 1.)
				pf->returning = 0;
		}
		else if(5. * 5. < VECSDIST(pt->pos, mother->st.pos)){
			pf->returning = 1;
		}*/
		// Docking is high priority task that being controlled by a human can cause errors, but for the other tasks, humans can take role.
		else{
//			double pos[3], dv[3], dist;
			Vec3d delta;
//			int i, n;
			bool trigger = true;
//			Vec3d opos;
//			pt->enemy = pm->enemy;
			if(pt->enemy /*&& VECSDIST(pt->pos, pm->pos) < 15. * 15.*/){
				double sp;
/*				const avec3_t guns[2] = {{.002, .001, -.005}, {-.002, .001, -.005}};*/
				Vec3d xh, dh, vh;
				Vec3d epos;
				double phi;
				estimate_pos(epos, enemy->pos, enemy->velo, pt->pos, pt->velo, bulletSpeed, w);
/*				xh[0] = pt->velo[2];
				xh[1] = pt->velo[1];
				xh[2] = -pt->velo[0];*/
				delta = epos - pt->pos;

				double dist = (enemy->pos - this->pos).len();
/*				Vec3d ldelta = rot.cnj().trans(delta);
				Vec3d lvelo = rot.cnj().trans(this->evelo - (enemy->velo - this->velo) * dist);
				if(ldelta.norm()[2] < -.5){
					double f = exp(-pid_ifactor * dt);
					integral[0] = integral[0] * f + pid_dfactor * lvelo.sp(Vec3d(1,0,0)) * dt;
					integral[1] = integral[1] * f + pid_dfactor * lvelo.sp(Vec3d(0,1,0)) * dt;
					ldelta[0] += pid_pfactor * integral[0];
					ldelta[1] += pid_pfactor * integral[1];
					delta = rot.trans(ldelta);
				}
				this->evelo = (enemy->velo - this->velo) * dist;
#if PIDAIM_PROFILE
				this->epos = epos;
				this->iepos = delta + pt->pos;
#endif
*/
				// Do not try shooting at very small target, that's just waste of ammo.
				if(enemy->hitradius() < dist / 300.)
					trigger = false;

			}
/*			else
			{
				delta = pm->pos - pt->pos;
				trigger = 0;
			}*/

			if(p->task == Undock){
				if(!pm)
					p->task = Auto;
				else{
					double sp;
					Vec3d dm = this->pos - pm->pos;
					Vec3d mzh = this->rot.trans(vec3_001);
					sp = -mzh.sp(dm);
					p->throttle = 1.;

					// Avoid collision with mother if in a progress of undocking.
					if(bbody)
						bbody->getBroadphaseProxy()->m_collisionFilterMask &= ~2;

					if(1. < sp)
						p->task = Parade;
				}
			}
			else if(!controlled) do{

				if(standingUp){
					Vec3d accel = w->accel(pos, velo);

					// If some acceleration, no matter gravitational or artificial, is present, we ought to stand againt it.
					if(DBL_EPSILON < accel.slen()){
						Vec3d downDir = accel.norm();
						bbody->applyTorque(btvc(-rot.trans(Vec3d(0,1,0)).vp(downDir) / bbody->getInvMass() * 10e-4) - bbody->getAngularVelocity() / bbody->getInvMass() * 2e-4);
						if(rot.trans(Vec3d(0,1,0)).sp(downDir) < -.99)
							standingUp = false;
					}
					else
						standingUp = false;
				}
				else if((task == Attack || task == Rest || task == Away) && !pt->enemy || task == Auto || task == Parade){
					if(forcedEnemy && enemy || pm && (pt->enemy = pm->enemy)){
						p->task = Attack;
					}
					else if(findEnemy()){
						p->task = Attack;
					}
					if(!enemy || (task == Auto || task == Parade)){
						if(pm)
							task = Parade;
						else
							task = Auto;
					}
				}
				else if(p->task == Moveto){
					Vec3d dr = pt->pos - p->dest;
					if(dr.slen() < .01 * .01){
						p->throttle = 0.;
						parking = 1;
						pt->velo += dr * -dt * .5;
						p->task = Idle;
					}
					else{
						steerArrival(dt, dest, vec3_000, 1. / 2., .0);
					}
				}
				else if(pt->enemy && (p->task == Attack || p->task == Away || task == Rest)){
					const Vec3d dv = delta.norm();
					Quatd totalrot = rot * aimRot();
					inputs.analog[0] = dv.sp(totalrot.trans(Vec3d(0,-1,0))) * 2e2;
					inputs.analog[1] = dv.sp(totalrot.trans(Vec3d(1,0,0))) * 2e2;

					// How much we face the enemy straight.
					double confrontness = dv.sp(totalrot.trans(Vec3d(0,0,-1)));

					if(floorTouched){

						// Try jumping only if the fuel is enough.
						if(.5 < confrontness && maxfuel() / 2. < fuel){
							// If we face some obstacles, try to jump over it.
							const btVector3 &btpos = bbody->getWorldTransform().getOrigin();
							const btVector3 &from = btpos;
							const btVector3 to = btpos + btvc(-mat.vec3(2) * .05);

							btCollisionWorld::ClosestRayResultCallback rayCallback(from, to);

							ws->bdw->rayTest(from, to, rayCallback);

							if(rayCallback.hasHit()){
								btRigidBody *body = btRigidBody::upcast(rayCallback.m_collisionObject);
								if(body && body->hasContactResponse() && body->isStaticObject()){
									inputs.press |= PL_Q;
									task = Jump;
									jumptime = 2.;
								}
							}

						}

						// If we are facing towards enemy, walk forward.
						if(.99 < confrontness){
							if((inputs.press & PL_Q) == 0 && .5 * .5 < delta.slen())
								inputs.press |= PL_W;
						}
						if(trigger && .999 < confrontness){
							pt->inputs.change |= PL_ENTER;
							pt->inputs.press |= PL_ENTER;
						}

						if(maxfuel() * .9 < fuel){
							task = Jump;
							jumptime = 1.;
						}
					}
					else{
						Vec3d forward;
						Vec3d xh, yh;
						double sx, sy, len, len2, maxspeed = maxAngleSpeed * dt;
						Quatd qres, qrot;

						// If a mother could not be aquired, fight to the death alone.
						if(p->fuel < 30. && (pm || (pm = findMother()))){
							p->task = Dockque;
							break;
						}

						double dist = delta.len();
						double awaybase = pt->enemy->hitradius() * 3. + .1;
						if(.6 < awaybase)
							awaybase = pt->enemy->hitradius() + 1.; // Constrain awaybase for large targets
						double attackrad = awaybase < .6 ? awaybase * 5. : awaybase + 4.;
/*						if(p->task == Attack && dist < awaybase){
							p->task = Away;
						}
						else if(p->task == Away && attackrad < dist){
							p->task = Attack;
						}
						forward = pt->rot.trans(avec3_001);
						if(p->task == Attack)
							forward *= -1;*/
			/*				sx = VECSP(&mat[0], dv);
						sy = VECSP(&mat[4], dv);
						pt->inputs.press |= (sx < 0 ? PL_4 : 0 < sx ? PL_6 : 0) | (sy < 0 ? PL_2 : 0 < sy ? PL_8 : 0);*/
						p->throttle = 1.;

						// Randomly vibrates to avoid bullets
/*						if(0 < fuel && !floorTouched){
							RandomSequence rs((unsigned long)this ^ (unsigned long)(w->war_time() / .1));
							Vec3d randomvec;
							for(int i = 0; i < 3; i++)
								randomvec[i] = rs.nextd() - .5;
							bbody->applyCentralForce(btvc(randomvec * mass * randomVibration));
						}*/

						if((p->task == Attack || task == Rest) /*|| forward.sp(dv) < -.5*/){
/*							xh = forward.vp(dv);
							len = len2 = xh.len();
							len = asin(len);
							len = sin(len / 2.);

							double velolen = bbody->getLinearVelocity().length();
							throttle = maxspeed < velolen ? (maxspeed - velolen) / maxspeed : 0.;

							// Suppress side slips
							Vec3d sidevelo = velo - mat.vec3(2) * mat.vec3(2).sp(velo);
							bbody->applyCentralForce(btvc(-sidevelo * mass));*/

							/*if(len && len2)*/{
/*								btVector3 btomg = bbody->getAngularVelocity();
								btVector3 btxh = btvc(xh.norm());

								// The second term is for suppressing rotation.
								btVector3 btaac = btxh * len - btomg * .15;

								// Maneuvering ability is limited. We cap angular acceleration here to simulate it.
								if((M_PI / 50.) * (M_PI / 50.) < btaac.length2())
									btaac.normalize() *= (M_PI / 50.);

								// Thruster's responsivity is also limited, we integrate desired accel over time to simulate.
								for(int i = 0; i < 3; i++)
									aac[i] = approach(aac[i], btaac[i], 2. * dt, 0);

								// Torque is applied to the rigid body, so we must convert angular acceleration to torque by taking product with inverse inertia tensor.
								bbody->applyTorque(bbody->getInvInertiaTensorWorld() * btvc(aac) * 2.e-2 * (3. - 2. * fwaverider));

								btTransform bttr = bbody->getWorldTransform();
								btVector3 laac = btMatrix3x3(bttr.getRotation().inverse()) * (btaac);
								if(laac[0] < 0) p->thrusts[0][0] += -laac[0];
								if(0 < laac[0]) p->thrusts[0][1] += laac[0];
								p->thrusts[0][0] = min(p->thrusts[0][0], 1.);
								p->thrusts[0][1] = min(p->thrusts[0][1], 1.);*/

								if(task == Rest){
									if(maxfuel() * .9 < fuel)
										task = Attack;
								}
								else{
									if(fuel < maxfuel() / 10.)
										task = Rest;
									else
										inputs.press |= PL_W;
								}
							}
							if(trigger && (task == Attack || task == Rest) && dist < 20. && .999 < confrontness){
								pt->inputs.change |= PL_ENTER;
								pt->inputs.press |= PL_ENTER;
							}
						}
						else{
							p->aac.clear();
						}
					}
				}
				else if(!pt->enemy && (p->task == Attack || task == Rest || p->task == Away)){
					p->task = Parade;
				}
				else if(task == Jump){
					inputs.press |= PL_Q;
					jumptime = approach(jumptime, 0., dt, 0);
					if(jumptime == 0.)
						task = Attack;
					else{

						// Keep watching the obstacle forward.
						const btVector3 &btpos = bbody->getWorldTransform().getOrigin();
						const btVector3 &from = btpos;
						const btVector3 to = btpos + btvc(-mat.vec3(2) * .05);

						btCollisionWorld::ClosestRayResultCallback rayCallback(from, to);

						ws->bdw->rayTest(from, to, rayCallback);

						if(rayCallback.hasHit()){
							btRigidBody *body = btRigidBody::upcast(rayCallback.m_collisionObject);
							if(!(body && body->hasContactResponse() && body->isStaticObject())){
								jumptime = 2.;
								task = JumpForward;
							}
						}
					}
				}
				else if(task == JumpForward){
					inputs.press |= PL_W;
					jumptime = approach(jumptime, 0., dt, 0);
					if(jumptime == 0.)
						task = Attack;
				}

				if(p->task == Parade){
					if(mother){
						if(paradec == -1)
							paradec = mother->enumParadeC(mother->Fighter);
						Vec3d target, target0(-1., 0., -1.);
						Quatd q2, q1;
						target0[0] += p->paradec % 10 * -.05;
						target0[2] += p->paradec / 10 * -.05;
						target = pm->rot.trans(target0);
						target += pm->pos;
						Vec3d dr = pt->pos - target;
						if(dr.slen() < .01 * .01){
							q1 = pm->rot;
							p->throttle = 0.;
							parking = 1;
							pt->velo += dr * (-dt * .5);
							q2 = Quatd::slerp(pt->rot, q1, 1. - exp(-dt));
							if(bbody){
								btVector3 btvelo = bbody->getLinearVelocity();
								btvelo += btvc(dr * (-dt * .5));
								bbody->setLinearVelocity(btvelo);
								btTransform wt = bbody->getWorldTransform();
								wt.setRotation(wt.getRotation().slerp(btqc(q1), 1. - exp(-dt)));
								bbody->setWorldTransform(wt);
							}
							pt->rot = q2;
						}
						else{

							// Suppress side slips
							Vec3d sidevelo = velo - mat.vec3(2) * mat.vec3(2).sp(velo);
							bbody->applyCentralForce(btvc(-sidevelo * mass));

//							p->throttle = dr.slen() / 5. + .01;
							steerArrival(dt, target, pm->velo, 1. / 2., .001);
						}
						if(1. < p->throttle)
							p->throttle = 1.;
					}
					else
						task = Auto;
				}
				else if(p->task == DeltaFormation){
					int nwingmen = 1; // Count yourself
					ReZEL *leader = NULL;
					for(ReZEL *wingman = formPrev; wingman; wingman = wingman->formPrev){
						nwingmen++;
						if(!wingman->formPrev)
							leader = wingman;
					}
					if(leader){
						Vec3d dp((nwingmen % 2 * 2 - 1) * (nwingmen / 2 * .05), 0., nwingmen / 2 * .05);
						dest = (leader->task == Moveto ? leader->dest : leader->pos) + dp;
						Vec3d dr = pt->pos - dest;
						if(dr.slen() < .01 * .01){
							Quatd q1 = quat_u;
							p->throttle = 0.;
							parking = 1;
							pt->velo += dr * (-dt * .5);
							pt->rot = Quatd::slerp(pt->rot, q1, 1. - exp(-dt));
						}
						else
							steerArrival(dt, dest, leader->velo, 1., .001);
					}
				}
				else if(p->task == Dockque || p->task == Dock){
					if(!pm)
						pm = findMother();

					// It is possible that no one is glad to become a mother.
					if(!pm)
						p->task = Auto;
					else{
						Vec3d target0(pm->getDocker()->getPortPos());
						Quatd q2, q1;
						collideignore = pm;

						// Mask to avoid collision with the mother
						if(bbody)
							bbody->getBroadphaseProxy()->m_collisionFilterMask &= ~2;

						// Runup length
						if(p->task == Dockque)
							target0 += pm->getDocker()->getPortRot().trans(Vec3d(0, 0, -.3));

						// Suppress side slips
						Vec3d sidevelo = velo - mat.vec3(2) * mat.vec3(2).sp(velo);
						bbody->applyCentralForce(btvc(-sidevelo * mass));

						Vec3d target = pm->rot.trans(target0);
						target += pm->pos;
						steerArrival(dt, target, pm->velo, p->task == Dockque ? 1. / 2. : -mat.vec3(2).sp(velo) < 0 ? 1. : .025, .01);
						double dist = (target - this->pos).len();
						if(dist < .01){
							if(p->task == Dockque)
								p->task = Dock;
							else{
								mother->dock(pt);
								if(p->pf){
									ImmobilizeTefpol3D(p->pf);
									p->pf = NULL;
								}
								p->docked = true;
	/*							if(mother->cargo < scarry_cargousage(mother)){
									scarry_undock(mother, pt, w);
								}*/
								return;
							}
						}
						if(1. < p->throttle)
							p->throttle = 1.;
					}
				}
				if(task == Idle || task == Auto){
					throttle = 0.;
					btVector3 btvelo = bbody->getLinearVelocity();
					if(!btvelo.isZero())
						bbody->applyCentralForce(-btvelo * mass);
					btVector3 btomg = bbody->getAngularVelocity();

					// Control rotation to approach stationary. Avoid expensive tensor products for zero vectors.
					if(!btomg.isZero()){
						btVector3 torqueImpulseToStop = bbody->getInvInertiaTensorWorld().inverse() * -btomg;
						double thrust = dt * 1.;
						if(torqueImpulseToStop.length2() < thrust * thrust)
							bbody->applyTorqueImpulse(torqueImpulseToStop);
						else
							bbody->applyTorqueImpulse(torqueImpulseToStop.normalize() * thrust);
					}
				}
			}while(0);
/*			else{
				double common = 0., normal = 0.;
				Vec3d qrot;
				Vec3d pos, velo;
				int i;
				if(common){
					avec3_t th0[2] = {{0., 0., .003}, {0., 0., -.003}};
					qrot = mat.vec3(0) * common;
					pt->rot = pt->rot.quatrotquat(qrot);
				}
				if(normal){
					avec3_t th0[2] = {{.005, 0., .0}, {-.005, 0., 0.}};
					qrot = mat.vec3(1) * normal;
					pt->rot = pt->rot.quatrotquat(qrot);
				}
			}*/

			if(pt->inputs.press & (PL_ENTER | PL_LCLICK)){
				if(weapon == 0)
					shootRifle(dt);
				else if(weapon == 1)
					shootShieldBeam(dt);

				// Do not shoot vulcan to too far targets
				if(!controlled && enemy && (this->pos - enemy->pos).len() < 2.)
					shootVulcan(dt);
			}

			if(controlled && (weapon == 2 && pt->inputs.press & (PL_ENTER | PL_LCLICK) || pt->inputs.press & PL_RCLICK)){
				shootVulcan(dt);
			}

			if(p->cooldown < dt)
				p->cooldown = 0;
			else
				p->cooldown -= dt;

			freload = approach(freload, 0., dt, 0.);
			vulcancooldown = approach(vulcancooldown, 0., dt, 0.);

#if 0
			if((pf->away ? 1. * 1. : .3 * .3) < VECSLEN(delta)/* && 0 < VECSP(pt->velo, pt->pos)*/){
				avec3_t xh;
				xh[0] = pt->velo[2];
				xh[1] = pt->velo[1];
				xh[2] = -pt->velo[0];
				pt->pyr[1] = approach(pt->pyr[1], fmod(atan2(-delta[0], delta[2]) + 2 * M_PI, 2 * M_PI), SCEPTOR_ROTSPEED * dt, 2 * M_PI);
				pt->pyr[0] = approach(pt->pyr[0], atan2(delta[1], sqrt(delta[0] * delta[0] + delta[2] * delta[2])), SCEPTOR_ROTSPEED * dt, 2 * M_PI);
				pf->away = 0;
			}
			else if(.2 * .2 < VECSLEN(delta)){
				avec3_t xh;
				xh[0] = pt->velo[2];
				xh[1] = pt->velo[1];
				xh[2] = -pt->velo[0];
				if(!pf->away && VECSP(pt->velo, delta) < 0){
					double desired;
					desired = fmod(atan2(-delta[0], delta[2]) + 2 * M_PI, 2 * M_PI);

					/* roll */
/*					pt->pyr[2] = approach(pt->pyr[2], (pt->pyr[1] < desired ? 1 : desired < pt->pyr[1] ? -1 : 0) * M_PI / 6., .1 * M_PI * dt, 2 * M_PI);*/

					pt->pyr[1] = approach(pt->pyr[1], desired, SCEPTOR_ROTSPEED * dt, 2 * M_PI);

					if(pt->enemy && pt->pyr[1] == desired){
					}

					pt->pyr[0] = approach(pt->pyr[0], atan2(delta[1], sqrt(delta[0] * delta[0] + delta[2] * delta[2])), SCEPTOR_ROTSPEED * dt, 2 * M_PI);
				}
				else{
					const double desiredy = pt->pos[1] < .1 ? -M_PI / 6. : 0.;
					pf->away = 1;
					pt->pyr[0] = approach(pt->pyr[0], desiredy, .1 * M_PI * dt, 2 * M_PI);
				}
			}
			else{
				double desiredp;
				desiredp = fmod(atan2(-delta[0], delta[2]) + M_PI + 2 * M_PI, 2 * M_PI);
				pt->pyr[1] = approach(pt->pyr[1], desiredp, SCEPTOR_ROTSPEED * dt, 2 * M_PI);
				pt->pyr[0] = approach(pt->pyr[0], pt->pos[1] < .1 ? -M_PI / 6. : 0., SCEPTOR_ROTSPEED * dt, 2 * M_PI);
			}
#endif
		}

		Vec3d desiredAccel = mat.vec3(2) * throttle;

		bbody->activate(true);
		if(inputs.analog[2] != 0 || inputs.analog[3] != 0){
			if(inputs.analog[2] != 0)
				bbody->applyTorque(btvc(inputs.analog[2] * -mat.vec3(1) * torqueAmount));
			if(inputs.analog[3] != 0)
				bbody->applyTorque(btvc(inputs.analog[3] * mat.vec3(0) * torqueAmount));
		}
		else if(0 < fonfeet){
			const double maxspeed = walkSpeed;
			btScalar lateral = (inputs.press & PL_A ? -1 : 0) + (inputs.press & PL_D ? 1 : 0);
			btScalar frontal = (inputs.press & PL_W ? -1 : 0) + (inputs.press & PL_S ? 1 : 0);

			// Normalize if we are heading diagonal direction.
			if(lateral && frontal){
				double normalizer = sqrt(lateral * lateral + frontal * frontal);
				lateral /= normalizer;
				frontal /= normalizer;
			}

			btScalar lvelo(0);
			{
				btVector3 v = bbody->getWorldTransform().getBasis().getColumn(0);
				lvelo = bbody->getLinearVelocity().dot(v);
				bbody->applyCentralForce(v.normalize() * (lateral * maxspeed - lvelo) / bbody->getInvMass());
				twist = approach(twist, lateral * M_PI / 2., M_PI / 2. * dt, 0);
			}
			{
				btVector3 v = bbody->getWorldTransform().getBasis().getColumn(2);
				btScalar svelo = bbody->getLinearVelocity().dot(v);
				bbody->applyCentralForce(v.normalize() * (frontal * maxspeed - svelo) / bbody->getInvMass());
				walkphase = (walkphase + (-svelo + fabs(lvelo)) / maxspeed * dt);
				walkphase -= floor(walkphase); // Normalize
			}
			btScalar yaw = (inputs.press & PL_4 ? 1 : 0) + (inputs.press & PL_6 ? -1 : 0);
			bbody->applyTorque(btvc((2 * yaw - 2e1 * bbody->getWorldTransform().getBasis().getColumn(1).dot(bbody->getAngularVelocity()) / bbody->getInvInertiaDiagLocal().y()) * mat.vec3(1) * torqueAmount));
			if(pt->inputs.press & PL_8){
				bbody->applyTorque(btvc(mat.vec3(0) * torqueAmount));
			}
			if(pt->inputs.press & PL_2){
				bbody->applyTorque(btvc(-mat.vec3(0) * torqueAmount));
			}
		}
		else{
/*			if(inputs.press & PL_W)
				p->throttle = MIN(throttle + dt, 1.);
			if(inputs.press & PL_S)
				p->throttle = MAX(throttle - dt, -1.); // Reverse thrust is permitted*/

			const double maxspeed = airMoveSpeed;
			btScalar lateral = (inputs.press & PL_A ? -1 : 0) + (inputs.press & PL_D ? 1 : 0);
			btScalar frontal = (inputs.press & PL_W ? -1 : 0) + (inputs.press & PL_S ? 1 : 0);

			btScalar lvelo(0);
			if(lateral){ // lateral thrust (leftward or rightward)
				btVector3 v = bbody->getWorldTransform().getBasis().getColumn(0);
				lvelo = bbody->getLinearVelocity().dot(v);
				/*bbody->applyCentralForce*/ desiredAccel += btvc(v.normalize() * .1 * (lateral * maxspeed - lvelo) / bbody->getInvMass());
			}
			if(frontal){ // frontal thrust (forward or backward)
				btVector3 v = bbody->getWorldTransform().getBasis().getColumn(2);
				btScalar svelo = bbody->getLinearVelocity().dot(v);
				/*bbody->applyCentralForce*/ desiredAccel += btvc(v.normalize() * .1 * (frontal * maxspeed - svelo) / bbody->getInvMass());
			}

			if(pt->inputs.press & PL_8){
				bbody->applyTorque(btvc(mat.vec3(0) * torqueAmount));
			}
			if(pt->inputs.press & PL_2){
				bbody->applyTorque(btvc(-mat.vec3(0) * torqueAmount));
			}
			if(pt->inputs.press & PL_4){
				bbody->applyTorque(btvc(mat.vec3(1) * torqueAmount));
			}
			if(pt->inputs.press & PL_6){
				bbody->applyTorque(btvc(-mat.vec3(1) * torqueAmount));
			}
			if(pt->inputs.press & PL_7){
				bbody->applyTorque(btvc(mat.vec3(2) * torqueAmount));
			}
			if(pt->inputs.press & PL_9){
				bbody->applyTorque(btvc(-mat.vec3(2) * torqueAmount));
			}
		}

		// Face the body to the aimed direction.
		// Typically, the body is slower in response compared to manipulators's aim.
		// So the body follows the aim gradually.
		// TODO: Add torque instead of changing orientation directly to increase physical reality.
		{
			btTransform wt = bbody->getWorldTransform();

			if(!floorProximity){
				double dpitch = rangein(aimdir[0], -dt, dt);
				wt.setRotation(wt.getRotation() * btQuaternion(btVector3(1, 0, 0), -dpitch));
				aimdir[0] -= dpitch;
			}

			double dyaw = rangein(aimdir[1], -dt, dt);
			wt.setRotation(wt.getRotation() * btQuaternion(btVector3(0, 1, 0), -dyaw));
			aimdir[1] -= dyaw;

			bbody->setWorldTransform(wt);
		}


		{ // vertical thrust (upward and downward)
			const double maxspeed = airMoveSpeed;
			btScalar vertical = (inputs.press & PL_Z ? -1 : 0) + (inputs.press & PL_Q ? 1 : 0);
			if(vertical){
				btVector3 v = bbody->getWorldTransform().getBasis().getColumn(1);
				btScalar svelo = bbody->getLinearVelocity().dot(v);
				/*bbody->applyCentralForce*/ desiredAccel += btvc(v.normalize() * .1 * (vertical * maxspeed - svelo) / bbody->getInvMass());
			}
		}

		if(stabilizer && !fonfeet){
			/* you're not allowed to accel further than certain velocity. */
			const double maxvelo = .5, speed = -p->velo.sp(mat.vec3(2));
			if(maxvelo < speed)
				p->throttle = 0.;
			else{
				if(!controlled && (p->task == Attack || task == Rest || p->task == Away))
					p->throttle = 1.;
				if(0 < speed && 1. - speed / maxvelo < throttle)
					throttle = 1. - speed / maxvelo;
			}

/*			{
				btVector3 btvelo = bbody->getLinearVelocity();
				bbody->applyCentralForce(-btvelo * mass * .1);
			}*/

			// Suppress rotation if it's not intensional.
			if(!(pt->inputs.press & (PL_4 | PL_6 | PL_8 | PL_2 | PL_7 | PL_9))){
				btVector3 btomg = bbody->getAngularVelocity();
//				if(ws)
//					btomg += btvc(ws->orientation(pos).trans(vec3_010).vp(rot.trans(vec3_010)) * .2);
				btScalar len = btomg.length();
				if(1. < len)
					btomg /= len;
				bbody->applyTorque(-btomg * torqueAmount);
			}
		}

		/* Friction (in space!) */
/*		if(parking){
			double f = 1. / (1. + dt / (parking ? 1. : 3.));
			pt->velo *= f;
		}*/

		{
			// Half the top acceleration if not transformed
			double desiredAccelLen = desiredAccel.len();
			double consump = dt * (desiredAccelLen + p->fcloak * 4.); /* cloaking consumes fuel extremely */
			if(p->fuel <= consump){
				if(.05 < pf->throttle)
					pf->throttle = .05;
				if(p->cloak)
					p->cloak = 0;
				p->fuel = 0.;

				// Capped by max capability times 0.05
				if(.05 < desiredAccelLen)
					desiredAccel *= .05 / desiredAccelLen;
			}
			else
				p->fuel -= consump;

			// fuel is constantly regained by the aid of fusion reactor.
			fuel = approach(fuel, maxfuel(), fuelRegenRate * dt, 0.);

			double spd = .025/*(controlled ? .025 : p->task != Attack ? .01 : .005)*/;
//			Vec3d acc = (this->rot.rotate(thrustvector * M_PI / 2., /*this->rot.trans*/(Vec3d(1,0,0)))).trans(Vec3d(0., 0., -1.));
			Vec3d acc = desiredAccel;
			bbody->applyCentralImpulse(btvc(acc * spd * .001 * mass));
//			pt->velo += acc * spd;
		}

		if(model){
			static const Quatd rotaxis(0, 1., 0., 0.);
			double motion_time[numof(motions)];
			getMotionTime(&motion_time);
			ysdnm_var *v = YSDNM_MotionInterpolate(motions, motion_time, numof(motions));
			Vec3d accel = btvc(bbody->getTotalForce() * bbody->getInvMass() / dt);
			Vec3d relpos; // Relative position to gravitational center
			// Torque in Bullet dynamics engine. 
			Vec3d bttorque = btvc(bbody->getTotalTorque() / dt /* bbody->getInvInertiaTensorWorld()*/);
			Quatd lrot; // Rotational component of transformation from world coordinates to local coordinates.
			for(int i = 0; i < numof(thrusterDirs); i++) if(model->getBonePos(gltestp::dstring("ReZEL_thruster") << i, *v, &relpos, &lrot)){
				Vec3d localAccel = -(rot * rotaxis * lrot).cnj().trans(accel + relpos.vp(bttorque));
				if(localAccel.slen() < .1 * .1)
					localAccel /= .1;
				else
					localAccel.normin();
				Vec3d delta = localAccel - thrusterDirs[i];
/*				if(thrusterDirs[i].sp(thrusterDir[i]) < .5 && delta.sp(thrusterDir[i]) < 0.)
					continue;
				thrusterDirs[i] = Quatd::rotation(dt * 5., localAccel.vp(thrusterDirs[i])).trans(thrusterDirs[i]).normin();*/
				const Vec3d &target = .5 < localAccel.sp(thrusterDir[i]) ? localAccel : thrusterDir[i];
				for(int j = 0; j < 3; j++)
					thrusterDirs[i][j] = approach(thrusterDirs[i][j], target[j], dt, 0.);
				thrusterDirs[i].normin();
				// The shoulder thrusters are not available when in MA (Waverider) form.
				thrusterPower[i] = approach(thrusterPower[i], max(0, (i == 3 || i == 4) && waverider ? 0. : localAccel.sp(thrusterDirs[i])), dt * 2., 0.);
			}
			YSDNM_MotionInterpolateFree(v);
		}

		/* heat dissipation */
		p->heat *= exp(-dt * .2);

/*		pt->pyr[2] = approach(pt->pyr[2] + M_PI, (pt->pyr[1] - oldyaw) / dt + M_PI, SCEPTOR_ROLLSPEED * dt, 2 * M_PI) - M_PI;
		{
			double spd = pf->away ? .2 : .11;
			pt->velo[0] = spd * cos(pt->pyr[0]) * sin(pt->pyr[1]);
			pt->velo[1] = -spd * sin(pt->pyr[0]);
			pt->velo[2] = -spd * cos(pt->pyr[0]) * cos(pt->pyr[1]);
		}*/
/*		{
			double spd = pf->throttle * (pf->away ? .02 : .011);
			avec3_t acc, acc0 = {0., 0., -1.};
			quatrot(acc, pt->rot, acc0);
			VECSADD(pt->velo, acc, spd * dt);
		}*/
		if(p->task != Undock && p->task != Undockque && task != Dock){
/*			WarSpace *ws = (WarSpace*)w;
			if(ws)
				space_collide(pt, ws, dt, collideignore, NULL);*/
		}
//		pt->pos += pt->velo * dt;

		fonfeet = approach(fonfeet, floorTouched, dt, 0);

		fcloak = approach(fcloak, p->cloak, dt, 0.);
		fwaverider = approach(fwaverider, waverider, dt, 0.);
		fweapon = approach(fweapon, weapon == 1, dt, 0.);
		twist = approach(twist, rangein(omg.sp(rot.trans(vec3_010)), -1., 1.), dt, 0.);
		pitch = approach(pitch, rangein(omg.sp(rot.trans(vec3_100)), -1., 1.), dt, 0.);
		{
			double dpitch = inputs.analog[0] * w->pl->fov * 2e-3;
			double dyaw = inputs.analog[1] * w->pl->fov * 2e-3;
			aimdir[0] = approach(aimdir[0], rangein(aimdir[0] + dpitch, -M_PI / 2., M_PI / 2.), M_PI * dt, 0);
			aimdir[1] = approach(aimdir[1], rangein(aimdir[1] + dyaw, -M_PI / 3., M_PI / 3.), M_PI * dt, 0);
		}
		if(3. <= fsabre)
			fsabre = 1.;
		fsabre = approach(fsabre, weapon == 3 ? 3. : 0., dt, 0.);
	}
	else{
		bbody->activate();
		pt->health += dt;
		if(0. < pt->health){
			struct tent3d_line_list *tell = w->getTeline3d();
//			effectDeath(w, pt);
//			playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .1, w->realtime);
/*			if(w->gibs && ((struct entity_private_static*)pt->vft)->sufbase){
				int i, n, base;
				struct entity_private_static *vft = (struct entity_private_static*)pt->vft;
				int m = vft->sufbase->np;
				n = m <= SCEPTOR_MAX_GIBS ? m : SCEPTOR_MAX_GIBS;
				base = m <= SCEPTOR_MAX_GIBS ? 0 : rseq(&w->rs) % m;
				for(i = 0; i < n; i++){
					double velo[3], omg[3];
					int j;
					j = (base + i) % m;
					velo[0] = pt->velo[0] + (drseq(&w->rs) - .5) * .1;
					velo[1] = pt->velo[1] + (drseq(&w->rs) - .5) * .1;
					velo[2] = pt->velo[2] + (drseq(&w->rs) - .5) * .1;
					omg[0] = 3. * 2 * M_PI * (drseq(&w->rs) - .5);
					omg[1] = 3. * 2 * M_PI * (drseq(&w->rs) - .5);
					omg[2] = 3. * 2 * M_PI * (drseq(&w->rs) - .5);
					AddTelineCallback3D(w->gibs, pt->pos, velo, 0.01, pt->pyr, omg, w->gravity, vft->gib_draw, (void*)j, TEL3_NOLINE | TEL3_REFLECT, 1.5 + drseq(&w->rs));
				}
			}*/
/*			pt->pos = pm->pos;
			pt->pos[1] += .01;
			VECCPY(pt->velo, pm->velo);
			pt->health = 350.;
			if(p->pf){
				ImmobilizeTefpol3D(p->pf);
				p->pf = NULL;
			}
			pt->active = 0;*/
/*			pf->docked = 1;*/

			/* smokes */
			for(int i = 0; i < 16; i++){
				Vec3d pos;
				COLOR32 col = 0;
				pos[0] = .02 * (drseq(&w->rs) - .5);
				pos[1] = .02 * (drseq(&w->rs) - .5);
				pos[2] = .02 * (drseq(&w->rs) - .5);
				col |= COLOR32RGBA(rseq(&w->rs) % 32 + 127,0,0,0);
				col |= COLOR32RGBA(0,rseq(&w->rs) % 32 + 127,0,0);
				col |= COLOR32RGBA(0,0,rseq(&w->rs) % 32 + 127,0);
				col |= COLOR32RGBA(0,0,0,191);
	//			AddTeline3D(w->tell, pos, NULL, .035, NULL, NULL, NULL, col, TEL3_NOLINE | TEL3_GLOW | TEL3_INVROTATE, 60.);
				AddTelineCallback3D(tell, pos + this->pos, pos / 1. + velo / 2., .02, quat_u, vec3_000, vec3_000, ::smokedraw, (void*)col, TEL3_INVROTATE | TEL3_NOLINE, 5.);
			}

			{/* explode shockwave thingie */
#if 0
				Quatd q;
				double p;
				Vec3d dr = vec3_001;

				// half-angle formula of trigonometry replaces expensive tri-functions to square root
				// cos(acos(dr[2]) / 2.)
				q[3] = sqrt((dr[2] + 1.) / 2.) ;

				Vec3d v = vec3_001.vp(dr);
				p = sqrt(1. - q[3] * q[3]) / VECLEN(v);
				q = v * p;
				q = quat_u;

				AddTeline3D(tell, this->pos, vec3_000, 1., q, vec3_000, vec3_000, COLOR32RGBA(255,191,63,255), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_QUAT, 1.);
#endif
				AddTeline3D(tell, this->pos, vec3_000, .3, quat_u, vec3_000, vec3_000, COLOR32RGBA(255,255,255,127), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_INVROTATE, .5);
			}
			this->w = NULL;
		}
		else{
			struct tent3d_line_list *tell = w->getTeline3d();
			if(tell){
				double pos[3], dv[3], dist;
				Vec3d gravity = w->accel(this->pos, this->velo) / 2.;
				int i, n;
				n = (int)(dt * deathSmokeFreq + drseq(&w->rs));
				for(i = 0; i < n; i++){
					pos[0] = pt->pos[0] + (drseq(&w->rs) - .5) * .01;
					pos[1] = pt->pos[1] + (drseq(&w->rs) - .5) * .01;
					pos[2] = pt->pos[2] + (drseq(&w->rs) - .5) * .01;
					dv[0] = .5 * pt->velo[0] + (drseq(&w->rs) - .5) * .01;
					dv[1] = .5 * pt->velo[1] + (drseq(&w->rs) - .5) * .01;
					dv[2] = .5 * pt->velo[2] + (drseq(&w->rs) - .5) * .01;
//					AddTeline3D(w->tell, pos, dv, .01, NULL, NULL, gravity, COLOR32RGBA(127 + rseq(&w->rs) % 32,127,127,255), TEL3_SPRITE | TEL3_INVROTATE | TEL3_NOLINE | TEL3_REFLECT, 1.5 + drseq(&w->rs) * 1.5);
					AddTelineCallback3D(tell, pos, dv, .02, quat_u, vec3_000, gravity, firesmokedraw, NULL, TEL3_INVROTATE | TEL3_NOLINE, 1.5 + drseq(&w->rs) * 1.5);
				}
			}
//			pt->pos += pt->velo * dt;

			// Stop thrusters
			for(int i = 0; i < numof(thrusterDirs); i++)
				thrusterPower[i] = approach(thrusterPower[i], 0, dt * 2., 0.);
		}
	}

	if(w)
		bbody->applyCentralForce(btvc(w->accel(this->pos, this->velo) * mass));

	reverser = approach(reverser, throttle < 0, dt * 5., 0.);

	// Decay muzzle flashes
	for(int i = 0; i < numof(muzzleFlash); i++){
		if(muzzleFlash[i] < dt)
			muzzleFlash[i] = 0.;
		else
			muzzleFlash[i] -= dt;
	}

	st::anim(dt);

	// if we are transitting WarField or being destroyed, trailing tefpols should be marked for deleting.
	if(this->pf && w != oldw)
		ImmobilizeTefpol3D(this->pf);
//	movesound3d(pf->hitsound, pt->pos);
}

// Docking and undocking will never stack.
bool ReZEL::solid(const Entity *o)const{
	return !(task == Undock || task == Dock);
}

int ReZEL::takedamage(double damage, int hitpart){
	int ret = 1;
	struct tent3d_line_list *tell = w->getTeline3d();
	if(this->health < 0.)
		return 1;
//	this->hitsound = playWave3D("hit.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
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

void ReZEL::postframe(){
	if(mother && mother->e && (!mother->e->w || docked && mother->e->w != w)){
		mother = NULL;

		// If the mother is lost, give up docking sequence.
		if(task == Dock || task == Dockque)
			task = Auto;
	}
	if(enemy && enemy->w != w)
		enemy = NULL;
	if(formPrev && formPrev->w != w)
		formPrev = NULL;
	st::postframe();
}

bool ReZEL::isTargettable()const{
	return true;
}
bool ReZEL::isSelectable()const{return true;}

double ReZEL::hitradius()const{
	return .01;
}

int ReZEL::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	double sc[3];
	double best = dt, retf;
	int reti = 0, n;
	for(n = 0; n < nhitboxes; n++){
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

#if 0
static int SCEPTOR_visibleFrom(const entity_t *viewee, const entity_t *viewer){
	SCEPTOR_t *p = (SCEPTOR_t*)viewee;
	return p->fcloak < .5;
}

static double SCEPTOR_signalSpectrum(const entity_t *pt, double lwl){
	SCEPTOR_t *p = (SCEPTOR_t*)pt;
	double x;
	double ret = 0.5;

	/* infrared emission */
	x = (lwl - -4) * .8; /* around 100 um */
	ret += (p->heat) * exp(-x * x);

	/* radar reflection */
	x = (lwl - log10(1)) * .5;
	ret += .5 * exp(-x * x); /* VHF */

	/* cloaking hides visible signatures */
	x = lwl - log10(475e-9);
	ret *= 1. - .95 * p->fcloak * exp(-x * x);

	/* engine burst emission cannot be hidden by cloaking */
	x = (lwl - -4) * .2; /* around 100 um */
	ret += 3. * (p->throttle) * exp(-x * x);

	return ret;
}


static warf_t *SCEPTOR_warp_dest(entity_t *pt, const warf_t *w){
	SCEPTOR_t *p = (SCEPTOR_t*)pt;
	if(!p->mother || !p->mother->st.st.vft->warp_dest)
		return NULL;
	return p->mother->st.st.vft->warp_dest(p->mother, w);
}
#endif

double ReZEL::maxfuel()const{
	return maxFuel;
}

bool ReZEL::command(EntityCommand *com){
	if(InterpretCommand<HaltCommand>(com)){
		task = Idle;
		forcedEnemy = false;
	}
/*	else if(InterpretCommand<DeltaCommand>(com)){
		Entity *e;
		for(e = next; e; e = e->next) if(e->classname() == classname() && e->race == race){
			formPrev = static_cast<ReZEL*>(e);
			task = DeltaFormation;
			break;
		}
		return true;
	}*/
	else if(TransformCommand *tc = InterpretCommand<TransformCommand>(com)){
		waverider = !!tc->formid;
		if(bbody)
			bbody->setCollisionShape(waverider ? waveRiderShape : shape);
	}
	else if(WeaponCommand *wc = InterpretCommand<WeaponCommand>(com)){
		weapon = wc->weaponid;
	}
	else if(StabilizerCommand *sc = InterpretCommand<StabilizerCommand>(com)){
		stabilizer = sc->stabilizer;
	}
	else if(InterpretCommand<ParadeCommand>(com)){
		task = Parade;
		if(!mother)
			findMother();
		return true;
	}
	else if(InterpretCommand<DockCommand>(com)){
		task = Dockque;
		return true;
	}
	else if(MoveCommand *mc = InterpretCommand<MoveCommand>(com)){
		switch(task){
			case Undockque:
			case Undock:
			case Dock:
				return false;
		}
		task = Moveto;
		this->dest = mc->destpos;
		return true;
	}
	if(AttackCommand *ac = InterpretDerivedCommand<AttackCommand>(com)){
		if(!ac->ents.empty()){
			Entity *e = *ac->ents.begin();
			if(e && e->getUltimateOwner() != getUltimateOwner()){
				enemy = e;
				forcedEnemy = ac->id() == ForceAttackCommand::sid;
				task = Auto;
				return true;
			}
		}
		return true;
	}
	return st::command(com);
}


/*Entity *ReZEL::create(WarField *w, Builder *mother){
	ReZEL *ret = new ReZEL(NULL);
	ret->pos = mother->pos;
	ret->velo = mother->velo;
	ret->rot = mother->rot;
	ret->omg = mother->omg;
	ret->race = mother->race;
	ret->task = Undock;
//	w->addent(ret);
	return ret;
}*/




#if 0
int ReZEL::cmd_dock(int argc, char *argv[], void *pv){
	Player *pl = (Player*)pv;
	for(Entity *e = pl->selected; e; e = e->selectnext){
		e->command(&DockCommand());
	}
	return 0;
}

int ReZEL::cmd_parade_formation(int argc, char *argv[], void *pv){
	Player *pl = (Player*)pv;
	for(Entity *e = pl->selected; e; e = e->selectnext){
		e->command(&ParadeCommand());
	}
	return 0;
}

static int cmd_delta_formation(int argc, char *argv[], void *pv){
	Player *pl = (Player*)pv;
	for(Entity *e = pl->selected; e; e = e->selectnext){
		e->command(&DeltaCommand());
	}
	return 0;
}

static void register_sceptor_cmd(void){
	extern Player pl;
	CmdAddParam("delta_formation", cmd_delta_formation, &pl);
}

static Initializator sss(register_sceptor_cmd);
#endif

double ReZEL::pid_pfactor = 1.;
double ReZEL::pid_ifactor = 1.;
double ReZEL::pid_dfactor = 1.;



IMPLEMENT_COMMAND(TransformCommand, "Transform");

TransformCommand::TransformCommand(HSQUIRRELVM v, Entity &){
	int argc = sq_gettop(v);
	if(argc < 2)
		throw SQFArgumentError();
	SQInteger i;
	if(SQ_FAILED(sq_getinteger(v, 3, &i)))
		throw SQFError(_SC("Invalid argument type"));
	formid = i;
}

IMPLEMENT_COMMAND(WeaponCommand, "Weapon");

WeaponCommand::WeaponCommand(HSQUIRRELVM v, Entity &){
	int argc = sq_gettop(v);
	if(argc < 2)
		throw SQFArgumentError();
	SQInteger i;
	if(SQ_FAILED(sq_getinteger(v, 3, &i)))
		throw SQFError(_SC("Invalid argument type"));
	weaponid = i;
}

IMPLEMENT_COMMAND(StabilizerCommand, "Stabilizer");

StabilizerCommand::StabilizerCommand(HSQUIRRELVM v, Entity &){
	int argc = sq_gettop(v);
	if(argc < 2)
		throw SQFArgumentError();
	SQInteger i;
	if(SQ_FAILED(sq_getinteger(v, 3, &i)))
		throw SQFError(_SC("Invalid argument type"));
	stabilizer = i;
}

