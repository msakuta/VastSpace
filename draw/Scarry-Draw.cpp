#include "../scarry.h"
#include "../judge.h"
extern "C"{
#include <clib/gl/gldraw.h>
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
