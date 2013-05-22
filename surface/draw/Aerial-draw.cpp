/** \file
 * \brief Definition of base classes for aerial Entities such as aeroplanes.
 */

#include "../Aerial.h"
#include "Player.h"
#include "yssurf.h"
#include "arms.h"
#include "sqadapt.h"
#include "Game.h"
#include "tefpol3d.h"
#include "motion.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/mqoadapt.h"
extern "C"{
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
#include <clib/suf/sufbin.h>
}
#include <cpplib/crc32.h>

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


#if 0
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
#endif

wardraw_t *g_smoke_wd;

static suf_t *sufleg = NULL;

Model *Aerial::model;

void Aerial::draw(WarDraw *wd){

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
#endif

	static OpenGLState::weak_ptr<bool> init;
	if(!init){
		model = LoadMQOModel("models/Aerial.mqo");

		init.create(*openGLState);
	}

/*	if(!vft->sufbase){
		double pos[3];
		GLubyte col[4] = {255,255,0,255};
		gldPseudoSphere(pos, fly_radius, col);
	}
	else*/
	if(model){
//		avec3_t *points = pt->vft == &valkie_s ? valkie_points : fly_points;
		static const double normal[3] = {0., 1., 0.};
		double scale = FLY_SCALE;
		double x;
		int lod = 0;
		int i;
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
			Mat4d mat;
			transform(mat);
/*			MAT3TO4(mat, pt->rot);*/
			glMultMatrixd(mat);
		}

#else
		glTranslated(pt->pos[0], pt->pos[1], pt->pos[2]);
		glRotated(deg_per_rad * pt->pyr[1], 0., -1., 0.);
		glRotated(deg_per_rad * pt->pyr[0], -1.,  0., 0.);
		glRotated(deg_per_rad * pt->pyr[2], 0.,  0., -1.);
#endif

#if 0
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
		else
#endif
			if(1){
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
			DrawMQOPose(model, nullptr);
//			DecalDrawSUF(vft->sufbase, SUF_ATR, &g_gldcache, NULL, NULL/*p->sd*/, &fly_s);
//			glScaled(1,1,-1);
//			glPushAttrib(GL_POLYGON_BIT);
//			glFrontFace(GL_CW);
//			DecalDrawSUF(vft->sufbase, SUF_ATR, &g_gldcache, NULL, NULL/*p->sd*/, &fly_s);
			glPopAttrib();
#endif
			glPopMatrix();
		}
		else{
//			DecalDrawSUF(lod ? vft->sufbase : &suf_fly1, SUF_ATR, &g_gldcache, NULL, p->sd, &fly_s);
		}

#if 0
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
#endif

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

#if 0
void fly_draw(entity_t *pt, wardraw_t *wd){
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
#endif

#if 0
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
#endif

void drawnavlight(const Vec3d &pos, const Vec3d &org, double rad, const GLubyte (*col)[4], const double (*irot)[16], wardraw_t *wd){
	double pixels, sf;
	if(wd->vw->gc->cullFrustum(pos, rad))
		return;
	pixels = rad * fabs(wd->vw->gc->scale(pos));
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
		gldSpriteGlow(pos, rad * sf, *col, *irot);
}

void drawmuzzleflash(const Vec3d &pos, const Vec3d &org, double rad, const Mat4d &irot){
	double (*cuts)[2];
	int i;
	cuts = CircleCuts(10);
	RandomSequence rs(crc32(pos, sizeof pos));
	glPushMatrix();
	gldTranslate3dv(pos);
	glMultMatrixd(irot);
	gldScaled(rad);
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

#if 0
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
#endif

void Aerial::drawtra(WarDraw *wd){
#if 0
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
#endif
}

/*
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
*/

void Flare::drawtra(WarDraw *wd){
	GLubyte white[4] = {255,255,255,255}, red[4] = {255,127,127,127};
	double len = /*((flare_t*)pt)->ttl < 1. ? ((flare_t*)pt)->ttl :*/ 1.;
	RandomSequence rs(crc32(&game->universe->global_time, sizeof game->universe->global_time, crc32(this, sizeof this)));
	red[3] = 127 + rseq(&rs) % 128;
	double sdist = (wd->vw->pos - this->pos).slen();
	gldSpriteGlow(this->pos, len * (sdist < .1 * .1 ? sdist / .1 * .05 : .05), red, wd->vw->irot);
	if(sdist < 1. * 1.)
		gldSpriteGlow(this->pos, .001, white, wd->vw->irot);
	if(this->pos[1] < .1){
		static const amat4_t mat = { /* pseudo rotation; it simply maps xy plane to xz */
			1,0,0,0,
			0,0,-1,0,
			0,1,0,0,
			0,0,0,1
		};
		Vec3d pos = this->pos;
		pos[1] = 0.;
		red[3] = red[3] * len * (.1 - this->pos[1]) / .1;
		gldSpriteGlow(pos, .10 * this->pos[1] * this->pos[1] / .1 / .1, red, mat);
	}
}

void Aerial::drawHUD(WarDraw *wd){
	char buf[128];
/*	timemeas_t tm;*/
/*	glColor4ub(0,255,0,255);*/

/*	TimeMeasStart(&tm);*/

	st::drawHUD(wd);

	glLoadIdentity();

	glDisable(GL_LINE_SMOOTH);

#if 0
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
#endif

/*	printf("fly_drawHUD %lg\n", TimeMeasLap(&tm));*/
}

void Aerial::drawCockpit(WarDraw *wd){
	static int init = 0;
	static suf_t *sufcockpit = NULL, *sufstick = NULL;
	static ysdnm_t *valcockpit = NULL;
	double scale = .0002 /*wd->pgc->znear / wd->pgc->zfar*/;
	double sonear = scale * wd->vw->gc->getNear();
#if 0
	double wid = sonear * wd->vw->gc->getFov() * wd->vw->gc-> / wd->pgc->res;
	double hei = sonear * wd->vw->gc->getFov() * wd->vw->gc->height / wd->pgc->res;
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
#endif
}
