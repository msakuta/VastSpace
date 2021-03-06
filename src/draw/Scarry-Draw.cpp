/** \file
 * \brief Remnants of Scarry class. Has some codes still active.
 */
#define NOMINMAX
#include "../scarry.h"
#include "judge.h"
#include "draw/material.h"
#include "../sceptor.h"
#include "draw/WarDraw.h"
extern "C"{
#include <clib/gl/gldraw.h>
}

#define texture(e) glMatrixMode(GL_TEXTURE); e; glMatrixMode(GL_MODELVIEW);


void GLWdock::draw(GLwindowState &ws, double t){
	typedef Entity::Dockable Dockable;
	int ix;
	if(!docker)
		return;
	GLWrect cr = clientRect();
	glColor4ub(255,255,255,255);
	glwpos2d(cr.x0, cr.y0 + 1 * getFontHeight());
	glwprintf("Resource Units: %.0lf", docker->e->getRU());
	glColor4ub(255,255,0,255);
	int fonth = getFontHeight();
	int iy = 0;

	if(!docker)
		return;
	glColor4ub(255,255,255,255);
	glwpos2d(cr.x0, cr.y0 + (2 + iy++) * fonth);
	glwprintf("Cool: %lg", docker->baycool);
	glwpos2d(cr.x0, cr.y0 + (2 + iy++) * fonth);
	glwprintf("Remain docked: %s", docker->remainDocked ? "Yes" : "No");
	glwpos2d(cr.x0, cr.y0 + (2 + iy++) * fonth);
	glwprintf("Grouping: %s", grouping ? "Yes" : "No");
	glwpos2d(cr.x0, cr.y0 + (2 + iy++) * fonth);
	glwprintf("Docked:");
	glColor4ub(255,255,255,255);

	int mx = ws.mx - cr.x0, my = ws.my - cr.y0;
	/* Mouse cursor highlights */
	if(!modal && 0 < mx && mx < width && (iy - 2 <= my / fonth && my / fonth <= iy - 1 || (1 + iy) * fonth < my)){
		glColor4ub(0,0,255,127);
		glRecti(cr.x0, cr.y0 + (my / fonth) * fonth, cr.x1, cr.y0 + (my / fonth + 1) * fonth);
		glColor4ub(255,255,255,255);
	}

	if(grouping){
		std::map<cpplib::dstring, int> map;

		for(WarField::EntityList::iterator it = docker->el.begin(); it != docker->el.end(); it++) if(*it){
			Entity *e = (*it)->toDockable();
			if(e)
				map[e->dispname()]++;
		}
		for(std::map<cpplib::dstring, int>::iterator it = map.begin(); it != map.end(); it++){
			glwpos2d(cr.x0, cr.y0 + (2 + iy++) * fonth);
			glwprintf("%d X %s", it->second, (const char*)it->first);
		}
	}
	else{
		for(WarField::EntityList::iterator it = docker->el.begin(); it != docker->el.end(); it++) if(*it){
			Entity *e = (*it)->toDockable();
			if(!e)
				continue;
			if(height < (2 + iy) * fonth)
				return;
			glwpos2d(cr.x0, cr.y0 + (2 + iy++) * fonth);
			glwprintf("%d X %s %lg m^3", 1, e->dispname(), e->getHitRadius() * e->getHitRadius() * e->getHitRadius());
		}
	}
}

