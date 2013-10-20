/** \file
 * \brief Implementation of base classes for aerial Entities such as aeroplanes.
 */

#define NOMINMAX
#include "Aerial.h"
#include "tent3d.h"
#include "yssurf.h"
#include "arms.h"
#include "sqadapt.h"
#include "btadapt.h"
#include "Game.h"
#include "tefpol3d.h"
#include "motion.h"
#include "SqInitProcess.h"
#include "cmd.h"
#include "StaticInitializer.h"
#include "Bullet.h"
#include "glw/GLWChart.h"
#include "SurfaceCS.h"
#include "Airport.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/rseq.h>
#include <clib/colseq/cs.h>
#include <clib/timemeas.h>
}
#include <cpplib/vec2.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <functional>



/* color sequences */
#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_firetrail[] = {
	{0.1, COLOR32RGBA(255, 255, 212, 0)},
	{0.1, COLOR32RGBA(255, 191, 191, 255)},
	{1.9, COLOR32RGBA(111, 111, 111, 255)},
	{1.9, COLOR32RGBA(63, 63, 63, 127)},
};
const struct color_sequence cs_firetrail = DEFINE_COLSEQ(cnl_firetrail, (COLOR32)-1, 4.0);



















void Aerial::WingProcess::process(HSQUIRRELVM v)const{
	sq_pushstring(v, name, -1); // root string

	// Not defining hardpoints is valid. Just ignore the case.
	if(SQ_FAILED(sq_get(v, -2))){ // root obj
		if(mandatory)
			throw SQFError(gltestp::dstring(name) << _SC(" not defined"));
		else
			return;
	}
	SQInteger len = sq_getsize(v, -1);
	if(-1 == len)
		throw SQFError(gltestp::dstring(name) << _SC(" size could not be acquired"));
	value.resize(len);
	for(int i = 0; i < len; i++){
		sq_pushinteger(v, i); // root obj i
		if(SQ_FAILED(sq_get(v, -2))) // root obj obj[i]
			continue;
		Wing &n = value[i];

		sq_pushstring(v, _SC("pos"), -1); // root obj obj[i] "pos"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].pos
			SQVec3d r;
			r.getValue(v, -1);
			n.pos = r.value;
			sq_poptop(v); // root obj obj[i]
		}
		else
			throw SQFError(gltestp::dstring(name) << _SC("[") << i << _SC("].pos not defined"));

		sq_pushstring(v, _SC("aero"), -1); // root obj obj[i] "aero"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].aero
			for(auto j = 0; j < 9; j++){
				sq_pushinteger(v, j); // root obj obj[i] obj[i].aero j
				if(SQ_FAILED(sq_get(v, -2))) // root obj obj[i] obj[i].aero obj[i].aero[j]
					throw SQFError(gltestp::dstring(name) << _SC("[") << i << _SC("].aero[") << j << _SC("] is lacking"));
				SQFloat f;
				if(SQ_FAILED(sq_getfloat(v, -1, &f)))
					throw SQFError(gltestp::dstring(name) << _SC("[") << i << _SC("].aero[") << j << _SC("] is not compatible with float"));
				n.aero[j] = f;
				sq_poptop(v); // root obj obj[i] obj[i].aero
			}
			sq_poptop(v); // root obj obj[i]
		}
		else
			throw SQFError(gltestp::dstring(name) << _SC("[") << i << _SC("].aero not defined"));

		sq_pushstring(v, _SC("name"), -1); // root obj obj[i] "name"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].name
			const SQChar *sqstr;
			if(SQ_SUCCEEDED(sq_getstring(v, -1, &sqstr)))
				n.name = sqstr;
			else // Throw an error because there's no such thing like default name.
				throw SQFError("WingProcess: name is not specified");
			sq_poptop(v); // root obj obj[i]
		}
		else
			n.name = gltestp::dstring("wing") << i;

		sq_pushstring(v, _SC("control"), -1); // root obj obj[i] "control"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].control
			const SQChar *sqstr;
			if(SQ_SUCCEEDED(sq_getstring(v, -1, &sqstr))){
				if(!scstrcmp(sqstr, _SC("aileron")))
					n.control = Wing::Control::Aileron;
				else if(!scstrcmp(sqstr, _SC("elevator")))
					n.control = Wing::Control::Elevator;
				else if(!scstrcmp(sqstr, _SC("rudder")))
					n.control = Wing::Control::Rudder;
				else
					throw SQFError("WingProcess: unknown control surface type");
			}
			else // Throw an error because there's no such thing like default name.
				throw SQFError("WingProcess: control is not specified");
			sq_poptop(v); // root obj obj[i]
		}
		else
			n.control = Wing::Control::None;

		sq_pushstring(v, _SC("axis"), -1); // root obj obj[i] "axis"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].axis
			SQVec3d r;
			r.getValue(v, -1);
			n.axis = r.value;
			sq_poptop(v); // root obj obj[i]
		}
		else
			n.axis = Vec3d(1,0,0); // Defaults X+

		sq_pushstring(v, _SC("sensitivity"), -1); // root obj obj[i] "sensitivity"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].sensitivity
			SQFloat f;
			sq_getfloat(v, -1, &f);
			n.sensitivity = f;
			sq_poptop(v); // root obj obj[i]
		}
		else if(n.control != Wing::Control::None)
			throw SQFError(gltestp::dstring(name) << _SC("[") << i << _SC("].sensitivity not defined"));

		sq_poptop(v); // root obj
	}
	sq_poptop(v); // root
}

