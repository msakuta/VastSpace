/** \file
 * \brief Implementation of ArmBase derived classes.
 */
#define NOMINMAX
#include "arms.h"
#include "MTurret.h"
#include "EntityRegister.h"
#include "Bullet.h"
#include "serial_util.h"
#include "Missile.h"
#include "EntityCommand.h"
#include "motion.h"
#include "Game.h"
#include "SqInitProcess.h"
#include "SqInitProcess-ex.h"
extern "C"{
#include <clib/mathdef.h>
#include <clib/cfloat.h>
}
#include <limits>


const char *hardpoint_static::classname()const{
	return "hardpoint_static";
}

const unsigned hardpoint_static::classid = registerClass("hardpoint_static", Conster<hardpoint_static>);

void hardpoint_static::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << pos; /* base position relative to object origin */
	sc.o << rot; /* base rotation */
	sc.o << name;
	sc.o << flagmask;
}

void hardpoint_static::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	cpplib::dstring name;
	sc.i >> pos; /* base position relative to object origin */
	sc.i >> rot; /* base rotation */
	sc.i >> name;
	sc.i >> flagmask;

	this->name = strnewdup(name, name.len());
}



void ArmBase::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << base;
	sc.o << target;
	sc.o << hp;
	sc.o << ammo;
	sc.o << online;
}

void ArmBase::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> base;
	sc.i >> target;
	sc.i >> hp;
	sc.i >> ammo;
	sc.i >> online;
}

void ArmBase::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext&)){
	st::dive(sc, method);

	// Dive in if it's the first occurrence of the associated hardpoint object.
	// Multiple ArmBases can share the same hardpoint object, and we do not want duplicate data sent.
	// Hardpoint_static class is a weird class. It's like a static data, but is a Serializable
	// because it can be saved to and loaded from a file to restore the state at the moment of save
	// operation, regardless of loaded environment's settings.
	// It's hard to decide if this applies to server and client transmission too.
	// But for the moment, it'd be too tough job to separate the logic between saved games and network games.
	//
	// TODO: Server side modification won't be sent, since game->idmap() continues to store the id
	// after the first frame. Probably this won't be a problem because the hardpoint object is fairly
	// static, but it's not forced to be.
	//
	// This trick does not work in all cases. We just give up and put hardpoint_static objects in the
	// serialization stream.
//	Game::IdMap &idmap = game->idmap();
//	if(idmap.find(hp->getid()) == idmap.end())
		const_cast<hardpoint_static*>(hp)->dive(sc, method);
}

void ArmBase::clientUpdate(double dt){
	// Default behavior is to act like in the server.
	anim(dt);
}

double ArmBase::getHitRadius()const{
	return 0.;
}

Entity *ArmBase::getOwner(){
	return base;
}

bool ArmBase::isTargettable()const{
	return false;
}
bool ArmBase::isSelectable()const{return true;}

Entity::Props ArmBase::props()const{
	Props ret = st::props();
//	ret.push_back(cpplib::dstring("Ammo: ") << ammo);
	return ret;
}

gltestp::dstring ArmBase::descript()const{
	return gltestp::dstring(idname());
};

void ArmBase::align(){
	// dead arms do not follow the base
	if(health <= 0. || !base)
		return;

	if(w != base->w){
		// Notify the refering Observers that this Entity is exiting current WarSpace.
		TransitEvent te(base->w);
		notifyEvent(te);
		base->w->addent(this);
	}
	race = base->race;

	/* calculate tr(pb->pos) * pb->pyr * pt->pos to get global coords */
	Mat4d mat;
	base->transform(mat);
	pos = mat.vp3(hp->pos);
	velo = base->velo + base->omg.vp(base->rot.trans(hp->pos));
	this->rot = base->rot * hp->rot;
}




double MTurret::modelScale = 0.05;
double MTurret::hitRadius = 5.;
double MTurret::turretVariance = .001 * M_PI;
double MTurret::turretIntolerance = M_PI / 20.;
double MTurret::rotateSpeed = 0.4 * M_PI;
double MTurret::manualRotateSpeed = rotateSpeed * 0.5;
gltestp::dstring MTurret::modelFile = "models/turretz1.mqo";
gltestp::dstring MTurret::turretObjName = "turretz1";
gltestp::dstring MTurret::barrelObjName = "barrelz1";
gltestp::dstring MTurret::muzzleObjName = "muzzlez1";

Entity::EntityRegisterNC<MTurret> MTurret::entityRegister("MTurret");

MTurret::MTurret(Entity *abase, const hardpoint_static *ahp) : st(abase, ahp), cooldown(0), mf(0), forceEnemy(false){
	init();
	health = getMaxHealth();
	ammo = 1500;
	py[0] = 0;
	py[1] = 0;
}

