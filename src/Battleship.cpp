#include "Battleship.h"
#include "judge.h"
#include "serial_util.h"
#include "EntityCommand.h"
#include "btadapt.h"
#include "draw/effects.h"
#include "motion.h"
#include "LTurret.h"
#include "SqInitProcess-ex.h"
extern "C"{
#include <clib/mathdef.h>
}

const unsigned Battleship::classid = registerClass("Battleship", Conster<Battleship>);
Entity::EntityRegister<Battleship> Battleship::entityRegister("Battleship");
const char *Battleship::classname()const{return "Battleship";}
const char *Battleship::dispname()const{return "Battleship";}


double Battleship::modelScale = .001;
std::vector<hardpoint_static*> Battleship::hardpoints;
HitBoxList Battleship::hitboxes;
GLuint Battleship::disp = 0;
std::vector<Warpable::Navlight> Battleship::navlights;


Battleship::Battleship(WarField *aw) : st(aw), engineHeat(0.){
	init();
	for(int i = 0; i < hardpoints.size(); i++){
		turrets[i] = 1||i % 3 != 0 ? (LTurretBase*)new LTurret(this, hardpoints[i]) : (LTurretBase*)new LMissileTurret(this, hardpoints[i]);
		if(aw)
			aw->addent(turrets[i]);
	}
	buildBody();
}

bool Battleship::buildBody(){
	if(!w || bbody)
		return false;
	WarSpace *ws = *w;
	if(ws && ws->bdw){
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

		// The space does not have friction whatsoever.
//		rbInfo.m_linearDamping = .5;
//		rbInfo.m_angularDamping = .25;
		bbody = new btRigidBody(rbInfo);

//		bbody->setSleepingThresholds(.0001, .0001);

		//add the body to the dynamics world
//		ws->bdw->addRigidBody(bbody);
		return true;
	}
	return false;
}

void Battleship::static_init(){
	static bool initialized = false;
	if(!initialized){
		sq_init(_SC("models/Battleship.nut"),
			ModelScaleProcess(modelScale) <<=
			HitboxProcess(hitboxes) <<=
			DrawOverlayProcess(disp) <<=
			NavlightsProcess(navlights) <<=
			HardPointProcess(hardpoints));
		initialized = true;
	}
}

void Battleship::init(){
	static_init();
	st::init();
	turrets = new ArmBase*[hardpoints.size()];
	mass = 1e9;
	engineHeat = 0.;
}

void Battleship::serialize(SerializeContext &sc){
	st::serialize(sc);
	for(int i = 0; i < hardpoints.size(); i++)
		sc.o << turrets[i];
}

void Battleship::unserialize(UnserializeContext &sc){
	st::unserialize(sc);

	// Update the dynamics body's parameters too.
	if(bbody){
		bbody->setCenterOfMassTransform(btTransform(btqc(rot), btvc(pos)));
		bbody->setAngularVelocity(btvc(omg));
		bbody->setLinearVelocity(btvc(velo));
	}

	for(int i = 0; i < hardpoints.size(); i++)
		sc.i >> turrets[i];
}

double Battleship::getHitRadius()const{return .6;}

void Battleship::anim(double dt){
	if(!w)
		return;
/*	if(bbody){
		const btTransform &tra = bbody->getCenterOfMassTransform();
		pos = btvc(tra.getOrigin());
		rot = btqc(tra.getRotation());
		velo = btvc(bbody->getLinearVelocity());
		omg = btvc(bbody->getAngularVelocity());
	}*/

	st::anim(dt);
	for(int i = 0; i < hardpoints.size(); i++) if(turrets[i])
		turrets[i]->align();

	// Exponential approach is more realistic (but costs more CPU cycles)
	engineHeat = direction & PL_W ? engineHeat + (1. - engineHeat) * (1. - exp(-dt)) : engineHeat * exp(-dt);
}

void Battleship::clientUpdate(double dt){
	anim(dt);
}

