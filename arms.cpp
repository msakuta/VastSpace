#include "arms.h"
#include "player.h"
#include "astro.h"
#include "viewer.h"
//#include "entity_p.h"
#include "antiglut.h"
#include "cmd.h"
#include "bullet.h"
//#include "aim9.h"
//#include "suflist.h"
//#include "yssurf.h"
//#include "walk.h"
#include "bullet.h"
//#include "warutil.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/gl/gldraw.h>
#include <clib/suf/sufbin.h>
#include <clib/suf/sufdraw.h>
}
#include <limits.h>



double ArmBase::hitradius(){
	return 0.;
}

void ArmBase::align(){
	Mat4d mat;

	/* calculate tr(pb->pos) * pb->pyr * pt->pos to get global coords */
	base->transform(mat);
	pos = mat.vp3(hp->pos);
	velo = base->velo + base->omg.vp(base->rot.trans(hp->pos));
	this->rot = base->rot * hp->rot;
}

#define STURRET_SCALE (.000005 * 3)
#define MTURRET_SCALE (.00005)
#define MTURRET_MAX_GIBS 30
#define MTURRET_BULLETSPEED 2.
#define MTURRET_VARIANCE (.001 * M_PI)
#define MTURRET_INTOLERANCE (M_PI / 20.)
#define MTURRETROTSPEED (.4*M_PI)

suf_t *CallLoadSUF(const char *fname);

suf_t *MTurret::suf_turret = NULL;
suf_t *MTurret::suf_barrel = NULL;


MTurret::MTurret(Entity *abase, const hardpoint_static *ahp) : st(abase, ahp){
	ammo = 1500;
	py[0] = 0;
	py[1] = 0;
}

const char *MTurret::idname()const{
	return "mturret";
};

const char *MTurret::classname()const{
	return "Mounted Turret";
};

void MTurret::draw(wardraw_t *wd){
	MTurret *a = this;
	static suf_t *suf = NULL;
	double scale;
	if(!suf_turret)
		suf_turret = CallLoadSUF("turretz1.bin");
	if(!suf_barrel)
		suf_barrel = CallLoadSUF("barrelz1.bin");

	{
		const double bscale = MTURRET_SCALE;
		static const GLfloat rotaxis2[16] = {
			-1,0,0,0,
			0,1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};

		glPushMatrix();
		gldTranslate3dv(pos);
		gldMultQuat(rot);
		glRotated(deg_per_rad * a->py[1], 0., 1., 0.);
		glPushMatrix();
		gldScaled(bscale);
		glMultMatrixf(rotaxis2);
		DrawSUF(suf_turret, SUF_ATR, NULL);
		glPopMatrix();
/*		if(5 < scale)*/{
			const Vec3d pos(0., .0001 * 3, -0.0002 * 3);
			gldTranslate3dv(pos);
			glRotated(deg_per_rad * a->py[0], 1., 0., 0.);
			gldTranslate3dv(-pos);
			gldScaled(bscale);
			glMultMatrixf(rotaxis2);
			DrawSUF(suf_barrel, SUF_ATR, NULL);
		}
		glPopMatrix();
	}
}

static const double mturret_ofs[3] = {0., 0., -.001};
static const double mturret_range[2][2] = {-M_PI / 8., M_PI / 6, -M_PI, M_PI};

