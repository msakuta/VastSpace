/** \file
 * \brief Implementation of MobileSuit class's graphical aspects.
 */
#include "../MobileSuit.h"
#include "draw/WarDraw.h"
#include "glw/glwindow.h"
#include "glstack.h"
extern "C"{
#include <clib/mathdef.h>
}



void MobileSuit::drawHUD(WarDraw *wd){
	if(game->player->mover != game->player->cockpitview)
		return;

	// This block draws list of weapons
	{
		int wmax = MAX(wd->vw->vp.w, wd->vw->vp.h);
		double xf = (double)wd->vw->vp.w / wmax;
		double yf = (double)wd->vw->vp.h / wmax;

		static const double wlLeft = 0.5;
		static const double wlRight = 0.99;
		static const double wlHeight = 0.12;
		static const double wlMargin = 0.01;

		GLpmatrix pm;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-xf, xf, -yf, yf, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		for(int i = 0; i < getWeaponCount(); i++){
			WeaponParams param;
			if(!getWeaponParams(i, param))
				continue;

			WeaponStatus &wst = weaponStatus[i];

			double ybase = -yf + 3 * wlHeight + wlMargin - i * wlHeight;
			for(int j = 0; j < 2; j++){
				glColor4fv(j == 0 ? Vec4f(0,0,0,0.75) : weapon == i ? Vec4f(1,1,1,1) : Vec4f(0.75,0.75,0,1));
				glBegin(j == 0 ? GL_QUADS : GL_LINE_LOOP);
				glVertex2d( wlLeft, ybase);
				glVertex2d( wlRight, ybase);
				glVertex2d( wlRight, ybase + wlHeight - wlMargin);
				glVertex2d( wlLeft, ybase + wlHeight - wlMargin);
				glEnd();
			}

			static const double fontSize = 0.0025;

			// Show weapon name
			glPushMatrix();
			glTranslated(wlLeft + 0.05, ybase + fontSize * GLwindow::getFontHeight() * 2, 0);
			glScaled(fontSize, -fontSize, .1);
			glwPutTextureString(gltestp::dstring(i + 1) << " " << param.name);
			glPopMatrix();

			// Show ammunition count
			double ammoFactor = 0;
			glPushMatrix();
			glTranslated(wlLeft + 0.05, ybase + fontSize * GLwindow::getFontHeight() * 1, 0);
			glScaled(fontSize, -fontSize, .1);
			if(wst.reload != 0){
				glwPutTextureString("  RELOADING");
				ammoFactor = 1. - wst.reload / param.reloadTime;
			}
			else if(param.magazineSize){
				glwPutTextureString(gltestp::dstring("  ") << wst.magazine << "/" << param.magazineSize);
				ammoFactor = (double)wst.magazine / param.magazineSize;
			}
			glPopMatrix();

			static const double margin = 0.05;
			// Show ammunition graph
			if(param.magazineSize || wst.reload){
				glPushMatrix();
				glTranslated(wlLeft + margin, ybase + 0.015, 0);
				glScaled((wlRight - margin) - (wlLeft + margin), 0.01, 1);
				glBegin(GL_QUADS);
				glColor4f(0,1,0,1);
				glVertex2d(0, 0);
				glVertex2d(ammoFactor, 0);
				glVertex2d(ammoFactor, 1);
				glVertex2d(0, 1);
				glColor4f(1,0,0,1);
				glVertex2d(ammoFactor, 0);
				glVertex2d(1, 0);
				glVertex2d(1, 1);
				glVertex2d(ammoFactor, 1);
				glEnd();
				glPopMatrix();
			}
		}

		glPopMatrix();
	}

	GLpmatrix pm;

	int wmin = MIN(wd->vw->vp.w, wd->vw->vp.h);
	double xf = (double)wd->vw->vp.w / wmin;
	double yf = (double)wd->vw->vp.h / wmin;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
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
		int wi = n == 0 ? 0 : 2; // Weapon index
		WeaponParams param;
		if(!getWeaponParams(wi, param))
			continue;

		WeaponStatus &wst = weaponStatus[wi];

		int divisor = param.magazineSize;
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
					float f = wst.reload != 0. ?
						i < divisor * (1. - wst.reload / param.reloadTime) ? 1. : 0. :
						(i < wst.magazine) ? 1. : 0.;
					glColor4f(1, f, f, 1);
				}
				else{
					if(wst.reload != 0.)
						glColor4f(1,0,0, i < divisor * (1. - wst.reload / param.reloadTime) ? .8 : .3);
					else
						glColor4f(1,1,1, (i < wst.magazine) ? .8 : .3);
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

