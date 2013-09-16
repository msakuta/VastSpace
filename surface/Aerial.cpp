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
#include "glw/GLWChart.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/rseq.h>
#include <clib/colseq/cs.h>
#include <clib/timemeas.h>
}
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>




/* color sequences */
#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_firetrail[] = {
	{0.1, COLOR32RGBA(255, 255, 212, 0)},
	{0.1, COLOR32RGBA(255, 191, 191, 255)},
	{1.9, COLOR32RGBA(111, 111, 111, 255)},
	{1.9, COLOR32RGBA(63, 63, 63, 127)},
};
const struct color_sequence cs_firetrail = DEFINE_COLSEQ(cnl_firetrail, (COLOR32)-1, 4.0);











#if 0
static void valkie_drawHUD(entity_t *pt, warf_t *, wardraw_t *, const double irot[16], void (*)(void));
static void valkie_cockpitview(entity_t *pt, warf_t*, double (*pos)[3], int *);
static void valkie_control(entity_t *pt, warf_t *w, input_t *inputs, double dt);
static void valkie_destruct(entity_t *pt);
static void valkie_anim(entity_t *pt, warf_t *w, double dt);
static void valkie_draw(entity_t *pt, wardraw_t *wd);
static void valkie_drawtra(entity_t *pt, wardraw_t *wd);
static int valkie_takedamage(entity_t *pt, double damage, warf_t *w);
static void valkie_gib_draw(const Teline3CallbackData *pl, void *pv);
static int valkie_flying(const entity_t *pt);
static void valkie_bullethole(entity_t *pt, sufindex, double rad, const double (*pos)[3], const double (*pyr)[3]);
static int valkie_getrot(struct entity*, warf_t *, double (*)[4]);
static void valkie_drawCockpit(struct entity*, const warf_t *, wardraw_t *);
static const char *valkie_idname(struct entity*pt){return "valkyrie";}
static const char *valkie_classname(struct entity*pt){return "Valkyrie";}

static struct entity_private_static valkie_s = {
	{
		fly_drawHUD/*/NULL*/,
		fly_cockpitview,
		fly_control,
		fly_destruct,
		fly_getrot,
		NULL, /* getrotq */
		fly_drawCockpit,
		NULL, /* is_warping */
		NULL, /* warp_dest */
		valkie_idname,
		valkie_classname,
		fly_start_control,
		fly_end_control,
		fly_analog_mask,
	},
	fly_anim,
	valkie_draw,
	fly_drawtra,
	fly_takedamage,
	fly_gib_draw,
	tank_postframe,
	fly_flying,
	M_PI,
	-M_PI / .6, M_PI / 4.,
	NULL, NULL, NULL, NULL,
	1,
	BULLETSPEED,
	0.020,
	FLY_SCALE,
	0, 0,
	fly_bullethole,
	{0., 20 * FLY_SCALE, .0},
	NULL, /* bullethit */
	NULL, /* tracehit */
	{NULL}, /* hitmdl */
	valkie_draw,
};

enum valkieform{ valkie_fighter, valkie_batloid, valkie_crouch, valkie_prone };

#define F(a) (1<<(a))

static const struct hardpoint_static valkie_hardpoints[1] = {
	{{FLY_SCALE * -55., FLY_SCALE * 5 - .0005, 0.}, {0,0,0,1}, "Right Hand", 0, F(armc_none) | F(armc_robothand)},
};

static arms_t valkie_arms[numof(valkie_hardpoints)] = {
	{arms_valkiegun, 1000},
};

typedef struct valkie{
	entity_t st;
	double aileron[2], elevator, rudder;
	double throttle;
	double gearphase;
	double cooldown;
	avec3_t force[5], sight;
	struct tent3d_fpol *pf, *vapor[2];
	char muzzle, brk, afterburner, navlight, gear;
	int missiles;
	sufdecal_t *sd;
	bhole_t *frei;
	bhole_t bholes[50];
	double torsopitch, twist;
	double vector[2];
	double leg[2], legr[2], downleg[2], foot[2];
	double arm[2], armr[2], downarm[2];
	double wing[2];
	double walkphase, batphase, crouchphase;
	double pitch, yaw;
	float walktime;
	enum valkieform bat;
	arms_t arms[1];
} valkie_t;
#endif


/*
int fly_togglenavlight(Player *pl){
	if(pl->control->vft == &fly_s || pl->control->vft == &valkie_s){
		((fly_t*)pl->control)->navlight = !((fly_t*)pl->control)->navlight;
		return 1;
	}
	return 0;
}

int fly_break(struct player *pl){
	if(pl->control->vft == &fly_s || pl->control->vft == &valkie_s){
		((fly_t*)pl->control)->brk = !((fly_t*)pl->control)->brk;
		return 1;
	}
	return 0;
}
*/









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

Aerial::Aerial(WarField *w) : st(w), iaileron(0), gear(false), gearphase(0), fspoiler(0), spoiler(false), destPos(0,0,0), destArrived(false), takingOff(false){
	moi = .2; /* kilograms * kilometer^2 */
	init();
}

#if 0
fly_t *ValkieNew(warf_t *w){
	fly_t *fly;
	valkie_t *p;
	entity_t *ret;
	int i;
	static int wingfile = 0;
	fly_loadwingfile();
	fly = (fly_t*)(p = (valkie_t*)malloc(sizeof *p));
	ret = &fly->st;
	EntityInit(ret, w, &valkie_s);
/*	VECNULL(ret->pos);
	VECNULL(ret->velo);
	VECNULL(ret->pyr);
	VECNULL(ret->omg);
	QUATZERO(ret->rot);
	ret->rot[3] = 1.;*/
	ret->mass = 12000.; /* kilograms */
	ret->moi = .2; /* kilograms * kilometer^2 */
	ret->health = 500.;
/*	ret->shoots = ret->shoots2 = ret->kills = ret->deaths = 0;
	ret->enemy = NULL;
	ret->active = 1;
	*(struct entity_private_static**)&ret->vft = &valkie_s;
	ret->next = w->tl;
	w->tl = ret;
	ret->health = 500;*/
	fly->pf = AddTefpolMovable3D(w->tepl, fly->st.pos, ret->velo, avec3_000, &cs_blueburn, TEP3_THICKER | TEP3_ROUGH, cs_blueburn.t);
	fly->vapor[0] = AddTefpolMovable3D(w->tepl, fly->st.pos, ret->velo, avec3_000, &cs_vapor, TEP3_FAINT | TEP3_ROUGH, cs_vapor.t * 10);
	fly->vapor[1] = AddTefpolMovable3D(w->tepl, fly->st.pos, ret->velo, avec3_000, &cs_vapor, TEP3_FAINT | TEP3_ROUGH, cs_vapor.t * 10);
	fly->sd = AllocSUFDecal(gsuf_fly);
	fly->sd->drawproc = bullethole_draw;
	fly_init(fly);
	p->torsopitch = 0.;
	p->twist = 0.;
	p->vector[0] = p->vector[1] = 0.;
	p->leg[0] = p->leg[1] = 0.;
	p->legr[0] = p->legr[1] = 0.;
	p->downleg[0] = p->downleg[1] = 0.;
	p->foot[0] = p->foot[1] = 0.;
	p->arm[0] = p->arm[1] = 0.;
	p->armr[0] = p->armr[1] = 0.;
	p->downarm[0] = p->downarm[1] = 0.;
	p->wing[0] = p->wing[1] = 0.;
	p->walkphase = 0.;
	p->batphase = 0.;
	p->crouchphase = 0.;
	p->pitch = p->yaw = 0.;
	p->bat = 0;
	memcpy(p->arms, valkie_arms, sizeof p->arms);
/*	if(!w->pl->chase)
		w->pl->chase = ret;*/
	return fly;
}
#endif


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

