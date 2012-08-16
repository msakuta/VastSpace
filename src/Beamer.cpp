/** \file
 * \brief Implementation of Beamer class.
 */
#include "Beamer.h"
#include "cmd.h"
#include "judge.h"
#include "serial_util.h"
//#include "sensor.h"
#include "motion.h"
#include "btadapt.h"
#include "Game.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
}



/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289

#define MAX_SHIELD_AMOUNT 5000.
#define BEAMER_HEALTH 15000.
#define BEAMER_MAX_SPEED .1
#define BEAMER_ACCELERATE .05
#define BEAMER_MAX_ANGLESPEED .4
#define BEAMER_ANGLEACCEL .2
#define BEAMER_SHIELDRAD .09

double Beamer::modelScale = 0.0002;
double Beamer::defaultMass = 1e5;
Warpable::ManeuverParams Beamer::mn = {
	.025, /* double accel; */
	.1, /* double maxspeed; */
	100., /* double angleaccel; */
	.4, /* double maxanglespeed; */
	50000., /* double capacity; [MJ] */
	100., /* double capacitor_gen; [MW] */
};
GLuint Beamer::disp = 0;
std::vector<hitbox> Beamer::hitboxes;
std::vector<Warpable::Navlight> Beamer::navlights;


Beamer::Beamer(WarField *aw) : st(aw){
	init();

	if(!aw)
		return;
	WarSpace *ws = *aw;
	if(ws && ws->bdw){
		buildBody();
	}
}

void Beamer::init(){
	static bool initialized = false;
	if(!initialized){
		sq_init(_SC("models/Beamer.nut"),
			ModelScaleProcess(modelScale) <<=
			MassProcess(defaultMass) <<=
			ManeuverParamsProcess(mn) <<=
			HitboxProcess(hitboxes) <<=
			NavlightsProcess(navlights) <<=
			DrawOverlayProcess(disp));
		initialized = true;
	}
	charge = 0.;
//	dock = NULL;
	undocktime = 0.f;
	cooldown = 0.;
	integral.clear();
	health = maxhealth();
	mass = defaultMass;
}

const char *Beamer::idname()const{
	return "beamer";
}

const char *Beamer::classname()const{
	return "Beamer";
}

const unsigned Beamer::classid = registerClass("Beamer", Conster<Beamer>);
Entity::EntityRegister<Beamer> Beamer::entityRegister("Beamer");

void Beamer::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << charge;
	sc.o << integral;
	sc.o << beamlen;
	sc.o << cooldown;
//	scarry_t *dock;
	sc.o << undocktime;
}

void Beamer::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> charge;
	sc.i >> integral;
	sc.i >> beamlen;
	sc.i >> cooldown;
//	scarry_t *dock;
	sc.i >> undocktime;

	// Update the dynamics body's parameters too.
	if(bbody){
		bbody->setCenterOfMassTransform(btTransform(btqc(rot), btvc(pos)));
		bbody->setAngularVelocity(btvc(omg));
		bbody->setLinearVelocity(btvc(velo));
	}
}

