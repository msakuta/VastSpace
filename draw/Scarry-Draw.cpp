#include "../scarry.h"
#include "../judge.h"
#include "../material.h"
#include "../sceptor.h"
extern "C"{
#include <clib/gl/gldraw.h>
}

void GLWbuild::progress_bar(double f, int width, int *piy){
	int iy = *piy;
	glColor4ub(0,255,0,255);
	glBegin(GL_QUADS);
	glVertex2d(xpos, ypos + (2 + iy - 1) * 12);
	glVertex2d(xpos + width * f, ypos + (2 + iy - 1) * 12);
	glVertex2d(xpos + width * f, ypos + (2 + iy - 0) * 12);
	glVertex2d(xpos + 0, ypos + (2 + iy - 0) * 12);
	glEnd();
	glColor4ub(127,127,127,255);
	glBegin(GL_LINE_LOOP);
	glVertex2d(xpos, ypos + (2 + iy - 1) * 12);
	glVertex2d(xpos + width, ypos + (2 + iy - 1) * 12);
	glVertex2d(xpos + width, ypos + (2 + iy - 0) * 12);
	glVertex2d(xpos + 0, ypos + (2 + iy - 0) * 12);
	glEnd();
	glColor4ub(255,0,0,255);
	glwpos2d(xpos, ypos + (2 + (*piy)++) * 12);
	glwprintf("%3.0lf%%", 100. * f);
}

const Builder::BuildStatic sceptor_build = {
	"Interceptor",
	Sceptor::create,
	10.,
	60.,
};

int GLWbuild::draw_tab(int ix, int iy, const char *s, int selected){
	int ix0 = ix;
	glBegin(GL_LINE_STRIP);
	glVertex2d(this->xpos + ix, this->ypos + iy + 12);
	glVertex2d(this->xpos + (ix += 3), this->ypos + iy + 1);
	ix += 8 * strlen(s);
	glVertex2d(this->xpos + ix, this->ypos + iy + 1);
	glVertex2d(this->xpos + (ix += 3), this->ypos + iy + 12);
	if(!selected){
		glVertex2d(xpos + ix0, ypos + iy + 12);
	}
	glEnd();
	glwpos2d(xpos + ix0 + 3, ypos + iy + 12);
	glwprintf("%s", s);
	return ix;
}

void GLWbuild::draw(GLwindowState &ws, double t){
	int ix;
	if(!builder)
		return;
	glColor4ub(255,255,255,255);
	glwpos2d(this->xpos, this->ypos + 2 * getFontHeight());
	glwprintf("Resource Units: %.0lf", builder->getRU());
	glColor4ub(255,255,0,255);
	ix = draw_tab(ix = 2, 2 * 12, "Build", tabindex == 0);
	ix = draw_tab(ix, 2 * 12, "Queue", tabindex == 1);
	glBegin(GL_LINES);
	glVertex2d(xpos + ix, ypos + 3 * 12);
	glVertex2d(xpos + width, ypos + 3 * 12);
	glEnd();
	const Builder::BuildData &top = builder->buildque[0];
	int fonth = getFontHeight();
	int iy = 2;
	if(tabindex == 0){
		int *builderc = NULL, builderi, j;
		if(builder->nbuildque){
			builderc = new int[Builder::nbuilder0];
			for(j = 0; j < Builder::nbuilder0; j++){
				if(top.st == Builder::builder0[j])
					builderc[j] = top.num;
				else
					builderc[j] = 0;
				if(top.st == Builder::builder0[j])
					builderi = j;
			}
		}
		glColor4ub(255,255,255,255);
		glwpos2d(xpos, ypos + (2 + iy++) * 12);
		glwprintf("Buildable Items:");

		/* Mouse cursor highlights */
		int mx = ws.mx - xpos, my = ws.my - ypos;
		if(!modal && 0 < mx && mx < width && (1 + iy) * fonth < my && my < (1 + iy + Builder::nbuilder0) * fonth){
			glColor4ub(0,0,255,127);
			glRecti(xpos, ypos + (my / fonth) * fonth, xpos + width, ypos + (my / fonth + 1) * fonth);
		}

		/* Progress bar of currently building item */
		if(builder->nbuildque && builder->build){
			glColor4ub(0,127,0,255);
			glRecti(xpos, ypos + (builderi + iy + 1) * fonth + 8, xpos + (1. - builder->build / top.st->buildtime) * width, ypos + (builderi + iy + 1) * fonth + fonth);
		}

		glColor4ub(191,191,255,255);
		for(j = 0; j < Builder::nbuilder0; j++){
			const Builder::BuildStatic *sta = Builder::builder0[j];
			glwpos2d(xpos, ypos + (2 + iy++) * fonth);
			glwprintf("%10s  %d  %lg RU", sta->name, builderc ? builderc[j] : 0, sta->cost);
		}
/*		glwpos2d(xpos, ypos + (2 + iy++) * 12);
		glwprintf("Container    %d  %lg RU %lg dm^3", builderc[0], scontainer_build.cost, 1e-3 * scontainer_s.hitradius * scontainer_s.hitradius * scontainer_s.hitradius * 8 * 1e9);
		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Worker       %d  %lg RU %lg dm^3", builderc[1], worker_build.cost, 1e-3 * worker_s.hitradius * worker_s.hitradius * worker_s.hitradius * 8 * 1e9);*/
/*		glwpos2d(xpos, ypos + (2 + iy++) * fonth);
		glwprintf("Interceptor  %d  %lg RU %lg dm^3", builderc ? builderc[0] : 0, sceptor_build.cost, 1e-3 * ((Sceptor*)NULL)->Sceptor::hitradius() * ((Sceptor*)NULL)->Sceptor::hitradius() * ((Sceptor*)NULL)->Sceptor::hitradius() * 8 * 1e9);*/
/*		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Lancer Class %d  %lg RU %lg dm^3", builderc[3], beamer_build.cost, 1e-3 * beamer_s.hitradius * beamer_s.hitradius * beamer_s.hitradius * 8 * 1e9);
		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Sabre Class  %d  %lg RU %lg dm^3", builderc[4], assault_build.cost, 1e-3 * assault_s.hitradius * assault_s.hitradius * assault_s.hitradius * 8 * 1e9);*/
		if(builder->nbuildque)
			delete[] builderc;
	}
	else{
		glColor4ub(255,255,255,255);
		glwpos2d(xpos, ypos + (2 + iy++) * fonth);
		glwprintf("Build Queue:");
		if(builder->nbuildque == 0)
			return;
		glwpos2d(xpos, ypos + (2 + iy++) * fonth);
		glwprintf(top.st->name);
		progress_bar((1. - builder->build / top.st->buildtime), 200, &iy);
		glColor4ub(255,255,255,255);
		for(int i = 0; i < builder->nbuildque; i++){
			if(height < (2 + iy) * fonth)
				return;
			glwpos2d(xpos, ypos + (2 + iy++) * fonth);
			glwprintf("%d X %s %lg RUs", builder->buildque[i].num, builder->buildque[i].st->name, builder->buildque[i].num * builder->buildque[i].st->cost);
		}
	}
}

