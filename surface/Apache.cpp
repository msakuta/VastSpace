/** \file
 * \brief Impelementation of Apache class.
 */
#include "Apache.h"
#include "Player.h"
#include "Bullet.h"
#include "tent3d.h"
//#include "island3.h"
//#include "aim9.h"
#include "antiglut.h"
//#include "glwindow.h"
//#include "walk.h"
//#include "hydra.h"
//#include "warutil.h"
#include "judge.h"
#include "arms.h"
#include "motion.h"
#include "SurfaceCS.h"
#include "btadapt.h"
extern "C"{
#include <clib/cfloat.h>
}

#include <assert.h>


#define APACHE_SPEED (.005 * 2)
#define APACHE_PHASE_SPEED M_PI
#define APACHE_TAIL_SPEED 3.
#define APACHE_SMOKE_FREQ 10.
#define APACHE_BULLETSPEED 1.
#define APACHE_PRICE 1450e1
#define BULLETDAMAGE 5.
#define WCS_TIME 1.
#define ROTSPEED M_PI
#define YAWSPEED (M_PI*.2)

#define SSM_MASS 80.
#define BULLETSPEED .08
#define SONICSPEED .340
#define WINGSPEED (.25 * M_PI)






Entity::EntityRegister<Apache> Apache::entityRegister("Apache");

double Apache::modelScale = 0.00001;
double Apache::defaultMass = 5000.;
double Apache::maxHealthValue = 100.;
double Apache::rotorAxisSpeed = 0.1 * M_PI;
double Apache::mainRotorLiftFactor = 0.023;
double Apache::tailRotorLiftFactor = 0.003;
double Apache::featherSpeed = 1.;
Vec3d Apache::cockpitOfs = Vec3d(0., .0008, -.0022);
HitBoxList Apache::hitboxes;


void Apache::init(){
	static bool initialized = false;
	if(!initialized){
		SqInit(game->sqvm, modPath() << _SC("models/apache.nut"),
			SingleDoubleProcess(modelScale, "modelScale") <<=
			SingleDoubleProcess(defaultMass, "mass") <<=
			SingleDoubleProcess(maxHealthValue, "maxhealth", false) <<=
			SingleDoubleProcess(rotorAxisSpeed, "rotorAxisSpeed", false) <<=
			SingleDoubleProcess(mainRotorLiftFactor, "mainRotorLiftFactor", false) <<=
			SingleDoubleProcess(tailRotorLiftFactor, "tailRotorLiftFactor", false) <<=
			SingleDoubleProcess(featherSpeed, "featherSpeed") <<=
			Vec3dProcess(cockpitOfs, "cockpitOfs") <<=
			HitboxProcess(hitboxes));
		initialized = true;
	}
	int i;
	rotor = rotoromega = tailrotor = 0.;
	rotoraxis[0] = rotoraxis[1] = 0.;
	crotoraxis[0] = crotoraxis[1] = 0.;
	gun[0] = gun[1] = 0.;
	throttle = 0.;
	feather = 1.;
	tail = 0.;
	cooldown = cooldown2 = 1.;
/*	p->armsmuzzle = 0;*/
	weapon = 0;
	ammo_chaingun = 1200;
	hellfires = 8;
	hydras = 38;
	aim9 = 2;
	contact = 0;
	muzzle = 0;
	health = getMaxHealth();
	brk = 1;
	navlight = 0;
	mass = 5000.;
//	memcpy(p->arms, apachearms_def, sizeof p->arms);
//	while(p->sw) ShieldWaveletFree(&p->sw);
/*	for(i = 0; i < numof(p->sw); i++)
		p->sw[i].phase = p->sw[i].amount = 0.;*/
}

Apache::Apache(WarField *w) : st(w){
	init();
}

