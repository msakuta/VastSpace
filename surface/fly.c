#include "commondraw.h"
#include "viewer.h"
#include "train.h"
#include "player.h"
#include "entity_p.h"
#include "bullet.h"
#include "ssm.h"
#include "tent3d.h"
#include "bhole.h"
#include "suflist.h"
#include "warmap.h"
#include "rigid.h"
#include "coordcnv.h"
#include "aim9.h"
#include "calc/calc.h"
#include "yssurf.h"
#include "arms.h"
#include "walk.h"
#include "warutil.h"
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/rseq.h>
#include <clib/GL/gldraw.h>
#include <clib/colseq/cs.h>
#include <clib/amat3.h>
#include <clib/amat4.h>
#include <clib/wavsound.h>
#include <clib/suf/sufdraw.h>
#include <clib/aquat.h>
#include <clib/timemeas.h>
#include <clib/lzw/lzw.h>
#include <clib/suf/sufbin.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>


#include "sufsrc/fly1.h"
#include "sufsrc/fly2.h"
#include "sufsrc/fly3.h"
#include "sufsrc/valkie.h"
#include "sufsrc/valkie_leg.h"
#include "sufsrc/fly2flap.h"
#include "sufsrc/fly2gear.h"
#include "sufsrc/fly2cockpit.h"
#include "sufsrc/fly2elev.h"
#include "sufsrc/fly2stick.h"


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

static const avec3_t flywingtips[2] = {
	{-160. * FLY_SCALE, 20. * FLY_SCALE, 10. * FLY_SCALE},
	{160. * FLY_SCALE, 20. * FLY_SCALE, 10. * FLY_SCALE},
},
valkiewingtips[2] = {
	{-440. * .5 * FLY_SCALE, 50. * .5 * FLY_SCALE, 210. * .5 * FLY_SCALE},
	{440. * .5 * FLY_SCALE, 50. * .5 * FLY_SCALE, 210. * .5 * FLY_SCALE},
};

void smoke_draw(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv);


static void fly_drawHUD(entity_t *pt, warf_t *, wardraw_t *, const double irot[16], void (*)(void));
static void fly_cockpitview(entity_t *pt, warf_t*, double (*pos)[3], int *);
static void fly_control(entity_t *pt, warf_t *w, input_t *inputs, double dt);
static void fly_destruct(entity_t *pt);
static void fly_anim(entity_t *pt, warf_t *w, double dt);
static void fly_draw(entity_t *pt, wardraw_t *wd);
static void fly_drawtra(entity_t *pt, wardraw_t *wd);
static int fly_takedamage(entity_t *pt, double damage, warf_t *w);
static void fly_gib_draw(const struct tent3d_line_callback *pl, void *pv);
static int fly_flying(const entity_t *pt);
static void fly_bullethole(entity_t *pt, sufindex, double rad, const double (*pos)[3], const double (*pyr)[3]);
static int fly_getrot(struct entity*, warf_t *, double (*)[4]);
static void fly_drawCockpit(struct entity*, const warf_t *, wardraw_t *);
static const char *fly_idname(struct entity*pt){return "fly";}
static const char *fly_classname(struct entity*pt){return "Fly";}
static void fly_start_control(entity_t *pt, warf_t *w);
static void fly_end_control(entity_t *pt, warf_t *w);
static void fly_shadowdraw(entity_t *pt, wardraw_t *wd);
static unsigned fly_analog_mask(const entity_t *pt, const warf_t *w);

static struct entity_private_static fly_s = {
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
		fly_idname,
		fly_classname,
		fly_start_control,
		fly_end_control,
		fly_analog_mask,
	},
	fly_anim,
	fly_draw,
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
	fly_draw,
};

typedef struct fly{
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
} fly_t;

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