void MTurret::init(){

	static bool initialized = false;
	if(!initialized){
		SqInit(game->sqvm, _SC("models/MTurret.nut"),
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

void MTurret::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << cooldown;
	sc.o << py[0] << py[1]; // pitch and yaw
	sc.o << mf; // muzzle flash time
	sc.o << forceEnemy;
}

void MTurret::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> cooldown;
	sc.i >> py[0] >> py[1]; // pitch and yaw
	sc.i >> mf; // muzzle flash time
	sc.i >> forceEnemy;
}

const char *MTurret::dispname()const{
	return "Mounted Turret";
};

Entity::EntityStatic &MTurret::getStatic()const{return entityRegister;}

void MTurret::cockpitView(Vec3d &pos, Quatd &rot, int)const{
	rot = this->rot * Quatd(0, sin(py[1]/2), 0, cos(py[1]/2)) * Quatd(sin(py[0]/2), 0, 0, cos(py[0]/2));
	pos = this->pos + rot.trans(Vec3d(.0, 5., 15.));
}

static const double mturret_ofs[3] = {0., 0., -1.};
static const double mturret_range[2][2] = {-M_PI / 16., M_PI / 2, -M_PI, M_PI};

void MTurret::findtarget(Entity *pb, const hardpoint_static *hp, const Entity *ignore_list[], int nignore_list){
	MTurret *pt = this;
	WarField *w = pb->w;
	double bulletrange = getBulletSpeed() * getBulletLife(); /* sense range */
	double best = std::numeric_limits<double>::max();
	static const Vec3d right(1., 0., 0.), left(-1., 0., 0.);
	Entity *closest = NULL;

	// Obtain reverse transformation matrix to the turret's local coordinate system.
	Mat4d mat2 = this->rot.cnj().tomat4().translatein(-this->pos);

	// Quit chasing target that is out of shooting range.
	if(target && best < (target->pos - pos).slen())
		target = NULL;

	for(WarField::EntityList::iterator it = w->el.begin(); it != w->el.end(); it++) if(*it){
		Entity *pt2 = *it;
		Vec3d delta, ldelta;
		double theta, phi, f;

		f = findtargetproc(pb, hp, pt2);
		if(f == 0.)
			continue;

		if(!(pt2->isTargettable() && pt2 != pb && pt2->w == w && pt2->getHealth() > 0. && pt2->race != -1 && pt2->race != pb->race))
			continue;

/*		if(!entity_visible(pb, pt2))
			continue;*/

		// Do not passively attack Resource Station that can be captured instead of being destroyed.
		if(!strcmp(pt2->classname(), "RStation"))
			continue;

		ldelta = mat2.vp3(pt2->pos);
		phi = -atan2(ldelta[2], ldelta[0]);
		theta = atan2(ldelta[1], sqrt(ldelta[0] * ldelta[0] + ldelta[2] * ldelta[2]));

		// Ignore targets that are out of turret rotation or barrel pitch range.
		if(!checkTargetRange(phi, theta))
			continue;

		double sdist = (pt2->pos - pb->pos).slen();
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
		target = closest;
}

double MTurret::findtargetproc(const Entity *pb, const hardpoint_static *hp, const Entity *pt2){
	return 1.;
}

bool MTurret::checkTargetRange(double phi, double theta)const{
	return mturret_range[1][0] < phi && phi < mturret_range[1][1] && mturret_range[0][0] < theta && theta < mturret_range[0][1];
}

int MTurret::wantsFollowTarget()const{return 2;}

void MTurret::tryshoot(){
	if(ammo <= 0){
		this->cooldown += getShootInterval();
		return;
	}
	static const avec3_t forward = {0., 0., -1.};
	Bullet *pz;
	Quatd qrot;
	pz = new Bullet(base, getBulletLife(), 120.);
	w->addent(pz);
	Mat4d mat2;
	base->transform(mat2);
	mat2.translatein(hp->pos);
	Mat4d rot = hp->rot.tomat4();
	Mat4d mat = mat2 * rot;
	mat.translatein(0., 1., -2.);
	mat2 = mat.roty(this->py[1] + (drseq(&w->rs) - .5) * getTurretVariance());
	mat = mat2.rotx(this->py[0] + (drseq(&w->rs) - .5) * getTurretVariance());
	pz->pos = mat.vp3(mturret_ofs);
	pz->velo = mat.dvp3(forward) * getBulletSpeed() + this->velo;
	this->cooldown += getShootInterval();
	this->mf += .2f;
	ammo--;
}

void MTurret::control(const input_t *inputs, double dt){
	this->inputs = *inputs;
}

void MTurret::anim(double dt){
	if(!base || !base->w)
		w = NULL;
	if(!w)
		return;
	MTurret *const a = this;
	Entity *pt = base;
//	Entity **ptarget = &target;
	Vec3d epos; /* estimated enemy position */
	double phi, theta; /* enemy direction */

	/* find enemy logic */
	if(!forceEnemy)
		findtarget(pt, hp);

/*	if(*ptarget && (w != (*ptarget)->w || (*ptarget)->health <= 0.))
		*ptarget = NULL;*/

	if(a->mf < dt)
		a->mf = 0.;
	else
		a->mf -= float(dt);

	if(online){
		if(game->player && controller){
			double pydst[2] = {py[0], py[1]};
			if(inputs.press & PL_A)
				pydst[1] += getManualRotateSpeed() * dt;
			if(inputs.press & PL_D)
				pydst[1] -= getManualRotateSpeed() * dt;
			if(inputs.press & PL_W)
				pydst[0] += getManualRotateSpeed() * dt;
			if(inputs.press & PL_S)
				pydst[0] -= getManualRotateSpeed() * dt;
			a->py[1] = approach(a->py[1] + M_PI, pydst[1] + M_PI, getRotateSpeed() * dt, 2 * M_PI) - M_PI;
			a->py[0] = rangein(approach(a->py[0] + M_PI, pydst[0] + M_PI, getRotateSpeed() * dt, 2 * M_PI) - M_PI, mturret_range[0][0], mturret_range[0][1]);
			/*if(game->isServer())*/{
				if(inputs.press & (PL_ENTER | PL_LCLICK)) while(a->cooldown < dt){
					tryshoot();
				}
			}
		}
		else if(target && wantsFollowTarget()) do{/* estimating enemy position */
			double bulletRange = getBulletSpeed() * getBulletLife();
			bool notReachable = bulletRange * bulletRange < (target->pos - this->pos).slen();

			// If not forced to attack certain target, forget about target that bullets have no way to reach.
			if(notReachable && !forceEnemy){
				target = NULL;
				break;
			}

			Vec3d pos, velo, pvelo, gepos;
			Mat4d rot;

			/* calculate tr(pb->pos) * pb->pyr * pt->pos to get global coords */
			Mat4d mat2 = this->rot.cnj().tomat4().translatein(-this->pos);
			pos = mat2.vp3(target->pos);
			velo = mat2.dvp3(target->velo);
			pvelo = mat2.dvp3(pt->velo);

			estimate_pos(epos, pos, velo, vec3_000, pvelo, getBulletSpeed(), w);
				
			/* these angles are in local coordinates */
			phi = -atan2(epos[0], -(epos[2]));
			theta = atan2(epos[1], sqrt(epos[0] * epos[0] + epos[2] * epos[2]));
			a->py[1] = approach(a->py[1] + M_PI, phi + M_PI, getRotateSpeed() * dt, 2 * M_PI) - M_PI;
			if(1 < wantsFollowTarget())
				a->py[0] = rangein(approach(a->py[0] + M_PI, theta + M_PI, getRotateSpeed() * dt, 2 * M_PI) - M_PI, mturret_range[0][0], mturret_range[0][1]);

			/* shooter logic */
			/*if(game->isServer())*/ while(a->cooldown < dt){
				double yaw = a->py[1];
				double pitch = a->py[0];

				// Do not waste bullets at not reachable target.
				if(!notReachable && fabs(phi - yaw) < getTurretIntolerance() && fabs(pitch - theta) < getTurretIntolerance()){
					tryshoot();
				}
				else
					a->cooldown += .5 + (drseq(&w->rs) - .5) * .2;
			}
		}while(0);
	}

	if(a->cooldown < dt)
		a->cooldown = 0.;
	else
		a->cooldown -= float(dt);
}

double MTurret::getHitRadius()const{
	return hitRadius;
}

Entity::Props MTurret::props()const{
	Props ret = st::props();
//	ret.push_back(gltestp::dstring("Cooldown: ") << cooldown);
	return ret;
}

gltestp::dstring MTurret::descript()const{
	return gltestp::dstring() << idname() << " " << health << " " << ammo;
}

float MTurret::getShootInterval()const{
	return 2.;
}

double MTurret::getBulletSpeed()const{
	return 2000.;
}

float MTurret::getBulletLife()const{
	return 3.;
}

bool MTurret::command(EntityCommand *com){
	if(InterpretCommand<HaltCommand>(com)){
		target = NULL;
		forceEnemy = false;
		return true;
	}
	else if(AttackCommand *ac = InterpretCommand<AttackCommand>(com)){
		if(!ac->ents.empty()){
			Entity *e = *ac->ents.begin();
			if(e && e->race != race && e->getUltimateOwner() != getUltimateOwner()){
				target = e;
				forceEnemy = true;
				return true;
			}
		}
	}
	else if(ForceAttackCommand *fac = InterpretCommand<ForceAttackCommand>(com)){
		if(!fac->ents.empty()){
			Entity *e = *fac->ents.begin();

			// Though if force attacked, you cannot hurt yourself.
			if(e && e->getUltimateOwner() != getUltimateOwner()){
				target = e;
				forceEnemy = true;
				return true;
			}
		}
	}
	return false;
}

#ifdef DEDICATED
void MTurret::draw(WarDraw *){}
void MTurret::drawtra(WarDraw *){}
#endif






double GatlingTurret::modelScale = 0.05;
double GatlingTurret::hitRadius = 5.;
double GatlingTurret::turretVariance = .001 * M_PI;
double GatlingTurret::turretIntolerance = M_PI / 20.;
double GatlingTurret::rotateSpeed = 0.4 * M_PI;
double GatlingTurret::manualRotateSpeed = rotateSpeed * 0.5;
double GatlingTurret::bulletDamage = 10.;
double GatlingTurret::bulletLife = 10.;
double GatlingTurret::shootInterval = 0.1;
double GatlingTurret::bulletSpeed = 4000.;
int GatlingTurret::magazineSize = 50;
double GatlingTurret::reloadTime = 5.;
double GatlingTurret::muzzleFlashDuration = 0.075;
double GatlingTurret::barrelRotSpeed = 2. * M_PI / shootInterval / 3.;
Vec3d GatlingTurret::shootOffset = Vec3d(0., 30, 0.) * modelScale;
gltestp::dstring GatlingTurret::modelFile = "models/turretg1.mqo";
gltestp::dstring GatlingTurret::turretObjName = "turretg1";
gltestp::dstring GatlingTurret::barrelObjName = "barrelg1";
gltestp::dstring GatlingTurret::barrelRotObjName = "barrelsg1";
gltestp::dstring GatlingTurret::muzzleObjName = "muzzleg1";

GatlingTurret::GatlingTurret(Game *game) : st(game), barrelrot(0), barrelomg(0){init();}

GatlingTurret::GatlingTurret(Entity *abase, const hardpoint_static *hp) : st(abase, hp), barrelrot(0), barrelomg(0){
	init();
	ammo = magazineSize;
}

void GatlingTurret::init(){

	static bool initialized = false;
	if(!initialized){
		SqInit(game->sqvm, _SC("models/GatlingTurret.nut"),
			SingleDoubleProcess(modelScale, _SC("modelScale")) <<=
			SingleDoubleProcess(hitRadius, _SC("hitRadius")) <<=
			SingleDoubleProcess(turretVariance, _SC("turretVariance")) <<=
			SingleDoubleProcess(turretIntolerance, _SC("turretIntolerance")) <<=
			SingleDoubleProcess(rotateSpeed, _SC("rotateSpeed")) <<=
			SingleDoubleProcess(manualRotateSpeed, _SC("manualRotateSpeed")) <<=
			SingleDoubleProcess(bulletDamage, _SC("bulletDamage")) <<=
			SingleDoubleProcess(bulletLife, _SC("bulletLife")) <<=
			SingleDoubleProcess(shootInterval, _SC("shootInterval")) <<=
			SingleDoubleProcess(bulletSpeed, _SC("bulletSpeed")) <<=
			IntProcess(magazineSize, _SC("magazineSize")) <<=
			SingleDoubleProcess(reloadTime, _SC("reloadTime")) <<=
			SingleDoubleProcess(barrelRotSpeed, _SC("barrelRotSpeed")) <<=
			Vec3dProcess(shootOffset, "shootOffset") <<=
			StringProcess(modelFile, _SC("modelFile")) <<=
			StringProcess(turretObjName, _SC("turretObjName")) <<=
			StringProcess(barrelObjName, _SC("barrelObjName")) <<=
			StringProcess(barrelRotObjName, _SC("barrelRotObjName")) <<=
			StringProcess(muzzleObjName, _SC("muzzleObjName"))
			);
		initialized = true;
	}
}


Entity::EntityRegisterNC<GatlingTurret> GatlingTurret::entityRegister("GatlingTurret");

void GatlingTurret::serialize(SerializeContext &sc){
	st::serialize(sc);
}

void GatlingTurret::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
}

Entity::EntityStatic &GatlingTurret::getStatic()const{return entityRegister;}

const char *GatlingTurret::dispname()const{
	return "Gatling Turret";
};

void GatlingTurret::anim(double dt){
	barrelrot += barrelomg * dt;
	barrelomg = MAX(0, barrelomg - dt * 2. * M_PI);
	st::anim(dt);
}

float GatlingTurret::getShootInterval()const{
	return shootInterval;
}

double GatlingTurret::getBulletSpeed()const{
	return bulletSpeed;
}

float GatlingTurret::getBulletLife()const{
	return bulletLife;
}

void GatlingTurret::tryshoot(){
	if(ammo <= 0){
		ammo = magazineSize;
		this->cooldown += reloadTime;
		return;
	}
	static const Vec3d forward(0., 0., -1.);
	Bullet *pz = new Bullet(base, getBulletLife(), bulletDamage);
	w->addent(pz);
	Mat4d mat;
	this->transform(mat);
	mat.translatein(shootOffset);
	Mat4d mat2 = mat.roty(this->py[1] + (drseq(&w->rs) - .5) * getTurretVariance());
	mat = mat2.rotx(this->py[0] + (drseq(&w->rs) - .5) * getTurretVariance());
	pz->pos = mat.vp3(mturret_ofs);
	pz->velo = mat.dvp3(forward) * getBulletSpeed() + this->velo;
	this->cooldown += getShootInterval();
	this->mf += muzzleFlashDuration;
	this->barrelomg = barrelRotSpeed;
	ammo--;
	if(ammo <= 0){
		ammo = magazineSize;
		this->cooldown += reloadTime;
	}
}

#ifdef DEDICATED
void GatlingTurret::draw(WarDraw *){}
void GatlingTurret::drawtra(WarDraw *){}
#endif















#if 0
const struct arms_static_info arms_static[num_armstype] = {
	{"N/A",				armc_none,			0,	0,		NULL,			NULL,	NULL,	0., 0., 0., 0., 0.},
	{"Hydra 70",		armc_launcher,		0,	16,		m261_draw,		NULL,	NULL,	0.2, 25., 2., 20., 5., 100.},
	{"Hellfire",		armc_launcher,		0,	4,		hellfire_draw,	NULL,	NULL,	1., 20., 45., 10., 25., 300.},
	{"AIM-9",			armc_missile,		0,	1,		aim9_draw,		NULL,	NULL,	4., 2., 20., 10., 84., 300.},
	{"Beam Cannon",		armc_launcher,		0,	10,		beamcannon_draw,NULL,	beamcannon_anim,	4., 120., 1., 1000., 200., 100.},
	{"Avenger",			armc_launcher,		0,	1000,	avenger_draw,	NULL,	avenger_anim,	0.1, 150., .5, 200., .1, 10.},
	{"LONGBOW Radar",	armc_radome,		0,	0,		NULL,			NULL,	NULL,	0., 50., 0., 200., 0.},
	{"Shield Dome",		armc_radome,		0,	0,		NULL,			NULL,	NULL,	0., 60., 0., 10000., 0.},
	{"S. Gun Turret",	armc_smallturret,	0,	100,	sturret_draw,	NULL,	sturret_anim,	1., 150., 0.5, 300., 12., 10.},
	{"VF-25 Gunpod",	armc_robothand,		0,	1000,	gunpod_draw,	NULL,	NULL,	.2, 150., 0.5, 600., 1., 10.},
	{"VF-25 Rifle",		armc_robothand,		0,	20,		gunpod_draw,	NULL,	NULL,	4., 200., 2, 1000., 20., 100.},
	{"M. Gun Turret",	armc_mediumturret,	0,	2000,	mturret_draw,	mturret_drawtra,	mturret_anim,	2., 2000., 20, 50000., 200., 80.},
	{"M. Beam Turret",	armc_mediumturret,	0,	100,	mturret_draw,	mturret_drawtra,	mturret_anim,	5., 2000., 20, 50000., 200., 80.},
	{"M. Mis. Turret",	armc_mediumturret,	0,	50,		mturret_draw,	mturret_drawtra,	mturret_anim,	5., 2000., 20, 50000., 200., 300.},
	{"M16A1 AR",		armc_infarm,		0,	20,		m16_draw,		NULL,	NULL,	.1, 4., .004, 2., .0001},
	{"Shotgun",			armc_infarm,		0,	8,		shotgun_draw,	NULL,	NULL,	1., 5., .015, 1.5, .0002},
	{"M40A1 Sniper",	armc_infarm,		0,	5,		m40_draw,		NULL,	NULL,	1.5, 6.57, .0095, 3., .00015},
	{"RPG-7",			armc_infarm,		0,	1,		rpg7_draw,		NULL,	NULL,	3., 7., 2.6, 1., .5},
	{"Mortar",			armc_infarm,		0,	1,		rpg7_draw,		NULL,	NULL,	2., 10., 4, 6., 1.5},
	{"PLG",				armc_infarm,		0,	1,		rpg7_draw,		NULL,	NULL,	2., 8., 4, 6., 1.5},
};



static GLfloat show_light_pos[4];

static void show_light_on(void){
	glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
	{
		int i;
		GLfloat mat_specular[] = {0., 0., 0., 1.}/*{ 1., 1., .1, 1.0 }*/;
		GLfloat mat_diffuse[] = { .5, .5, .5, 1.0 };
		GLfloat mat_ambient_color[] = { 0.25, 0.25, 0.25, 1.0 };
		GLfloat mat_shininess[] = { 50.0 };
		GLfloat color[] = {1., 1., 1., .8}, amb[] = {1., 1., 1., 1.};
		avec3_t pos;

		glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_color);
		glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
		glLightfv(GL_LIGHT0, GL_POSITION, show_light_pos);
		glLightfv(GL_LIGHT0, GL_SPECULAR, mat_specular);
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, color);
	}
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_NORMALIZE);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
}

