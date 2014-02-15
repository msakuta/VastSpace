/** \file
 * \brief Implementation of Mobile Suit base class.
 */

#include "MobileSuit.h"
#include "draw/effects.h"
#include "GundamCommands.h"
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

double MobileSuit::defaultMass = 4e3;
double MobileSuit::maxHealthValue = 100.;
HSQOBJECT MobileSuit::sqPopupMenu = sq_nullobj();
HSQOBJECT MobileSuit::sqCockpitView = sq_nullobj();
StaticBindDouble MobileSuit::deathSmokeFreq = 20.; ///< Smokes per second
StaticBindDouble MobileSuit::bulletSpeed = 2.;
StaticBindDouble MobileSuit::walkSpeed = .03;
StaticBindDouble MobileSuit::airMoveSpeed = .2;
StaticBindDouble MobileSuit::torqueAmount = .1;
StaticBindDouble MobileSuit::floorProximityDistance = .05;
StaticBindDouble MobileSuit::floorTouchDistance = .02;
StaticBindDouble MobileSuit::standUpTorque = (6e-4 * 5e4);
StaticBindDouble MobileSuit::standUpFeedbackTorque = 1e1;
StaticBindDouble MobileSuit::randomVibration = .15;


const Vec3d MobileSuit::thrusterDir[7] = {
	Vec3d(0, -1, 0),
	Vec3d(0, -1, 0),
	Vec3d(0, -1, 0),
	Vec3d(1, 0, 0),
	Vec3d(-1, 0, 0),
	Vec3d(0, -1, 0),
	Vec3d(0, -1, 0),
};


Entity::Dockable *MobileSuit::toDockable(){return this;}

/* color sequences */
//extern const struct color_sequence cs_orangeburn, cs_shortburn;





typedef std::map<gltestp::dstring, StaticBind *> StaticBindSet;

SQInteger MobileSuit::sqf_get(HSQUIRRELVM v){
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

HSQUIRRELVM MobileSuit::sqvm;

static StaticBindSet staticBind;



void MobileSuit::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << aac; /* angular acceleration */
	sc.o << throttle;
	sc.o << fuel;
	sc.o << dest;
	sc.o << fcloak;
	sc.o << waverider;
	sc.o << fwaverider;
	sc.o << weapon;
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
	sc.o << coverRight;

	sc.o << (int)weaponStatus.size();
	for(int i = 0; i < weaponStatus.size(); ++i){
		sc.o << weaponStatus[i].cooldown;
		sc.o << weaponStatus[i].reload;
		sc.o << weaponStatus[i].magazine;
		sc.o << weaponStatus[i].ammo;
	}
}

void MobileSuit::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> aac; /* angular acceleration */
	sc.i >> throttle;
	sc.i >> fuel;
	sc.i >> dest;
	sc.i >> fcloak;
	sc.i >> waverider;
	sc.i >> fwaverider;
	sc.i >> weapon;
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
	sc.i >> coverRight;

	int size;
	sc.i >> size;
	weaponStatus.resize(size);
	for(int i = 0; i < size; ++i){
		sc.i >> weaponStatus[i].cooldown;
		sc.i >> weaponStatus[i].reload;
		sc.i >> weaponStatus[i].magazine;
		sc.i >> weaponStatus[i].ammo;
	}

	// Re-create temporary entities if flying in a WarSpace. If environment is a WarField, don't restore.
	WarSpace *ws;
	if(w && (ws = (WarSpace*)w))
		pf = ws->tepl->addTefpolMovable(this->pos, this->velo, avec3_000, &cs_orangeburn, TEP3_THICK | TEP3_ROUGH, cs_orangeburn.t);
	else
		pf = NULL;
}

const char *MobileSuit::dispname()const{
	return "MobileSuit";
};

double MobileSuit::getMaxHealth()const{
	return maxHealthValue;
}





