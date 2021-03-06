/** \file
 * \brief Impelementation of Apache class.
 */
#define NOMINMAX
#include "Apache.h"
#include "Player.h"
#include "ExplosiveBullet.h"
#include "motion.h"
#include "SurfaceCS.h"
#include "btadapt.h"
#include "sqadapt.h"
#ifndef DEDICATED
#include "tefpol3d.h"
#endif
#include "Launcher.h"
#include "audio/playSound.h"
#include "audio/wavemixer.h"
extern "C"{
#include <clib/cfloat.h>
}
#include <cpplib/crc32.h>

#include <assert.h>


#define APACHE_SMOKE_FREQ 10.
#define APACHE_PRICE 1450e1

#define SSM_MASS 80.
#define BULLETSPEED 80.
#define SONICSPEED 340.
#define WINGSPEED (.25 * M_PI)






Entity::EntityRegister<Apache> Apache::entityRegister("Apache");

Model *Apache::model = NULL;
double Apache::modelScale = 0.01;
double Apache::defaultMass = 5000.;
double Apache::maxHealthValue = 100.;
double Apache::rotorAxisSpeed = 0.1 * M_PI;
double Apache::mainRotorLiftFactor = 23;
double Apache::mainRotorTorqueFactor = 1.;
double Apache::mainRotorDragFactor = 0.05;
double Apache::tailRotorLiftFactor = 1.0;
double Apache::featherSpeed = 1.;
double Apache::tailRotorSpeed = 3.;
double Apache::chainGunCooldown = 60. / 625.; ///< Defaults 625 rpm
double Apache::chainGunMuzzleSpeed = 1000.;
double Apache::chainGunDamage = 30.;
double Apache::chainGunVariance = 0.015;
double Apache::chainGunLife = 5.;
double Apache::hydraDamage = 300.;
HSQOBJECT Apache::sqFire = sq_nullobj();
HSQOBJECT Apache::sqQueryAmmo = sq_nullobj();
Vec3d Apache::cockpitOfs = Vec3d(0., .8, -2.2);
HitBoxList Apache::hitboxes;
Vec3d Apache::hudPos;
double Apache::hudSize;
GLuint Apache::overlayDisp = 0;
std::vector<hardpoint_static*> Apache::hardpoints;
StringList Apache::defaultArms;
StringList Apache::weaponList;