static void show_light_off(void){
	glPopAttrib();
}

static void show_init_wardraw(wardraw_t *wd, Viewer *vw){
	wd->listLight = 0;
	wd->light_on = show_light_on;
	wd->light_off = show_light_off;
	VECCPY(wd->view, vw->pos);
	wd->light[0] = 1.;
	wd->light[1] = 1.;
	wd->light[2] = 1.;
	VECNORMIN(wd->light);
	wd->ambient = COLOR32RGBA(255,255,255,255);
	VECSCALE(wd->viewdir, vw->rot, -1.);
	wd->fov = 1.;
	wd->pgc = NULL;
	wd->vw = vw;
	wd->gametime = 0.;
	wd->rot = &mat4identity;
	MAT4CPY(wd->irot, &mat4identity);
	wd->irot_init = 1;
	wd->maprange = 1.;
}













static void armswar_think(warf_t *w, double dt){}
static void armswar_draw(warf_t *w, wardraw_t *wd){}
static int armswar_pointhit(warf_t *w, const avec3_t *pos, const avec3_t *velo, double dt, struct contact_info *ci){return 0;}
static void armswar_accel(warf_t *w, avec3_t *dst, const avec3_t *srcpos, const avec3_t *srcvelo){VECCPY(*dst, avec3_000);}
static double armswar_pressure(warf_t *w, const avec3_t *pos){return 1.;}
static double armswar_sonic_speed(warf_t *w, const avec3_t *pos){return .340;}
int WarOrientation(struct war_field*, amat3_t *dst, const avec3_t *pos);
double WarNearPlane(const warf_t *);
double WarFarPlane(const warf_t *);