#if 0
#if 1
const char *valkie_bonenames[3] = {
	"GUN_MOVE",
	"LEFTARM_MOVE",
	"HEAD",
};
#else
const char *valkie_bonenames[30] = {

	/* root */
	"GW_BTL1",
/*	"a",*/

	/* torso */
/*	"BT_JOINTNULL1", "0007", "0006",*/
	"a", "a", "a",

	/* leg root */
/*	"0004", "0011",*/
	"a", "a",

	/* variable nozzle */
	"NZ-LT_AL", "NZ-LB_AL",
	"LZ-RT_AL", "LZ-RB_AL",

	/* up leg */
/*	"0016", "0023",*/
	"LEG1L_BTNZ", "LEG1R_BTNZ",

	/* down leg */
/*	"0019", "0027",*/
	"LEG2L_BT", "LEG2R_BT",

	/* up arm */
	"LEFTARM_MOVE", "GUN_MOVE",
/*	"0015", "0045",
	"0046",*/
	"a","a",
	"a",

	/* down arm */
/*	"LARM1_GW", "ARM2R_GW",*/
	"a","a",

	/* gun */
/*	"0049",*/
	"a",

	/* wings */
/*	"0005", 
	"0031", "0003",*/
	"a", 
	"a", "a",

	/* wings */
	"LWING_BTL", "RWING_BTL",

	/* head */
	"HEAD_BTNULL", "LASER_NULL",

	/* nose */
	/*"0009"*//*"NOSE_BTNULL"*/"a",
};
#endif

