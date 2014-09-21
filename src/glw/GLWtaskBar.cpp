/** \file
 * \brief Implementation of GLtaskBar.
 */
#include "glw/GLWtaskBar.h"
#include "Application.h"
#include "cmd.h"
#include "../cmd_int.h"
#include "antiglut.h"
#include "Viewer.h"
#include "draw/material.h"
#include "Player.h" // GLWmoveOrderButton
#include "sqadapt.h"
#include "glw/message.h"
#include "GLWtip.h"
#include "StaticInitializer.h"
extern "C"{
#include <clib/c.h>
#include <clib/GL/gldraw.h>
}
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define projection(e) glMatrixMode(GL_PROJECTION); e; glMatrixMode(GL_MODELVIEW);

static sqa::Initializer definer("GLWtaskBar", GLWtaskBar::sq_define);


GLWtaskBar::GLWtaskBar(Game *game) : st(game), xbuttonsize(64), ybuttonsize(64){
	xpos = 0;
	ypos = 0;
	height = 1024;
	width = 64;
	align = AlignX0 | AlignY0 | AlignY1;
}

void GLWtaskBar::anim(double dt){
	for(int i = 0; i < buttons.size(); i++) if(buttons[i])
		buttons[i]->anim(dt);
}

int GLWtaskBar::mouse(GLwindowState &ws, int button, int state, int mousex, int mousey){
	if(state == GLUT_KEEP_DOWN || state == GLUT_KEEP_UP){
		for(int i = 0; i < buttons.size(); i++) if(buttons[i])
			buttons[i]->mouse(ws, button, state, mousex, mousey);
	}
	else{
		int y = (mousey) / ybuttonsize;
		GLWrect cr = clientRect();
		if(0 <= y && y < buttons.size()){
			GLWbutton *p = buttons[y];
			if(p)
				return p->mouse(ws, button, state, mousex, mousey);
		}
	}
	return 1;
}

void GLWtaskBar::mouseEnter(GLwindowState &ws){
	for(int i = 0; i < buttons.size(); i++) if(buttons[i])
		buttons[i]->mouseEnter(ws);
}

void GLWtaskBar::mouseLeave(GLwindowState &ws){
	for(int i = 0; i < buttons.size(); i++) if(buttons[i])
		buttons[i]->mouseLeave(ws);
}

bool GLWtaskBar::addButton(GLWbutton *b, int pos){
	int i;
	if(pos < 0){
		pos = buttons.size();
		buttons.push_back(b);
	}
//	else if(0 <= pos && pos < buttons.size())
//		buttons[pos] = b;
	else
		return false;
	b->parent = this;
	b->setExtent(GLWrect(0, pos * ybuttonsize, xbuttonsize, pos * ybuttonsize + ybuttonsize));
	return true;
}

static SQInteger sqf_GLWtaskBar_addButton(HSQUIRRELVM v){
	GLWtaskBar *p = static_cast<GLWtaskBar*>(GLWtaskBar::sq_refobj(v));
	const SQChar *cmd, *path, *tips;
	if(SQ_FAILED(sq_getstring(v, 2, &cmd))){
		// The addButton function is polymorphic, it can take GLWbutton object as argument too.
		GLelement *element = GLelement::sq_refobj(v, 2);
		if(!element)
			return SQ_ERROR;
		// TODO: Type unsafe! element can be object other than GLWbutton.
		if(!p->addButton(static_cast<GLWbutton*>(element)))
			return sq_throwerror(v, _SC("Could not add button"));
		element->sq_pushobj(v, element);
		return 1;
	}
	if(SQ_FAILED(sq_getstring(v, 3, &path)))
		return SQ_ERROR;
	if(SQ_FAILED(sq_getstring(v, 4, &tips)))
		tips = NULL;
	GLWcommandButton *b = new GLWcommandButton(p->getGame(), path, cmd, tips);
	if(!p->addButton(b)){
		delete b;
		return sq_throwerror(v, _SC("Could not add button"));
	}
	// Returning created GLWbutton instance is a bit costly, but it won't be frequent operation.
	b->sq_pushobj(v, b);
	return 1;
}

bool GLWtaskBar::sq_define(HSQUIRRELVM v){
	st::sq_define(v);
	sq_pushstring(v, _SC("GLWtaskBar"), -1);
	sq_pushstring(v, st::s_sqClassName(), -1);
	sq_get(v, -3);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, "GLWtaskBar");
	sq_setclassudsize(v, -1, sizeof(WeakPtr<GLelement>));
	register_closure(v, _SC("constructor"), sqf_constructor);
	register_closure(v, _SC("addButton"), sqf_GLWtaskBar_addButton);
	sq_createslot(v, -3);
	return true;
}

/// Squirrel constructor
SQInteger GLWtaskBar::sqf_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	Game *game = reinterpret_cast<Game*>(sq_getforeignptr(v));

	GLWtaskBar *p = new GLWtaskBar(game);

	sq_assignobj(v, p);
	glwAppend(p);
	return 0;
}

void GLWtaskBar::drawInt(GLwindowState &ws, double t, int mousex, int mousey, int ww, int hh){
	int w = ws.w, h = ws.h, m = ws.m;
	projection((glPushMatrix(), glLoadIdentity(), glOrtho(0, w, h, 0, -1, 1)));
	GLWrect r = extentRect();

	glBegin(GL_QUADS);
	glColor4f(.3f, .3f, .3f, 1.f);
	glVertex2i(r.x0, r.y0);
	glVertex2i(r.x0, r.y1);
	glColor4f(.2f, .2f, .2f, 1.f);
	glVertex2i(r.x1, r.y1);
	glVertex2i(r.x1, r.y0);
	glEnd();

	glPushMatrix();
	glTranslatef(r.x0, r.y0, 0);
	glColor4f(.5,.5,.5,1);
	for(int y = 0; y < buttons.size(); y++){
		if(buttons[y])
			buttons[y]->draw(ws, 0);
	}
	glPopMatrix();

	projection((glPopMatrix()));
}
