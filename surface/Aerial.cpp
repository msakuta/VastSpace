/** \file
 * \brief Definition of base classes for aerial Entities such as aeroplanes.
 */

#include "Aerial.h"
#include "Bullet.h"
#include "tent3d.h"
#include "yssurf.h"
#include "arms.h"
#include "sqadapt.h"
#include "Game.h"
#include "tefpol3d.h"
#include "motion.h"
#include "draw/WarDraw.h"
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




#define FLY_QUAT 1
#define FLY_MAX_GIBS 30
#define FLY_MISSILE_FREQ .1
#define FLY_SMOKE_FREQ 10.
#define FLY_RETHINKTIME .5
#define FLARE_INTERVAL 2.5
#define FLY_SCALE (1./20000)
#define FLY_YAWSPEED (.1 * M_PI)
#define FLY_PITCHSPEED  (.05 * M_PI)
#define FLY_ROLLSPEED (.1 * M_PI)
#define FLY_RELOADTIME .07
#define BULLETSPEED .78
#define numof(a) (sizeof(a)/sizeof*(a))
#define SONICSPEED .340
#define putstring(a) gldPutString(a)
#define VALKIE_WALK_SPEED (.015)
#define VALKIE_WALK_PHASE_SPEED (M_PI)
#define YAWSPEED (M_PI*.2)


/* color sequences */
#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_blueburn[] = {
	{0.1, COLOR32RGBA(0,203,255,15)},
	{0.15, COLOR32RGBA(63,255,255,13)},
	{0.45, COLOR32RGBA(255,127,31,7)},
	{2.3, COLOR32RGBA(255,31,0,0)},
};
static const struct color_sequence cs_blueburn = DEFINE_COLSEQ(cnl_blueburn, (COLOR32)-1, 3.);
static const struct color_node cnl0_vapor[] = {
	{0.4, COLOR32RGBA(255,255,255,0)},
	{0.4, COLOR32RGBA(255,255,255,191)},
	{0.4, COLOR32RGBA(191,191,191,127)},
	{.4, COLOR32RGBA(127,127,127,0)},
};
static struct color_node cnl_vapor[] = {
	{0.4, COLOR32RGBA(255,255,255,0)},
	{0.4, COLOR32RGBA(255,255,255,191)},
	{0.4, COLOR32RGBA(191,191,191,127)},
	{.4, COLOR32RGBA(127,127,127,0)},
};
static const struct color_sequence cs_vapor = DEFINE_COLSEQ(cnl_vapor, (COLOR32)-1, 1.6);
static const struct color_node cnl_bluework[] = {
	{0.03, COLOR32RGBA(255, 255, 255, 255)},
	{0.035, COLOR32RGBA(128, 128, 255, 255)},
	{0.035, COLOR32RGBA(32, 32, 128, 255)},
};
static const struct color_sequence cs_bluework = DEFINE_COLSEQ(cnl_bluework, (COLOR32)-1, 0.1);


static const struct color_node cnl_firetrail[] = {
	{0.05, COLOR32RGBA(255, 255, 212, 0)},
	{0.05, COLOR32RGBA(255, 191, 191, 255)},
	{0.4, COLOR32RGBA(111, 111, 111, 255)},
	{0.3, COLOR32RGBA(63, 63, 63, 127)},
};
const struct color_sequence cs_firetrail = DEFINE_COLSEQ(cnl_firetrail, (COLOR32)-1, .8);


static const Vec3d flywingtips[2] = {
	Vec3d(-160. * FLY_SCALE, 20. * FLY_SCALE, 10. * FLY_SCALE),
	Vec3d(160. * FLY_SCALE, 20. * FLY_SCALE, 10. * FLY_SCALE),
}/*,
valkiewingtips[2] = {
	{-440. * .5 * FLY_SCALE, 50. * .5 * FLY_SCALE, 210. * .5 * FLY_SCALE},
	{440. * .5 * FLY_SCALE, 50. * .5 * FLY_SCALE, 210. * .5 * FLY_SCALE},
}*/;

void smoke_draw(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv);



static const double gunangle = 0.;
static double thrust_strength = .010;
static avec3_t wings0[5] = {
	{0.003, 20 * FLY_SCALE, -.0/*02*/},
	{-0.003, 20 * FLY_SCALE, -.0/*02*/},
	{.002, 20 * FLY_SCALE, .005},
	{-.002, 20 * FLY_SCALE, .005},
	{.0, 50 * FLY_SCALE, .005}
};
static amat3_t aerotensor0[3] = {
#if 1 /* experiments say that this configuration is responding well. */
	{
		-.1, 0, 0,
		0, -6.5, 0,
		0, -.3, -.025,
	}, {
		-.1, 0, 0,
		0, -1.9, 0,
		0, -0.0, -.015,
	}, {
		-.9, 0, 0,
		0, -.05, 0,
		0, 0., -.015,
	}
#else
	{
		-.2, 0, 0,
		0, -.7, 0,
		0, -1, -.1,
	}, {
		-.2, 0, 0,
		0, -.5, 0,
		0, 0., -.1,
	}, {
		-.5, 0, 0,
		0, -.1, 0,
		0, 0., 0,
	}
#endif
};

#if 0
static void valkie_drawHUD(entity_t *pt, warf_t *, wardraw_t *, const double irot[16], void (*)(void));
static void valkie_cockpitview(entity_t *pt, warf_t*, double (*pos)[3], int *);
static void valkie_control(entity_t *pt, warf_t *w, input_t *inputs, double dt);
static void valkie_destruct(entity_t *pt);
static void valkie_anim(entity_t *pt, warf_t *w, double dt);
static void valkie_draw(entity_t *pt, wardraw_t *wd);
static void valkie_drawtra(entity_t *pt, wardraw_t *wd);
static int valkie_takedamage(entity_t *pt, double damage, warf_t *w);
static void valkie_gib_draw(const struct tent3d_line_callback *pl, void *pv);
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




