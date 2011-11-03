/** \file
 * \brief Implementation of Sceptor class.
 */
#include "sceptor.h"
#include "Player.h"
#include "Bullet.h"
#include "judge.h"
#include "serial_util.h"
#include "Warpable.h"
#include "Docker.h"
#include "Scarry.h"
#include "draw/material.h"
//#include "worker.h"
//#include "glsl.h"
//#include "astro_star.h"
//#include "sensor.h"
#include "cmd.h"
#include "astrodraw.h"
#include "EntityCommand.h"
#include "btadapt.h"
#include "sqadapt.h"
#include "draw/effects.h"
#include "motion.h"
#include "Game.h"
#include "glw/popup.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/wavsound.h>
#include <clib/zip/UnZip.h>
}
#include <assert.h>
#include <string.h>


/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289

#define SCEPTOR_SCALE 1./10000
#define SCEPTOR_SMOKE_FREQ 20.
#define SCEPTOR_COOLTIME .2
#define SCEPTOR_ROLLSPEED (.2 * M_PI)
#define SCEPTOR_ROTSPEED (.3 * M_PI)
#define SCEPTOR_MAX_ANGLESPEED (M_PI * .5)
#define SCEPTOR_ANGLEACCEL (M_PI * .2)
#define SCEPTOR_MAX_GIBS 20
#define BULLETSPEED 2.
#define SCEPTOR_MAGAZINE 10
#define SCEPTOR_RELOADTIME 2.



Entity::Dockable *Sceptor::toDockable(){return this;}

/* color sequences */
extern const struct color_sequence cs_orangeburn, cs_shortburn;


struct hitbox Sceptor::hitboxes[] = {
	hitbox(Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(.005, .002, .003)),
};
const int Sceptor::nhitboxes = numof(Sceptor::hitboxes);

/*static const struct hitbox sceptor_hb[] = {
	hitbox(Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(.005, .002, .003)),
};*/

const char *Sceptor::idname()const{
	return "sceptor";
}

const char *Sceptor::classname()const{
	return "Sceptor";
}

const unsigned Sceptor::classid = registerClass("Sceptor", Conster<Sceptor>);
Entity::EntityRegister<Sceptor> Sceptor::entityRegister("Sceptor");

void Sceptor::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << aac; /* angular acceleration */
	sc.o << throttle;
	sc.o << fuel;
	sc.o << cooldown;
	sc.o << dest;
	sc.o << fcloak;
	sc.o << heat;
	sc.o << mother; // Mother ship
//	sc.o << hitsound;
	sc.o << paradec;
	sc.o << (int)task;
	sc.o << docked << returning << away << cloak << forcedEnemy;
	sc.o << formPrev;
}

void Sceptor::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> aac; /* angular acceleration */
	sc.i >> throttle;
	sc.i >> fuel;
	sc.i >> cooldown;
	sc.i >> dest;
	sc.i >> fcloak;
	sc.i >> heat;
	sc.i >> mother; // Mother ship
//	sc.i >> hitsound;
	sc.i >> paradec;
	sc.i >> (int&)task;
	sc.i >> docked >> returning >> away >> cloak >> forcedEnemy;
	sc.i >> formPrev;

	// Re-create temporary entities if flying in a WarSpace. If environment is a WarField, don't restore.
	WarSpace *ws;
	if(w && (ws = (WarSpace*)w))
		pf = AddTefpolMovable3D(ws->tepl, this->pos, this->velo, avec3_000, &cs_orangeburn, TEP3_THICK | TEP3_ROUGH, cs_orangeburn.t);
	else
		pf = NULL;
}

const char *Sceptor::dispname()const{
	return "Interceptor";
};

double Sceptor::maxhealth()const{
	return 200.;
}





Sceptor::Sceptor() : mother(NULL), mf(0), paradec(-1){
}

Sceptor::Sceptor(WarField *aw) : st(aw),
	mother(NULL),
	task(Auto),
	fuel(maxfuel()),
	reverser(0),
	mf(0),
	paradec(-1),
	forcedEnemy(false),
	formPrev(NULL),
	evelo(vec3_000),
	attitude(Passive)
{
	Sceptor *const p = this;
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
	p->magazine = SCEPTOR_MAGAZINE;
	p->fcloak = 0.;
	p->cloak = 0;
	p->heat = 0.;
	integral[0] = integral[1] = 0.;
}

