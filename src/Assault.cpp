/** \file
 * \brief Implementation of Assault class.
 */
#define NOMINMAX
#include "Assault.h"
#include "Beamer.h"
#include "judge.h"
#include "Player.h"
#include "serial_util.h"
#include "EntityCommand.h"
#include "judge.h"
#include "btadapt.h"
//#include "glw/glwindow.h"
#include "motion.h"
#include "Game.h"
extern "C"{
#include <clib/mathdef.h>
}
#include <algorithm>

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.h>
#include <BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.h>


#define BEAMER_SCALE .0002




std::vector<hardpoint_static*> Assault::hardpoints;
GLuint Assault::disp = 0;
std::vector<hitbox> Assault::hitboxes;
std::vector<Warpable::Navlight> Assault::navlights;



Assault::Assault(WarField *aw) : st(aw), formPrev(NULL), engineHeat(0.f){
	st::init();
	init();
	for(int i = 0; i < hardpoints.size(); i++){
		turrets[i] = (0 || i % 2 ? new MTurret(this, hardpoints[i]) : new GatlingTurret(this, hardpoints[i]));
		if(aw)
			aw->addent(turrets[i]);
	}

	if(!aw)
		return;
	WarSpace *ws = *aw;
	if(ws && ws->bdw){
		buildBody();
	}
}

void Assault::init(){
	static bool initialized = false;
	if(!initialized){
		std::vector<SqInitProcess*> procs;
		HitboxProcess hp(hitboxes);
		procs.push_back(&hp);
		DrawOverlayProcess dop(disp);
		procs.push_back(&dop);
		NavlightsProcess np(navlights);
		procs.push_back(&np);
		HardPointProcess hdp(hardpoints);
		procs.push_back(&hdp);
		sq_init(_SC("models/Assault.nut"), procs);
		initialized = true;
	}
	turrets = new ArmBase*[hardpoints.size()];
	mass = 1e5;
	mother = NULL;
	paradec = -1;
	engineHeat = 0.f;
}

const char *Assault::idname()const{
	return "assault";
}

const char *Assault::classname()const{
	return "Assault";
}

const unsigned Assault::classid = registerClass("Assault", Conster<Assault>);
Entity::EntityRegister<Assault> Assault::entityRegister("Assault");

void Assault::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << formPrev;
	for(int i = 0; i < hardpoints.size(); i++)
		sc.o << turrets[i];
}

void Assault::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> (Entity*&)formPrev;
	for(int i = 0; i < hardpoints.size(); i++)
		sc.i >> turrets[i];

	// Update the dynamics body's parameters too.
	if(bbody){
		bbody->setCenterOfMassTransform(btTransform(btqc(rot), btvc(pos)));
		bbody->setAngularVelocity(btvc(omg));
		bbody->setLinearVelocity(btvc(velo));
	}
}

bool Assault::buildBody(){
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

const char *Assault::dispname()const{
	return "Sabre class";
}

void Assault::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	static const Vec3d src[] = {
		Vec3d(.002, .018, .022),
		Vec3d(0., 0./*05*/, 0.150),
		Vec3d(0., 0./*1*/, 0.300),
		Vec3d(0., 0., 0.300),
		Vec3d(0., 0., 1.000),
	};
	static const Quatd rot0[] = {
		quat_u,
		quat_u,
		Quatd::rotation(M_PI / 3., 0, 1, 0).rotate(M_PI / 8., 1, 0, 0),
		Quatd::rotation(-M_PI / 3., 0, 1, 0).rotate(-M_PI / 8., 1, 0, 0),
		Quatd::rotation(M_PI / 3., 0, 1, 0),
	};
	Mat4d mat;
	seatid = (seatid + numof(src)) % numof(src);
	rot = this->rot * rot0[seatid];
	Vec3d ofs = src[seatid];
	pos = this->pos + rot.trans(src[seatid]);
}

