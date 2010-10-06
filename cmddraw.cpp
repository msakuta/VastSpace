#include "cmd.h"
#include "cmd_int.h"
#include "viewer.h"
extern "C"{
#include <clib/gl/gldraw.h>
#include <clib/timemeas.h>
}
#include <string.h>
#include <stdio.h>
#include <stddef.h>

static int console_texfont = 1;
int console_cursorposdisp = 1;
static GLfloat console_bgopacity = .5f;
static GLdouble console_windowsize = 1.f;

void CmdDrawInit(void){
	CvarAdd("console_texfont", &console_texfont, cvar_int);
	CvarAdd("console_cursorposdisp", &console_cursorposdisp, cvar_int);
	CvarAdd("console_bgopacity", &console_bgopacity, cvar_float);
	CvarAdd("console_windowsize", &console_windowsize, cvar_double);
}

void CmdDraw(viewport &gvp){
	int i, p, linechars = int(gvp.w * .8 * 2. / 24.);
	void (*putstring)(const char *, int) = console_texfont ? gldPutTextureStringN : gldPutStrokeStringN;
	double cbot = -(console_windowsize * 2. - 1.);

	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
	glColor4f(0,0,0, console_bgopacity);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
/*		glTranslated(0,0,1);*/
	glBegin(GL_QUADS);
	glVertex2d(-1, cbot);
	glVertex2d(1, cbot);
	glVertex2i(1, 1);
	glVertex2i(-1, 1);
	glEnd();

	glColor4ub(255,255,255,255);
	glBegin(GL_LINE_LOOP);
	glVertex2d(-.9, cbot + .2);
	glVertex2d(-.9 + 32. / gvp.w, cbot + .2);
	glVertex2d(-.9 + 32. / gvp.w, .8);
	glVertex2d(-.9, .8);
	glEnd();
	{
		double top = cbot + .2 + 1.5 * cmddispline / (CB_LINES - (int)(gvp.h * .8 * 2. / 30.));
		glBegin(GL_QUADS);
		glVertex2d(-.9, top);
		glVertex2d(-.9 + 32. / gvp.w, top);
		glVertex2d(-.9 + 32. / gvp.w, top + .1);
		glVertex2d(-.9, top + .1);
		glEnd();
	}
	i = cmddispline + 1;
	for(p = 1; i < CB_LINES; i++) if(cbot + .2 + (p) * 30. / gvp.h < 1.){
		int j = (CB_LINES - i + cmdcurline) % CB_LINES;
		int wraps = 0, l;
/*		glRasterPos2d(-.8, .8 - (i + 1) * 20. / gvp.h);
		gldPutString(cmdbuffer[j]);*/
		wraps = (cmdbuffer[j].len() + linechars - 1) / linechars;
		for(l = 0; l < wraps; l++, p++){
			glPushMatrix();
			glTranslated(-.8, cbot + .2 + (p) * 30. / gvp.h, 0.);
			glScaled(24. / gvp.w, 30. / gvp.w, 1.);
			putstring(&cmdbuffer[j][(wraps - l - 1) * linechars], linechars);
			glPopMatrix();
		}
	}
/*	glRasterPos2d(-.8 - 16. / gvp.w, .8 - (CB_LINES + 1) * 20. / gvp.h);
	gldPutStringN("]", 1);
	gldPutStringN(cmdline, cmdcur);*/
	glPushMatrix();
	glTranslated(-.8 - 24. / gvp.w, cbot + .2, 0.);
	glScaled(24. / gvp.w, 30. / gvp.w, 1.);
	putstring("]", 1);
	putstring(cmdline, cmdline.len());
	putstring("\001", 1); /* cursor */
	glPopMatrix();
/*	glBegin(GL_LINES);
	glVertex2d(-.8 + (cmdcur) * 16. / gvp.w, .8 - (CB_LINES) * 20. / gvp.h);
	glVertex2d(-.8 + (cmdcur) * 16. / gvp.w, .8 - (CB_LINES + 1) * 20. / gvp.h);
	glEnd();*/

	glPopAttrib();
}