void Apache::init(){
	static bool initialized = false;
	if(!initialized){
		SqInit(game->sqvm, modPath() << _SC("models/apache.nut"),
			SingleDoubleProcess(modelScale, "modelScale") <<=
			SingleDoubleProcess(defaultMass, "mass") <<=
			SingleDoubleProcess(maxHealthValue, "maxhealth", false) <<=
			SingleDoubleProcess(rotorAxisSpeed, "rotorAxisSpeed", false) <<=
			SingleDoubleProcess(mainRotorLiftFactor, "mainRotorLiftFactor", false) <<=
			SingleDoubleProcess(mainRotorTorqueFactor, "mainRotorTorqueFactor", false) <<=
			SingleDoubleProcess(mainRotorDragFactor, "mainRotorDragFactor", false) <<=
			SingleDoubleProcess(tailRotorLiftFactor, "tailRotorLiftFactor", false) <<=
			SingleDoubleProcess(featherSpeed, "featherSpeed") <<=
			SingleDoubleProcess(tailRotorSpeed, "tailRotorSpeed") <<=
			SingleDoubleProcess(chainGunCooldown, "chainGunCooldown") <<=
			SingleDoubleProcess(chainGunMuzzleSpeed, "chainGunMuzzleSpeed") <<=
			SingleDoubleProcess(chainGunDamage, "chainGunDamage") <<=
			SingleDoubleProcess(chainGunVariance, "chainGunVariance") <<=
			SingleDoubleProcess(chainGunLife, "chainGunLife") <<=
			SingleDoubleProcess(hydraDamage, "hydraDamage") <<=
			Vec3dProcess(cockpitOfs, "cockpitOfs") <<=
			HitboxProcess(hitboxes) <<=
			Vec3dProcess(hudPos, "hudPos") <<=
			SingleDoubleProcess(hudSize, "hudSize") <<=
			DrawOverlayProcess(overlayDisp) <<=
			SqCallbackProcess(sqFire, "fire") <<=
			SqCallbackProcess(sqQueryAmmo, "queryAmmo") <<=
			HardPointProcess(hardpoints) <<=
			StringListProcess(defaultArms, "defaultArms") <<=
			StringListProcess(weaponList, "weaponList"));
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
	health = getMaxHealth();
	mass = 5000.;

	// Setup default configuration for arms
	for(int i = 0; i < hardpoints.size(); i++){
		if(defaultArms.size() <= i)
			return;
		ArmBase *arm = defaultArms[i] == "HydraRocketLauncher" ? (ArmBase*)new HydraRocketLauncher(this, hardpoints[i]) :
			defaultArms[i] == "HellfireLauncher" ? (ArmBase*)new HellfireLauncher(this, hardpoints[i]) : NULL;
		if(arm){
			arms.push_back(arm);
			if(w)
				w->addent(arm);
		}
	}

//	while(p->sw) ShieldWaveletFree(&p->sw);
/*	for(i = 0; i < numof(p->sw); i++)
		p->sw[i].phase = p->sw[i].amount = 0.;*/
}

Apache::Apache(WarField *w) : st(w), rotorSid(0){
	init();
}

void Apache::leaveField(WarField *w){
	if(rotorSid){
		stopsound3d(rotorSid);
		rotorSid = 0;
	}
}

void Apache::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	amat4_t mat2, mat3;
	static const Vec3d apache_overview_ofs(0., 10., 25.);
	int camera = (seatid + 6) % 6;
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
			dr0 *= 25.;
			dr0 += dr2 * 100.;
			pos = this->pos + dr0;
		}
		else{
			pos = mat.vp3(apache_overview_ofs);
		}
	}
	else if(camera == 3){
		Vec3d pos0;
		const double period = this->velo.slen() < 100. * 100. ? 500. : 1000.;
//		struct contact_info ci;
		pos0[0] = floor(this->pos[0] / period + .5) * period;
		pos0[1] = floor(this->pos[1] / period + .5) * period;
		pos0[2] = floor(this->pos[2] / period + .5) * period;
//		if(w && w->vft->spherehit && w->vft->spherehit(w, pos0, .002, &ci))
//			VECSADD(pos0, ci.normal, ci.depth);
		pos = pos0;
	}
	else if(camera == 4){
		if(lastMissile){
			pos = lastMissile->pos + lastMissile->rot.trans(Vec3d(0, 2., 5.));
			rot = lastMissile->rot;
			return;
		}
		else
			pos = mat.vp3(cockpitOfs);
	}
	else if(camera == 5){
		// Rotation along with the rotor.  Useful on debugging rotor blade pitch control.
		Vec3d pos0 = Vec3d(sin(rotor), .35, cos(rotor)) * 10000.;
		pos = mat.vp3(pos0);
		Quatd rot0 = Quatd::rotation(rotor, 0, 1, 0);
		rot = this->rot * rot0;
		return;
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
		double best = 3000. * 3000.;
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

	// Switch weapons
	if(inputs->change & inputs->press & PL_RCLICK){
		weapon = (weapon + 1) % 4;
	}
}

/** determine if target can be hit by ballistic bullets from the starting position.
 * returns desired angles also if feasible. */