void MTurret::findtarget(Entity *pb, const hardpoint_static *hp){
	MTurret *pt = this;
	WarField *w = pb->w;
	double best = 5. * 5.; /* sense range */
	double sdist;
	Vec3d right(1., 0., 0.), left(-1., 0., 0.), pos;
	Mat4d mat;
	Entity *pt2, *closest = NULL;

	/* obtain matrix that converts enemy position into turret's local coordinate system */
	{
		Quatd q;
		Mat4d mat2, mat3;
		mat2 = hp->rot.cnj().tomat4();
		mat2.vec3(3) = -hp->pos;
		mat3 = pb->rot.cnj().tomat4();
		mat3.vec3(3) = -pb->pos;
		mat = mat2 * mat3;
/*		QUATMUL(q, pb->rot, pt->rot);
		QUATCNJIN(q);
		quat2imat(mat, q);*/
	}

/*			pt->enemy = &head[(i+1)%n];*/
	for(pt2 = w->el; pt2; pt2 = pt2->next){
		Vec3d delta, ldelta;
		double theta, phi;

		if(!(pt2 != pb && pt2->w == w && pt2->health > 0. && pt2->race != -1 && pt2->race != pb->race))
			continue;

/*		if(!entity_visible(pb, pt2))
			continue;*/

/*		VECSUB(delta, pt2->pos, pb->pos);*/
		ldelta = mat.vp3(pt2->pos);
		phi = -atan2(ldelta[2], ldelta[0]);
		theta = atan2(ldelta[1], sqrt(ldelta[0] * ldelta[0] + ldelta[2] * ldelta[2]));

		if(mturret_range[1][0] < phi && phi < mturret_range[1][1] && mturret_range[0][0] < theta && theta < mturret_range[0][1]
			&& (sdist = VECSDIST(pt2->pos, pb->pos)) < best)
		{
			best = sdist;
			closest = pt2;
		}
	}
	if(target){
		Entity **ptarget = &(target);
		if(closest)
			*ptarget = closest;
		else if(*ptarget && 10. * 10. < VECSDIST(pb->pos, (*ptarget)->pos))
			*ptarget = NULL;
	}
}