static ysdnmv_t *valkie_boneset(valkie_t *p, double (*rot)[7], ysdnmv_t *v, int forab){
#if 1
	static struct ysdnm_motion *motions[3];
	static struct ysdnm_motion *motbat, *motwalk, *motgerwalk, *motcrouch;
	static double valkie_rot[3][7];
	static ysdnmv_t *dnmv = NULL, gunmove = {0};
	static double lasttimes[3];
	double times[3];
	if(!motions[0]){
		motbat = YSDNM_MotionLoad("batloid.mot");
		motwalk = YSDNM_MotionLoad("walk.mot");
		motgerwalk = YSDNM_MotionLoad("gerwalk.mot");
		motcrouch = YSDNM_MotionLoad("crouch.mot");
		motions[2] = YSDNM_MotionLoad("gear.mot");
	}
	if(p->bat == valkie_crouch || p->bat == valkie_batloid){
		motions[1] = motcrouch;
		times[1] = p->crouchphase * 10;
	}
	else{
		motions[1] = motgerwalk;
		times[1] = p->vector[0] * 10;
	}
	if(p->bat && p->walkphase){
		motions[0] = motwalk;
		times[0] = (p->walkphase / (M_PI) - floor(p->walkphase / (M_PI))) * 40.;
	}
	else{
		motions[0] = motbat;
		times[0] = p->batphase * 20;
	}
	times[2] = p->batphase / .5 < p->gearphase ? p->gearphase * 20 : p->batphase * 20 / .5;
	if(dnmv && !memcmp(lasttimes, times, sizeof times))
		goto control_surfaces;
	else if(dnmv)
		YSDNM_MotionInterpolateFree(dnmv);
	dnmv = YSDNM_MotionInterpolate(motions, times, 3);
	memcpy(lasttimes, times, sizeof lasttimes);

/*	{
		ysdnmv_t **ppv;
		for(ppv = &dnmv; *ppv; ppv = &(*ppv)->next);
		*ppv = &gunmove;
	}*/
control_surfaces:
	dnmv->fcla = (1 << 1) | (1 << 8) | (1 << 7) | (1 << 6); /* weapon bay, aileron, elevator */
	dnmv->cla[1] = 1. - p->throttle;
	dnmv->cla[6] = p->elevator;
	dnmv->cla[7] = p->aileron[0];
	dnmv->cla[8] = p->rudder;
	dnmv->target = NULL;

	{
		int i;
		aquat_t q, qx, qy;
		qx[0] = sin(p->torsopitch / 2.);
		qx[1] = 0.;
		qx[2] = 0.;
		qx[3] = cos(p->torsopitch / 2.);
		v = &gunmove;
		v->bonenames = valkie_bonenames;
		v->bones = numof(valkie_bonenames);
		v->bonerot = valkie_rot;
		for(i = 0; i < 3; i++){
			QUATCPY(valkie_rot[i], qx);
			VECNULL(&valkie_rot[i][4]);
		}
		v->next = dnmv;
		return v;
	}

	return dnmv;
#else
	aquat_t q, q2, q3, qroot, qroot2;
	static const aquat_t q1 = {0,0,0,1};
	avec3_t omg, omg0;
	int i = 0;

	if(v){
		static const char *skipnames[] = {
/*			"LEG1L_BTNZ", "LEG1R_BTNZ",*/
			"0016", "0023",
			"ARM_FIGHTER",
			"AB1L", "AB2L", "AB1R", "AB2R",
			"AB1L_BT", "AB2L_BT", "AB1R_BT", "AB2R_BT",
			"LGEARHT_GW", "RGEAR-HT_GW"
		};
		v->fcla = (1 << 0) | (1 << 9) | (1 << 8) | (1 << 7) | (1 << 6) | (1 << 5); /* weapon bay, aileron, elevator */
		v->fviscla = (1 << 9) | (1 << 5);
		v->cla[0] = 1. - p->batphase;
		v->cla[5] = p->batphase;
		v->cla[6] = p->elevator;
		v->cla[7] = p->aileron[0];
		v->cla[8] = p->rudder;
		v->cla[9] = p->batphase;
		v->skipnames = skipnames;
		v->skips = numof(skipnames) - (p->bat ? 0 : 2);
		v->target = NULL;
		v->next = NULL;
	}

	/* root */
	q[0] = 0.;
	q[1] = sin(p->twist * .5);
	q[2] = 0.;
	q[3] = sqrt(1. - q[1] * q[1]);
	q2[0] = 0./*sin(p->wing[0] * .75)*/;
	q2[1] = 0.;
	q2[2] = 0.;
	q2[3] = 1./*sqrt(1. - q2[0] * q2[0])*/;
	QUATMUL(qroot, q, q2);
	QUATCPY(rot[i], qroot);
	i++;

	/* torso */
	q[0] = 0./*sin(-p->wing[0] * 2.3)*/;
	q[1] = 0.;
	q[2] = 0.;
	q[3] = 1./*cos(-p->wing[0] * 2.3)*/;
/*	quatrotquat(rot[i++], omg, q1);*/
	QUATCPY(rot[i], q);
	QUATMUL(qroot2, qroot, q);
	i++;

	/* colors */
	q[0] = sin(p->wing[0] * .9);
	q[1] = 0.;
	q[2] = 0.;
	q[3] = sqrt(1. - q[0] * q[0]);
	QUATCPY(rot[i], q);
	QUATMUL(qroot, qroot2, q);
	i++;

	/* body-headjoint */
	q[0] = sin(-p->batphase * M_PI / 6.);
	q[1] = 0.;
	q[2] = 0.;
	q[3] = sqrt(1. - q[0] * q[0]);
	QUATCPY(rot[i], q);
	i++;

	/* leg root */
	q[0] = sin(p->batphase * 0 * -M_PI / 6.);
	q[1] = 0.;
	q[2] = 0.;
	q[3] = sqrt(1. - q[0] * q[0]);
	QUATCPY(rot[i], q);
	i++;

	q[0] = sin(p->batphase * -M_PI / 6. /*.75*/);
	q[1] = 0.;
	q[2] = 0.;
	q[3] = sqrt(1. - q[0] * q[0]);
	QUATCPY(rot[i], q);
	i++;

	/* variable nozzle ties to aileron, elevator and rudder */
	if(forab){
		omg[0] = (p->aileron[1] + p->elevator) * .5;
		omg[1] = 0.;
		omg[2] = 0.;
		quatrotquat(q, omg, q1);
		omg[0] = 0.;
		omg[1] = p->rudder * .5;
		omg[2] = 0.;
		quatrotquat(rot[i++], omg, q);
		QUATCPY(rot[i], rot[i-1]);
		i++;

		omg[0] = (p->aileron[0] + p->elevator) * .5;
		omg[1] = 0.;
		omg[2] = 0.;
		quatrotquat(q, omg, q1);
		omg[0] = 0.;
		omg[1] = p->rudder * .5;
		omg[2] = 0.;
		quatrotquat(rot[i++], omg, q);
		QUATCPY(rot[i], rot[i-1]);
		i++;
	}
	else{
		aquat_t q2;
		q2[0] = sin(p->batphase * M_PI / 6.);
		q2[1] = 0.;
		q2[2] = 0.;
		q2[3] = sqrt(1. - q[0] * q[0]);

		omg[0] = (p->aileron[1] + p->elevator) * 0 * .5 + p->foot[0] + p->batphase * M_PI / 8.;
		omg[1] = 0.;
		omg[2] = 0.;
		quatrotquat(q, omg, q1);
		omg[0] = 0;
		omg[1] = 0 * p->rudder * .5;
		omg[2] = 0.;
		quatrotquat(rot[i++], omg, q);
		omg[0] = (p->aileron[1] + p->elevator) * 0 * .5 + p->foot[0] - p->batphase * M_PI / 8.;
		omg[1] = 0.;
		omg[2] = 0.;
		quatrotquat(q, omg, q1);
		omg[0] = 0;
		omg[1] = 0 * p->rudder * .5;
		omg[2] = 0.;
		quatrotquat(rot[i++], omg, q);

		omg[0] = (p->aileron[0] + p->elevator) * 0 * .5 + p->foot[1] + p->batphase * M_PI / 8.;
		omg[1] = 0.;
		omg[2] = 0.;
		quatrotquat(q, omg, q1);
		omg[0] = 0;
		omg[1] = 0 * p->rudder * .5;
		omg[2] = 0.;
		quatrotquat(rot[i++], omg, q);
		omg[0] = (p->aileron[0] + p->elevator) * 0 * .5 + p->foot[1] - p->batphase * M_PI / 8.;
		omg[1] = 0.;
		omg[2] = 0.;
		quatrotquat(q, omg, q1);
		omg[0] = 0;
		omg[1] = 0 * p->rudder * .5;
		omg[2] = 0.;
		quatrotquat(rot[i++], omg, q);
	}

	/* up legs */
	omg0[0] = p->leg[0] + p->batphase * -M_PI / 6./* - p->wing[0] * .5*/;
	omg0[1] = 0.;
	omg0[2] = 0.;
	q[0] = 0.;
	q[2] = 0.;
	q[1] = sin(p->legr[0]);
	q[3] = cos(p->legr[0]);
	quatrot(omg, q, omg0);
/*		quatrotquat(q, omg, q1);
		QUATMUL(q2, qroot, q);
		quatrot(q3, q2, avec3_001);
		VECSCALEIN(q3, sin(p->twist * .5));
		q3[3] = cos(p->twist * .5);
		QUATMUL(rot[i], q, q3);
		i++;*/
	quatrotquat(rot[i++], omg, q1);
	omg0[0] = p->leg[1] + p->batphase * -M_PI / 6./* + p->wing[1] * .5*/;
	q[1] = sin(p->legr[1]);
	q[3] = cos(p->legr[0]);
	quatrot(omg, q, omg0);
/*		quatrotquat(q, omg, q1);
		QUATMUL(q2, qroot, q);
		quatrot(q3, q2, avec3_001);
		VECSCALEIN(q3, sin(p->twist * .5));
		q3[3] = cos(p->twist * .5);
		QUATMUL(rot[i], q, q3);
		i++;*/
	quatrotquat(rot[i++], omg, q1);

	omg[0] = p->downleg[0];
	omg[1] = 0.;
	omg[2] = 0.;
	quatrotquat(rot[i++], omg, q1);
	omg[0] = p->downleg[1];
	quatrotquat(rot[i++], omg, q1);

	/* arm base */
	q[0] = sin((p->torsopitch) * .5);
	q[1] = 0.;
	q[2] = 0.;
	q[3] = sqrt(1. - q[0] * q[0]);
	q2[0] = 0.;
	q2[1] = sin(-p->twist * .5);
	q2[2] = 0.;
	q2[3] = sqrt(1. - q2[1] * q2[1]);
	QUATMUL(rot[i], q2, q);
	i++;
	QUATCPY(rot[i], rot[i-1]);
	i++;
/*	q2[0] = 0.;
	q2[1] = 0.;
	q2[2] = sin((p->arm[1]) * .5);
	q2[3] = sqrt(1. - q2[2] * q2[2]);
	QUATMUL(q3, q, q2);
	q[0] = -sin(p->wing[0] * .75 + p->wing[0] * .7 - p->wing[0] * 2.2);
	q[3] = cos(p->wing[0] * .75 + p->wing[0] * .7 - p->wing[0] * 2.2);
	QUATMUL(rot[i], q, q3);
	QUATCPY(rot[i], q1);
	i++;*/
/*	omg0[1] = p->arm[1];
	q[1] = sin(p->armr[1] * .5);
	q[3] = cos(p->armr[1] * .5);
	quatrot(omg, q, omg0);
	quatrot(q2, q, avec3_100);
	VECSCALEIN(q2, sin(p->torsopitch * .5));
	q2[3] = cos(p->torsopitch * .5);
	quatrotquat(rot[i++], omg, q1);*/
/*	quatrotquat(q2, omg, q1);
	QUATMUL(rot[i], q2, q);
	i++;*/
/*	quatrotquat(rot[i++], omg, q);*/

	/* uparm */
	q2[0] = 0.;
	q2[1] = sin(p->batphase * .5 * M_PI * .5);
	q2[2] = 0.;
	q2[3] = sqrt(1. - q2[1] * q2[1]);
	QUATCPY(rot[i], q2);
	i++;

	q2[0] = 0.;
	q2[1] = sin(-p->batphase * 1.5 * M_PI * .5);
	q2[2] = 0.;
	q2[3] = cos(-p->batphase * 1.5 * M_PI * .5);
	QUATCPY(rot[i], q2);
	i++;

/*	q[0] = 0.;
	q[1] = sin(-p->batphase * M_PI * .5);
	q[2] = 0.;
	q[3] = sqrt(1. - q[1] * q[1]);*/
	q2[0] = sin(-p->batphase * .5 * M_PI * .5 + p->batphase * .25 * M_PI * .5);
	q2[1] = 0.;
	q2[2] = 0.;
	q2[3] = cos(-p->batphase * .5 * M_PI * .5 + p->batphase * .25 * M_PI * .5);
	QUATCPY(rot[i], q2);
/*	QUATMUL(rot[i], q, q2);*/
	i++;

	omg[0] = -p->batphase * M_PI * .25;
	omg[1] = 0. /*-p->downarm[0]*/;
	omg[2] = 0.;
	quatrotquat(rot[i++], omg, q1);
/*	omg[1] = -p->downarm[1]*/;
	quatrotquat(rot[i++], omg, q1);

	/* gun */
	q[0] = sin(p->wing[0] * 2.);
	q[1] = 0.;
	q[2] = 0.;
	q[3] = sqrt(1. - q[0] * q[0]);
	QUATCPY(rot[i], q);
	i++;

	/* wings */
	q[0] = sin(p->wing[0] * (4. / 3.) - -p->batphase * M_PI / 6.);
	q[1] = 0.;
	q[2] = 0.;
	q[3] = sqrt(1. - q[0] * q[0]);
/*	quatrotquat(rot[i++], omg, q1);*/
	QUATCPY(rot[i], q);
	i++;
/*	omg[0] = p->wing[0] * 4.;
	omg[1] = 0.;
	omg[2] = 0.;
	quatrotquat(rot[i++], omg, q1);*/

	omg[0] = 0.;
	omg[1] = 0.;
	omg[2] = p->wing[0];
	quatrotquat(rot[i++], omg, q1);
	omg[2] = p->wing[1];
	quatrotquat(rot[i++], omg, q1);

	omg[0] = 0.;
	omg[1] = p->wing[0] * .5 - p->throttle * .25;
	omg[2] = 0.;
	quatrotquat(rot[i++], omg, q1);
	omg[1] = p->wing[1] * .5 + p->throttle * .25;
	quatrotquat(rot[i++], omg, q1);

	/* head */
	QUATZERO(q);
	q[0] = sin(p->batphase * M_PI * .25 * -(1.25 - .5) + p->torsopitch * .5 + p->batphase * M_PI / 4.);
	q[3] = cos(p->batphase * M_PI * .25 * -(1.25 - .5) + p->torsopitch * .5 + p->batphase * M_PI / 4.);
	QUATZERO(q2);
	q2[1] = sin(-p->twist * .5);
	q2[3] = cos(-p->twist * .5);
	QUATMUL(rot[i], q2, q);
	i++;
/*	quatrotquat(rot[i++], omg, q);*/

	q[0] = sin(-p->wing[0] * .5);
	q[1] = 0.;
	q[2] = 0.;
	q[3] = sqrt(1. - q[0] * q[0]);
	QUATCPY(rot[i], q);
	i++;

	/* nose */
	q[0] = sin(-p->wing[0] * 2.);
	q[1] = 0.;
	q[2] = 0.;
	q[3] = sqrt(1. - q[0] * q[0]);
	QUATCPY(rot[i], q);
	i++;
/*	omg[0] = -p->wing[0] * 4.;
	omg[1] = 0.;
	omg[2] = 0.;
	quatrotquat(rot[i++], omg, q1);*/
#endif
}


