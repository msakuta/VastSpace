#include "beamer.h"
//#include "entity_p.h"
#include "player.h"
//#include "train.h"
//#include "bullet.h"
//#include "bhole.h"
//#include "aim9.h"
//#include "coordcnv.h"
#include "coordsys.h"
#include "viewer.h"
//#include "spacewar.h"
//#include "mturret.h"
#include "cmd.h"
#include "glwindow.h"
//#include "arms.h"
//#include "bullet.h"
//#include "warutil.h"
#include "judge.h"
#include "astrodef.h"
#include "stellar_file.h"
//#include "glwindow.h"
//#include "antiglut.h"
//#include "worker.h"
//#include "glsl.h"
#include "astro_star.h"
#include "glextcall.h"
//#include "sensor.h"
extern "C"{
#include "bitmap.h"
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/suf/sufbin.h>
#include <clib/suf/sufdraw.h>
//#include "suflist.h"
#include <clib/avec3.h>
#include <clib/amat4.h>
#include <clib/aquatrot.h>
#include <clib/GL/gldraw.h>
#include <clib/wavsound.h>
#include <clib/zip/UnZip.h>
}
#include <assert.h>
#include <string.h>



/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289

#define VIEWDIST_FACTOR 1.2
#define MTURRET_SCALE (.00005)
#define MTURRET_MAX_GIBS 30
#define MTURRET_BULLETSPEED 2.
#define MTURRET_VARIANCE (.001 * M_PI)
#define MTURRET_INTOLERANCE (M_PI / 20.)
#define MTURRETROTSPEED (.4*M_PI)
#define RETHINKTIME .5
#define SCEPTER_SCALE 1./10000
#define SCEPTER_SMOKE_FREQ 10.
#define SCEPTER_RELOADTIME .3
#define SCEPTER_ROLLSPEED (.2 * M_PI)
#define SCEPTER_ROTSPEED (.3 * M_PI)
#define SCEPTER_MAX_ANGLESPEED (M_PI * .5)
#define SCEPTER_ANGLEACCEL (M_PI * .2)
#define SCEPTER_MAX_GIBS 20
#define BULLETSPEED 2.
#define SCARRY_MAX_HEALTH 200000
#define SCARRY_MAX_SPEED .03
#define SCARRY_ACCELERATE .01
#define SCARRY_MAX_ANGLESPEED (.005 * M_PI)
#define SCARRY_ANGLEACCEL (.002 * M_PI)
#define BUILDQUESIZE SCARRY_BUILDQUESIZE
#define numof(a) (sizeof(a)/sizeof*(a))
#define signof(a) ((a)<0?-1:0<(a)?1:0)


#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef ABS
#define ABS(a) ((a)<0?-(a):(a))
#endif

int g_healthbar = 1;
double g_capacitor_gen_factor = 1.;

/* color sequences */
#if 0
#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_orangeburn[] = {
	{0.1, COLOR32RGBA(255,255,191,255)},
	{0.15, COLOR32RGBA(255,255,31,191)},
	{0.45, COLOR32RGBA(255,127,31,95)},
	{2.3, COLOR32RGBA(255,31,0,63)},
};
static const struct color_sequence cs_orangeburn = DEFINE_COLSEQ(cnl_orangeburn, (COLOR32)-1, 3.);
static const struct color_node cnl_shortburn[] = {
	{0.1, COLOR32RGBA(255,255,191,255)},
	{0.15, COLOR32RGBA(255,255,31,191)},
	{0.25, COLOR32RGBA(255,127,31,0)},
};
static const struct color_sequence cs_shortburn = DEFINE_COLSEQ(cnl_shortburn, (COLOR32)-1, 0.5);
#endif


#if 0


static void warp_anim(struct war_field *, double dt);
static void warp_draw(struct war_field *, struct war_draw_data *);
static int warp_pointhit(warf_t *w, const avec3_t *pos, const avec3_t *velo, double dt, const struct contact_info *);
static void warp_accel(warf_t *w, avec3_t *dst, const avec3_t *srcpos, const avec3_t *srcvelo);
static double warp_atmospheric_pressure(warf_t *w, const avec3_t *pos);
static int warp_spherehit(warf_t *w, const avec3_t *pos, double rad, struct contact_info *ci);
static int warp_orientation(warf_t *w, amat3_t *dst, const avec3_t *pos);

static struct war_field_static warp_static = {
	warp_anim,
	WarPostFrame,
	WarEndFrame,
	warp_draw,
	warp_pointhit,
	warp_accel,
	warp_atmospheric_pressure,
	warp_atmospheric_pressure,
	warp_spherehit,
	warp_orientation,
};


#endif

#if 0
static void Warp::anim(struct war_field *w, double dt){
	struct bullet **ppb = &w->bl;
	static int wingfile = 0;
	static int walks = 0, apaches = 2;

	w->effects = 0;

	w->war_time += dt;

	while(*ppb){
		struct bullet *pb = *ppb;
		if(!pb->vft->anim(pb, w, w->tell, dt)){
			pb->active = 0;
		}
		ppb = &pb->next;
	}

	/* tank think */
	{
		entity_t **ppt = &w->tl;
		for(; *ppt; ppt = &(*ppt)->next){
			((struct entity_private_static*)(*ppt)->vft)->anim(*ppt, w, dt);
		}
	}
#if 0

	/* postframe process. do not change activity state while in postframe functions. */
	{
		struct bullet *pb = w->bl;
		for(; pb; pb = pb->next)
			pb->vft->postframe(pb);
	}
	{
		entity_t *pt = w->tl;
		for(; pt; pt = pt->next) if(((struct entity_private_static*)pt->vft)->postframe)
			((struct entity_private_static*)pt->vft)->postframe(pt);
	}

	/* purge inactive entities */
/*	ppb = &w->bl;
	while(*ppb){
		struct bullet *pb = *ppb;
		if(!pb->active){
			if(pb == (struct bullet*)w->pl->chase)
				w->pl->chase = NULL;
			{
				struct bullet *pb = (*ppb)->next;
				free(*ppb);
				*ppb = pb;
			}
			continue;
		}
		ppb = &pb->next;
	}*/
	{
		entity_t **ppt;
		warf_t *dw;
		for(ppt = &w->tl; *ppt; ) if(!(*ppt)->active && !(1 & ((struct entity_private_static*)(*ppt)->vft)->reuse) /*&& ((*ppt)->vft == &flare_s || (*ppt)->vft != &fly_s && (*ppt)->vft != &tank_s)*/){

			/* there are two possibilities to disappear from a warfield, that is
			  to be destroyed or to be transported to another field. we determine
			  this by calling the virtual function designed for this. 
			   note that activity state must be disabled prior to transportation,
			  to force referrers to clear their inter-field references that tend to
			  cause problems due to assumption referrer and referree belongs to the
			  same field. */
			if((*ppt)->vft->warp_dest && (dw = (*ppt)->vft->warp_dest(*ppt, w)) && dw != w){
				entity_t *pt = *ppt;
				*ppt = pt->next;
				pt->next = dw->tl;
				dw->tl = pt;
				pt->active = 1;
			}
			else{
				entity_t *next = (*ppt)->next;
				if(!(2 & ((struct entity_private_static*)(*ppt)->vft)->reuse)){
					if(*ppt == w->pl->chase)
						w->pl->chase = NULL;
					if(*ppt == w->pl->control)
						w->pl->control = NULL;
					if(*ppt == w->pl->selected)
						w->pl->selected = NULL;
					if((*ppt)->vft->destruct)
						(*ppt)->vft->destruct(*ppt);
				}
				if(!(2 & ((struct entity_private_static*)(*ppt)->vft)->reuse))
					free(*ppt);
				*ppt = next;
			}
		}
		else
			ppt = &(*ppt)->next;
	}
#endif

}

static void warp_draw(struct war_field *w, struct war_draw_data *wd){
	amat4_t lightmats[3];
	struct bullet **ppb;

	wd->lightdraws = 0;
	wd->light_on(wd);

	g_gldcache.valid = 0;
	{
		entity_t *pt;
		for(pt = w->tl; pt; pt = pt->next) if(((const struct entity_private_static*)pt->vft)->draw){
			((const struct entity_private_static*)pt->vft)->draw(pt, wd);
		}
	}
	{
		GLfloat whity[4] = {1., 1., 1., 1.};
		gldMaterialfv(GL_FRONT, GL_DIFFUSE, &whity, &g_gldcache);
		gldMaterialfv(GL_FRONT, GL_AMBIENT, &whity, &g_gldcache);
	}
	for(ppb = &w->bl; *ppb; ppb = &(*ppb)->next){
		struct bullet *pb = *ppb;
		pb->vft->drawmodel(pb, wd);
	}
	if(w->gibs){
		struct tent3d_line_drawdata dd;
/*		glPushAttrib(GL_ENABLE_BIT);*/
		glDisable(GL_CULL_FACE);
		VECCPY(dd.viewpoint, wd->view);
		VECCPY(dd.viewdir, wd->viewdir);
		VECNULL(dd.pyr);
		memcpy(dd.invrot, wd->irot, sizeof dd.invrot);
		dd.fov = wd->fov;
		dd.pgc = wd->pgc;
		DrawTeline3D(w->gibs, &dd);
/*		glPopAttrib();*/
	}
/*	glCallList(wd->listLight + 1);*/
	wd->light_off(wd);


	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(0);
	{
		entity_t *pt;
		for(pt = w->tl; pt; pt = pt->next){
			if(((const struct entity_private_static*)pt->vft)->drawtra)
				((const struct entity_private_static*)pt->vft)->drawtra(pt, wd);
		}
	}
	for(ppb = &w->bl; *ppb; ppb = &(*ppb)->next){
		struct bullet *pb = *ppb;
		pb->vft->draw(pb, wd);
	}
	if(w->tepl)
		DrawTefpol3D(w->tepl, wd->view, wd->pgc);

	glPopAttrib();
}