void Aerial::CameraPosProcess::process(HSQUIRRELVM v)const{
	sq_pushstring(v, name, -1); // root string

	// Not defining hardpoints is valid. Just ignore the case.
	if(SQ_FAILED(sq_get(v, -2))){ // root obj
		if(mandatory)
			throw SQFError(gltestp::dstring(name) << _SC(" not defined"));
		else
			return;
	}
	SQInteger len = sq_getsize(v, -1);
	if(-1 == len)
		throw SQFError(gltestp::dstring(name) << _SC(" size could not be acquired"));
	value.resize(len);
	for(int i = 0; i < len; i++){
		sq_pushinteger(v, i); // root obj i
		if(SQ_FAILED(sq_get(v, -2))) // root obj obj[i]
			continue;
		CameraPos &n = value[i];

		sq_pushstring(v, _SC("pos"), -1); // root obj obj[i] "pos"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].pos
			SQVec3d r;
			r.getValue(v, -1);
			n.pos = r.value;
			sq_poptop(v); // root obj obj[i]
		}
		else
			throw SQFError(gltestp::dstring(name) << _SC("[") << i << _SC("].pos not defined"));

		sq_pushstring(v, _SC("rot"), -1); // root obj obj[i] "rot"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].rot
			SQQuatd r;
			r.getValue(v, -1);
			n.rot = r.value;
			sq_poptop(v); // root obj obj[i]
		}
		else
			n.rot = quat_u;

		sq_pushstring(v, _SC("type"), -1); // root obj obj[i] "control"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].control
			const SQChar *sqstr;
			if(SQ_SUCCEEDED(sq_getstring(v, -1, &sqstr))){
				if(!scstrcmp(sqstr, _SC("normal")))
					n.type = CameraPos::Type::Normal;
				else if(!scstrcmp(sqstr, _SC("cockpit")))
					n.type = CameraPos::Type::Cockpit;
				else if(!scstrcmp(sqstr, _SC("missiletrack")))
					n.type = CameraPos::Type::MissileTrack;
				else if(!scstrcmp(sqstr, _SC("viewtrack")))
					n.type = CameraPos::Type::ViewTrack;
				else
					throw SQFError("CameraPosProcess: unknown camera type");
			}
			else
				throw SQFError("CameraPosProcess: camera type must be a string");
			sq_poptop(v); // root obj obj[i]
		}
		else
			n.type = CameraPos::Type::Normal;

		sq_poptop(v); // root obj
	}
	sq_poptop(v); // root
}

void Aerial::init(){
	this->weapon = 0;
}