#if 0
void Scarry::draw(wardraw_t *wd){
	Scarry *const p = this;
	static int init = 0;
	static suftex_t *pst;
	static suf_t *sufbase = NULL;
	if(!w)
		return;

	/* cull object */
/*	if(beamer_cull(this, wd))
		return;*/
//	wd->lightdraws++;

	draw_healthbar(this, wd, health / getMaxHealth(), getHitRadius(), -1., capacitor / maxenergy());

	if(wd->vw->gc->cullFrustum(pos, getHitRadius()))
		return;
#if 0
	if(init == 0) do{
		init = 1;
		sufbase = CallLoadSUF("models/spacecarrier.bin");
		if(!sufbase) break;
		CallCacheBitmap("bridge.bmp", "bridge.bmp", NULL, NULL);
		CallCacheBitmap("beamer_panel.bmp", "beamer_panel.bmp", NULL, NULL);
		CallCacheBitmap("bricks.bmp", "bricks.bmp", NULL, NULL);
		CallCacheBitmap("runway.bmp", "runway.bmp", NULL, NULL);
		suftexparam_t stp;
		stp.flags = STP_MAGFIL | STP_MINFIL | STP_ENV;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		stp.env = GL_ADD;
		stp.mipmap = 0;
		CallCacheBitmap5("engine2.bmp", "engine2br.bmp", &stp, "engine2.bmp", NULL);
		pst = AllocSUFTex(sufbase);
		extern GLuint screentex;
		glNewList(pst->a[0].list, GL_COMPILE);
		glBindTexture(GL_TEXTURE_2D, screentex);
		glTexEnvi(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glDisable(GL_LIGHTING);
		glEndList();
	} while(0);
	if(sufbase){
		static const double normal[3] = {0., 1., 0.};
		double scale = SCARRY_SCALE;
		static const GLdouble rotaxis[16] = {
			-1,0,0,0,
			0,1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};
		Mat4d mat;

		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);

#if 1
		for(int i = 0; i < nhitboxes; i++){
			Mat4d rot;
			glPushMatrix();
			gldTranslate3dv(hitboxes[i].org);
			rot = hitboxes[i].rot.tomat4();
			glMultMatrixd(rot);
			hitbox_draw(this, hitboxes[i].sc);
			glPopMatrix();
		}
#endif

		Mat4d modelview, proj;
		glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
		glGetDoublev(GL_PROJECTION_MATRIX, proj);
		Mat4d trans = proj * modelview;
		texture((glPushMatrix(),
			glScaled(1./2., 1./2., 1.),
			glTranslated(1, 1, 0),
			glMultMatrixd(trans)
		));
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGendv(GL_S, GL_EYE_PLANE, Vec4d(1,0,0,0));
		glEnable(GL_TEXTURE_GEN_S);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGendv(GL_T, GL_EYE_PLANE, Vec4d(0,1,0,0));
		glEnable(GL_TEXTURE_GEN_T);
		glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGeniv(GL_R, GL_EYE_PLANE, Vec4<int>(0,0,1,0));
		glEnable(GL_TEXTURE_GEN_R);
		glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGeniv(GL_Q, GL_EYE_PLANE, Vec4<int>(0,0,0,1));
		glEnable(GL_TEXTURE_GEN_Q);

		glPushMatrix();
		glScaled(scale, scale, scale);
		glMultMatrixd(rotaxis);
		DecalDrawSUF(sufbase, SUF_ATR, NULL, pst, NULL, NULL);
		glPopMatrix();

		texture((glPopMatrix()));
		glPopMatrix();
		glPopAttrib();
	}
#endif
}