void Assault::enterField(WarField *target){
	WarSpace *ws = *target;

	if(ws && ws->bdw){
		buildBody();
		//add the body to the dynamics world
		ws->bdw->addRigidBody(bbody, 1, ~2);
	}
}

void Assault::leaveField(WarField *w){
	st::leaveField(w);
}


void Assault::anim(double dt){
	WarSpace *ws;
	if(!(ws = *w)){
		st::anim(dt);
		return;
	}

	if(bbody && !warping){
		const btTransform &tra = bbody->getCenterOfMassTransform();
		pos = btvc(tra.getOrigin());
		rot = btqc(tra.getRotation());
		velo = btvc(bbody->getLinearVelocity());
	}

	if(health <= 0){
		if(game->isServer())
			delete this;
		return;
	}

	Entity *pm = mother ? mother->e : NULL;

	if(task == sship_undock){
		if(!mother || !mother->e)
			task = sship_idle;
		else{
			double sp;
			Vec3d dm = this->pos - mother->e->pos;
			Vec3d mzh = this->rot.trans(vec3_001);
			sp = -mzh.sp(dm);
			inputs.press |= PL_W;
			if(1. < sp)
				task = sship_parade;
		}
	}
	else if(!controller){
		inputs.press = 0;
		if(!enemy){ /* find target */
			double best = 20. * 20.;
			Entity *t;
			for(WarField::EntityList::iterator it = w->entlist().begin(); it != w->entlist().end(); it++) if(*it){
				Entity *t = *it;
				if(t != this && t->race != -1 && t->race != this->race && 0. < t->health && hitradius() / 2. < t->hitradius()/* && t->vft != &rstation_s*/){
					double sdist = (this->pos - t->pos).slen();
					if(sdist < best){
						this->enemy = t;
						best = sdist;
					}
				}
			}
		}

		// In parade formation mode, be passive to battle.
		if(task == sship_parade && enemy && 5. * 5. < (enemy->pos - pos).slen()){
			enemy = NULL;
		}

		if(task == sship_delta){
			int nwingmen = 1; // Count yourself
			Assault *leader = NULL;
			for(Assault *wingman = formPrev; wingman; wingman = wingman->formPrev){
				nwingmen++;
				if(!wingman->formPrev)
					leader = wingman;
			}
			if(leader){
				double spacing = .2;
				Vec3d dp((nwingmen % 2 * 2 - 1) * (nwingmen / 2 * spacing), 0., nwingmen / 2 * spacing);
				dest = (leader->task == sship_moveto ? leader->dest : leader->pos) + dp;
				Vec3d dr = this->pos - dest;
				if(dr.slen() < .03 * .03){
					Quatd q1 = quat_u;
					this->velo += dr * (-dt * .5);
					this->rot = Quatd::slerp(this->rot, q1, 1. - exp(-dt));
				}
				else
					steerArrival(dt, dest, leader->velo, 1., .001);
			}
		}
		if(!enemy && task == sship_parade){
			if(mother){
				if(paradec == -1)
					paradec = mother->enumParadeC(mother->Frigate);
				Vec3d target, target0(1.5, -1., -1.);
				Quatd q2, q1;
				target0[0] += paradec % 10 * .30;
				target0[2] += paradec / 10 * -.30;
				target = pm->rot.trans(target0);
				target += pm->pos;
				Vec3d dr = this->pos - target;
				if(dr.slen() < .10 * .10){
					q1 = pm->rot;
//					inputs.press |= PL_W;
//					parking = 1;
					this->velo += dr * (-dt * .5);
					q2 = Quatd::slerp(this->rot, q1, 1. - exp(-dt));
					this->rot = q2;
				}
				else{
	//							p->throttle = dr.slen() / 5. + .01;
					steerArrival(dt, target, pm->velo, 1. / 10., .001);
				}
			}
			else
				task = sship_idle;
		}
/*		if(p->dock && p->undocktime == 0){
			if(pt->enemy)
				assault_undock(p, p->dock);
		}
		else */
		if(this->enemy){
			Vec3d xh, yh;
			long double sx, sy, len, len2, maxspeed = getManeuve().maxanglespeed * dt;
			Quatd qres, qrot;
			Vec3d dv = this->enemy->pos - this->pos;
			dv.normin();
			Vec3d forward = this->rot.trans(avec3_001);
			forward *= -1;
			xh = forward.vp(dv);
			len = len2 = xh.len();
			len = asinl(std::min(len, (long double)1.));
			if(maxspeed < len){
				len = maxspeed;
			}
			len = sinl(len / 2.);
			if(len && len2){
				(Vec3d&)qrot = xh * (len / len2);
				qrot[3] = sqrt(1. - len * len);
				qres = qrot * this->rot;
				this->rot = qres.norm();
			}
			if(.9 < dv.sp(forward)){
				this->inputs.change |= PL_ENTER;
				this->inputs.press |= PL_ENTER;
				if(5. * 5. < (this->enemy->pos - this->pos).slen())
					this->inputs.press |= PL_W;
				else if((this->enemy->pos, this->pos).slen() < 1. * 1.)
					this->inputs.press |= PL_S;
				else
					this->inputs.press |= PL_A; /* strafe around */
			}
		}
	}

	if(ws)
		space_collide(this, ws, dt, NULL, NULL);

	st::anim(dt);
	for(int i = 0; i < hardpoints.size(); i++) if(turrets[i])
		turrets[i]->align();

	// Exponential approach is more realistic (but costs more CPU cycles)
	engineHeat = direction & PL_W ? engineHeat + (1. - engineHeat) * (1. - exp(-dt / 2.)) : engineHeat * exp(-dt / 2.);
}