void Aerial::loadWingFile(){
	static bool wingfile = false;
	if(!wingfile){
		FILE *fp;
		int i;
		wingfile = true;
		fp = fopen("fly.cfg", "r");
		if(fp){
			fscanf(fp, "thrust = %lg\n", &thrust_strength);
			for(i = 0; i < 5; i++){
				char buf[512], *p;
				int j;
				fgets(buf, sizeof buf, fp);
				if(p = strchr(buf, '\n'))
					*p = '\0';
				for(j = 0; j < 3; j++){
					p = strtok(j ? NULL : buf, ",");
					if(p){
//						wings0[i][j] = calc3(&p, NULL, NULL);
						HSQUIRRELVM v = game->sqvm;
						StackReserver st2(v);
						cpplib::dstring dst = cpplib::dstring("return(") << p << ")";
						if(SQ_SUCCEEDED(sq_compilebuffer(v, dst, dst.len(), _SC("aerotensor"), SQTrue))){
							sq_push(v, -2);
							if(SQ_SUCCEEDED(sq_call(v, 1, SQTrue, SQTrue))){
								sqa::SQQuatd q;
								q.getValue(v, -1);
								rot = q.value;
							}
						}
					}
					else break;
				}
/*				fscanf(fp, "%lg %lg %lg\n", &wings0[i][0], &wings0[i][1], &wings0[i][2]);*/
			}
			for(i = 0; i < 3; i++){
				fscanf(fp, "%lg %lg %lg\n""%lg %lg %lg\n""%lg %lg %lg\n",
					&aerotensor0[i][0], &aerotensor0[i][1], &aerotensor0[i][2],
					&aerotensor0[i][3], &aerotensor0[i][4], &aerotensor0[i][5],
					&aerotensor0[i][6], &aerotensor0[i][7], &aerotensor0[i][8]);
			}
/*			printf("fly.cfg loaded.\n");*/
			fclose(fp);
		}
	}
}

void Aerial::init(){
	int i;
	this->weapon = 0;
	this->aileron[0] = this->aileron[1] = 0.;
	this->elevator = 0.;
	this->rudder = 0.;
	this->gearphase = 0.;
	this->cooldown = 0.;
	this->muzzle = 0;
	this->brk = 1;
	this->afterburner = 0;
	this->navlight = 0;
	this->gear = 0;
	this->missiles = 10;
	this->throttle = 0.;
/*	for(i = 0; i < numof(fly->bholes)-1; i++)
		fly->bholes[i].next = &fly->bholes[i+1];
	fly->bholes[numof(fly->bholes)-1].next = NULL;
	fly->frei = fly->bholes;
	for(i = 0; i < fly->sd->np; i++)
		fly->sd->p[i] = NULL;
	fly->st.inputs.press = fly->st.inputs.change = 0;*/
}