void Apache::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	amat4_t mat2, mat3;
	static const Vec3d apache_overview_ofs(0., .010, .025);
	int camera = (seatid + 4) % 4;
	Mat4d mat;
	transform(mat);
	if(camera == 1){
		pos = mat.vp3(apache_overview_ofs);
	}
	else if(camera == 2){
		if(enemy){
			Mat3d ort = mat3d_u();
//			w->orientation(w, &ort, pt->pos);
			Vec3d dr0 = this->pos - enemy->pos;
			dr0.normin();
			Vec3d dr1 = dr0.vp(ort.vec3(1));
			Vec3d dr2 = dr1.vp(dr0);
			dr2.normin();
			dr0 *= .025;
			dr0 += dr2 * .01;
			pos = this->pos + dr0;
		}
		else{
			pos = mat.vp3(apache_overview_ofs);
		}
	}
	else if(camera == 3){
		Vec3d pos0;
		const double period = this->velo.slen() < .1 * .1 ? .5 : 1.;
//		struct contact_info ci;
		pos0[0] = floor(this->pos[0] / period + .5) * period;
		pos0[1] = floor(this->pos[1] / period + .5) * period;
		pos0[2] = floor(this->pos[2] / period + .5) * period;
//		if(w && w->vft->spherehit && w->vft->spherehit(w, pos0, .002, &ci))
//			VECSADD(pos0, ci.normal, ci.depth);
		pos = pos0;
	}
	else
		pos = mat.vp3(cockpitOfs);
	rot = this->rot;
}

void Apache::control(const input_t *inputs, double dt){
	st::control(inputs, dt);

	static const double mouseSensitivity = 0.01;

	// Rotate the rotor's axis
	double desired[2];
	for(int i = 0; i < 2; i++){
		crotoraxis[i] = rangein(crotoraxis[i] + inputs->analog[i] * rotorAxisSpeed * mouseSensitivity, -M_PI / 8., M_PI / 8.);
	}

	if(inputs->change & inputs->press & PL_SPACE){
		double best = 3. * 3.;
		double mini = enemy ? (pos - enemy->pos).slen() : 0.;
		double sdist;
		Entity *target = NULL;
		for(WarField::EntityList::iterator it = w->entlist().begin(); it != w->entlist().end(); ++it){
			Entity *t = *it;
			if(t != this && this != enemy && 0. < health && mini < (sdist = (this->pos - t->pos).slen()) && sdist < best){
				best = sdist;
				target = t;
			}
		}
		if(target)
			enemy = target;
		else{
			for(WarField::EntityList::iterator it = w->entlist().begin(); it != w->entlist().end(); ++it){
				Entity *t = *it;
				if(t != this && t != enemy && 0. < t->getHealth() && (sdist = (pos - t->pos).slen()) < best){
					best = sdist;
					target = t;
				}
				enemy = target;
			}
		}
	}
}

/** determine if target can be hit by ballistic bullets from the starting position.
 * returns desired angles also if feasible. */
int shoot_angle2(const double pos[3], const double epos[3], double phi0, double v, double (*ret_angles)[2], int farside, const Vec3d &gravity){
	double g = gravity.len();
	double vd[3], vv[3];
	double dy = pos[1] - epos[1], dx, D, rD, vx2, vx;
/*			if((VECSUB(vd, epos, pt->pos), vecnormin(vd), vecnorm(vv, pt->velo), VECSP(vd, vv)) < .99)
		break;*/
	{
		double d0 = epos[0] - pos[0], d1 = epos[2] - pos[2];
		dx = sqrt(d0 * d0 + d1 * d1);
	}
	D = v * v * (v * v - 2 * dy * g) - g * g * dx * dx;
/*	D = v * v * v * v - g * g * dx * dx;*/
	if(D <= 0)
		return 0;
	rD = sqrt(D);
/*	vx2 = -g * dx * dx / 2 / (v * v - dy * g + (farside ? 1 : -1) * rD);*/
	vx2 = dx * dx / 2 / (dy * dy + dx * dx) * (v * v - dy * g + (farside ? -1 : 1) * rD);
/*	vx2 = 1. / 2 * (v * v + rD);*/
	if(vx2 <= 0.)
		return 0;
	vx = sqrt(vx2);
	if(vx <= 0.)
		return 0;
	if(ret_angles){
		(*ret_angles)[0] = (vx2 * dy / dx / dx + g * dx / 2 / vx < 0 ? 1 : -1) * acos(vx / v);
/*		if(farside)
			(*ret_angles)[0] = M_PI / 2. - (*ret_angles)[0];*/
		(*ret_angles)[1] = phi0;
	}
	return 1;
}