static int warp_pointhit(warf_t *w, const avec3_t *pos, const avec3_t *velo, double dt, struct contact_info *ci){
	return 0;
}

static void warp_accel(warf_t *w, avec3_t *dst, const avec3_t *srcpos, const avec3_t *srcvelo){
	VECNULL(*dst);
}

static double warp_atmospheric_pressure(warf_t *w, const avec3_t *pos){
	return 0.;
}

static int warp_spherehit(warf_t *w, const avec3_t *pos, double plrad, struct contact_info *ci){
	return 0;
}

static int warp_orientation(warf_t *w, amat3_t *dst, const avec3_t *pos){
	MATIDENTITY(*dst);
	return 1;
}



#endif








void draw_healthbar(Entity *pt, wardraw_t *wd, double v, double scale, double s, double g){
	double x = v * 2. - 1., h = MIN(.1, .1 / (1. + scale)), hs = h / 2.;
	if(!g_healthbar)
		return;
	if(g_healthbar == 1 && wd->w && wd->w->pl){
		Entity *pt2;
		for(pt2 = wd->w->pl->selected; pt2; pt2 = pt2->selectnext) if(pt2 == pt){
			break;
		}
		if(!pt2)
			return;
	}
	glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glPushMatrix();
	gldTranslate3dv(pt->pos);
	glMultMatrixd(wd->vw->irot);
	gldScaled(scale);
#if 0 /* signal spectrum drawing */
	if(vft->signalSpectrum){
		double x;
		glColor4ub(0,255,127,255);
		glBegin(GL_LINES);
		for(x = -10; x < 10; x++){
			glVertex2d(x / 10., -1);
			glVertex2d(x / 10., -.95);
		}
		glEnd();
		glBegin(GL_LINE_STRIP);
		for(x = -100; x < 100; x++){
			glVertex2d(x / 100., -1 + vft->signalSpectrum(pt, x / 10.));
		}
		glEnd();
	}
#endif
	glBegin(GL_QUADS);
	glColor4ub(0,255,0,255);
	glVertex3d(-1., 1., 0.);
	glVertex3d( x, 1., 0.);
	glVertex3d( x, 1. - h, 0.);
	glVertex3d(-1., 1. - h, 0.);
	glColor4ub(255,0,0,255);
	glVertex3d( x, 1., 0.);
	glVertex3d( 1., 1., 0.);
	glVertex3d( 1., 1. - h, 0.);
	glVertex3d( x, 1. - h, 0.);
	if(0 <= s){
		x = s * 2. - 1.;
		glColor4ub(0,255,255,255);
		glVertex3d(-1., 1. + hs * 2, 0.);
		glVertex3d( x, 1. + hs * 2, 0.);
		glVertex3d( x, 1. + hs, 0.);
		glVertex3d(-1., 1. + hs, 0.);
		glColor4ub(255,0,127,255);
		glVertex3d( x, 1. + hs * 2, 0.);
		glVertex3d( 1., 1. + hs * 2, 0.);
		glVertex3d( 1., 1. + hs, 0.);
		glVertex3d( x, 1. + hs, 0.);
	}
	glEnd();
	if(0 <= g){
		x = g * 2. - 1.;
		glTranslated(0, -2. * h, 0.);
		glBegin(GL_QUADS);
		glColor4ub(255,255,0,255);
		glVertex3d(-1., 1. - hs, 0.);
		glVertex3d( x, 1. - hs, 0.);
		glVertex3d( x, 1., 0.);
		glVertex3d(-1., 1., 0.);
		glColor4ub(255,0,127,255);
		glVertex3d( x, 1. - hs, 0.);
		glVertex3d( 1., 1. - hs, 0.);
		glVertex3d( 1., 1., 0.);
		glVertex3d( x, 1., 0.);
		glEnd();
	}
/*	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);*/
/*	glBegin(GL_LINE_LOOP);
	glColor4ub(0,255,255,255);
	glVertex3d(-1., 1., 0.);
	glVertex3d( 1, 1., 0.);
	glVertex3d( 1, 1. - h, 0.);
	glVertex3d(-1., 1. - h, 0.);
	glEnd();*/
	glPopMatrix();
	glPopAttrib();
}








#define MAX_SHIELD_AMOUNT 15000.
#define BEAMER_HEALTH 25000.
#define BEAMER_SCALE .0002
#define BEAMER_MAX_SPEED .1
#define BEAMER_ACCELERATE .05
#define BEAMER_MAX_ANGLESPEED .4
#define BEAMER_ANGLEACCEL .2
#define BEAMER_SHIELDRAD .09

































struct maneuve{
	double accel;
	double maxspeed;
	double angleaccel;
	double maxanglespeed;
	double capacity; /* capacity of capacitor [MJ] */
	double capacitor_gen; /* generated energy [MW] */
};


/* maneuvering spaceships, integrated common responses.
 assumption is that accelerations towards all directions except forward movement
 is a half the maximum accel. */
void Warpable::maneuver(const amat4_t mat, double dt, const struct maneuve *mn){
	Entity *pt = this;
	double const maxspeed2 = mn->maxspeed * mn->maxspeed;
	if(!warping){
		if(pt->inputs.press & PL_2){
			VECSADD(pt->omg, &mat[0], dt * mn->angleaccel);
		}
		if(pt->inputs.press & PL_8){
			VECSADD(pt->omg, &mat[0], -dt * mn->angleaccel);
		}
		if(pt->inputs.press & PL_4){
			VECSADD(pt->omg, &mat[4], dt * mn->angleaccel);
		}
		if(pt->inputs.press & PL_6){
			VECSADD(pt->omg, &mat[4], -dt * mn->angleaccel);
		}
		if(pt->inputs.press & PL_7){
			VECSADD(pt->omg, &mat[8], dt * mn->angleaccel);
		}
		if(pt->inputs.press & PL_9){
			VECSADD(pt->omg, &mat[8], -dt * mn->angleaccel);
		}
		if(mn->maxanglespeed * mn->maxanglespeed < VECSLEN(pt->omg)){
			VECNORMIN(pt->omg);
			VECSCALEIN(pt->omg, mn->maxanglespeed);
		}
		if(!(pt->inputs.press & (PL_8 | PL_2 | PL_4 | PL_6 | PL_7 | PL_9))){
			double f;
			f = VECLEN(pt->omg);
			if(f){
				VECSCALEIN(pt->omg, 1. / f);
				f = MAX(0, f - dt * mn->angleaccel);
				VECSCALEIN(pt->omg, f);
			}
		}

		if(pt->inputs.press & PL_W){
			VECSADD(pt->velo, &mat[8], -dt * mn->accel);
			if(VECSLEN(pt->velo) < maxspeed2);
			else{
				VECNORMIN(pt->velo);
				VECSCALEIN(pt->velo, mn->maxspeed);
			}
		}
		if(pt->inputs.press & PL_S){
			VECSADD(pt->velo, &mat[8], dt * mn->accel * .5);
			if(VECSLEN(pt->velo) < maxspeed2);
			else{
				VECNORMIN(pt->velo);
				VECSCALEIN(pt->velo, mn->maxspeed);
			}
		}
		if(pt->inputs.press & PL_A){
			VECSADD(pt->velo, &mat[0], -dt * mn->accel * .5);
			if(VECSLEN(pt->velo) < maxspeed2);
			else{
				VECNORMIN(pt->velo);
				VECSCALEIN(pt->velo, mn->maxspeed);
			}
		}
		if(pt->inputs.press & PL_D){
			VECSADD(pt->velo, &mat[0],  dt * mn->accel * .5);
			if(VECSLEN(pt->velo) < maxspeed2);
			else{
				VECNORMIN(pt->velo);
				VECSCALEIN(pt->velo, mn->maxspeed);
			}
		}
		if(pt->inputs.press & PL_Q){
			VECSADD(pt->velo, &mat[4],  dt * mn->accel * .5);
			if(VECSLEN(pt->velo) < maxspeed2);
			else{
				VECNORMIN(pt->velo);
				VECSCALEIN(pt->velo, mn->maxspeed);
			}
		}
		if(pt->inputs.press & PL_Z){
			VECSADD(pt->velo, &mat[4], -dt * mn->accel * .5);
			if(VECSLEN(pt->velo) < maxspeed2);
			else{
				VECNORMIN(pt->velo);
				VECSCALEIN(pt->velo, mn->maxspeed);
			}
		}
		if(1){
			double f, dropoff = !(pt->inputs.press & (PL_W | PL_S | PL_A | PL_D | PL_Q | PL_Z)) ? mn->accel : mn->accel * .2;
			f = VECLEN(pt->velo);
			if(f){
				VECSCALEIN(pt->velo, 1. / f);
				f = MAX(0, f - dt * dropoff);
				VECSCALEIN(pt->velo, f);
			}
		}
	}
}




typedef struct shieldWavelet shieldw_t;


#if 0
static void beamer_cockpitview(struct entity*, warf_t *, double (*pos)[3], int *chasecamera);
static void warpable_control(entity_t *pt, warf_t *w, input_t *inputs, double dt);
static void warpable_drawHUD(entity_t *pt, warf_t *wf, wardraw_t *wd, const double irot[16], void (*gdraw)(void));
static int beamer_is_warping(entity_t *pt, const warf_t *wf){ return ((warpable_t*)pt)->warping; }
static warf_t *warpable_warp_dest(entity_t *pt, const warf_t *wf);
static const char *beamer_idname(entity_t *pt){ return "beamer"; }
static const char *beamer_classname(entity_t *pt){ return "Lancer Class"; }
static int beamer_getrot(struct entity *pt, warf_t *w, double (*rot)[16]);
static void beamer_anim(entity_t *pt, warf_t *w, double dt);
static void beamer_draw(entity_t *pt, wardraw_t *wd);
static void beamer_drawtra(entity_t *pt, wardraw_t *wd);
static int beamer_takedamage(entity_t *pt, double damage, warf_t *w);
static void beamer_gib_draw(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv);
static void beamer_bullethit(entity_t *pt, warf_t *w, const bullet_t *pb);
static int beamer_tracehit(entity_t *pt, warf_t *w, const double src[3], const double dir[3], double rad, double dt, double *ret, double (*retp)[3], double (*ret_normal)[3]);
static double assault_signalSpectrum(const struct entity *pt, double wavelength);
static int assault_sensorSpectrum(const struct entity *, struct signalContext *sc);