void GLWdock::draw(GLwindowState &ws, double t){
	typedef Entity::Dockable Dockable;
	int ix;
	if(!docker)
		return;
	glColor4ub(255,255,255,255);
	glwpos2d(this->xpos, this->ypos + 2 * getFontHeight());
	glwprintf("Resource Units: %.0lf", docker->e->getRU());
	glColor4ub(255,255,0,255);
	int fonth = getFontHeight();
	int iy = 1;

	if(!docker)
		return;
	glColor4ub(255,255,255,255);
	glwpos2d(xpos, ypos + (2 + iy++) * fonth);
	glwprintf("Cool: %lg", docker->baycool);
	glwpos2d(xpos, ypos + (2 + iy++) * fonth);
	glwprintf("Remain docked: %s", docker->remainDocked ? "Yes" : "No");
	glwpos2d(xpos, ypos + (2 + iy++) * fonth);
	glwprintf("Grouping: %s", grouping ? "Yes" : "No");
	glwpos2d(xpos, ypos + (2 + iy++) * fonth);
	glwprintf("Docked:");
	glColor4ub(255,255,255,255);

	int mx = ws.mx - xpos, my = ws.my - ypos;
	/* Mouse cursor highlights */
	if(!modal && 0 < mx && mx < width && (iy - 2 <= my / fonth && my / fonth <= iy - 1 || (1 + iy) * fonth < my)){
		glColor4ub(0,0,255,127);
		glRecti(xpos, ypos + (my / fonth) * fonth, xpos + width, ypos + (my / fonth + 1) * fonth);
		glColor4ub(255,255,255,255);
	}

	if(grouping){
		std::map<cpplib::dstring, int> map;

		for(Dockable *e = docker->el; e; e = e->next ? e->next->toDockable() : NULL){
			map[e->dispname()]++;
		}
		for(std::map<cpplib::dstring, int>::iterator it = map.begin(); it != map.end(); it++){
			glwpos2d(xpos, ypos + (2 + iy++) * fonth);
			glwprintf("%d X %s", it->second, (const char*)it->first);
		}
	}
	else{
		for(Dockable *e = docker->el; e; e = e->next ? e->next->toDockable() : NULL){
			if(height < (2 + iy) * fonth)
				return;
			glwpos2d(xpos, ypos + (2 + iy++) * fonth);
			glwprintf("%d X %s %lg m^3", 1, e->dispname(), e->hitradius() * e->hitradius() * e->hitradius());
		}
	}
}


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

	draw_healthbar(this, wd, health / maxhealth(), hitradius(), -1., capacitor / maxenergy());

	if(init == 0) do{
		sufbase = CallLoadSUF("spacecarrier.bin");
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
		init = 1;
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

		glPushMatrix();
		glScaled(scale, scale, scale);
		glMultMatrixd(rotaxis);
		DecalDrawSUF(sufbase, SUF_ATR, NULL, pst, NULL, NULL);
		glPopMatrix();

		glPopMatrix();
	}
}

void Scarry::drawtra(wardraw_t *wd){
	Scarry *p = this;
	Scarry *pt = this;
	Mat4d mat;
	Vec3d pa, pb, pa0(.01, 0, 0), pb0(-.01, 0, 0);
	double scale;

/*	if(scarry_cull(pt, wd))
		return;*/

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
}
