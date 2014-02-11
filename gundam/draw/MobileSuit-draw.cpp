/** \file
 * \brief Implementation of MobileSuit class's graphical aspects.
 */
#include "../MobileSuit.h"
#include "draw/WarDraw.h"
#include "glstack.h"
extern "C"{
#include <clib/mathdef.h>
}




void MobileSuit::drawHUD(WarDraw *wd){
	if(game->player->mover != game->player->cockpitview)
		return;
	GLpmatrix pm;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	int wmin = min(wd->vw->vp.w, wd->vw->vp.h);
	double xf = (double)wd->vw->vp.w / wmin;
	double yf = (double)wd->vw->vp.h / wmin;
	glOrtho(-xf, xf, -yf, yf, -1, 1);
//	glOrtho(0, wd->vw->vp.w, wd->vw->vp.h, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// Crosshair
	glColor4f(1,1,1,1);
	glBegin(GL_LINES);
	glVertex2d(-.05, 0);
	glVertex2d( .05, 0);
	glVertex2d(0, -.05);
	glVertex2d(0,  .05);
	glEnd();

	glScaled(.2, .2, .2);
	glColor4f(1,1,1,.3);

	glBegin(GL_QUAD_STRIP);
	glVertex2d(-.3, .3);
	glVertex2d(-.4, .3);
	glVertex2d(-.4, .15);
	glVertex2d(-.5, .15);
	glVertex2d(-.4, -.15);
	glVertex2d(-.5, -.15);
	glVertex2d(-.3, -.3);
	glVertex2d(-.4, -.3);
	glEnd();

	glBegin(GL_QUAD_STRIP);
	glVertex2d( .3, .3);
	glVertex2d( .4, .3);
	glVertex2d( .4, .15);
	glVertex2d( .5, .15);
	glVertex2d( .4, -.15);
	glVertex2d( .5, -.15);
	glVertex2d( .3, -.3);
	glVertex2d( .4, -.3);
	glEnd();

	// Ammo indicator
	for(int n = 0; n < 2; n++){
		int divisor = n ? getRifleMagazineSize() : getVulcanMagazineSize();
		glPushMatrix();
		glTranslated((n * 2 - 1) * .2, 0., 0.);
		for(int m = 0; m < 2; m++){
			for(int i = 0; i < divisor; i++){
				double s0 = sin((n * 2 - 1) * (i + .1) * M_PI / divisor);
				double c0 = -cos((i + .1) * M_PI / divisor);
				double s1 = sin((n * 2 - 1) * (i + .9) * M_PI / divisor);
				double c1 = -cos((i + .9) * M_PI / divisor);
				double r0 = .8;
				double r1 = 1.;
				if(m){
					float f = (n ? freload != 0. : getVulcanCooldownTime() < vulcancooldown) ?
						i < divisor * (n ? 1. - freload / getReloadTime() : 1. - vulcancooldown / getVulcanReloadTime()) ? 1. : 0. :
						(n ? i < magazine : i < vulcanmag) ? 1. : 0.;
					glColor4f(1, f, f, 1);
				}
				else{
					if(n ? freload != 0. : getVulcanCooldownTime() < vulcancooldown)
						glColor4f(1,0,0, i < divisor * (n ? 1. - freload / getReloadTime() : 1. - vulcancooldown / getVulcanReloadTime()) ? .8 : .3);
					else
						glColor4f(1,1,1, (n ? i < magazine : i < vulcanmag) ? .8 : .3);
				}
				glBegin(m ? GL_LINE_LOOP : GL_QUADS);
				glVertex2d(r0 * s0, r0 * c0);
				glVertex2d(r1 * s0, r1 * c0);
				glVertex2d(r1 * s1, r1 * c1);
				glVertex2d(r0 * s1, r0 * c1);
				glEnd();
			}
		}
		glPopMatrix();
	}

	glPopMatrix();
}