static void fly_destruct(entity_t *pt){
	fly_t *fly = (fly_t*)pt;
	if(fly->sd)
		FreeSUFDecal(fly->sd);
	if(fly->vapor[0])
		ImmobilizeTefpol3D(fly->vapor[0]);
	if(fly->vapor[1])
		ImmobilizeTefpol3D(fly->vapor[1]);
}
#endif


/*static int dropflare(entity_t *pt, warf_t *w, double dt){
	fly_t *p = (fly_t*)pt;
	struct bullet *pb;
	struct ssm *ssm;
	flare_t *flare;
	if(p->cooldown < dt){
		flare = FlareNew(w, pt->pos);
		VECCPY(flare->st.velo, pt->velo);
		flare->st.race = -1;
		for(pb = w->bl; pb; pb = pb->next) if(pb->active && pb->vft->get_homing_target(pb) == pt && rseq(&w->rs) % 2 == 0){
			((struct ssm*)pb)->target = &flare->st;
		}
		p->cooldown = FLARE_INTERVAL;
	}
}

#define SAMPLERATE 11025

void shootsound(entity_t *pt, warf_t *w, double delay){
	static int init = 0;
	static BYTE wave[512];
	static WAVEHDR wh;
	avec3_t deltav, delta;
	double sp;
	if(!init){
		int i;
		init = 1;
		for(i = 0; i < numof(wave); i++)
			wave[i] = (BYTE)(255 * (1.
			+ (i < numof(wave) / 4 ? .5 * cos((double)i / SAMPLERATE * 2 * M_PI * 1100) : 0.)
			+ .5 * cos((double)i * pow(2, i * 10. / SAMPLERATE) / SAMPLERATE * 2 * M_PI * 330) * (numof(wave) - i) / numof(wave)
			) / 2.);
//		display(wave, numof(wave), 1);
	}
	VECSUB(deltav, pt->velo, w->pl->velo);
	VECSUB(delta, pt->pos, w->pl->pos);
	sp = VECSP(deltav, delta) / VECLEN(delta);
	if(sp <= wave_sonic_speed){
		extern double dwo;
		playMemoryWave3D(wave, sizeof wave, -sp * 32 / wave_sonic_speed, pt->pos, w->pl->pos, w->pl->pyr, .5, .01, w->realtime + delay);
	}

}*/

static void find_gun(const char *name, double *hint){
	if(!strcmp(name, "GunBase")){
		static const avec3_t pos = {.13, -.37, 3.};
		amat4_t mat;
		glGetDoublev(GL_MODELVIEW_MATRIX, mat);
		mat4vp3(hint, mat, pos);
	}
}

static ysdnm_t *dnm;





void Aerial::control(const input_t *inputs, double dt){
	if(health <= 0.)
		return;
	this->inputs = *inputs;
/*	if(inputs->change)
		p->brk = !!(inputs->press & PL_B);*/
	if(.1 * .1 <= velo[0] * velo[0] + velo[2] * velo[2]){
/*		if(inputs & PL_A){
			pt->pyr[1] = fmod(pt->pyr[1] + M_PI - FLY_YAWSPEED * dt, 2 * M_PI) - M_PI;
		}
		if(inputs & PL_D){
			pt->pyr[1] = fmod(pt->pyr[1] + M_PI + FLY_YAWSPEED * dt, 2 * M_PI) - M_PI;
		}
		if(inputs & PL_S)
			pt->pyr[0] = fmod(pt->pyr[0] + M_PI + FLY_PITCHSPEED * dt, 2 * M_PI) - M_PI;
		if(inputs & PL_W)
			pt->pyr[0] = fmod(pt->pyr[0] + M_PI - FLY_PITCHSPEED * dt, 2 * M_PI) - M_PI;*/
	}
/*	if(fly_flying(pt)){
		if(inputs->press & PL_Z)
			dropflare(pt, w, dt);
	}*/
	if(inputs->press & (PL_LCLICK | PL_ENTER)){
		shoot(dt);
	}
	aileron = rangein(approach(aileron, aileron + inputs->analog[0] * 0.01, dt, 0.), -1, 1);
	elevator = rangein(approach(elevator, elevator + inputs->analog[1] * 0.01, dt, 0.), -1, 1);
/*	pt->pyr[2] = approach(pt->pyr[2] + M_PI, (pt->pyr[1] - oldyaw) / dt + M_PI, FLY_ROLLSPEED * dt, 2 * M_PI) - M_PI;*/
}

void Aerial::shoot(double dt){}

/*
static unsigned fly_analog_mask(const entity_t *pt, const warf_t *w){
	pt;w;
	return 3;
}
*/

//extern struct player *ppl;

struct afterburner_hint{
	int ab;
	int muzzle;
	double thrust;
	double gametime;
	avec3_t wingtips[3];
};

static void find_wingtips(const char *name, struct afterburner_hint *hint);

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


#if 0
	if(health <= 0){
		enemy = NULL;
//		deaths++;
//		if(race < numof(w->races))
//			w->races[pt->race].deaths++;
		health = 500;
		if(w->vft->pointhit == WarPointHit){
			pt->pos[0] = 0.;
			pt->pos[2] = 3.5 + drseq(&w->rs) * .2;
			pt->pos[1] = /*h =*/ warmapheight(w->map, pt->pos[0], pt->pos[2], NULL)
	#if FLY_QUAT
				+ .002;
	#endif
				;
			VECNULL(pt->rot);
			pt->rot[3] = 1.;
/*			if(0. < h)*/{
	/*			pt->pos[1] += .2;*/
	/*			pt->velo[2] = -100.;*/
			}
		}
		else{
			pt->pos[0] = 0.;
			pt->pos[1] = -.3;
			pt->pos[2] = 3.25;
			VECNULL(pt->rot);
			pt->rot[3] = -(pt->rot[0] = sqrt(1. / 2.));
		}
		VECNULL(pt->velo);
/*		pt->velo[2] = -.1;*/
		VECNULL(pt->pyr);
		VECNULL(pt->omg);
		pt->pyr[1] = 0.;
		pt->active = 1;
		((fly_t*)pt)->pf = AddTefpolMovable3D(w->tepl, pt->pos, pt->velo, avec3_000, &cs_blueburn, TEP3_THICKER | TEP3_ROUGH, cs_blueburn.t);
		fly_init((fly_t*)pt);
		pt->inputs.change = pt->inputs.press = 0;
	}
#endif

//	w->orientation(w, &ort3, pt->pos);
	Mat3d ort3 = mat3d_u();
/*	VECSADD(pt->velo, w->gravity, dt);*/
	Vec3d accel = w->accel(pos, velo);
	velo += accel * dt;

	double air = w->atmosphericPressure(pos);

	Mat4d mat;
	transform(mat);