int shoot_angle(const Vec3d &pos, const Vec3d &epos, double phi0, double v, double (*ret_angles)[2], bool farside = false, const Vec3d &gravity = vec3_000){
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

void Apache::find_enemy_logic(){
	double best = 100.e3 * 100.e3; /* sense range */
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
	bool onfeet = taxi(dt);

	bool inwater = false;

	Mat4d mat;
	transform(mat);
	double air = w->atmosphericPressure(pos);

	if(WarSpace *ws = *w){
		const Vec3d offset(0, 1., 0);
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
			gcommon = common = crotoraxis[0] = rangein(crotoraxis[0] + M_PI * .001 * inputs.analog[1], -M_PI / 6., M_PI / 6.);
			gnormal = normal = crotoraxis[1] = rangein(crotoraxis[1] - M_PI * .001 * inputs.analog[0], -M_PI / 6., M_PI / 6.);
		}
		else{
			Mat3d mat3 = mat.tomat3();
			Mat3d matt = iort3 * mat3;
			// Orientation stabilizer that tries to control cyclic to get the vehicle upright.
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
			estimate_pos(epos, enemy->pos, enemy->velo, this->pos, this->velo, chainGunMuzzleSpeed, w);
			Vec3d delta = epos - this->pos;
			double sd = delta.slen();
			Vec3d ldelta = qc.trans(delta);
			ldelta[1] -= 2.;
			shoot_angle(vec3_000, ldelta, 0., chainGunMuzzleSpeed, &retangles);
			gnormal = -atan2(ldelta[0], -ldelta[2]);
			gcommon = retangles[0]/*- -asin(ldelta[1] / sqrt(ldelta[0] * ldelta[0] + ldelta[2] * ldelta[2]))*/;
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
			feather = approach(feather, rangein(!!onfeet + .5 * dist + 5. * (ort3.vec3(1).sp(enemy->pos) - ort3.vec3(1).sp(this->pos) + .2), 0., 1.), .2 * dt, 5.);
		if(inputs.press & PL_Q)
			feather = approach(feather, 1., featherSpeed * dt, 5.);
		if(inputs.press & PL_Z)
			feather = approach(feather, -.5, featherSpeed * dt, 5.);

		/* tail rotor control */
		if(!controller){
			if(enemy)
				tail = approach(tail, rangein(gnormal, -.2, .2), tailRotorSpeed * dt, 5.);
		}
		else{
			if(inputs.press & PL_A)
				tail = approach(tail, 1., tailRotorSpeed * dt, 5.);
			else if(inputs.press & PL_D)
				tail = approach(tail, -1., tailRotorSpeed * dt, 5.);
			else /*if(pt->inputs.press & PL_R)*/
				tail = approach(tail, 0., tailRotorSpeed * dt, 5.);
		}

		shoot(dt);
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
	Vec3d accel = w->accel(this->pos, this->velo);
	if(bbody){
		bbody->applyCentralForce(btvc(accel * mass));
	}
	else{
		this->velo += accel * dt;
	}

	{
		double airflux;
		Mat4d rot = mat.rotx(rotoraxis[0]);
		Mat4d rot2 = rot.rotz(rotoraxis[1]);

		/* momentum of the air flows into rotor's plane. downwards positive because we refer vehicle's velocity. */
		{
			Vec3d localvelo = this->velo;
			airflux = rot2.vec3(1).sp(localvelo) * air;
		}

		// Main rotor receives energy from engine */
		{
			double v = throttle * dt;
			rotoromega = rangein(rotoromega + v, -1., 5.);
		}
		rotor = fmod(rotor + rotoromega * 10. * M_PI * dt, M_PI * 2.);

		tailrotor = fmod(tailrotor + (rotoromega/* + p->tail*/) * 6. * M_PI * dt, M_PI * 2.);
		{
			double thrustForce = air * mainRotorLiftFactor * rotoromega * feather;
			double mag = thrustForce * mass;
			Vec3d thrust = rot2.vec3(1) * mag;
			// The main rotor's hub is above the center of gravity of the fuselage, so it could
			// generate torque by pure thrust, but it will complicate the simulation, so we ignore it for now.
			if(bbody)
				bbody->applyCentralForce(btvc(thrust));
//			else
//				RigidAddMomentum(pt, pos, thrust);

			// Rotor looses energy by thrusting
			double drag = fabs(thrustForce) * mainRotorDragFactor;
			rotoromega *= exp(-drag * dt);

			if(bbody){
				double invInertia = bbody->getInvInertiaDiagLocal().length();
				if(0 < invInertia){
					// The main rotor's torque is induced by cyclic pitch change, not by the rotor plane's angle.
					double torque = air * mainRotorTorqueFactor * rotoromega / invInertia;
					bbody->applyTorque(btvc(mat.dvp3(Vec3d(rotoraxis[0], 0, rotoraxis[1])) * torque));
				}
			}

			Vec3d tail(.5, .00, 8.8); // Relative position of tail rotor from center of gravity
			Vec3d pos = mat.dvp3(tail);
			mag = air * tailRotorLiftFactor * rotoromega * this->tail * mass;
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

	// Align belonging arms at the end of frame
	for(ArmList::iterator it = arms.begin(); it != arms.end(); ++it) if(*it)
		(*it)->align();

	if(!rotorSid){
		rotorSid = playSound3D(modPath() << "sound/apache-rotor.ogg", this->pos, 0.5, 10000., 0, true);
	}
	else
		movesound3d(rotorSid, this->pos);
}



int Apache::takedamage(double damage, int hitpart){
	Teline3List *tell = w->getTeline3d();
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
		// We need to explicitly call leaveField() here because the destructor of Entity calls
		// Entity::leaveField(), not Apache::leaveField().  The derived class ought to be destroyed
		// by the time of super class's destruction, so it cannot call method of derived.
		// It means Entity::leaveField() is called twice.  Is there any nice way to avoid this?
		leaveField(w);
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

SQInteger Apache::sqGet(HSQUIRRELVM v, const SQChar *name)const{
	if(!scstrcmp(name, _SC("cooldown"))){
		if(!this){
			sq_pushnull(v);
			return 1;
		}
		sq_pushfloat(v, cooldown);
		return 1;
	}
	else if(!scstrcmp(name, _SC("arms"))){
		// Prepare an empty array in Squirrel VM for adding arms.
		sq_newarray(v, 0); // array

		// Retrieve library-provided "append" method for an array.
		// We'll reuse the method for all the elements, which is not exactly the same way as
		// an ordinally Squirrel codes evaluate.
		sq_pushstring(v, _SC("append"), -1); // array "append"
		if(SQ_FAILED(sq_get(v, -2))) // array array.append
			return sq_throwerror(v, _SC("append not found"));

		for(ArmList::const_iterator it = arms.begin(); it != arms.end(); it++){
			ArmBase *arm = *it;
			if(arm){
				sq_push(v, -2); // array array.append array
				Entity::sq_pushobj(v, arm); // array array.append array Entity-instance
				sq_call(v, 2, SQFalse, SQFalse); // array array.append
			}
		}

		// Pop the saved "append" method
		sq_poptop(v); // array

		return 1;
	}
	else if(!scstrcmp(name, _SC("ammo_chaingun"))){
		sq_pushinteger(v, ammo_chaingun);
		return 1;
	}
	else
		return st::sqGet(v, name);
}

SQInteger Apache::sqSet(HSQUIRRELVM v, const SQChar *name){
	if(!scstrcmp(name, _SC("cooldown"))){
		SQFloat retf;
		if(SQ_FAILED(sq_getfloat(v, 3, &retf)))
			return SQ_ERROR;
		cooldown = retf;
		return 0;
	}
	else if(!scstrcmp(name, _SC("ammo_chaingun"))){
		SQInteger ret;
		if(SQ_FAILED(sq_getinteger(v, 3, &ret)))
			return SQ_ERROR;
		ammo_chaingun = ret;
		return 0;
	}
	else
		return st::sqSet(v, name);
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

/// \brief Try to shoot the M230 chain gum mounted on the bottom of cockpit.
/// \returns Count of bullets shot in this frame
int Apache::shootChainGun(double dt){
	static const Vec3d pos0(0., -1.2, -3.2), nh0(0., 0., -1.), xh0(1., 0., 0.);
	double phi, theta;
	double scale = modelScale;
	double variance = chainGunVariance;
	int ret = 0;

	Mat4d mat;
	transform(mat);

	if((!controller && enemy || inputs.press & (PL_ENTER | PL_LCLICK))){
		bool shot = false;
		while(0 < ammo_chaingun && cooldown < dt){

			double t = w->war_time() + cooldown;
			RandomSequence rs(crc32(&t, sizeof t));
			Mat4d rmat = mat.translate(pos0[0], pos0[1], pos0[2])
				.rotx(gun[0] + (rs.nextd() - .5) * variance)
				.roty(gun[1] + (rs.nextd() - .5) * variance);

			Bullet *pb = new ExplosiveBullet(this, chainGunLife, chainGunDamage, 10., false);
			w->addent(pb);
			Vec3d nh = rmat.dvp3(nh0).norm();
			pb->velo = this->velo + nh * chainGunMuzzleSpeed;
			pb->pos = rmat.vec3(3);
			pb->mass = .05;
			Vec3d dr = pb->pos - this->pos;
			Vec3d momentum = pb->velo * -pb->mass;
			if(bbody)
				bbody->applyImpulse(btvc(momentum), btvc(dr));
	//		else
	//			RigidAddMomentum(pt, dr, momentum);
			pb->anim(dt - cooldown);
			cooldown += chainGunCooldown;
			ret++;

#if 0 // Apache's chain gun (M230) emits used cartridges on firing, so we should add them to temporary gibs, but we do not have a model yet.
			WarSpace *ws = *w;
			if(ws->gibs){
				Vec3d pos = rmat.vec3(3) - nh * .0005;
				Vec3d xh = rmat.vec3(0);
				Vec3d velo(
					this->velo[0] - (rs.nextd() + .5) * .002 * xh[0],
					this->velo[1] + .001 + rs.nextd() * .001,
					this->velo[2] - (rs.nextd() + .5) * .002 * xh[2]);
				pos += velo * dt;
				Vec3d omg(
					.5 * 2 * M_PI * (drseq(&w->rs) - .5),
					.5 * 2 * M_PI * (drseq(&w->rs) - .5),
					3. * 2 * M_PI * (drseq(&w->rs) - .5));
				AddTelineCallback3D(ws->gibs, pos, velo, 0.01, this->rot, omg, w->accel(pos, velo), small_brass_draw, NULL, TEL3_NOLINE | TEL3_REFLECT | TEL3_QUAT, 1.5 + w->rs.nextd());
			}
#endif
			this->muzzle = 1;
			this->ammo_chaingun--;
			shot = true;
		}
		if(shot)
			playSound3D(modPath() << "sound/apache-gunshot.ogg", this->pos, .6, 10000.);
	}
	return ret;
}

/// \brief Try to shoot rockets, missiles or gun.
void Apache::shoot(double dt){
	if(weapon == 0 || weapon == 1){
		shootChainGun(dt);
		return;
	}

//	playWave3D(CvarGetString("sound_gunshot"), pt->pos, w->pl->pos, w->pl->pyr, .6, .01, w->realtime + t);
	if((!controller && enemy || inputs.press & (PL_ENTER | PL_LCLICK))){
		HSQUIRRELVM v =game->sqvm;
		StackReserver sr(v);
		sq_pushobject(v, sqFire);
		sq_pushroottable(v);
		Entity::sq_pushobj(v, this);
		sq_pushfloat(v, dt);
		sq_call(v, 3, SQFalse, SQTrue);
	}
}

#ifdef DEDICATED
void Apache::draw(WarDraw *){}
void Apache::drawtra(WarDraw *){}
void Apache::drawHUD(WarDraw *){}
void Apache::drawCockpit(WarDraw *){}
#endif