int shoot_angle(const double pos[3], const double epos[3], double phi0, double v, double (*ret_angles)[2]){
	return shoot_angle2(pos, epos, phi0, v, ret_angles, 0, vec3_000);
}

void Apache::find_enemy_logic(){
	double best = 100. * 100.; /* sense range */
	double sdist;
	Entity *closest = NULL;
/*			pt->enemy = &head[(i+1)%n];*/
	WarField::EntityList::iterator it = w->entlist().begin();
	for(; it != w->entlist().end(); it++) if(*it){
		Entity *pt2 = *it;
		if(pt2 != this && pt2->w == w && pt2->getHealth() > 0. && 0 <= pt2->race && pt2->race != race
			&& strcmp(pt2->idname(), "respawn") && (sdist = (pt2->pos - pos).slen()) < best){
			best = sdist;
			closest = pt2;
		}
	}
	if(closest)
		enemy = closest;
}

void Apache::anim(double dt){
	bool inwater = false;

	Mat4d mat;
	transform(mat);
	double air = w->atmosphericPressure(pos);
	double height = 10. * log(air);

	if(WarSpace *ws = *w){
		const Vec3d offset(0, 0.001, 0);
		// If the CoordSys is a SurfaceCS, we can expect ground in negative y direction.
		// dynamic_cast should be preferred.
		if(&w->cs->getStatic() == &SurfaceCS::classRegister){
			SurfaceCS *s = static_cast<SurfaceCS*>(w->cs);
			Vec3d normal;
			double height = s->getHeight(pos[0], pos[2], &normal);
			if(pos[1] - offset[1] < height){
				Vec3d dest(pos[0], height + offset[1], pos[2]);
				Vec3d newVelo = (velo - normal.sp(velo) * normal) * exp(-dt);
				setPosition(&dest, NULL, &newVelo);
#if 0 // Enable after adding bbody
				btVector3 btOmega = bbody->getAngularVelocity() - normal.vp(bbody->getWorldTransform().getBasis().getColumn(1)) * 5. * dt;
				bbody->setAngularVelocity(btOmega * exp(-3. * dt));
				floorTouch = true;
#endif
			}
		}
	}

	if(0. < health){
		double common, normal;
		double gcommon, gnormal;
		double dist = 0., sp = 0.;

		Mat3d ort3 = mat3d_u();
//		w->vft->orientation(w, &ort3, pt->pos);
		Mat3d iort3 = ort3.transpose();

		if(enemy && enemy->getHealth() <= 0.)
			enemy = NULL;
		if(!enemy){
			find_enemy_logic();
		}
		if(controller){
			Mat3d mat3 = mat.tomat3();
			Mat3d matt = iort3 * mat3;
			common = matt[7] / 2. /*pt->velo[1] / 5e-0*/;
			normal = rangein(-matt[1] / .7 / 2., -M_PI / 4., M_PI / 4.);
			gcommon = common += crotoraxis[0] = rangein(crotoraxis[0] + M_PI * .001 * inputs.analog[1], -M_PI / 6., M_PI / 6.);
			gnormal = normal += crotoraxis[1] = rangein(crotoraxis[1] - M_PI * .001 * inputs.analog[0], -M_PI / 6., M_PI / 6.);
		}
		else{
			Mat3d mat3 = mat.tomat3();
			Mat3d matt = iort3 * mat3;
			common = matt[7] / 1. /*pt->velo[1] / 5e-0*/;
			if(enemy){
				Vec3d dr = enemy->pos - this->pos;
				dist = dr.len();
				Vec3d zh = this->rot.trans(vec3_001);
				sp = dr.sp(zh) / dist;
				common += rangein(.5 * (zh.sp(dr) + .2), -.4, .4);
			}
			normal = rangein(-matt[1] / .7, -M_PI / 4., M_PI / 4.);
/*			common = 0.;*/
			inputs.change = 0;
			gcommon = 0.;
			gnormal = 0.;
		}

		if((!controller || weapon == 1) && enemy){
			double retangles[2];

/*				do if(p->corb){
				study_corb(p, dt);
			} while(0);*/

			Quatd qc = rot.cnj();
			Vec3d epos;
			estimate_pos(epos, enemy->pos, enemy->velo, this->pos, this->velo, APACHE_BULLETSPEED, w);
			Vec3d delta = epos - this->pos;
			double sd = delta.slen();
			Vec3d ldelta = qc.trans(delta);
			ldelta[1] -= .002;
			shoot_angle(avec3_000, ldelta, 0., APACHE_BULLETSPEED, &retangles);
//			gnormal = pyr[1] = -atan2(ldelta[0], -ldelta[2]);
//			gcommon = pyr[0] = retangles[0]/*- -asin(ldelta[1] / sqrt(ldelta[0] * ldelta[0] + ldelta[2] * ldelta[2]))*/;
		}
		gun[0] = rangein(approach(gun[0], gcommon, M_PI * dt, 2 * M_PI), -M_PI / 4., M_PI / 8.);
		gun[1] = rangein(approach(gun[1], gnormal, M_PI * dt, 2 * M_PI), -M_PI / 2., M_PI / 2.);

		rotoraxis[0] = rangein(approach(rotoraxis[0], rangein(common, -M_PI / 6., M_PI / 6.), M_PI * dt, 2 * M_PI), -M_PI / 8., M_PI / 8.);
		rotoraxis[1] = rangein(approach(rotoraxis[1], rangein(normal, -M_PI / 6., M_PI / 6.), M_PI * dt, 2 * M_PI), -M_PI / 8., M_PI / 8.);

		/* control of throttle */
		if(!controller && enemy)
			throttle = approach(throttle, 1., .2 * dt, 5.);
		if(inputs.press & PL_W)
			throttle = approach(throttle, 1., .2 * dt, 5.);
		if(inputs.press & PL_S)
			throttle = approach(throttle, 0., .2 * dt, 5.);

		/* control of feathering angle of main rotor blades */
		if(!controller && enemy)
			feather = approach(feather, rangein(!!contact + .5 * dist + 5. * (ort3.vec3(1).sp(enemy->pos) - ort3.vec3(1).sp(this->pos) + .2), 0., 1.), .2 * dt, 5.);
		if(inputs.press & PL_Q)
			feather = approach(feather, 1., featherSpeed * dt, 5.);
		if(inputs.press & PL_Z)
			feather = approach(feather, -.5, featherSpeed * dt, 5.);

		/* tail rotor control */
		if(!controller){
			if(enemy)
				tail = approach(tail, rangein(gnormal, -.2, .2), APACHE_TAIL_SPEED * dt, 5.);
		}
		else{
			if(inputs.press & PL_A)
				tail = approach(tail, 1., APACHE_TAIL_SPEED * dt, 5.);
			else if(inputs.press & PL_D)
				tail = approach(tail, -1., APACHE_TAIL_SPEED * dt, 5.);
			else /*if(pt->inputs.press & PL_R)*/
				tail = approach(tail, 0., APACHE_TAIL_SPEED * dt, 5.);
		}

	}
	else{
		throttle = approach(throttle, 0., .2 * dt, 5.);
		tail = approach(tail, 0., .5 * dt, 5.);
	}
	if(cooldown < dt)
		cooldown = 0.;
	else
		cooldown -= dt;

	if(cooldown2 < dt)
		cooldown2 = 0.;
	else
		cooldown2 -= dt;

	/* global acceleration */
	if(!bbody){
		Vec3d accel = w->accel(this->pos, this->velo);
		this->velo += accel * dt;
	}

	{
		double airflux;
		Mat4d rot = mat.rotx(rotoraxis[0]);
		Mat4d rot2 = rot.rotz(rotoraxis[1]);

		/* momentum of the air flows into rotor's plane. downwards positive because we refer vehicle's velocity. */
		{
			Vec3d localvelo = this->velo;
			airflux = rot2.vec3(1).sp(localvelo) * air / .1;
		}

		/* rotor thrust */
		{
			double dest = throttle;
			double v = (throttle - (rotoromega < 0. ? -1. : 1.) * (rotoromega * rotoromega * (5. * !!inwater + .7 + air * (.15 + .15 * feather * feather)) + .02) - (1 * feather * airflux)) * .3 * dt;
			rotoromega = rangein(rotoromega + v, -1., 5.);
	/*		p->rotoromega = approach(p->rotoromega, dest, v, 1e10);*/
		}
		rotor = fmod(rotor + rotoromega * 10. * M_PI * dt, M_PI * 2.);
	/*	rotor = fmod(rotor + throttle * 8. * M_PI * dt, M_PI * 2.);*/
		tailrotor = fmod(tailrotor + (rotoromega/* + p->tail*/) * 6. * M_PI * dt, M_PI * 2.);
		{
			Vec3d org(0., 0.0002, 0.), tail(0.0005, .00, .0088);
			Vec3d pos = mat.dvp3(org);
			double mag = air * mainRotorLiftFactor * rotoromega * (feather - airflux) * mass * dt;
			Vec3d thrust = rot2.vec3(1) * mag;
			if(bbody)
				bbody->applyForce(btvc(thrust), btvc(pos));
//			else
//				RigidAddMomentum(pt, pos, thrust);
			pos = mat.dvp3(tail);
			mag = air * tailRotorLiftFactor * rotoromega * this->tail * mass * dt;
			thrust = mat.vec3(0) * mag;
			if(bbody)
				bbody->applyForce(btvc(thrust), btvc(pos));
//			else
//				RigidAddMomentum(pt, pos, thrust);
		}
	}

	if(bbody){
		this->pos = btvc(bbody->getCenterOfMassPosition());
		this->rot = btqc(bbody->getWorldTransform().getRotation());
		this->velo = btvc(bbody->getLinearVelocity());
		this->omg = btvc(bbody->getAngularVelocity());
	}
	else{
		this->pos += this->velo * dt;
		double rate = inwater ? 1. : 0.;
		this->omg *= 1. / ((rate + .4) * dt + 1.);
		this->velo *= 1. / ((rate + air * .1) * dt + 1.);
	}
}