void Battleship::postframe(){
	st::postframe();
	for(int i = 0; i < hardpoints.size(); i++) if(turrets[i] && turrets[i]->w != w)
		turrets[i] = NULL;
}

int Battleship::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	double sc[3];
	double best = dt, retf;
	int reti = 0, i, n;
#if 0
	if(0 < p->shieldAmount){
		Vec3d hitpos;
		if(jHitSpherePos(pos, BEAMER_SHIELDRAD + rad, src, dir, dt, ret, &hitpos)){
			if(retp) *retp = hitpos;
			if(retn) *retn = (hitpos - pos).norm();
			return 1000; /* something quite unlikely to reach */
		}
	}
#endif
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
}

void Battleship::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	rot = this->rot;
	pos = rot.trans(Vec3d(0, .08, .0)) + this->pos;
}

int Battleship::takedamage(double damage, int hitpart){
	Battleship *p = this;
	Teline3List *tell = w->getTeline3d();
	int ret = 1;

//	playWave3D("hit.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
	if(0 < p->health && p->health - damage <= 0){
		int i;
		ret = 0;
		WarSpace *ws = *w;
		if(ws){

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
					pos[j] += getHitRadius() * (drseq(&w->rs) - .5);
				AddTelineCallback3D(ws->gibs, pos, velo, .010, quat_u, omg, vec3_000, debrigib, NULL, TEL3_QUAT | TEL3_NOLINE, 15. + drseq(&w->rs) * 5.);
			}

			/* smokes */
			for(i = 0; i < 64; i++){
				double pos[3];
				COLOR32 col = 0;
				VECCPY(pos, p->pos);
				pos[0] += .3 * (drseq(&w->rs) - .5);
				pos[1] += .3 * (drseq(&w->rs) - .5);
				pos[2] += .3 * (drseq(&w->rs) - .5);
				col |= COLOR32RGBA(rseq(&w->rs) % 32 + 127,0,0,0);
				col |= COLOR32RGBA(0,rseq(&w->rs) % 32 + 127,0,0);
				col |= COLOR32RGBA(0,0,rseq(&w->rs) % 32 + 127,0);
				col |= COLOR32RGBA(0,0,0,191);
	//			AddTeline3D(w->tell, pos, NULL, .035, NULL, NULL, NULL, col, TEL3_NOLINE | TEL3_GLOW | TEL3_INVROTATE, 60.);
				AddTelineCallback3D(ws->tell, pos, vec3_000, .07, quat_u, vec3_000,
					vec3_000, smokedraw, (void*)col, TEL3_INVROTATE | TEL3_NOLINE, 20.);
			}

			{/* explode shockwave thingie */
#if 0
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
				p = sqrt(1. - q[3] * q[3]) / VECLEN(v);
				q = v * p;

				AddTeline3D(tell, this->pos, vec3_000, 5., q, vec3_000, vec3_000, COLOR32RGBA(255,191,63,255), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_QUAT, 2.);
#endif
				AddTeline3D(tell, this->pos, vec3_000, 3., quat_u, vec3_000, vec3_000, COLOR32RGBA(255,255,255,127), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_INVROTATE, 2.);
			}
		}
//		playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
		p->w = NULL;
	}
	p->health -= damage;
	return ret;
}

double Battleship::getMaxHealth()const{return 100000./2;}

int Battleship::armsCount()const{
	return hardpoints.size();
}

ArmBase *Battleship::armsGet(int i){
	if(i < 0 || armsCount() <= i)
		return NULL;
	return turrets[i];
}

bool Battleship::command(EntityCommand *com){
	return st::command(com);
}

double Battleship::maxenergy()const{return getManeuve().capacity;}

const Warpable::ManeuverParams &Battleship::getManeuve()const{
	static const ManeuverParams frigate_mn = {
		.025, /* double accel; */
		.075, /* double maxspeed; */
		2000000 * .1, /* double angleaccel; */
		.2, /* double maxanglespeed; */
		150000., /* double capacity; [MJ] */
		300., /* double capacitor_gen; [MW] */
	};
	return frigate_mn;
}