static struct entity_private_static beamer_s = {
	{
		warpable_drawHUD,
		beamer_cockpitview,
		warpable_control,
		NULL, /* destruct */
		beamer_getrot,
		NULL, /* getrotq */
		NULL, /* drawCockpit */
		beamer_is_warping,
		warpable_warp_dest,
		beamer_idname,
		beamer_classname,
		NULL, /* start_control */
		NULL, /* end_control */
		NULL, /* analog_mask */
		warpable_popup_menu,
	},
	beamer_anim,
	beamer_draw,
	beamer_drawtra,
	beamer_takedamage,
	beamer_gib_draw,
	tank_postframe,
	truefunc,
	M_PI,
	-M_PI / .6, M_PI / 4.,
	NULL, NULL, NULL, NULL,
	0,
	MTURRET_BULLETSPEED,
	0.090,
	BEAMER_SCALE,
	0, 1,
	NULL/*beamer_bullethole*/,
	{0},
	beamer_bullethit/*bullethit*/,
	beamer_tracehit,
	{0}, /* hitmdl */
	NULL, /* shadowdraw */
	NULL, NULL, /* visibleTo, visibleFrom */
	assault_signalSpectrum,
	assault_sensorSpectrum,
};
#endif

static const struct maneuve beamer_mn = {
	BEAMER_ACCELERATE, /* double accel; */
	BEAMER_MAX_SPEED, /* double maxspeed; */
	BEAMER_ANGLEACCEL, /* double angleaccel; */
	BEAMER_MAX_ANGLESPEED, /* double maxanglespeed; */
	50000., /* double capacity; [MJ] */
	100., /* double capacitor_gen; [MW] */
};


Warpable::Warpable(){
	warpSpeed = /*1e6 * LIGHTYEAR_PER_KILOMETER */5. * AU_PER_KILOMETER;
	warping = 0;
//	warp_next_warf = NULL;
	capacitor = 0.;
	inputs.change = 0;
}

Beamer::Beamer(){
	charge = 0.;
//	dock = NULL;
	undocktime = 0.f;
	cooldown = 0.;
	sw = NULL;
	shieldAmount = MAX_SHIELD_AMOUNT;
	shield = 0.;
	VECNULL(integral);
	health = BEAMER_HEALTH;
}

const char *Beamer::idname()const{
	return "beamer";
}