int fly_togglenavlight(struct player *pl){
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






static void flare_anim(entity_t *pt, warf_t *w, double dt);
static void flare_drawtra(entity_t *pt, wardraw_t *wd);
static int flare_flying(const entity_t *pt);

struct entity_private_static flare_s = {
	{
		NULL,
		NULL,
	},
	flare_anim,
	NULL,
	flare_drawtra,
	NULL,
	NULL,
	NULL,
	flare_flying,
	0., 0., 0.,
	NULL, NULL, NULL, NULL,
	0,
	0.
};

typedef struct flare{
	entity_t st;
	struct tent3d_fpol *pf;
	double ttl;
} flare_t;

static void fly_loadwingfile(){
	static int wingfile = 0;
	if(!wingfile){
		FILE *fp;
		int i;
		wingfile = 1;
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
					if(p)
						wings0[i][j] = calc3(&p, NULL, NULL);
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

static void fly_init(fly_t *fly){
	int i;
	fly->st.weapon = 0;
	fly->aileron[0] = fly->aileron[1] = 0.;
	fly->elevator = 0.;
	fly->rudder = 0.;
	fly->gearphase = 0.;
	fly->cooldown = 0.;
	fly->muzzle = 0;
	fly->brk = 1;
	fly->afterburner = 0;
	fly->navlight = 0;
	fly->gear = 0;
	fly->missiles = 10;
	fly->throttle = 0.;
	for(i = 0; i < numof(fly->bholes)-1; i++)
		fly->bholes[i].next = &fly->bholes[i+1];
	fly->bholes[numof(fly->bholes)-1].next = NULL;
	fly->frei = fly->bholes;
	for(i = 0; i < fly->sd->np; i++)
		fly->sd->p[i] = NULL;
	fly->st.inputs.press = fly->st.inputs.change = 0;
}

fly_t *FlyNew(warf_t *w){
	fly_t *fly;
	entity_t *ret;
	fly_loadwingfile();
	fly = malloc(sizeof *fly);
	ret = &fly->st;
	EntityInit(ret, w, &fly_s);
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
	*(struct entity_private_static**)&ret->vft = &fly_s;
	ret->next = w->tl;
	w->tl = ret;
	ret->health = 500;*/
	fly->pf = AddTefpolMovable3D(w->tepl, fly->st.pos, ret->velo, avec3_000, &cs_blueburn, TEP3_THICKER | TEP3_ROUGH, cs_blueburn.t);
	fly->vapor[0] = AddTefpolMovable3D(w->tepl, fly->st.pos, ret->velo, avec3_000, &cs_vapor, TEP3_FAINT | TEP3_ROUGH, cs_vapor.t * 10);
	fly->vapor[1] = AddTefpolMovable3D(w->tepl, fly->st.pos, ret->velo, avec3_000, &cs_vapor, TEP3_FAINT | TEP3_ROUGH, cs_vapor.t * 10);
	fly->sd = AllocSUFDecal(gsuf_fly);
	fly->sd->drawproc = bullethole_draw;
	fly_init(fly);
/*	if(!w->pl->chase)
		w->pl->chase = ret;*/
	return fly;
}

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


flare_t *FlareNew(warf_t *w, avec3_t pos){
	flare_t *flare;
	entity_t *ret;
	flare = malloc(sizeof *flare);
	ret = &flare->st;
	VECCPY(ret->pos, pos);
	VECNULL(ret->velo);
	VECNULL(ret->pyr);
	ret->health = 1.;
	ret->shoots = ret->shoots2 = ret->kills = ret->deaths = 0;
	ret->enemy = NULL;
	ret->active = 1;
	*(struct entity_private_static**)&ret->vft = &flare_s;
	ret->next = w->tl;
	w->tl = ret;
	ret->health = 500;
	flare->pf = AddTefpolMovable3D(w->tepl, ret->pos, ret->velo, avec3_000, &cs_firetrail, TEP3_THICKER | TEP3_ROUGH, cs_firetrail.t);
	flare->ttl = 5.;
	return flare;
}

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

static void fly_cockpitview(entity_t *pt, warf_t *w, double (*pos)[3], int *chasecam){
	avec4_t ofs, src[] = {
		/*{0., .008, 0.015, 1.}*/
		{0., 40 * FLY_SCALE, -120 * FLY_SCALE},
		{0., .0055, .015},
		{0.013, .007, .025},
		{.010, .007, -.010},
		{.010, .007, -.010},
		{0.004, .0, .0},
		{-0.004, .0, .0},
	};
	amat4_t mat;
	int camera;
	if(chasecam){
		camera = *chasecam;
		camera = MAX(0, MIN(numof(src)+2, camera));
		*chasecam = camera;
	}
	else
		camera = 0;
#if FLY_QUAT
	tankrot(&mat, pt);
	if(camera == numof(src)+1){
		VECCPY(*pos, pt->pos);
		(*pos)[0] += .05 * cos(w->war_time * 2. * M_PI / 15.);
		(*pos)[1] += .02;
		(*pos)[2] += .05 * sin(w->war_time * 2. * M_PI / 15.);
	}
	else if(camera == 4 && pt->vft == &valkie_s){
		valkie_t *p = (valkie_t*)pt;
		amat4_t mat2;
		mat4roty(mat2, mat, /*5 * M_PI / 4.*/ + p->batphase * M_PI);
		mat4vp3(*pos, mat2, src[camera]);
	}
	else if(camera == numof(src)){
		extern double g_viewdist;
		mat4dvp3(ofs, mat, avec3_001);
		if(camera)
			VECSCALEIN(ofs, g_viewdist);
		VECADD(*pos, pt->pos, ofs);
	}
	else if(camera == numof(src)+2){
		avec3_t pos0;
		const double period = VECSLEN(pt->velo) < .1 * .1 ? .5 : 1.;
		struct contact_info ci;
		pos0[0] = floor(pt->pos[0] / period + .5) * period;
		pos0[1] = 0./*floor(pt->pos[1] / period + .5) * period*/;
		pos0[2] = floor(pt->pos[2] / period + .5) * period;
		if(w && w->vft->pointhit && w->vft->pointhit(w, pos0, avec3_000, 0., &ci))
			VECSADD(pos0, ci.normal, ci.depth);
		else if(w && w->vft->spherehit && w->vft->spherehit(w, pos0, .002, &ci))
			VECSADD(pos0, ci.normal, ci.depth);
		VECCPY(*pos, pos0);
	}
	else{
		MAT4VP3(*pos, mat, src[camera]);
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

static int dropflare(entity_t *pt, warf_t *w, double dt){
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

}

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

static void flyShootDualGun(entity_t *pt, warf_t *w, double dt){
	fly_t *p = (fly_t*)pt;
	avec3_t velo, gunpos, velo0 = {0., 0., -BULLETSPEED};
	amat4_t mat, mat0;
	double reloadtime = FLY_RELOADTIME;
	struct bullet *(*New)(warf_t*, entity_t*, double) = BulletNew;
	if(dt <= p->cooldown)
		return;
	tankrot(&mat0, pt);
	if(pt->vft == &valkie_s){
		valkie_t *p = (valkie_t *)pt;
		mat4rotx(mat, mat0, -p->torsopitch);
		if(p->arms[0].type != arms_none){
			reloadtime = arms_static[p->arms[0].type].cooldown;
		}
		if(p->arms[0].type == arms_valkierifle){
			New = BeamNew;
		}
	}
	else
		mat4rotx(mat, mat0, -gunangle / deg_per_rad);
	while(p->cooldown < dt){
		int i = 0;
		do{
			struct bullet *pb;
			double phi, theta;
/*			MAT4VP3(gunpos, mat, fly_guns[i]);*/
			pb = New(w, pt, 5./*/50.*/);
			
			pb->mass = .005;
			pb->life = 10.;
/*			phi = pt->pyr[1] + (drseq(&w->rs) - .5) * .005;
			theta = pt->pyr[0] + (drseq(&w->rs) - .5) * .005;
			VECCPY(pb->velo, pt->velo);
			pb->velo[0] +=  BULLETSPEED * sin(phi) * cos(theta);
			pb->velo[1] += -BULLETSPEED * sin(theta);
			pb->velo[2] += -BULLETSPEED * cos(phi) * cos(theta);
			pb->pos[0] = pt->pos[0] + gunpos[0];
			pb->pos[1] = pt->pos[1] + gunpos[1];
			pb->pos[2] = pt->pos[2] + gunpos[2];*/
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
				mat4vp3(pb->pos, mat, fly_guns[i]);
			mat4dvp3(pb->velo, mat, velo0);
			VECADDIN(pb->velo, pt->velo);
			pb->velo[0] += (drseq(&w->rs) - .5) * .005;
			pb->velo[1] += (drseq(&w->rs) - .5) * .005;
			pb->velo[2] += (drseq(&w->rs) - .5) * .005;
			pb->vft->anim(pb, w, w->tell, dt - p->cooldown);
		} while(pt->vft != &valkie_s && !i++);
		p->cooldown += reloadtime;
		pt->shoots += 1;
	}
	shootsound(pt, w, p->cooldown);
	((fly_t*)pt)->muzzle = 1;
}

static void fly_control(entity_t *pt, warf_t *w, input_t *inputs, double dt){
	fly_t *p = (fly_t*)pt;
	double oldyaw = pt->pyr[1];
	if(!pt->active || pt->health <= 0.)
		return;
	pt->inputs = *inputs;
/*	if(inputs->change)
		p->brk = !!(inputs->press & PL_B);*/
	if(.1 * .1 <= pt->velo[0] * pt->velo[0] + pt->velo[2] * pt->velo[2]){
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
	if(p->cooldown < dt)
		p->cooldown = 0;
	else
		p->cooldown -= dt;
	if(inputs->press & (PL_LCLICK | PL_ENTER)){
		if(!pt->weapon)
			flyShootDualGun(pt, w, dt);
		else if(0 < p->missiles && p->cooldown < dt){
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
	}
	if(inputs->press & inputs->change & (PL_G))
		p->gear = !p->gear;
	if(pt->vft != &valkie_s || !((valkie_t*)pt)->bat){
		p->aileron[0] = rangein(approach(p->aileron[0], p->aileron[0] + .0003 * M_PI * inputs->analog[0], dt, 0.), -M_PI / 2., M_PI / 2.);
		p->aileron[1] = rangein(approach(p->aileron[1], p->aileron[1] - .0003 * M_PI * inputs->analog[0], dt, 0.), -M_PI / 2., M_PI / 2.);
		p->elevator = rangein(approach(p->elevator, p->elevator + .0003 * M_PI * inputs->analog[1], dt, 0.), -M_PI / 2., M_PI / 2.);
	}
	else{
		p->aileron[0] = rangein(approach(p->aileron[0], 0., dt, 0.), -M_PI / 2., M_PI / 2.);
		p->aileron[1] = rangein(approach(p->aileron[1], 0., dt, 0.), -M_PI / 2., M_PI / 2.);
		p->elevator = rangein(approach(p->elevator, 0., dt, 0.), -M_PI / 2., M_PI / 2.);
	}
	if(p->cooldown < dt)
		p->cooldown = 0;
	else
		p->cooldown -= dt;
/*	pt->pyr[2] = approach(pt->pyr[2] + M_PI, (pt->pyr[1] - oldyaw) / dt + M_PI, FLY_ROLLSPEED * dt, 2 * M_PI) - M_PI;*/
}

static unsigned fly_analog_mask(const entity_t *pt, const warf_t *w){
	pt;w;
	return 3;
}

extern struct player *ppl;

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

static void fly_anim(entity_t *pt, warf_t *w, double dt){
	fly_t *const p = (fly_t*)pt;
	struct entity_private_static *vft = (struct entity_private_static*)pt->vft;
	double air;
	int inputs;
	int inwater = 0;
	int onfeet = 0;
	int walking = 0;
	int direction = 0;
	amat4_t mat;
	amat3_t ort3;
	avec3_t accel;

	if(!pt->active){
		pt->enemy = NULL;
		pt->deaths++;
		if(pt->race < numof(w->races))
			w->races[pt->race].deaths++;
		pt->health = 500;
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

	w->vft->orientation(w, &ort3, pt->pos);
/*	VECSADD(pt->velo, w->gravity, dt);*/
	w->vft->accel(w, &accel, pt->pos, pt->velo);
	VECSADD(pt->velo, accel, dt);

	air = w->vft->atmospheric_pressure(w, &pt->pos)/*exp(-pt->pos[1] / 10.)*/;

	tankrot(&mat, pt);

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

	inputs = pt->inputs.press;

	if(0. < pt->health){
		double common = 0., normal = 0., best = .3;
		avec3_t nh, nh0 = {0., 0., -1.};
		entity_t *pt2;
/*		common = inputs & PL_W ? M_PI / 4. : inputs & PL_S ? -M_PI / 4. : 0.;
		normal = inputs & PL_A ? M_PI / 4. : inputs & PL_D ? -M_PI / 4. : 0.;*/
		if(w->pl->control == pt){
			avec3_t pyr;
			quat2pyr(w->pl->rot, pyr);
			common = -pyr[0];
			normal = -pyr[1];
		}
		else{
			amat3_t mat3, mat2, iort3;
			quat2mat(&mat, pt->rot);
			MAT4TO3(mat3, mat);
			MATTRANSPOSE(iort3, ort3);
			MATMP(mat2, iort3, mat3);
#if 1
			common = VECSP(pt->velo, &ort3[3]) / 5e-0;
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
		p->rudder = rangein(approach(p->rudder, (inputs & PL_A ? -M_PI / 6. : inputs & PL_D ? M_PI / 6. : 0), 1. * M_PI * dt, 0.), -M_PI / 6., M_PI / 6.);

		p->gearphase = approach(p->gearphase, p->gear, 1. * dt, 0.);

		if(inputs & PL_W)
			p->throttle = approach(p->throttle, 1., .5 * dt, 5.);
		if(inputs & PL_S){
			p->throttle = approach(p->throttle, 0., .5 * dt, 5.);
			if(p->afterburner && p->throttle < .5)
				p->afterburner = 0;
		}

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

		if(pt->inputs.change & pt->inputs.press & PL_RCLICK)
			pt->weapon = !pt->weapon;
/*		if(pt->inputs.change & pt->inputs.press & PL_B)
			p->brk = !p->brk;*/
/*		if(pt->inputs.change & pt->inputs.press & PL_TAB){
			p->afterburner = !p->afterburner;
			if(p->afterburner && p->throttle < .7)
				p->throttle = .7;
		}*/
		quatrot(nh, pt->rot, nh0);
		for(pt2 = w->tl; pt2; pt2 = pt2->next) if(pt2 != pt && pt2->race != -1 && ((struct entity_private_static*)pt2->vft)->flying(pt2)){
			avec3_t delta;
			double c;
			VECSUB(delta, pt2->pos, pt->pos);
			c = VECSP(nh, delta) / VECLEN(delta);
			if(best < c && VECSLEN(delta) < 5. * 5.){
				best = c;
				pt->enemy = pt2;
				break;
			}
		}

	}
	pt->inputs.press = 0;
	pt->inputs.change = 0;

	/* forget about beaten enemy */
	if(pt->enemy && pt->enemy->health <= 0.)
		pt->enemy = NULL;

/*	h = warmapheight(w->map, pt->pos[0], pt->pos[2], NULL);*/
	if(w->vft->inwater)
		inwater = w->vft->inwater(w, pt->pos);


#if FLY_QUAT

	if(p->pf){
		avec3_t pos, pos0 = {0., .001, .0065};
		MAT4VP3(pos, mat, pos0);
		MoveTefpol3D(((fly_t *)pt)->pf, pos, avec3_000, cs_blueburn.t, 0);
	}
	if(vft != &valkie_s){
		int i, skip;
		avec3_t *wingtips = vft == &fly_s ? flywingtips : valkiewingtips;
		skip = !(.1 < -VECSP(pt->velo, &mat[8]));
		for(i = 0; i < 2; i++) if(p->vapor[i]){
			avec3_t pos;
			MAT4VP3(pos, mat, wingtips[i]);
			MoveTefpol3D(p->vapor[i], pos, avec3_000, cs_vapor.t, skip);
		}
	}
	if(!inwater && 0. < pt->health){
		double thrustsum;
		avec3_t zh, zh0 = {0., 0., 1.};
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
	}

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
	VECSADD(pt->pos, pt->velo, dt);
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
	if(pt->health <= 0.){
		if(w->tell){
			double pos[3], dv[3], dist;
			avec3_t gravity;
			int i, n;
			VECSCALE(gravity, w->gravity, -1./2.);
			n = (int)(dt * FLY_SMOKE_FREQ + drseq(&w->rs));
			for(i = 0; i < n; i++){
				pos[0] = pt->pos[0] + (drseq(&w->rs) - .5) * .01;
				pos[1] = pt->pos[1] + (drseq(&w->rs) - .5) * .01;
				pos[2] = pt->pos[2] + (drseq(&w->rs) - .5) * .01;
				dv[0] = .5 * pt->velo[0] + (drseq(&w->rs) - .5) * .01;
				dv[1] = .5 * pt->velo[1] + (drseq(&w->rs) - .5) * .01;
				dv[2] = .5 * pt->velo[2] + (drseq(&w->rs) - .5) * .01;
/*				AddTeline3D(w->tell, pos, dv, .01, NULL, NULL, gravity, COLOR32RGBA(0,0,0,255), TEL3_SPRITE | TEL3_INVROTATE | TEL3_NOLINE | TEL3_REFLECT, 1.5 + drseq(&w->rs) * 1.5);*/
				AddTelineCallback3D(w->tell, pos, dv, .01, NULL, NULL, gravity, smoke_draw, w, 0, 1.5 + drseq(&w->rs) * 1.5);
			}
		}
/*		VECSADD(pt->velo, w->gravity, dt);
		VECSADD(pt->pos, pt->velo, dt);*/
	}
}

static int fly_cull(entity_t *pt, wardraw_t *wd, double *ppixels){
	double pixels;
	if(glcullFrustum(pt->pos, .012, wd->pgc))
		return 1;
	pixels = .008 * fabs(glcullScale(&pt->pos, wd->pgc));
	if(ppixels)
		*ppixels = pixels;
	if(pixels < 2)
		return 1;
	return 0;
}

static void draw_arms(const char *name, valkie_t *p){
	static ysdnm_t *dnmgunpod1 = NULL;
	int j;
	double x;

	if(!strcmp(name, "GunBase")){
		glTranslated(0.0000, - -0.3500, - -3.0000);
		if(arms_static[p->arms[0].type].draw)
			arms_static[p->arms[0].type].draw(&p->arms[0], p->batphase);
/*		struct random_sequence rs;
		static const amat4_t mat = {
			-1,0,0,0,
			0,-1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};
		avec3_t pos = {.13, -.37, 3.};
		if(!dnmgunpod1)
			dnmgunpod1 = LoadYSDNM("vf25_gunpod.dnm");
		if(dnmgunpod1){
			double rot[numof(valkie_bonenames)][7];
			ysdnmv_t v, *dnmv;

			dnmv = valkie_boneset(p, rot, &v, 0);
			v.bonenames = valkie_bonenames;
			v.bonerot = rot;
			v.bones = numof(valkie_bonenames);
			v.target = NULL;
			v.next = NULL;

			DrawYSDNM_V(dnmgunpod1, dnmv);
/*			DrawSUF(sufgunpod1, SUF_ATR, &g_gldcache);
		}*/
	}
}

wardraw_t *g_smoke_wd;

static suf_t *sufleg = NULL;

static void fly_draw_int(entity_t *pt, wardraw_t *wd, double pixels){
	static int init = 0;
	static suf_t *sufflap;
	fly_t *p = (fly_t*)pt;
	struct entity_private_static *vft = (struct entity_private_static *)pt->vft;
	int i;

#if 0
	for(i = 0; i < numof(cnl_vapor); i++){
		cnl_vapor[i].col = COLOR32MUL(cnl0_vapor[i].col, wd->ambient);
	}

	if(!pt->active)
		return;

	/* cull object */
	if(fly_cull(pt, wd, &pixels))
		return;
	wd->lightdraws++;


	if(!vft->sufbase) do{
		int i, j, k;
		double best = 0., t, t2;
		FILE *fp;
		suf_t *suf;
		timemeas_t tm;
		TimeMeasStart(&tm);
		vft->sufbase = /*gsuf_fly /*//*&suf_fly2*/ /*lzuc(lzw_fly2, sizeof lzw_fly2, NULL)*/
			lzuc(lzw_valkie, sizeof lzw_valkie, NULL);
		if(!vft->sufbase)
			break;
		t = TimeMeasLap(&tm);
		RelocateSUF(vft->sufbase);
		t2 = TimeMeasLap(&tm);
		printf("relocf: %lg - %lg = %lg\n", t2, t, t2 - t);
		sufflap = &suf_fly2flap;
		for(i = 0; i < vft->sufbase->nv; i++){
			double sd = VECSLEN(vft->sufbase->v[i]);
			if(best < sd)
				best = sd;
		}
/*		vft->hitradius = FLY_SCALE * sqrt(best) * 1.2;*/
		suf = vft->sufbase;
		for(j = 0; j < suf->np; j++){
			avec3_t v01, n;

			VECSUB(v01, suf->v[suf->p[j]->p.v[1][0]], suf->v[suf->p[j]->p.v[0][0]]);
			for(k = 2; k < suf->p[j]->p.n; k++){
				avec3_t v, norm, vp;
				VECSUB(v, suf->v[suf->p[j]->p.v[k][0]], suf->v[suf->p[j]->p.v[0][0]]);
				VECVP(norm, v, v01);
				VECNORMIN(norm);
				if(k != 2 && (VECVP(vp, n, norm), VECSLEN(vp) != 0.)){
/*					assert(0);
					exit(0);*/
				}
				else
					VECCPY(n, norm);
			}
		}
		init = 1;
	} while(0);
#else
	if(!sufflap){
		sufflap = &suf_fly2flap;
	}
#endif

/*	if(!vft->sufbase){
		double pos[3];
		GLubyte col[4] = {255,255,0,255};
		gldPseudoSphere(pos, fly_radius, col);
	}
	else*/{
		avec3_t *points = pt->vft == &valkie_s ? valkie_points : fly_points;
		static const double normal[3] = {0., 1., 0.};
		double scale = FLY_SCALE;
		double x;
		int i, lod = pixels < 20. ? 0 : 1;
/*		static const GLfloat rotaxis[16] = {
			0,0,-1,0,
			-1,0,0,0,
			0,1,0,0,
			0,0,0,1,
		};*/
		static const GLfloat rotaxis[16] = {
			0,0,1,0,
			1,0,0,0,
			0,1,0,0,
			0,0,0,1,
		};
		static const GLfloat rotmqo[16] = {
			0,0,-1,0,
			0,1,0,0,
			1,0,0,0,
			0,0,0,1,
		};
		static const avec3_t flapjoint = {-60, 20, 0}, flapaxis = {-100 - -60, 20 - 20, -30 - -40};
		static const avec3_t elevjoint = {0, 20, 110};
		double pyr[3];
/*		glBegin(GL_POINTS);
		glVertex3dv(pt->pos);
		glEnd();*/
		amat4_t mat;

		glPushMatrix();
#if FLY_QUAT
		{
			amat4_t mat;
			tankrot(mat, pt);
/*			MAT3TO4(mat, pt->rot);*/
			glMultMatrixd(mat);
		}

#else
		glTranslated(pt->pos[0], pt->pos[1], pt->pos[2]);
		glRotated(deg_per_rad * pt->pyr[1], 0., -1., 0.);
		glRotated(deg_per_rad * pt->pyr[0], -1.,  0., 0.);
		glRotated(deg_per_rad * pt->pyr[2], 0.,  0., -1.);
#endif

		if(vft != &valkie_s && lod) for(i = 0; i < MIN(2, p->missiles); i++){
			glPushMatrix();
			gldTranslate3dv(fly_hardpoint[i]);
			gldScaled(.000005);
			DrawSUF(gsuf_ssm, SUF_ATR, &g_gldcache);
			glPopMatrix();
		}

		if(vft != &valkie_s && lod) for(i = 0; i < 3; i++){
			double scale = .001 / 280;
			glPushMatrix();
			gldTranslate3dv(points[i]);
			gldScaled(scale);
			DrawSUF(&suf_fly2gear, SUF_ATR, &g_gldcache);
			glPopMatrix();
		}

		if(vft == &valkie_s){
			struct valkie *p = (struct valkie*)pt;
#if 1
			const char **names = valkie_bonenames;
			double rot[numof(valkie_bonenames)][7];
			ysdnmv_t v, *dnmv;
			glPushMatrix();
/*			glMultMatrixf(rotmqo);*/

			dnmv = valkie_boneset(p, rot, &v, 0);
			v.bonenames = names;
			v.bonerot = rot;
			v.bones = numof(valkie_bonenames);

			glRotated(180, 0, 1, 0);
			glScaled(-.001, .001, .001);
/*			glTranslated(0, p->batphase * -7, 0.);
			glTranslated(0, 0, 5. * p->wing[0] / (M_PI / 4.));*/
			glPushAttrib(GL_POLYGON_BIT);
			glFrontFace(GL_CW);
/*			DrawYSDNM(dnm, names, rot, numof(valkie_bonenames), skipnames, numof(skipnames) - (p->bat ? 0 : 2));*/
/*			DrawYSDNM_V(dnm, &v);*/
			DrawYSDNM_V(dnm, dnmv);
			TransYSDNM_V(dnm, dnmv, draw_arms, p);
/*			TransYSDNM(dnm, names, rot, numof(valkie_bonenames), NULL, 0, draw_arms, p);*/
			glPopAttrib();
			glPopMatrix();
#else
			glPushMatrix();
			glScaled(-.5,.5,-.5);
			DecalDrawSUF(vft->sufbase, SUF_ATR, &g_gldcache, NULL, NULL/*p->sd*/, &fly_s);
			glPushMatrix();
			glRotated(deg_per_rad * ((valkie_t*)p)->leg[0], 1., 0., 0.);
			DrawSUF(sufleg, SUF_ATR, &g_gldcache);
			glPopMatrix();
			glScaled(-1,1,1);
			glPushAttrib(GL_POLYGON_BIT);
			glFrontFace(GL_CW);
			DecalDrawSUF(vft->sufbase, SUF_ATR, &g_gldcache, NULL, NULL/*p->sd*/, &fly_s);
			glPushMatrix();
			glRotated(deg_per_rad * ((valkie_t*)p)->leg[1], 1., 0., 0.);
			DrawSUF(sufleg, SUF_ATR, &g_gldcache);
			glPopMatrix();
			glPopAttrib();
			glPopMatrix();
#endif
		}
		else if(1){
			glScaled(scale, scale, scale);
			glGetDoublev(GL_MODELVIEW_MATRIX, mat);
			glPushMatrix();
#if 0
/*			glPushAttrib(GL_POLYGON_BIT);
			glDisable(GL_CULL_FACE);*/
			glRotated(180, 0, 1, 0);
			gldScaled(.001 / scale);
			DrawYSDNM(dnm);
/*			glPopAttrib();*/
#else
			glMultMatrixf(rotmqo);
			DecalDrawSUF(vft->sufbase, SUF_ATR, &g_gldcache, NULL, NULL/*p->sd*/, &fly_s);
			glScaled(1,1,-1);
			glPushAttrib(GL_POLYGON_BIT);
			glFrontFace(GL_CW);
			DecalDrawSUF(vft->sufbase, SUF_ATR, &g_gldcache, NULL, NULL/*p->sd*/, &fly_s);
			glPopAttrib();
#endif
			glPopMatrix();
		}
		else{
			DecalDrawSUF(lod ? vft->sufbase : &suf_fly1, SUF_ATR, &g_gldcache, NULL, p->sd, &fly_s);
		}


		if(vft == &fly_s && lod){
			glPushMatrix();
			glTranslated(flapjoint[0], flapjoint[1], flapjoint[2]);
			glRotated(deg_per_rad * p->aileron[0], flapaxis[0], flapaxis[1], flapaxis[2]);
			glTranslated(-flapjoint[0], -flapjoint[1], -flapjoint[2]);
			DrawSUF(sufflap, SUF_ATR, &g_gldcache, NULL);
			glPopMatrix();

			glPushMatrix();
			glTranslated(-flapjoint[0], flapjoint[1], flapjoint[2]);
			glRotated(deg_per_rad * -p->aileron[1], -flapaxis[0], flapaxis[1], flapaxis[2]);
			glTranslated(- -flapjoint[0], -flapjoint[1], -flapjoint[2]);
			glScaled(-1., 1., 1.);
			glFrontFace(GL_CW);
			DrawSUF(sufflap, SUF_ATR, &g_gldcache, NULL);
			glFrontFace(GL_CCW);
			glPopMatrix();

			/* elevator */
			glPushMatrix();
			gldTranslate3dv(elevjoint);
			glRotated(deg_per_rad * p->elevator, -1., 0., 0.);
			glTranslated(-elevjoint[0], -elevjoint[1], -elevjoint[2]);
			DrawSUF(&suf_fly2elev, SUF_ATR, &g_gldcache, NULL);
			glPopMatrix();
		}

		glPopMatrix();

/*		if(0 < wd->light[1]){
			static const double normal[3] = {0., 1., 0.};
			ShadowSUF(&suf_fly1, wd->light, normal, pt->pos, pt->pyr, scale, NULL);
		}*/
	/*
		glPushMatrix();
		VECCPY(pyr, pt->pyr);
		pyr[0] += M_PI / 2;
		pyr[1] += pt->turrety;
		glMultMatrixd(rotaxis);
		ShadowSUF(train_s.sufbase, wd->light, normal, pt->pos, pt->pyr, scale);
		VECCPY(pyr, pt->pyr);
		pyr[1] += pt->turrety;
		ShadowSUF(train_s.sufturret, wd->light, normal, pt->pos, pyr, tscale);
		glPopMatrix();*/
	}
}

static void fly_draw(entity_t *pt, wardraw_t *wd){
	static int init = 0;
	static suf_t *sufflap;
	double pixels;
	fly_t *p = (fly_t*)pt;
	struct entity_private_static *vft = (struct entity_private_static *)pt->vft;
	int i;

	g_smoke_wd = wd;

	for(i = 0; i < numof(cnl_vapor); i++){
		cnl_vapor[i].col = COLOR32MUL(cnl0_vapor[i].col, wd->ambient);
	}

	if(!pt->active)
		return;

	/* cull object */
	if(fly_cull(pt, wd, &pixels))
		return;
	wd->lightdraws++;


	if(!vft->sufbase) do{
		int i, j, k;
		double best = 0., t, t2;
		FILE *fp;
		suf_t *suf;
		timemeas_t tm;
#if 0
/*		vft->sufbase = LoadYSSUF("vf25_coarse.srf");*/
		vft->sufbase = LoadYSSUF("vf25_coll.srf");
		dnm = LoadYSDNM(CvarGetString("valike_dnm")/*"vf25f_coarse.dnm"*/);
#else
		TimeMeasStart(&tm);
		vft->sufbase = /*gsuf_fly /*//*&suf_fly2*//* lzuc(lzw_fly2, sizeof lzw_fly2, NULL)*/lzuc(lzw_fly3, sizeof lzw_fly3, NULL);
		if(!vft->sufbase)
			break;
		t = TimeMeasLap(&tm);
		RelocateSUF(vft->sufbase);
		t2 = TimeMeasLap(&tm);
		printf("relocf: %lg - %lg = %lg\n", t2, t, t2 - t);
#endif
		sufflap = &suf_fly2flap;
		for(i = 0; i < vft->sufbase->nv; i++){
			double sd = VECSLEN(vft->sufbase->v[i]);
			if(best < sd)
				best = sd;
		}
		vft->hitradius = FLY_SCALE * sqrt(best) * 1.2;
		suf = vft->sufbase;
		for(j = 0; j < suf->np; j++){
			avec3_t v01, n;

			VECSUB(v01, suf->v[suf->p[j]->p.v[1][0]], suf->v[suf->p[j]->p.v[0][0]]);
			for(k = 2; k < suf->p[j]->p.n; k++){
				avec3_t v, norm, vp;
				VECSUB(v, suf->v[suf->p[j]->p.v[k][0]], suf->v[suf->p[j]->p.v[0][0]]);
				VECVP(norm, v, v01);
				VECNORMIN(norm);
				if(k != 2 && (VECVP(vp, n, norm), VECSLEN(vp) != 0.)){
/*					assert(0);
					exit(0);*/
				}
				else
					VECCPY(n, norm);
			}
		}
		init = 1;
	} while(0);

	fly_draw_int(pt, wd, pixels);
}

static void valkie_draw(entity_t *pt, wardraw_t *wd){
	static int init = 0;
	static suf_t *sufflap;
	double pixels;
	valkie_t *p = (valkie_t*)pt;
	struct entity_private_static *vft = (struct entity_private_static *)pt->vft;
	int i;

	g_smoke_wd = wd;

	for(i = 0; i < numof(cnl_vapor); i++){
		cnl_vapor[i].col = COLOR32MUL(cnl0_vapor[i].col, wd->ambient);
	}

	if(!pt->active)
		return;

	/* cull object */
	if(fly_cull(pt, wd, &pixels))
		return;
	wd->lightdraws++;


	if(!init) do{
		int i, j, k;
		double best = 0., t, t2;
		FILE *fp;
		suf_t *suf;
		timemeas_t tm;
		TimeMeasStart(&tm);
#if 1
		dnm = LoadYSDNM(CvarGetString("valkie_dnm")/*"vf25f_coarse.dnm"*/);
		t = TimeMeasLap(&tm);
		printf("LoadYSDNM: %lg\n", t);
#else
		vft->sufbase = lzuc(lzw_valkie, sizeof lzw_valkie, NULL);
		if(!vft->sufbase)
			break;
		t = TimeMeasLap(&tm);
		RelocateSUF(vft->sufbase);
		t2 = TimeMeasLap(&tm);
		printf("relocf: %lg - %lg = %lg\n", t2, t, t2 - t);
		sufflap = &suf_fly2flap;
		for(i = 0; i < vft->sufbase->nv; i++){
			double sd = VECSLEN(vft->sufbase->v[i]);
			if(best < sd)
				best = sd;
		}
/*		vft->hitradius = FLY_SCALE * sqrt(best) * 1.2;*/
		suf = vft->sufbase;
		for(j = 0; j < suf->np; j++){
			avec3_t v01, n;

			VECSUB(v01, suf->v[suf->p[j]->p.v[1][0]], suf->v[suf->p[j]->p.v[0][0]]);
			for(k = 2; k < suf->p[j]->p.n; k++){
				avec3_t v, norm, vp;
				VECSUB(v, suf->v[suf->p[j]->p.v[k][0]], suf->v[suf->p[j]->p.v[0][0]]);
				VECVP(norm, v, v01);
				VECNORMIN(norm);
				if(k != 2 && (VECVP(vp, n, norm), VECSLEN(vp) != 0.)){
/*					assert(0);
					exit(0);*/
				}
				else
					VECCPY(n, norm);
			}
		}
#endif
		init = 1;
	} while(0);

	if(!sufleg){
		sufleg = LZUC(lzw_valkie_leg);
	}

	fly_draw_int(pt, wd, pixels);
}

void drawnavlight(const double (*pos)[3], const double (*org)[3], double rad, const GLubyte (*col)[4], const double (*irot)[16], wardraw_t *wd){
	double pixels, sf;
	if(glcullFrustum(pos, rad, wd->pgc))
		return;
	pixels = rad * fabs(glcullScale(pos, wd->pgc));
	sf = 1. / sqrt(.1 * pixels);
	pixels *= sf;
	if(pixels < 4.){
/*		glBegin(GL_POINTS);
		glColor4ubv(*col);
		glVertex3dv(*pos);
		glEnd();*/
		glColor4ubv(*col);
/*		gldPoint(pos, pixels / wd->g);*/
		glPointSize(pixels * .5);
		glBegin(GL_POINTS);
		glVertex3dv(pos);
		glEnd();
	}
	else
		gldSpriteGlow(*pos, rad * sf, *col, *irot);
}

void drawmuzzleflash(const double (*pos)[3], const double (*org)[3], double rad, const double (*irot)[16]){
	double (*cuts)[2];
	struct random_sequence rs;
	int i;
	cuts = CircleCuts(10);
	{
		double posum = (*pos)[0] + (*pos)[1] + (*pos)[2];
		init_rseq(&rs, *(long*)&posum);
	}
	glPushMatrix();
	glTranslated((*pos)[0], (*pos)[1], (*pos)[2]);
	glMultMatrixd(*irot);
	glScaled(rad, rad, rad);
	glBegin(GL_TRIANGLE_FAN);
	glColor4ub(255,255,31,255);
	glVertex3d(0., 0., 0.);
	glColor4ub(255,0,0,0);
	{
		double first;
		first = drseq(&rs) + .5;
		glVertex3d(first * cuts[0][1], first * cuts[0][0], 0.);
		for(i = 1; i < 10; i++){
			int k = i % 10;
			double r;
			r = drseq(&rs) + .5;
			glVertex3d(r * cuts[k][1], r * cuts[k][0], 0.);
		}
		glVertex3d(first * cuts[0][1], first * cuts[0][0], 0.);
	}
	glEnd();
	glPopMatrix();
}

static void draw_afterburner(const char *name, struct afterburner_hint *hint){
	double (*cuts)[2];
	int j;
	double x;
	if(hint->ab && (!strcmp(name, "ABbaseL") || !strcmp(name, "ABbaseR") /*!strcmp(name, "AB1L_BT") || !strcmp(name, "AB1L") || !strcmp(name, "AB1R_BT") || !strcmp(name, "AB1R")*/)){
		avec3_t pos[2] = {{-1.2, 0, -8.}, {1.2, 0, -8.}};
		struct random_sequence rs;
		GLubyte firstcol;
		avec3_t firstpos;
		cuts = CircleCuts(8);
		init_rseq(&rs, *(long*)&hint->gametime ^ (long)name);
		glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		glEnable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glPushMatrix();
#if 0
		gldTranslate3dv(!strcmp(name, "ABbaseL") /*!strcmp(name, "AB1L_BT") || !strcmp(name, "AB1L")*/ ? pos[0] : pos[1]);
#endif
		glScaled(-.5, -.5, -5. * (.5 + hint->thrust * 1));
		glBegin(GL_QUAD_STRIP);
		for(j = 0; j <= 8; j++){
			int j1 = j % 8;
			x = (drseq(&rs) - .5) * .25;
			if(j == 0){
				glColor4ub(255,127,0, firstcol = rseq(&rs) % 128 + 128);
			}
			else if(j == 8){
				glColor4ub(255,127,0, firstcol);
			}
			else{
				glColor4ub(255,127,0,rseq(&rs) % 128 + 128);
			}
			glVertex3d(cuts[j1][1], cuts[j1][0], 0);
			glColor4ub(255,0,0,0);
			if(j == 0)
				glVertex3d(firstpos[0] = x + 1.5 * cuts[j1][1], firstpos[1] = (drseq(&rs) - .5) * .25 + 1.5 * cuts[j1][0], firstpos[2] = 1);
			else if(j == 8)
				glVertex3dv(firstpos);
			else
				glVertex3d(x + 1.5 * cuts[j1][1], (drseq(&rs) - .5) * .25 + 1.5 * cuts[j1][0], 1);
		}
		glEnd();
		glBegin(GL_TRIANGLE_FAN);
		glColor4ub(255,127,0,rseq(&rs) % 64 + 128);
		x = (drseq(&rs) - .5) * .25;
		glVertex3d(x, (drseq(&rs) - .5) * .25, 1);
		for(j = 0; j <= 8; j++){
			int j1 = j % 8;
			glColor4ub(0,127,255,rseq(&rs) % 128 + 128);
			glVertex3d(cuts[j1][1], cuts[j1][0], 0);
		}
		glEnd();
		glPopMatrix();
		glPopAttrib();
/*		glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &j);
		printf("mv %d", j);
		glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH, &j);
		printf("/%d\n", j);*/
	}

	/* muzzle flash */
	if(hint->muzzle && !strcmp(name, "GunBase")){
		struct random_sequence rs;
		static const amat4_t mat = {
			-1,0,0,0,
			0,-1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};
		avec3_t pos = {.13, -.37, 3.};
		init_rseq(&rs, *(long*)&hint->gametime);
		drawmuzzleflash4(pos, mat, 7., mat, &rs, avec3_000);
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

static void fly_drawtra(entity_t *pt, wardraw_t *wd){
	const GLubyte red[4] = {255,31,31,255}, white[4] = {255,255,255,255};
	static const avec3_t
	flylights[3] = {
		{-160. * FLY_SCALE, 20. * FLY_SCALE, 10. * FLY_SCALE},
		{160. * FLY_SCALE, 20. * FLY_SCALE, 10. * FLY_SCALE},
		{0. * FLY_SCALE, 20. * FLY_SCALE, -130 * FLY_SCALE},
	};
	avec3_t valkielights[3] = {
		{-440. * .5, 50. * .5, 210. * .5},
		{440. * .5, 50. * .5, 210. * .5},
		{0., -12. * .5 * FLY_SCALE, -255. * .5 * FLY_SCALE},
	};
	avec3_t *navlights = pt->vft == &fly_s ? flylights : valkielights;
	avec3_t *wingtips = pt->vft == &fly_s ? flywingtips : valkiewingtips;
	static const GLubyte colors[3][4] = {
		{255,0,0,255},
		{0,255,0,255},
		{255,255,255,255},
	};
	double air;
	double scale = FLY_SCALE;
	avec3_t v0, v;
	GLdouble mat[16];
	fly_t *p = (fly_t*)pt;
	struct random_sequence rs;
	int i;
	if(!pt->active)
		return;

	if(fly_cull(pt, wd, NULL))
		return;

	init_rseq(&rs, (long)(wd->gametime * 1e5));

#if 0 /* up */
	{
		avec3_t nh;
		nh[0] = sin(pt->pyr[0]) * sin(pt->pyr[1]);
		nh[1] = cos(pt->pyr[0]);
		nh[2] = -sin(pt->pyr[0]) * cos(pt->pyr[1]);
		VECSCALEIN(nh, .01);
		VECADDIN(nh, pt->pos);
		glColor4ub(255,0,127,255);
		glBegin(GL_LINES);
		glVertex3dv(pt->pos);
		glVertex3dv(nh);
		glEnd();
	}
#endif

/*	{
		int i;
		amat4_t mat;
		tankrot(mat, pt);
		glColor3ub(255,255,255);
		glBegin(GL_LINES);
		for(i = 0; i < 5; i++){
			avec3_t v, f;
			MAT4VP3(v, mat, wings0[i]);
			glVertex3dv(v);
			VECSCALE(f, p->force[i], .1);
			VECADDIN(f, v);
			glVertex3dv(f);
		}
		glEnd();
	}*/

/*	glPushMatrix();
	glLoadIdentity();
	glTranslated(pt->pos[0], pt->pos[1], pt->pos[2]);
	glScaled(scale, scale, scale);
	glRotated(deg_per_rad * pt->pyr[1], 0., -1., 0.);
	glRotated(deg_per_rad * pt->pyr[0], -1.,  0., 0.);
	glGetDoublev(GL_MODELVIEW_MATRIX, mat);
	glPopMatrix();*/
#if FLY_QUAT
	tankrot(&mat, pt);
#else
	{
		struct smat4{double a[16];};
		amat4_t mtra, mrot;
/*		MAT4IDENTITY(mtra);*/
		*(struct smat4*)mtra = *(struct smat4*)mat4identity;
		MAT4TRANSLATE(mtra, pt->pos[0], pt->pos[1], pt->pos[2]);
		pyrmat(pt->pyr, &mrot);
		MAT4SCALE(mrot, scale, scale, scale);
		MAT4MP(mat, mtra, mrot);
	}
#endif

	air = wd->w->vft->atmospheric_pressure(wd->w, pt->pos);

	if(air && SONICSPEED * SONICSPEED < VECSLEN(pt->velo)){
		const double (*cuts)[2];
		int i, k, lumi = rseq(&rs) % ((int)(127 * (1. - 1. / (VECSLEN(pt->velo) / SONICSPEED / SONICSPEED))) + 1);
		COLOR32 cc = COLOR32RGBA(255, 255, 255, lumi), oc = COLOR32RGBA(255, 255, 255, 0);

		cuts = CircleCuts(8);
		glPushMatrix();
		glMultMatrixd(mat);
		glTranslated(0., 20. * FLY_SCALE, -160 * FLY_SCALE);
		glScaled(.01, .01, .01 * (VECLEN(pt->velo) / SONICSPEED - 1.));
		glBegin(GL_TRIANGLE_FAN);
		gldColor32(COLOR32MUL(cc, wd->ambient));
		glVertex3i(0, 0, 0);
		gldColor32(COLOR32MUL(oc, wd->ambient));
		for(i = 0; i <= 8; i++){
			k = i % 8;
			glVertex3d(cuts[k][0], cuts[k][1], 1.);
		}
		glEnd();
		glPopMatrix();
	}

#if 0
	if(p->afterburner) for(i = 0; i < (pt->vft == &fly_s ? 1 : 2); i++){
		double (*cuts)[2];
		int j;
		struct random_sequence rs;
		double x;
		init_rseq(&rs, *(long*)&wd->gametime);
		cuts = CircleCuts(8);
		glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		glEnable(GL_CULL_FACE);
		glPushMatrix();
		glMultMatrixd(mat);
		if(pt->vft == &fly_s)
			glTranslated(pt->vft == &fly_s ? 0. : (i * 105. * 2. - 105.) * .5 * FLY_SCALE, pt->vft == &fly_s ? FLY_SCALE * 20. : FLY_SCALE * .5 * -5., FLY_SCALE * (pt->vft == &fly_s ? 160 : 420. * .5));
		else
			glTranslated((i * 1.2 * 2. - 1.2) * .001, -.0002, .008);
		glScaled(.0005, .0005, .005);
		glBegin(GL_TRIANGLE_FAN);
		glColor4ub(255,127,0,rseq(&rs) % 64 + 128);
		x = (drseq(&rs) - .5) * .25;
		glVertex3d(x, (drseq(&rs) - .5) * .25, 1);
		for(j = 0; j <= 8; j++){
			int j1 = j % 8;
			glColor4ub(0,127,255,rseq(&rs) % 128 + 128);
			glVertex3d(cuts[j1][1], cuts[j1][0], 0);
		}
		glEnd();
		glPopMatrix();
		glPopAttrib();
	}
#endif

	if(pt->vft == &valkie_s){
		valkie_t *p = (valkie_t*)pt;
		double rot[numof(valkie_bonenames)][7];
		struct afterburner_hint hint;
		ysdnmv_t v, *dnmv;
		dnmv = valkie_boneset(p, rot, &v, 1);
		hint.ab = p->afterburner;
		hint.muzzle = FLY_RELOADTIME - .03 < p->cooldown || p->muzzle;
		hint.thrust = p->throttle;
		hint.gametime = wd->gametime;
		if(1 || p->navlight){
			glPushMatrix();
			glLoadIdentity();
			glRotated(180, 0, 1., 0);
			glScaled(-.001, .001, .001);
/*			glTranslated(0, p->batphase * -7, 0.);
			glTranslated(0, 0, 5. * p->wing[0] / (M_PI / 4.));*/
			TransYSDNM_V(dnm, dnmv, find_wingtips, &hint);
/*			TransYSDNM(dnm, valkie_bonenames, rot, numof(valkie_bonenames), NULL, 0, find_wingtips, &hint);*/
			glPopMatrix();
			for(i = 0; i < 3; i++)
				VECCPY(valkielights[i], hint.wingtips[i]);
		}
		if(hint.muzzle || p->afterburner){
/*			v.fcla = 1 << 9;
			v.cla[9] = p->batphase;*/
			v.bonenames = valkie_bonenames;
			v.bonerot = rot;
			v.bones = numof(valkie_bonenames);
			v.skipnames = NULL;
			v.skips = 0;
			glPushMatrix();
			glMultMatrixd(mat);
			glRotated(180, 0, 1., 0);
			glScaled(-.001, .001, .001);
/*			glTranslated(0, p->batphase * -7, 0.);
			glTranslated(0, 0, 5. * p->wing[0] / (M_PI / 4.));*/
			TransYSDNM_V(dnm, dnmv, draw_afterburner, &hint);
			glPopMatrix();
			p->muzzle = 0;
		}
	}


/*	{
		GLubyte col[4] = {255,127,127,255};
		glBegin(GL_LINES);
		glColor4ub(255,255,255,255);
		glVertex3dv(pt->pos);
		glVertex3dv(p->sight);
		glEnd();
		gldSpriteGlow(p->sight, .001, col, wd->irot);
	}*/

/*	glPushAttrib(GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(0);*/
	if(p->navlight){
		gldBegin();
		VECCPY(v0, navlights[0]);
/*		VECSCALE(v0, navlights[0], FLY_SCALE);*/
		mat4vp3(v, mat, v0);
	/*	gldSpriteGlow(v, .001, red, wd->irot);*/
		drawnavlight(&v, &wd->view, .0005, colors[0], &wd->irot, wd);
		VECCPY(v0, navlights[1]);
/*		VECSCALE(v0, navlights[1], FLY_SCALE);*/
		mat4vp3(v, mat, v0);
		drawnavlight(&v, &wd->view, .0005, colors[1], &wd->irot, wd);
		if(fmod(wd->gametime, 1.) < .1){
			VECCPY(v0, navlights[2]);
/*			VECSCALE(v0, navlights[2], FLY_SCALE);*/
			mat4vp3(v, mat, v0);
			drawnavlight(&v, &wd->view, .0005, &white, &wd->irot, wd);
		}
		gldEnd();
	}

	if(air && .1 < -VECSP(pt->velo, &mat[8])){
		avec3_t vh;
		VECNORM(vh, pt->velo);
		for(i = 0; i < 2; i++){
			struct random_sequence rs;
			struct gldBeamsData bd;
			avec3_t pos;
			int mag = 1 + 255 * (1. - .1 * .1 / VECSLEN(pt->velo));
			mat4vp3(v, mat, navlights[i]);
			init_rseq(&rs, (unsigned long)((v[0] * v[1] * 1e6)));
			bd.cc = bd.solid = 0;
			VECSADD(v, vh, -.0005);
			gldBeams(&bd, wd->view, v, .0001, COLOR32RGBA(255,255,255,0));
			VECSADD(v, vh, -.0015);
			gldBeams(&bd, wd->view, v, .0003, COLOR32RGBA(255,255,255, rseq(&rs) % mag));
			VECSADD(v, vh, -.0015);
			gldBeams(&bd, wd->view, v, .0002, COLOR32RGBA(255,255,255,rseq(&rs) % mag));
			VECSADD(v, vh, -.0015);
			gldBeams(&bd, wd->view, v, .00002, COLOR32RGBA(255,255,255,rseq(&rs) % mag));
		}
	}

	if(pt->vft != &valkie_s){
		fly_t *fly = ((fly_t*)pt);
		int i = 0;
		if(fly->muzzle){
			amat4_t ir, mat2;
/*			pyrmat(pt->pyr, &mat);*/
			do{
				avec3_t pos, gunpos;
	/*			MAT4TRANSLATE(mat, fly_guns[i][0], fly_guns[i][1], fly_guns[i][2]);*/
	/*			pyrimat(pt->pyr, &ir);
				MAT4MP(mat2, mat, ir);*/
/*				MAT4VP3(gunpos, mat, fly_guns[i]);
				VECADD(pos, pt->pos, gunpos);*/
				MAT4VP3(pos, mat, fly_guns[i]);
				drawmuzzleflash(&pos, &wd->view, .0025, &wd->irot);
			} while(!i++);
			fly->muzzle = 0;
		}
	}
/*	glPopAttrib();*/

#if 0
	{
		double (*cuts)[2];
		double f = ((struct entity_private_static*)pt->vft)->hitradius;
		int i;

		cuts = CircleCuts(32);
		glPushMatrix();
		glTranslated(pt->pos[0], pt->pos[1], pt->pos[2]);
		glMultMatrixd(wd->irot);
		glColor3ub(255,255,127);
		glBegin(GL_LINE_LOOP);
		for(i = 0; i < 32; i++)
			glVertex3d(cuts[i][0] * f, cuts[i][1] * f, 0.);
		glEnd();
		glPopMatrix();
	}
#endif
}

static int fly_takedamage(entity_t *pt, double damage, warf_t *w){
	struct tent3d_line_list *tell = w->tell;
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
	playWave3D("hit.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .1, w->realtime);
	if(0 < pt->health && pt->health - damage <= 0){
		int i;
		ret = 0;
/*		effectDeath(w, pt);*/
		for(i = 0; i < 32; i++){
			double pos[3], velo[3];
			velo[0] = drseq(&w->rs) - .5;
			velo[1] = drseq(&w->rs) - .5;
			velo[2] = drseq(&w->rs) - .5;
			VECNORMIN(velo);
			VECCPY(pos, pt->pos);
			VECSCALEIN(velo, .1);
			VECSADD(pos, velo, .1);
			AddTeline3D(w->tell, pos, velo, .005, NULL, NULL, w->gravity, COLOR32RGBA(255, 31, 0, 255), TEL3_HEADFORWARD | TEL3_THICK | TEL3_FADEEND | TEL3_REFLECT, 1.5 + drseq(&w->rs));
		}
		((fly_t*)pt)->pf = AddTefpolMovable3D(w->tepl, pt->pos, pt->velo, avec3_000, &cs_firetrail, TEP3_THICKER | TEP3_ROUGH, cs_firetrail.t);
		playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
	}
	pt->health -= damage;
	if(pt->health < -1000.){
		pt->active = 0.;
	}
	return ret;
}

static void fly_gib_draw(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	int i = (int)pv;
	suf_t *suf;
	double scale;
	if(i < fly_s.sufbase->np){
		suf = fly_s.sufbase;
		scale = FLY_SCALE;
		gib_draw(pl, suf, scale, i);
	}
}

static int fly_flying(const entity_t *pt){
	return pt->active && .05 * .05 < VECSLEN(pt->velo) && .001 < pt->pos[1];
}

static void fly_bullethole(entity_t *pt, sufindex si, double rad, const double (*ppos)[3], const double (*ppyr)[3]){
	fly_t *p = (fly_t*)pt;
	bhole_t *bh;
	bh = bhole_alloc(&(bhole_t*)p->sd->p[si], &p->frei);
	if(bh){
		bh->rad = rad;
		VECCPY(bh->pos, *ppos);
		VECCPY(bh->pyr, *ppyr);
	}
/*	AddSUFDecal(((fly_t*)pt)->sd, si, pa);*/
}

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


static void flare_anim(entity_t *pt, warf_t *w, double dt){
	flare_t *const flare = (flare_t*)pt;
	if(pt->pos[1] < 0. || ((flare_t*)pt)->ttl < dt){
		pt->active = 0;
		return;
	}
	((flare_t*)pt)->ttl -= dt;
	if(flare->pf)
		MoveTefpol3D(flare->pf, pt->pos, NULL, cs_firetrail.t, 0);
	VECSADD(pt->velo, w->gravity, dt);
	VECSADD(pt->pos, pt->velo, dt);
}

static void flare_drawtra(entity_t *pt, wardraw_t *wd){
	GLubyte white[4] = {255,255,255,255}, red[4] = {255,127,127,127};
	struct random_sequence rs;
	double len = ((flare_t*)pt)->ttl < 1. ? ((flare_t*)pt)->ttl : 1.;
	init_rseq(&rs, *(long*)&wd->gametime | (long)pt);
	red[3] = 127 + rseq(&rs) % 128;
	gldSpriteGlow(pt->pos, len * (VECSDIST(wd->view, pt->pos) < .1 * .1 ? VECDIST(wd->view, pt->pos) / .1 * .05 : .05), red, wd->irot);
	if(VECSDIST(wd->view, pt->pos) < 1. * 1.)
		gldSpriteGlow(pt->pos, .001, white, wd->irot);
	if(pt->pos[1] < .1){
		double pos[3];
		amat4_t mat = { /* pseudo rotation; it simply maps xy plane to xz */
			1,0,0,0,
			0,0,-1,0,
			0,1,0,0,
			0,0,0,1
		};
		VECCPY(pos, pt->pos);
		pos[1] = 0.;
		red[3] = red[3] * len * (.1 - pt->pos[1]) / .1;
		gldSpriteGlow(pos, .10 * pt->pos[1] * pt->pos[1] / .1 / .1, red, mat);
	}
}

static int flare_flying(const entity_t *pt){
	if(.01 < pt->pos[1])
		return 1;
	else return 0;
}

static void fly_drawHUD(entity_t *pt, warf_t *wf, wardraw_t *wd, const double irot[16], void (*gdraw)(void)){
	char buf[128];
/*	timemeas_t tm;*/
	fly_t *p = (fly_t*)pt;
/*	glColor4ub(0,255,0,255);*/

/*	TimeMeasStart(&tm);*/

	base_drawHUD(pt, wf, wd, gdraw);

	glLoadIdentity();

	glDisable(GL_LINE_SMOOTH);

	if(!wf->pl->chasecamera && pt->enemy){
		int i;
		double pos[3];
		double epos[3];
		amat4_t rot;

		glPushMatrix();
		MAT4TRANSPOSE(rot, irot);
		glLoadMatrixd(rot);
		VECSUB(pos, pt->enemy->pos, pt->pos);
		VECNORMIN(pos);
		glTranslated(pos[0], pos[1], pos[2]);
		glMultMatrixd(irot);
		{
			double dist, dp[3], dv[3];
			VECSUB(dp, pt->enemy->pos, pt->pos);
			dist = VECLEN(dp);
			if(dist < 1.)
				sprintf(buf, "%.4g m", dist * 1000.);
			else
				sprintf(buf, "%.4g km", dist);
			glRasterPos3d(.01, .02, 0.);
			putstring(buf);
			VECSUB(dv, pt->enemy->velo, pt->velo);
			dist = VECSP(dv, dp) / dist;
			if(dist < 1.)
				sprintf(buf, "%.4g m/s", dist * 1000.);
			else
				sprintf(buf, "%.4g km/s", dist);
			glRasterPos3d(.01, -.03, 0.);
			putstring(buf);
		}
		glBegin(GL_LINE_LOOP);
		glVertex3d(.05, .05, 0.);
		glVertex3d(-.05, .05, 0.);
		glVertex3d(-.05, -.05, 0.);
		glVertex3d(.05, -.05, 0.);
		glEnd();
		glBegin(GL_LINES);
		glVertex3d(.04, .0, 0.);
		glVertex3d(.02, .0, 0.);
		glVertex3d(-.04, .0, 0.);
		glVertex3d(-.02, .0, 0.);
		glVertex3d(.0, .04, 0.);
		glVertex3d(.0, .02, 0.);
		glVertex3d(.0, -.04, 0.);
		glVertex3d(.0, -.02, 0.);
		glEnd();
		glPopMatrix();

	}
	{
		GLint vp[4];
		int w, h, m, mi;
		double left, bottom;
		double (*cuts)[2];
		double velo, d;
		int i;
		glGetIntegerv(GL_VIEWPORT, vp);
		w = vp[2], h = vp[3];
		m = w < h ? h : w;
		mi = w < h ? w : h;
		left = -(double)w / m;
		bottom = -(double)h / m;

/*		air = wf->vft->atmospheric_pressure(wf, &pt->pos)/*exp(-pt->pos[1] / 10.)*/;
/*		glRasterPos3d(left, bottom + 50. / m, -1.);
		gldprintf("atmospheric pressure: %lg height: %lg", exp(-pt->pos[1] / 10.), pt->pos[1] * 1e3);*/
/*		glRasterPos3d(left, bottom + 70. / m, -1.);
		gldprintf("throttle: %lg", ((fly_t*)pt)->throttle);*/

		if(((fly_t*)pt)->brk){
			glRasterPos3d(left, bottom + 110. / m, -1.);
			gldprintf("BRK");
		}

		if(((fly_t*)pt)->afterburner){
			glRasterPos3d(left, bottom + 150. / m, -1.);
			gldprintf("AB");
		}

		glRasterPos3d(left, bottom + 130. / m, -1.);
		gldprintf("Missiles: %d", p->missiles);

		glPushMatrix();
		glScaled(1./*(double)mi / w*/, 1./*(double)mi / h*/, (double)m / mi);

		/* throttle */
		glBegin(GL_LINE_LOOP);
		glVertex3d(-.8, -0.7, -1.);
		glVertex3d(-.8, -0.8, -1.);
		glVertex3d(-.78, -0.8, -1.);
		glVertex3d(-.78, -0.7, -1.);
		glEnd();
		glBegin(GL_QUADS);
		glVertex3d(-.8, -0.8 + p->throttle * .1, -1.);
		glVertex3d(-.8, -0.8, -1.);
		glVertex3d(-.78, -0.8, -1.);
		glVertex3d(-.78, -0.8 + p->throttle * .1, -1.);
		glEnd();

/*		printf("flyHUD %lg\n", TimeMeasLap(&tm));*/

		/* boundary of control surfaces */
		glBegin(GL_LINE_LOOP);
		glVertex3d(-.45, -.5, -1.);
		glVertex3d(-.45, -.7, -1.);
		glVertex3d(-.60, -.7, -1.);
		glVertex3d(-.60, -.5, -1.);
		glEnd();

		glBegin(GL_LINES);

		/* aileron */
		glVertex3d(-.45, (p->aileron[0]) / M_PI * .2 - .6, -1.);
		glVertex3d(-.5, (p->aileron[0]) / M_PI * .2 - .6, -1.);
		glVertex3d(-.55, (p->aileron[1]) / M_PI * .2 - .6, -1.);
		glVertex3d(-.6, (p->aileron[1]) / M_PI * .2 - .6, -1.);

		/* rudder */
		glVertex3d((p->rudder) * 3. / M_PI * .1 - .525, -.7, -1.);
		glVertex3d((p->rudder) * 3. / M_PI * .1 - .525, -.8, -1.);

		/* elevator */
		glVertex3d(-.5, (-p->elevator) / M_PI * .2 - .6, -1.);
		glVertex3d(-.55, (-p->elevator) / M_PI * .2 - .6, -1.);

		glEnd();

/*		printf("flyHUD %lg\n", TimeMeasLap(&tm));*/

		if(!wf->pl->chasecamera){
			int i;
			double (*cuts)[2];
			avec3_t velo;
			amat4_t rot;
			MAT4TRANSPOSE(rot, wf->irot);
			VECNORM(velo, pt->velo);
			glPushMatrix();
			glMultMatrixd(rot);
			glTranslated(velo[0], velo[1], velo[2]);
			glMultMatrixd(irot);
			cuts = CircleCuts(16);
			glBegin(GL_LINE_LOOP);
			for(i = 0; i < 16; i++)
				glVertex3d(cuts[i][0] * .02, cuts[i][1] * .02, 0.);
			glEnd();
			glPopMatrix();
		}

		if(wf->pl->control != pt){
			amat4_t rot, rot2;
			avec3_t pyr;
/*			MAT4TRANSPOSE(rot, wf->irot);
			quat2imat(rot2, pt->rot);*/
			VECSCALE(pyr, wf->pl->pyr, -1);
			pyrimat(pyr, rot);
/*			glMultMatrixd(rot2);*/
			glMultMatrixd(rot);
/*			glRotated(deg_per_rad * wf->pl->pyr[2], 0., 0., 1.);
			glRotated(deg_per_rad * wf->pl->pyr[0], 1., 0., 0.);
			glRotated(deg_per_rad * wf->pl->pyr[1], 0., 1., 0.);*/
		}
		else{
/*			glMultMatrixd(wf->irot);*/
		}

/*		glPushMatrix();
		glRotated(-gunangle, 1., 0., 0.);
		glBegin(GL_LINES);
		glVertex3d(-.15, 0., -1.);
		glVertex3d(-.05, 0., -1.);
		glVertex3d( .15, 0., -1.);
		glVertex3d( .05, 0., -1.);
		glVertex3d(0., -.15, -1.);
		glVertex3d(0., -.05, -1.);
		glVertex3d(0., .15, -1.);
		glVertex3d(0., .05, -1.);
		glEnd();
		glPopMatrix();*/

/*		glRotated(deg_per_rad * pt->pyr[0], 1., 0., 0.);
		glRotated(deg_per_rad * pt->pyr[2], 0., 0., 1.);

		cuts = CircleCuts(64);

		glBegin(GL_LINES);
		glVertex3d(-.35, 0., -1.);
		glVertex3d(-.20, 0., -1.);
		glVertex3d( .20, 0., -1.);
		glVertex3d( .35, 0., -1.);
		for(i = 0; i < 16; i++){
			int k = i < 8 ? i : i - 8, sign = i < 8 ? 1 : -1;
			double y = sign * cuts[k][0];
			double z = -cuts[k][1];
			glVertex3d(-.35, y, z);
			glVertex3d(-.25, y, z);
			glVertex3d( .35, y, z);
			glVertex3d( .25, y, z);
		}
		glEnd();
*/
	}
	glPopMatrix();

/*	printf("fly_drawHUD %lg\n", TimeMeasLap(&tm));*/
}

static void fly_drawCockpit(struct entity *pt, const warf_t *w, wardraw_t *wd){
	static int init = 0;
	static suf_t *sufcockpit = NULL, *sufstick = NULL;
	static ysdnm_t *valcockpit = NULL;
	fly_t *p = (fly_t*)pt;
	double scale = .0002 /*wd->pgc->znear / wd->pgc->zfar*/;
	double sonear = scale * wd->pgc->znear;
	double wid = sonear * wd->pgc->fov * wd->pgc->width / wd->pgc->res;
	double hei = sonear * wd->pgc->fov * wd->pgc->height / wd->pgc->res;
	avec3_t seat = {0., 40. * FLY_SCALE, -130. * FLY_SCALE};
	avec3_t stick = {0., 68. * FLY_SCALE / 2., -270. * FLY_SCALE / 2.};
	amat4_t rot;
	if(w->pl->chasecamera)
		return;

	if(!init){
/*		timemeas_t tm;*/
		init = 1;
/*		TimeMeasStart(&tm);*/
		sufcockpit = lzuc(lzw_fly2cockpit, sizeof lzw_fly2cockpit, NULL);
		RelocateSUF(sufcockpit);
		sufstick = lzuc(lzw_fly2stick, sizeof lzw_fly2stick, NULL);
		RelocateSUF(sufstick);
/*		printf("reloc: %lg\n", TimeMeasLap(&tm));*/
	}

	if(pt->vft == &valkie_s && !valcockpit){
		valcockpit = LoadYSDNM("vf25_cockpit.dnm");
	}

	glLoadMatrixd(wd->rot);
	quat2mat(rot, pt->rot);
	glMultMatrixd(rot);

	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glClearDepth(1.);
	glClear(GL_DEPTH_BUFFER_BIT);
/*	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBegin(GL_QUADS);
	glVertex3d(.5, .5, -1.);
	glVertex3d(-.5, .5, -1.);
	glVertex3d(-.5, -.5, -1.);
	glVertex3d(.5, -.5, -1.);
	glEnd();*/


	glPushMatrix();


	glMatrixMode (GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glFrustum (-wid, wid,
		-hei, hei,
		sonear, wd->pgc->znear * 5.);
	glMatrixMode (GL_MODELVIEW);

	if(pt->vft == &valkie_s && valcockpit){
		glScaled(.00001, .00001, -.00001);
		glTranslated(0, - 1., -3.5);
		glPushAttrib(GL_POLYGON_BIT);
		glFrontFace(GL_CW);
		DrawYSDNM(valcockpit, NULL, NULL, 0, NULL, 0);
		glPopAttrib();
	}
	else{
		gldTranslaten3dv(seat);
		gldScaled(FLY_SCALE / 2.);
		DrawSUF(sufcockpit, SUF_ATR, &g_gldcache);
	}

	glPopMatrix();

	if(0. < pt->health){
		GLfloat mat_diffuse[] = { .5, .5, .5, .2 };
		GLfloat mat_ambient_color[] = { 0.5, 0.5, 0.5, .2 };
		amat4_t m;
		double d, air, velo;
		int i;

		air = w->vft->atmospheric_pressure(w, &pt->pos)/*exp(-pt->pos[1] / 10.)*/;

		glPushMatrix();
	/*	glRotated(-gunangle, 1., 0., 0.);*/
		glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_LINE_SMOOTH);
		glDepthMask(GL_FALSE);
/*		gldTranslate3dv(seat);*/
		gldScaled(FLY_SCALE / 2.);
		glTranslated(0., 80, -276.);
		gldScaled(9.);
/*		gldScaled(.0002);
		glTranslated(0., 0., -1.);*/
		glGetDoublev(GL_MODELVIEW_MATRIX, &m);

		glDisable(GL_LIGHTING);
/*		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_color);*/
		glColor4ub(0,127,255,255);
		glBegin(GL_LINE_LOOP);
		glVertex2d(-1., -1.);
		glVertex2d( 1., -1.);
		glVertex2d( 1.,  1.);
		glVertex2d(-1.,  1.);
		glEnd();

		/* crosshair */
		glColor4ubv(wd->hudcolor);
		glBegin(GL_LINES);
		glVertex2d(-.15, 0.);
		glVertex2d(-.05, 0.);
		glVertex2d( .15, 0.);
		glVertex2d( .05, 0.);
		glVertex2d(0., -.15);
		glVertex2d(0., -.05);
		glVertex2d(0., .15);
		glVertex2d(0., .05);
		glEnd();

		/* director */
		for(i = 0; i < 24; i++){
			d = fmod(-pt->pyr[1] + i * 2. * M_PI / 24. + M_PI / 2., M_PI * 2.) - M_PI / 2.;
			if(-.8 < d && d < .8){
				glBegin(GL_LINES);
				glVertex2d(d, .8);
				glVertex2d(d, i % 6 == 0 ? .7 : 0.75);
				glEnd();
				if(i % 6 == 0){
					int j, len;
					char buf[16];
					glPushMatrix();
					len = sprintf(buf, "%d", i / 6 * 90);
					glTranslated(d - (len - 1) * .03, .9, 0.);
					glScaled(.03, .05, .1);
					for(j = 0; buf[j]; j++){
						gldPolyChar(buf[j]);
						glTranslated(2., 0., 0.);
					}
					glPopMatrix();
				}
			}
		}

		/* pressure/height gauge */
		glBegin(GL_LINE_LOOP);
		glVertex2d(.92, -0.25);
		glVertex2d(.92, 0.25);
		glVertex2d(.9, 0.25);
		glVertex2d(.9, -.25);
		glEnd();
		glBegin(GL_LINES);
		glVertex2d(.92, -0.25 + air * .5);
		glVertex2d(.9, -0.25 + air * .5);
		glEnd();

		/* height gauge */
		{
			double height;
			char buf[16];
			height = -10. * log(air);
			glBegin(GL_LINE_LOOP);
			glVertex2d(.8, .0);
			glVertex2d(.9, 0.025);
			glVertex2d(.9, -0.025);
			glEnd();
			glBegin(GL_LINES);
			glVertex2d(.8, .3);
			glVertex2d(.8, height < .05 ? -height * .3 / .05 : -.3);
			for(d = MAX(.01 - fmod(height, .01), .05 - height) - .05; d < .05; d += .01){
				glVertex2d(.85, d * .3 / .05);
				glVertex2d(.8, d * .3 / .05);
			}
			glEnd();
			sprintf(buf, "%lg", height / 30.48e-5);
			glPushMatrix();
			glTranslated(.7, .7, 0.);
			glScaled(.015, .025, .1);
			gldPolyString(buf);
			glPopMatrix();
		}
/*		glRasterPos3d(.45, .5 + 20. / mi, -1.);
		gldprintf("%lg meters", pt->pos[1] * 1e3);
		glRasterPos3d(.45, .5 + 0. / mi, -1.);
		gldprintf("%lg feet", pt->pos[1] / 30.48e-5);*/

#if 1
		/* velocity */
		glPushMatrix();
		glTranslated(-.2, 0., 0.);
		glBegin(GL_LINES);
		velo = VECLEN(pt->velo);
		glVertex2d(-.5, .3);
		glVertex2d(-.5, velo < .05 ? -velo * .3 / .05 : -.3);
		for(d = MAX(.01 - fmod(velo, .01), .05 - velo) - .05; d < .05; d += .01){
			glVertex2d(-.55, d * .3 / .05);
			glVertex2d(-.5, d * .3 / .05);
		}
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex2d(-.5, .0);
		glVertex2d(-.6, 0.025);
		glVertex2d(-.6, -0.025);
		glEnd();
		glPopMatrix();
		glPushMatrix();
		glTranslated(-.7, .7, 0.);
		glScaled(.015, .025, .1);
		gldPolyPrintf("M %lg", velo / .34);
		glPopMatrix();
		glPushMatrix();
		glTranslated(-.7, .7 - .08, 0.);
		glScaled(.015, .025, .1);
/*		gldPolyPrintf("%lg m/s", velo * 1e3);*/
		gldPolyPrintf("%lg", 1944. * velo);
		glPopMatrix();
#endif

		/* climb */
		glRotated(deg_per_rad * pt->pyr[2], 0., 0., 1.);
		for(i = -12; i <= 12; i++){
			d = 2. * (fmod(pt->pyr[0] + i * 2. * M_PI / 48. + M_PI / 2., M_PI * 2.) - M_PI / 2.);
			if(-.8 < d && d < .8){
				glBegin(GL_LINES);
				glVertex2d(-.4, d);
				glVertex2d(i % 6 == 0 ? -.5 : -0.45, d);
				glVertex2d(.4, d);
				glVertex2d(i % 6 == 0 ? .5 : 0.45, d);
				glEnd();
				if(i % 6 == 0){
					int j, len;
					char buf[16];
					glPushMatrix();
					len = sprintf(buf, "%d", i / 6 * 45);
					glTranslated(-.5 - len * .06, d, 0.);
					glScaled(.03, .05, .1);
					for(j = 0; buf[j]; j++){
						gldPolyChar(buf[j]);
						glTranslated(2., 0., 0.);
					}
					glPopMatrix();
				}
			}
		}

		glPopAttrib();
		glPopMatrix();
	}

	glPushMatrix();

	gldTranslate3dv(stick);
	glRotated(deg_per_rad * p->elevator, 1., 0., 0.);
	glRotated(deg_per_rad * p->aileron[0], 0., 0., -1.);
	gldTranslaten3dv(stick);
	gldScaled(FLY_SCALE / 2.);

	DrawSUF(sufstick, SUF_ATR, &g_gldcache);

	glMatrixMode (GL_PROJECTION);
	glPopMatrix();
	glMatrixMode (GL_MODELVIEW);

	glPopMatrix();

	glPopAttrib();
}


#if 0

static void fly_cockpitview(entity_t *pt, warf_t*, double (*pos)[3], int *);
static void fly_control(entity_t *pt, warf_t *w, input_t *inputs, double dt);
static void fly_destruct(entity_t *pt);
static void fly_anim(entity_t *pt, warf_t *w, double dt);
static void fly_draw(entity_t *pt, wardraw_t *wd);
static void fly_drawtra(entity_t *pt, wardraw_t *wd);
static int fly_takedamage(entity_t *pt, double damage, warf_t *w);
static void fly_gib_draw(const struct tent3d_line_callback *pl, void *pv);
static int fly_flying(const entity_t *pt);
static void fly_bullethole(entity_t *pt, sufindex, double rad, const double (*pos)[3], const double (*pyr)[3]);
static int fly_getrot(struct entity*, warf_t *, double (*)[4]);
static void fly_drawCockpit(struct entity*, const warf_t *, wardraw_t *);

static struct entity_private_static fly_s = {
	{
		fly_drawHUD/*/NULL*/,
		fly_cockpitview,
		fly_control,
		fly_destruct,
		fly_getrot,
		NULL, /* getrotq */
		fly_drawCockpit,
	},
	fly_anim,
	fly_draw,
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
};

typedef struct fly{
	entity_t st;
	double aileron[2], elevator, rudder;
	double throttle;
	avec3_t force[5], sight;
	struct tent3d_fpol *pf, *vapor[2];
	char muzzle, brk;
	int missiles;
	sufdecal_t *sd;
	bhole_t *frei;
	bhole_t bholes[50];
} fly_t;

#endif

#define SQRT2P2 (1.41421356/2.)

void smoke_draw(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	wardraw_t *wd = g_smoke_wd;
	amat4_t mat;
	double scale = pl->len;
	GLfloat ambient[4] = {.1,.1,.1,1.};
/*	glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);*/
	wd->light_on(wd);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glPushMatrix();
	gldTranslate3dv(pl->pos);
	glMultMatrixd(dd->invrot);
	glScaled(scale, scale, scale);
	{
		int i;
		double (*cuts)[2];
		cuts = CircleCuts(10);
		glBegin(GL_TRIANGLE_FAN);
		glColor4ub(95,95,63,255);
		glNormal3d(0,0,1);
		glVertex3d(0., 0., 0.);
		glColor4ub(95,95,63,0);
		for(i = 0; i <= 10; i++){
			int k = i % 10;
			glNormal3d(cuts[k][1] * SQRT2P2, cuts[k][0] * SQRT2P2, 0/*SQRT2P2*/);
			glVertex3d(cuts[k][1], cuts[k][0], 0.);
		}
		glEnd();
	}
	glPopMatrix();
	wd->light_off(wd);
}

int cmd_togglevalkie(int argc, const char *argv[]){
	ArmsShowWindow(ValkieNew, 500e3, offsetof(valkie_t, arms), valkie_arms, valkie_hardpoints, numof(valkie_hardpoints));
	return 0;
}