bool Beamer::buildBody(){
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

const char *Beamer::dispname()const{
	return "Lancer Class";
}

#if 0
void beamer_dock(beamer_t *p, scarry_t *pc, warf_t *w){
	p->dock = pc;
	VECCPY(p->st.st.pos, pc->st.st.pos);
}

void beamer_undock(beamer_t *p, scarry_t *pm){
	static const avec3_t pos0 = {-100 * SCARRY_SCALE, 50 * SCARRY_SCALE, 100 * SCARRY_SCALE};
	if(p->dock != pm)
		p->dock = pm;
	if(p->dock->baycool){
		VECCPY(p->st.st.pos, p->dock->st.st.pos);
		p->undocktime = 0.f;
	}
	else{
		entity_t *dock = &p->dock->st.st;
		p->dock->baycool = p->undocktime = 15.f;
		QUATCPY(p->st.st.rot, dock->rot);
		quatrot(p->st.st.pos, dock->rot, pos0);
		VECADDIN(p->st.st.pos, dock->pos);
		VECCPY(p->st.st.velo, dock->velo);
	}
}
#endif

void Beamer::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
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

void Beamer::anim(double dt){
	if(!w->operator WarSpace *()){
		st::anim(dt);
		return;
	}

	/* forget about beaten enemy */
	if(enemy && (enemy->health <= 0. || enemy->w != w))
		enemy = NULL;

	Mat4d mat;
	transform(mat);

	if(0 < health){
		Entity *collideignore = NULL;
		int i, n;
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
		else if(controller){
		}
		else if(!enemy && task == sship_parade){
			Entity *pm = mother ? mother->e : NULL;
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
					inputs.press &= ~PL_W;
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
		else{
			Vec3d pos, dv;
			double dist;
			Vec3d opos;
			inputs.press = 0;
			if((task == sship_idle || task == sship_attack) && !enemy){ /* find target */
				double best = 20. * 20.;
				for(WarField::EntityList::iterator it = w->el.begin(); it != w->el.end(); it++) if(*it){
					Entity *t = *it;
					if(t != this && t->race != -1 && t->race != race && 0. < t->health){
						double sdist = (this->pos - t->pos).slen();
						if(sdist < best){
							enemy = t;
							best = sdist;
							task = sship_attack;
						}
					}
				}
			}
			if(task == sship_attack && !enemy)
				task = sship_idle;

			if(task == sship_attack){
				if(this->enemy){
					Vec3d dv, forward;
					Vec3d xh, yh;
					long double sx, sy, len, len2, maxspeed = BEAMER_MAX_ANGLESPEED * dt;
					Quatd qres, qrot;
					dv = (enemy->pos - this->pos).normin();
					forward = -this->rot.trans(avec3_001);
					xh = forward.vp(dv);
					len = len2 = xh.len();
					len = asinl(len);
					if(maxspeed < len){
						len = maxspeed;
					}
					len = sinl(len / 2.);
					if(len && len2){
						double sd, df, drl, decay;
						Vec3d ptomg, omg, dvv, delta, diff;
/*						VECSCALE(qrot, xh, len / len2);
						qrot[3] = sqrt(1. - len * len);
						QUATMUL(qres, qrot, pt->rot);
						QUATNORM(pt->rot, qres);*/
/*						drl = VECDIST(pt->enemy->pos, pt->pos);
						VECSUB(dvv, pt->enemy->velo, pt->velo);
						VECVP(ptomg, dv, dvv);*/
						VECCPY(diff, xh);
/*						VECSCALEIN(xh, 1.);
						VECSADD(xh, ptomg, .0 / drl);
						VECSUB(omg, pt->omg, ptomg);
						sd = VECSDIST(omg, xh);
						df = 1. / (.1 + sd);
						df = dt * MIN(df, frigate_mn.angleaccel);
						VECSCALEIN(omg, 1. - df);
						VECADD(pt->omg, omg, ptomg);
						len2 = VECLEN(xh);*/
						df = dt * MIN(len2, frigate_mn.angleaccel) / len2;
						df = MIN(df, 1);
						delta = diff - this->omg;
						delta += integral * .2;
						this->omg += delta * df;
						if(frigate_mn.maxanglespeed * frigate_mn.maxanglespeed < this->omg.slen()){
							this->omg.normin();
							this->omg *= frigate_mn.maxanglespeed;
						}
						if(.25 < len2)
							diff *= .25 / len2;
						integral += diff * dt;
						decay = exp(-.1 * dt);
						integral *= decay;
						inputs.press |= PL_2 | PL_8;
					}
					if(.9 < dv.sp(forward)){
						inputs.change |= PL_ENTER;
						inputs.press |= PL_ENTER;
						if(5. * 5. < (enemy->pos - this->pos).slen())
							inputs.press |= PL_W;
						else if((enemy->pos - this->pos).slen() < 1. * 1.)
							inputs.press |= PL_S;
/*						else
							inputs.press |= PL_A; *//* strafe around */
					}
				}

				/* follow another */
				if(0 && !enemy && !warping){
					Entity *pt2;
					Warpable *leader = NULL;
					for(WarField::EntityList::iterator it = w->el.begin(); it != w->el.end(); it++) if(*it){
						Entity *pt2 = *it;
						if(this != pt2 && race == pt2->race && pt2->toWarpable() && pt2->controller){
							leader = pt2->toWarpable();
							break;
						}
					}

/*					if(leader && leader->charge){
						pt->inputs.change |= PL_ENTER;
						pt->inputs.press |= PL_ENTER;
					}*/

					/*for(pt2 = w->tl; pt2; pt2 = pt2->next) if(pt != pt2 && pt2->vft == pt->vft && w->pl->control == pt2 && ((beamer_t*)pt2)->warping)*/
					if(leader && leader->warping && leader->warpdstcs != w->cs){
						Warpable *p2 = leader;
						int k;
						warping = p2->warping;
						warpcs = NULL;
						warpdst = p2->warpdst;
						for(k = 0; k < 3; k++)
							warpdst[k] += drseq(&w->rs) - .5;
						warpdstcs = p2->warpdstcs;
	/*					break;*/
					}
				}
			}
			else if(task == sship_dockque || task == sship_dock){
				Entity *pm = findMother();

				// It is possible that no one is glad to become a mother.
				if(!pm)
					task = sship_idle;
				else{
					Vec3d target0(mother->getPortPos(this));
					Quatd q2, q1;
					collideignore = pm;

					if(bbody){
						// Mask to avoid collision with the mother
						bbody->getBroadphaseProxy()->m_collisionFilterMask &= ~2;

						// Suppress side slips
						Vec3d sidevelo = velo - mat.vec3(2) * mat.vec3(2).sp(velo);
						bbody->applyCentralForce(btvc(-sidevelo * mass));
					}

					// Runup length
					if(task == sship_dockque)
						target0 += mother->getPortRot(this).trans(Vec3d(0, 0, .8));

					Vec3d target = pm->rot.trans(target0);
					target += pm->pos;
					steerArrival(dt, target, pm->velo, task == sship_dockque ? 1. / 2. : -mat.vec3(2).sp(velo) < 0 ? 1. : .025, .01);
					double dist = (target - this->pos).len();
					if(dist < .01){
						if(task == sship_dockque){
							task = sship_dock;
							mother->dockque(this); // Notify the mother that this Entity is queued for docking.
						}
						else{
							mother->dock(this);
							return;
						}
					}
//					if(1. < p->throttle)
//						p->throttle = 1.;
				}
			}
		}
	}
	else{
		if(game->isServer())
			delete this;
		return;
	}

	if(cooldown == 0. && inputs.press & (PL_ENTER | PL_LCLICK)){
		charge = 6.;
		cooldown = 10.;
	}

	if(charge < dt)
		charge = 0.;
	else
		charge -= dt;

	if(cooldown < dt)
		cooldown = 0.;
	else
		cooldown -= dt;

	WarSpace *ws = *w;
	// Collision resolution should be done in Bullet dynamics engine.
//	if(ws){
//		space_collide(this, ws, dt, NULL, NULL);
//	}

	if(0. < charge && charge < 4.){
		Entity *hit = NULL;
		Vec3d start, dir, start0(0., 0., -.04), dir0(0., 0., -10.);
		double best = 10., sdist;
		int besthitpart = 0, hitpart;
		start = rot.trans(start0) + pos;
		dir = rot.trans(dir0);
		for(WarField::EntityList::iterator it = w->el.begin(); it != w->el.end(); it++) if(*it){
			Entity *pt2 = *it;
			double rad = pt2->getHitRadius();
			Vec3d delta = pt2->pos - pos;
			if(pt2 == this)
				continue;
			if(!jHitSphere(pt2->pos, rad + .005, start, dir, 1.))
				continue;
			if((hitpart = pt2->tracehit(start, dir, .005, 1., &sdist, NULL, NULL)) && (sdist *= -dir0[2], 1)){
				hit = pt2;
				best = sdist;
				besthitpart = hitpart;
			}
		}
		if(ws && hit){
			beamlen = best;
			hit->takedamage(1000. * dt, besthitpart);
#ifndef DEDICATED
			Vec3d pos = mat.vec3(2) * -best + this->pos;
			Quatd qrot = Quatd::direction(mat.vec3(2));
			if(drseq(&w->rs) * .1 < dt){
				avec3_t velo;
				int i;
				for(i = 0; i < 3; i++)
					velo[i] = (drseq(&w->rs) - .5) * .1;
				AddTeline3D(ws->tell, pos, velo, drseq(&w->rs) * .01 + .01, quat_u, vec3_000, vec3_000, COLOR32RGBA(0,127,255,95), TEL3_NOLINE | TEL3_GLOW | TEL3_INVROTATE, .5);
			}
			AddTeline3D(ws->tell, pos, vec3_000, drseq(&w->rs) * .25 + .25, qrot, vec3_000, vec3_000, COLOR32RGBA(0,255,255,255), TEL3_NOLINE | TEL3_CYLINDER | TEL3_QUAT, .1);
#endif
		}
		else
			beamlen = 10.;
	}
	st::anim(dt);
#if 0
	if(p->pf){
		int i;
		avec3_t pos, pos0[numof(p->pf)] = {
			{.0, -.003, .045},
		};
		for(i = 0; i < numof(p->pf); i++){
			MAT4VP3(pos, mat, pos0[i]);
			MoveTefpol3D(p->pf[i], pos, avec3_000, cs_orangeburn.t, 0);
		}
	}
#endif
}





Entity::Props Beamer::props()const{
	Props ret = st::props();
	ret.push_back(gltestp::dstring("Cooldown: ") << cooldown);
	return ret;
}

bool Beamer::undock(Docker *d){
	task = sship_undock;
	mother = d;
	return true;
}

Warpable::ManeuverParams &Beamer::getManeuve()const{
	return mn;
}

/*Entity *Beamer::create(WarField *w, Builder *mother){
	Beamer *ret = new Beamer(NULL);
	ret->pos = mother->pos;
	ret->velo = mother->velo;
	ret->rot = mother->rot;
	ret->omg = mother->omg;
	ret->race = mother->race;
//	w->addent(ret);
	return ret;
}

const Builder::BuildStatic Beamer::builds = {
	"Lancer class",
	Beamer::create,
	100.,
	600.,
};*/



double Beamer::maxhealth()const{return BEAMER_HEALTH;}

std::vector<hitbox> *Beamer::getTraceHitBoxes()const{
	return &hitboxes;
}

#ifdef DEDICATED
void Beamer::draw(WarDraw *){}
void Beamer::drawtra(WarDraw *){}
void Beamer::drawOverlay(WarDraw *){}
#endif