bool Aerial::buildBody(){
	if(!bbody){
		btCompoundShape *shape = getShape();
		if(!shape)
			return false;
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
	return true;
}

btCompoundShape *Aerial::buildShape(){
	btCompoundShape *shape = new btCompoundShape();
	for(auto it : getHitBoxes()){
		const Vec3d &sc = it.sc;
		const Quatd &rot = it.rot;
		const Vec3d &pos = it.org;
		btBoxShape *box = new btBoxShape(btvc(sc));
		btTransform trans = btTransform(btqc(rot), btvc(pos));
		shape->addChildShape(trans, box);
	}
	return shape;
}

/// \return Defaults 1
short Aerial::bbodyGroup()const{
	return 1;
}

/// \return Defaults all bits raised
short Aerial::bbodyMask()const{
	return ~0;
}

void Aerial::addRigidBody(WarSpace *ws){
	if(ws && ws->bdw){
		buildBody();
		//add the body to the dynamics world
		if(bbody)
			ws->bdw->addRigidBody(bbody, bbodyGroup(), bbodyMask());
	}
}

Aerial::Aerial(Game *game) : st(game), fspoiler(0), spoiler(false), destPos(0,0,0), destArrived(false), takingOff(false){
}

Aerial::Aerial(WarField *w) : st(w),
	iaileron(0),
	brake(false), afterburner(false), navlight(false),
	gear(false), gearphase(0), fspoiler(0), spoiler(false),
	showILS(false), destPos(0,0,0),
	destArrived(false), onFeet(false), takingOff(false)
{
	moi = .2; /* kilograms * kilometer^2 */
	init();
}



Flare::Flare(Game *game) : st(game){
	velo = vec3_000;
	rot = quat_u;
	health = 1.;
	enemy = NULL;
	health = 500;
#ifndef DEDICATED
	TefpolList *tl = w->getTefpol3d();
	if(tl)
		pf = tl->addTefpolMovable(pos, velo, vec3_000, &cs_firetrail, TEP3_THICKER | TEP3_ROUGH, cs_firetrail.t);
	else
		pf = NULL;
	ttl = 5.;
#endif
}







void Aerial::control(const input_t *inputs, double dt){
	if(health <= 0.)
		return;
	this->inputs = *inputs;
	if(.1 * .1 <= velo[0] * velo[0] + velo[2] * velo[2]){
	}
	if(inputs->press & (PL_LCLICK | PL_ENTER)){
		shoot(dt);
	}
	aileron = rangein(approach(aileron, aileron + inputs->analog[0] * 0.01, dt, 0.), -1, 1);
	elevator = rangein(approach(elevator, elevator + inputs->analog[1] * 0.01, dt, 0.), -1, 1);
}

void Aerial::shoot(double dt){}

static double angularDamping = 20.;

void Aerial::anim(double dt){
	int inwater = 0;
	bool onfeet = false;
	int walking = 0;
	int direction = 0;

	if(bbody){
		bbody->activate();
		pos = btvc(bbody->getCenterOfMassPosition());
		velo = btvc(bbody->getLinearVelocity());
		rot = btqc(bbody->getOrientation());
		omg = btvc(bbody->getAngularVelocity());
	}

//	w->orientation(w, &ort3, pt->pos);
	Mat3d ort3 = mat3d_u();
/*	VECSADD(pt->velo, w->gravity, dt);*/
	Vec3d accel = w->accel(pos, velo);
	velo += accel * dt;

	double air = w->atmosphericPressure(pos);

	Mat4d mat;
	transform(mat);

	int inputs = this->inputs.press;

	if(0. < health){
		double common = 0., normal = 0., best = .3;
//		entity_t *pt2;
/*		common = inputs & PL_W ? M_PI / 4. : inputs & PL_S ? -M_PI / 4. : 0.;
		normal = inputs & PL_A ? M_PI / 4. : inputs & PL_D ? -M_PI / 4. : 0.;*/
		if(this->controller == game->player){
//			avec3_t pyr;
//			quat2pyr(w->pl->rot, pyr);
//			common = -pyr[0];
//			normal = -pyr[1];
		}
		else{
			Mat4d mat = this->rot.tomat4();
			Mat3d mat3 = mat.tomat3();
//			MATTRANSPOSE(iort3, ort3);
//			MATMP(mat2, iort3, mat3);
			Mat3d mat2 = mat3;
#if 1
			common = this->velo.sp(ort3.vec3(1)) / 5e-0;
			normal = rangein(-mat2[1] / .7, -M_PI / 4., M_PI / 4.);
#else
			common = /*-mat[9] / .1 */pt->velo[1] / 5e-0;
			normal = rangein(-mat[1] / .7, -M_PI / 4., M_PI / 4.);
#endif
		}
		if(controller)
			rudder = rangein(approach(rudder, (inputs & PL_A ? -M_PI / 6. : inputs & PL_D ? M_PI / 6. : 0), 1. * M_PI * dt, 0.), -M_PI / 6., M_PI / 6.);

		fspoiler = approach(fspoiler, spoiler, dt, 0);

		if(bbody)
			bbody->setFriction(0.1 + 0.9 * brake);

		gearphase = approach(gearphase, gear, 1. * dt, 0.);

		if(inputs & PL_W)
			throttle = approach(throttle, 1., .5 * dt, 5.);
		if(inputs & PL_S){
			throttle = approach(throttle, 0., .5 * dt, 5.);
			if(afterburner && throttle < .5)
				afterburner = 0;
		}

		if(this->inputs.change & this->inputs.press & PL_RCLICK)
			toggleWeapon();
		Vec3d nh = this->rot.quatrotquat(Vec3d(0., 0., -1.));
		for(auto pt2 : w->entlist()){
			if(pt2 != this && pt2->race != -1 && pt2->race != this->race){
				Vec3d delta = pt2->pos - this->pos;
				double c = nh.sp(delta) / delta.len();
				if(best < c && delta.slen() < 5. * 5.){
					best = c;
					this->enemy = pt2;
					break;
				}
			}
		}

	}
	else
		throttle = approach(throttle, 0, dt, 0);
	this->inputs.press = 0;
	this->inputs.change = 0;

	/* forget about beaten enemy */
	if(this->enemy && this->enemy->getHealth() <= 0.)
		this->enemy = NULL;

/*	h = warmapheight(w->map, pt->pos[0], pt->pos[2], NULL);*/
//	if(w->vft->inwater)
//		inwater = w->vft->inwater(w, pt->pos);

	if(cooldown < dt)
		cooldown = 0;
	else
		cooldown -= dt;



	if(!inwater && 0. < health){
		Vec3d zh0(0., 0., 1.);

		Vec3d zh = mat.dvp3(zh0);
		double thrustsum = -getThrustStrength() * this->mass * this->throttle * air * (1. + this->afterburner);
		zh *= thrustsum;
		if(bbody)
			bbody->applyCentralForce(btvc(zh));
//		RigidAddMomentum(pt, avec3_000, zh);
	}

#if 1
	if(air){
		const double f = 1.;

		double velolen = velo.slen();

		force.clear();
		// Acquire wing positions
		for(auto it : getWings()){
			// Position relative to center of gravity with rotation in world coordinates.
			Vec3d rpos = mat.dvp3(it.pos);

			Quatd rot = this->rot;
			if(it.control == Wing::Control::Elevator)
				rot *= Quatd::rotation(elevator * it.sensitivity, it.axis);
			else if(it.control == Wing::Control::Aileron)
				rot *= Quatd::rotation(aileron * it.sensitivity, it.axis);
			else if(it.control == Wing::Control::Rudder)
				rot *= Quatd::rotation(getRudder() * it.sensitivity, it.axis);

			/* retrieve velocity of the wing center in absolute coordinates */
			Vec3d velo = this->omg.vp(rpos) + this->velo;

			double len = velolen < .05 ? velolen / .05 : velolen < .340 ? 1. : velolen / .340;

			// Calculate attenuation factor
			double f2 = f * air * this->mass * .15 * len;

			Mat3d aeroBuf;
			const Mat3d *aeroRef = &it.aero;
			if(it.control == Wing::Control::Aileron){
				aeroBuf = it.aero;

				// Increase drag by amplifying diagonal elements
				const double f = 2. * fspoiler;
				aeroBuf[0] *= 1. + f;
//				aeroBuf[4] *= 1. + f; // Don't increase vertical drag, it will make aileron sensitivity look higher.
				aeroBuf[8] *= 1. + f;

				// Spoil lift force
				aeroBuf[7] /= 1. + 4. * fspoiler;

				aeroRef = &aeroBuf;
			}

			// Calculate the drag in local coordinates
			Vec3d v1 = rot.itrans(velo);
			Vec3d v = (*aeroRef * v1) * f2;
			Vec3d v2 = rot.trans(v);
			this->force.push_back(v2);
			if(bbody)
				bbody->applyForce(btvc(v2), btvc(rpos));
//			RigidAddMomentum(pt, wings[i], v1);
		}

		// Drag force generated by fuselage and landing gear.  The factor can be tweaked.
		if(bbody){
			double f = air * 0.01 * (1. + gear);
			bbody->setDamping(f, angularDamping * f);
		}
	}
#endif

	this->pos += this->velo * dt;
}

gltestp::dstring &operator<<(gltestp::dstring &ds, Vec3d pos){
	ds << "(" << pos[0] << "," << pos[1] << "," << pos[2] << ")";
	return ds;
}

Entity::Props Aerial::props()const{
	Props &ret = st::props();
	ret.push_back(gltestp::dstring("Aileron: ") << aileron);
	ret.push_back(gltestp::dstring("Elevator: ") << elevator);
	ret.push_back(gltestp::dstring("Throttle: ") << throttle);
	for(int i = 0; i < force.size(); i++)
		ret.push_back(gltestp::dstring("Force") << i << ": " << force[i]);
	return ret;
}

SQInteger Aerial::sqGet(HSQUIRRELVM v, const SQChar *name)const{
	if(!scstrcmp(name, _SC("brake"))){
		sq_pushbool(v, brake);
		return 1;
	}
	else if(!scstrcmp(name, _SC("gear"))){
		sq_pushbool(v, gear);
		return 1;
	}
	else if(!scstrcmp(name, _SC("spoiler"))){
		sq_pushbool(v, spoiler);
		return 1;
	}
	else if(!scstrcmp(name, _SC("weapon"))){
		sq_pushinteger(v, weapon);
		return 1;
	}
	else if(!scstrcmp(name, _SC("afterburner"))){
		sq_pushbool(v, SQBool(afterburner));
		return 1;
	}
	else if(!scstrcmp(name, _SC("destPos"))){
		SQVec3d r;
		r.value = destPos;
		r.push(v);
		return 1;
	}
	else if(!scstrcmp(name, _SC("destArrived"))){
		sq_pushbool(v, SQBool(destArrived));
		return 1;
	}
	else if(!scstrcmp(name, _SC("onFeet"))){
		sq_pushbool(v, SQBool(onFeet));
		return 1;
	}
	else if(!scstrcmp(name, _SC("takingOff"))){
		sq_pushbool(v, SQBool(takingOff));
		return 1;
	}
	else if(!scstrcmp(name, _SC("showILS"))){
		sq_pushbool(v, SQBool(showILS));
		return 1;
	}
	else if(!scstrcmp(name, _SC("landingAirport"))){
		sq_pushobj(v, landingAirport);
		return 1;
	}
	else
		return st::sqGet(v, name);
}

SQInteger Aerial::sqSet(HSQUIRRELVM v, const SQChar *name){
	auto boolSetter = [&](const SQChar *name, bool &value){
		SQBool b;
		if(SQ_FAILED(sq_getbool(v, 3, &b)))
			return sq_throwerror(v, gltestp::dstring("could not convert to bool ") << name);
		value = b != SQFalse;
		return SQRESULT(0);
	};
	if(!scstrcmp(name, _SC("brake")))
		return boolSetter(_SC("brake"), brake);
	else if(!scstrcmp(name, _SC("gear")))
		return boolSetter(_SC("gear"), gear);
	else if(!scstrcmp(name, _SC("spoiler")))
		return boolSetter(_SC("spoiler"), spoiler);
	else if(!scstrcmp(name, _SC("weapon"))){
		SQInteger i;
		if(SQ_FAILED(sq_getinteger(v, 3, &i)))
			return sq_throwerror(v, _SC("Argument type must be compatible with integer"));
		weapon = i;
		return 0;
	}
	else if(!scstrcmp(name, _SC("afterburner"))){
		SQBool b;
		if(SQ_FAILED(sq_getbool(v, 3, &b)))
			return sq_throwerror(v, _SC("Argument type must be compatible with bool"));
		afterburner = b;

		// You cannot keep afterburner active without sufficient fuel supply
		if(afterburner && throttle < .7)
			throttle = .7;
		return 0;
	}
	else if(!scstrcmp(name, _SC("destPos"))){
		SQVec3d r;
		r.getValue(v, 3);
		destPos = r.value;
		return 0;
	}
	else if(!scstrcmp(name, _SC("destArrived")))
		return boolSetter(_SC("destArrived"), destArrived);
	else if(!scstrcmp(name, _SC("takingOff")))
		return boolSetter(_SC("takingOff"), takingOff);
	else if(!scstrcmp(name, _SC("showILS"))){
		return boolSetter(_SC("showILS"), showILS);
	}
	else if(!scstrcmp(name, _SC("landingAirport"))){
		landingAirport = sq_refobj(v, 3);
		return 0;
	}
	else
		return st::sqSet(v, name);
}

static Serializable::Id monitor = 0;
static double estimateSpeed = 0.5; ///< Virtual approaching speed for estimating position
static double rollSense = 2.;
static double omgSense = 2.;
static double intSense = -1.;
static double intMax = 1.;
static double turnClimb = 0.3;
static double turnBank = 2. / 5. * M_PI;
static double rudderSense = -20.; ///< Rudder stabilizer sensitivity
static double rudderAim = 17.; ///< Aim strength factor for rudder
static double landingGSOffset = -0.12; ///< Offset to Glide Slope of ILS when approaching runway

static void initSense(){
	CvarAdd("estimateSpeed", &estimateSpeed, cvar_double);
	CvarAdd("rollSense", &rollSense, cvar_double);
	CvarAdd("omgSense", &omgSense, cvar_double);
	CvarAdd("intSense", &intSense, cvar_double);
	CvarAdd("intMax", &intMax, cvar_double);
	CvarAdd("omgDamp", &angularDamping, cvar_double);
	CvarAdd("turnClimb", &turnClimb, cvar_double);
	CvarAdd("turnBank", &turnBank, cvar_double);
	CvarAdd("rudderSense", &rudderSense, cvar_double);
	CvarAdd("rudderAim", &rudderAim, cvar_double);
	CvarAdd("landingGSOffset", &landingGSOffset, cvar_double);
}

static StaticInitializer initializer(initSense);

double Aerial::getRudder()const{
	double yawDeflect = velo.norm().sp(rot.trans(Vec3d(1,0,0)));
	if(getid() == monitor)
		GLWchart::addSampleToCharts("yawDeflect", yawDeflect);
	return rangein(rudder + rudderSense * yawDeflect, -1, 1);
}

/// \brief Taxiing motion, should be called in the derived class's anim().
/// \returns if touched a ground.
bool Aerial::taxi(double dt){
	if(!bbody)
		return false;
	bool ret = false;
	WarSpace *ws = *w;
	if(ws){
		int num = ws->bdw->getDispatcher()->getNumManifolds();
		for(int i = 0; i < num; i++){
			btPersistentManifold* contactManifold = ws->bdw->getDispatcher()->getManifoldByIndexInternal(i);
			const btCollisionObject* obA = static_cast<const btCollisionObject*>(contactManifold->getBody0());
			const btCollisionObject* obB = static_cast<const btCollisionObject*>(contactManifold->getBody1());
			if(obA == bbody || obB == bbody){
				int numContacts = contactManifold->getNumContacts();
				for(int j = 0; j < numContacts; j++){
					btManifoldPoint& pt = contactManifold->getContactPoint(j);
					if(pt.getDistance() < 0.f){
						const btVector3& ptA = pt.getPositionWorldOnA();
						const btVector3& ptB = pt.getPositionWorldOnB();
						if(ptA.getY() < bbody->getCenterOfMassPosition().getY() || ptA.getY() < bbody->getCenterOfMassPosition().getY()){
							btVector3 btvelo = bbody->getLinearVelocity();
							btScalar len = btvelo.length();
							const double threshold = 0.015;
							double run = len * dt;

							// Sensitivity is dropped when the vehicle is in high speed, to prevent falling by centrifugal force.
							double sensitivity = -M_PI * 5. * (len < threshold ? 1. : threshold / len);

							btTransform wt = bbody->getWorldTransform();

							// Assume steering direction to be controlled by rudder.
							wt.setRotation(wt.getRotation() * btQuaternion(btVector3(0,1,0), rudder * run * sensitivity));
							bbody->setWorldTransform(wt);

							// Cancel lateral velocity to enable steering. This should really be anisotropic friction, because
							// it is possible that the lift suppress the friction force, but our method won't take it into account.
							bbody->setLinearVelocity(btvelo - wt.getBasis().getColumn(0) * wt.getBasis().getColumn(0).dot(btvelo));
							ret = true;
						}
					}
				}
			}
		}
	}
	// Remember the last state as a member variable for later reference from a Squirrel script.
	onFeet = ret;
	return ret;
}

inline double sqGetter(HSQUIRRELVM v, const SQChar *name){
	StackReserver sr(v);
	sq_pushstring(v, name, -1);
	if(SQ_FAILED(sq_get(v, -2)))
		throw SQFError(gltestp::dstring("Name not found: ") << name);
	SQFloat f;
	if(SQ_FAILED(sq_getfloat(v, -1, &f)))
		throw SQFError(gltestp::dstring("Could not convert to double: ") << name);
	return f;
}

inline bool sqBoolGetter(HSQUIRRELVM v, const SQChar *name, bool raise = true){
	try{
		StackReserver sr(v);
		sq_pushstring(v, name, -1);
		if(SQ_FAILED(sq_get(v, -2)))
			throw SQFError(gltestp::dstring("Name not found: ") << name);
		SQBool f;
		if(SQ_FAILED(sq_getbool(v, -1, &f)))
			throw SQFError(gltestp::dstring("Could not convert to bool: ") << name);
		return f != SQFalse;
	}
	catch(SQFError &e){
		// Re-throw only if desired
		if(raise)
			throw;
	}
}


/// \brief Taxiing AI
void Aerial::animAI(double dt, bool onfeet){
	Mat4d mat;
	transform(mat);

	onfeet = onfeet || velo.slen() < 0.05 * 0.05 && 0.9 < mat.vec3(1)[1];

	Vec3d deltaPos((!onfeet && enemy ? enemy->pos : destPos) - this->pos); // Delta position towards the destination
	if(!(!onfeet && enemy))
		deltaPos[1] = 0.; // Ignore height component
	double sdist = deltaPos.slen();

	// If the body is stationary and upright, assume it's on the ground.
	if(onfeet){
		aileron = approach(aileron, 0, dt, 0);
		Vec3d localDeltaPos = this->rot.itrans(deltaPos);
		double phi = atan2(localDeltaPos[0], -localDeltaPos[2]);
		const double arriveDist = 0.03;
		if(landingAirport){
			// If we are landing and decelerated below a very slow speed by landing gear brake,
			// just assume we have landed.  Clearing landingAirport would trigger the next action.
			if(velo.slen() < 0.01 * 0.01){
				landingAirport = nullptr;
				if(showILS)
					showILS = false;
			}
		}
		else if(takingOff){
			if(destArrived || sdist < arriveDist * arriveDist){
				destArrived = true;
				throttle = approach(throttle, 1, dt, 0);
				rudder = approach(rudder, 0, dt, 0);
			}
			else{
				throttle = approach(throttle, fabs(phi) < 0.05 * M_PI ? 1 : 0.2, dt, 0);
				rudder = rangein(approach(rudder, phi, 1. * M_PI * dt, 0.), -M_PI / 6., M_PI / 6.);
			}
			elevator = approach(elevator, 1, dt, 0);
			brake = false;
			spoiler = false;
		}
		else if(destArrived || sdist < arriveDist * arriveDist){
			destArrived = true;
			throttle = approach(throttle, 0, dt, 0);
			rudder = approach(rudder, 0, dt, 0);
			brake = true;
			spoiler = true;
		}
		else{
			double velolen = velo.len();
			throttle = approach(throttle, std::max(0., 0.2 - velolen * 20. + (fabs(phi) < 0.05 * M_PI ? 0.1 : 0.)), dt, 0);
			rudder = rangein(approach(rudder, phi, 1. * M_PI * dt, 0.), -M_PI / 6., M_PI / 6.);
			brake = false;
			spoiler = false;
		}
		gear = true;
	}
	else{
		// Automatic stabilizer (Auto Pilot)

		// Set timer for gear retraction if we're taking off
		if(takingOff)
			takeOffTimer = 5.;

		// Count down for gear retraction
		if(takeOffTimer < dt){
			takeOffTimer = 0.;
			gear = false;
		}
		else
			takeOffTimer -= dt;

		Mat3d mat2 = mat.tomat3();
		double turnRange = 3.;
		bool landing = false;
		if(landingAirport){
			GetILSCommand gic;
			if(landingAirport->command(&gic) && 0 < (gic.pos - this->pos).sp(-mat.vec3(2))){
				landing = true;
				if(!showILS)
					showILS = true; // Turn on ILS display on the HUD.
				deltaPos = gic.pos - this->pos;
			}

			turnRange = 5; // Landing requires careful positioning than just pass over a landmark, so we need more working space.
		}

		bool crashing = false; // We're going to crash in 3 seconds!
		if(!landing && &w->cs->getStatic() == &SurfaceCS::classRegister){
			SurfaceCS *sc = static_cast<SurfaceCS*>(w->cs);

			// Ray trace the terrain to see if the ground is in front of us.
			if(sc->traceHit(this->pos, this->velo, 0., 3.)){
				crashing = true;
			}
		}

		Vec3d estPos;
		estimate_pos(estPos, deltaPos, enemy ? enemy->velo - this->velo : -this->velo, vec3_000, vec3_000, estimateSpeed, nullptr);
		double rudderTarget = 0;
		double trim;
		std::function<double()> throttler = [&](){return (0.5 - velo.len()) * 2.;};
		if(crashing){
			// If we are going to crash, do not turn around to approach the target; you can do that later.
			// Instead just fly up to clear the obstacles.
			trim = 1.; // Full uptrim
		}
		else{
			double sp = estPos.sp(mat.vec3(2));
			double turning = 0;
			std::function<double()> targetClimber = [&](){
				Vec2d planar(deltaPos[0], deltaPos[2]);
				return deltaPos.sp(mat.vec3(2)) < 0 ? rangein(deltaPos[1] / planar.len() , -0.5, 0.5) : 0;
			};

			double turnAngle = 0;
			if(landing){
				try{
					HSQUIRRELVM v = game->sqvm;
					StackReserver sr(v);
					sq_pushroottable(v);
					sq_pushstring(v, _SC("aerialLanding"), -1);
					if(SQ_FAILED(sq_get(v, -3)))
						throw SQFError(gltestp::dstring("aerialLanding function not found"));
					sq_pushroottable(v);
					sq_pushobj(v, this);
					if(SQ_FAILED(sq_call(v, 2, SQTrue, SQTrue)))
						throw SQFError(gltestp::dstring("aerialLanding function is not callable"));

					turnAngle = sqGetter(v, _SC("roll"));
					turning = std::min(1., fabs(turnAngle));

					double climb = sqGetter(v, _SC("climb"));
					targetClimber = [climb](){return climb;};

					double thro = sqGetter(v, _SC("throttle"));
					throttler = [thro](){return thro;};

					brake = sqBoolGetter(v, _SC("brake"));
					spoiler = sqBoolGetter(v, _SC("spoiler"));
					gear = sqBoolGetter(v, _SC("gear"));
				}
				catch(SQFError &e){
					CmdPrint(gltestp::dstring("aerialLanding Error: ") << e.what());
				}
			}
			else if(0.5 * 0.5 < sdist){
				if(turnRange * turnRange < sdist && 0. < sp){ // Going away
					// Turn around to get closer to target.
					turnAngle += -mat.vec3(2).sp(estPos) < 0 ? turnBank : -turnBank;
					turning = 1;
				}
				else{
					estPos.normin();
					turning = estPos.vp(-mat.vec3(2))[1] * (1. - fabs(estPos[1]));
					if(3. * 3. < sdist || sp < 0){ // Approaching
						turnAngle += turnBank * turning;
						rudderTarget = -turning * rudderAim;
					}
					else // Receding
						turnAngle += -turnBank * turning;
					turning = fabs(turning);
				}

				// Upside down; get upright as soon as possible or we'll crash!
				if(mat.vec3(1)[1] < 0){
					turning = 1;
					turnAngle = mat.vec3(0)[1] < 0 ? -turnBank : turnBank;
				}
			}
			else
				destArrived = true;

			mat2 = mat.tomat3().rotz(turnAngle);

			double targetClimb = targetClimber();

			// You cannot control altitude and yaw at the same time efficiently. Let's do one at a time.
			trim = mat.vec3(2)[1] + velo[1] + (1. - turning) * targetClimb + turning * turnClimb; // P + D
		}

		Vec3d x_along_y = Vec3d(mat.vec3(0)[0], 0, mat.vec3(0)[2]).normin();
		double croll = -x_along_y.sp(mat2.vec3(1)); // Current Roll
		double rollOmg = mat.vec3(2).sp(omg);

		if(monitor == 0)
			monitor = getid();
		GLWchart::addSampleToCharts("croll", croll);
		GLWchart::addSampleToCharts("omg", rollOmg);
		GLWchart::addSampleToCharts("iaileron", iaileron);

		double roll = croll * rollSense + rollOmg * omgSense + iaileron * intSense; // P + D + I
		iaileron = rangein(iaileron + roll * dt, -intMax, intMax);

		aileron = rangein(aileron + roll * dt, -1, 1);
		throttle = approach(throttle, rangein(throttler(), 0, 1), dt, 0);
		rudder = approach(rudder, rudderTarget, dt, 0);
		elevator = rangein(elevator + trim * dt, -1, 1);
		takingOff = false;

		if(enemy){
			double apparentRadius = enemy->getHitRadius() / deltaPos.len() * 2.;
			double sp = -mat.vec3(2).sp(deltaPos.norm());
			if(1. - apparentRadius < sp)
				shoot(dt);
		}
	}
}

void Aerial::calcCockpitView(Vec3d &pos, Quatd &rot, const CameraPos &cam)const{
	Mat4d mat;
	transform(mat);
	rot = this->rot;
	if(cam.type == CameraPos::Type::MissileTrack){
		if(lastMissile){
			pos = lastMissile->pos + lastMissile->rot.trans(Vec3d(0, 0.002, 0.005));
			rot = lastMissile->rot;
			return;
		}
		else
			pos = mat.vp3(cam.pos);
	}
	else if(cam.type == CameraPos::Type::ViewTrack && enemy){
		Player *player = game->player;
		rot = this->rot * Quatd::direction(this->rot.cnj().trans(this->pos - enemy->pos));
		pos = this->pos + rot.trans(Vec3d(cam.pos[0], cam.pos[1], cam.pos[2] / player->fov)); // Trackback if zoomed
	}
	else if(cam.type == CameraPos::Type::Rotate){
		Vec3d pos0;
		const double period = this->velo.len() < .1 * .1 ? .5 : 1.;
		struct contact_info ci;
		pos0[0] = floor(this->pos[0] / period + .5) * period;
		pos0[1] = 0./*floor(pt->pos[1] / period + .5) * period*/;
		pos0[2] = floor(this->pos[2] / period + .5) * period;
/*		if(w && w->->pointhit && w->vft->pointhit(w, pos0, avec3_000, 0., &ci))
			pos0 += ci.normal * ci.depth;
		else if(w && w->vft->spherehit && w->vft->spherehit(w, pos0, .002, &ci))
			pos0 += ci.normal * ci.depth;*/
		pos = pos0;
	}
	else{
		pos = mat.vp3(cam.pos);
	}
}





void Flare::anim(double dt){
	if(pos[1] < 0. || ttl < dt){
		if(game->isServer())
			delete this;
		return;
	}
	ttl -= dt;
	if(pf)
		pf->move(pos, vec3_000, cs_firetrail.t, 0);
	velo += w->accel(pos, velo) * dt;
	pos += velo * dt;
}