Aerial::Aerial(WarField *w){
	loadWingFile();
/*	VECNULL(ret->pos);
	VECNULL(ret->velo);
	VECNULL(ret->pyr);
	VECNULL(ret->omg);
	QUATZERO(ret->rot);
	ret->rot[3] = 1.;*/
	mass = 12000.; /* kilograms */
	moi = .2; /* kilograms * kilometer^2 */
	health = 500.;
/*	ret->shoots = ret->shoots2 = ret->kills = ret->deaths = 0;
	ret->enemy = NULL;
	ret->active = 1;
	*(struct entity_private_static**)&ret->vft = &fly_s;
	ret->next = w->tl;
	w->tl = ret;
	ret->health = 500;*/
#ifndef DEDICATED
	TefpolList *tl = w->getTefpol3d();
	if(tl){
		pf = tl->addTefpolMovable(pos, velo, vec3_000, &cs_blueburn, TEP3_THICKER | TEP3_ROUGH, cs_blueburn.t);
		vapor[0] = tl->addTefpolMovable(pos, velo, vec3_000, &cs_vapor, TEP3_FAINT | TEP3_ROUGH, cs_vapor.t * 10);
		vapor[1] = tl->addTefpolMovable(pos, velo, vec3_000, &cs_vapor, TEP3_FAINT | TEP3_ROUGH, cs_vapor.t * 10);
	}
/*	sd = AllocSUFDecal(gsuf_fly);
	sd->drawproc = bullethole_draw;*/
#endif
	init();
/*	if(!w->pl->chase)
		w->pl->chase = ret;*/
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

void Aerial::cockpitView(Vec3d &pos, Quatd &rot, int *chasecam){
	avec4_t src[] = {
		/*{0., .008, 0.015, 1.}*/
		{0., 40 * FLY_SCALE, -120 * FLY_SCALE},
		{0., .0055, .015},
		{0.013, .007, .025},
		{.010, .007, -.010},
		{.010, .007, -.010},
		{0.004, .0, .0},
		{-0.004, .0, .0},
	};
	Mat4d mat;
	int camera;
	if(chasecam){
		camera = *chasecam;
		camera = MAX(0, MIN(numof(src)+2, camera));
		*chasecam = camera;
	}
	else
		camera = 0;
#if FLY_QUAT
	transform(mat);
	if(camera == numof(src)+1){
		pos = this->pos;
		pos[0] += .05 * cos(w->war_time() * 2. * M_PI / 15.);
		pos[1] += .02;
		pos[2] += .05 * sin(w->war_time() * 2. * M_PI / 15.);
	}
#if 0
	else if(camera == 4 && pt->vft == &valkie_s){
		valkie_t *p = (valkie_t*)pt;
		amat4_t mat2;
		mat4roty(mat2, mat, /*5 * M_PI / 4.*/ + p->batphase * M_PI);
		mat4vp3(*pos, mat2, src[camera]);
	}
#endif
	else if(camera == numof(src)){
		const Player *player = game->player;
		Vec3d ofs = mat.dvp3(vec3_001);
		if(camera)
			ofs *= player ? player->viewdist : 1.;
		pos = this->pos + ofs;
	}
	else if(camera == numof(src)+2){
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
		pos = mat.vp3(src[camera]);
	}
#else
#if 1
	pyrmat(pt->pyr, &mat);
#else
	glPushMatrix();
		glLoadIdentity();
/*		glRotated(pyr[1] / 2. / M_PI * 360, 0.0, -1.0, 0.0);
		glRotated(pyr[0] / 2. / M_PI * 360, -1.0, 0.0, 0.0);
		glRotated(pyr[2] / 2. / M_PI * 360, 0.0, 0.0, -1.0);*/
		glRotated(pt->pyr[1] / 2. / M_PI * 360, 0.0, -1.0, 0.0);
		glRotated(pt->pyr[0] / 2. / M_PI * 360, -1.0, 0.0, 0.0);
		glRotated(pt->pyr[2] / 2. / M_PI * 360, 0.0, 0.0, -1.0);
		glGetDoublev(GL_MODELVIEW_MATRIX, mat);
	glPopMatrix();
#endif
	MAT4VP(ofs, mat, src);
	VECADD(*pos, pt->pos, ofs);
#endif
}

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


const avec3_t fly_guns[2] = {{.001, .001, -.006}, {-.001, .001, -.006}};
const avec3_t fly_hardpoint[2] = {{.005, .0005, -.000}, {-.005, .0005, -.000}};

void Aerial::shootDualGun(double dt){
	Vec3d velo0(0., 0., -BULLETSPEED);
	double reloadtime = FLY_RELOADTIME;
	if(dt <= this->cooldown)
		return;
/*	if(pt->vft == &valkie_s){
		valkie_t *p = (valkie_t *)pt;
		mat4rotx(mat, mat0, -p->torsopitch);
		if(p->arms[0].type != arms_none){
			reloadtime = arms_static[p->arms[0].type].cooldown;
		}
		if(p->arms[0].type == arms_valkierifle){
			New = BeamNew;
		}
	}
	else*/
	Mat4d mat0;
	transform(mat0);
	Mat4d mat = mat0.rotx(-gunangle / deg_per_rad);
	while(this->cooldown < dt){
		int i = 0;
		do{
			double phi, theta;
/*			MAT4VP3(gunpos, mat, fly_guns[i]);*/
			Bullet *pb = new Bullet(this, 2., 5.);
			
			pb->mass = .005;
//			pb->life = 10.;
/*			phi = pt->pyr[1] + (drseq(&w->rs) - .5) * .005;
			theta = pt->pyr[0] + (drseq(&w->rs) - .5) * .005;
			VECCPY(pb->velo, pt->velo);
			pb->velo[0] +=  BULLETSPEED * sin(phi) * cos(theta);
			pb->velo[1] += -BULLETSPEED * sin(theta);
			pb->velo[2] += -BULLETSPEED * cos(phi) * cos(theta);
			pb->pos[0] = pt->pos[0] + gunpos[0];
			pb->pos[1] = pt->pos[1] + gunpos[1];
			pb->pos[2] = pt->pos[2] + gunpos[2];*/
#if 0
			if(pt->vft == &valkie_s && dnm){
				avec3_t org;
				glPushMatrix();
				glLoadIdentity();
				TransYSDNM_V(dnm, valkie_boneset((valkie_t*)pt, NULL, NULL, 0), find_gun, org);
				VECSCALEIN(org, 1e-3);
				mat4vp3(pb->pos, mat, org);
				glPopMatrix();
			}
			else
#endif
				pb->pos = mat.vp3(fly_guns[i]);
			pb->velo = mat.dvp3(velo0) + this->velo;
			for(int j = 0; j < 3; j++)
				pb->velo[j] += (drseq(&w->rs) - .5) * .005;
			pb->anim(dt - this->cooldown);
		} while(/*pt->vft != &valkie_s && */!i++);
		this->cooldown += reloadtime;
//		this->shoots += 1;
	}
//	shootsound(pt, w, p->cooldown);
	this->muzzle = 1;
}

void Aerial::control(input_t *inputs, double dt){
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
	if(cooldown < dt)
		cooldown = 0;
	else
		cooldown -= dt;
	if(inputs->press & (PL_LCLICK | PL_ENTER)){
		if(!weapon)
			shootDualGun(dt);
#if 0
		else if(0 < missiles && cooldown < dt){
			struct hellfire *pb;
			amat4_t rot;
			avec3_t v, pos;
		/*			VECCPY(pb->pos, pt->pos);*/
			quat2mat(rot, pt->rot);
/*			VECCPY(pb->st.velo, pt->velo);*/
/*			MAT4DVP3(v, rot, v0);
			VECADDIN(pb->st.velo, v);*/
			MAT4DVP3(v, rot, fly_hardpoint[p->missiles % 2]);
			VECADD(pos, pt->pos, v);
/*			pb = add_sidewinder(w, pos, pt->enemy, pt, pt->velo, pt->rot, 150.);*/
			pb = add_aim9(w, pos);
			pb->st.owner = pt;
			QUATCPY(pb->st.rot, pt->rot);
			VECCPY(pb->st.velo, pt->velo);
			pb->target = pt->enemy;
			p->missiles--;
			pt->shoots++;
		//	playWAVEFile("missile.wav");
			playWave3D("missile.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime + p->cooldown);
			p->cooldown += 2.;
		}
#endif
	}
	if(inputs->press & inputs->change & (PL_G))
		gear = !gear;
	if(/*pt->vft != &valkie_s || !((valkie_t*)pt)->bat*/1){
		aileron[0] = rangein(approach(aileron[0], aileron[0] + .0003 * M_PI * inputs->analog[0], dt, 0.), -M_PI / 2., M_PI / 2.);
		aileron[1] = rangein(approach(aileron[1], aileron[1] - .0003 * M_PI * inputs->analog[0], dt, 0.), -M_PI / 2., M_PI / 2.);
		elevator = rangein(approach(elevator, elevator + .0003 * M_PI * inputs->analog[1], dt, 0.), -M_PI / 2., M_PI / 2.);
	}
	else{
		aileron[0] = rangein(approach(aileron[0], 0., dt, 0.), -M_PI / 2., M_PI / 2.);
		aileron[1] = rangein(approach(aileron[1], 0., dt, 0.), -M_PI / 2., M_PI / 2.);
		elevator = rangein(approach(elevator, 0., dt, 0.), -M_PI / 2., M_PI / 2.);
	}
	if(cooldown < dt)
		cooldown = 0;
	else
		cooldown -= dt;
/*	pt->pyr[2] = approach(pt->pyr[2] + M_PI, (pt->pyr[1] - oldyaw) / dt + M_PI, FLY_ROLLSPEED * dt, 2 * M_PI) - M_PI;*/
}

/*
static unsigned fly_analog_mask(const entity_t *pt, const warf_t *w){
	pt;w;
	return 3;
}
*/

//extern struct player *ppl;

#if 0
static void cmd_afterburner(int argc, char *argv[]){
	if(ppl && ppl->control){
		if(ppl->control->vft == &fly_s || ppl->control->vft == &valkie_s){
			fly_t *p = (fly_t*)ppl->control;
			p->afterburner = !p->afterburner;
			if(p->afterburner && p->throttle < .7)
				p->throttle = .7;
		}
	}
}

static void fly_start_control(entity_t *pt, warf_t *w){
	static int init = 0;
	const char *s;
	if(!init){
		init = 1;
		RegisterAvionics();
		CmdAdd("afterburner", cmd_afterburner);
	}
	s = CvarGetString("fly_start_control");
	if(s){
		CmdExec(s/*"exec fly_control.cfg"*/);
	}
}

static void fly_end_control(entity_t *pt, warf_t *w){
	const char *s;
	s = CvarGetString("fly_end_control");
	if(s){
		CmdExec(s);
	}
/*	CmdExec("exec fly_uncontrol.cfg");*/
}

const static GLdouble
train_offset[3] = {0., 0.0025, 0.}, fly_radius = .002;

static int tryshoot(warf_t *w, entity_t *pt, const double epos[3], double phi0, double v, double damage, double variance, const avec3_t pos){
	fly_t *p = (fly_t*)pt;
	struct bullet *pb;
	double yaw = pt->pyr[1];
	double pitch = pt->pyr[0];
	double desired[2];
	double (*theta_phi)[2];

	pb = BulletNew(w, pt, damage);
	{
/*		double phi = (rot ? phi0 : yaw) + (drseq(&gsrs) - .5) * variance;
		double theta = pt->pyr[0] + (drseq(&gsrs) - .5) * variance;
		theta += acos(vx / v);*/
		double phi, theta;
		phi = phi0 + (drseq(&w->rs) - .5) * variance;
		theta = pitch + (drseq(&w->rs) - .5) * variance;
		VECCPY(pb->velo, pt->velo);
		pb->velo[0] +=  v * sin(phi) * cos(theta);
		pb->velo[1] +=  v * sin(theta);
		pb->velo[2] += -v * cos(phi) * cos(theta);
		pb->pos[0] = pt->pos[0] + pos[0];
		pb->pos[1] = pt->pos[1] + pos[1];
		pb->pos[2] = pt->pos[2] + pos[2];
	}
	return 1;
}
#endif

static const avec3_t fly_points[] = {
	{.0, -.001, -.005},
	{-.001, -.0008, .003},
	{.001, -.0008, .003},
	{160 * FLY_SCALE, 20 * FLY_SCALE, -0 * FLY_SCALE},
	{-160 * FLY_SCALE, 20 * FLY_SCALE, -0 * FLY_SCALE},
	{0 * FLY_SCALE, 20 * FLY_SCALE, -160 * FLY_SCALE},
	{0 * FLY_SCALE, 20 * FLY_SCALE, 160 * FLY_SCALE},
};
static const avec3_t valkie_points[] = {
	{.0, -.0019, -.005},
	{-.0015, -.0018, .003},
	{.0015, -.0018, .003},
	{160 * FLY_SCALE, 20 * FLY_SCALE, -0 * FLY_SCALE},
	{-160 * FLY_SCALE, 20 * FLY_SCALE, -0 * FLY_SCALE},
	{0 * FLY_SCALE, 20 * FLY_SCALE, -160 * FLY_SCALE},
	{0 * FLY_SCALE, 20 * FLY_SCALE, 160 * FLY_SCALE},
};

static void foot_height(const char *name, void *hint){
	int i = 0;
	if(!strcmp(name, "ABbaseL")){
		amat4_t mat;
		glGetDoublev(GL_MODELVIEW_MATRIX, mat);
		VECCPY(((avec3_t*)hint)[0], &mat[12]);
	}
	else if(!strcmp(name, "ABbaseR")){
		amat4_t mat;
		glGetDoublev(GL_MODELVIEW_MATRIX, mat);
		VECCPY(((avec3_t*)hint)[1], &mat[12]);
	}
/*	else if(!strcmp(name, "LWING_BTL") || !strcmp(name, "RWING_BTL") && (i = 1) || !strcmp(name, "0009") && (i = 2)){
		amat4_t mat;
		avec3_t pos0[2] = {{(8.12820 - 3.4000) * .75, 0, (6.31480 - 3.) * -.5}, {0, 0, 6.}};
		glGetDoublev(GL_MODELVIEW_MATRIX, mat);
		if(!i)
			pos0[0][0] *= -1;
		mat4vp3(((avec3_t*)hint)[i+2], mat, pos0[i/2]);
	}*/
}

struct afterburner_hint{
	int ab;
	int muzzle;
	double thrust;
	double gametime;
	avec3_t wingtips[3];
};

static void find_wingtips(const char *name, struct afterburner_hint *hint);

void Aerial::anim(double dt){
	int inwater = 0;
	int onfeet = 0;
	int walking = 0;
	int direction = 0;

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

//	double air = w->atmospheric_pressure(w, &pt->pos)/*exp(-pt->pos[1] / 10.)*/;

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
		rudder = rangein(approach(rudder, (inputs & PL_A ? -M_PI / 6. : inputs & PL_D ? M_PI / 6. : 0), 1. * M_PI * dt, 0.), -M_PI / 6., M_PI / 6.);

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
			weapon = !weapon;
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
	this->inputs.press = 0;
	this->inputs.change = 0;

	/* forget about beaten enemy */
	if(this->enemy && this->enemy->getHealth() <= 0.)
		this->enemy = NULL;

/*	h = warmapheight(w->map, pt->pos[0], pt->pos[2], NULL);*/
//	if(w->vft->inwater)
//		inwater = w->vft->inwater(w, pt->pos);


#if FLY_QUAT

	if(this->pf){
		Vec3d pos = mat.vp3(Vec3d(0., .001, .0065));
		this->pf->move(pos, vec3_000, cs_blueburn.t, 0);
//		MoveTefpol3D(((fly_t *)pt)->pf, pos, avec3_000, cs_blueburn.t, 0);
	}
/*	if(vft != &valkie_s)*/{
		const Vec3d (&wingtips)[2] = /*vft == &fly_s ?*/ flywingtips /*: valkiewingtips*/;
		bool skip = !(.1 < -this->velo.sp(mat.vec3(2)));
		for(int i = 0; i < 2; i++) if(vapor[i]){
			Vec3d pos = mat.vp3(wingtips[i]);
			vapor[i]->move(pos, vec3_000, cs_vapor.t, skip);
		}
	}
	if(!inwater && 0. < health){
		double thrustsum;
		avec3_t zh, zh0 = {0., 0., 1.};
#if 0
		if(vft == &valkie_s){
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
		}
		MAT4DVP3(zh, mat, zh0);
		if(pt->vft == &valkie_s)
			thrustsum = -thrust_strength * pt->mass * p->throttle * (air + 1.5 * p->afterburner) * dt;
		else
			thrustsum = -thrust_strength * pt->mass * p->throttle * air * (1. + p->afterburner) * dt;
		VECSCALEIN(zh, thrustsum);
		RigidAddMomentum(pt, avec3_000, zh);
/*		VECSADD(pt->velo, zh, -.012 * p->throttle * dt);*/
#endif
	}

#if 0
	if(air){
		int i;
		avec3_t wings[5];
		avec3_t lifts[2];
		avec3_t angle, angle0 = {0., 1., 0.};
		avec3_t v, velo, v_cross_velo;
		amat4_t rot, irot;
		double s, velolen, f = 1.;
		{
			double c = aerotensor0[0][8] / aerotensor0[0][7];
			s = asin(c);
		}
		if(vft == &valkie_s){
			int i;
			struct valkie *p = (struct valkie*)pt;
			if(p->bat)
				f = (M_PI / 4. - -p->wing[0]) / (M_PI / 4.);
		}

		velolen = VECSLEN(pt->velo);

		quat2mat(rot, pt->rot);
		quat2imat(irot, pt->rot);

		MAT4DVP3(angle, mat, angle0);
		for(i = 0; i < 5; i++){
			MAT4DVP3(wings[i], mat, wings0[i]);
/*			printf("gggg %lg\n", VECSP(angle, wings[i]));*/
		}
		for(i = 0; i < 5; i++){
			avec3_t v1;
			double len, sp, f2;

			/* retrieve velocity of the wing center in absolute coordinates */
			VECVP(velo, pt->omg, wings[i]);
			VECADDIN(velo, pt->velo);

/*			VECVP(v, angle, velo);
			VECVP(v_cross_velo, v, velo);
			len = VECLEN(v);
			VECNORMIN(v_cross_velo);

			VECSCALE(lifts[i], v_cross_velo, -.1 * (p->flap[i] + .2 * M_PI) * len * pt->mass * dt);
			sp = VECSP(angle, velo);
			sp = -1. * ABS(sp) * pt->mass * dt;
			VECSCALE(v, velo, sp);
			VECADDIN(lifts[i], v);*/

			len = velolen < .05 ? velolen / .05 : velolen < .340 ? 1. : velolen / .340;
			f2 = f * air * pt->mass * dt * .15 * len;

			MAT4VP3(v1, irot, velo);

			if(i < 2){
				amat3_t rot2, rot3;
				MATROTY(rot2, aerotensor0[i/2], i * .2 - .1);
				MATROTX(rot3, rot2, p->aileron[i] / 2.);
				MATVP(v, rot3, v1);
				if(vft == &valkie_s)
					f2 *= 1. - p->throttle * .5;
			}
			else if(i < 4){
				amat3_t rot3;
				MATROTX(rot3, aerotensor0[i/2], p->elevator);
				MATVP(v, rot3, v1);
			}
			else if(i == 4){
				amat3_t rot3;
				MATROTY(rot3, aerotensor0[i/2], -p->rudder);
				MATVP(v, rot3, v1);
			}
			else{
				MATVP(v, aerotensor0[i/2], v1);
			}
			VECSCALEIN(v, f2/** velolen * 100.*/);
			MAT4VP3(v1, rot, v);
			VECCPY(p->force[i], v1);
			RigidAddMomentum(pt, wings[i], v1);
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

#else
	if(p->pf){
		MoveTefpol3D(((fly_t *)pt)->pf, pt->pos, avec3_000, cs_blueburn.t, 0);
	}
	if(0 < pt->health && pt->velo[0] * pt->velo[0] + pt->velo[2] * pt->velo[2] < .1 * .1){
		double acc[3];
		acc[0] = dt * .005 * sin(pt->pyr[1]);
		acc[1] = dt * .0098 / .05 / .05 * (pt->velo[0] * pt->velo[0] + pt->velo[2] * pt->velo[2] - (pt->pos[1] - h) / .1 * .1 * .1);
		acc[2] = dt * -.005 * cos(pt->pyr[1]);
		VECADDIN(pt->velo, acc);
		VECSADD(pt->velo, w->gravity, dt);
/*		if(.05 * .05 < VECSLEN(pt->velo))
			pt->velo[1] += dt / 10.;*/
		VECSADD(pt->pos, pt->velo, dt);
		if(pt->pos[1] < h){
			pt->pos[1] = h;
			pt->velo[1] = 0.;
		}
		pt->pyr[0] = atan2(-pt->velo[1], sqrt(pt->velo[0] * pt->velo[0] + pt->velo[2] * pt->velo[2]));
		return;
	}

	/* under earth; crash */
	if(pt->pos[1] < h){
		struct tent3d_line_list *tell = w->tell;
		effectDeath(w, pt);
		playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .1, w->realtime);
		if(w->gibs && ((struct entity_private_static*)pt->vft)->sufbase){
			int i, n, base;
			struct entity_private_static *vft = (struct entity_private_static*)pt->vft;
			int m = vft->sufbase->np;
			n = m <= FLY_MAX_GIBS ? m : FLY_MAX_GIBS;
			base = m <= FLY_MAX_GIBS ? 0 : rseq(&w->rs) % m;
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
		}
		pt->active = 0;
	}
	else if(0 < pt->health){
		double oldyaw = pt->pyr[1];
		if(w->pl->control == pt){
			int i;
			avec3_t wings[2], wings0[2] = {{0.003, 0., 0.}, {-0.003, 0., 0.}};
			avec3_t lifts[2];
			avec3_t angle, angle0 = {0., 1., 0.};
			avec3_t v;
			amat4_t rot;
			pt->pyr[0] -= .2 * (p->flap[0] + p->flap[0]) * dt;
			pt->pyr[1] -= .1 * (p->flap[0] - p->flap[1]) * dt;
			pt->pyr[2] -= .1 * (p->flap[0] - p->flap[1]) * dt;
			pt->velo[0] += dt * p->throttle * .11 * cos(pt->pyr[0]) * sin(pt->pyr[1]);
			pt->velo[1] += dt * p->throttle * -.11 * sin(pt->pyr[0]);
			pt->velo[2] += dt * p->throttle * -.11 * cos(pt->pyr[0]) * cos(pt->pyr[1]);
			pyrmat(pt->pyr, &rot);
			for(i = 0; i < 2; i++){
				MAT4VP3(wings[i], rot, wings0[i]);
			}
			MAT4DVP3(angle, rot, angle0);
			VECVP(v, angle, pt->velo);
			for(i = 0; i < 2; i++){
				double len;
				len = VECLEN(v);
				VECSCALE(lifts[i], angle, p->flap[i] * len);
				VECSADD(pt->velo, lifts[i], dt);
			}
			VECSCALEIN(pt->velo, 1. / (1. + .01 * dt));
			VECSADD(pt->pos, pt->velo, dt);
		}
		else{
		double pos[3], dv[3], dist;
		int i, n;
		avec3_t opos;
#if 0
		pt->pyr[1] = w->war_time / 10. + M_PI;
		VECCPY(opos, pt->pos);
		pt->pos[0] = .5 * cos(w->war_time / 10.);
		pt->pos[1] = .1;
		pt->pos[2] = .5 * sin(w->war_time / 10.);
		VECSUB(pt->velo, pt->pos, opos);
		VECSCALEIN(pt->velo, 1./dt);
#else
		if(!pt->enemy){ /* find target */
			double best = 2. * 2.;
			entity_t *t;
			for(t = w->tl; t; t = t->next) if(t != pt && t->vft != &flare_s && t->race != pt->race && 0. < t->health){
				double sdist = VECSDIST(pt->pos, t->pos);
				if(sdist < best){
					pt->enemy = t;
					best = sdist;
				}
			}
		}
		{
			avec3_t delta;
			if(pt->enemy){
				double sp;
				const avec3_t guns[2] = {{.002, .001, -.005}, {-.002, .001, -.005}};
				avec3_t xh, dh, vh;
				avec3_t epos;
				double phi;
				estimate_pos(&epos, pt->enemy->pos, pt->enemy->velo, pt->pos, pt->velo, ((struct entity_private_static*)pt->enemy->vft)->flying(pt) ? NULL : &sgravity, BULLETSPEED, w->map, w, ((struct entity_private_static*)pt->enemy->vft)->flying(pt->enemy));
				xh[0] = pt->velo[2];
				xh[1] = pt->velo[1];
				xh[2] = -pt->velo[0];
				VECSUB(delta, pt->pos, epos);
				{
					avec3_t rv;
					double eposlen;
					VECSUB(rv, pt->enemy->velo, pt->velo);
					eposlen = VECDIST(epos, pt->pos) / (VECSP(delta, rv) + BULLETSPEED);
					VECSADD(epos, w->gravity, -eposlen);
				}
				VECSUB(delta, pt->pos, epos);
				delta[1] += .001;
				if(VECSLEN(delta) < 2. * 2.){
					VECNORM(dh, delta);
					VECNORM(vh, pt->velo);
					sp = VECSP(dh, vh);
					if(sp < -.99){
#	if 1
						shootDualGun(pt, w, dt);
						if(pt->cooldown2 < dt)
							pt->cooldown2 = 0;
						else
							pt->cooldown2 -= dt;
#	else
						avec3_t epos;
						double phi;
						estimate_pos(&epos, pt->enemy->pos, pt->enemy->velo, pt->pos, pt->velo, pt->enemy->vft == &fly_s ? NULL : &sgravity, BULLETSPEED, w, ((struct entity_private_static*)pt->enemy->vft)->flying(pt->enemy));
						VECSUB(delta, epos, pt->pos);
						phi = atan2(epos[0] - pt->pos[0], -(epos[2] - pt->pos[2]));
						if(shoot_angle(pt->pos, epos, phi, BULLETSPEED, &pt->desired)){
							if(pt->cooldown2 < dt){
								avec3_t gunpos;
								amat4_t mat;
								pt->barrelp = /*(pt->enemy->vft == &fly_s ? 1 : -1) **/ pt->desired[0] - pt->pyr[0];
/*								VECNORMIN(xh);
								VECSCALE(gunpos, xh,  .001);*/
								pyrmat(pt->pyr, &mat);
								MAT4VP3(gunpos, mat, guns[0]);
								if(tryshoot(w, pt, epos, phi, BULLETSPEED, 5., .005, gunpos)){
/*									VECSCALE(gunpos, xh,  -.001);*/
									MAT4VP3(gunpos, mat, guns[1]);
									tryshoot(w, pt, epos, phi, BULLETSPEED, 5., .005, gunpos);
									shootsound(pt, w);
									pt->shoots++;
									pt->cooldown2 = FLY_RELOADTIME;
								}
								else
									pt->cooldown2 = FLY_RELOADTIME;
							}
							else
								pt->cooldown2 -= dt;
						}
#endif
					}
				}
			}
			else
				VECCPY(delta, pt->pos);
			if(pt->pos[1] < h + .05){
				/* going down to ground! */
				pt->pyr[0] = approach(pt->pyr[0] + M_PI, -M_PI * .3 + M_PI, .02 * M_PI * dt, 2 * M_PI) - M_PI;
			}
			else if(2. * 2. < VECSLEN(delta)/* && 0 < VECSP(pt->velo, pt->pos)*/){
				avec3_t xh;
				xh[0] = pt->velo[2];
				xh[1] = pt->velo[1];
				xh[2] = -pt->velo[0];
				if(VECSP(delta, xh) < 0.)
					pt->pyr[1] -= .1 * M_PI * dt;
				else
					pt->pyr[1] += .1 * M_PI * dt;
				pt->pyr[0] = approach(pt->pyr[0] + M_PI, 0. + M_PI, .02 * M_PI * dt, 2 * M_PI) - M_PI;
			}
			else if(.5 * .5 < VECSLEN(delta)){
				avec3_t xh;
				xh[0] = pt->velo[2];
				xh[1] = pt->velo[1];
				xh[2] = -pt->velo[0];
				if(VECSP(pt->velo, delta) < 0){
					double desired;
					desired = fmod(atan2(-delta[0], delta[2]) + 2 * M_PI, 2 * M_PI);

					/* roll */
/*					pt->pyr[2] = approach(pt->pyr[2], (pt->pyr[1] < desired ? 1 : desired < pt->pyr[1] ? -1 : 0) * M_PI / 6., .1 * M_PI * dt, 2 * M_PI);*/

					/* precise approach when shooting! */
					pt->pyr[1] = approach(pt->pyr[1], desired, .1 * M_PI * dt, 2 * M_PI);

					if(pt->enemy && pt->pyr[1] == desired){
					}

					pt->pyr[0] = approach(pt->pyr[0], atan2(delta[1], sqrt(delta[0] * delta[0] + delta[2] * delta[2])), .02 * M_PI * dt, 2 * M_PI);
				}
				else{
					const double desiredy = pt->pos[1] < .1 ? -M_PI / 6. : 0.;
					pt->pyr[0] = approach(pt->pyr[0], desiredy, .02 * M_PI * dt, 2 * M_PI);
				}
			}
			else{
				double desiredp;
				desiredp = fmod(atan2(-delta[0], delta[2]) + M_PI + 2 * M_PI, 2 * M_PI);
				pt->pyr[1] = approach(pt->pyr[1], desiredp, .1 * M_PI * dt, 2 * M_PI);
				pt->pyr[0] = approach(pt->pyr[0], pt->pos[1] < .1 ? -M_PI / 6. : 0., .02 * M_PI * dt, 2 * M_PI);
			}
		}

		/* detect infrared guided missiles toward me and drop a flare to evade them */
		{
			if(pt->cooldown < dt){
				struct ssm *ssm;
				extern struct bullet_static ssm_s; /* temporary!! */
				for(ssm = w->bl; ssm; ssm = ssm->next) if(ssm->active && ssm->vft == &ssm_s && ssm->target == pt)
					break;
				if(ssm != NULL)
					dropflare(pt, w, dt);
				else
					pt->cooldown += FLY_RETHINKTIME;
			}
			else
				pt->cooldown -= dt;
		}
		pt->pyr[2] = approach(pt->pyr[2] + M_PI, (pt->pyr[1] - oldyaw) / dt + M_PI, FLY_ROLLSPEED * dt, 2 * M_PI) - M_PI;
#if 0 /* can't figure aerodynamics */
		{
			double velo;
			avec3_t nh;
			nh[0] = sin(pt->pyr[0]) * sin(pt->pyr[1]);
			nh[1] = cos(pt->pyr[0]);
			nh[2] = -sin(pt->pyr[0]) * cos(pt->pyr[1]);
			velo = VECLEN(pt->velo);
			VECSCALEIN(pt->velo, 1. - 10. * velo * velo * dt);
			VECSADD(pt->velo, nh, .01 * (1. - 1. / (1. + 50. * velo)) * dt);
		}
		pt->velo[0] += dt * .011 * cos(pt->pyr[0]) * sin(pt->pyr[1]);
		pt->velo[1] += dt * -.011 * sin(pt->pyr[0]);
		pt->velo[2] += dt * -.011 * cos(pt->pyr[0]) * cos(pt->pyr[1]);
		pt->pyr[2] = approach(pt->pyr[2] + M_PI, (pt->pyr[1] - oldyaw) / dt + M_PI, FLY_ROLLSPEED * dt, 2 * M_PI) - M_PI;
		VECSADD(pt->pos, pt->velo, dt);
#else
		pt->velo[0] = .11 * cos(pt->pyr[0]) * sin(pt->pyr[1]);
		pt->velo[1] = -.11 * sin(pt->pyr[0]);
		pt->velo[2] = -.11 * cos(pt->pyr[0]) * cos(pt->pyr[1]);
		VECSADD(pt->pos, pt->velo, dt);
#endif
#endif
#if 0
		n = (int)(dt * FLY_MISSILE_FREQ + drseq(&w->rs));
		for(i = 0; i < n; i++){
			struct ssm *pb;
			entity_t *target;
			int nt;
			pos[0] = pt->pos[0] + (drseq(&w->rs) - .5) * .01;
			pos[1] = pt->pos[1] + (drseq(&w->rs) - .5) * .01;
			pos[2] = pt->pos[2] + (drseq(&w->rs) - .5) * .01;
			dv[0] = .5 * pt->velo[0] + (drseq(&w->rs) - .5) * .01;
			dv[1] = .5 * pt->velo[1] + (drseq(&w->rs) - .5) * .01;
			dv[2] = .5 * pt->velo[2] + (drseq(&w->rs) - .5) * .01;
			pb = add_ssm(w, pos);
			pb->owner = pt;
			pb->damage = 100;
			VECCPY(pb->velo, dv);
			pb->target = pt->enemy;
/*			target = w->tl;
			for(nt = 0; target; target = target->next) if(target->next != pt) nt++;
			nt = rseq(&w->rs) % nt;
			for(target = w->tl; nt; target = target->next) if(target->next != pt) nt--;
			pb->target = target == pt ? target->next : target;*/
			pt->shoots++;
		}
#endif
		}
	}
	else
#endif
	if(health <= 0.){
/*		if(tent3d_line_list *tell = w->getTeline3d()){
			Vec3d gravity = w->accel(this->pos, this->velo) * -1./2.;
			int n = (int)(dt * FLY_SMOKE_FREQ + drseq(&w->rs));
			for(int i = 0; i < n; i++){
				Vec3d pos = this->pos;
				for(int j = 0; j < 3; j++)
					pos[j] += (drseq(&w->rs) - .5) * .01;
				Vec3d dv = this->velo * .5;
				for(int j = 0; j < 3; j++)
					dv[j] += (drseq(&w->rs) - .5) * .01;
				AddTelineCallback3D(tell, pos, dv, .01, quat_u, vec3_000, gravity, smoke_draw, w, 0, 1.5 + drseq(&w->rs) * 1.5);
			}
		}*/
/*		VECSADD(pt->velo, w->gravity, dt);
		VECSADD(pt->pos, pt->velo, dt);*/
	}
}

bool Aerial::cull(WarDraw *wd)const{
	if(wd->vw->gc->cullFrustum(pos, .012))
		return 1;
	double pixels = .008 * fabs(wd->vw->gc->scale(pos));
//	if(ppixels)
//		*ppixels = pixels;
	if(pixels < 2)
		return 1;
	return 0;
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

int Aerial::takedamage(double damage, int hitpart){
	struct tent3d_line_list *tell = w->getTeline3d();
	int ret = 1;
	if(tell){
/*		int j, n;
		frexp(damage, &n);
		for(j = 0; j < n; j++){
			double velo[3];
			velo[0] = .15 * (drseq(&w->rs) - .5);
			velo[1] = .15 * (drseq(&w->rs) - .5);
			velo[2] = .15 * (drseq(&w->rs) - .5);
			AddTeline3D(tell, pt->pos, velo, .001, NULL, NULL, w->gravity,
				j % 2 ? COLOR32RGBA(255,255,255,255) : COLOR32RGBA(255,191,63,255),
				TEL3_HEADFORWARD | TEL3_REFLECT | TEL3_FADEEND, .5 + drseq(&w->rs) * .5);
		}*/
	}
//	playWave3D("hit.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .1, w->realtime);
	if(0 < health && health - damage <= 0){
		int i;
		ret = 0;
/*		effectDeath(w, pt);*/
		for(i = 0; i < 32; i++){
			Vec3d pos = this->pos;
			Vec3d velo = this->velo;
			for(int j = 0; j < 3; j++)
				velo[j] = drseq(&w->rs) - .5;
			velo.normin() *= 0.1;
			pos += velo, 0.1;
			AddTeline3D(w->getTeline3d(), pos, velo, .005, quat_u, vec3_000, w->accel(pos, velo),
				COLOR32RGBA(255, 31, 0, 255), TEL3_HEADFORWARD | TEL3_THICK | TEL3_FADEEND | TEL3_REFLECT, 1.5 + drseq(&w->rs));
		}
		TefpolList *tepl = w->getTefpol3d();
		pf = tepl->addTefpolMovable(this->pos, this->velo, vec3_000, &cs_firetrail, TEP3_THICKER | TEP3_ROUGH, cs_firetrail.t);
//		playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
	}
	health -= damage;
	if(health < -1000.){
		if(game->isServer())
			delete this;
		return 0;
	}
	return ret;
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
