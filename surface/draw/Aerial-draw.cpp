/** \file
 * \brief Implementation of Aerial class's drawing methods.
 */

#include "../Aerial.h"
#include "Player.h"
#include "Game.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/mqoadapt.h"
extern "C"{
#include <clib/GL/gldraw.h>
}
#include <cpplib/crc32.h>


bool Aerial::cull(WarDraw *wd)const{
	if(wd->vw->gc->cullFrustum(pos, .012))
		return 1;
	double pixels = .008 * fabs(wd->vw->gc->scale(pos));
//	if(ppixels)
//		*ppixels = pixels;
	if(pixels < 2)
		return 1;
	return 0;
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


