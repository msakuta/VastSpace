/** \file
 * \brief Implements Attacker and AttackerDocker.
 */
#include "Attacker.h"
#include "war.h"
#include "btadapt.h"
#include "judge.h"
#include "Docker.h"
#include "LTurret.h"
#include "EntityCommand.h"
#include "draw/effects.h"
#include "draw/WarDraw.h"
#include "serial_util.h"
#include "motion.h"
extern "C"{
#include <clib/cfloat.h>
#include <clib/mathdef.h>
}



double Attacker::modelScale = .001;
double Attacker::defaultMass = 2e8;
Warpable::ManeuverParams Attacker::maneuverParams = {
	.025, /* double accel; */
	.1, /* double maxspeed; */
	5000000 * .1, /* double angleaccel; */
	.2, /* double maxanglespeed; */
	200000., /* double capacity; [MJ] */
	300., /* double capacitor_gen; [MW] */
};
std::vector<hardpoint_static*> Attacker::hardpoints;
std::vector<hitbox> Attacker::hitboxes;
GLuint Attacker::overlayDisp = 0;
std::vector<Warpable::Navlight> Attacker::navlights;

const char *Attacker::classname()const{return "Attacker";}
const unsigned Attacker::classid = registerClass("Attacker", Conster<Attacker>);
Entity::EntityRegister<Attacker> Attacker::entityRegister("Attacker");

Attacker::Attacker(Game *game) : st(game), docker(NULL), engineHeat(0){init();}

Attacker::Attacker(WarField *aw) : st(aw), docker(new AttackerDocker(this)), engineHeat(0){
	init();
	static int count = 0;
	for(int i = 0; i < hardpoints.size(); i++){
		turrets[i] = (true || count % 2 ? (LTurretBase*)new LTurret(this, hardpoints[i]) : (LTurretBase*)new LMissileTurret(this, hardpoints[i]));
		if(aw)
			aw->addent(turrets[i]);
	}
	count++;
	buildBody();
}

Attacker::~Attacker(){delete docker;}

void Attacker::static_init(){
	static bool initialized = false;
	if(!initialized){
		sq_init(_SC("models/Attacker.nut"),
			ModelScaleProcess(modelScale) <<=
			MassProcess(defaultMass) <<=
			ManeuverParamsProcess(maneuverParams) <<=
			HitboxProcess(hitboxes) <<=
			DrawOverlayProcess(overlayDisp) <<=
			NavlightsProcess(navlights) <<=
			HardPointProcess(hardpoints));
		initialized = true;
	}
}

void Attacker::init(){
	static_init();
	st::init();
	turrets = new ArmBase*[hardpoints.size()];
	mass = defaultMass;
	engineHeat = 0.;
}

void Attacker::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	st::dive(sc, method);
	docker->dive(sc, method);
}

void Attacker::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << docker;
	for(int i = 0; i < hardpoints.size(); i++)
		sc.o << turrets[i];
}

void Attacker::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> docker;

	// Update the dynamics body's parameters too.
	if(bbody){
		bbody->setCenterOfMassTransform(btTransform(btqc(rot), btvc(pos)));
		bbody->setAngularVelocity(btvc(omg));
		bbody->setLinearVelocity(btvc(velo));
	}

	for(int i = 0; i < hardpoints.size(); i++)
		sc.i >> turrets[i];
}

void Attacker::anim(double dt){
	st::anim(dt);
	docker->anim(dt);
	for(int i = 0; i < hardpoints.size(); i++) if(turrets[i])
		turrets[i]->align();

//	engineHeat = approach(engineHeat, direction & PL_W ? 1.f : 0.f, dt, 0.);
	// Exponential approach is more realistic (but costs more CPU cycles)
	engineHeat = direction & PL_W ? engineHeat + (1. - engineHeat) * (1. - exp(-dt)) : engineHeat * exp(-dt);
}

void Attacker::clientUpdate(double dt){
	anim(dt);
}

void Attacker::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	static const Vec3d offsets[2] = {Vec3d(0, .065, .110), Vec3d(0, .165, .350)};
	rot = this->rot;
	pos = rot.trans(offsets[seatid % 2]) + this->pos;
}

bool Attacker::command(EntityCommand *com){
	return st::command(com);
}

double Attacker::hitradius()const{return .3;}
double Attacker::maxhealth()const{return 100000.;}
double Attacker::maxenergy()const{return maneuverParams.capacity;}

ArmBase *Attacker::armsGet(int index){
	if(0 <= index && index < hardpoints.size())
		return turrets[index];
	return NULL;
}

int Attacker::armsCount()const{
	return hardpoints.size();
}

const Warpable::ManeuverParams &Attacker::getManeuve()const{
	return maneuverParams;
}

Docker *Attacker::getDockerInt(){
	return docker;
}

int Attacker::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
#if 0
	return st::tracehit(src, dir, rad, dt, ret, retp, retn);
#else
	double sc[3];
	double best = dt, retf;
	int reti = 0, i, n;
	for(n = 0; n < hitboxes.size(); n++){
		Vec3d org;
		Quatd rot;
		org = this->rot.itrans(hitboxes[n].org) + this->pos;
		rot = this->rot * hitboxes[n].rot;
		for(i = 0; i < 3; i++)
			sc[i] = hitboxes[n].sc[i] + rad;
		if((jHitBox(org, sc, rot, src, dir, 0., best, &retf, retp, retn)) && (retf < best)){
			best = retf;
			if(ret) *ret = retf;
			reti = i + 1;
		}
	}
	return reti;