int Apache::takedamage(double damage, int hitpart){
	tent3d_line_list *tell = w->getTeline3d();
	int ret = 1;

#if 0
	playWave3DPitch(CvarGetString("sound_bullethit")/*"hit.wav"*/, pt->pos, w->pl->pos, w->pl->pyr, 1., .1, rseq(&w->rs) % 64 - 32 + 256);
#endif
	if(0 < health && health - damage <= 0){ /* death! */
//		effectDeath(w, pt);

		if(enemy)
			enemy = NULL;
//		playWave3D(CvarGetString("sound_blast")/*"blast.wav"*/, pt->pos, w->pl->pos, w->pl->pyr, 1., .01);
		ret = 0;
	}
	health -= damage;
	if(health < -100.){
		delete this;
	}
	return ret;
}

int Apache::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	double best = dt;
	int reti = 0, i = 0;
	double retf;
	const HitBoxList &hitboxes = getHitBoxes();
	for(HitBoxList::const_iterator it = hitboxes.begin(); it != hitboxes.end(); ++it){
		const hitbox &hb = *it;
		Vec3d org = this->rot.trans(hb.org) + this->pos;
		Quatd rot = this->rot * hb.rot;
		double sc[3];
		for(int j = 0; j < 3; j++)
			sc[j] = hb.sc[j] + rad;
		if((jHitBox(org, sc, rot, src, dir, 0., best, &retf, retp, retn)) && (retf < best)){
			best = retf;
			if(ret) *ret = retf;
			reti = i + 1;
		}
		++i;
	}
	return reti;
}

bool Apache::buildBody(){
	if(!bbody){
		static btCompoundShape *shape = NULL;
		if(!shape){
			shape = new btCompoundShape();
			const HitBoxList &hitboxes = getHitBoxes();
			for(HitBoxList::const_iterator i = hitboxes.begin(); i != hitboxes.end(); ++i){
				const HitBox &it = *i;
				const Vec3d &sc = it.sc;
				const Quatd &rot = it.rot;
				const Vec3d &pos = it.org;
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
		rbInfo.m_linearDamping = .05;
		rbInfo.m_angularDamping = .05;
		bbody = new btRigidBody(rbInfo);
		bbody->setActivationState(DISABLE_DEACTIVATION);

//		bbody->setSleepingThresholds(.0001, .0001);
	}
	return true;
}