#if 0
	/* batloid walk move */
	if(vft == &valkie_s){
		valkie_t *p = (valkie_t*)pt;
		/*if(p->bat && !p->afterburner)*/ if(dnm){
			int inputs = pt->inputs.press, ii1, ii2 = 0, i;
			struct contact_info ci, ci2;
			avec3_t feetpos[4], feetpos1, feetpos0[5] = {{0., -.007, 0.}, {0., -.007, 0.}};
			double rot[numof(valkie_bonenames)][7];
			avec3_t up;
			double f;
			ysdnmv_t *dnmv;
			glPushMatrix();
			glLoadIdentity();
			gldScaled(1e-3);
/*			glTranslated(0, p->batphase * -7, 0.);
			glTranslated(0, 0, 5. * p->wing[0] / (M_PI / 4.));*/
			dnmv = valkie_boneset(p, rot, NULL, 0);
			TransYSDNM_V(dnm, dnmv, foot_height, &feetpos0);
/*			TransYSDNM(dnm, valkie_bonenames, rot, numof(valkie_bonenames), NULL, 0, foot_height, &feetpos0);*/

			/* vapor */
			if(air){
				struct afterburner_hint hint;
				TransYSDNM_V(dnm, dnmv, find_wingtips, &hint);
/*				TransYSDNM(dnm, valkie_bonenames, rot, numof(valkie_bonenames), NULL, 0, find_wingtips, &hint);*/
				for(i = 0; i < 2; i++) if(p->vapor[i]){
					avec3_t pos;
					int skip;
					skip = !(.1 < -VECSP(pt->velo, &mat[8]));
	//				VECSCALEIN(feetpos0[i+2], 1e-3);
					mat4vp3(pos, mat, hint.wingtips[i]);
					MoveTefpol3D(p->vapor[i], pos, avec3_000, cs_vapor.t, skip);
				}
			}

			glPopMatrix();
			mat4vp3(feetpos[0], mat, feetpos0[0]);
			mat4vp3(feetpos[1], mat, feetpos0[1]);

/*			VECSCALEIN(feetpos0[0], 1.8);*/
			feetpos0[0][1] -= .007; 
			mat4vp3(feetpos1, mat, feetpos0[0]);

			ii1 = w->vft->pointhit(w, feetpos[0], pt->velo, dt, &ci);
			ii2 = w->vft->pointhit(w, feetpos[1], pt->velo, dt, &ci2);

			if(ii1 || ii2){
				if(!ii1 || ii2 && ci.depth < ci2.depth)
					ci = ci2;
				VECSADD(pt->pos, ci.normal, ci.depth * 1.0001);
				f = -VECSP(pt->velo, ci.normal);
				if(0. < f)
					VECSADD(pt->velo, ci.normal, f);
				p->walktime = .75;
			}
			if(p->bat && !p->afterburner && p->walktime)
				onfeet = 1;
			if(p->walktime < dt)
				p->walktime = 0.;
			else
				p->walktime -= dt;

			if(p->bat && !p->afterburner && w->vft->pointhit(w, feetpos1, pt->velo, dt, &ci)){
/*				onfeet = 1;*/

/*				if(.0005 < ci.depth)
					ci.depth -= .0005;*/

			}

			if(p->bat && !onfeet){
				if(inputs & PL_A){
/*					avec3_t omg;
					VECSCALE(omg, &mat[4], YAWSPEED * dt);
					VECADDIN(pt->omg, omg);
					quatrot(omg, pt->rot, avec3_010);
					VECSCALEIN(omg, YAWSPEED * dt);
					quatrotquat(pt->rot, omg, pt->rot);*/
				}
				if(inputs & PL_D){
/*					avec3_t omg;
					VECSCALE(omg, &mat[4], -YAWSPEED * dt);
					VECADDIN(pt->omg, omg);
					quatrot(omg, pt->rot, avec3_010);
					VECSCALEIN(omg, -YAWSPEED * dt);
					quatrotquat(pt->rot, omg, pt->rot);*/
				}
				if(inputs & PL_7){
					avec3_t omg;
					VECSCALE(omg, &mat[8], M_PI * dt);
					VECADDIN(pt->omg, omg);
/*					quatrot(omg, pt->rot, avec3_010);
					VECSCALEIN(omg, YAWSPEED * dt);
					quatrotquat(pt->rot, omg, pt->rot);*/
				}
				if(inputs & PL_9){
					avec3_t omg;
					VECSCALE(omg, &mat[8], -M_PI * dt);
					VECADDIN(pt->omg, omg);
/*					quatrot(omg, pt->rot, avec3_010);
					VECSCALEIN(omg, -YAWSPEED * dt);
					quatrotquat(pt->rot, omg, pt->rot);*/
				}
			}
			if(p->bat && w->pl->control == pt){
				double f;
				avec3_t omg;
				avec3_t pyr;
/*				quat2pyr(w->pl->rot, pyr);*/
				if(onfeet){
					quatrot(omg, pt->rot, avec3_010);
					f = approach(0., -.0002 * M_PI * pt->inputs.analog[0] + 0., dt, 0.);
					VECSCALEIN(omg, f);
					quatrotquat(pt->rot, omg, pt->rot);
					p->torsopitch = rangein(approach(p->torsopitch, p->torsopitch + .0002 * M_PI * pt->inputs.analog[1], dt, 0.), -M_PI * .25, M_PI * .4);
	/*				pyr[0] -= p->torsopitch;*/
				}
				else{
					avec3_t omg;
					VECSCALE(omg, &mat[4], -.01 * M_PI * pt->inputs.analog[0] * dt);
					VECADDIN(pt->omg, omg);
					VECSCALE(omg, &mat[0], +.01 * M_PI * pt->inputs.analog[1] * dt);
					VECADDIN(pt->omg, omg);
				}
/*				pyr[1] = 0.;
				pyr[0] *= -1;
				pyr2quat(w->pl->rot, pyr);*/
			}

			/* friction */
			{
				avec3_t dest = {0.};
				double walkspeed = p->bat == valkie_batloid ? VALKIE_WALK_SPEED : p->bat == valkie_crouch ? VALKIE_WALK_SPEED * .5 : VALKIE_WALK_SPEED * .25;
				double phasespeed = p->bat == valkie_batloid ? VALKIE_WALK_PHASE_SPEED : VALKIE_WALK_PHASE_SPEED * 1.5;

				if(ii1 || ii2 || onfeet && inputs & (PL_W | PL_S | PL_A | PL_D))
					VECSCALE(dest, pt->velo, -1);

				if(onfeet){
					if(inputs & PL_W){
						avec3_t fore;
						quatrot(fore, pt->rot, avec3_001);
						f = -VECSP(fore, ci.normal);
						VECSADD(fore, ci.normal, f);
						VECSADD(dest, fore, -walkspeed);
						if(!walking/*!p->gear*/){
							walking = 1;
							p->walkphase = p->walkphase + phasespeed * dt;
						}
					}
					if(inputs & PL_S){
						avec3_t fore;
						quatrot(fore, pt->rot, avec3_001);
						f = -VECSP(fore, ci.normal);
						VECSADD(fore, ci.normal, f);
						VECSADD(dest, fore, walkspeed);
						if(!walking/*!p->gear*/){
							walking = 1;
							p->walkphase = p->walkphase - phasespeed * dt;
						}
					}
					if(inputs & PL_A){
						avec3_t fore;
						quatrot(fore, pt->rot, avec3_100);
						f = -VECSP(fore, ci.normal);
						VECSADD(fore, ci.normal, f);
						VECSADD(dest, fore, -walkspeed);
						if(!walking/*!p->gear*/){
							walking = 1;
							p->walkphase = p->walkphase + phasespeed * dt;
							if(p->bat != valkie_batloid)
								direction = 1;
						}
					}
					if(inputs & PL_D){
						avec3_t fore;
						quatrot(fore, pt->rot, avec3_100);
						f = -VECSP(fore, ci.normal);
						VECSADD(fore, ci.normal, f);
						VECSADD(dest, fore, walkspeed);
						if(!walking/*!p->gear*/){
							walking = 1;
							p->walkphase = p->walkphase + phasespeed * dt;
							if(p->bat != valkie_batloid)
								direction = 1;
						}
					}
					if(inputs & pt->inputs.change & PL_Z && p->bat < valkie_prone){
						p->walktime = 0.;
						p->bat++;
					}
					if(inputs & pt->inputs.change & PL_Q && valkie_batloid < p->bat){
						p->walktime = 0.;
						p->bat--;
					}
					p->twist = approach(p->twist,
						p->bat != valkie_batloid ? 0. :
						inputs & PL_W ?
						inputs & PL_A ? -M_PI / 4. : inputs & PL_D ?  M_PI / 4. : 0. :
						inputs & PL_S ?
						inputs & PL_A ?  M_PI / 4. : inputs & PL_D ? -M_PI / 4. : 0. :
						inputs & PL_A ? -M_PI / 2. : inputs & PL_D ?  M_PI / 2. : 0.,
							M_PI * dt, 2. * M_PI);
				}

				f = (1. - exp(-5. * (dt)));
				if(f < 1.)
					VECSADD(pt->velo, dest, f);
				else
					VECADDIN(pt->velo, dest);
			}

			if(p->bat){
			/* auto-standup */
				double sp;
				avec3_t xh, yh, zh, omega;
				aquat_t qc;
/*				quatrot(xh, pt->rot, avec3_100);
				quatrot(zh, pt->rot, avec3_001);*/
				quatrot(yh, pt->rot, avec3_010);
/*				sp = -VECSP(ci.normal, xh);*/
				if(onfeet){
					VECVP(omega, ci.normal, yh);
					VECSCALEIN(omega, -dt);
	/*				VECSCALE(omega, zh, sp * dt);*/
					quatrotquat(pt->rot, omega, pt->rot);
				}
				else if(accel[0] || accel[1] || accel[2]){
					avec3_t naccel;
					VECNORM(naccel, accel);
					VECVP(omega, naccel, yh);
					VECSCALEIN(omega, dt);
					VECADDIN(pt->omg, omega);
				}
			}
			if(p->bat && !p->afterburner && onfeet){
				pt->inputs.press &= PL_B;
				pt->inputs.change &= PL_B;
				pt->inputs.press |= PL_S;
			}
		}
		if(!walking){
			p->walkphase = approach(p->walkphase - floor(p->walkphase / (M_PI)) * (M_PI), 0.,VALKIE_WALK_PHASE_SPEED * dt, M_PI * 1.);
		}
		if(!onfeet){
			p->twist = approach(p->twist, 0., M_PI * dt, 2. * M_PI);
			p->torsopitch = approach(p->torsopitch, 0., dt, 2 * M_PI);
		}
	}