static struct war_field_static armswar_static = {
	armswar_think,
	WarPostFrame,
	WarEndFrame,
	armswar_draw,
	armswar_pointhit,
	armswar_accel,
	armswar_pressure,
	armswar_sonic_speed,
	NULL,
	WarOrientation,
	WarNearPlane,
	WarFarPlane,
	NULL,
};
static warf_t armswar = {
	&armswar_static,
	NULL, /* pl */
	NULL, /* tl */
	NULL, /* bl */
	NULL, /* tell */
	NULL, /* gibs */
	NULL, /* tefl */
	NULL, /* map */
	NULL, /* wmd */
/*	NULL, *//* patriot_home */
	0.,
	NULL,
	0,
	{0, 0}
};



struct glwindowarms{
	glwindow st;
	entity_t *pt;
	double baseprice;
	double lastt;
	double fov;
	avec3_t focus;
	aquat_t rot;
	struct hardpoint_static *hp;
	int nhp;
	arms_t *arms, *retarms;
};

static int buttonid(struct glwindow *wnd, int mx, int my){
	return 320 <= mx && mx <= 380 && wnd->h - 72 <= my && my <= wnd->h - 60 ? 1
		: 400 <= mx && mx <= 460 && wnd->h - 72 <= my && my <= wnd->h - 60 ? 2 : 0;
}

