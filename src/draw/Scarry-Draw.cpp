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

void GLWbuild::progress_bar(double f, int width, int *piy){
	GLWrect cr = clientRect();
	int iy = *piy;
	glColor4ub(0,255,0,255);
	glBegin(GL_QUADS);
	glVertex2d(cr.x0, cr.y0 + iy * 12);
	glVertex2d(cr.x0 + width * f, cr.y0 + iy * 12);
	glVertex2d(cr.x0 + width * f, cr.y0 + (iy + 1) * 12);
	glVertex2d(cr.x0 + 0, cr.y0 + (iy + 1) * 12);
	glEnd();
	glColor4ub(127,127,127,255);
	glBegin(GL_LINE_LOOP);
	glVertex2d(cr.x0, cr.y0 + iy * 12);
	glVertex2d(cr.x0 + width, cr.y0 + iy * 12);
	glVertex2d(cr.x0 + width, cr.y0 + (iy + 1) * 12);
	glVertex2d(cr.x0 + 0, cr.y0 + (iy + 1) * 12);
	glEnd();
	glColor4ub(255,0,0,255);
	glwpos2d(cr.x0, cr.y0 + (1 + (*piy)++) * 12);
	glwprintf("%3.0lf%%", 100. * f);
}

int GLWbuild::draw_tab(int ix, int iy, const char *s, int selected){
	GLWrect cr = clientRect();
	int ix0 = ix;
	glBegin(GL_LINE_STRIP);
	glVertex2d(cr.x0 + ix, cr.y0 + iy + 12);
	glVertex2d(cr.x0 + (ix += 3), cr.y0 + iy + 1);
	ix += 8 * strlen(s);
	glVertex2d(cr.x0 + ix, cr.y0 + iy + 1);
	glVertex2d(cr.x0 + (ix += 3), cr.y0 + iy + 12);
	if(!selected){
		glVertex2d(cr.x0 + ix0, cr.y0 + iy + 12);
	}
	glEnd();
	glwpos2d(cr.x0 + ix0 + 3, cr.y0 + iy + 12);
	glwprintf("%s", s);
	return ix;
}