const char *Beamer::classname()const{
	return "Lancer class";
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

Beamer::Beamer(WarField *w){
//	EntityInit(ret, w, &beamer_s);
	mass = 1e6;
	health = BEAMER_HEALTH;
/*	for(int i = 0; i < numof(pf); i++)
		pf[i] = AddTefpolMovable3D(w->tepl, pos, velo, avec3_000, &cs_orangeburn, TEP3_THICKEST | TEP3_ROUGH, cs_orangeburn.t);*/
}

#if 0
static void beamer_cockpitview(struct entity *pt, warf_t *w, double (*pos)[3], int *chasecamera){
	avec3_t ofs;
	static const avec3_t src[] = {
		{.002, .018, .022},
#if 1
		{0., 0., 1.0},
#else
		{0., 0./*05*/, 0.150},
		{0., 0./*1*/, 0.300},
		{0., 0., 1.000},
		{0., 0., 2.000},
#endif
	};
	amat4_t mat;
	aquat_t q;
	*chasecamera = (*chasecamera + numof(src)) % numof(src);
	if(*chasecamera){
		QUATMUL(q, pt->rot, w->pl->rot);
		quat2mat(&mat, q);
	}
	else
		quat2mat(&mat, pt->rot);
	mat4vp3(ofs, mat, src[*chasecamera]);
	if(*chasecamera)
		VECSCALEIN(ofs, g_viewdist);
	VECADD(*pos, pt->pos, ofs);
}

extern struct astrobj earth, island3, moon, jupiter;
extern struct player *ppl;
#endif

int find_teleport(const char *name, int flags, CoordSys **cs, Vec3d &pos){
	int i;
	for(i = 0; i < ntplist; i++) if(!strcmp(name, tplist[i].name) && flags & tplist[i].flags){
		*cs = tplist[i].cs;
		VECCPY(pos, tplist[i].pos);
		return 1;
	}
	return 0;
}

// transit to a CoordSys from another, keeping absolute position and velocity.
int cmd_transit(int argc, char *argv[], void *pv){
	Player *ppl = (Player*)pv;
	Entity *pt;
	if(argc < 2){
		CmdPrintf("Usage: transit dest");
		return 1;
	}
	for(pt = ppl->selected; pt; pt = pt->selectnext){
		Vec3d pos;
		CoordSys *pcs;
		if(pcs = const_cast<CoordSys*>(ppl->cs)->findcspath(argv[1])){
			pt->transit_cs(pcs);
		}
	}
	return 0;
}

int cmd_warp(int argc, char *argv[], void *pv){
	double g_warp_cost_factor = 1.;
	Player *ppl = (Player*)pv;
	Entity *pt;
	if(argc < 2){
		CmdPrintf("Usage: warp dest [x] [y] [z]");
		return 1;
	}
	for(pt = ppl->selected; pt; pt = pt->selectnext){
		Warpable *p = pt->toWarpable();
		if(!p)
			continue;
		WarField *w = p->w;
		if(!p->warping){
			Vec3d delta, pos;
			const CoordSys *pa = NULL;
			CoordSys *pcs;
			double landrad;
			double dist, cost;
			extern coordsys *g_galaxysystem;
			Vec3d dstpos = vec3_000;
			if(find_teleport(argv[1], TELEPORT_WARP, &pcs, pos)){
				delta = w->cs->tocs(pos, pcs);
				VECSUBIN(delta, pt->pos);
				VECCPY(dstpos, pos);
				pa = pcs;
			}
			else if(pa = ppl->cs->findcs(argv[1])){
				dstpos[0] = argc < 3 ? 0. : atof(argv[2]);
				dstpos[1] = argc < 4 ? 0. : atof(argv[3]);
				dstpos[2] = argc < 5 ? 0. : atof(argv[4]);
				delta = w->cs->tocs(dstpos, pa);
				VECSUBIN(delta, pt->pos);
			} 
			else
				continue;
			dist = VECLEN(delta);
			cost = g_warp_cost_factor * pt->mass / 1e3 * (log10(dist + 1.) + 1.);
			if(cost < p->capacitor){
				double f;
				int i;
				p->warping = 1;
				p->capacitor -= cost;
	/*			f = VECLEN(delta);
				f = (f - pa->rad * 1.1) / f;
				VECNULL(p->warpdst);
				VECSCALE(p->warpdst, delta, f);
				VECADDIN(p->warpdst, pt->pos);*/
				VECCPY(p->warpdst, dstpos);
				for(i = 0; i < 3; i++)
					p->warpdst[i] += 2. * (drseq(&p->w->rs) - .5);
				p->totalWarpDist = dist;
				p->currentWarpDist = 0.;
				p->warpcs = NULL;
				p->warpdstcs = const_cast<CoordSys*>(pa);
//				p->warp_next_warf = NULL;
			}
		}
	}
	return 0;
}

static int enum_cs_flags(const CoordSys *root, int mask, int flags, const CoordSys ***ret, int left){
	const CoordSys *cs;
	if((root->flags & mask) == (flags & mask)){
		*(*ret)++ = root;
		if(!--left)
			return 0;
	}
	for(cs = root->children; cs; cs = cs->next) if(!(left = enum_cs_flags(cs, mask, flags, ret, left)))
		return left;
	return left;
}



int cmd_togglewarpmenu(int argc, char *argv[], void *){
	extern coordsys *g_galaxysystem;
	char *cmds[64]; /* not much of menu items as 64 are able to displayed after all */
	const char *subtitles[64];
	coordsys *reta[64], **retp = reta;
	static const char *windowtitle = "Warp Destination";
	glwindow *wnd, **ppwnd;
	int left, i;
	ppwnd = GLwindow::findpp(&glwlist, &GLwindow::TitleCmp(windowtitle));
	if(ppwnd){
		glwActivate(ppwnd);
		return 0;
	}
/*	for(ppwnd = &glwlist; *ppwnd; ppwnd = &(*ppwnd)->next) if((*ppwnd)->title == windowtitle){
		glwActivate(ppwnd);
		return 0;
	}*/
	for(i = left = 0; i < ntplist && left < numof(cmds); i++) if(tplist[i].flags & TELEPORT_WARP){
		struct teleport *tp = &tplist[i];
		cmds[left] = (char*)malloc(sizeof "warp \"\"" + strlen(tp->name));
		strcpy(cmds[left], "warp \"");
		strcat(cmds[left], tp->name);
		strcat(cmds[left], "\"");
		subtitles[left] = tp->name;
		left++;
	}
	wnd = glwMenu(windowtitle, left, subtitles, NULL, cmds, 0);
	for(i = 0; i < left; i++){
		free(cmds[i]);
	}
/*	left = enum_cs_flags(g_galaxysystem, CS_WARPABLE, CS_WARPABLE, &retp, numof(reta));
	{
		int i;
		for(i = 0; i < numof(cmds) - left; i++){
			cmds[i] = malloc(sizeof "warp \"\"" + strlen(reta[i]->name));
			strcpy(cmds[i], "warp \"");
			strcat(cmds[i], reta[i]->name);
			strcat(cmds[i], "\"");
			subtitles[i] = reta[i]->fullname ? reta[i]->fullname : reta[i]->name;
		}
	}
	wnd = glwMenu(numof(reta) - left, subtitles, NULL, cmds, 0);*/
//	wnd->title = windowtitle;
	return 0;
}

int Warpable::popupMenu(char ***const titles, int **keys, char ***cmds, int *pnum){
	int i;
	int num;
	int acty = 1;
	char *titles1[] = {"Warp To..."};
	char *cmds1[] = {"Warp To..."};
	for(i = 0; i < *pnum; i++) if(!strcmp((*titles)[i], titles1[0]) && !strcmp((*cmds)[i], cmds1[0]))
		acty = 0;
	if(!acty)
		return 0;
	i = *pnum;
	num = *pnum += 2;
	*titles = (char**)realloc(*titles, num * sizeof **titles);
	*keys = (int*)realloc(*keys, num * sizeof **keys);
	*cmds = (char**)realloc(*cmds, num * sizeof **cmds);
	(*titles)[i] = (char*)glwMenuSeparator;
	(*keys)[i] = '\0';
	(*cmds)[i] = NULL;
	i++;
	(*titles)[i] = "Warp To...";
	(*keys)[i] = '\0';
	(*cmds)[i] = "togglewarpmenu";
	st::popupMenu(titles, keys, cmds, pnum);
	return 0;
}

Warpable *Warpable::toWarpable(){
	return this;
}

void Warpable::control(input_t *inputs, double dt){
	Warpable *p = this;

	/* camera distance is always configurable. */
/*	if(inputs->press & PL_MWU){
		g_viewdist *= VIEWDIST_FACTOR;
	}
	if(inputs->press & PL_MWD){
		g_viewdist /= VIEWDIST_FACTOR;
	}*/

	if(!w || health <= 0.)
		return;
	this->inputs = *inputs;

/*	if(inputs->change & inputs->press & PL_G)
		p->menu = !p->menu;*/

#if 0
	if(p->menu && !p->warping){
		extern coordsys *g_sun_low_orbit, *g_iserlohnsystem, *g_rwport, *g_earthorbit, *g_lagrange1, *g_moon_low_orbit, *g_jupiter_low_orbit, *g_saturn_low_orbit;
		double landrad;
		avec3_t dstpos = {.0};
		coordsys *pa = NULL;
		if(0);
/*		else if(inputs->change & inputs->press & PL_D){
			pa = g_iserlohnsystem;
			dstpos[0] = +75.;
		}*/
		else if(inputs->change & inputs->press & PL_B){
			pa = g_rwport;
		}
		else if(inputs->change & inputs->press & PL_A){
			pa = g_earthorbit;
			dstpos[1] = 1.;
		}
		else if(inputs->change & inputs->press & PL_S){
			pa = g_lagrange1;
			dstpos[0] = +15.;
			dstpos[1] = -22.;
		}
		else if(inputs->change & inputs->press & PL_D){
			pa = g_moon_low_orbit;
		}
		else if(inputs->change & inputs->press & PL_W){
			pa = g_jupiter_low_orbit;
		}
		else if(inputs->change & inputs->press & PL_Z){
			pa = g_saturn_low_orbit;
		}
		else if(inputs->change & inputs->press & PL_Q){
			pa = g_sun_low_orbit;
		}
		if(pa){
			avec3_t delta;
			double f;
			int i;
			p->warping = 1;
			tocs(delta, w->cs, pa->pos, pa);
			VECSUBIN(delta, pt->pos);
			f = VECLEN(delta);
			f = (f - pa->rad * 1.1) / f;
/*			VECNULL(p->warpdst);
			VECSCALE(p->warpdst, delta, f);
			VECADDIN(p->warpdst, pt->pos);*/
			VECCPY(p->warpdst, dstpos);
			for(i = 0; i < 3; i++)
				p->warpdst[i] += 2. * (drseq(&w->rs) - .5);
			p->menu = 0;
			p->warpcs = NULL;
			p->warpdstcs = pa;
			p->warp_next_warf = NULL;
		}
	}
#endif
}

unsigned Warpable::analog_mask(){
	return 0;
}

#if 0
static void warpable_drawHUD(entity_t *pt, warf_t *wf, wardraw_t *wd, const double irot[16], void (*gdraw)(void)){
	warpable_t *p = (warpable_t*)pt;
/*	base_drawHUD_target(pt, wf, wd, gdraw);*/
	base_drawHUD(pt, wf, wd, irot, gdraw);
	glPushMatrix();
	glPushAttrib(GL_CURRENT_BIT);
	if(p->warping){
		avec3_t warpdstpos, eyedelta;
		tocs(warpdstpos, wf->cs, p->warpdst, p->warpdstcs);
		VECSUB(eyedelta, warpdstpos, wd->view);
		VECNORMIN(eyedelta);
		glLoadMatrixd(wd->rot);
		glRasterPos3dv(eyedelta);
		gldprintf("warpdst");
	}
	glLoadIdentity();

	{
		GLint vp[4];
		int w, h, m, mi, i;
		double left, bottom, velo;
		amat3_t ort;

		glGetIntegerv(GL_VIEWPORT, vp);
		w = vp[2], h = vp[3];
		m = w < h ? h : w;
		mi = w < h ? w : h;
		left = -(double)w / m;
		bottom = -(double)h / m;

		velo = p->warping && p->warpcs ? VECLEN(p->warpcs->velo) : VECLEN(pt->velo);
		wf->vft->orientation(wf, &ort, pt->pos);
		glRasterPos3d(left + 20. / m, -bottom - 100. / m, -1);
		gldprintf("%lg km/s", velo);
		glRasterPos3d(left + 20. / m, -bottom - 120. / m, -1);
		gldprintf("%lg kt", 1944. * velo);

/*		if(p->menu){
			glRasterPos3d(left + 00. / m, bottom + 140. / m, -1);
			gldprintf("Q Sun");
			glRasterPos3d(left + 00. / m, bottom + 120. / m, -1);
			gldprintf("Z Saturn");
			glRasterPos3d(left + 00. / m, bottom + 100. / m, -1);
			gldprintf("A Earth");
			glRasterPos3d(left + 00. / m, bottom + 80. / m, -1);
			gldprintf("S Island 3");
			glRasterPos3d(left + 00. / m, bottom + 60. / m, -1);
			gldprintf("D Moon");
			glRasterPos3d(left + 00. / m, bottom + 40. / m, -1);
			gldprintf("W Jupiter");
		}*/

		if(p->warping){
			double desiredvelo, velo, f, dist;
			double (*cuts)[2];
			char buf[128];
			avec3_t *pvelo = p->warpcs ? &p->warpcs->velo : &pt->velo;
			avec3_t dstcspos, warpdst, delta; /* current position measured in destination coordinate system */
			int iphase;
			tocs(dstcspos, p->warpdstcs, pt->pos, wf->cs);
			tocs(warpdst, wf->cs, p->warpdst, p->warpdstcs);

			velo = VECLEN(*pvelo);
			VECSUB(delta, p->warpdst, dstcspos);
			dist = VECLEN(delta);
			p->totalWarpDist = p->currentWarpDist + dist;
			cuts = CircleCuts(32);
			glTranslated(0, -.5, -1);
			glScaled(.25, .25, 1.);

			f = log10(velo / p->warpSpeed);
			iphase = -8 < f ? 8 + f : 0;

			/* speed meter */
			glColor4ub(127,127,127,255);
			glBegin(GL_QUAD_STRIP);
			for(i = 4; i <= 4 + iphase; i++){
				glVertex2d(-cuts[i][1], cuts[i][0]);
				glVertex2d(-.5 * cuts[i][1], .5 * cuts[i][0]);
			}
			glEnd();

			glColor4ub(255,255,255,255);
			glBegin(GL_LINE_LOOP);
			for(i = 4; i <= 12; i++){
				glVertex2d(cuts[i][1], cuts[i][0]);
			}
			for(i = 12; 4 <= i; i--){
				glVertex2d(.5 * cuts[i][1], .5 * cuts[i][0]);
			}
			glEnd();

			/* progress */
			glColor4ub(127,127,127,255);
			glBegin(GL_QUADS);
			glVertex2d(-1., -.5);
			glVertex2d(p->currentWarpDist / p->totalWarpDist * 2. - 1., -.5);
			glVertex2d(p->currentWarpDist / p->totalWarpDist * 2. - 1., -.7);
			glVertex2d(-1., -.7);
			glEnd();

			glColor4ub(255,255,255,255);
			glBegin(GL_LINE_LOOP);
			glVertex2d(-1., -.5);
			glVertex2d(1., -.5);
			glVertex2d(1., -.7);
			glVertex2d(-1., -.7);
			glEnd();

			sprintf(buf, "%lg%%", p->currentWarpDist / p->totalWarpDist * 100.);
			glRasterPos2d(- 0. / w, -.4 + 16. * 4 / h);
			gldPutString(buf);
			if(velo != 0){
				double eta = (p->totalWarpDist - p->currentWarpDist) / p->warpSpeed;
				eta = fabs(eta);
				if(3600 * 24 < eta)
					sprintf(buf, "ETA: %d days %02d:%02d:%02lg", (int)(eta / (3600 * 24)), (int)(fmod(eta, 3600 * 24) / 3600), (int)(fmod(eta, 3600) / 60), fmod(eta, 60));
				else if(60 < eta)
					sprintf(buf, "ETA: %02d:%02d:%02lg", (int)(eta / 3600), (int)(fmod(eta, 3600) / 60), fmod(eta, 60));
				else
					sprintf(buf, "ETA: %lg sec", eta);
				glRasterPos2d(- 8. / w, -.4);
				gldPutString(buf);
			}
		}
	}

	glPopAttrib();
	glPopMatrix();
}



static int beamer_getrot(struct entity *pt, warf_t *w, double (*rot)[16]){
	quat2imat(*rot, pt->rot);
	return 1/*w->pl->control != pt*/;
}

static warf_t *warpable_warp_dest(entity_t *pt, const warf_t *w){
	warpable_t *p = ((warpable_t*)pt);
	if(!p->warping)
		return NULL;
	return p->warp_next_warf;
}
#endif


void Warpable::warp_collapse(){
	int i;
	Warpable *p = this;
	Entity *pt2;
	Vec3d dstcspos; /* current position measured in destination coordinate system */
/*	tocs(dstcspos, p->warpdstcs, pt->pos, w->cs);
	VECCPY(pt->pos, dstcspos);
	tocsv(dstcspos, p->warpdstcs, pt->velo, pt->pos, w->cs);
	VECCPY(pt->velo, dstcspos);*/
/*	transit_cs(pt, w, p->warpdstcs);*/
	for(pt2 = w->el; pt2; pt2 = pt2->next)
		transit_cs(p->warpdstcs);
	if(p->warpcs){
/*		for(i = 0; i < npf; i++){
			if(pf[i])
				ImmobilizeTefpol3D(pf[i]);
			pf[i] = AddTefpolMovable3D(p->warpdstcs->w->tepl, p->st.pos, pt->velo, avec3_000, &cs_orangeburn, TEP3_THICKEST | TEP3_ROUGH, cs_orangeburn.t);
		}*/
		p->warpcs->flags |= CS_DELETE;
		p->warpcs = NULL;
	}
}

const maneuve Warpable::mymn = {
	0, // double accel;
	0, // double maxspeed;
	0, // double angleaccel;
	0, // double maxanglespeed;
	0, // double capacity; /* capacity of capacitor [MJ] */
	0, // double capacitor_gen; /* generated energy [MW] */
};
const maneuve &Warpable::getManeuve()const{
	return mymn;
}
void Warpable::anim(double dt){
	Mat4d mat;
	transform(mat);
	Warpable *const p = this;
	Entity *pt = this;
	const maneuve *mn = &getManeuve();

	if(p->warping){
		double desiredvelo, velo;
		Vec3d *pvelo = p->warpcs ? &p->warpcs->velo : &pt->velo;
		Vec3d dstcspos, warpdst; /* current position measured in destination coordinate system */
		double sp, scale;
		dstcspos = p->warpdstcs->tocs(pt->pos, w->cs);
		warpdst = w->cs->tocs(p->warpdst, p->warpdstcs);
		{
			Quatd qc;
			Vec3d omega, dv, forward;
			dv = warpdst - pt->pos;
			dv.normin();
			forward = pt->rot.trans(avec3_001);
			forward *= -1.;
			sp = dv.sp(forward);
			omega = dv.vp(forward);
			if(sp < 0.){
				omega += mat.vec3(0) * mn->angleaccel;
			}
			scale = -.5 * dt;
			if(scale < -1.)
				scale = -1.;
			omega *= scale;
			pt->rot = pt->rot.quatrotquat(omega);
		}
		velo = (*pvelo).len();
		desiredvelo = .5 * VECDIST(warpdst, dstcspos);
		desiredvelo = MIN(desiredvelo, p->warpSpeed);
/*		desiredvelo = desiredvelo < 5. ? desiredvelo * desiredvelo / 5. : desiredvelo;*/
/*		desiredvelo = MIN(desiredvelo, 1.47099e8);*/
		if((warpdst - dstcspos).slen() < 1. * 1.){
			warp_collapse();
			p->warping = 0;
			pt->velo.clear();
			if(w->pl->chase == pt){
				w->pl->velo = w->pl->cs->tocsv(pt->velo, pt->pos, w->cs);
			}
		}
		else if(desiredvelo < velo){
			Vec3d delta, dst, dstvelo;
			double dstspd, u, len;
			delta = p->warpdst - dstcspos;
/*			VECSUB(delta, warpdst, pt->pos);*/
			len = delta.len();
#if 1
			u = desiredvelo / len;
			dstvelo = delta * u;
			*pvelo = dstvelo;
#else
			u = -exp(-desiredvelo * dt);
			VECSCALE(dst, delta, u);
			VECADDIN(dst, warpdst);
			VECSUB(*pvelo, dst, pt->pos);
			VECSCALEIN(*pvelo, 1. / dt);
#endif
			if(p->warpcs){
#if 1
				p->warpcs->adopt_child(p->warpdstcs);
#else
				avec3_t pos, velo;
				int i;
				for(i = 0; i < numof(p->warpcs->children); i++) if(p->warpcs->parent->children[i] == p->warpcs)
					break;
				assert(i != numof(p->warpcs->children));
				tocs(pos, p->warpdstcs, p->warpcs->pos, p->warpcs->parent);
				tocsv(velo, p->warpdstcs, p->warpcs->velo, p->warpcs->pos, p->warpcs->parent);
/*				p->warpcs->parent->children[i] = NULL;*/
				memmove(&p->warpcs->parent->children[i], &p->warpcs->parent->children[i+1], (numof(p->warpcs->parent->children) - i - 1) * sizeof p->warpcs);
				p->warpcs->parent->nchildren--;
				VECCPY(p->warpcs->pos, pos);
				VECCPY(p->warpcs->velo, velo);
				p->warpcs->parent = p->warpdstcs;
				legitimize_child(p->warpcs);
#endif
			}
		}
		else if(.9 < sp){
			avec3_t delta, dstvelo;
			double dstspd, u, len;
			const double L = LIGHT_SPEED;
			VECSUB(delta, warpdst, pt->pos);
			len = VECLEN(delta);
			u = (velo + .5) * 1e1 /** 5e-1 * (len - p->sight->rad)*/;
	/*		u = L - L * L / (u + L);*/
	/*		dstspd = (u + v) / (1 + u * v / LIGHT_SPEED / LIGHT_SPEED);*/
			VECNORMIN(delta);
			VECSCALE(dstvelo, delta, u);
			VECSUBIN(dstvelo, *pvelo);
			VECSADD(*pvelo, dstvelo, .2 * dt);
	/*		VECSUB(delta, dstvelo, p->velo);
			VECCPY(p->velo, dstvelo);*/
	/*		VECSADD(p->velo, delta, dt * 1e-8 * (1e0 + VECLEN(p->velo)));*/
		}
		if(!p->warpcs && w->cs != p->warpdstcs && w->cs->csrad * w->cs->csrad < VECSLEN(pt->pos)){
			int i;
			p->warpcs = new CoordSys("warpbubble", w->cs);//malloc(sizeof *p->warpcs + sizeof *p->warpcs->w);
			p->warpcs->pos = pt->pos;
			p->warpcs->velo = pt->velo;
/*			VECNULL(pt->velo);*/
			p->warpcs->csrad = pt->hitradius() * 10.;
			p->warpcs->flags = 0;
			p->warpcs->w = new WarField(p->warpcs);
			p->warpcs->w->pl = w->pl;
/*			for(i = 0; i < npf; i++) if(pf[i]){
				ImmobilizeTefpol3D(pf[i]);
				pf[i] = NULL;
			}*/
			transit_cs(p->warpcs);
#if 0
			/* TODO: bring docked objects along */
			if(pt->vft == &scarry_s){
				entity_t **ppt2;
				for(ppt2 = &w->tl; *ppt2; ppt2 = &(*ppt2)->next){
					entity_t *pt2 = *ppt2;
					if(pt2->vft == &scepter_s && ((scepter_t*)pt2)->mother == p){
						if(((scepter_t*)*ppt2)->docked)
							transit_cs(pt2, w, p->warpcs);
						else
							((scepter_t*)*ppt2)->mother = NULL;
/*						if(((scepter_t*)*ppt2)->docked){
							transit_cs
							*ppt2 = pt2->next;
							pt2->next = p->warpcs->w->tl;
							p->warpcs->w->tl = pt2;
						}*/
					}
				}
			}
#endif
		}
		else{
			if(w->cs == p->warpcs && dstcspos.slen() < p->warpdstcs->csrad * p->warpdstcs->csrad)
				warp_collapse();
		}

		if(p->warpcs){
			p->currentWarpDist += VECLEN(p->warpcs->velo) * dt;
			VECSADD(p->warpcs->pos, p->warpcs->velo, dt);
		}
		pos += this->velo * dt;
	}
	else{ /* Regenerate energy in capacitor only unless warping */
		double gen = dt * g_capacitor_gen_factor * mn->capacitor_gen;
		if(p->capacitor + gen < mn->capacity)
			p->capacitor += gen;
		else
			p->capacitor = mn->capacity;

		maneuver(mat, dt, mn);

		pos += velo * dt;
		rot = rot.quatrotquat(omg * dt);
	/*	VECSCALEIN(pt->omg, 1. / (dt * .4 + 1.));
		VECSCALEIN(pt->velo, 1. / (dt * .01 + 1.));*/
	}
	st::anim(dt);
}
#if 0
static int warpable_popup_menu(const entity_t *pt, char ***titles, int **keys, char ***cmds, int *pnum){
	int i;
	int num;
	int acty = 1;
	char *titles1[] = {"Warp To..."};
	char *cmds1[] = {"Warp To..."};
	for(i = 0; i < *pnum; i++) if(!strcmp((*titles)[i], titles1[0]) && !strcmp((*cmds)[i], cmds1[0]))
		acty = 0;
	if(!acty)
		return 0;
	i = *pnum;
	num = *pnum += 2;
	*titles = realloc(*titles, num * sizeof **titles);
	*keys = realloc(*keys, num * sizeof **keys);
	*cmds = realloc(*cmds, num * sizeof **cmds);
	(*titles)[i] = glwMenuSeparator;
	(*keys)[i] = '\0';
	(*cmds)[i] = NULL;
	i++;
	(*titles)[i] = "Warp To...";
	(*keys)[i] = '\0';
	(*cmds)[i] = "togglewarpmenu";
	return 0;
}

int warpable_dest(entity_t *pt, avec3_t ret, coordsys *cs){
	if((pt->vft == &scarry_s || pt->vft == &beamer_s || pt->vft == &assault_s) && ((warpable_t*)pt)->warping){
		tocs(ret, cs, ((warpable_t*)pt)->warpdst, ((warpable_t*)pt)->warpdstcs);
		return 1;
	}
	return 0;
}
#endif

double Beamer::hitradius(){return .1;}
const maneuve &Beamer::getManeuve()const{return beamer_mn;}
void Beamer::anim(double dt){
	Mat4d mat;

	if(!w){
	}

	if(shield < dt)
		shield = 0.;
	else
		shield -= dt;

	/* shield regeneration */
	if(0 < health){
		if(MAX_SHIELD_AMOUNT < shieldAmount + dt * 5.)
			shieldAmount = MAX_SHIELD_AMOUNT;
		else
			shieldAmount += dt * 5.;
	}

//	animShieldWavelet(pt, &p->sw, dt);

	/* forget about beaten enemy */
	if(enemy && enemy->health <= 0.)
		enemy = NULL;

/*	if(p->dock && !p->undocktime){
		beamer_undock(p, p->dock);
		return;
	}*/

//	tankrot(&mat, pt);
	mat = Mat4d(mat4_u).translatein(pos) * rot.tomat4();

#if 0
	if(0 < pt->health){
		double oldyaw = pt->pyr[1];
		entity_t *collideignore = NULL;
		int i, n;
		if(p->dock && p->undocktime){
			pt->inputs.press = PL_W;

			if(p->undocktime <= dt){
				p->undocktime = 0.;
				p->dock = NULL;
			}
			else
				p->undocktime -= dt;
		}
		else if(w->pl->control == pt){
		}
		else{
			double pos[3], dv[3], dist;
			avec3_t opos;
			pt->inputs.press = 0;
			if(!pt->enemy){ /* find target */
				double best = 20. * 20.;
				entity_t *t;
				for(t = w->tl; t; t = t->next) if(t != pt && t->race != -1 && t->race != pt->race && 0. < t->health && t->vft != &rstation_s){
					double sdist = VECSDIST(pt->pos, t->pos);
					if(sdist < best){
						pt->enemy = t;
						best = sdist;
					}
				}
			}

			if(p->dock && p->undocktime == 0){
				if(pt->enemy)
					beamer_undock(p, p->dock);
			}
			else{
				if(pt->enemy){
					avec3_t dv, forward;
					avec3_t xh, yh;
					long double sx, sy, len, len2, maxspeed = BEAMER_MAX_ANGLESPEED * dt;
					aquat_t qres, qrot;
					VECSUB(dv, pt->enemy->pos, pt->pos);
					VECNORMIN(dv);
					quatrot(forward, pt->rot, avec3_001);
					VECSCALEIN(forward, -1);
	/*				sx = VECSP(&mat[0], dv);
					sy = VECSP(&mat[4], dv);
					pt->inputs.press |= (sx < 0 ? PL_4 : 0 < sx ? PL_6 : 0) | (sy < 0 ? PL_2 : 0 < sy ? PL_8 : 0);*/
					VECVP(xh, forward, dv);
					len = len2 = VECLEN(xh);
					len = asinl(len);
					if(maxspeed < len){
						len = maxspeed;
					}
					len = sinl(len / 2.);
					if(len && len2){
						double sd, df, drl, decay;
						avec3_t ptomg, omg, dvv, delta, diff;
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
						df = dt * MIN(df, beamer_mn.angleaccel);
						VECSCALEIN(omg, 1. - df);
						VECADD(pt->omg, omg, ptomg);
						len2 = VECLEN(xh);*/
						df = dt * MIN(len2, beamer_mn.angleaccel) / len2;
						df = MIN(df, 1);
						VECSUB(delta, diff, pt->omg);
						VECSADD(delta, p->integral, .2);
						VECSADD(pt->omg, delta, df);
						if(beamer_mn.maxanglespeed * beamer_mn.maxanglespeed < VECSLEN(pt->omg)){
							VECNORMIN(pt->omg);
							VECSCALEIN(pt->omg, beamer_mn.maxanglespeed);
						}
						if(.25 < len2)
							VECSCALEIN(diff, .25 / len2);
						VECSADD(p->integral, diff, dt);
						decay = exp(-.1 * dt);
						VECSCALEIN(p->integral, decay);
						pt->inputs.press |= PL_2 | PL_8;
					}
					if(.9 < VECSP(dv, forward)){
						pt->inputs.change |= PL_ENTER;
						pt->inputs.press |= PL_ENTER;
						if(5. * 5. < VECSDIST(pt->enemy->pos, pt->pos))
							pt->inputs.press |= PL_W;
						else if(VECSDIST(pt->enemy->pos, pt->pos) < 1. * 1.)
							pt->inputs.press |= PL_S;
/*						else
							pt->inputs.press |= PL_A; *//* strafe around */
					}
				}

				/* follow another */
				if(0 && !pt->enemy && !pw->warping){
					entity_t *pt2;
					beamer_t *leader = NULL;
					for(pt2 = w->tl; pt2; pt2 = pt2->next) if(pt != pt2 && pt2->vft == pt->vft && w->pl->control == pt2){
						leader = (warpable_t*)pt2;
						break;
					}

					if(leader && leader->charge){
						pt->inputs.change |= PL_ENTER;
						pt->inputs.press |= PL_ENTER;
					}

					/*for(pt2 = w->tl; pt2; pt2 = pt2->next) if(pt != pt2 && pt2->vft == pt->vft && w->pl->control == pt2 && ((beamer_t*)pt2)->warping)*/
					if(leader && leader->st.warping && leader->st.warpdstcs != w->cs){
						warpable_t *p2 = &leader->st;
						int k;
						pw->warping = p2->warping;
						pw->warpcs = NULL;
						VECCPY(pw->warpdst, p2->warpdst);
						for(k = 0; k < 3; k++)
							pw->warpdst[k] += drseq(&w->rs) - .5;
						pw->warpdstcs = p2->warpdstcs;
						pw->warp_next_warf = NULL;
	/*					break;*/
					}
				}
			}
		}

#endif
		if(cooldown == 0. && inputs.change & inputs.press & (PL_ENTER | PL_LCLICK)){
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

#if 0
		if(!p->dock){
			space_collide(pt, w, dt, NULL, NULL);
		}
	}
	else
		pt->active = 0;
#endif
	if(0. < charge && charge < 4.){
		Entity *pt2, *hit = NULL;
		Vec3d start, dir, start0(0., 0., -.04), dir0(0., 0., -10.);
		double best = 10., sdist;
		int besthitpart = 0, hitpart;
		start = rot.trans(start0) + pos;
		dir = rot.trans(dir0);
		for(pt2 = w->el; pt2; pt2 = pt2->next){
			double rad = pt2->hitradius();
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
		if(hit){
			Vec3d pos;
			Quatd qrot;
			beamlen = best;
			hit->takedamage(500. * dt, besthitpart);
			pos = mat.vec3(2) * -best + this->pos;
			quatdirection(qrot, &mat[8]);
			if(drseq(&w->rs) * .1 < dt){
				avec3_t velo;
				int i;
				for(i = 0; i < 3; i++)
					velo[i] = (drseq(&w->rs) - .5) * .1;
//				AddTeline3D(w->tell, pos, velo, drseq(&w->rs) * .01 + .01, NULL, NULL, NULL, COLOR32RGBA(0,127,255,95), TEL3_NOLINE | TEL3_GLOW | TEL3_INVROTATE, .5);
			}
//			AddTeline3D(w->tell, pos, NULL, drseq(&w->rs) * .25 + .25, qrot, NULL, NULL, COLOR32RGBA(0,255,255,255), TEL3_NOLINE | TEL3_CYLINDER | TEL3_QUAT, .1);
		}
		else
			beamlen = 10.;
	}
#if 1
	Warpable::anim(dt);
#endif
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

#if 0
static int beamer_cull(entity_t *pt, wardraw_t *wd){
	double pixels;
	if(glcullFrustum(&pt->pos, .6, wd->pgc))
		return 1;
	pixels = .8 * fabs(glcullScale(&pt->pos, wd->pgc));
	if(pixels < 2)
		return 1;
	return 0;
}

static const double beamer_sc[3] = {.05, .055, .075};
/*static const double beamer_sc[3] = {.05, .05, .05};*/
static struct hitbox beamer_hb[] = {
	{{0., 0., -.02}, {0,0,0,1}, {.015, .015, .075}},
	{{.025, -.015, .02}, {0,0, -SIN15, COS15}, {.0075, .002, .02}},
	{{-.025, -.015, .02}, {0,0, SIN15, COS15}, {.0075, .002, .02}},
	{{.0, .03, .0325}, {0,0,0,1}, {.002, .008, .010}},
};
#endif

static const suftexparam_t defstp = {
	NULL, NULL,  // const BITMAPINFO *bmi;
	GL_MODULATE, // GLuint env;
	GL_LINEAR,   // GLuint magfil;
	0, 1,        // int mipmap, alphamap;
};

GLuint CallCacheBitmap5(const char *entry, const char *fname1, suftexparam_t *pstp1, const char *fname2, suftexparam_t *pstp2){
	const struct suftexcache *stc;
	suftexparam_t stp, stp2;
	BITMAPFILEHEADER *bfh, *bfh2;
	GLuint ret;

	stc = FindTexCache(entry);
	if(stc)
		return stc->list;
	stp = pstp1 ? *pstp1 : defstp;
	bfh = (BITMAPFILEHEADER*)ZipUnZip("rc.zip", fname1, NULL);
	stp.bmi = !bfh ? ReadBitmap(fname1) : (BITMAPINFO*)&bfh[1];
	if(!stp.bmi)
		return 0;
	stp2 = pstp2 ? *pstp2 : defstp;
	if(fname2){
		bfh2 = (BITMAPFILEHEADER*)ZipUnZip("rc.zip", fname2, NULL);
		stp2.bmi = !bfh2 ? ReadBitmap(fname2) : (BITMAPINFO*)&bfh2[1];
		if(!stp2.bmi)
			return 0;
	}
	ret = CacheSUFMTex(entry, &stp, fname2 ? &stp2 : NULL);
	if(bfh)
		ZipFree(bfh);
	else
		LocalFree((HLOCAL)stp.bmi);
	if(fname2){
		if(bfh)
			ZipFree(bfh2);
		else
			LocalFree((HLOCAL)stp2.bmi);
	}
	return ret;
}

GLuint CallCacheBitmap(const char *entry, const char *fname1, suftexparam_t *pstp, const char *fname2){
	return CallCacheBitmap5(entry, fname1, pstp, fname2, pstp);
}

static void cache_bridge(void){
	CallCacheBitmap("bridge.bmp", "bridge.bmp", NULL, NULL);
	CallCacheBitmap("beamer_panel.bmp", "beamer_panel.bmp", NULL, NULL);
/*	if(!FindTexCache("bridge.bmp")){
		suftexparam_t stp;
		stp.bmi = lzuc(lzw_bridge, sizeof lzw_bridge, NULL);
		stp.env = GL_MODULATE;
		stp.mipmap = 0;
		stp.alphamap = 0;
		stp.magfil = GL_LINEAR;
		CacheSUFMTex("bridge.bmp", &stp, NULL);
		free(stp.bmi);
	}
	if(!FindTexCache("beamer_panel.bmp")){
		BITMAPINFO *bmi;
		bmi = lzuc(lzw_beamer_panel, sizeof lzw_beamer_panel, NULL);
		CacheSUFTex("beamer_panel.bmp", bmi, 0);
		free(bmi);
	}*/
}

suf_t *CallLoadSUF(const char *fname){
	suf_t *ret;
	ret = (suf_t*)ZipUnZip("rc.zip", fname, NULL);
	if(!ret)
		ret = LoadSUF(fname);
	if(!ret)
		return NULL;
	return RelocateSUF(ret);
}

suf_t *Beamer::sufbase = NULL;

void Beamer::draw(wardraw_t *wd){
	Beamer *const p = this;
	static int init = 0;
	static suftex_t *pst;
	if(!w)
		return;

	/* cull object */
/*	if(beamer_cull(pt, wd))
		return;*/
//	wd->lightdraws++;

	draw_healthbar(this, wd, health / BEAMER_HEALTH, .1, shieldAmount / MAX_SHIELD_AMOUNT, capacitor / beamer_mn.capacity);

	if(init == 0) do{
//		suftexparam_t stp, stp2;
/*		beamer_s.sufbase = LZUC(lzw_assault);*/
/*		beamer_s.sufbase = LZUC(lzw_beamer);*/
		sufbase = CallLoadSUF("beamer.bin");
		if(!sufbase) break;
		cache_bridge();
		pst = AllocSUFTex(sufbase);
/*		beamer_hb[1].rot[2] = sin(M_PI / 3.);
		beamer_hb[1].rot[3] = cos(M_PI / 3.);
		beamer_hb[2].rot[2] = sin(-M_PI / 3.);
		beamer_hb[2].rot[3] = cos(-M_PI / 3.);*/
		init = 1;
	} while(0);
	if(sufbase){
		static const double normal[3] = {0., 1., 0.};
		double scale = BEAMER_SCALE;
//		double x;
//		int i;
/*		static const GLdouble rotaxis[16] = {
			0,0,-1,0,
			-1,0,0,0,
			0,1,0,0,
			0,0,0,1,
		};*/
		static const GLdouble rotaxis[16] = {
			-1,0,0,0,
			0,1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};
		Mat4d mat;
/*		glBegin(GL_POINTS);
		glVertex3dv(pt->pos);
		glEnd();*/

		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);

#if 0
		for(i = 0; i < numof(beamer_hb); i++){
			amat4_t rot;
			glPushMatrix();
			gldTranslate3dv(beamer_hb[i].org);
			quat2mat(rot, beamer_hb[i].rot);
			glMultMatrixd(rot);
			hitbox_draw(pt, beamer_hb[i].sc);
			glPopMatrix();

			glPushMatrix();
			gldTranslate3dv(beamer_hb[i].org);
			quat2imat(rot, beamer_hb[i].rot);
			glMultMatrixd(rot);
			hitbox_draw(pt, beamer_hb[i].sc);
			glPopMatrix();
		}
#endif

/*		glTranslated(pt->pos[0], pt->pos[1], pt->pos[2]);
		glRotated(deg_per_rad * pt->pyr[1], 0., -1., 0.);
		glRotated(deg_per_rad * pt->pyr[0], -1.,  0., 0.);
		glRotated(deg_per_rad * pt->pyr[2], 0.,  0., -1.);*/
		glPushMatrix();
		glScaled(scale, scale, scale);
		glMultMatrixd(rotaxis);
/*		glPushAttrib(GL_TEXTURE_BIT);*/
/*		DrawSUF(beamer_s.sufbase, SUF_ATR, &g_gldcache);*/
		DecalDrawSUF(sufbase, SUF_ATR, NULL, pst, NULL, NULL);
/*		DecalDrawSUF(&suf_beamer, SUF_ATR, &g_gldcache, beamer_s.tex, pf->sd, &beamer_s);*/
/*		glPopAttrib();*/
		glPopMatrix();

/*		for(i = 0; i < numof(pf->turrets); i++)
			mturret_draw(&pf->turrets[i], pt, wd);*/
		glPopMatrix();

/*		if(0 < wd->light[1]){
			static const double normal[3] = {0., 1., 0.};
			ShadowSUF(&suf_beamer, wd->light, normal, pt->pos, pt->pyr, scale, rotaxis);
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

void Beamer::drawtra(wardraw_t *wd){
	Beamer *p = this;
	Mat4d mat;

/*	if(p->dock && p->undocktime == 0)
		return;*/

	transform(mat);

#if 1
	if(!wd->vw->gc->cullFrustum(pos, .1)){
		glColor4ub(255,255,9,255);
		glBegin(GL_LINES);
		glVertex3dv(mat.vp3(Vec3d(.001,0,0)));
		glVertex3dv(mat.vp3(Vec3d(-.001,0,0)));
		glEnd();
		{
			int i, n = 3;
			static const avec3_t pos0[] = {
				{0, 210 * BEAMER_SCALE, 240 * BEAMER_SCALE},
				{190 * BEAMER_SCALE, -120 * BEAMER_SCALE, 240 * BEAMER_SCALE},
				{-190 * BEAMER_SCALE, -120 * BEAMER_SCALE, 240 * BEAMER_SCALE},
				{0, .002 + 50 * BEAMER_SCALE, -60 * BEAMER_SCALE},
				{0, .002 + 60 * BEAMER_SCALE, 40 * BEAMER_SCALE},
				{0, -.002 + -50 * BEAMER_SCALE, -60 * BEAMER_SCALE},
				{0, -.002 + -60 * BEAMER_SCALE, 40 * BEAMER_SCALE},
			};
			avec3_t pos;
			double rad = .002;
			GLubyte col[4] = {255, 127, 127, 255};
			random_sequence rs;
			init_rseq(&rs, (unsigned long)this);
			if(fmod(wd->vw->viewtime + drseq(&rs) * 2., 2.) < .2){
				col[1] = col[2] = 255;
				n = numof(pos0);
				rad = .003;
			}
			for(i = 0 ; i < n; i++){
				mat4vp3(pos, mat, pos0[i]);
				gldSpriteGlow(pos, rad, col, wd->vw->irot);
			}
		}

		/* shield effect */
/*		if(0. < p->shieldAmount && 0. < p->shield){
			GLubyte col[4] = {0,127,255,255};
			avec3_t dr;
			amat4_t irot;
			col[0] = 128 * (1. - p->shieldAmount / MAX_SHIELD_AMOUNT);
			col[2] = 255 * (p->shieldAmount / MAX_SHIELD_AMOUNT);
			col[3] = 128 * (1. - 1. / (1. + p->shield));
			VECSUB(dr, wd->vw->pos, pt->pos);
			gldLookatMatrix(irot, dr);
			drawShieldSphere(pt->pos, wd->vw->pos, BEAMER_SHIELDRAD, col, irot);
		}

		drawShieldWavelets(pt, p->sw, BEAMER_SHIELDRAD);*/
	}
#endif
#if 1
	if(charge){
		int i;
		GLubyte azure[4] = {63,0,255,95}, bright[4] = {127,63,255,255};
		Vec3d muzzle, muzzle0(0., 0., -.100);
		double glowrad;
		muzzle = mat.vp3(muzzle0);
/*		quatrot(muzzle, pt->rot, muzzle0);
		VECADDIN(muzzle, pt->pos);*/

		if(0. < charge && charge < 4.){
			double beamrad;
			Vec3d end, end0(0., 0., -10.);
			end0[2] = -beamlen;
			end = mat.vp3(end0);
/*			quatrot(end, pt->rot, end0);
			VECADDIN(end, pt->pos);*/
			beamrad = (charge < .5 ? charge / .5 : 3.5 < charge ? (4. - charge) / .5 : 1.) * .005;
			for(i = 0; i < 5; i++){
				Vec3d p0, p1;
				p0 = muzzle * i / 5.;
				p0 += end * (5 - i) / 5.;
				p1 = muzzle * (i + 1) / 5.;
				p1 += end * (5 - i - 1) / 5.;
				glColor4ub(63,191,255, GLubyte(MIN(1., charge / 2.) * 255));
				gldBeam(wd->vw->pos, p0, p1, beamrad);
				glColor4ub(63,0,255, GLubyte(MIN(1., charge / 2.) * 95));
				gldBeam(wd->vw->pos, p0, p1, beamrad * 5.);
			}
#if 0
			for(i = 0; i < 3; i++){
				int j;
				avec3_t pos1, pos;
				struct random_sequence rs;
				struct gldBeamsData bd;
				init_rseq(&rs, (unsigned long)pt + i);
				bd.cc = 0;
				bd.solid = 0;
				glColor4ub(63,0,255, MIN(1., p->charge / 2.) * 95);
				VECCPY(pos1, pt->pos);
				pos1[0] += (drseq(&rs) - .5) * .5;
				pos1[1] += (drseq(&rs) - .5) * .5;
				pos1[2] += (drseq(&rs) - .5) * .5;
				init_rseq(&rs, *(unsigned long*)&wd->gametime + i);
				for(j = 0; j <= 64; j++){
					int c;
					avec3_t p1;
					for(c = 0; c < 3; c++)
						p1[c] = (muzzle[c] * j + end[c] * (64 - j)) / 64. + (drseq(&rs) - .5) * .05;
/*					glColor4ub(63,127,255, MIN(1., p->charge / 2.) * 191);*/
					gldBeams(&bd, wd->view, p1, beamrad * .5, COLOR32RGBA(63,127,255,MIN(1., p->charge / 2.) * 191));
				}
			}
#endif
		}

#if 0
		if(0) for(i = 0; i < 64; i++){
			int j;
			avec3_t pos1, pos;
			struct random_sequence rs;
			struct gldBeamsData bd;
			init_rseq(&rs, (unsigned long)pt + i);
			bd.cc = 0;
			bd.solid = 0;
			glColor4ub(63,0,255, MIN(1., p->charge / 2.) * 95);
			VECCPY(pos1, pt->pos);
			pos1[0] += (drseq(&rs) - .5) * .5;
			pos1[1] += (drseq(&rs) - .5) * .5;
			pos1[2] += (drseq(&rs) - .5) * .5;
			init_rseq(&rs, *(unsigned long*)&wd->gametime + i);
			for(j = 0; j < 16; j++){
				int c;
				for(c = 0; c < 3; c++)
					pos[c] = (pt->pos[c] * j + pos1[c] * (16 - j)) / 16. + (drseq(&rs) - .5) * .01;
				gldBeams(&bd, wd->view, pos, .001, COLOR32RGBA(255, j * 256 / 16, j * 64 / 16, MIN(1., p->charge / 2.) * j * 128 / 16));
			}
		}
#endif
		glowrad = charge < 4. ? charge / 4. * .02 : (6. - charge) / 2. * .02;
		gldSpriteGlow(muzzle, glowrad * 3., azure, wd->vw->irot);
		gldSpriteGlow(muzzle, glowrad, bright, wd->vw->irot);
	}
#endif
}

#if 0
static void beamer_gib_draw(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	int i = (int)pv;
	suf_t *suf;
	double scalex, scaley;
	struct random_sequence rs;
	init_rseq(&rs, (unsigned long)pl);
	scalex = (drseq(&rs) + .5) * .1 * pl->len;
	scaley = (drseq(&rs) + .5) * 1. * pl->len;
	glPushAttrib(GL_CURRENT_BIT | GL_LIGHTING_BIT | GL_ENABLE_BIT);
	glDisable(GL_CULL_FACE);
	glPushMatrix();
	gldTranslate3dv(pl->pos);
	gldMultQuat(pl->rot);
	glBegin(GL_QUADS);
	glNormal3d(0, 0, 1.);
	glVertex2d(-scalex, -scaley);
	glVertex2d( scalex, -scaley);
	glVertex2d( scalex,  scaley);
	glVertex2d(-scalex,  scaley);
	glEnd();
	glPopMatrix();
	glPopAttrib();
}


static int beamer_takedamage(entity_t *pt, double damage, warf_t *w, int hitpart){
	beamer_t *p = (beamer_t*)pt;
	struct tent3d_line_list *tell = w->tell;
	int ret = 1;

	if(hitpart == 1000){
		p->shield = MIN(p->shield + damage * .05, 5.);
		if(damage < p->shieldAmount)
			p->shieldAmount -= damage;
		else
			p->shieldAmount = -50.;
		return 1;
	}
	else if(p->shieldAmount < 0.)
		p->shieldAmount = -50.;

/*	if(tell){
		int j, n;
		frexp(damage, &n);
		for(j = 0; j < n; j++){
			double velo[3];
			velo[0] = .15 * (drseq(&w->rs) - .5);
			velo[1] = .15 * (drseq(&w->rs) - .5);
			velo[2] = .15 * (drseq(&w->rs) - .5);
			AddTeline3D(tell, pt->pos, velo, .001, NULL, NULL, w->gravity,
				j % 2 ? COLOR32RGBA(255,255,255,255) : COLOR32RGBA(255,191,63,255),
				TEL3_HEADFORWARD | TEL3_REFLECT | TEL3_FADEEND, .5 + drseq(&w->rs) * .5);
		}
	}*/
	playWave3D("hit.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
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
			AddTeline3D(w->tell, pos, velo, .005, NULL, NULL, NULL, COLOR32RGBA(255, 31, 0, 255), TEL3_HEADFORWARD | TEL3_THICK | TEL3_FADEEND, 15. + drseq(&w->rs) * 5.);
		}

		if(w->gibs) for(i = 0; i < 32; i++){
			double pos[3], velo[3], omg[3];
			/* gaussian spread is desired */
			velo[0] = .025 * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
			velo[1] = .025 * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
			velo[2] = .025 * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
			omg[0] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
			omg[1] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
			omg[2] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
			VECCPY(pos, pt->pos);
			VECSADD(pos, velo, .1);
			AddTelineCallback3D(w->gibs, pos, velo, .010, NULL, omg, NULL, beamer_gib_draw, NULL, TEL3_QUAT | TEL3_NOLINE, 15. + drseq(&w->rs) * 5.);
		}

		/* smokes */
		for(i = 0; i < 32; i++){
			double pos[3], velo[3];
			COLOR32 col = 0;
			VECCPY(pos, pt->pos);
			pos[0] += .1 * (drseq(&w->rs) - .5);
			pos[1] += .1 * (drseq(&w->rs) - .5);
			pos[2] += .1 * (drseq(&w->rs) - .5);
			col |= COLOR32RGBA(rseq(&w->rs) % 32 + 127,0,0,0);
			col |= COLOR32RGBA(0,rseq(&w->rs) % 32 + 127,0,0);
			col |= COLOR32RGBA(0,0,rseq(&w->rs) % 32 + 127,0);
			col |= COLOR32RGBA(0,0,0,191);
			AddTeline3D(w->tell, pos, NULL, .035, NULL, NULL, w->gravity, col, TEL3_NOLINE | TEL3_GLOW | TEL3_INVROTATE, 60.);
		}

		{/* explode shockwave thingie */
			static const double pyr[3] = {M_PI / 2., 0., 0.};
			amat3_t ort;
			avec3_t dr, v;
			aquat_t q;
			amat4_t mat;
			double p;
			w->vft->orientation(w, &ort, &pt->pos);
			VECCPY(dr, &ort[3]);

			/* half-angle formula of trigonometry replaces expensive tri-functions to square root */
			q[3] = sqrt((dr[2] + 1.) / 2.) /*cos(acos(dr[2]) / 2.)*/;

			VECVP(v, avec3_001, dr);
			p = sqrt(1. - q[3] * q[3]) / VECLEN(v);
			VECSCALE(q, v, p);

			AddTeline3D(tell, pt->pos, NULL, 5., q, NULL, NULL, COLOR32RGBA(255,191,63,255), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_QUAT, 1.);
			AddTeline3D(tell, pt->pos, NULL, 2., NULL, NULL, NULL, COLOR32RGBA(255,255,255,127), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_INVROTATE, 1.);
		}
		playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
		pt->active = 0;
	}
	pt->health -= damage;
	return ret;
}

/*static void beamer_gib_draw(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	int i = (int)pv;
	suf_t *suf;
	double scale;
	if(i < beamer_s.sufbase->np){
		suf = beamer_s.sufbase;
		scale = .0001;
		gib_draw(pl, suf, scale, i);
	}
}*/

static void beamer_bullethit(entity_t *pt, warf_t *w, const bullet_t *pb){
	beamer_t *p = (beamer_t*)pt;
	if(pb->damage < p->shieldAmount){
		double pos[3], velo[3];
		double pi;
		avec3_t dr, v;
		aquat_t q;

		VECSUB(dr, pb->pos, pt->pos);
		VECNORMIN(dr);

		/* half-angle formula of trigonometry replaces expensive tri-functions to square root */
		q[3] = sqrt((dr[2] + 1.) / 2.) /*cos(acos(dr[2]) / 2.)*/;

		VECVP(v, avec3_001, dr);
		pi = sqrt(1. - q[3] * q[3]) / VECLEN(v);
		VECSCALE(q, v, pi);

		VECCPY(pos, pb->pos);
		VECCPY(velo, pt->velo);

		ShieldWaveletAlloc(&p->sw, q, MIN(pb->damage * .05 + .2, 5.));

	}
}

static int beamer_tracehit(entity_t *pt, warf_t *w, const double src[3], const double dir[3], double rad, double dt, double *ret, double (*retp)[3], double (*retn)[3]){
	beamer_t *p = (beamer_t*)pt;
	double sc[3];
	double best = dt, retf;
	int reti = 0, i, n;
	if(0 < p->shieldAmount){
		if(jHitSpherePos(pt->pos, BEAMER_SHIELDRAD + rad, src, dir, dt, ret, *retp))
			return 1000; /* something quite unlikely to reach */
	}
	for(n = 0; n < numof(beamer_hb); n++){
		avec3_t org;
		aquat_t rot;
		quatirot(org, pt->rot, beamer_hb[n].org);
		VECADDIN(org, pt->pos);
		QUATMUL(rot, pt->rot, beamer_hb[n].rot);
		for(i = 0; i < 3; i++)
			sc[i] = beamer_hb[n].sc[i] + rad;
		if((jHitBox(org, sc, rot, src, dir, 0., best, &retf, retp, retn)) && (retf < best)){
			best = retf;
			if(ret) *ret = retf;
			reti = i + 1;
		}
	}
	return reti;
}
#endif

