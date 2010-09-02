/** \file
 * \brief Implementation of GLWmessage.
 */
#include "../cmd.h"
#include "../sqadapt.h"
#include "message.h"
extern "C"{
#include <clib/c.h>
#include <clib/GL/gldraw.h>
}
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


#define fontwidth (GLwindow::glwfontwidth * glwfontscale)
#define fontheight (GLwindow::glwfontheight * glwfontscale)


/** \brief Constructs a message window.
 * \param messagestring The message string to display. The window size is adjusted to fit to the string.
 * \param timer Sets time to display the message. Set zero to introduce infinite message.
 * \param v A Squirrel VM to invoke onDestroy script on.
 * \param onDestroy Squirrel script executed on destructon of this object. */
GLWmessage::GLWmessage(const char *messagestring, double atimer, HSQUIRRELVM av, const char *aonDestroy)
: resized(false), string(messagestring), timer(atimer), v(av), onDestroy(aonDestroy){
	sq_resetobject(&hoOnDestroy);
}

/** \brief Constructs a message window with destroy event handler as a Squirrel function.
 * \param messagestring The message string to display. The window size is adjusted to fit to the string.
 * \param timer Sets time to display the message. Set zero to introduce infinite message.
 * \param v A Squirrel VM to invoke hoOnDestroy function on.
 * \param ahoOnDestroy Squirrel function or closure object called on destructon of this object.
 *        Parameter is not checked if it's valid function but called anyway. */
GLWmessage::GLWmessage(const char *messagestring, double atimer, HSQUIRRELVM av, HSQOBJECT &ahoOnDestroy)
: resized(false), string(messagestring), timer(atimer), v(av), hoOnDestroy(ahoOnDestroy){
	// Unhappy mess with raw object handling, but we cannot lose the Squirrel object.
	sq_addref(g_sqvm, &hoOnDestroy);
}

/// Ensures destroy event handler to be invoked and member Squirrel object released.
GLWmessage::~GLWmessage(){
	if(onDestroy.len()){
		if(SQ_FAILED(sq_compilebuffer(v, onDestroy, onDestroy.len(), _SC("GLWmessage"), 0))){
			CmdPrint("Compile error");
		}
		else{
			sq_pushroottable(v);
			sq_call(v, 1, 0, 0);
			sq_pop(v, 1);
		}
	}
	else{
		sq_pushobject(v, hoOnDestroy);
		sq_pushroottable(v); // The this pointer could be altered to realize method pointer.
		sq_call(v, 1, 0, 1);
		sq_pop(v, 1);
		sq_release(v, &hoOnDestroy); // Make sure to release
	}
}

void GLWmessage::draw(GLwindowState &ws, double){
	if(!string)
		return;

	if(!resized){
		resized = true;

		int hcenter = ws.w / 2;
		int width = glwsizef(string);

		GLWrect localrect = GLWrect(hcenter - width / 2, ws.h / 3, hcenter + width / 2, ws.h / 3 + long(fontheight));

		GLWrect wrect = adjustRect(localrect);

		setExtent(wrect);
	}

	GLWrect r = clientRect();
	glwpos2d(r.x0, r.y0 + fontheight);
	glwprintf(string);
}

void GLWmessage::anim(double dt){
	if(timer){
		if(timer - dt < 0.){
			postClose();
		}
		else
			timer -= dt;
	}
}

/// Define class GLWmessage for Squirrel
void GLWmessage::sq_define(HSQUIRRELVM v){
	sq_pushstring(v, _SC("GLWmessage"), -1);
	sq_pushstring(v, _SC("GLwindow"), -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_pushstring(v, _SC("constructor"), -1);
	sq_newclosure(v, sqf_constructor, 0);
	sq_createslot(v, -3);
	sq_createslot(v, -3);
}

/// Squirrel constructor
SQInteger GLWmessage::sqf_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	const SQChar *string;
	SQFloat timer;
	if(argc <= 1 || SQ_FAILED(sq_getstring(v, 2, &string)))
		string = "";
	if(argc <= 2 || SQ_FAILED(sq_getfloat(v, 3, &timer)))
		timer = 0.;

	GLWmessage *p = NULL;
	if(3 <= argc){
		const SQChar *onDestroy;
		HSQOBJECT ho;
		if(SQ_SUCCEEDED(sq_getstring(v, 4, &onDestroy)))
			p = new GLWmessage(string, timer, v, onDestroy);
		else if(SQ_SUCCEEDED(sq_getstackobj(v, 4, &ho)))
			p = new GLWmessage(string, timer, v, ho);
	}
	if(!p)
		p = new GLWmessage(string, timer);

	if(!sqa_newobj(v, p, 1))
		return SQ_ERROR;
	glwAppend(p);
	return 0;
}


