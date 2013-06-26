/** \file
 * \brief Implementation of Tank class's graphics.
 */
#include "../Tank.h"
#include "Viewer.h"
#include "draw/WarDraw.h"
#include "draw/material.h"
#include "draw/mqoadapt.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
}


#define FORESPEED 0.004
#define TURRETROTSPEED (.2 * M_PI)
#define TURRETROTRANGE (.6 * M_PI)
#define ROTSPEED (.075 * M_PI)
#define TANKGUNSPEED 1.7
#define TANKGUNDAMAGE 500.
#define GUNVARIANCE .005
#define INTOLERANCE (M_PI / 50.)
#define BULLETRANGE 3.
#define RELOADTIME 3.
#define RETHINKTIME .5
#define TANK_MAX_GIBS 30
/*#define TANK_SCALE .0001*/
#define TANK_SCALE (3.33 / 200 * 1e-3)



void Tank::draw(WarDraw *wd){
	static int init = 0;
/*	static suf_t *sufbase = NULL;
	static suf_t *sufturret = NULL;
	static suf_t *sufbarrel = NULL;
	static suf_t *sufbarrel1 = NULL;
	static suf_t *sufm2 = NULL;*/
	static Model *model = NULL;
	double pixels;
	if(!w)
		return;

	/* cull object */
	if(wd->vw->gc->cullFrustum(pos, .007))
		return;
	pixels = .005 * fabs(wd->vw->gc->scale(pos));
	if(pixels < 2)
		return;
	wd->lightdraws++;


	if(init == 0){
		init = 1;
		model = LoadMQOModel(modPath() << _SC("models/type90.mqo"));
	};

	if(model){
//		GLfloat fv[4] = {.8f, .5f, 0.f, 1.f}, ambient[4] = {.4f, .25f, 0.f, 1.f};
//		static const double normal[3] = {0., 1., 0.}, mountpos[3] = {-33, 125, 45};
		double scale = 3.4 / 200. * 1e-3, tscale = scale, bscale = scale;
//		if(race % 2)
//			fv[0] = 0., fv[2] = .8f, ambient[0] = 0., ambient[2] = .4f;

		glPushMatrix();
/*		glTranslated(pt->pos[0], pt->pos[1], pt->pos[2]);*/
		{
/*			const avec3_t cog = {0., .001, 0.};*/
			Mat4d mat;
			transform(mat);
/*			MAT3TO4(mat, pt->rot);*/
			glMultMatrixd(mat);
		}

		/* center of gravity offset */
		glTranslated(0, -.0007, 0);



#if 1
		// Unlike most ModelEntities, Tank's model need not be rotated 180 degrees because
		// the model is made such way.
		glScaled(scale, scale, scale);
		DrawMQOPose(model, NULL);
		glPopMatrix();
#else
		DrawSUF(sufbase, SUF_ATR, NULL);
/*		glRotated(deg_per_rad * pt->turrety, 0., 0., -1.);*/
		glRotated(deg_per_rad * turrety, 0., -1., 0.);
		if(game->player->chase != this || game->player->getChaseCamera())
			DrawSUF(sufturret, SUF_ATR, NULL);

		glPushMatrix();
		gldTranslaten3dv(mountpos);
		glRotated(deg_per_rad * mountpy[1], 0., -1., 0.);
		glRotated(deg_per_rad * mountpy[0], 1., 0., 0.);
		gldTranslate3dv(mountpos);
		DrawSUF(sufm2, SUF_ATR, NULL);
		glPopMatrix();

		glTranslated(0., 90., -85.);
/*		glTranslated(0., .0025 / tscale, -0.0015 / tscale);*/
		glRotated(deg_per_rad * barrelp, 1., 0., 0.);

		/* level of detail */
#if 0
		if(0 && 100 < pixels){
			glScaled(1. / tscale, 1. / tscale, 1. / tscale);
	/*		glTranslated(0., .0025, -0.0015);*/
			glScaled(bscale, bscale, bscale);
			DrawSUF(sufbarrel1, SUF_ATR, NULL);
		}
		else
#endif
		{
			glTranslated(0., -90., 85.);
/*			glTranslated(0., -.0025 / tscale, 0.0015 / tscale);*/
			DrawSUF(sufbarrel, SUF_ATR, NULL);
		}

		glPopMatrix();
#endif
	}

}