void MTurret::anim(double dt){
	MTurret *a = this;
	WarField *w = base->w;
	Entity *pt = base;
		Entity **ptarget = &target;
		Vec3d epos; /* estimated enemy position */
		double phi, theta; /* enemy direction */

		/* find enemy logic */
		/*if(!pt->target)*/{
			findtarget(pt, hp);
		}

		if(*ptarget && (w != (*ptarget)->w || (*ptarget)->health <= 0.))
			*ptarget = NULL;

		if(a->mf < dt)
			a->mf = 0.;
		else
			a->mf -= dt;

		/* estimating enemy position */
		if((*ptarget)){
			Vec3d pos, velo, pvelo, gepos;
			Mat4d rot, mat, mat2;

			/* calculate tr(pb->pos) * pb->pyr * pt->pos to get global coords */
			mat = mat4_u;
/*			pyrimat(pt->base, &rot);*/
			rot = hp->rot.cnj().tomat4();
			rot.translatein(-hp->pos[0], -hp->pos[1], -hp->pos[2]);
/*			pyrimat(pb->pyr, &mat);*/
			mat = pt->rot.cnj().tomat4();
			mat2 = rot * mat;
			mat2.translatein(-pt->pos[0], -pt->pos[1], -pt->pos[2]);
			pos = mat2.vp3((*ptarget)->pos);
			velo = mat2.dvp3((*ptarget)->velo);
			pvelo = mat2.dvp3(pt->velo);

/*			if(a->type == arms_mediummissileturret)
				VECCPY(epos, pos);
			else*/
				estimate_pos(epos, pos, velo, vec3_000, pvelo, /*a->type == arms_mediumbeamturret ? 5. :*/ 2., w);
			
			/* epos = -pt->base * -pb->pyr * tr(-pos) * gepos */
/*			pyrimat(pt->base, &rot);
			pyrimat(pb->pyr, &mat);
			MAT4MP(mat2, rot, mat);
			MAT4TRANSLATE(mat2, -pt->pos[0], -pt->pos[1], -pt->pos[2]);
			MAT4VP3(epos, mat2, gepos);*/

			/* these angles are in local coordinates */
			phi = -atan2(epos[0], -(epos[2]));
			theta = atan2(epos[1], sqrt(epos[0] * epos[0] + epos[2] * epos[2]));
/*			shoot_angle(avec3_000, epos, phi, 1., &desired);*/
			a->py[1] = approach(a->py[1] + M_PI, phi + M_PI, MTURRETROTSPEED * dt, 2 * M_PI) - M_PI;
			a->py[0] = rangein(approach(a->py[0] + M_PI, theta + M_PI, MTURRETROTSPEED * dt, 2 * M_PI) - M_PI, mturret_range[0][0], mturret_range[0][1]);

			/* shooter logic */
			while(a->cooldown < dt){
				double yaw = a->py[1];
				double pitch = a->py[0];
				if(fabs(phi - yaw) < MTURRET_INTOLERANCE && fabs(pitch - theta) < MTURRET_INTOLERANCE){
					static const avec3_t forward = {0., 0., -1.};
/*					avec3_t pyr;*/
					struct bullet *pz;
					struct hellfire *pm;
					Quatd qrot;
/*					if(a->type == arms_mediummissileturret){
						struct hellfire *ph;
						ph = add_aim9(w, pt->pos);
						pz = &ph->st;
						pz->owner = pt;
						pz->damage = 100.;
						ph->explo = 200.;
					}
					else*/
//						pz = new Bullet(w, pt, 80.);
/*					pz = &(pm = add_aim9(w, pt->pos))->st;
					pm->st.owner = pb;
					QUATMUL(qrot, pb->rot, pt->rot);
					QUATCPY(pm->rot, qrot);
					VECNULL(pm->st.velo);
					pm->target = pt->target;*/
/*					pz->grav = 0;
					pz->life = 0 || a->type != arms_mediumgunturret ? 10. : VECLEN(epos) / 2. - .1;
					pz->life = MAX(pz->life, .1);*/
/*					VECCPY(pyr, pt->pyr);
					pyr[0] += (drseq(&w->rs) - .5) * MTURRET_VARIANCE;
					pyr[1] += (drseq(&w->rs) - .5) * MTURRET_VARIANCE;*/
#if 0
					/* retrieve pb->pyr * pt->base * pt->pyr */
					tankrot(&mat2, pt);
/*					MAT4IDENTITY(mat);
					MAT4TRANSLATE(mat, pb->pos[0], pb->pos[1], pb->pos[2]);
					pyrmat(pb->pyr, &rot);
					mat4mp(mat2, mat, rot);*/
					mat4translate(mat2, hp->pos[0], hp->pos[1], hp->pos[2]);
/*					pyrmat(pt->base, &rot);*/
					quat2mat(&rot, hp->rot);
					mat4mp(mat, mat2, rot);
					mat4translate(mat, 0., .001, -0.002);
					mat4roty(mat2, mat, yaw = a->py[1] + (drseq(&w->rs) - .5) * MTURRET_VARIANCE);
					mat4rotx(mat, mat2, pitch = a->py[0] + (drseq(&w->rs) - .5) * MTURRET_VARIANCE);
/*					pyrmat(pyr, &rot);
					MAT4MP(mat2, mat, rot);*/
					mat4vp3(pz->pos, mat, mturret_ofs);
					mat4dvp3(pz->velo, mat, forward);
					if(a->type == arms_mediumbeamturret)
						VECSCALEIN(pz->velo, 5.);
					else if(a->type == arms_mediumgunturret)
						VECSCALEIN(pz->velo, 2.);
					else if(a->type == arms_mediummissileturret){
						aquat_t q1, q2, q3, qp, qy;
						QUATMUL(q1, pt->rot, hp->rot);
						qp[0] = sin(pitch / 2.);
						qp[1] = qp[2] = 0;
						qp[3] = cos(pitch / 2.);
						qy[0] = 0.;
						qy[1] = sin(yaw / 2.);
						qy[2] = 0.;
						qy[3] = cos(yaw / 2.);
						QUATMUL(q2, q1, qy);
						QUATMUL(pz->rot, q2, qp);
/*						imat2quat(((struct hellfire*)pz)->rot, mat);*/
						((struct hellfire*)pz)->target = *ptarget;
						VECSCALEIN(pz->velo, .1);
					}

					/* TODO: rotation contribution? */
					VECADDIN(pz->velo, pt->velo);


					playWave3DPitch(CvarGetString("sound_cannonshot")/*"gunfire8.wav"*/, pz->pos, w->pl->pos, NULL, 1., .02, w->realtime, rseq(&w->rs) % 32 + 256 - 16);
					{
						double t = arms_static[a->type].cooldown + (drseq(&w->rs) - .5) * .2;
						pz->vft->anim(pz, w, w->tell, dt - a->cooldown);
						a->cooldown += t;
						a->mf = .2;
					}
#endif
				}
				else
					a->cooldown += .5 + (drseq(&w->rs) - .5) * .2;
			}
		}
		if(a->cooldown < dt)
			a->cooldown = 0.;
		else
			a->cooldown -= dt;


}