#endif
}

int Attacker::takedamage(double damage, int hitpart){
	struct tent3d_line_list *tell = w->getTeline3d();
	int ret = 1;

//	playWave3D("hit.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
	if(0 < health && health - damage <= 0){
		int i;
		ret = 0;
		WarSpace *ws = *w;
		if(ws){

#if 1
			if(ws->gibs)
				AddTelineCallback3D(ws->gibs, pos, velo, .010, quat_u, omg, vec3_000, debrigib_multi, (void*)64, TEL3_QUAT | TEL3_NOLINE, 20.);
#else
			if(ws->gibs) for(i = 0; i < 128; i++){
				double pos[3], velo[3] = {0}, omg[3];
				/* gaussian spread is desired */
				for(int j = 0; j < 6; j++)
					velo[j / 2] += .025 * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				omg[0] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				omg[1] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				omg[2] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				VECCPY(pos, this->pos);
				for(int j = 0; j < 3; j++)
					pos[j] += hitradius() * (drseq(&w->rs) - .5);
				AddTelineCallback3D(ws->gibs, pos, velo, .010, quat_u, omg, vec3_000, debrigib, NULL, TEL3_QUAT | TEL3_NOLINE, 15. + drseq(&w->rs) * 5.);
			}
#endif

			/* smokes */
			for(i = 0; i < 64; i++){
				Vec3d pos;
//				COLOR32 col = 0;
				pos = this->pos
					+ Vec3d(.3 * (w->rs.nextd() - .5),
							.3 * (w->rs.nextd() - .5),
							.3 * (w->rs.nextd() - .5));
/*				col |= COLOR32RGBA(rseq(&w->rs) % 32 + 127,0,0,0);
				col |= COLOR32RGBA(0,rseq(&w->rs) % 32 + 127,0,0);
				col |= COLOR32RGBA(0,0,rseq(&w->rs) % 32 + 127,0);
				col |= COLOR32RGBA(0,0,0,191);*/
				static smokedraw_swirl_data sdata = {COLOR32RGBA(127,127,127,191), false};
				AddTelineCallback3D(ws->tell, pos, vec3_000, .07, quat_u, Vec3d(0., 0., .05 * M_PI * w->rs.nextGauss()),
					vec3_000, smokedraw_swirl, (void*)&sdata, TEL3_INVROTATE | TEL3_NOLINE, 20.);
			}

			{/* explode shockwave thingie */
#if 0 // Shockwave effect is difficult in that which direction it should expand and how to sort order among other tranparencies.
				static const double pyr[3] = {M_PI / 2., 0., 0.};
				amat3_t ort;
				Vec3d dr, v;
				Quatd q;
				amat4_t mat;
				double p;
	/*			w->vft->orientation(w, &ort, &pt->pos);
				VECCPY(dr, &ort[3]);*/
				dr = vec3_001;

				/* half-angle formula of trigonometry replaces expensive tri-functions to square root */
				q[3] = sqrt((dr[2] + 1.) / 2.) /*cos(acos(dr[2]) / 2.)*/;

				v = vec3_001.vp(dr);
				double vl = v.len();
				if(vl){
					p = sqrt(1. - q[3] * q[3]) / VECLEN(v);
					q = v * p;
				}
				else
					q = quat_u;

				AddTeline3D(tell, this->pos, vec3_000, 5., q, vec3_000, vec3_000, COLOR32RGBA(255,191,63,255), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_QUAT, 2.);
#endif
				AddTeline3D(tell, this->pos, vec3_000, 3., quat_u, vec3_000, vec3_000, COLOR32RGBA(255,255,255,127), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_INVROTATE, 2.);
			}
		}
//		playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
		this->w = NULL;
	}
	health -= damage;
	return ret;
}

short Attacker::bbodyGroup()const{
	return 3; // We could collide with dockable ships, what would we do?
}

bool Attacker::buildBody(){
	// If a rigid body object is brought from the old WarSpace, reuse it rather than to reconstruct.
	if(!bbody){
		static btCompoundShape *shape = NULL;
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

//		bbody->setSleepingThresholds(.0001, .0001);
	}

	return true;
}






























const unsigned AttackerDocker::classid = registerClass("AttackerDocker", Conster<AttackerDocker>);

const char *AttackerDocker::classname()const{return "AttackerDocker";}

bool AttackerDocker::undock(Entity::Dockable *pe){
	// Forbid undocking while warping
	if(e->toWarpable()->warping)
		return false;
	if(st::undock(pe)){
		pe->setPosition(&Vec3d(e->pos + e->rot.trans(Vec3d(-.085 * (nextport * 2 - 1), -0.015, 0))),
			&Quatd(e->rot * Quatd(0, 0, sin(-(nextport * 2 - 1) * 5. * M_PI / 4. / 2.), cos(5. * M_PI / 4. / 2.))),
			&Vec3d(e->velo),
			&Vec3d(e->omg));
		nextport = (nextport + 1) % 2;
		return true;
	}
	return false;
}

Vec3d AttackerDocker::getPortPos(Dockable *)const{
	return Vec3d(0., -0.035, 0.070);
}

Quatd AttackerDocker::getPortRot(Dockable *)const{
	return Quatd(1, 0, 0, 0);
}