MobileSuit::MobileSuit(Game *game) : 
	st(game),
	mother(NULL),
	paradec(-1),
	vulcanSoundCooldown(0.f),
	twist(0.f),
	pitch(0.f),
	fsabre(0.f),
	fonfeet(0.f),
	walkphase(0.f),
	pDelta(0,0,0),
	pDeltaTime(0)
{
	muzzleFlash[0] = 0.;
	muzzleFlash[1] = 0.;
	for(int i = 0; i < numof(thrusterDirs); i++){
		thrusterDirs[i] = thrusterDir[i];
		thrusterPower[i] = 0.;
	}
}

MobileSuit::MobileSuit(WarField *aw) : st(aw),
	mother(NULL),
	task(Auto),
	standingUp(false),
	reverser(0),
	waverider(false),
	fwaverider(0.),
	weapon(0),
	fweapon(0.),
	vulcanSoundCooldown(0.f),
	twist(0.f),
	pitch(0.f),
	paradec(-1),
	forcedEnemy(false),
	formPrev(NULL),
	evelo(vec3_000),
	attitude(Passive),
	fsabre(0.f),
	fonfeet(0.f),
	coverRight(0.f),
	walkphase(0.f),
	stabilizer(true),
	pDelta(0,0,0),
	pDeltaTime(0)
{
	aimdir[0] = 0.;
	aimdir[1] = 0.;
	muzzleFlash[0] = 0.;
	muzzleFlash[1] = 0.;
	for(int i = 0; i < numof(thrusterDirs); i++){
		thrusterDirs[i] = thrusterDir[i];
		thrusterPower[i] = 0.;
	}
	MobileSuit *const p = this;
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

void MobileSuit::init(){
	// Initialize weapon status by acquired parameters.
	int count = getWeaponCount();
	weaponStatus.resize(count);
	for(int i = 0; i < count; ++i){
		weaponStatus[i].cooldown = 0;
		weaponStatus[i].reload = 0;
		WeaponParams param;
		if(getWeaponParams(i, param)){
			weaponStatus[i].magazine = param.magazineSize;
			weaponStatus[i].ammo = param.maxAmmo;
		}
		else{
			weaponStatus[i].magazine = 0;
			weaponStatus[i].ammo = 0;
		}
	}

	fuel = maxfuel();
}

void MobileSuit::control(const input_t *inputs, double dt){
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

Entity::Props MobileSuit::props()const{
	Props ret = st::props();
	ret.push_back(gltestp::dstring("Task: ") << task);
	ret.push_back(gltestp::dstring("Fuel: ") << fuel << '/' << maxfuel());
	ret.push_back(gltestp::dstring("Throttle: ") << throttle);
	ret.push_back(gltestp::dstring("Stabilizer: ") << stabilizer);
	return ret;
}

bool MobileSuit::undock(Docker *d){
	st::undock(d);
	task = Undock;
	mother = d;
	if(this->pf)
		this->pf->immobilize();
	WarSpace *ws;
	if(w && w->getTefpol3d())
		this->pf = w->getTefpol3d()->addTefpolMovable(this->pos, this->velo, avec3_000, &cs_orangeburn, TEP3_THICK | TEP3_ROUGH, cs_orangeburn.t);
	d->baycool += 1.;
	return true;
}


// find the nearest enemy
bool MobileSuit::findEnemy(){
	if(forcedEnemy)
		return !!enemy;
	Entity *pt2, *closest = NULL;
	double best = 1e2 * 1e2;
	for(WarField::EntityList::iterator it = w->el.begin(); it != w->el.end(); ++it){
		Entity *pt2 = *it;

		if(!(pt2->isTargettable() && pt2 != this && pt2->w == w && pt2->getHealth() > 0. && pt2->race != -1 && pt2->race != this->race))
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



void MobileSuit::steerArrival(double dt, const Vec3d &atarget, const Vec3d &targetvelo, double speedfactor, double minspeed){
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
	double rotationSpeed = getRotationSpeed();
	if(rotationSpeed * rotationSpeed < this->omg.slen())
		this->omg.normin().scalein(rotationSpeed);
	bbody->setAngularVelocity(btvc(this->omg));
	dr.normin();
	Vec3d sidevelo = velo - dr * dr.sp(velo);
	bbody->applyCentralForce(btvc(-sidevelo * mass));
//	this->rot = this->rot.quatrotquat(this->omg * dt);
}

// Find mother if has none
Entity *MobileSuit::findMother(){
	Entity *pm = NULL;
	double best = 1e10 * 1e10, sl;
	for(WarField::EntityList::iterator it = w->entlist().begin(); it != w->entlist().end(); ++it){
		Entity *e = *it;
		if(e->race == race && e->getDocker() && (sl = (e->pos - this->pos).slen()) < best){
			mother = e->getDocker();
			pm = mother->e;
			best = sl;
		}
	}
	return pm;
}

Quatd MobileSuit::aimRot()const{
	return Quatd::rotation(aimdir[1], 0, -1, 0) * Quatd::rotation(aimdir[0], -1, 0, 0);
}

void MobileSuit::anim(double dt){
	WarField *oldw = w;
	Entity *pt = this;
	MobileSuit *const pf = this;
	MobileSuit *const p = this;
	Entity *pm = mother && mother->e ? mother->e : NULL;
//	scarry_t *const mother = pf->mother;
	Mat4d mat, imat;

	if(!w)
		return;

	if(Docker *docker = *w){
		fuel = min(fuel + dt * 20., maxfuel()); // it takes 6 seconds to fully refuel
		health = min(health + dt * 100., getMaxHealth()); // it takes 7 seconds to be fully repaired
		if(fuel == maxfuel() && health == getMaxHealth() && !docker->remainDocked)
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
		pf->pf->move(pt->pos + pt->rot.trans(Vec3d(0,0,.005)), avec3_000, cs_orangeburn.t, 0/*pf->docked*/);
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
	if(pt->enemy && pt->enemy->getHealth() <= 0.)
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

	if(0 < pt->getHealth()){
//		double oldyaw = pt->pyr[1];
		bool controlled = controller;
//		int parking = 0;
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
		btVector3 btdown;

		// Check if we are touching the ground.
		if(!btaccel.isZero()) do{
			btdown = btaccel.normalized();
			const btVector3 &btpos = bbody->getWorldTransform().getOrigin();
			const btVector3 &from = btpos;
			const btVector3 &btvelo = bbody->getLinearVelocity();
			btScalar closingSpeed = btvelo.dot(btdown);
			const btVector3 to = btpos + btaccel.normalized() * (closingSpeed < floorProximityDistance ? floorProximityDistance : closingSpeed);

			btCollisionWorld::ClosestRayResultCallback rayCallback(from, to);

			ws->bdw->rayTest(from, to, rayCallback);

			if(rayCallback.hasHit()){
				const btRigidBody *body = btRigidBody::upcast(rayCallback.m_collisionObject);
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
					const btCollisionObject* obA = static_cast<const btCollisionObject*>(contactManifold->getBody0());
					const btCollisionObject* obB = static_cast<const btCollisionObject*>(contactManifold->getBody1());

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
			Vec3d delta = getDelta();

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
			else if(!controlled){
				AIUpdate(dt, floorTouched);
			}

			if(task == CoverRight){
				const HitBoxList &hitboxes = getHitBoxes();
				if(0 < hitboxes.size()){
					Vec3d delta = coverPoint.pos + coverPoint.rot.trans(Vec3d(-.001 + aimdir[1] * -.015 / M_PI, hitboxes[0].sc[1], .01)) - pos;
					bbody->applyCentralForce(btvc((delta * 5. - velo * 2.) * mass));
					btTransform wt = bbody->getWorldTransform();
					wt.setRotation(wt.getRotation().slerp(btqc(coverPoint.rot), dt));
					bbody->setWorldTransform(wt);
				}
			}

			if(pt->inputs.press & (PL_ENTER | PL_LCLICK) && (task != CoverRight || coverRight == 2.)){
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

			int weaponCount = getWeaponCount();
			for(int i = 0; i < weaponCount; ++i){
				WeaponStatus &wst = weaponStatus[i];
				wst.cooldown = approach(wst.cooldown, 0., dt, 0.);
				wst.reload = approach(wst.reload, 0., dt, 0.);
			}
			vulcanSoundCooldown = approach(vulcanSoundCooldown, 0., dt, 0.);

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
		if(task != CoverRight){
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
			fuel = approach(fuel, maxfuel(), getFuelRegenRate() * dt, 0.);

			double spd = .025/*(controlled ? .025 : p->task != Attack ? .01 : .005)*/;
//			Vec3d acc = (this->rot.rotate(thrustvector * M_PI / 2., /*this->rot.trans*/(Vec3d(1,0,0)))).trans(Vec3d(0., 0., -1.));
			Vec3d acc = desiredAccel;
			bbody->applyCentralImpulse(btvc(acc * spd * .001 * mass));
//			pt->velo += acc * spd;
		}

#if 0
		if(model){
			static const Quatd rotaxis(0, 1., 0., 0.);
			MotionPoseSet &v = motionInterpolate();
			Vec3d accel = btvc(bbody->getTotalForce() * bbody->getInvMass() / dt);
			Vec3d relpos; // Relative position to gravitational center
			// Torque in Bullet dynamics engine. 
			Vec3d bttorque = btvc(bbody->getTotalTorque() / dt /* bbody->getInvInertiaTensorWorld()*/);
			Quatd lrot; // Rotational component of transformation from world coordinates to local coordinates.
			for(int i = 0; i < numof(thrusterDirs); i++) if(model->getBonePos(gltestp::dstring("MobileSuit_thruster") << i, v[0], &relpos, &lrot)){
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
//			YSDNM_MotionInterpolateFree(v);
//			delete pv;
			motionInterpolateFree(v);
		}
#endif

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
			const double pitchrange = task == CoverRight ? M_PI / 4. : M_PI / 2.;
			const double yawrange = task == CoverRight ? M_PI / 4. : M_PI / 3.;
			double dpitch = inputs.analog[1] * game->player->fov * 2e-3;
			double dyaw = inputs.analog[0] * game->player->fov * 2e-3;
			aimdir[0] = approach(aimdir[0], rangein(aimdir[0] + dpitch, -pitchrange, pitchrange), M_PI * dt, 0);
			aimdir[1] = approach(aimdir[1], rangein(aimdir[1] + dyaw, -yawrange, yawrange), M_PI * dt, 0);
		}
		if(3. <= fsabre)
			fsabre = 1.;
		fsabre = approach(fsabre, weapon == 3 ? 3. : 0., dt, 0.);
		coverRight = approach(coverRight, task == CoverRight ? inputs.press & (PL_ENTER | PL_LCLICK) ? 2. : 1. : 0., coverRight <= 1. ? dt : 3. * dt, 0.);
	}
	else{
		bbody->activate();
		health += dt;
		if(0. < health){
			Teline3List *tell = w->getTeline3d();
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
			Teline3List *tell = w->getTeline3d();
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
		this->pf->immobilize();
//	movesound3d(pf->hitsound, pt->pos);
}

// Docking and undocking will never stack.
bool MobileSuit::solid(const Entity *o)const{
	return !(task == Undock || task == Dock);
}

int MobileSuit::takedamage(double damage, int hitpart){
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

bool MobileSuit::isTargettable()const{
	return true;
}
bool MobileSuit::isSelectable()const{return true;}

double MobileSuit::getHitRadius()const{
	return .01;
}

bool MobileSuit::command(EntityCommand *com){
	if(InterpretCommand<HaltCommand>(com)){
		task = Idle;
		forcedEnemy = false;
	}
/*	else if(InterpretCommand<DeltaCommand>(com)){
		Entity *e;
		for(e = next; e; e = e->next) if(e->classname() == classname() && e->race == race){
			formPrev = static_cast<MobileSuit*>(e);
			task = DeltaFormation;
			break;
		}
		return true;
	}*/
	else if(TransformCommand *tc = InterpretCommand<TransformCommand>(com)){
		waverider = !!tc->formid;
		if(bbody)
			bbody->setCollisionShape(waverider && getWaveRiderShape() ? getWaveRiderShape() : getShape());
	}
	else if(WeaponCommand *wc = InterpretCommand<WeaponCommand>(com)){
		weapon = wc->weaponid;
	}
	else if(StabilizerCommand *sc = InterpretCommand<StabilizerCommand>(com)){
		stabilizer = sc->stabilizer;
	}
	else if(GetCoverCommand *gc = InterpretCommand<GetCoverCommand>(com)){
		if(gc->v == 2)
			gc->v = task != CoverRight;
		if(gc->v){
			GetCoverPointsMessage gcpm;
			gcpm.org = pos;
			gcpm.radius = .3;
			if(w->sendMessage(gcpm) && gcpm.cpv.size()){
				double best = .1 * .1;
				CoverPointVector::iterator it = gcpm.cpv.begin();
				for(; it != gcpm.cpv.end(); it++) if(it->type == it->RightEdge){
					double sdist = (it->pos - pos).slen();
					if(sdist < best){
						coverPoint = *it;
						best = sdist;
						task = CoverRight;
					}
				}
			}
		}
		else
			task = Auto;
	}
	else if(InterpretCommand<ReloadCommand>(com)){
		reloadWeapon();
		return true;
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

Vec3d MobileSuit::getDelta()const{
	// If we have calculated the delta vector in this frame, reuse the result
	if(pDeltaTime == game->universe->global_time)
		return pDelta;

//			int i, n;
//			Vec3d opos;
//			pt->enemy = pm->enemy;
	if(enemy /*&& VECSDIST(pt->pos, pm->pos) < 15. * 15.*/){
		double sp;
/*				const avec3_t guns[2] = {{.002, .001, -.005}, {-.002, .001, -.005}};*/
		Vec3d xh, dh, vh;
		Vec3d epos;
		double phi;
		estimate_pos(epos, enemy->pos, enemy->velo, this->pos, this->velo, bulletSpeed, w);
/*				xh[0] = pt->velo[2];
		xh[1] = pt->velo[1];
		xh[2] = -pt->velo[0];*/
		pDelta = epos - this->pos;

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
	}

	pDeltaTime = game->universe->global_time;
	return pDelta;
}

void MobileSuit::AIUpdate(double dt, bool floorTouched){
	WarSpace *ws = *w;
	MobileSuit *pt = this;
	Entity *pm = mother && mother->e ? mother->e : NULL;
	Vec3d delta = getDelta();
	Mat4d mat;
	transform(mat);

	bool trigger = true;
	if(pt->enemy){
		double dist = (enemy->pos - this->pos).len();

		// Do not try shooting at very small target, that's just waste of ammo.
		if(enemy->getHitRadius() < dist / 300.)
			trigger = false;
	}
/*			else
	{
		delta = pm->pos - pt->pos;
		trigger = 0;
	}*/

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
			task = Attack;
		}
		else if(findEnemy()){
			task = Attack;
		}
		if(!enemy || (task == Auto || task == Parade)){
			if(pm)
				task = Parade;
			else
				task = Auto;
		}
	}
	else if(task == Moveto){
		Vec3d dr = pt->pos - dest;
		if(dr.slen() < .01 * .01){
			throttle = 0.;
//			parking = 1;
			pt->velo += dr * -dt * .5;
			task = Idle;
		}
		else{
			steerArrival(dt, dest, vec3_000, 1. / 2., .0);
		}
	}
	else if(enemy && (task == Attack || task == Away || task == Rest)){
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
					const btRigidBody *body = btRigidBody::upcast(rayCallback.m_collisionObject);
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
			double sx, sy, len, len2, maxspeed = getMaxAngleSpeed() * dt;
			Quatd qres, qrot;

			// If a mother could not be aquired, fight to the death alone.
			if(fuel < 30. && (pm || (pm = findMother()))){
				task = Dockque;
				return;
			}

			double dist = delta.len();
			double awaybase = pt->enemy->getHitRadius() * 3. + .1;
			if(.6 < awaybase)
				awaybase = pt->enemy->getHitRadius() + 1.; // Constrain awaybase for large targets
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
			throttle = 1.;

			// Randomly vibrates to avoid bullets
/*						if(0 < fuel && !floorTouched){
				RandomSequence rs((unsigned long)this ^ (unsigned long)(w->war_time() / .1));
				Vec3d randomvec;
				for(int i = 0; i < 3; i++)
					randomvec[i] = rs.nextd() - .5;
				bbody->applyCentralForce(btvc(randomvec * mass * randomVibration));
			}*/

			if((task == Attack || task == Rest) /*|| forward.sp(dv) < -.5*/){
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
				aac.clear();
			}
		}
	}
	else if(!pt->enemy && (task == Attack || task == Rest || task == Away)){
		task = Parade;
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
				const btRigidBody *body = btRigidBody::upcast(rayCallback.m_collisionObject);
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

	if(task == Parade){
		if(mother){
			if(paradec == -1)
				paradec = mother->enumParadeC(mother->Fighter);
			Vec3d target, target0(-1., 0., -1.);
			Quatd q2, q1;
			target0[0] += paradec % 10 * -.05;
			target0[2] += paradec / 10 * -.05;
			target = pm->rot.trans(target0);
			target += pm->pos;
			Vec3d dr = pt->pos - target;
			if(dr.slen() < .01 * .01){
				q1 = pm->rot;
				throttle = 0.;
//				parking = 1;
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
			if(1. < throttle)
				throttle = 1.;
		}
		else
			task = Auto;
	}
	else if(task == DeltaFormation){
		int nwingmen = 1; // Count yourself
		MobileSuit *leader = NULL;
		for(MobileSuit *wingman = formPrev; wingman; wingman = wingman->formPrev){
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
				throttle = 0.;
//				parking = 1;
				pt->velo += dr * (-dt * .5);
				pt->rot = Quatd::slerp(pt->rot, q1, 1. - exp(-dt));
			}
			else
				steerArrival(dt, dest, leader->velo, 1., .001);
		}
	}
	else if(task == Dockque || task == Dock){
		if(!pm)
			pm = findMother();

		// It is possible that no one is glad to become a mother.
		if(!pm)
			task = Auto;
		else{
			Vec3d target0(pm->getDocker()->getPortPos(this));
			Quatd q2, q1;
//			collideignore = pm;

			// Mask to avoid collision with the mother
			if(bbody)
				bbody->getBroadphaseProxy()->m_collisionFilterMask &= ~2;

			// Runup length
			if(task == Dockque)
				target0 += pm->getDocker()->getPortRot(this).trans(Vec3d(0, 0, -.3));

			// Suppress side slips
			Vec3d sidevelo = velo - mat.vec3(2) * mat.vec3(2).sp(velo);
			bbody->applyCentralForce(btvc(-sidevelo * mass));

			Vec3d target = pm->rot.trans(target0);
			target += pm->pos;
			steerArrival(dt, target, pm->velo, task == Dockque ? 1. / 2. : -mat.vec3(2).sp(velo) < 0 ? 1. : .025, .01);
			double dist = (target - this->pos).len();
			if(dist < .01){
				if(task == Dockque)
					task = Dock;
				else{
					mother->dock(pt);
					if(this->pf){
						this->pf->immobilize();
						this->pf = NULL;
					}
					this->docked = true;
/*							if(mother->cargo < scarry_cargousage(mother)){
						scarry_undock(mother, pt, w);
					}*/
					return;
				}
			}
			if(1. < throttle)
				throttle = 1.;
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
}

SQInteger MobileSuit::sqGet(HSQUIRRELVM v, const SQChar *name)const{
	if(!scstrcmp(name, _SC("waverider"))){
		sq_pushbool(v, waverider);
		return 1;
	}
	else if(!scstrcmp(name, _SC("stabilizer"))){
		sq_pushbool(v, stabilizer);
		return 1;
	}
	else if(!scstrcmp(name, _SC("coverRight"))){
		sq_pushfloat(v, coverRight);
		return 1;
	}
	else if(!scstrcmp(name, _SC("weapon"))){
		sq_pushinteger(v, weapon);
		return 1;
	}
	else if(!scstrcmp(name, _SC("aimdir"))){
		sq_newarray(v, 0);
		for(int i = 0; i < numof(aimdir); i++){
			sq_pushfloat(v, aimdir[i]);
			sq_arrayappend(v, -2);
		}
		return 1;
	}
	else
		return st::sqGet(v, name);
}

SQInteger MobileSuit::sqSet(HSQUIRRELVM v, const SQChar *name){
	if(!scstrcmp(name, _SC("waverider")))
		return boolSetter(v, _SC("waverider"), waverider);
	else if(!scstrcmp(name, _SC("stabilizer")))
		return boolSetter(v, _SC("stabilizer"), stabilizer);
	else if(!scstrcmp(name, _SC("coverRight"))){
		SQFloat f;
		if(SQ_FAILED(sq_getfloat(v, 3, &f)))
			return sq_throwerror(v, gltestp::dstring("could not convert to float ") << name);
		coverRight = f;
		return 0;
	}
	else if(!scstrcmp(name, _SC("weapon"))){
		SQInteger i;
		if(SQ_FAILED(sq_getinteger(v, 3, &i)))
			return sq_throwerror(v, gltestp::dstring("could not convert to integer ") << name);
		this->weapon = i;
		return 0;
	}
	else
		return st::sqSet(v, name);
}

SQInteger MobileSuit::boolSetter(HSQUIRRELVM v, const SQChar *name, bool &value){
	SQBool b;
	if(SQ_FAILED(sq_getbool(v, 3, &b)))
		return sq_throwerror(v, gltestp::dstring("could not convert to bool ") << name);
	value = b != SQFalse;
	return 0;
}

void MobileSuit::reloadWeapon(){
	WeaponParams param;
	if(!getWeaponParams(weapon, param))
		return;
	WeaponStatus &wst = weaponStatus[weapon];
	if(wst.magazine < param.magazineSize && (0 == param.maxAmmo || param.magazineSize < wst.ammo)){
		wst.magazine = param.magazineSize;
		wst.cooldown = wst.reload = param.reloadTime;
		wst.ammo -= param.magazineSize;
	}
}

/*Entity *MobileSuit::create(WarField *w, Builder *mother){
	MobileSuit *ret = new MobileSuit(NULL);
	ret->pos = mother->pos;
	ret->velo = mother->velo;
	ret->rot = mother->rot;
	ret->omg = mother->omg;
	ret->race = mother->race;
	ret->task = Undock;
//	w->addent(ret);
	return ret;
}*/



double MobileSuit::pid_pfactor = 1.;
double MobileSuit::pid_ifactor = 1.;
double MobileSuit::pid_dfactor = 1.;









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

IMPLEMENT_COMMAND(GetCoverCommand, "GetCover");

GetCoverCommand::GetCoverCommand(HSQUIRRELVM v, Entity &){
	int argc = sq_gettop(v);
	if(argc < 2)
		throw SQFArgumentError();
	SQInteger i;
	if(SQ_FAILED(sq_getinteger(v, 3, &i)))
		throw SQFError(_SC("Invalid argument type"));
	this->v = i;
}

IMPLEMENT_COMMAND(ReloadCommand, "Reload");