void MTurret::drawtra(wardraw_t *wd){
#if 0
	double bscale = MTURRET_SCALE, tscale = .00002;
	if(pt->mf){
		struct random_sequence rs;
		amat4_t mat2, mat, rot;
		avec3_t pos, pos0 = {0, 0, -.008};
		init_rseq(&rs, (long)pt ^ *(long*)&wd->gametime);
		MAT4IDENTITY(mat);
		mat4translate(mat, pb->pos[0], pb->pos[1], pb->pos[2]);
		quat2mat(rot, pb->rot);
		mat4mp(mat2, mat, rot);
		mat4translate(mat2, hp->pos[0], hp->pos[1], hp->pos[2]);
		quat2mat(rot, hp->rot);
		mat4mp(mat, mat2, rot);
		mat4roty(mat2, mat, pt->py[1]);
/*		glMultMatrixd(rotaxis);*/
		mat4translate(mat2, 0., .001, -0.002);
		mat4rotx(mat, mat2, pt->py[0]);
		mat4vp3(pos, mat, pos0);
		MAT4CPY(rot, mat);
		VECNULL(&rot[12]);
		drawmuzzleflash4(&pos, rot, .01, wd->irot, &rs, &wd->view);
	}
#endif
}

double MTurret::hitradius(){
	return .005;
}


#if 0
const struct arms_static_info arms_static[num_armstype] = {
	{"N/A",				armc_none,			0,	0,		NULL,			NULL,	NULL,	0., 0., 0., 0., 0.},
	{"Hydra 70",		armc_launcher,		0,	16,		m261_draw,		NULL,	NULL,	0.2, 25., 2., 20., 5., 100.},
	{"Hellfire",		armc_launcher,		0,	4,		hellfire_draw,	NULL,	NULL,	1., 20., 45., 10., 25., 300.},
	{"AIM-9",			armc_missile,		0,	1,		aim9_draw,		NULL,	NULL,	4., 2., 20., 10., 84., 300.},
	{"Beam Cannon",		armc_launcher,		0,	10,		beamcannon_draw,NULL,	beamcannon_anim,	4., 120., 1., 1000., 200., 100.},
	{"Avenger",			armc_launcher,		0,	1000,	avenger_draw,	NULL,	avenger_anim,	0.1, 150., .5, 200., .1, 10.},
	{"LONGBOW Radar",	armc_radome,		0,	0,		NULL,			NULL,	NULL,	0., 50., 0., 200., 0.},
	{"Shield Dome",		armc_radome,		0,	0,		NULL,			NULL,	NULL,	0., 60., 0., 10000., 0.},
	{"S. Gun Turret",	armc_smallturret,	0,	100,	sturret_draw,	NULL,	sturret_anim,	1., 150., 0.5, 300., 12., 10.},
	{"VF-25 Gunpod",	armc_robothand,		0,	1000,	gunpod_draw,	NULL,	NULL,	.2, 150., 0.5, 600., 1., 10.},
	{"VF-25 Rifle",		armc_robothand,		0,	20,		gunpod_draw,	NULL,	NULL,	4., 200., 2, 1000., 20., 100.},
	{"M. Gun Turret",	armc_mediumturret,	0,	2000,	mturret_draw,	mturret_drawtra,	mturret_anim,	2., 2000., 20, 50000., 200., 80.},
	{"M. Beam Turret",	armc_mediumturret,	0,	100,	mturret_draw,	mturret_drawtra,	mturret_anim,	5., 2000., 20, 50000., 200., 80.},
	{"M. Mis. Turret",	armc_mediumturret,	0,	50,		mturret_draw,	mturret_drawtra,	mturret_anim,	5., 2000., 20, 50000., 200., 300.},
	{"M16A1 AR",		armc_infarm,		0,	20,		m16_draw,		NULL,	NULL,	.1, 4., .004, 2., .0001},
	{"Shotgun",			armc_infarm,		0,	8,		shotgun_draw,	NULL,	NULL,	1., 5., .015, 1.5, .0002},
	{"M40A1 Sniper",	armc_infarm,		0,	5,		m40_draw,		NULL,	NULL,	1.5, 6.57, .0095, 3., .00015},
	{"RPG-7",			armc_infarm,		0,	1,		rpg7_draw,		NULL,	NULL,	3., 7., 2.6, 1., .5},
	{"Mortar",			armc_infarm,		0,	1,		rpg7_draw,		NULL,	NULL,	2., 10., 4, 6., 1.5},
	{"PLG",				armc_infarm,		0,	1,		rpg7_draw,		NULL,	NULL,	2., 8., 4, 6., 1.5},
};