#endif

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
#if 0
		p->aileron[0] = rangein(approach(p->aileron[0], rangein(- normal * .3, -M_PI / 3., M_PI / 3.), M_PI * dt, 2 * M_PI), -M_PI / 3., M_PI / 3.);
		p->aileron[1] = rangein(approach(p->aileron[1], rangein(+ normal * .3, -M_PI / 3., M_PI / 3.), M_PI * dt, 2 * M_PI), -M_PI / 3., M_PI / 3.);
		p->elevator = rangein(approach(p->elevator, rangein(-common * .3, -M_PI / 3., M_PI / 3.), M_PI * dt, 2 * M_PI), -M_PI / 3., M_PI / 3.);
		if(vft != &valkie_s && (w->pl->control != pt || inputs & (PL_A | PL_D | PL_R)))
			p->rudder = rangein(approach(p->rudder + M_PI, (inputs & PL_A ? -M_PI / 6. : inputs & PL_D ? M_PI / 6. : 0) + M_PI, .1 * M_PI * dt, 2 * M_PI) - M_PI, -M_PI / 6., M_PI / 6.);
#endif
		if(controller)
			rudder = rangein(approach(rudder, (inputs & PL_A ? -M_PI / 6. : inputs & PL_D ? M_PI / 6. : 0), 1. * M_PI * dt, 0.), -M_PI / 6., M_PI / 6.);

		fspoiler = approach(fspoiler, spoiler, dt, 0);
		if(bbody)
			bbody->setFriction(0.1 + 0.9 * fspoiler);

		gearphase = approach(gearphase, gear, 1. * dt, 0.);

		if(inputs & PL_W)
			throttle = approach(throttle, 1., .5 * dt, 5.);
		if(inputs & PL_S){
			throttle = approach(throttle, 0., .5 * dt, 5.);
			if(afterburner && throttle < .5)
				afterburner = 0;
		}

#if 0
		if(vft == &valkie_s){
			int i;
			struct valkie *p = (struct valkie*)pt;
			if(p->bat){
				for(i = 0; i < 2; i++){
					p->vector[i] = approach(p->vector[i], 0 * M_PI * .25, 1.2 * dt, M_PI * 2.);
					if(p->bat == valkie_crouch || p->bat == valkie_prone)
						p->legr[i] = approach(p->legr[i], direction ? (i * 2 - 1) * sin(p->walkphase) * M_PI * .2 : 0., M_PI * .25 * dt, M_PI * 2.);
					else
						p->legr[i] = approach(p->legr[i], (i * 2 - 1) * M_PI / 32., M_PI * .25 * dt, M_PI * 2.);
					if(p->bat == valkie_crouch){
						p->leg[i] = approach(p->leg[i], +M_PI / 48. - i * M_PI * (.25 + 1. / 24.) - (direction ? 1. : (i * 2 - 1) * sin(-p->walkphase) + 1.) * M_PI / 24., VALKIE_WALK_PHASE_SPEED * dt, M_PI * 2.);
						p->downleg[i] = approach(p->downleg[i], M_PI / 4. - -(direction ? 1. : cos(-p->walkphase + i * M_PI) + 1.) * M_PI / 24., VALKIE_WALK_PHASE_SPEED * dt, M_PI * 2.);
						p->foot[i] = approach(p->foot[i], - (1 - i) * M_PI * (.25 + 1. / 24.) - ((i * 2 - 1) * sin(-p->walkphase) + 1.) * M_PI / 24., VALKIE_WALK_PHASE_SPEED * dt, M_PI * 2.);
					}
					else if(p->bat == valkie_prone){
						p->leg[i] = approach(p->leg[i], +M_PI / 48. - M_PI * .25, VALKIE_WALK_PHASE_SPEED * dt, M_PI * 2.);
						p->downleg[i] = approach(p->downleg[i], M_PI * 3. / 4., VALKIE_WALK_PHASE_SPEED * dt, M_PI * 2.);
						p->foot[i] = approach(p->foot[i], -M_PI * 1. / 2., VALKIE_WALK_PHASE_SPEED * dt, M_PI * 2.);
					}
					else if(!onfeet){
						p->leg[i] = approach(p->leg[i], 0 * M_PI * .25, VALKIE_WALK_PHASE_SPEED * dt, M_PI * 2.);
						p->downleg[i] = approach(p->downleg[i], M_PI / 16., VALKIE_WALK_PHASE_SPEED * dt, M_PI * 2.);
						p->foot[i] = approach(p->foot[i], -M_PI / 24., VALKIE_WALK_PHASE_SPEED * dt, M_PI * 2.);
					}
					else if(!walking){
						p->leg[i] = approach(p->leg[i], 0 * -M_PI / 24., VALKIE_WALK_PHASE_SPEED * dt, M_PI * 2.);
						p->downleg[i] = approach(p->downleg[i], - -M_PI / 12., VALKIE_WALK_PHASE_SPEED * dt, M_PI * 2.);
						p->foot[i] = approach(p->foot[i], -M_PI / 24., VALKIE_WALK_PHASE_SPEED * dt, M_PI * 2.);
					}
					else{
						p->leg[i] = approach(p->leg[i], -((i * 2 - 1) * sin(-p->walkphase) + 1.) * M_PI / 12., VALKIE_WALK_PHASE_SPEED * dt, M_PI * 2.);
						p->downleg[i] = approach(p->downleg[i], - -(cos(-p->walkphase + i * M_PI) + 1.) * M_PI / 6., VALKIE_WALK_PHASE_SPEED * dt, M_PI * 2.);
						p->foot[i] = approach(p->foot[i], ((i * 2 - 1) * sin(p->walkphase)) * M_PI / 24., VALKIE_WALK_PHASE_SPEED * dt, M_PI * 2.);
					}
					p->arm[i] = approach(p->arm[i], -(i * 2 - 1) * M_PI / 2., M_PI * dt, 5.);
					p->armr[i] = approach(p->armr[i], -(i * 2 - 1) * M_PI / 4., M_PI * dt, 5.);
					p->downarm[i] = approach(p->downarm[i], -(i * 2 - 1) * M_PI / 6., 1.2 * dt, 5.);
					p->wing[i] = approach(p->wing[i], (i * 2 - 1) * M_PI / 4., M_PI / 2. * dt, 5.);
				}
				p->batphase = approach(p->batphase, 1., 2. * dt, 5.);
			}
			else{
				for(i = 0; i < 2; i++){
					if(inputs & PL_Z){
						p->vector[i] = approach(p->vector[i], 1., 1.2 * dt, M_PI * 2.);
					}
					if(inputs & PL_Q){
						p->vector[i] = approach(p->vector[i], 0., 1.2 * dt, M_PI * 2.);
					}
					p->leg[i] = approach(p->leg[i], -p->vector[i] * M_PI / 3., M_PI * .5 * dt, M_PI * 2.);
					p->downleg[i] = approach(p->downleg[i], -p->vector[i] * M_PI / 6., M_PI * .25 * dt, M_PI * 2.);
					p->foot[i] = approach(p->foot[i], 0., VALKIE_WALK_PHASE_SPEED * dt, M_PI * 2.);
					p->legr[i] = approach(p->legr[i], (i * 2 - 1) * M_PI / 12., 1.2 * dt, M_PI * 2.);
					p->arm[i] = approach(p->arm[i], 0., M_PI * dt, 5.);
					p->armr[i] = approach(p->armr[i], 0, M_PI * dt, 5.);
					p->downarm[i] = approach(p->downarm[i], 0., 1.2 * dt, 5.);
					p->wing[i] = approach(p->wing[i], 0., M_PI / 2. * dt, 5.);
				}
				p->batphase = approach(p->batphase, 0., 2. * dt, 5.);
			}
			p->crouchphase = approach(p->crouchphase, p->bat == valkie_crouch, 2. * dt, 5.);
			if(pt->inputs.change & pt->inputs.press & PL_B){
				p->bat = !p->bat;
			}
		}