void Scarry::drawtra(wardraw_t *wd){
	st::drawtra(wd);
	Scarry *p = this;
	Scarry *pt = this;
	Mat4d mat;
	Vec3d pa, pb, pa0(.01, 0, 0), pb0(-.01, 0, 0);
	double scale;

/*	if(scarry_cull(pt, wd))
		return;*/

	if(wd->vw->gc->cullFrustum(pos, getHitRadius()))
		return;

	scale = fabs(wd->vw->gc->scale(this->pos));

	transform(mat);

	const double blastscale = .04;
	drawCapitalBlast(wd, Vec3d(0, 0, .55), blastscale);
	drawCapitalBlast(wd, Vec3d(.08, .08, .55), blastscale);
	drawCapitalBlast(wd, Vec3d(-.08, .08, .55), blastscale);
	drawCapitalBlast(wd, Vec3d(-.08, -.08, .55), blastscale);
	drawCapitalBlast(wd, Vec3d(.08, -.08, .55), blastscale);

	pa = pt->rot.trans(pa0);
	pa += pt->pos;
	pb = pt->rot.trans(pb0);
	pb += pt->pos;
	glColor4ub(255,255,9,255);
	glBegin(GL_LINES);
	glVertex3dv(pa);
	glVertex3dv(pb);
	glEnd();

	{
		int i;
		static const avec3_t lights[] = {
			{0, 520 * SCARRY_SCALE, 220 * SCARRY_SCALE},
			{0, -520 * SCARRY_SCALE, 220 * SCARRY_SCALE},
			{140 * SCARRY_SCALE, 370 * SCARRY_SCALE, 220 * SCARRY_SCALE},
			{-140 * SCARRY_SCALE, 370 * SCARRY_SCALE, 220 * SCARRY_SCALE},
			{140 * SCARRY_SCALE, -370 * SCARRY_SCALE, 220 * SCARRY_SCALE},
			{-140 * SCARRY_SCALE, -370 * SCARRY_SCALE, 220 * SCARRY_SCALE},
			{100 * SCARRY_SCALE, -360 * SCARRY_SCALE, -600 * SCARRY_SCALE},
			{100 * SCARRY_SCALE,  360 * SCARRY_SCALE, -600 * SCARRY_SCALE},
			{ 280 * SCARRY_SCALE,   20 * SCARRY_SCALE, 520 * SCARRY_SCALE},
			{ 280 * SCARRY_SCALE,  -20 * SCARRY_SCALE, 520 * SCARRY_SCALE},
			{-280 * SCARRY_SCALE,   20 * SCARRY_SCALE, 520 * SCARRY_SCALE},
			{-280 * SCARRY_SCALE,  -20 * SCARRY_SCALE, 520 * SCARRY_SCALE},
			{-280 * SCARRY_SCALE,   20 * SCARRY_SCALE, -300 * SCARRY_SCALE},
			{-280 * SCARRY_SCALE,  -20 * SCARRY_SCALE, -300 * SCARRY_SCALE},
			{ 280 * SCARRY_SCALE,   20 * SCARRY_SCALE, -480 * SCARRY_SCALE},
			{ 280 * SCARRY_SCALE,  -20 * SCARRY_SCALE, -480 * SCARRY_SCALE},
		};
		avec3_t pos;
		double rad = .01;
		double t;
		GLubyte col[4] = {255, 31, 31, 255};
		random_sequence rs;
		init_rseq(&rs, (unsigned long)this);

		/* color calculation of static navlights */
		t = fmod(wd->vw->viewtime + drseq(&rs) * 2., 2.);
		if(t < 1.){
			rad *= (t + 1.) / 2.;
			col[3] *= t;
		}
		else{
			rad *= (2. - t + 1.) / 2.;
			col[3] *= 2. - t;
		}
		for(i = 0 ; i < numof(lights); i++){
			mat4vp3(pos, mat, lights[i]);
			gldSpriteGlow(pos, rad, col, wd->vw->irot);
		}

		/* runway lights */
		if(1 < scale * .01){
			col[0] = 0;
			col[1] = 191;
			col[2] = 255;
			for(i = 0 ; i <= 10; i++){
				avec3_t pos0;
				pos0[0] = -160 * SCARRY_SCALE;
				pos0[1] = 20 * SCARRY_SCALE + .0025;
				pos0[2] = (i * -460 + (10 - i) * -960) * SCARRY_SCALE / 10;
				rad = .005 * (1. - fmod(i / 10. + t / 2., 1.));
				col[3] = 255/*rad * 255 / .01*/;
				mat4vp3(pos, mat, pos0);
				gldSpriteGlow(pos, rad, col, wd->vw->irot);
				pos0[0] = -40 * SCARRY_SCALE;
				mat4vp3(pos, mat, pos0);
				gldSpriteGlow(pos, rad, col, wd->vw->irot);
				pos0[1] = -20 * SCARRY_SCALE - .0025;
				mat4vp3(pos, mat, pos0);
				gldSpriteGlow(pos, rad, col, wd->vw->irot);
				pos0[0] = -160 * SCARRY_SCALE;
				mat4vp3(pos, mat, pos0);
				gldSpriteGlow(pos, rad, col, wd->vw->irot);
			}
		}

/*		for(i = 0; i < numof(p->turrets); i++)
			mturret_drawtra(&p->turrets[i], pt, wd);*/
	}

	static int init = 0;
	static suftex_t *pst;
	static suf_t *sufbase = NULL;
	if(init == 0) do{
		init = 1;
		sufbase = CallLoadSUF("models/spacecarrier.bin");
	} while(0);
	if(sufbase){
		static const double normal[3] = {0., 1., 0.};
		double scale = SCARRY_SCALE;
		static const GLdouble rotaxis[16] = {
			-1,0,0,0,
			0,1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};
		Mat4d mat;

		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
		glEnable(GL_CULL_FACE);
		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);

		extern GLuint screentex;
		glBindTexture(GL_TEXTURE_2D, screentex);
		glTexEnvi(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glColor4f(1.,1.,1.,1.);
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);

		Mat4d modelview, proj;
		glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
		glGetDoublev(GL_PROJECTION_MATRIX, proj);
		Mat4d trans = proj * modelview;
		texture((glPushMatrix(),
			glScaled(1./2., 1./2., 1.),
			glTranslated(1, 1, 0),
			glMultMatrixd(trans)
		));
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGendv(GL_S, GL_EYE_PLANE, Vec4d(.9,0,0,0));
		glEnable(GL_TEXTURE_GEN_S);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGendv(GL_T, GL_EYE_PLANE, Vec4d(0,.9,0,0));
		glEnable(GL_TEXTURE_GEN_T);
		glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGendv(GL_R, GL_EYE_PLANE, Vec4d(0,0,.9,0));
		glEnable(GL_TEXTURE_GEN_R);
		glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGeniv(GL_Q, GL_EYE_PLANE, Vec4<int>(0,0,0,1));
		glEnable(GL_TEXTURE_GEN_Q);

		glPushMatrix();
		glScaled(-scale, scale, -scale);
		DrawSUF(sufbase, 0, NULL);
		glPopMatrix();

		texture((glPopMatrix()));
		glPopMatrix();
		glPopAttrib();
	}
}
#endif