void Tank::drawtra(wardraw_t *wd){

/*	if(p->muzzle & 1){
		avec3_t mpos;
		tankmuzzlepos(p, &mpos, NULL);
		drawmuzzleflasha(mpos, wd->view, .007, wd->irot);
		p->muzzle = 0;
	}*/

/*	avec3_t v;
	amat4_t mat;
	int i;
	tankrot(mat, pt);
	glPushMatrix();
	glMultMatrixd(mat);
	glBegin(GL_LINES);
	for(i = 0; i < numof(p->forces); i++){
		VECCPY(v, points[i]);
		VECSADD(v, p->forces[i], 1e-3);
		glColor4ub(255,0,0,255);
		glVertex3dv(points[i]);
		glVertex3dv(v);
		VECCPY(v, points[i]);
		VECSADD(v, p->normals[i], 1e-3);
		glColor4ub(0,0,255,255);
		glVertex3dv(points[i]);
		glVertex3dv(v);
	}
	glEnd();
	glPopMatrix();*/
}

#if 0
void gib_draw(const struct tent3d_line_callback *pl, suf_t *suf, double scale, int i){
	glPushMatrix();
	glTranslated(pl->pos[0], pl->pos[1], pl->pos[2]);
	glScaled(scale, scale, scale);
/*	glRotated(-pl->pyr[1] * 360 / 2. / M_PI, 0., 1., 0.);
	glRotated(pl->pyr[0] * 360 / 2. / M_PI, 1., 0., 0.);*/
	DrawSUFPoly(suf, i, SUF_ATR, &g_gldcache);
	glPopMatrix();
}

static void tank_gib_draw(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	int i = (int)pv;
	gib_draw(pl, i < tank_s.sufbase->np ? tank_s.sufbase : tank_s.sufturret, TANK_SCALE, i < tank_s.sufbase->np ? i : i - tank_s.sufbase->np);
}
#endif

#if 0
void tank_drawHUD(entity_t *pt, warf_t *wf, wardraw_t *wd, const double irot[16], void (*gdraw)(void)){
	tank2_t *p = (tank2_t*)pt;
	base_drawHUD_target(pt, wf, wd, gdraw);
	base_drawHUD_map(pt, wf, wd, irot, gdraw);
	base_drawHUD(pt, wf, wd, gdraw);

	if(wf->pl->control != pt){
	}
	else{
		GLint vp[4];
		int w, h, m;
		double left, bottom;
/*		int tkills, tdeaths, ekills, edeaths;*/
/*		entity_t *pt2;*/
		double v;
		glGetIntegerv(GL_VIEWPORT, vp);
		w = vp[2], h = vp[3];
		m = w < h ? h : w;
		left = -(double)w / m;
		bottom = -(double)h / m;

		v = VECLEN(pt->velo);

		glPushMatrix();
		glLoadIdentity();
		glRasterPos3d(left + 20. / m, bottom + 60. / m, -1);
		gldprintf("%lg m/s", v * 1e3);
		glRasterPos3d(left + 20. / m, bottom + 80. / m, -1);
		gldprintf("%lg km/h", v * 1e3 * 3600. / 1000.);
		glRasterPos3d(-left - 500. / m, bottom + 60. / m, -1);
		gldprintf("%c 120mm Main Gun x %d", pt->weapon == 0 ? '>' : ' ', p->ammo[0]);
		glRasterPos3d(-left - 500. / m, bottom + 40. / m, -1);
		gldprintf("%c 7.62mm Type 74 x %d", pt->weapon == 1 ? '>' : ' ', p->ammo[1]);
		glRasterPos3d(-left - 500. / m, bottom + 20. / m, -1);
		gldprintf("%c 12.7mm M2 Browning x %d", pt->weapon == 2 ? '>' : ' ', p->ammo[2]);
		glRasterPos3d(-left - 500. / m, bottom + 0. / m, -1);
		gldprintf("%c 120mm Shotgun x %d", pt->weapon == 3 ? '>' : ' ', p->ammo[0]);
		glPopMatrix();
	}

}
#endif