const avec3_t Sceptor::gunPos[2] = {{35. * SCEPTOR_SCALE, -4. * SCEPTOR_SCALE, -15. * SCEPTOR_SCALE}, {-35. * SCEPTOR_SCALE, -4. * SCEPTOR_SCALE, -15. * SCEPTOR_SCALE}};

void Sceptor::cockpitView(Vec3d &pos, Quatd &q, int seatid)const{
	Player *ppl = w->pl;
	Vec3d ofs;
	static const Vec3d src[3] = {Vec3d(0., .001, -.002), Vec3d(0., .008, 0.020), Vec3d(0., .008, .020)};
	Mat4d mat;
	seatid = (seatid + 3) % 3;
	if(seatid == 2 && enemy && enemy->w == w){
		q = this->rot * Quatd::direction(this->rot.cnj().trans(this->pos - enemy->pos));
		ofs = q.trans(Vec3d(src[seatid][0], src[seatid][1], src[seatid][2] / ppl->fov)); // Trackback if zoomed
	}
	else{
		q = this->rot;
		ofs = q.trans(src[seatid]);
	}
	pos = this->pos + ofs;
}

/*static void SCEPTOR_control(entity_t *pt, warf_t *w, input_t *inputs, double dt){
	SCEPTOR_t *p = (SCEPTOR_t*)pt;
	if(!pt->active || pt->health <= 0.)
		return;
	pt->inputs = *inputs;
}*/

int Sceptor::popupMenu(PopupMenu &list){
	int ret = st::popupMenu(list);
	list.append(sqa_translate("Dock"), 0, "dock")
		.append(sqa_translate("Military Parade Formation"), 0, "parade_formation")
		.append(sqa_translate("Cloak"), 0, "cloak")
		.append(sqa_translate("Delta Formation"), 0, "delta_formation");
	return ret;
}

Entity::Props Sceptor::props()const{
	Props ret = st::props();
//	ret.push_back(cpplib::dstring("Task: ") << task);
//	ret.push_back(cpplib::dstring("Fuel: ") << fuel << '/' << maxfuel());
	return ret;
}