static int ArmsShow(struct glwindow *wnd, int mx, int my, double gametime){
	struct glwindowarms *p = (struct glwindowarms *)wnd;
	wardraw_t wd;
	Viewer viewer;
	struct glcull glc;
	extern warf_t warf;
	double dt = p->lastt ? gametime - p->lastt : 0.;
	GLfloat light_position[4] = { 1., 1., 1., 0.0 };
/*	extern double g_left, g_right, g_bottom, g_top, g_near, g_far;*/
	double totalweight = 0., totalprice = 0., totalattack = 0.;
	int button = 0;
	int i, ind = wnd && 0 <= my && 300 <= mx && mx <= wnd->w ? (my) / 24 : -1, ammosel = wnd && 310 + 8 * 12 < mx;

	button = buttonid(wnd, mx, my);
	if(button)
		ind = -1;

	p->lastt = gametime;

	if(0 <= mx && mx < 300 && 0 <= my && my <= 300){
		glPushMatrix();
		glTranslated(wnd->x, wnd->y + 12, 0);
		glColor4ub(0,0,255,255);
		glBegin(GL_LINE_LOOP);
		glVertex2i(1, 1);
		glVertex2i(299, 1);
		glVertex2i(299, 299);
		glVertex2i(1, 299);
		glEnd();
		glPopMatrix();
	}

	MAT4IDENTITY(viewer.rot);
	quat2mat(viewer.rot, p->rot);
/*	MAT4ROTY(viewer.rot, mat4identity, gametime * .5);*/
	MAT4IDENTITY(viewer.irot);
	quat2imat(viewer.irot, p->rot);
/*	MAT4ROTY(viewer.irot, mat4identity, -gametime * .5);*/
	MAT4CPY(viewer.relrot, viewer.rot);
	MAT4CPY(viewer.relirot, viewer.irot);
	VECCPY(viewer.pos, &viewer.irot[8]);
	VECSCALEIN(viewer.pos, ((struct entity_private_static*)p->pt->vft)->getHitRadius * 3.);
	MAT4DVP3(show_light_pos, viewer.irot, light_position);
	VECNULL(viewer.velo);
	VECNULL(viewer.pyr);
	viewer.velolen = 0.;
	viewer.fov = 1.;
	viewer.cs = NULL;
	viewer.relative = 0;
	viewer.detail = 0;
	viewer.vp.w = viewer.vp.h = viewer.vp.m = 100;
	show_init_wardraw(&wd, &viewer);
	wd.gametime = gametime;
	wd.pgc = &glc;
	wd.w = NULL;
	VECNULL(p->pt->pos);
	VECNULL(p->pt->velo);
	VECNULL(p->pt->omg);
	VECNULL(p->pt->pyr);
	VECNULL(p->pt->rot);
	p->pt->rot[3] = 1.;
	p->pt->active = 1;

	if(wnd) for(i = 0; i < p->nhp; i++){
		char buf[128];
		i == ind && !ammosel ? glColor4ub(0,255,255,255) : glColor4ub(255,255,255,255);
		glRasterPos2d(wnd->x + 300, wnd->y + (i + 1) * 24);
		gldPutString(p->hp[i].name);
		glRasterPos2d(wnd->x + 310, wnd->y + (i + 1) * 24 + 12);
		if(arms_static[p->arms[i].type].maxammo)
			gldPutStringN(arms_static[p->arms[i].type].name, 15);
		else
			gldPutString(arms_static[p->arms[i].type].name);
		if(arms_static[p->arms[i].type].maxammo){
			i == ind && ammosel ? glColor4ub(0,255,255,255) : glColor4ub(255,255,255,255);
			glRasterPos2d(wnd->x + 310 + 8 * 15, wnd->y + (i + 1) * 24 + 12);
			sprintf(buf, "x %d", p->arms[i].ammo);
			gldPutString(buf);
		}
		totalweight += arms_static[p->arms[i].type].emptyweight + p->arms[i].ammo * arms_static[p->arms[i].type].ammoweight;
		totalprice += arms_static[p->arms[i].type].emptyprice + p->arms[i].ammo * arms_static[p->arms[i].type].ammoprice;
		totalattack += arms_static[p->arms[i].type].cooldown ? arms_static[p->arms[i].type].ammodamage / arms_static[p->arms[i].type].cooldown : 0.;
	}
	{
		char buf[128];
		if(button == 1)
			glColor4ub(0,255,255,255);
		else
			glColor4ub(255,255,255,255);
		glRasterPos2d(wnd->x + 320, wnd->y + wnd->h - 48);
		gldPutString("OK");
		if(button == 2)
			glColor4ub(0,255,255,255);
		else
			glColor4ub(255,255,255,255);
		glRasterPos2d(wnd->x + 400, wnd->y + wnd->h - 48);
		gldPutString("Cancel");

		glColor4ub(255,255,255,255);
		glwpos2d(wnd->x + 300, wnd->y + wnd->h - 36);
		glwprintf("Attack: %lg Points/sec", totalattack);
		sprintf(buf, "Weight: %lg kg", totalweight + p->pt->mass);
		glRasterPos2d(wnd->x + 300, wnd->y + wnd->h - 24);
		gldPutString(buf);
		if(totalprice + p->baseprice < 1e6)
			sprintf(buf, "Price: $ %lg k", totalprice + p->baseprice);
		else
			sprintf(buf, "Price: $ %lg M", (totalprice + p->baseprice) * 1e-3);
		glRasterPos2d(wnd->x + 300, wnd->y + wnd->h - 12);
		gldPutString(buf);
	}

	{
	amat4_t proj;
	avec3_t dst;
	double f;
	GLint vp[4];
	f = 1. - exp(-dt / .05);
	if(wnd){
		RECT rc;
		int mi = MIN(wnd->w, wnd->h - 12);
		GetClientRect(WindowFromDC(wglGetCurrentDC()), &rc);
		glGetIntegerv(GL_VIEWPORT, vp);
		glViewport(wnd->x, rc.bottom - rc.top - (wnd->y + wnd->h), mi, mi);
	}
	glClear(GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glGetDoublev(GL_PROJECTION_MATRIX, &proj);
	glLoadIdentity();
	{
		double dst;
		if(wnd && 0 <= ind && ind < p->nhp)
			dst = .1;
		else
			dst = .5;
		p->fov += (dst - p->fov) * f;
		glFrustum (-p->fov * 1e-3, p->fov * 1e-3, -p->fov * 1e-3, p->fov * 1e-3, 1.5 * 1e-3, 10.);
	}
	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
	glLoadMatrixd(viewer.rot);
	glcullInit(&glc, 1., &viewer.pos, &viewer.irot, -10., 10.);
	VECNULL(dst);
	if(wnd && 0 <= ind && ind < p->nhp){
		VECADDIN(dst, p->hp[ind].pos);
		/*gldTranslaten3dv(arms_s[ind].pos);*/
	}
	{
		avec3_t delta;
		VECSUB(delta, dst, p->focus);
		VECSCALEIN(delta, f);
		VECADDIN(p->focus, delta);
	}
	VECADDIN(viewer.pos, p->focus);
	gldTranslaten3dv(viewer.pos);
	wd.light_on(&wd);
	((struct entity_private_static*)p->pt->vft)->draw(p->pt, &wd);
	wd.light_off(&wd);
	if(wnd && 0 <= ind && ind < p->nhp) for(i = 0; i < p->nhp; i++){
		i == ind ? glColor4ub(0,255,255,255) : glColor4ub(255,255,255,255);
		glRasterPos3dv(p->hp[i].pos);
		gldPutString(p->hp[i].name);
	}
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(proj);
	glMatrixMode(GL_MODELVIEW);

	if(wnd){
		glViewport(vp[0], vp[1], vp[2], vp[3]);
	}
	}

	return 0;
}

static void ArmsShowDestruct(struct glwindow *wnd){
	struct glwindowarms *p = (struct glwindowarms *)wnd;
	if(p->pt)
		free(p->pt);
}

static int ArmsShowMouse(struct glwindow *wnd, int mbutton, int state, int mx, int my){
	struct glwindowarms *p = (struct glwindowarms *)wnd;
	int i, ind = wnd && 300 <= mx && mx <= wnd->w ? (my) / 24 : -1, ammosel = wnd && 310 + 8 * 12 < mx;
	int button;

	button = buttonid(wnd, mx, my);
	if(button)
		ind = -1;

	if(state == GLUT_UP && button == 1){
		memcpy(p->retarms, p->arms, p->nhp * sizeof *p->retarms);
		if(!(wnd->flags & GLW_PINNED))
			wnd->flags |= GLW_TODELETE;
	}
	else if(state == GLUT_UP && button == 2){
		wnd->flags |= GLW_TODELETE;
	}
	else if(state == GLUT_KEEP_DOWN && ind == -1 && 12 < my){
		aquat_t q, q2, qr;
		q[0] = (150 - (my)) / 150. * .5;
		q[1] = q[2] = 0.;
		q[3] = sqrt(1. - q[0] * q[0]);
		q2[0] = 0.;
		q2[1] = -sin(((150 - (mx)) / 150. * M_PI) / 2.);
		q2[2] = 0.;
		q2[3] = sqrt(1. - q2[1] * q2[1]);
		QUATMUL(qr, q, q2);
		QUATZERO(q);
		q[1] = 1.;
		QUATMUL(q2, q, qr);
		QUATCPY(p->rot, q2);
	}
	if(state != GLUT_UP)
		return 1;
	if(wnd && 0 <= ind && ind < p->nhp) if(ammosel){
		if(arms_static[p->arms[ind].type].maxammo)
			p->arms[ind].ammo = (p->arms[ind].ammo + (mbutton ? -1 : 1) + arms_static[p->arms[ind].type].maxammo + 1) % (arms_static[p->arms[ind].type].maxammo + 1);
	}
	else{
		do{
			p->arms[ind].type = (p->arms[ind].type + (mbutton ? -1 : 1) + num_armstype) % num_armstype;
		} while(!(p->hp[ind].clsmask[arms_static[p->arms[ind].type].cls / (sizeof(unsigned) * CHAR_BIT)] & (1 << arms_static[p->arms[ind].type].cls % (sizeof(unsigned) * CHAR_BIT))));
		p->arms[ind].ammo = arms_static[p->arms[ind].type].maxammo;
	}
	return 1;
}

glwindow *ArmsShowWindow(entity_t *creator(warf_t *), double baseprice, size_t armsoffset, arms_t *retarms, struct hardpoint_static *hardpoints, int count){
	glwindow *ret;
	glwindow **ppwnd;
	struct glwindowarms *p;
	int i;
	static const char *windowtitle = "Arms Fitting";
	for(ppwnd = &glwlist; *ppwnd; ppwnd = &(*ppwnd)->next) if((*ppwnd)->title == windowtitle){
		glwActivate(ppwnd);
		return 0;
	}
	p = malloc(sizeof *p);
	ret = &p->st;
	ret->x = 50;
	ret->y = 50;
	ret->w = 500;
	ret->h = 312;
	ret->title = windowtitle;
	ret->flags = GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE;
	ret->modal = NULL;
	ret->draw = ArmsShow;
	ret->mouse = ArmsShowMouse;
	ret->key = NULL;
	ret->destruct = ArmsShowDestruct;
	glwActivate(glwAppend(ret));
/*	memset(p->arms, 0, sizeof p->arms);*/
	p->pt = creator(&armswar);
	p->baseprice = baseprice;
	p->arms = (arms_t*)&((char*)p->pt)[armsoffset];
	p->retarms = retarms;
	memcpy(p->arms, retarms, count * sizeof *retarms);
	p->lastt = 0.;
	p->fov = .5;
	VECNULL(p->focus);
	QUATZERO(p->rot);
	p->rot[1] = 1.;
	p->hp = hardpoints;
	p->nhp = count;
/*	memcpy(p->arms, apachearms_def, sizeof apachearms_def);*/
	return ret;
}





#endif



hardpoint_static *hardpoint_static::load(const char *fname, int &num){
#if 0
	char buf[512];
	UniformLoader *ss;
	ss = ULzopen("rc.zip", fname, UL_TRYFILE);
	if(!ss){
		num = 0;
		return NULL;
	}
	hardpoint_static *ret = NULL;
	struct varlist vl = {0, NULL, calc_mathvars()};
	num = 0;
	while(ULfgets(buf, sizeof buf, ss)){
		int argc, c = 0;
		char *argv[16], *post;
		argc = argtok(argv, buf, &post, numof(argv));
		if(argc < 1)
			continue;

		// definition of identifier in expressions.
		if(argc == 3 && !strcmp(argv[0], "define")){
			struct var *v;
			vl.l = (var*)realloc(vl.l, ++vl.c * sizeof *vl.l);
			v = &vl.l[vl.c-1];
			v->name = (char*)malloc(strlen(argv[1]) + 1);
			strcpy(v->name, argv[1]);
			v->type = var::CALC_D;
			v->value.d = 2 < argc ? calc3(&argv[2], &vl, NULL) : 0.;
			continue;
		}

		hardpoint_static *a = new hardpoint_static[num+1];
		for(int i = 0; i < num; i++)
			a[i] = ret[i];
		if(ret)
			delete[] ret;
		ret = a;

		// tokenize with ,
		for(int i = 0; i < argc; i++) if(char *p = strchr(argv[i], ','))
			*p = '\0';

		hardpoint_static &h = ret[num++];
		for(int i = 0; i < 3; i++)
			h.pos[i] = c < argc ? calc3(&argv[c++], &vl, NULL) : 0;
		for(int i = 0; i < 4; i++)
			h.rot[i] = c < argc ? calc3(&argv[c++], &vl, NULL) : i == 3;
		h.name = new char[c < argc ? strlen(argv[c]) + 1 : 2];
		strcpy(const_cast<char*>(h.name), c < argc ? argv[c++] : "?");
		h.flagmask = c < argc ? atoi(argv[c++]) : 0;
	}
	for(int i = 0; i < vl.c; i++)
		free(vl.l[i].name);
	free(vl.l);
	ULclose(ss);
	return ret;
#endif
	return NULL;
}