void GLWbuild::draw(GLwindowState &ws, double t){
	int ix;
	GLWrect cr = clientRect();
	int fonth = getFontHeight();
	if(!builder)
		return;
	glColor4ub(255,255,255,255);
	glwpos2d(cr.x0, cr.y0 + fonth);
	glwprintf("Resource Units: %.0lf", std::max(0., builder->getRU())); // The max is to avoid -0.
	glColor4ub(255,255,0,255);
	ix = draw_tab(ix = 2, fonth, "Build", tabindex == 0);
	ix = draw_tab(ix, fonth, "Queue", tabindex == 1);
	glBegin(GL_LINES);
	glVertex2d(cr.x0 + ix, cr.y0 + 2 * fonth);
	glVertex2d(cr.x1, cr.y0 + 2 * fonth);
	glEnd();
	const Builder::BuildData &top = builder->buildque[0];
	int iy = 2;
	if(tabindex == 0){
		int *builderc = NULL, builderi, j;
		if(builder->nbuildque){
			builderc = new int[Builder::buildRecipes.size()];
			for(j = 0; j < Builder::buildRecipes.size(); j++){
				builderc[j] = 0;

				// Accumulate all build queue entries
				for(int i = 0; i < builder->nbuildque; i++){
					if(builder->buildque[i].st == &Builder::buildRecipes[j])
						builderc[j] += builder->buildque[i].num;
				}

				// Remember the top (currently building) build queue entry for progress bar.
				if(top.st == &Builder::buildRecipes[j])
					builderi = j;
			}
		}
		glColor4ub(255,255,255,255);
		glwpos2d(cr.x0, cr.y0 + (1 + iy++) * fonth);
		glwprintf("Buildable Items:");

		/* Mouse cursor highlights */
		int mx = ws.mx - cr.x0, my = ws.my - cr.y0;
		if(!modal && 0 < mx && mx < cr.width() && (iy) * fonth < my && my < (iy + Builder::buildRecipes.size()) * fonth){
			glColor4ub(0,0,255,127);
			glRecti(cr.x0, cr.y0 + (my / fonth) * fonth, cr.x1, cr.y0 + (my / fonth) * fonth);
		}

		/* Progress bar of currently building item */
		if(builder->nbuildque && builder->build){
			glColor4ub(0,127,0,255);
			glRecti(cr.x0, cr.y0 + (builderi + iy) * fonth + 8, cr.x0 + (1. - builder->build / top.st->buildtime) * cr.width(), cr.y0 + (builderi + iy) * fonth + fonth);
		}

		// Draw the buildable item's name and cost after the progress bar.
		glColor4ub(191,191,255,255);
		for(j = 0; j < Builder::buildRecipes.size(); j++){
			const Builder::BuildRecipe *sta = &Builder::buildRecipes[j];
			glwpos2d(cr.x0, cr.y0 + (1 + iy++) * fonth);
			glwprintf("%10s  %d  %lg RU", (const char*)sta->name, builderc ? builderc[j] : 0, sta->cost);
		}
/*		glwpos2d(xpos, ypos + (2 + iy++) * 12);
		glwprintf("Container    %d  %lg RU %lg dm^3", builderc[0], scontainer_build.cost, 1e-3 * scontainer_s.getHitRadius * scontainer_s.getHitRadius * scontainer_s.getHitRadius * 8 * 1e9);
		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Worker       %d  %lg RU %lg dm^3", builderc[1], worker_build.cost, 1e-3 * worker_s.getHitRadius * worker_s.getHitRadius * worker_s.getHitRadius * 8 * 1e9);*/
/*		glwpos2d(xpos, ypos + (2 + iy++) * fonth);
		glwprintf("Interceptor  %d  %lg RU %lg dm^3", builderc ? builderc[0] : 0, sceptor_build.cost, 1e-3 * ((Sceptor*)NULL)->Sceptor::getHitRadius() * ((Sceptor*)NULL)->Sceptor::getHitRadius() * ((Sceptor*)NULL)->Sceptor::getHitRadius() * 8 * 1e9);*/
/*		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Lancer Class %d  %lg RU %lg dm^3", builderc[3], beamer_build.cost, 1e-3 * beamer_s.getHitRadius * beamer_s.getHitRadius * beamer_s.getHitRadius * 8 * 1e9);
		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Sabre Class  %d  %lg RU %lg dm^3", builderc[4], assault_build.cost, 1e-3 * assault_s.getHitRadius * assault_s.getHitRadius * assault_s.getHitRadius * 8 * 1e9);*/
		if(builder->nbuildque)
			delete[] builderc;
	}
	else{
		glColor4ub(255,255,255,255);
		glwpos2d(cr.x0, cr.y0 + (1 + iy++) * fonth);
		glwprintf("Build Queue:");
		if(builder->nbuildque == 0)
			return;
		glwpos2d(cr.x0, cr.y0 + (1 + iy++) * fonth);
		glwprintf(top.st->name);
		progress_bar((1. - builder->build / top.st->buildtime), 200, &iy);
		for(int i = 0; i < builder->nbuildque; i++){
			if(cr.height() < (2 + iy) * fonth)
				return;
			glColor4ub(255,255,255,255);
			glwpos2d(cr.x0, cr.y0 + (1 + iy) * fonth);
			int widthChars = glwprintf("Cancel");
			int widthPixels = widthChars * getFontWidth();

			glColor4ub(0,255,255,255);
			glBegin(GL_LINE_LOOP);
			glVertex2i(cr.x0, cr.y0 + (iy) * fonth);
			glVertex2i(cr.x0 + widthPixels, cr.y0 + (iy) * fonth);
			glVertex2i(cr.x0 + widthPixels, cr.y0 + (1 + iy) * fonth - 1);
			glVertex2i(cr.x0, cr.y0 + (1 + iy) * fonth - 1);
			glEnd();

			glColor4ub(255,255,255,255);
			glwpos2d(cr.x0 + widthPixels + 8, cr.y0 + (1 + iy++) * fonth);
			glwprintf("%d X %s %lg RUs", builder->buildque[i].num, builder->buildque[i].st->name.c_str(), builder->buildque[i].num * builder->buildque[i].st->cost);
		}
	}
}

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

	draw_healthbar(this, wd, health / maxhealth(), getHitRadius(), -1., capacitor / maxenergy());

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