bool Sceptor::undock(Docker *d){
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
void Sceptor::shootDualGun(double dt){
	Vec3d velo, gunpos, velo0(0., 0., -BULLETSPEED);
	Mat4d mat;
	int i = 0;
	if(dt <= cooldown)
		return;
	transform(mat);
	do{
		Bullet *pb;
		double phi, theta;
		pb = new Bullet(this, 5, 15.);
		w->addent(pb);
		pb->pos = mat.vp3(gunPos[i]);
/*		phi = pt->pyr[1] + (drseq(&w->rs) - .5) * .005;
		theta = pt->pyr[0] + (drseq(&w->rs) - .5) * .005;
		VECCPY(pb->velo, pt->velo);
		pb->velo[0] +=  BULLETSPEED * sin(phi) * cos(theta);
		pb->velo[1] += -BULLETSPEED * sin(theta);
		pb->velo[2] += -BULLETSPEED * cos(phi) * cos(theta);*/
		pb->velo = mat.dvp3(velo0);
		pb->velo += this->velo;
		pb->life = 3.;
		this->heat += .025;
	} while(!i++);
//	shootsound(pt, w, p->cooldown);
//	pt->shoots += 2;
	if(0 < --magazine)
		this->cooldown += SCEPTOR_COOLTIME * (fuel <= 0 ? 3 : 1);
	else{
		magazine = SCEPTOR_MAGAZINE;
		this->cooldown += SCEPTOR_RELOADTIME;
	}
	this->mf = .1;
}

// find the nearest enemy
bool Sceptor::findEnemy(){
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



void Sceptor::steerArrival(double dt, const Vec3d &atarget, const Vec3d &targetvelo, double speedfactor, double minspeed){
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
	if(SCEPTOR_ROTSPEED * SCEPTOR_ROTSPEED < this->omg.slen())
		this->omg.normin().scalein(SCEPTOR_ROTSPEED);
	bbody->setAngularVelocity(btvc(this->omg));
	dr.normin();
	Vec3d sidevelo = velo - dr * dr.sp(velo);
	bbody->applyCentralForce(btvc(-sidevelo * mass));
//	this->rot = this->rot.quatrotquat(this->omg * dt);
}

// Find mother if has none
Entity *Sceptor::findMother(){
	Entity *pm = NULL;
	double best = 1e10 * 1e10, sl;
	for(Entity *e = w->entlist(); e; e = e->next) if(e->race == race && e->getDocker() && (sl = (e->pos - this->pos).slen()) < best){
		mother = e->getDocker();
		pm = mother->e;
		best = sl;
	}
	return pm;
}

void Sceptor::enterField(WarField *target){
	WarSpace *ws = *target;
	if(ws && ws->bdw){
		static btCompoundShape *shape = NULL;
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

		//add the body to the dynamics world
		ws->bdw->addRigidBody(bbody, 1, ~2);
	}
}

void Sceptor::anim(double dt){
	WarField *oldw = w;
	Entity *pt = this;
	Sceptor *const pf = this;
	Sceptor *const p = this;
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

	// Do not follow remote mothership. Probably you'd better finding another mothership.
	if(mother != w && pm && pm->w != w){
		mother = NULL;
		pm = NULL;
	}

	if(bbody){
		const btTransform &tra = bbody->getCenterOfMassTransform();
		pos = btvc(tra.getOrigin());
		rot = btqc(tra.getRotation());
		velo = btvc(bbody->getLinearVelocity());
	}

/*	if(!mother){
		if(p->pf){
			ImmobilizeTefpol3D(p->pf);
			p->pf = NULL;
		}
		pt->active = 0;
		return;
	}*/

	if(pf->pf)
		MoveTefpol3D(pf->pf, pt->pos + pt->rot.trans(Vec3d(0,0,.005)), avec3_000, cs_orangeburn.t, 0/*pf->docked*/);

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

	if(0 < pt->health){
//		double oldyaw = pt->pyr[1];
		bool controlled = controller;
		int parking = 0;
		Entity *collideignore = NULL;

		// Default is to collide
		if(bbody)
			bbody->getBroadphaseProxy()->m_collisionFilterMask |= 2;

		if(!controlled)
			pt->inputs.press = 0;
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
				estimate_pos(epos, enemy->pos, enemy->velo, pt->pos, pt->velo, BULLETSPEED, w);
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
				if(enemy->hitradius() < dist / 100.)
					trigger = 0;

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
			else if(!controller) do{
				if((task == Attack || task == Away) && !pt->enemy || task == Auto || task == Parade){
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
				else if(pt->enemy && (p->task == Attack || p->task == Away)){
					Vec3d dv, forward;
					Vec3d xh, yh;
					double sx, sy, len, len2, maxspeed = SCEPTOR_MAX_ANGLESPEED * dt;
					Quatd qres, qrot;

					// If a mother could not be aquired, fight to the death alone.
					if(p->fuel < 30. && (pm || (pm = findMother()))){
						p->task = Dockque;
						break;
					}

					double dist = delta.len();
					dv = delta;
					double awaybase = pt->enemy->hitradius() * 3. + .1;
					if(.6 < awaybase)
						awaybase = pt->enemy->hitradius() + 1.; // Constrain awaybase for large targets
					double attackrad = awaybase < .6 ? awaybase * 5. : awaybase + 4.;
					if(p->task == Attack && dist < awaybase){
						p->task = Away;
					}
					else if(p->task == Away && attackrad < dist){
						p->task = Attack;
					}
					dv.normin();
					forward = pt->rot.trans(avec3_001);
					if(p->task == Attack)
						forward *= -1;
		/*				sx = VECSP(&mat[0], dv);
					sy = VECSP(&mat[4], dv);
					pt->inputs.press |= (sx < 0 ? PL_4 : 0 < sx ? PL_6 : 0) | (sy < 0 ? PL_2 : 0 < sy ? PL_8 : 0);*/
					p->throttle = 1.;

					// Randomly vibrates to avoid bullets
					if(0 < fuel){
						struct random_sequence rs;
						init_rseq(&rs, (unsigned long)this ^ (unsigned long)(w->war_time() / .1));
						Vec3d randomvec;
						for(int i = 0; i < 3; i++)
							randomvec[i] = drseq(&rs) - .5;
						pt->velo += randomvec * dt * .5;
						bbody->applyCentralForce(btvc(randomvec * mass * .5));
					}

					if(p->task == Attack || forward.sp(dv) < -.5){
						xh = forward.vp(dv);
						len = len2 = xh.len();
						len = asin(len);
						if(maxspeed < len){
							len = maxspeed;
						}
						len = sin(len / 2.);

						double velolen = bbody->getLinearVelocity().length();
						throttle = maxspeed < velolen ? (maxspeed - velolen) / maxspeed : 0.;

						// Suppress side slips
						Vec3d sidevelo = velo - mat.vec3(2) * mat.vec3(2).sp(velo);
						bbody->applyCentralForce(btvc(-sidevelo * mass));

						if(len && len2){
							btVector3 btomg = bbody->getAngularVelocity();
							btVector3 btxh = btvc(xh.norm());
							btVector3 btsideomg = btomg - btxh * btxh.dot(btomg);
							btVector3 btaac = btxh * len - btsideomg * 1.;
//							bbody->applyTorque(btaac);
							bbody->setAngularVelocity(btxh * len / dt);
/*							Vec3d omg, laac;
							qrot = xh * len / len2;
							qrot[3] = sqrt(1. - len * len);
							qres = qrot * pt->rot;
							pt->rot = qres.norm();*/

							/* calculate angular acceleration for displaying thruster bursts */
/*							omg = qrot.operator Vec3d&() * 1. / dt;
							p->aac = omg - pt->omg;
							p->aac *= 1. / dt;
							pt->omg = omg;
							laac = pt->rot.cnj().trans(p->aac);*/
							btTransform bttr = bbody->getWorldTransform();
							btVector3 laac = btMatrix3x3(bttr.getRotation().inverse()) * (btaac);
							if(laac[0] < 0) p->thrusts[0][0] += -laac[0];
							if(0 < laac[0]) p->thrusts[0][1] += laac[0];
							p->thrusts[0][0] = min(p->thrusts[0][0], 1.);
							p->thrusts[0][1] = min(p->thrusts[0][1], 1.);
						}
						if(trigger && p->task == Attack && dist < 5. * 2. && .99 < dv.sp(forward)){
							pt->inputs.change |= PL_ENTER;
							pt->inputs.press |= PL_ENTER;
						}
					}
					else{
						p->aac.clear();
					}
				}
				else if(!pt->enemy && (p->task == Attack || p->task == Away)){
					p->task = Parade;
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
					Sceptor *leader = NULL;
					for(Sceptor *wingman = formPrev; wingman; wingman = wingman->formPrev){
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
			else{
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
			}

			if(pt->inputs.press & (PL_ENTER | PL_LCLICK)){
				shootDualGun(dt);
			}

			if(p->cooldown < dt)
				p->cooldown = 0;
			else
				p->cooldown -= dt;

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

		static const double torqueAmount = .1;
		bbody->activate(true);
		if(pt->inputs.press & PL_A){
			bbody->applyTorque(btvc(mat.vec3(1) * torqueAmount));
		}
		if(pt->inputs.press & PL_D){
			bbody->applyTorque(btvc(-mat.vec3(1) * torqueAmount));
		}
		if(pt->inputs.press & PL_W){
			bbody->applyTorque(btvc(mat.vec3(0) * torqueAmount));
		}
		if(pt->inputs.press & PL_S){
			bbody->applyTorque(btvc(-mat.vec3(0) * torqueAmount));
		}

		if(controlled){
			if(inputs.press & PL_Q)
				p->throttle = MIN(throttle + dt, 1.);
			if(inputs.press & PL_Z)
				p->throttle = MAX(throttle - dt, -1.); // Reverse thrust is permitted
		}

		/* you're not allowed to accel further than certain velocity. */
		const double maxvelo = .5, speed = -p->velo.sp(mat.vec3(2));
		if(maxvelo < speed)
			p->throttle = 0.;
		else{
			if(!controlled && (p->task == Attack || p->task == Away))
				p->throttle = 1.;
			if(1. - speed / maxvelo < throttle)
				throttle = 1. - speed / maxvelo;
		}

		/* Friction (in space!) */
		if(parking){
			double f = 1. / (1. + dt / (parking ? 1. : 3.));
			pt->velo *= f;
		}

		{
			double consump = dt * (fabs(pf->throttle) + p->fcloak * 4.); /* cloaking consumes fuel extremely */
			Vec3d acc, acc0(0., 0., -1.);
			if(p->fuel <= consump){
				if(.05 < pf->throttle)
					pf->throttle = .05;
				if(p->cloak)
					p->cloak = 0;
				p->fuel = 0.;
			}
			else
				p->fuel -= consump;
			double spd = pf->throttle * (p->task != Attack ? .01 : .005);
			acc = pt->rot.trans(acc0);
			bbody->applyCentralForce(btvc(acc * spd * 20. * mass));
			pt->velo += acc * spd;
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

		p->fcloak = approach(p->fcloak, p->cloak, dt, 0.);
	}
	else{
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
				n = (int)(dt * SCEPTOR_SMOKE_FREQ + drseq(&w->rs));
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
			pt->pos += pt->velo * dt;
		}
	}

	reverser = approach(reverser, throttle < 0, dt * 5., 0.);

	if(mf < dt)
		mf = 0.;
	else
		mf -= dt;

	st::anim(dt);

	// if we are transitting WarField or being destroyed, trailing tefpols should be marked for deleting.
	if(this->pf && w != oldw)
		ImmobilizeTefpol3D(this->pf);
//	movesound3d(pf->hitsound, pt->pos);
}

// Docking and undocking will never stack.
bool Sceptor::solid(const Entity *o)const{
	return !(task == Undock || task == Dock);
}

int Sceptor::takedamage(double damage, int hitpart){
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
		health = -2.;
//		pt->deaths++;
	}
	else
		health -= damage;
	return ret;
}

void Sceptor::postframe(){
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

bool Sceptor::isTargettable()const{
	return true;
}
bool Sceptor::isSelectable()const{return true;}

double Sceptor::hitradius()const{
	return .01;
}

int Sceptor::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
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

double Sceptor::maxfuel()const{
	return 120.;
}

bool Sceptor::command(EntityCommand *com){
	if(InterpretCommand<HaltCommand>(com)){
		task = Idle;
		forcedEnemy = false;
	}
	else if(InterpretCommand<DeltaCommand>(com)){
		Entity *e;
		for(e = next; e; e = e->next) if(e->classname() == classname() && e->race == race){
			formPrev = static_cast<Sceptor*>(e);
			task = DeltaFormation;
			break;
		}
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


Entity *Sceptor::create(WarField *w, Builder *mother){
	Sceptor *ret = new Sceptor(NULL);
	ret->pos = mother->pos;
	ret->velo = mother->velo;
	ret->rot = mother->rot;
	ret->omg = mother->omg;
	ret->race = mother->race;
	ret->task = Undock;
//	w->addent(ret);
	return ret;
}





int Sceptor::cmd_dock(int argc, char *argv[], void *pv){
	Player *pl = (Player*)pv;
	for(Entity *e = pl->selected; e; e = e->selectnext){
		e->command(&DockCommand());
	}
	return 0;
}

int Sceptor::cmd_parade_formation(int argc, char *argv[], void *pv){
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

static void register_sceptor_cmd(Game &game){
	CmdAddParam("delta_formation", cmd_delta_formation, game.player);
}

static void register_server(){
	Game::addServerInits(register_sceptor_cmd);
}

static Initializator sss(register_server);


double Sceptor::pid_pfactor = 1.;
double Sceptor::pid_ifactor = 1.;
double Sceptor::pid_dfactor = 1.;
