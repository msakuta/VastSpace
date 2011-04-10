#include "../Assault.h"
#include "../Beamer.h"
#include "draw/material.h"
#include "draw/OpenGLState.h"
extern "C"{
#include <clib/gl/gldraw.h>
}


#define BEAMER_SCALE .0002






suf_t *Assault::sufbase = NULL;


void Assault::draw(wardraw_t *wd){
	Assault *const p = this;
	static OpenGLState::weak_ptr<bool> init;
	static suftex_t *pst;
	static GLuint list = 0;
	if(!w)
		return;

	/* cull object */
	if(cull(wd))
		return;
//	wd->lightdraws++;

	draw_healthbar(this, wd, health / maxhealth(), .1, shieldAmount / maxshield(), capacitor / maxenergy());

	if(!init) do{
		sufbase = CallLoadSUF("models/assault.bin");
		if(!sufbase) break;
		Beamer::cache_bridge();
		pst = gltestp::AllocSUFTex(sufbase);
		init.create(*openGLState);
	} while(0);
	if(sufbase){
		static const double normal[3] = {0., 1., 0.};
		double scale = BEAMER_SCALE;
		static const GLdouble rotaxis[16] = {
			-1,0,0,0,
			0,1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};
		Mat4d mat;

		// testing global mesh
/*		glColor4ub(255,0,255,255);
		glBegin(GL_LINES);
		for(int i = -10; i <= 10; i++){
			glVertex3d(i / 10., -1., 0.);
			glVertex3d(i / 10.,  1., 0.);
			glVertex3d(-1., i / 10., 0.);
			glVertex3d( 1., i / 10., 0.);
		}
		glEnd();*/

		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);
/*		if(!list){
		glNewList(list = glGenLists(1), GL_COMPILE);*/
#if 0
		for(int i = 0; i < nhitboxes; i++){
			Mat4d rot;
			glPushMatrix();
			gldTranslate3dv(hitboxes[i].org);
			rot = hitboxes[i].rot.tomat4();
			glMultMatrixd(rot);
			hitbox hb(mat.vp3(hitboxes[i].org), this->rot * hitboxes[i].rot, hitboxes[i].sc);
			hitbox_draw(this, hitboxes[i].sc/*, jHitBoxPlane(hb, Vec3d(0,0,0), Vec3d(0,0,1))*/);
			glPopMatrix();
		}
#endif

		glPushMatrix();
		glScaled(scale, scale, scale);
		glMultMatrixd(rotaxis);
/*		glPushAttrib(GL_TEXTURE_BIT);*/
/*		DrawSUF(beamer_s.sufbase, SUF_ATR, &g_gldcache);*/
		DecalDrawSUF(sufbase, SUF_ATR, NULL, pst, NULL, NULL);
/*		DecalDrawSUF(&suf_beamer, SUF_ATR, &g_gldcache, beamer_s.tex, pf->sd, &beamer_s);*/
/*		glPopAttrib();*/
		glPopMatrix();
/*		glEndList();
		}

		glCallList(list);*/

/*		for(i = 0; i < numof(pf->turrets); i++)
			mturret_draw(&pf->turrets[i], pt, wd);*/
		glPopMatrix();
	}
}

void Assault::drawtra(wardraw_t *wd){
	st::drawtra(wd);
	if(cull(wd))
		return;
	drawCapitalBlast(wd, Vec3d(0,-0.003,.06), .01);
	drawShield(wd);
}

void Assault::drawOverlay(wardraw_t *){
	glBegin(GL_LINE_LOOP);
	glVertex2d(-.40, -.25);
	glVertex2d(-.80, -.1);
	glVertex2d(-.80,  .05);
	glVertex2d(-.4 ,  .3);
	glVertex2d( .1 ,  .3);
	glVertex2d( .7 ,  .6);
	glVertex2d( .7 ,  .3);
	glVertex2d( .8 ,  .3);
	glVertex2d( .8 , -.2);
	glVertex2d( .7 , -.2);
	glVertex2d( .7 , -.6);
	glVertex2d( .4 , -.4);
	glVertex2d( .3 , -.25);
	glEnd();
}