#endif

		if(this->inputs.change & this->inputs.press & PL_RCLICK)
			toggleWeapon();
/*		if(pt->inputs.change & pt->inputs.press & PL_B)
			p->brk = !p->brk;*/
/*		if(pt->inputs.change & pt->inputs.press & PL_TAB){
			p->afterburner = !p->afterburner;
			if(p->afterburner && p->throttle < .7)
				p->throttle = .7;
		}*/
		Vec3d nh = this->rot.quatrotquat(Vec3d(0., 0., -1.));
		for(auto pt2 : w->entlist()){
			if(pt2 != this && pt2->race != -1 /*&& ((struct entity_private_static*)pt2->vft)->flying(pt2)*/){
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
#if 1
/*		if(vft == &valkie_s){
			double f;
			aquat_t qrot = {0};
			struct valkie *p = (struct valkie*)pt;
			if(p->bat)
				f = (-p->wing[0]);
			else
				f = p->vector[0] * 3. * M_PI / 8.;
			qrot[0] = sin(f);
			qrot[3] = cos(f);
			quatrot(zh0, qrot, zh0);
			qrot[0] = 0.;
			qrot[1] = 0.;
			qrot[2] = sin(-p->rudder * .5);
			qrot[3] = cos(-p->rudder * .5);
			quatrot(zh0, qrot, zh0);
		}*/
//		MAT4DVP3(zh, mat, zh0);

		Vec3d zh = mat.dvp3(zh0);
//		if(pt->vft == &valkie_s)
	//		thrustsum = -thrust_strength * pt->mass * p->throttle * (air + 1.5 * p->afterburner) * dt;
//		else
		double thrustsum = -getThrustStrength() * this->mass * this->throttle * air * (1. + this->afterburner);
//		VECSCALEIN(zh, thrustsum);
		zh *= thrustsum;
		if(bbody)
			bbody->applyCentralForce(btvc(zh));
//		RigidAddMomentum(pt, avec3_000, zh);
/*		VECSADD(pt->velo, zh, -.012 * p->throttle * dt);*/
#endif
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
				rot *= Quatd::rotation(rudder * it.sensitivity, it.axis);

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

#if 0
	if(vft == &valkie_s && !((valkie_t*)pt)->bat && !onfeet){
		avec3_t omg, omg0;
		VECNULL(omg0);
		omg0[2] = -p->aileron[0] * dt;
		mat4dvp3(omg, mat, omg0);
		quatrotquat(pt->rot, omg, pt->rot);
		VECNULL(omg0);
		omg0[0] = p->elevator * dt;
		mat4dvp3(omg, mat, omg0);
		quatrotquat(pt->rot, omg, pt->rot);
	}
#endif

#if 0
	{
		avec3_t *points = pt->vft == &valkie_s ? valkie_points : fly_points;
		double friction[numof(fly_points)] = {
			.1,
			.1,
			.1,
			1.,
			1.,
			1.,
			1.,
		};
		unsigned long contact;
		if(p->brk)
			friction[0] = friction[1] = friction[2] = .5;
		contact = RigidPointHit(pt, w, dt, points, NULL, numof(fly_points), friction, NULL, NULL);
		if(pt->health <= 0. && contact){
			pt->active = 0;
			return;
		}
		if(pt->health <= 0. && contact){
			pt->active = 0;
			return;
		}
		if(contact & ((1 << 3) | (1 << 4) | (1 << 5) | (1 << 6)) || inwater){
			double amount;
			amount = VECSLEN(pt->velo) * 1e2 * 1e2 + 10.;
			if(0 < amount && !vft->takedamage(pt, amount * dt, w, 0)){
				pt->active = 0;
				return;
			}
		}
	}
#endif

	this->pos += this->velo * dt;
#if 0
	{
		amat4_t nmat;
		amat3_t irot3, rot3, iort3, nmat3, inmat3;
		aquat_t q;
		rbd_anim(pt, dt, &q, &nmat);
		QUATCPY(pt->rot, q);
		MAT4TO3(nmat3, nmat);
		MATTRANSPOSE(inmat3, nmat3);
		MATMP(rot3, inmat3, ort3);
		MAT3TO4(nmat, rot3);
/*		MAT4TO3(nmat3, nmat);
		MATTRANSPOSE(iort3, ort3);
		MATMP(rot3, iort3, nmat3);
		MATTRANSPOSE(irot3, rot3);
		MAT3TO4(nmat, irot3);*/
		imat2pyr(nmat, pt->pyr);
/*		quat2pyr(pt->rot, pt->pyr);*/
/*		{
			avec3_t zh, zh0 = {0, -1., 0}, src, ret;
			double wid;
			MAT4DVP3(zh, nmat, zh0);
			wid = w->map->vft->width(w->map);
			src[0] = pt->pos[0] + wid / 2;
			src[1] = pt->pos[1];
			src[2] = pt->pos[2] + wid / 2;
			w->map->vft->linehit(w->map, &src, &zh, 1., &ret);
			p->sight[0] = ret[0] - wid / 2;
			p->sight[1] = ret[1];
			p->sight[2] = ret[2] - wid / 2;
		}*/

	}
	{
		double rate = inwater ? 1. : 0., factor = 1.;
		if(vft == &valkie_s){
			struct valkie *p = (struct valkie*)pt;
			if(p->bat)
				factor = fabs(p->wing[0]) * 3. / (M_PI / 4.) + 1.;
		}

		/* stabilizer is active only when functional */
		if(pt->health <= 0.)
			factor *= air;
		VECSCALEIN(pt->omg, 1. / (factor * (rate + .4) * dt + 1.));
		if(0. < pt->health)
			factor *= air;
		VECSCALEIN(pt->velo, 1. / (factor * (rate + .01) * dt + 1.));
	}
#endif


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
	if(!scstrcmp(name, _SC("gear"))){
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
	else if(!scstrcmp(name, _SC("takingOff"))){
		sq_pushbool(v, SQBool(takingOff));
		return 1;
	}
	else
		return st::sqGet(v, name);
}

SQInteger Aerial::sqSet(HSQUIRRELVM v, const SQChar *name){
	if(!scstrcmp(name, _SC("gear"))){
		SQBool b;
		if(SQ_FAILED(sq_getbool(v, 3, &b)))
			return sq_throwerror(v, _SC("Argument type must be compatible with bool"));
		gear = b;
		return 0;
	}
	if(!scstrcmp(name, _SC("spoiler"))){
		SQBool b;
		if(SQ_FAILED(sq_getbool(v, 3, &b)))
			return sq_throwerror(v, _SC("Argument type must be compatible with bool"));
		spoiler = b;
		return 0;
	}
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
	else if(!scstrcmp(name, _SC("destArrived"))){
		SQBool b;
		if(SQ_FAILED(sq_getbool(v, 3, &b)))
			return sq_throwerror(v, _SC("could not convert to bool for destArrived"));
		destArrived = b != SQFalse;
		return 1;
	}
	else if(!scstrcmp(name, _SC("takingOff"))){
		SQBool b;
		if(SQ_FAILED(sq_getbool(v, 3, &b)))
			return sq_throwerror(v, _SC("could not convert to bool for takingOff"));
		takingOff = b != SQFalse;
		return 1;
	}
	else
		return st::sqSet(v, name);
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
	return ret;
}

static double rollSense = 2.;
static double omgSense = 2.;
static double intSense = -1.;
static double intMax = 1.;
static double turnClimb = 0.1;

static void initSense(){
	CvarAdd("rollSense", &rollSense, cvar_double);
	CvarAdd("omgSense", &omgSense, cvar_double);
	CvarAdd("intSense", &intSense, cvar_double);
	CvarAdd("intMax", &intMax, cvar_double);
	CvarAdd("omgDamp", &angularDamping, cvar_double);
	CvarAdd("turnClimb", &turnClimb, cvar_double);
}

static StaticInitializer initializer(initSense);

/// \brief Taxiing AI
void Aerial::animAI(double dt, bool onfeet){
	Mat4d mat;
	transform(mat);

	onfeet = onfeet || velo.slen() < 0.05 * 0.05 && 0.9 < mat.vec3(1)[1];

	// Automatic stabilizer (Auto Pilot)
	Vec3d x_along_y = Vec3d(mat.vec3(0)[0], 0, mat.vec3(0)[2]).normin();
	double croll = -x_along_y.sp(mat.vec3(1)); // Current Roll
	double rollOmg = mat.vec3(2).sp(omg);
	double roll = croll * rollSense + rollOmg * omgSense + iaileron * intSense; // P + D + I
	iaileron = rangein(iaileron + roll * dt, -intMax, intMax);
	Vec3d deltaPos((!onfeet && enemy ? enemy->pos : destPos) - this->pos); // Delta position towards the destination
	if(!(!onfeet && enemy))
		deltaPos[1] = 0.; // Ignore height component
	double sdist = deltaPos.slen();
	if(0.5 * 0.5 < sdist){
		double sp = deltaPos.sp(mat.vec3(2));
		if(3. * 3. < sdist && 0.75 < sp)
			roll += 0.25;
		else if(3. * 3. < sdist || sp < 0){
			deltaPos.normin();
			roll += 0.25 * deltaPos.vp(-mat.vec3(2))[1];
		}
	}

	static Id monitor = -1;
	if(monitor < 0)
		monitor = getid();
	GLWchart::addSampleToCharts("croll", croll);
	GLWchart::addSampleToCharts("omg", rollOmg);
	GLWchart::addSampleToCharts("iaileron", iaileron);

	// If the body is stationary and upright, assume it's on the ground.
	if(onfeet){
		aileron = approach(aileron, 0, dt, 0);
		Vec3d localDeltaPos = this->rot.itrans(deltaPos);
		double phi = atan2(localDeltaPos[0], -localDeltaPos[2]);
		const double arriveDist = 0.03;
		if(takingOff){
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
			spoiler = false;
		}
		else if(destArrived || sdist < arriveDist * arriveDist){
			destArrived = true;
			throttle = approach(throttle, 0, dt, 0);
			rudder = approach(rudder, 0, dt, 0);
			spoiler = true;
		}
		else{
			double velolen = velo.len();
			throttle = approach(throttle, std::max(0., 0.2 - velolen * 20. + (fabs(phi) < 0.05 * M_PI ? 0.1 : 0.)), dt, 0);
			rudder = rangein(approach(rudder, phi, 1. * M_PI * dt, 0.), -M_PI / 6., M_PI / 6.);
			spoiler = false;
		}
		gear = true;
	}
	else{
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

		aileron = rangein(aileron + roll * dt, -1, 1);
		throttle = approach(throttle, rangein((0.5 - velo.len()) * 2., 0, 1), dt, 0);
		rudder = approach(rudder, 0, dt, 0);
		double trim = mat.vec3(2)[1] + velo[1] + rangein(deltaPos[1], -0.5, 0.5) + fabs(croll) * turnClimb; // P + D
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

static void find_wingtips(const char *name, struct afterburner_hint *hint){
	int i = 0;
	if(!strcmp(name, "LWING_BTL") || !strcmp(name, "RWING_BTL") && (i = 1) || !strcmp(name, "Nose") && (i = 2)){
		amat4_t mat;
		avec3_t pos0[2] = {{(8.12820 - 3.4000) * .75, 0, (6.31480 - 3.) * -.5}, {0, 0, 6.}};
		glGetDoublev(GL_MODELVIEW_MATRIX, mat);
		if(!i)
			pos0[0][0] *= -1;
		mat4vp3(hint->wingtips[i], mat, pos0[i/2]);
	}
}



/*
static int fly_flying(const entity_t *pt){
	return pt->active && .05 * .05 < VECSLEN(pt->velo) && .001 < pt->pos[1];
}
*/

#if 0
static int fly_getrot(struct entity *pt, warf_t *w, double (*rot)[16]){
#if FLY_QUAT
	amat4_t ret;
	if(w->pl->chasecamera == 8){
		amat4_t rot2;
/*		avec3_t dv;
		aquat_t q;
		double c, s;
		dv[0] = -.02 * (c = cos(w->war_time * 2. * M_PI / 15.));
		dv[1] = .05;
		dv[2] = -.02 * (s = sin(w->war_time * 2. * M_PI / 15.));
		VECNORM(q, dv);
		VECSCALEIN(q, -c);
		q[3] = s;
		quat2mat(*rot, q);*/
		mat4rotx(rot2, mat4identity, M_PI / 8.);
		mat4roty(ret, rot2, w->war_time * 2. * M_PI / 15. - M_PI / 2.);
	}
	else if(w->pl->chasecamera == 9){
		avec3_t pos, dr, dr0;
		amat3_t ort, iort, rot2, rot3;
		int camera = w->pl->chasecamera;
		double phi, theta;
		fly_cockpitview(pt, w, &pos, &camera);
		VECSUB(dr0, pos, pt->pos);
		w->vft->orientation(w, &ort, &pos);
/*		MATIDENTITY(ort);*/
		MATTRANSPOSE(iort, ort);
		MATVP(dr, iort, dr0);
		phi = -atan2(dr[0], dr[2]);
		theta = atan2(dr[1], sqrt(dr[0] * dr[0] + dr[2] * dr[2]));
		MATROTX(rot2, mat3identity, theta);
		MATROTY(rot3, rot2, phi);
		MATMP(rot2, rot3, iort);
		MAT3TO4(ret, rot2);
/*		gldLookatMatrix(rot, &dr);*/
/*		glPushMatrix();
		directrot(dr, avec3_000, *rot);
		glPopMatrix();*/
	}
	else if(w->pl->chasecamera == 3){
		amat4_t mat2, mat;
		mat4rotx(mat2, mat4identity, M_PI / 6.);
		mat4roty(mat, mat2, 5 * M_PI / 4.);
		quat2imat(mat2, pt->rot);
		mat4mp(ret, mat, mat2);
	}
	else if(w->pl->chasecamera == 4 && pt->vft == &valkie_s){
		valkie_t *p = (valkie_t*)pt;
		amat4_t mat2, mat;
		mat4rotx(mat2, mat4identity, M_PI / 6.);
		mat4roty(mat, mat2, 5 * M_PI / 4. - p->batphase * M_PI);
		quat2imat(mat2, pt->rot);
		mat4mp(ret, mat, mat2);
	}
	else if(w->pl->chasecamera == 7){
		quat2imat(*rot, pt->rot);
		return w->pl->control != pt;
	}
	else
		quat2imat(ret, pt->rot);
	if(pt->vft == &valkie_s && w->pl->chasecamera < 3){
		amat4_t matp;
		valkie_t *p = (valkie_t*)pt;
		mat4rotx(matp, mat4identity, p->torsopitch);
		mat4mp(*rot, matp, ret);
	}
	else
		MAT4CPY(*rot, ret);
	return w->pl->control != pt;
#else
	if(w->pl->control == pt){
		VECSUB(*pyr, pt->pyr, w->pl->pyr);
	}
	else
		VECCPY(*pyr, pt->pyr);
#endif
}
#endif


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

/*
static int flare_flying(const entity_t *pt){
	if(.01 < pt->pos[1])
		return 1;
	else return 0;
}
*/
/*
int cmd_togglevalkie(int argc, const char *argv[]){
	ArmsShowWindow(ValkieNew, 500e3, offsetof(valkie_t, arms), valkie_arms, valkie_hardpoints, numof(valkie_hardpoints));
	return 0;
}
*/