void Assault::clientUpdate(double dt){
	anim(dt);
}

void Assault::postframe(){
	st::postframe();
	for(int i = 0; i < hardpoints.size(); i++) if(turrets[i] && turrets[i]->w != w)
		turrets[i] = NULL;
	if(mother && (!mother->e || !mother->e->w))
		mother = NULL;
}

double Assault::maxhealth()const{return 10000.;}

int Assault::armsCount()const{return numof(turrets);}

ArmBase *Assault::armsGet(int i){
	if(i < 0 || armsCount() <= i)
		return NULL;
	return turrets[i];
}

bool Assault::undock(Docker *d){
//	if(!st::undock(d))
//		return false;
	task = sship_undock;
	mother = d;
	for(int i = 0; i < hardpoints.size(); i++) if(turrets[i])
		d->e->w->addent(turrets[i]);
	d->baycool += 5.;
	return true;
}

bool Assault::command(EntityCommand *com){
	if(InterpretCommand<DeltaCommand>(com)){
		bool foundself = false;
		WarField::EntityList::iterator it = w->el.begin();
		for(; it != w->el.end(); it++) if(*it){
			Entity *e = *it;
			if(!foundself){
				if(e == this)
					foundself = true;
				continue;
			}
			if(e->classname() == classname() && e->race == race){
				formPrev = static_cast<Assault*>(e->toWarpable());
				task = sship_delta;
				break;
			}
		}
		return true;
	}
	if(com->derived(AttackCommand::sid)){
		for(int i = 0; i < hardpoints.size(); i++) if(turrets[i])
			turrets[i]->command(com);
		return st::command(com);
	}
	return st::command(com);
}



/*Entity *Assault::create(WarField *w, Builder *mother){
	Assault *ret = new Assault(NULL);
	ret->pos = mother->pos;
	ret->velo = mother->velo;
	ret->rot = mother->rot;
	ret->omg = mother->omg;
	ret->race = mother->race;
//	w->addent(ret);
	return ret;
}*/

/*const Builder::BuildStatic Assault::builds = {
	"Sabre class",
	Assault::create,
	100.,
	600.,
};*/

#ifdef DEDICATED
void Assault::draw(WarDraw*){}
void Assault::drawtra(WarDraw*){}
void Assault::drawOverlay(WarDraw*){}
#endif