static GLfloat show_light_pos[4];

static void show_light_on(void){
	glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
	{
		int i;
		GLfloat mat_specular[] = {0., 0., 0., 1.}/*{ 1., 1., .1, 1.0 }*/;
		GLfloat mat_diffuse[] = { .5, .5, .5, 1.0 };
		GLfloat mat_ambient_color[] = { 0.25, 0.25, 0.25, 1.0 };
		GLfloat mat_shininess[] = { 50.0 };
		GLfloat color[] = {1., 1., 1., .8}, amb[] = {1., 1., 1., 1.};
		avec3_t pos;

		glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_color);
		glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
		glLightfv(GL_LIGHT0, GL_POSITION, show_light_pos);
		glLightfv(GL_LIGHT0, GL_SPECULAR, mat_specular);
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, color);
	}
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_NORMALIZE);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
}

static void show_light_off(void){
	glPopAttrib();
}

static void show_init_wardraw(wardraw_t *wd, Viewer *vw){
	wd->listLight = 0;
	wd->light_on = show_light_on;
	wd->light_off = show_light_off;
	VECCPY(wd->view, vw->pos);
	wd->light[0] = 1.;
	wd->light[1] = 1.;
	wd->light[2] = 1.;
	VECNORMIN(wd->light);
	wd->ambient = COLOR32RGBA(255,255,255,255);
	VECSCALE(wd->viewdir, vw->rot, -1.);
	wd->fov = 1.;
	wd->pgc = NULL;
	wd->vw = vw;
	wd->gametime = 0.;
	wd->rot = &mat4identity;
	MAT4CPY(wd->irot, &mat4identity);
	wd->irot_init = 1;
	wd->maprange = 1.;
}













static void armswar_think(warf_t *w, double dt){}
static void armswar_draw(warf_t *w, wardraw_t *wd){}
static int armswar_pointhit(warf_t *w, const avec3_t *pos, const avec3_t *velo, double dt, struct contact_info *ci){return 0;}
static void armswar_accel(warf_t *w, avec3_t *dst, const avec3_t *srcpos, const avec3_t *srcvelo){VECCPY(*dst, avec3_000);}
static double armswar_pressure(warf_t *w, const avec3_t *pos){return 1.;}
static double armswar_sonic_speed(warf_t *w, const avec3_t *pos){return .340;}
int WarOrientation(struct war_field*, amat3_t *dst, const avec3_t *pos);
double WarNearPlane(const warf_t *);
double WarFarPlane(const warf_t *);

static struct war_field_static armswar_static = {
	armswar_think,
	WarPostFrame,
	WarEndFrame,
	armswar_draw,
	armswar_pointhit,
	armswar_accel,
	armswar_pressure,
	armswar_sonic_speed,
	NULL,
	WarOrientation,
	WarNearPlane,
	WarFarPlane,
	NULL,
};
static warf_t armswar = {
	&armswar_static,
	NULL, /* pl */
	NULL, /* tl */
	NULL, /* bl */
	NULL, /* tell */
	NULL, /* gibs */
	NULL, /* tefl */
	NULL, /* map */
	NULL, /* wmd */
/*	NULL, *//* patriot_home */
	0.,
	NULL,
	0,
	{0, 0}
};



struct glwindowarms{
	glwindow st;
	entity_t *pt;
	double baseprice;
	double lastt;
	double fov;
	avec3_t focus;
	aquat_t rot;
	struct hardpoint_static *hp;
	int nhp;
	arms_t *arms, *retarms;
};

static int buttonid(struct glwindow *wnd, int mx, int my){
	return 320 <= mx && mx <= 380 && wnd->h - 72 <= my && my <= wnd->h - 60 ? 1
		: 400 <= mx && mx <= 460 && wnd->h - 72 <= my && my <= wnd->h - 60 ? 2 : 0;
}

static int ArmsShow(struct glwindow *wnd, int mx, int my, double gametime){
	struct glwindowarms *p = (struct glwindowarms *)wnd;
	wardraw_t wd;
	Viewer viewer;
	struct glcull glc;
	extern warf_t warf;
	double dt = p->lastt ? gametime - p->lastt : 0.;
	GLfloat light_position[4] = { 1., 1., 1., 0.0 };
/*	extern double g_left, g_right, g_bottom, g_top, g_near, g_far;*/
	double totalweight = 0., totalprice = 0., totalattack = 0.;
	int button = 0;
	int i, ind = wnd && 0 <= my && 300 <= mx && mx <= wnd->w ? (my) / 24 : -1, ammosel = wnd && 310 + 8 * 12 < mx;

	button = buttonid(wnd, mx, my);
	if(button)
		ind = -1;

	p->lastt = gametime;

	if(0 <= mx && mx < 300 && 0 <= my && my <= 300){
		glPushMatrix();
		glTranslated(wnd->x, wnd->y + 12, 0);
		glColor4ub(0,0,255,255);
		glBegin(GL_LINE_LOOP);
		glVertex2i(1, 1);
		glVertex2i(299, 1);
		glVertex2i(299, 299);
		glVertex2i(1, 299);
		glEnd();
		glPopMatrix();
	}

	MAT4IDENTITY(viewer.rot);
	quat2mat(viewer.rot, p->rot);
/*	MAT4ROTY(viewer.rot, mat4identity, gametime * .5);*/
	MAT4IDENTITY(viewer.irot);
	quat2imat(viewer.irot, p->rot);
/*	MAT4ROTY(viewer.irot, mat4identity, -gametime * .5);*/
	MAT4CPY(viewer.relrot, viewer.rot);
	MAT4CPY(viewer.relirot, viewer.irot);
	VECCPY(viewer.pos, &viewer.irot[8]);
	VECSCALEIN(viewer.pos, ((struct entity_private_static*)p->pt->vft)->hitradius * 3.);
	MAT4DVP3(show_light_pos, viewer.irot, light_position);
	VECNULL(viewer.velo);
	VECNULL(viewer.pyr);
	viewer.velolen = 0.;
	viewer.fov = 1.;
	viewer.cs = NULL;
	viewer.relative = 0;
	viewer.detail = 0;
	viewer.vp.w = viewer.vp.h = viewer.vp.m = 100;
	show_init_wardraw(&wd, &viewer);
	wd.gametime = gametime;
	wd.pgc = &glc;
	wd.w = NULL;
	VECNULL(p->pt->pos);
	VECNULL(p->pt->velo);
	VECNULL(p->pt->omg);
	VECNULL(p->pt->pyr);
	VECNULL(p->pt->rot);
	p->pt->rot[3] = 1.;
	p->pt->active = 1;

	if(wnd) for(i = 0; i < p->nhp; i++){
		char buf[128];
		i == ind && !ammosel ? glColor4ub(0,255,255,255) : glColor4ub(255,255,255,255);
		glRasterPos2d(wnd->x + 300, wnd->y + (i + 1) * 24);
		gldPutString(p->hp[i].name);
		glRasterPos2d(wnd->x + 310, wnd->y + (i + 1) * 24 + 12);
		if(arms_static[p->arms[i].type].maxammo)
			gldPutStringN(arms_static[p->arms[i].type].name, 15);
		else
			gldPutString(arms_static[p->arms[i].type].name);
		if(arms_static[p->arms[i].type].maxammo){
			i == ind && ammosel ? glColor4ub(0,255,255,255) : glColor4ub(255,255,255,255);
			glRasterPos2d(wnd->x + 310 + 8 * 15, wnd->y + (i + 1) * 24 + 12);
			sprintf(buf, "x %d", p->arms[i].ammo);
			gldPutString(buf);
		}
		totalweight += arms_static[p->arms[i].type].emptyweight + p->arms[i].ammo * arms_static[p->arms[i].type].ammoweight;
		totalprice += arms_static[p->arms[i].type].emptyprice + p->arms[i].ammo * arms_static[p->arms[i].type].ammoprice;
		totalattack += arms_static[p->arms[i].type].cooldown ? arms_static[p->arms[i].type].ammodamage / arms_static[p->arms[i].type].cooldown : 0.;
	}
	{
		char buf[128];
		if(button == 1)
			glColor4ub(0,255,255,255);
		else
			glColor4ub(255,255,255,255);
		glRasterPos2d(wnd->x + 320, wnd->y + wnd->h - 48);
		gldPutString("OK");
		if(button == 2)
			glColor4ub(0,255,255,255);
		else
			glColor4ub(255,255,255,255);
		glRasterPos2d(wnd->x + 400, wnd->y + wnd->h - 48);
		gldPutString("Cancel");

		glColor4ub(255,255,255,255);
		glwpos2d(wnd->x + 300, wnd->y + wnd->h - 36);
		glwprintf("Attack: %lg Points/sec", totalattack);
		sprintf(buf, "Weight: %lg kg", totalweight + p->pt->mass);
		glRasterPos2d(wnd->x + 300, wnd->y + wnd->h - 24);
		gldPutString(buf);
		if(totalprice + p->baseprice < 1e6)
			sprintf(buf, "Price: $ %lg k", totalprice + p->baseprice);
		else
			sprintf(buf, "Price: $ %lg M", (totalprice + p->baseprice) * 1e-3);
		glRasterPos2d(wnd->x + 300, wnd->y + wnd->h - 12);
		gldPutString(buf);
	}

	{
	amat4_t proj;
	avec3_t dst;
	double f;
	GLint vp[4];
	f = 1. - exp(-dt / .05);
	if(wnd){
		RECT rc;
		int mi = MIN(wnd->w, wnd->h - 12);
		GetClientRect(WindowFromDC(wglGetCurrentDC()), &rc);
		glGetIntegerv(GL_VIEWPORT, vp);
		glViewport(wnd->x, rc.bottom - rc.top - (wnd->y + wnd->h), mi, mi);
	}
	glClear(GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glGetDoublev(GL_PROJECTION_MATRIX, &proj);
	glLoadIdentity();
	{
		double dst;
		if(wnd && 0 <= ind && ind < p->nhp)
			dst = .1;
		else
			dst = .5;
		p->fov += (dst - p->fov) * f;
		glFrustum (-p->fov * 1e-3, p->fov * 1e-3, -p->fov * 1e-3, p->fov * 1e-3, 1.5 * 1e-3, 10.);
	}
	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
	glLoadMatrixd(viewer.rot);
	glcullInit(&glc, 1., &viewer.pos, &viewer.irot, -10., 10.);
	VECNULL(dst);
	if(wnd && 0 <= ind && ind < p->nhp){
		VECADDIN(dst, p->hp[ind].pos);
		/*gldTranslaten3dv(arms_s[ind].pos);*/
	}
	{
		avec3_t delta;
		VECSUB(delta, dst, p->focus);
		VECSCALEIN(delta, f);
		VECADDIN(p->focus, delta);
	}
	VECADDIN(viewer.pos, p->focus);
	gldTranslaten3dv(viewer.pos);
	wd.light_on(&wd);
	((struct entity_private_static*)p->pt->vft)->draw(p->pt, &wd);
	wd.light_off(&wd);
	if(wnd && 0 <= ind && ind < p->nhp) for(i = 0; i < p->nhp; i++){
		i == ind ? glColor4ub(0,255,255,255) : glColor4ub(255,255,255,255);
		glRasterPos3dv(p->hp[i].pos);
		gldPutString(p->hp[i].name);
	}
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(proj);
	glMatrixMode(GL_MODELVIEW);

	if(wnd){
		glViewport(vp[0], vp[1], vp[2], vp[3]);
	}
	}

	return 0;
}

static void ArmsShowDestruct(struct glwindow *wnd){
	struct glwindowarms *p = (struct glwindowarms *)wnd;
	if(p->pt)
		free(p->pt);
}

static int ArmsShowMouse(struct glwindow *wnd, int mbutton, int state, int mx, int my){
	struct glwindowarms *p = (struct glwindowarms *)wnd;
	int i, ind = wnd && 300 <= mx && mx <= wnd->w ? (my) / 24 : -1, ammosel = wnd && 310 + 8 * 12 < mx;
	int button;

	button = buttonid(wnd, mx, my);
	if(button)
		ind = -1;

	if(state == GLUT_UP && button == 1){
		memcpy(p->retarms, p->arms, p->nhp * sizeof *p->retarms);
		if(!(wnd->flags & GLW_PINNED))
			wnd->flags |= GLW_TODELETE;
	}
	else if(state == GLUT_UP && button == 2){
		wnd->flags |= GLW_TODELETE;
	}
	else if(state == GLUT_KEEP_DOWN && ind == -1 && 12 < my){
		aquat_t q, q2, qr;
		q[0] = (150 - (my)) / 150. * .5;
		q[1] = q[2] = 0.;
		q[3] = sqrt(1. - q[0] * q[0]);
		q2[0] = 0.;
		q2[1] = -sin(((150 - (mx)) / 150. * M_PI) / 2.);
		q2[2] = 0.;
		q2[3] = sqrt(1. - q2[1] * q2[1]);
		QUATMUL(qr, q, q2);
		QUATZERO(q);
		q[1] = 1.;
		QUATMUL(q2, q, qr);
		QUATCPY(p->rot, q2);
	}
	if(state != GLUT_UP)
		return 1;
	if(wnd && 0 <= ind && ind < p->nhp) if(ammosel){
		if(arms_static[p->arms[ind].type].maxammo)
			p->arms[ind].ammo = (p->arms[ind].ammo + (mbutton ? -1 : 1) + arms_static[p->arms[ind].type].maxammo + 1) % (arms_static[p->arms[ind].type].maxammo + 1);
	}
	else{
		do{
			p->arms[ind].type = (p->arms[ind].type + (mbutton ? -1 : 1) + num_armstype) % num_armstype;
		} while(!(p->hp[ind].clsmask[arms_static[p->arms[ind].type].cls / (sizeof(unsigned) * CHAR_BIT)] & (1 << arms_static[p->arms[ind].type].cls % (sizeof(unsigned) * CHAR_BIT))));
		p->arms[ind].ammo = arms_static[p->arms[ind].type].maxammo;
	}
	return 1;
}

glwindow *ArmsShowWindow(entity_t *creator(warf_t *), double baseprice, size_t armsoffset, arms_t *retarms, struct hardpoint_static *hardpoints, int count){
	glwindow *ret;
	glwindow **ppwnd;
	struct glwindowarms *p;
	int i;
	static const char *windowtitle = "Arms Fitting";
	for(ppwnd = &glwlist; *ppwnd; ppwnd = &(*ppwnd)->next) if((*ppwnd)->title == windowtitle){
		glwActivate(ppwnd);
		return 0;
	}
	p = malloc(sizeof *p);
	ret = &p->st;
	ret->x = 50;
	ret->y = 50;
	ret->w = 500;
	ret->h = 312;
	ret->title = windowtitle;
	ret->flags = GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE;
	ret->modal = NULL;
	ret->draw = ArmsShow;
	ret->mouse = ArmsShowMouse;
	ret->key = NULL;
	ret->destruct = ArmsShowDestruct;
	glwActivate(glwAppend(ret));
/*	memset(p->arms, 0, sizeof p->arms);*/
	p->pt = creator(&armswar);
	p->baseprice = baseprice;
	p->arms = (arms_t*)&((char*)p->pt)[armsoffset];
	p->retarms = retarms;
	memcpy(p->arms, retarms, count * sizeof *retarms);
	p->lastt = 0.;
	p->fov = .5;
	VECNULL(p->focus);
	QUATZERO(p->rot);
	p->rot[1] = 1.;
	p->hp = hardpoints;
	p->nhp = count;
/*	memcpy(p->arms, apachearms_def, sizeof apachearms_def);*/
	return ret;
}





#endif
