/** \file
 * \brief Implementation of GLwindow and its subclasses.
 * Implements GLWbutton branch too.
 */
#include "glw/glwindow.h"
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

/// There is only one font size. It could be bad for some future.
double GLwindow::glwfontscale = 1.; ///< Font size for all GUI.
#define fontwidth (GLwindow::glwfontwidth * GLwindow::glwfontscale)
#define fontheight (GLwindow::glwfontheight * GLwindow::glwfontscale)
double GLwindow::getFontWidth(){return fontwidth;}
double GLwindow::getFontHeight(){return fontheight;}

/// Window margin
const long margin = 6;

void GLelement::changeExtent(){
	if(onChangeExtent)
		onChangeExtent(this);
}

GLWrect GLelement::extentRect()const{return GLWrect(xpos, ypos, xpos + width, ypos + height);}

void GLelement::sq_pushobj(HSQUIRRELVM v, GLelement *p){
	sq_pushroottable(v);
	sq_pushstring(v, p->sqClassName(), -1);
	if(SQ_FAILED(sq_get(v, -2)))
		throw SQFError("Something's wrong with GLelement class definition.");
	if(SQ_FAILED(sq_createinstance(v, -1)))
		throw SQFError("Couldn't create class GLelement");
	sq_assignobj(v, p, -1);
	sq_remove(v, -2); // Remove Class
	sq_remove(v, -2); // Remove root table
}

GLelement *GLelement::sq_refobj(HSQUIRRELVM v, SQInteger idx){
	SQUserPointer up;
	// If the instance does not have a user pointer, it's a serious exception that might need some codes fixed.
	if(SQ_FAILED(sq_getinstanceup(v, idx, &up, NULL)) || !up)
		throw SQFError("Something's wrong with Squirrel Class Instace of GLelement.");
	return *(WeakPtr<GLelement>*)up;
}

/// \param idx Index of the instance in the stack. Default is 1, which is the case of constructor.
void GLelement::sq_assignobj(HSQUIRRELVM v, GLelement *p, SQInteger idx){
	SQUserPointer up;
	if(SQ_FAILED(sq_getinstanceup(v, idx, &up, NULL)))
		throw SQFError("Something's wrong with Squirrel Class Instace of GLelement.");

	// We made the WeakPtr's raw type a GLelement pointer, though GLelement is barely used
	// compared to GLwindow, because having many base types for WeakPtr would be inefficient.
	// I have considered making this type WeakPtr<GLwindow>, but it would require duplicate
	// codes to take care of.
	new(up) WeakPtr<GLelement>(p);

	sq_setreleasehook(v, idx, sqh_release);
}

const SQUserPointer GLelement::tt_GLelement = SQUserPointer("GLelement");

static Initializer init_GLelement("GLelement", GLelement::sq_define);

bool GLelement::sq_define(HSQUIRRELVM v){
	StackReserver sr(v);
	sq_pushstring(v, _SC("GLelement"), -1);
	if(SQ_SUCCEEDED(sq_get(v, -2)))
		return false;
	sq_pushstring(v, _SC("GLelement"), -1);
	sq_newclass(v, SQFalse);
	sq_settypetag(v, -1, tt_GLelement);
	sq_setclassudsize(v, -1, sizeof(WeakPtr<GLelement>));
	register_closure(v, _SC("_get"), sqf_get);
	register_closure(v, _SC("_set"), sqf_set);
	register_closure(v, _SC("close"), sqf_close);
	sq_createslot(v, -3);
	return true;
}

SQInteger GLelement::sqGet(HSQUIRRELVM v, const SQChar *wcs)const{
	if(!strcmp(wcs, _SC("x"))){
		SQInteger x = extentRect().x0;
		sq_pushinteger(v, x);
		return 1;
	}
	else if(!strcmp(wcs, _SC("y"))){
		SQInteger y = extentRect().y0;
		sq_pushinteger(v, y);
		return 1;
	}
	else if(!strcmp(wcs, _SC("width"))){
		sq_pushinteger(v, extentRect().width());
		return 1;
	}
	else if(!strcmp(wcs, _SC("height"))){
		sq_pushinteger(v, extentRect().height());
		return 1;
	}
	else
		return SQ_ERROR;
}

SQInteger GLelement::sqf_get(HSQUIRRELVM v){
	try{
		GLelement *p = sq_refobj(v);
		const SQChar *wcs;
		sq_getstring(v, 2, &wcs);

		// Check if alive first
		if(!strcmp(wcs, _SC("alive"))){
			sq_pushbool(v, SQBool(!!p));
			return 1;
		}

		// It's not uncommon that a customized Squirrel code accesses a destroyed object.
		if(!p)
			return sq_throwerror(v, _SC("The object being accessed is destructed in the game engine"));

		return p->sqGet(v, wcs);
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.what());
	}
}

SQInteger GLelement::sqSet(HSQUIRRELVM v, const SQChar *wcs){
	if(!strcmp(wcs, _SC("x"))){
		SQInteger x;
		if(SQ_FAILED(sq_getinteger(v, 3, &x)))
			return SQ_ERROR;
		GLWrect r = extentRect();
		r.x1 += x - r.x0;
		r.x0 = x;
		setExtent(r);
		return 0;
	}
	else if(!strcmp(wcs, _SC("y"))){
		SQInteger y;
		if(SQ_FAILED(sq_getinteger(v, 3, &y)))
			return SQ_ERROR;
		GLWrect r = extentRect();
		r.y1 += y - r.y0;
		r.y0 = y;
		setExtent(r);
		return 0;
	}
	else if(!strcmp(wcs, _SC("width"))){
		SQInteger x;
		if(SQ_FAILED(sq_getinteger(v, 3, &x)))
			return SQ_ERROR;
		GLWrect r = extentRect();
		r.x1 = r.x0 + x;
		setExtent(r);
		return 0;
	}
	else if(!strcmp(wcs, _SC("height"))){
		SQInteger y;
		if(SQ_FAILED(sq_getinteger(v, 3, &y)))
			return SQ_ERROR;
		GLWrect r = extentRect();
		r.y1 = r.y0 + y;
		setExtent(r);
		return 0;
	}
	else
		return SQ_ERROR;
}

SQInteger GLelement::sqf_set(HSQUIRRELVM v){
	try{
		if(sq_gettop(v) < 3)
			return SQ_ERROR;
		GLelement *p = sq_refobj(v);
		const SQChar *wcs;
		sq_getstring(v, 2, &wcs);

		// It's not uncommon that a customized Squirrel code accesses a destroyed object.
		if(!p)
			return sq_throwerror(v, _SC("The object being accessed is destructed in the game engine"));

		return p->sqSet(v, wcs);
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.what());
	}
}

const SQChar *GLelement::sqClassName()const{
	return _SC("GLelement");
}

SQInteger GLelement::sqf_close(HSQUIRRELVM v){
	GLelement *p = sq_refobj(v);
	// close?
//	p->postClose();
	return 0;
}

/// \brief The release hook of Entity that clears the weak pointer.
SQInteger GLelement::sqh_release(SQUserPointer p, SQInteger){
	((WeakPtr<GLelement>*)p)->~WeakPtr<GLelement>();
	return 1;
}



GLwindow *glwlist = NULL; ///< Global list of all GLwindows. 
GLwindow *GLwindow::glwfocus = NULL; ///< Focused window.
GLwindow *glwdrag = NULL; ///< Window being dragged.
int glwdragpos[2] = {0}; ///< Memory for starting position of dragging.


GLwindow::GLwindow(Game *game, const char *atitle) : GLelement(game), title(atitle), modal(NULL), next(NULL), onFocusEnter(NULL), onFocusLeave(NULL){
	// Default is to snap on border
	align |= AlignAuto;
}

/// Appends a GLwindow to screen window list.
/// Usually calling this function manually is not necessary because the constructor does it.
GLwindow **glwAppend(GLwindow *wnd){
	assert(wnd != glwlist);
	wnd->next = glwlist;
	glwlist = wnd;
	return &glwlist;
}

/// Brings the given window on top and set keyboard focus on it.
/// \relates GLwindow
void glwActivate(GLwindow **ppwnd){
	GLwindow **last, *wnd = *ppwnd;
/*	for(last = &glwlist; *last; last = &(*last)->next);
	if(last != &(*ppwnd)->next){
		*last = *ppwnd;
		*ppwnd = wnd->next;
		wnd->next = NULL;
	}*/
	*ppwnd = wnd->next;
	wnd->next = glwlist;
	glwlist = wnd;
	if(wnd->focusable()){
		wnd->focus();
		wnd->flags &= ~GLW_COLLAPSE;
	}
	else if(GLwindow::getFocus() == wnd)
		GLwindow::getFocus()->defocus();
}

/// Called when the OpenGL viewport (usually the OS's window) is changed its size.
void GLwindow::reshapeFunc(int w, int h){
	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);
	GLwindow *wnd;
	for(wnd = glwlist; wnd; wnd = wnd->next){
		GLWrect r = wnd->extentRect();
		unsigned align = wnd->getAlign();
		if(align & AlignAuto){
			// If automatic alignment is enabled, ignore preset flags first
			align &= ~(AlignX0 | AlignX1 | AlignY0 | AlignY1);

			// If a window is aligned to or crossing with border, snap it
			if(r.x0 <= 0)
				align |= AlignX0;
			if(vp[2] - vp[0] <= r.x1 || w <= r.x1)
				align |= AlignX1;
			if(r.y0 <= 0)
				align |= AlignY0;
			if(vp[3] - vp[1] <= r.y1 || h <= r.y1)
				align |= AlignY1;
		}

		// We really wanted to name the flags AlignLeft, AlignRight and such.
		// But concept of up and down can vary in coordinate systems, so we
		// just name the flags with relative coordinate values.
		if((AlignX0 | AlignX1) == (align & (AlignX0 | AlignX1)))
			r.x1 = w;
		else if(align & AlignX1)
			r.rmove(w - (vp[2] - vp[0]), 0);
		if((AlignY0 | AlignY1) == (align & (AlignY0 | AlignY1)))
			r.y1 = h;
		else if(align & AlignY1)
			r.rmove(0, h - (vp[3] - vp[1]));


		wnd->setExtent(r);
	}
}

int r_window_scissor = 1; ///< Whether enable scissor tests to draw client area.
static int s_minix;
//static const int r_titlebar_height = 12;
#define r_titlebar_height fontheight

template<typename T>
static T clamp(const T &src, const T &min, const T &max){
	return src < min ? min : max < src ? max : src;
}

/// Draw a GLwindow with its non-client frame.
void GLwindow::drawInt(GLwindowState &gvp, double t, int wx, int wy, int ww, int wh){
//	GLwindow *wnd = this;
	int s_mousex = gvp.mx, s_mousey = gvp.my;
	char buf[128];
	GLint vp[4];
	int w = gvp.w, h = gvp.h, m = gvp.m, mi, i, x;
	GLubyte alpha;
	double left, bottom;

	if(flags & GLW_INVISIBLE)
		return;

	glGetIntegerv(GL_VIEWPORT, vp);
	mi = MIN(h, w);
	left = -(double)w / m;
	bottom = -(double)h / m;

	int titleheight = fontheight + margin;
	const GLWrect cr = clientRect();

	projection((glPushMatrix(), glLoadIdentity(), glOrtho(0, w, h, 0, -1, 1)));
	glLoadIdentity();
	glColor4ub(0,0,0, flags & GLW_PINNED ? 63 : 192);
	glBegin(GL_QUADS);
	glVertex2d(wx, wy);
	glVertex2d(wx + ww, wy);
	glVertex2d(wx + ww, wy + wh);
	glVertex2d(wx, wy + wh);
	glEnd();
	const int border = (flags & (GLW_POPUP | GLW_COLLAPSE) ? 2 : margin)/* + !!(wnd->flags & GLW_SIZEABLE)*/;
	alpha = glwfocus != this && flags & GLW_PINNED ? wx <= s_mousex && s_mousex < wx + ww && wy <= s_mousey && s_mousey < wy + wh ? 63 : 31 : 255;
	for(i = 0; i < border; i++){
		static const GLubyte cols[4] = {255, 127, 95, 191};
		GLubyte col;
		int ofs = i, k = i ? 0 : 2;
		col = cols[(k) % 4];
		glColor4ub(col * !modal, col, col * (glwfocus == this), alpha);
		glBegin(GL_LINES);
		glVertex2i(wx + ofs, wy + ofs);
		glVertex2i(wx + ww - ofs, wy + ofs);
		col = cols[(k + 1) % 4];
		glColor4ub(col * !modal, col, col * (glwfocus == this), alpha);
		glVertex2i(wx + ww - ofs, wy + ofs);
		glVertex2i(wx + ww - ofs, wy + wh - ofs);
		col = cols[(k + 2) % 4];
		glColor4ub(col * !modal, col, col * (glwfocus == this), alpha);
		glVertex2i(wx + ww - ofs, wy + wh - ofs);
		glVertex2i(wx + ofs, wy + wh - ofs);
		col = cols[(k + 3) % 4];
		glColor4ub(col * !modal, col, col * (glwfocus == this), alpha);
		glVertex2i(wx + ofs, wy + wh - ofs);
		glVertex2i(wx + ofs, wy + ofs);
		glEnd();
	}
	if(!(flags & GLW_COLLAPSE) && r_window_scissor){
		GLWrect r = cr;
		if(title.len() || flags & (GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE))
			r.y0 -= fontheight + margin;
		glPushAttrib(GL_SCISSOR_BIT);

		// Try to put the scissor rectangle inside the viewport, because if we do not, OpenGL raises
		// an error. The code is so counter-intuitive because not only the rectangle is upside down
		// in screen coords, but also 3rd and 4th parameters are width and height, not absolute coords.
		int x0 = clamp<int>(r.x0 - 1, 0, gvp.w);
		int y0 = clamp<int>(gvp.h - (r.y1 + 1), 0, gvp.h);
		glScissor(x0, y0, clamp<int>(r.x1 + 1, 0, gvp.w) - x0, clamp<int>(gvp.h - (r.y0 - 1), 0, gvp.h) - y0);

		glEnable(GL_SCISSOR_TEST);
	}
	if(!(flags & GLW_COLLAPSE)){
		gvp.mousex = gvp.mx - cr.x0;
		gvp.mousey = gvp.my - cr.y0;
		draw(gvp, t);
	}

	// Collapsed or pinned window cannot be sized.
	if((flags & (GLW_COLLAPSE | GLW_PINNED | GLW_SIZEABLE)) == GLW_SIZEABLE){
		glColor4ub(255, 255, 255 * (glwfocus == this), 255);
		glBegin(GL_LINE_LOOP);
		glVertex2d(cr.x1 - 2, cr.y1 - 10);
		glVertex2d(cr.x1 - 2, cr.y1 - 2);
		glVertex2d(cr.x1 - 10, cr.y1 - 2);
		glEnd();
	}

	// System command icon buttons. Do not draw if pinned and mouse pointer is afar.
	if(32 < alpha && !(flags & GLW_COLLAPSE) && (title.len() || flags & (GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE))){
		glColor4ub(255,255, 255 * (glwfocus == this), alpha);
		glBegin(GL_LINES);
		glVertex2d(wx, wy + titleheight);
		glVertex2d(wx + ww, wy + titleheight);
		const int iconsize = titleheight - margin;
		x = width - titleheight;
		if(flags & GLW_CLOSE){
			glVertex2i(xpos + x, ypos + margin);
			glVertex2i(xpos + x, ypos + titleheight);
			glVertex2i(xpos + width - margin - 2, ypos + margin + 2);
			glVertex2i(xpos + width - (titleheight - 2), ypos + titleheight - 2);
			glVertex2i(xpos + width - (titleheight - 2), ypos + margin + 2);
			glVertex2i(xpos + width - margin - 2, ypos + titleheight - 2);
			x -= iconsize;
		}
		if(flags & GLW_COLLAPSABLE){
			glVertex2i(xpos + x, ypos + margin);
			glVertex2i(xpos + x, ypos + titleheight);
			glVertex2i(xpos + x + 2, ypos + titleheight - 2);
			glVertex2i(xpos + x + iconsize - 2, ypos + titleheight - 2);
			if(flags & GLW_COLLAPSE){
				glVertex2i(xpos + x + iconsize - 2, ypos + titleheight - 2);
				glVertex2i(xpos + x + iconsize - 2, ypos + 2);
				glVertex2i(xpos + x + iconsize - 2, ypos + 2);
				glVertex2i(xpos + x + 2, ypos + 2);
				glVertex2i(xpos + x + 2, ypos + 2);
				glVertex2i(xpos + x + 2, ypos + titleheight - 2);
			}
			x -= iconsize;
		}
		if(flags & GLW_PINNABLE){
			glVertex2i(xpos + x, ypos + margin);
			glVertex2i(xpos + x, ypos + titleheight);
			glVertex2i(xpos + x + iconsize / 2, ypos + margin + 2);
			glVertex2i(xpos + x + 2, ypos + margin + iconsize / 2);
			glVertex2i(xpos + x + 2, ypos + margin + iconsize / 2);
			glVertex2i(xpos + x + iconsize / 2, ypos + margin + iconsize - 2);
			glVertex2i(xpos + x + iconsize / 2, ypos + margin + iconsize - 2);
			glVertex2i(xpos + x + iconsize - 2, ypos + margin + iconsize / 2);
			glVertex2i(xpos + x + iconsize - 2, ypos + margin + iconsize / 2);
			glVertex2i(xpos + x + iconsize / 2, ypos + margin + 2);
			if(flags & GLW_PINNED){
				glVertex2i(xpos + x + iconsize / 2 - 2, ypos + margin + iconsize / 2 - 2);
				glVertex2i(xpos + x + iconsize / 2 + 2, ypos + margin + iconsize / 2 + 2);
				glVertex2i(xpos + x + iconsize / 2 + 2, ypos + margin + iconsize / 2 - 2);
				glVertex2i(xpos + x + iconsize / 2 - 2, ypos + margin + iconsize / 2 + 2);
			}
			x -= iconsize;
		}
		glEnd();
	}
	else
		x = width;
	if(32 < alpha && title.len()){
		glColor4ub(255,255,255,255);
		if(flags & GLW_COLLAPSE)
			glwpos2d(wx, wy + fontheight);
		else
			glwpos2d(wx + margin, wy + titleheight);
//		glwprintf("%.*s", (int)(x / fontwidth), title);
		glwprintf("%s", title.c_str());
	}
	if(!(flags & GLW_COLLAPSE) && r_window_scissor){
		glPopAttrib();
	}
	projection(glPopMatrix());
}

/** \brief An intermediate function that eventually calls anim().
 *
 * This function is using recursive call to do reverse rendering order,
 * for reversing message process instead is more costly.
 */
void GLwindow::glwDraw(GLwindowState &vp, double t, int *minix){
	if(!this)
		return;

	next->glwDraw(vp, t, minix);

	if(!(flags & GLW_COLLAPSE))
		drawInt(vp, t, xpos, ypos, width, height);
}

/** \brief Draw minimized windows.
 *
 * Minimized windows never overlap, so drawing order can be arbitrary,
 * making recursive call unnecessary.
 */
void GLwindow::glwDrawMinimized(GLwindowState &gvp, double t, int *pp){
	GLwindow *wndy = this;
	GLwindow *wnd;
	int minix = 2, miniy = gvp.h - r_titlebar_height - 2;
	for(wnd = glwlist; wnd; wnd = wnd->next) if(wnd->flags & GLW_COLLAPSE){
		int wx, wy, ww, wh;
		ww = wnd->title.len() ? wnd->title.len() * fontwidth + 2 : 80;
		if(ww < gvp.w && gvp.w < minix + ww)
			wx = minix = 2, wy = miniy -= r_titlebar_height + 2;
		else
			wx = minix, wy = miniy;
		wh = r_titlebar_height;
		minix += ww + 2;
		wnd->drawInt(gvp, t, wx, wy, ww, wh);
	}
}

/// Call anim() for all windows.
void GLwindow::glwAnim(double dt){
	for(GLwindow *wnd = glwlist; wnd; wnd = wnd->next)
		wnd->anim(dt);
}

/// Finds and returns pointer to pointer to the object.
GLwindow **glwFindPP(GLwindow *wnd){
	GLwindow **ret;
	for(ret = &glwlist; *ret; ret = &(*ret)->next) if(*ret == wnd)
		return ret;
	return NULL;
}

int GLwindow::glwMouseCursorState(int x, int y){
	for(GLwindow *glw = glwlist; glw; glw = glw->next) if(glw->extentRect().include(x, y)){
		return glw->mouseCursorState(x, y);
	}
	return 0;
}

int GLwindow::mouseCursorState(int mousex, int mousey)const{
	return 0;
}

void GLwindow::glwFree(){
	GLwindow *wnd = this;
	GLwindow **ppwnd, *wnd2;
	if(wnd == glwfocus)
		(glwfocus->flags & GLW_POPUP || wnd->next && wnd->next->flags & GLW_COLLAPSE ? NULL : wnd->next)->focus();
	if(wnd == lastover)
		lastover = NULL;
	if(wnd == dragstart)
		dragstart = NULL;
	for(wnd2 = glwlist; wnd2; wnd2 = wnd2->next) if(wnd2->modal == wnd)
		wnd2->modal = NULL;
	for(ppwnd = &glwlist; *ppwnd;) if(*ppwnd == wnd)
		*ppwnd = wnd->next;
	else
		ppwnd = &(*ppwnd)->next;
/*	*ppwnd = wnd->next;*/
	delete wnd;
}

/// Sets or replaces window title string.
void GLwindow::setTitle(const char *atitle){
	title = atitle;
}

const char *GLwindow::classname()const{return "GLwindow";}
GLWrect GLwindow::clientRect()const{return GLWrect(xpos + margin, (title.len() || flags & (GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE)) ? ypos + margin + fontheight : ypos + margin, xpos + width - margin, ypos + height - margin);}
GLWrect GLwindow::extentRect()const{return GLWrect(xpos, ypos, xpos + width, ypos + height);}
GLWrect GLwindow::adjustRect(const GLWrect &r)const{return GLWrect(r.x0 - margin, (title.len() || flags & (GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE)) ? r.y0 - margin - fontheight : r.y0 - margin, r.x1 + margin, r.y1 + margin);}
/// Derived classes can override the default attribute.
bool GLwindow::focusable()const{return true;}
/// Draws client area.
/// \param t Global time
void GLwindow::draw(GLwindowState &,double){}

/** Mouse clicks and drags can be handled in this function.
 * \param key can be GLUT_LEFT_BUTTON or GLUT_RIGHT_BUTTON
 * \param state can be GLUT_DOWN, GLUT_UP, GLUT_KEEP_DOWN or GLUT_KEEP_UP.
 * The latter 2 is called when the mouse moves.
 * \param x Mouse position in client coordinates.
 * \param y Mouse position in client coordinates.
 * \return 0 if this window does not process the mouse event, otherwise consume the event.
 * \sa mouseFunc(), mouseDrag(), mouseEnter(), mouseNC() and mouseLeave()
 */
int GLwindow::mouse(GLwindowState&,int,int,int,int){return 0;}

void GLwindow::mouseEnter(GLwindowState&){} ///< Derived classes can override to define mouse responses.
void GLwindow::mouseLeave(GLwindowState&){} ///< Derived classes can override to define mouse responses.

/// Derived classes can override to define focus responses.
void GLwindow::focusEnter(){
	if(onFocusEnter)
		onFocusEnter(this);
}

/// Derived classes can override to define focus responses.
void GLwindow::focusLeave(){
	if(onFocusLeave)
		onFocusLeave(this);
}

bool GLwindow::mouseHit(GLwindowState &ws, int x, int y)const{
	if(this->flags & GLW_COLLAPSE){
		int minix = 2, miniy = ws.h - r_titlebar_height - 2;
		int wx, wy;
		int ww = this->title.len() ? this->title.len() * fontwidth + 2 : 80;
		if(ww < ws.w && ws.w < minix + ww)
			wx = minix = 2, wy = miniy -= r_titlebar_height + 2;
		else
			wx = minix, wy = miniy;
		int wh = r_titlebar_height;
		return GLWrect(wx, wy, wx + ww, wy + wh).include(x, y);
	}
	else
		return extentRect().include(x, y);
}

/// \param key Key code of inputs from the keyboard. Printable keys are passed as its ASCII code.
/// \return 0 if this window does not process the key event, otherwise consume the event.
int GLwindow::key(int key){return 0;}
/// \param key Virtual key code of inputs from the keyboard.
int GLwindow::specialKey(int){return 0;}
/// \param dt frametime of this frame.
void GLwindow::anim(double){}
void GLwindow::postframe(){}
GLwindow::~GLwindow(){}

GLwindow *GLwindow::lastover = NULL;
GLwindow *GLwindow::captor = NULL;
GLwindow *GLwindow::dragstart = NULL; ///< Technically, it can be aquired by drag start position, but there's a chance windows move over drag start position.

static int messagecount = 0;

/// Mouse event handler for non-client region of the window.
int GLwindow::mouseFuncNC(GLwindow **ppwnd, GLwindowState &ws, int button, int state, int x, int y, int minix, int miniy){
	GLwindow *wnd = *ppwnd;
	int wx, wy, ww, wh;
	int nowheel = !(button == GLUT_WHEEL_UP || button == GLUT_WHEEL_DOWN);
	int titleheight = fontheight + margin;
	int ret = 0, killfocus;

	// Update dragstart
	if(state == GLUT_DOWN && button == GLUT_LEFT_BUTTON)
		dragstart = wnd->extentRect().include(x, y) ? wnd : NULL;
	else if(state == GLUT_UP && button == GLUT_LEFT_BUTTON)
		dragstart = NULL;

	if(wnd->mouseNC(ws, button, state, x, y))
		return 1;
	else{
	if(state == GLUT_KEEP_DOWN || state == GLUT_KEEP_UP){
		bool caught = false;
		if(captor == wnd || wnd->extentRect().include(x, y)){
			if(lastover && lastover != wnd){
				GLWrect cr = lastover->clientRect();
				ws.mousex = ws.mx - cr.x0;
				ws.mousey = ws.my - cr.y0;
				lastover->mouseLeave(ws);
				wnd->mouseEnter(ws);
				messagecount++;
			}
			lastover = wnd;
			caught = true;
		}
		else if(lastover == wnd){ // Mouse leaves
			GLWrect cr = wnd->clientRect();
			ws.mousex = ws.mx - cr.x0;
			ws.mousey = ws.my - cr.y0;
			wnd->mouseLeave(ws);
			messagecount++;
			lastover = NULL;
			return 0;
/*				ppwnd = &(*ppwnd)->next;
			continue;*/
		}
		else{ // Mouse neither moves over or leaves the window.
			return 0;
/*				ppwnd = &(*ppwnd)->next;
			continue;*/
		}
		GLWrect cr = wnd->clientRect();
		ws.mousex = ws.mx - cr.x0;
		ws.mousey = ws.my - cr.y0;
		wnd->mouse(ws, button, state, ws.mousex, ws.mousey);
		messagecount++;
		if(caught)
			return 1;
//				break;
		else{
			return 0;
/*				ppwnd = &(*ppwnd)->next;
			continue;*/
		}
	}
	if(wnd->flags & GLW_COLLAPSE){
		ww = wnd->title.len() ? wnd->title.len() * fontwidth + 2 : 80;
		if(ww < ws.w && ws.w < minix + ww)
			wx = minix = 2, wy = miniy -= r_titlebar_height + 2;
		else
			wx = minix, wy = miniy;
		wh = r_titlebar_height;
		minix += ww + 2;
	}
	else
		wx = wnd->xpos, wy = wnd->ypos, ww = wnd->width, wh = wnd->height;
	if((nowheel || glwfocus == wnd) && wnd->mouseHit(ws, x, y)){
		const int iconsize = titleheight - margin;
		int sysx = wnd->width - titleheight - (wnd->flags & GLW_CLOSE ? iconsize : 0);
		int pinx = sysx - (wnd->flags & GLW_COLLAPSABLE ? iconsize : 0);

		/* Window with modal window will never receive mouse messages. */
		if(wnd->modal){
			if(nowheel){
				glwActivate(glwFindPP(wnd->modal));
//					ret = 1;
				killfocus = 0;
			}
			return 1;
//				break;
		}

		// Titlebar branch
		if((wnd->title.len() || wnd->flags & (GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE)) && wnd->ypos <= y && y <= wnd->ypos + titleheight){
			if(wnd->flags & GLW_COLLAPSE || wnd->flags & GLW_COLLAPSABLE && wnd->xpos + sysx <= x && x <= wnd->xpos + sysx + iconsize){
	//				ret = 1;
				killfocus = 0;

				// Cannot collapse when pinned, but consume mouse messages.
				if(wnd->flags & GLW_PINNED)
					return 1;
	//					break;

				if(nowheel && state == GLUT_UP){
					wnd->flags ^= GLW_COLLAPSE;
					if(wnd->flags & GLW_COLLAPSE){
						/* Insert at the end of list to make majority of taskbar do not change */
						*ppwnd = wnd->next;
						for(; *ppwnd; ppwnd = &(*ppwnd)->next);
						*ppwnd = wnd;
						wnd->next = NULL;
						/* Defocus also to avoid the window catching key strokes */
						if(wnd == glwfocus)
							glwfocus->defocus();
					}
					else
						glwActivate(ppwnd);
					return 1;
	//					break;
				}
				return 1;
	//				break;
			}
			else if(wnd->flags & GLW_CLOSE && wnd->xpos + wnd->width - titleheight <= x && x <= wnd->xpos + wnd->width){
				ret = 1;

				// Cannot close when pinned, but consume mouse messages.
				if(wnd->flags & GLW_PINNED)
					return 1;
	//					break;

				if(nowheel && state == GLUT_UP){
					wnd->glwFree();
					return 1;
	//					break;
				}
				killfocus = 0;
				return 1;
	//				break;
			}
			else if(wnd->flags & GLW_PINNABLE && wnd->xpos + pinx <= x && x <= wnd->xpos + pinx + iconsize){
				ret = 1;
				killfocus = 0;
				if(nowheel && state == GLUT_UP){
					wnd->flags ^= GLW_PINNED;
//					if(wnd->flags & GLW_PINNED && wnd == glwfocus)
//						glwfocus->defocus();
					return 1;
	//					break;
				}
				return 1;
	//				break;
			}
			else if(wnd->xpos <= x && x <= wnd->xpos + wnd->width - (titleheight * !!(wnd->flags & GLW_CLOSE))){
				if(!nowheel)
					return 1;
	//					break;
				killfocus = 0;
				ret = 1;

				// Cannot be dragged around when pinned, but consume mouse messages.
				if(wnd->flags & GLW_PINNED)
					return 1;
	//					break;

				glwfocus = wnd;
				glwActivate(ppwnd);

				if(button != GLUT_LEFT_BUTTON)
					return 1;
	//					break;
				glwdrag = wnd;
				glwdragpos[0] = x - wnd->xpos;
				glwdragpos[1] = y - wnd->ypos;
	//				break;
				return 1;
			}
		}
		else if(!(wnd->flags & GLW_COLLAPSE) && (captor == wnd || glwfocus == wnd)){
			GLWrect cr = wnd->clientRect();
			wnd->mouse(ws, button, state, x - cr.x0, y - cr.y0);
		}
		if(wnd->flags & GLW_TODELETE){
			wnd->glwFree();
/*				if(wnd->destruct)
				wnd->destruct(wnd);
			if(wnd == glwfocus)
				glwfocus = wnd->next;
			for(ppwnd = &glwlist; *ppwnd; ppwnd = &(*ppwnd)->next) if(*ppwnd == wnd)
				*ppwnd = wnd->next;
			free(wnd);*/
		}
		else if(nowheel){
			// Send mouse message too if the new window is just focused.
			if(glwfocus != wnd){
				GLWrect cr = wnd->clientRect();
				wnd->mouse(ws, button, state, x - cr.x0, y - cr.y0);
			}

			if(/*!(wnd->flags & GLW_PINNED) &&*/ wnd->focusable() && glwfocus != wnd){
				wnd->focus();
				glwActivate(ppwnd);
			}
			killfocus = 0;
		}
		ret = 1;
		return 1;
//			break;
	}
	else if(wnd->flags & GLW_POPUP){
		wnd->glwFree();
		return 0;
//			continue;
	}
	}
	return 0;
}


/// Handles titlebar button events or otherwise call mouse() virtual function.
int GLwindow::mouseFunc(int button, int state, int x, int y, GLwindowState &ws){
	GLwindow **ppwnd, *wnd;
	int minix = 2, miniy = ws.h - r_titlebar_height - 2;
	int ret = 0;

	if(captor){
		GLwindow **ppwnd = glwFindPP(captor);
		if(ppwnd)
			return mouseFuncNC(ppwnd, ws, button, state, x, y, minix, miniy);
	}

	for(ppwnd = &glwlist; *ppwnd;){
		wnd = *ppwnd;
		ret = wnd->mouseFuncNC(ppwnd, ws, button, state, x, y, minix, miniy);
		if(ret)
			break;
		ppwnd = &(*ppwnd)->next;
	}
	return ret;
}

static int snapborder(int x0, int w0, int x1, int w1){
	int snapdist = 10;
	if(x1 - snapdist < x0 && x0 < x1 + snapdist)
		return x1;
	if(x1 - snapdist < x0 + w0 && x0 + w0 < x1 + snapdist)
		return x1 - w0 - 2;
	if(x1 + w1 - snapdist < x0 + w0 && x0 + w0 < x1 + w1 + snapdist)
		return x1 + w1 - w0;
	if(x1 + w1 - snapdist < x0 && x0 < x1 + w1 + snapdist)
		return x1 + w1 + 2;
	return x0;
}

/// Drag a window's title to move it around.
void GLwindow::mouseDrag(int x, int y){
	this->xpos = x;
	this->ypos = y;
	const int snapdist = 10;
	viewport gvp;
	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);
	gvp.set(vp);
	for(GLwindow *glw = glwlist; glw; glw = glw->next) if(glw != glwdrag && glw->xpos - snapdist < glwdrag->xpos + glwdrag->width && glwdrag->xpos - snapdist < glw->xpos + glw->width
		&& glw->ypos - snapdist < glwdrag->ypos + glwdrag->height && glwdrag->ypos - snapdist < glw->ypos + glw->height){
		glwdrag->xpos = snapborder(glwdrag->xpos, glwdrag->width, glw->xpos, glw->width);
		glwdrag->ypos = snapborder(glwdrag->ypos, glwdrag->height, glw->ypos, glw->height);
	}
	if(-snapdist < glwdrag->xpos && glwdrag->xpos < snapdist)
		glwdrag->xpos = 0;
	if(gvp.w - snapdist < glwdrag->xpos + glwdrag->width && glwdrag->xpos + glwdrag->width < gvp.w + snapdist)
		glwdrag->xpos = gvp.w - glwdrag->width;
	if(-snapdist < glwdrag->ypos && glwdrag->ypos < snapdist)
		glwdrag->ypos = 0;
	if(gvp.h - snapdist < glwdrag->ypos + glwdrag->height && glwdrag->ypos + glwdrag->height < gvp.h + snapdist)
		glwdrag->ypos = gvp.h - glwdrag->height;
	glwdrag->changeExtent();
}

/// Called at very end of a frame, to free objects marked as to be deleted.
void GLwindow::glwEndFrame(){
	GLwindow *ret;
	for(ret = glwlist; ret;) if(ret->flags & GLW_TODELETE){

		// Quit dragging destroyed window.
		if(glwdrag == ret)
			glwdrag = NULL;

		GLwindow *next = ret->next;
		ret->glwFree();
		ret = next;
	}
	else
		ret = ret->next;
}

void GLwindow::sq_pushobj(HSQUIRRELVM v, GLwindow *p){
	sq_pushroottable(v);
	sq_pushstring(v, _SC("GLwindow"), -1);
	if(SQ_FAILED(sq_get(v, -2)))
		throw SQFError("Something's wrong with GLwindow class definition.");
	if(SQ_FAILED(sq_createinstance(v, -1)))
		throw SQFError("Couldn't create class GLwindow");
	sq_assignobj(v, p);
	sq_remove(v, -2); // Remove Class
	sq_remove(v, -2); // Remove root table
}

GLwindow *GLwindow::sq_refobj(HSQUIRRELVM v, SQInteger idx){
	SQUserPointer up;

	// If the instance does not have a user pointer, it's a serious exception that might need some codes fixed.
	if(SQ_FAILED(sq_getinstanceup(v, idx, &up, NULL)) || !up)
		throw SQFError("Something's wrong with Squirrel Class Instace of GLwindow.");

	// It's crucial to static_cast in this order, not just reinterpret_cast.
	// It'll make difference when multiple inheritance is involved.
	// Either way, virtual inheritances are prohibited, or use dynamic_cast.
	return static_cast<GLwindow*>(static_cast<GLelement*>(*(WeakPtr<GLelement>*)up));
}

const SQUserPointer GLwindow::tt_GLwindow = SQUserPointer("GLwindow");

static Initializer init_GLwindow("GLwindow", GLwindow::sq_define);

bool GLwindow::sq_define(HSQUIRRELVM v){
	StackReserver sr(v);
	sq_pushstring(v, _SC("GLwindow"), -1);
	if(SQ_SUCCEEDED(sq_get(v, -2)))
		return false;
	GLelement::sq_define(v);
	sq_pushstring(v, _SC("GLwindow"), -1);
	sq_pushstring(v, _SC("GLelement"), -1);
	if(SQ_FAILED(sq_get(v, -3)))
		throw SQFError(_SC("GLelement is not defined"));
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, tt_GLwindow);
	sq_setclassudsize(v, -1, sizeof(WeakPtr<GLelement>));
	register_closure(v, _SC("close"), sqf_close);
	sq_createslot(v, -3);
	return true;
}

SQInteger GLwindow::sqGet(HSQUIRRELVM v, const SQChar *wcs)const{
	if(!strcmp(wcs, _SC("next"))){
		if(!getNext())
			sq_pushnull(v);
		else{
			sq_pushroottable(v);
			sq_pushstring(v, _SC("GLwindow"), -1);
			sq_get(v, -2);
			sq_createinstance(v, -1);
			sq_pushobj(v, getNext());
		}
		return 1;
	}
	else if(!strcmp(wcs, _SC("closable"))){
		sq_pushbool(v, getClosable());
		return 1;
	}
	else if(!strcmp(wcs, _SC("pinned"))){
		sq_pushbool(v, getPinned());
		return 1;
	}
	else if(!strcmp(wcs, _SC("pinnable"))){
		sq_pushbool(v, getPinnable());
		return 1;
	}
	else if(!strcmp(wcs, _SC("collapsed"))){
		sq_pushbool(v, flags & GLW_COLLAPSE);
		return 1;
	}
	else if(!strcmp(wcs, _SC("visible"))){
		sq_pushbool(v, getVisible());
		return 1;
	}
	else if(!strcmp(wcs, _SC("title"))){
		sq_pushstring(v, getTitle(), -1);
		return 1;
	}
	else
		return GLelement::sqGet(v, wcs);
}

SQInteger GLwindow::sqSet(HSQUIRRELVM v, const SQChar *wcs){
	if(!strcmp(wcs, _SC("closable"))){
		SQBool b;
		if(SQ_FAILED(sq_getbool(v, 3, &b)))
			return SQ_ERROR;
		setClosable(!!b);
		return 0;
	}
	else if(!strcmp(wcs, _SC("pinned"))){
		SQBool b;
		if(SQ_FAILED(sq_getbool(v, 3, &b)))
			return SQ_ERROR;
		setPinned(!!b);
		return 0;
	}
	else if(!strcmp(wcs, _SC("pinnable"))){
		SQBool b;
		if(SQ_FAILED(sq_getbool(v, 3, &b)))
			return SQ_ERROR;
		setPinnable(!!b);
		return 0;
	}
	else if(!strcmp(wcs, _SC("collapsed"))){
		SQBool b;
		if(SQ_FAILED(sq_getbool(v, 3, &b)))
			return SQ_ERROR;
		if(b)
			flags |= GLW_COLLAPSE;
		else
			flags &= ~GLW_COLLAPSE;
		return 0;
	}
	else if(!strcmp(wcs, _SC("visible"))){
		SQBool b;
		if(SQ_FAILED(sq_getbool(v, 3, &b)))
			return SQ_ERROR;
		setVisible(!!b);
		return 0;
	}
	else if(!strcmp(wcs, _SC("title"))){
		const SQChar *s;
		if(SQ_FAILED(sq_getstring(v, 3, &s)))
			return SQ_ERROR;
		setTitle(s);
		return 0;
	}
	else
		return GLelement::sqSet(v, wcs);
}

SQInteger GLwindow::sqf_close(HSQUIRRELVM v){
	GLwindow *p = sq_refobj(v);
	p->postClose();
	return 0;
}

const SQChar *GLwindow::sqClassName()const{
	return _SC("GLwindow");
}


















void glwVScrollBarDraw(GLwindow *wnd, int x0, int y0, int w, int h, int range, int iy){
	int x1 = x0 + w, y1 = y0 + h, ypos = range <= 1 ? 0 : (h - 30) * iy / (range - 1);
	glColor4ub(191,191,47,255);
	glBegin(GL_QUADS);
	glVertex2i(x0, y0 + ypos + 10);
	glVertex2i(x1, y0 + ypos + 10);
	glVertex2i(x1, y0 + ypos + 20);
	glVertex2i(x0, y0 + ypos + 20);
	glEnd();
	glColor4ub(127,127,31,255);
	glBegin(GL_LINES);
	glVertex2i(x0, y0 + 10);
	glVertex2i(x1, y0 + 10);
	glVertex2i(x0, y1 - 10);
	glVertex2i(x1, y1 - 10);
	glEnd();

	/* Up arrow */
	glBegin(GL_TRIANGLES);
	glVertex2i(x0 + 8, y0 + 7);
	glVertex2i(x0 + 2, y0 + 7);
	glVertex2i(x0 + 5, y0 + 3);
	glEnd();

	/* Down arrow */
	glBegin(GL_TRIANGLES);
	glVertex2i(x0 + 2, y1 - 7);
	glVertex2i(x0 + 8, y1 - 7);
	glVertex2i(x0 + 5, y1 - 3);
	glEnd();

	glBegin(GL_LINE_LOOP);
	glVertex2i(x0, y0);
	glVertex2i(x1, y0);
	glVertex2i(x1, y1);
	glVertex2i(x0, y1);
	glEnd();
}

int glwVScrollBarMouse(GLwindow *wnd, int mousex, int mousey, int x0, int y0, int w, int h, int range, int iy){
	int x1 = x0 + w, y1 = y0 + h;
	if(mousex < x0 || x1 < mousex || mousey < y0 || y1 < mousey)
		return -1;
	if(mousey < y0 + 10)
		return 0 <= iy - 1 ? iy - 1 :  iy;
	if(y1 - 10 < mousey)
		return iy + 1 < range ? iy + 1 : iy;
	if(1 < range){
		int ret = (mousey - y0 - 10) * (range - 1) / (h - 30);
		if(ret < 0 || range <= ret)
			return -1;
		else
			return ret;
	}
	return iy;
}



const int GLwindowSizeable::GLWSIZEABLE_BORDER = 6;



GLwindowSizeable::GLwindowSizeable(Game *game, const char *title) : st(game, title){
	flags |= GLW_SIZEABLE;
	ratio = 1.;
	sizing = 0;
	minw = minh = 40;
	maxw = maxh = 1000;
}

int GLwindowSizeable::mouse(GLwindowState &, int, int, int, int){return 0;}

bool GLwindowSizeable::mouseNC(GLwindowState &, int button, int state, int x, int y){
	GLWrect r = clientRect();

	// Pinned window should not be able to change size.
	if(button == GLUT_LEFT_BUTTON && !getPinned()){
		int edgeflags = GLwindowSizeable::mouseCursorState(x, y);
		if(state == GLUT_DOWN){
			if(edgeflags){
				sizing = edgeflags;
				captor = this;
				return 1;
			}
		}
		else if(state == GLUT_UP){
			sizing = 0;
			captor = NULL;
			return !!edgeflags;
		}
	}

	if(button == GLUT_LEFT_BUTTON && state == GLUT_KEEP_DOWN && sizing){
		GLWrect wr = extentRect();
		if((sizing & 3) == 1 && minw <= width - (x - wr.x0) && width - (x - wr.x0) < maxw){
			wr.x0 = x;
			setExtent(wr);
		}
		else if((sizing & 3) == 2 && minw <= (x - wr.x0) && (x - wr.x0) < maxw){
			wr.x1 = x + GLWSIZEABLE_BORDER;
			setExtent(wr);
		}
		if(((sizing >> 2) & 3) == 1 && minh <= height - (y - wr.y0) && height - (y - wr.y0) < maxh){
			wr.y0 = y;
			setExtent(wr);
		}
		else if(((sizing >> 2) & 3) == 2 && minh <= (y - wr.y0) && (y - wr.y0) < maxh){
			wr.y1 = y + GLWSIZEABLE_BORDER;
			setExtent(wr);
		}
		return 1;
	}
	return 0;
}

int GLwindowSizeable::mouseCursorState(int x, int y)const{
	if(sizing)
		return sizing;
	if(getPinned())
		return 0;
	GLWrect r = extentRect();
	int edgeflags = 0;
	if(r.y0 <= y && y < r.y1)
		edgeflags |= r.x1 - GLWSIZEABLE_BORDER <= x && x < r.x1 ? 2 : r.x0 <= x && x < r.x0 + GLWSIZEABLE_BORDER ? 1 : 0;
	if(r.x0 <= x && x < r.x1)
		edgeflags |= (r.y1 - GLWSIZEABLE_BORDER <= y && y < r.y1 ? 2 : r.y0 <= y && y < r.y0 + GLWSIZEABLE_BORDER ? 1 : 0) << 2;
	return edgeflags;
}








/// Initially NULL. Created in glwInit() which is deferred after the Game object is created.
///
GLWtip *glwtip = NULL;





const char *GLWbutton::classname()const{return "GLWbutton";}
void GLWbutton::mouseEnter(GLwindowState &){}
void GLWbutton::mouseLeave(GLwindowState &){}
int GLWbutton::mouse(GLwindowState &ws, int button, int state, int mousex, int mousey){
	if(!extentRect().include(mousex, mousey)){
		depress = false;
		if(glwtip->parent == this){
			glwtip->tips = NULL;
			glwtip->parent = NULL;
			glwtip->setVisible(false);
		}
		return 0;
	}
	else if(state == GLUT_KEEP_UP && tipstring){
		int xs, ys;
		glwGetSizeStringML(tipstring, GLwindow::getFontHeight(), &xs, &ys);
		GLWrect localrect = GLWrect(xpos, ypos - ys - 3 * margin, xpos + xs + 3 * margin, ypos);
		GLWrect parentrect = parent->clientRect();
		localrect.rmove(parentrect.x0, parentrect.y0);

		// Adjust rect to fit in the screen. No care is taken if tips window is larger than the screen.
		if(localrect.x0 < 0)
			localrect.move(0, localrect.y0);
		else if(ws.w < localrect.x1)
			localrect.rmove(ws.w - localrect.x1, 0);
		if(localrect.y0 < 0)
			localrect.move(localrect.x0, 0);
		else if(ws.h < localrect.y1)
			localrect.rmove(0, ws.h - localrect.y1);

		glwtip->setExtent(localrect);
		glwtip->tips = tipstring;
		glwtip->parent = this;
		glwtip->setVisible(true);
		glwActivate(glwFindPP(glwtip));
//		glwActivate(GLwindow::findpp(&glwlist, &GLWpointerCompar(glwtip)));
	}
	if(state == GLUT_UP){
		if(depress){
			depress = false;
			press();
		}
	}
	else if(state == GLUT_DOWN){
		depress = true;
	}
	return 0;
}

GLWcommandButton::GLWcommandButton(Game *game, const char *filename, const char *command, const char *tips) :
	st(game),
	texname(filename)
{
	xpos = ypos = 0;
	width = height = 32;
	if(command){
		this->command = new const char[strlen(command) + 1];
		strcpy(const_cast<char*>(this->command), command);
	}
	else
		this->command = NULL;

	if(tips){
		this->tipstring = new const char[strlen(tips) + 1];
		strcpy(const_cast<char*>(this->tipstring), tips);
	}
	else
		this->tipstring = NULL;
}

GLWcommandButton::~GLWcommandButton(){
	delete command;
	delete tipstring;
}

void GLWcommandButton::draw(GLwindowState &ws, double){
	GLubyte mod = depress ? 127 : 255;
	const gltestp::TexCacheBind *tcb = gltestp::FindTexture(texname);
	GLuint texlist;
	if(tcb)
		texlist = tcb->getList();
	else{
		suftexparam_t stp;
		stp.flags = STP_MINFIL | STP_ALPHA;
		stp.minfil = GL_LINEAR;
		texlist = CallCacheBitmap(texname, texname, &stp, NULL);
	}
	if(!texlist)
		return;
	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT);
	glColor4ub(mod,mod,mod,255);
	glCallList(texlist);
	glBegin(GL_QUADS);
	glTexCoord2i(0,1); glVertex2i(xpos, ypos);
	glTexCoord2i(1,1); glVertex2i(xpos + width, ypos);
	glTexCoord2i(1,0); glVertex2i(xpos + width, ypos + height);
	glTexCoord2i(0,0); glVertex2i(xpos, ypos + height);
	glEnd();
	glPopAttrib();
}

void GLWcommandButton::press(){
	CmdExec(command);
}

void GLWcommandButton::mouseLeave(GLwindowState &ws){
	depress = false;
	if(glwtip->parent == this){
		glwtip->tips = NULL;
		glwtip->parent = NULL;
		glwtip->setExtent(GLWrect(-10,-10,-10,-10));
	}
}



GLWstateButton::GLWstateButton(Game *game, const char *filename, const char *filename1, const char *tips) :
	st(game),
	texname(filename), texname1(filename1)
{
	xpos = ypos = 0;
	width = height = 32;
/*	if(command){
		this->command = new const char[strlen(command) + 1];
		strcpy(const_cast<char*>(this->command), command);
	}
	else
		this->command = NULL;*/

	if(tips){
		this->tipstring = new const char[strlen(tips) + 1];
		strcpy(const_cast<char*>(this->tipstring), tips);
	}
	else
		this->tipstring = NULL;
}

GLWstateButton::~GLWstateButton(){
//	delete command;
	delete tipstring;
}

void GLWstateButton::mouseLeave(GLwindowState &ws){
	depress = false;
	if(glwtip->parent == this){
		glwtip->tips = NULL;
		glwtip->parent = NULL;
		glwtip->setExtent(GLWrect(-10,-10,-10,-10));
	}
}

/// Draws button image. Button image 0 is drawn when not
/// active.
void GLWstateButton::draw(GLwindowState &ws, double){
	bool s = state(); // Store the value to the local variable to prevent multiple evaluation

	// We could have empty texname1, in which case we use the darkened version of the texture
	// of the active state.  The image can be darkened when the button is depressed, too.
	GLubyte mod = depress || this->texname1 == "" && !s ? 127 : 255;
	gltestp::dstring texname = s || this->texname1 == "" ? this->texname : this->texname1;

	GLuint texlist;
	const gltestp::TexCacheBind *tcb = gltestp::FindTexture(texname);
	if(tcb)
		texlist = tcb->getList();
	else{
		suftexparam_t stp;
		stp.flags = STP_MINFIL | STP_ALPHA;
		stp.minfil = GL_LINEAR;
		texlist = CallCacheBitmap(texname, texname, &stp, NULL);
	}
	if(!texlist)
		return;
	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT);
	glColor4ub(mod,mod,mod,255);
	glCallList(texlist);
	glBegin(GL_QUADS);
	glTexCoord2i(0,1); glVertex2i(xpos, ypos);
	glTexCoord2i(1,1); glVertex2i(xpos + width, ypos);
	glTexCoord2i(1,0); glVertex2i(xpos + width, ypos + height);
	glTexCoord2i(0,0); glVertex2i(xpos, ypos + height);
	glEnd();
	glPopAttrib();
}




bool GLWtoggleCvarButton::state()const{
	cvar *cv = CvarFind(var);
	if(!cv || cv->type != cvar_int)
		return 0;
	else
		return *cv->v.i;
}

void GLWtoggleCvarButton::press(){
	cvar *cv = CvarFind(var);
	if(!cv || cv->type != cvar_int)
		;
	else
		*cv->v.i = !*cv->v.i;
}






GLWbuttonMatrix::GLWbuttonMatrix(Game *game, int x, int y, int xsize, int ysize) :
	st(game, "Button Matrix"),
	xbuttons(x), ybuttons(y),
	xbuttonsize(xsize), ybuttonsize(ysize),
	buttons(new (GLWbutton*[x * y]))
{
	flags |= GLW_COLLAPSABLE | GLW_CLOSE | GLW_PINNABLE;
	GLWrect r = adjustRect(GLWrect(0, 0, x * xsize, y * ysize));
	width = r.x1 - r.x0;
	height = r.y1 - r.y0;
	memset(buttons, 0, x * y * sizeof(GLWbutton*));
}

GLWbuttonMatrix::~GLWbuttonMatrix(){
	// Delete contained controls
	for(int y = 0; y < ybuttons; y++) for(int x = 0; x < xbuttons; x++){
		if(buttons[x + y * xbuttons])
			delete buttons[x + y * xbuttons];
	}
}


void GLWbuttonMatrix::draw(GLwindowState &ws, double dt){
	GLWrect r = clientRect();
	glPushMatrix();
	glTranslatef(r.x0, r.y0, 0);
	glColor4f(.5,.5,.5,1);
	glBegin(GL_LINES);
	for(int y = 1; y < ybuttons; y++){
		glVertex2i(0, y * ybuttonsize);
		glVertex2i(xbuttons * xbuttonsize, y * ybuttonsize);
	}
	for(int x = 1; x < xbuttons; x++){
		glVertex2i(x * xbuttonsize, 0);
		glVertex2i(x * xbuttonsize, ybuttons * ybuttonsize);
	}
	glEnd();
	for(int y = 0; y < ybuttons; y++) for(int x = 0; x < xbuttons; x++){
		if(buttons[y * xbuttons + x])
			buttons[y * xbuttons + x]->draw(ws, dt);
	}
	glPopMatrix();
}

bool GLWbuttonMatrix::focusable()const{
	return false;
}

int GLWbuttonMatrix::mouse(GLwindowState &ws, int button, int state, int mousex, int mousey){
	// You shouldn't be able to press or show hints for buttons in a collapsed window.
	if(flags & GLW_COLLAPSE)
		return 0;

	if(state == GLUT_KEEP_DOWN || state == GLUT_KEEP_UP){
		for(int i = 0; i < xbuttons * ybuttons; i++) if(buttons[i])
			buttons[i]->mouse(ws, button, state, mousex, mousey);
	}
	else{
		int x = (mousex) / xbuttonsize;
		int y = (mousey) / ybuttonsize;
		GLWrect cr = clientRect();
		if(0 <= x && x < xbuttons && 0 <= y && y < ybuttons){
			GLWbutton *p = buttons[y * xbuttons + x];
			if(p)
				return p->mouse(ws, button, state, mousex, mousey);
		}
	}
	return 1;
}

void GLWbuttonMatrix::mouseEnter(GLwindowState &ws){
	for(int i = 0; i < xbuttons * ybuttons; i++) if(buttons[i])
		buttons[i]->mouseEnter(ws);
}

void GLWbuttonMatrix::mouseLeave(GLwindowState &ws){
	for(int i = 0; i < xbuttons * ybuttons; i++) if(buttons[i])
		buttons[i]->mouseLeave(ws);
}

bool GLWbuttonMatrix::addButton(GLWbutton *b, int x, int y){
	int i;
	if(x < 0 || y < 0){
		for(i = 0; i < xbuttons * ybuttons; i++) if(!buttons[i])
			break;
		if(xbuttons * ybuttons <= i)
			return false;
	}
	else if(0 <= x && x < xbuttons && 0 <= y && y < ybuttons)
		i = x + y * xbuttons;
	else
		return false;
	b->parent = this;
	this->buttons[i] = b;
	b->setExtent(GLWrect(i % xbuttons * xbuttonsize, i / xbuttons * ybuttonsize, i % xbuttons * xbuttonsize + xbuttonsize, i / xbuttons * ybuttonsize + ybuttonsize));
	return true;
}

static SQInteger sqf_GLWbuttonMatrix_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	Game *game = (Game*)sq_getforeignptr(v);
	if(!game)
		return sq_throwerror(v, _SC("The game object is not assigned"));
	SQInteger x, y, sx, sy;
	if(argc <= 1 || SQ_FAILED(sq_getinteger(v, 2, &x)))
		x = 3;
	if(argc <= 2 || SQ_FAILED(sq_getinteger(v, 3, &y)))
		y = 3;
	if(argc <= 3 || SQ_FAILED(sq_getinteger(v, 4, &sx)))
		sx = 64;
	if(argc <= 4 || SQ_FAILED(sq_getinteger(v, 5, &sy)))
		sy = 64;
	GLWbuttonMatrix *p = new GLWbuttonMatrix(game, x, y, sx, sy);
	GLelement::sq_assignobj(v, p);
	glwAppend(p);
	return 0;
}

static SQInteger sqf_GLWbuttonMatrix_addButton(HSQUIRRELVM v){
	GLWbuttonMatrix *p = GLWbuttonMatrix::sq_refobj(v);
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

static SQInteger sqf_GLWbuttonMatrix_addToggleButton(HSQUIRRELVM v){
	GLWbuttonMatrix *p = GLWbuttonMatrix::sq_refobj(v);
	const SQChar *cvarname, *path, *path1, *tips;
	if(SQ_FAILED(sq_getstring(v, 2, &cvarname)))
		return SQ_ERROR;
/*	int value;
	cvar *cv = CvarFind(cvarname);
	if(!cv || cv->type != cvar_int)
		value = 0;
	else
		value = *cv->v.i;*/
	if(SQ_FAILED(sq_getstring(v, 3, &path)))
		return SQ_ERROR;
	if(SQ_FAILED(sq_getstring(v, 4, &path1)))
		return SQ_ERROR;
	if(SQ_FAILED(sq_getstring(v, 5, &tips)))
		tips = NULL;

	// Temporary!! the third param must be a reference to persistent object, which is not in this case!
	GLWtoggleCvarButton *b = new GLWtoggleCvarButton(p->getGame(), path, path1, cvarname, tips);
	if(!p->addButton(b)){
		delete b;
		return sq_throwerror(v, _SC("Could not add button"));
	}
	b->sq_pushobj(v, b);
	return 1;
}

static SQInteger sqf_GLWbuttonMatrix_addMoveOrderButton(HSQUIRRELVM v){
	GLWbuttonMatrix *p = GLWbuttonMatrix::sq_refobj(v);
	const SQChar *path, *path1, *tips;
	if(SQ_FAILED(sq_getstring(v, 2, &path)))
		return SQ_ERROR;
	if(SQ_FAILED(sq_getstring(v, 3, &path1)))
		return SQ_ERROR;
	if(SQ_FAILED(sq_getstring(v, 4, &tips)))
		tips = NULL;
	GLWstateButton *b = Player::newMoveOrderButton(p->getGame(), path, path1, tips);
	if(!p->addButton(b)){
		delete b;
		return sq_throwerror(v, _SC("Could not add button"));
	}
	b->sq_pushobj(v, b);
	return 1;
}

class GLWsqStateButton : public GLWstateButton{
public:
	HSQOBJECT hstatefunc;
	HSQOBJECT hpressfunc;
	GLWsqStateButton(Game *game, const char *filename, const char *filename1, const HSQOBJECT &hstatefunc, const HSQOBJECT &hpressfunc, const char *tip = NULL) :
		GLWstateButton(game, filename, filename1, tip), hstatefunc(hstatefunc), hpressfunc(hpressfunc){}
	virtual bool state()const;
	virtual void press();
	virtual const SQChar *sqClassName()const{return _SC("GLWsqStateButton");}
	static SQInteger sqf_constructor(HSQUIRRELVM v);
	static bool sq_define(HSQUIRRELVM v);
	~GLWsqStateButton(){
		HSQUIRRELVM v = game->sqvm;
		sq_release(v, &hstatefunc);
		sq_release(v, &hpressfunc);
	}
};

bool GLWsqStateButton::state()const{
	if(hstatefunc._type == OT_NULL)
		return false;
	HSQUIRRELVM v = game->sqvm;
	StackReserver sr(v);
	sq_pushobject(v, hstatefunc);
	sq_pushroottable(v);
	if(SQ_FAILED(sq_call(v, 1, SQTrue, SQTrue)))
		return false;
	SQBool b;
	if(SQ_FAILED(sq_getbool(v, -1, &b)))
		return false;
	return b;
}

void GLWsqStateButton::press(){
	if(hpressfunc._type == OT_NULL)
		return;
	HSQUIRRELVM v = game->sqvm;
	StackReserver sr(v);
	sq_pushobject(v, hpressfunc);
	sq_pushroottable(v);
	if(SQ_FAILED(sq_call(v, 1, SQTrue, SQTrue)))
		return;
}

SQInteger GLWsqStateButton::sqf_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	Game *game = (Game*)sq_getforeignptr(v);
	if(!game)
		return sq_throwerror(v, _SC("The game object is not assigned"));
	const SQChar *path, *path1, *tips, *statefunc, *pressfunc;
	if(SQ_FAILED(sq_getstring(v, 2, &path)))
		return SQ_ERROR;
	if(SQ_FAILED(sq_getstring(v, 3, &path1)))
		path1 = ""; // We allow null for the second argument.

	// Assign hstatefunc member variable
	HSQOBJECT hstatefunc;
	if(SQ_FAILED(sq_getstackobj(v, 4, &hstatefunc)))
		return SQ_ERROR;
	sq_addref(v, &hstatefunc);

	// Assign hpressfunc member variable
	HSQOBJECT hpressfunc;
	if(SQ_FAILED(sq_getstackobj(v, 5, &hpressfunc)))
		return SQ_ERROR;
	sq_addref(v, &hpressfunc);

	if(SQ_FAILED(sq_getstring(v, 6, &tips)))
		tips = NULL;
	GLWsqStateButton *b = new GLWsqStateButton(game, path, path1, hstatefunc, hpressfunc, tips);
	GLelement::sq_assignobj(v, b);
	return 1;
}

bool GLWsqStateButton::sq_define(HSQUIRRELVM v){
	// Define class GLWsqStateButton
	GLwindow::sq_define(v);
	sq_pushstring(v, _SC("GLWsqStateButton"), -1);
	sq_pushstring(v, _SC("GLelement"), -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, "GLWsqStateButton");
	sq_setclassudsize(v, -1, sizeof(WeakPtr<GLelement>));
	register_closure(v, _SC("constructor"), sqf_constructor);
	sq_createslot(v, -3);
	return true;
}

static sqa::Initializer GLWsqStateButton_init("GLWsqStateButton", GLWsqStateButton::sq_define);



GLWbuttonMatrix *GLWbuttonMatrix::sq_refobj(HSQUIRRELVM v, SQInteger idx){
	// It's crucial to static_cast in this order, not just reinterpret_cast.
	// It'll make difference when multiple inheritance is involved.
	// Either way, virtual inheritances are prohibited, or use dynamic_cast.
	return static_cast<GLWbuttonMatrix*>(GLwindow::sq_refobj(v, idx));
}



static bool GLWbuttonMatrix_sq_define(HSQUIRRELVM v){
	// Define class GLWbuttonMatrix
	GLwindow::sq_define(v);
	sq_pushstring(v, _SC("GLWbuttonMatrix"), -1);
	sq_pushstring(v, _SC("GLwindow"), -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, "GLWbuttonMatrix");
	sq_setclassudsize(v, -1, sizeof(WeakPtr<GLelement>));
	register_closure(v, _SC("constructor"), sqf_GLWbuttonMatrix_constructor);
	register_closure(v, _SC("addButton"), sqf_GLWbuttonMatrix_addButton);
	register_closure(v, _SC("addToggleButton"), sqf_GLWbuttonMatrix_addToggleButton);
	register_closure(v, _SC("addMoveOrderButton"), sqf_GLWbuttonMatrix_addMoveOrderButton);
	sq_createslot(v, -3);
	return true;
}

static sqa::Initializer init_GLWbuttonMatrix("GLWbuttonMatrix", GLWbuttonMatrix_sq_define);



class GLWprop : public GLwindowSizeable{
public:
	typedef GLwindowSizeable st;
	GLWprop(Game *game, const char *title, Entity *e = NULL) : st(game, title), a(e){
		xpos = 120;
		ypos = 40;
		width = 200;
		height = 200;
		flags |= GLW_COLLAPSABLE | GLW_CLOSE;
	}
	virtual void draw(GLwindowState &ws, double t);
protected:
	WeakPtr<Entity> a;
};

int Entity::cmd_property(int argc, char *argv[], void *pv){
	static int counter = 0;
	ClientApplication *pclient = (ClientApplication*)pv;
	if(!pclient || !pclient->clientGame)
		return 0;
	Player *ppl = pclient->clientGame->player;
	if(!ppl || ppl->selected.empty())
		return 0;
	glwAppend(new GLWprop(application.clientGame, cpplib::dstring("Entity Property ") << counter++, *ppl->selected.begin()));
	return 0;
}

static void register_cmd_property(){
	CmdAddParam("property", Entity::cmd_property, &application);
}

static StaticInitializer init_cmd_property(register_cmd_property);

void GLWprop::draw(GLwindowState &ws, double t){
	GLWrect cr = clientRect();
	if(!a){
		glColor4f(1,0.5f,0.5f,1);
		glwpos2d(cr.x0, cr.y0 + getFontHeight());
		glwprintf("Object not found");
		return;
	}
	Entity::Props pl = a->props();
	int i = 0;
	for(Entity::Props::iterator e = pl.begin(); e != pl.end(); e++){
		glColor4f(0,1,1,1);
		glwpos2d(cr.x0, cr.y0 + (1 + i++) * getFontHeight());
		glwprintf(*e);
	}
}






























#define GLDTW 512
#define GLDTH 512

/// Key for indexing GlyphCache entries.
struct GlyphCacheKey{
	wchar_t c;
	short size;
	GlyphCacheKey(wchar_t ac = 0, short asize = 0) : c(ac), size(asize){}
	bool operator==(const GlyphCacheKey &o)const{
		return c == o.c && size == o.size;
	}
	bool operator<(const GlyphCacheKey &o)const{
		return (c | (size << 16)) < (o.c | (o.size << 16));
	}
};

/// Entry of text glyph cache.
/// Should have OpenGL texture name if so many glyphs present.
struct GlyphCache{
	int x0, y0, x1, y1;
	GlyphCacheKey c; ///< For debugging purpose
};

HFONT newFont(int size){
	return CreateFont(size,
		size * 8 / 12,
		0,
		0,
		FW_DONTCARE,
		0,
		0,
		0,
		DEFAULT_CHARSET,
		OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY,
		/*DEFAULT_PITCH | FF_DONTCARE/*/FF_MODERN | FIXED_PITCH,
		NULL);
}


static HDC InitGlwHDC(int size){
	extern HWND hWndApp;
	static bool init = false;
	static HDC g_glwhdc = NULL;
	HDC &hdc = g_glwhdc;
	if(!init){
		init = true;
		HWND hw = hWndApp;
		HDC hwdc = GetDC(hw);
		hdc = CreateCompatibleDC(hwdc);
		ReleaseDC(hw,hwdc);
	}
	static int currentfontheight = 0;
	if(currentfontheight != size){
		currentfontheight = size;
		DeleteObject(SelectObject(hdc, newFont(size)));
	}
	return hdc;
}

int glwPutTextureStringN(const char *s, int n, int size){
	static int init = 0;
	static GLuint fonttex = 0;
	GLint oldtex;
	int i;
	static std::map<GlyphCacheKey, GlyphCache> listmap;
	static int sx = 0, sy = 0, maxheight = 0;
	static void *buf;
	static GLubyte backbuf[GLDTW][GLDTW] = {0};
	HDC hdc = InitGlwHDC(size);

	glGetIntegerv(GL_TEXTURE_2D, &oldtex);
	if(!init){
		init = 1;

		glGenTextures(1, &fonttex);
		glBindTexture(GL_TEXTURE_2D, fonttex);

		// Initialize with pitch black image
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, GLDTW, GLDTW, 0, GL_ALPHA, GL_UNSIGNED_BYTE, backbuf);
	}

	glBindTexture(GL_TEXTURE_2D, fonttex);

	size_t wsz = MultiByteToWideChar(CP_UTF8, 0, s, n, NULL, 0);
	wchar_t *wstr = new wchar_t[wsz];
	MultiByteToWideChar(CP_UTF8, 0, s, -1, wstr, wsz);
	for(i = 0; i < wsz;){
		int nc = 1;
		wchar_t wch;
		wch = wstr[i];
/*		wch = (unsigned char)s[i];
		while(nc < 3 && 0x80 <= (unsigned char)s[i + nc - 1] && (unsigned char)s[i + nc - 1] <= 0xfe){
			nc++;
			MultiByteToWideChar(CP_UTF8, 0, &s[i], nc, &wch, 1);
		}*/
		GlyphCacheKey gck(wch, size);
		if(listmap.find(gck) == listmap.end()){
			TEXTMETRIC tm;
			GetTextMetrics(hdc, &tm);
			GlyphCache gc;
			int wid;
			GetCharWidth32W(hdc, wch, wch, &wid);
			tm.tmHeight += 2;

			GLYPHMETRICS gm;
			MAT2 mat2 = {
				{0, 1}, {0, 0}, {0, 0}, {0, 1}
			};

			// DWORD is silly type name that is not portable, but size_t could be 64 bits in 64bit architecture,
			// which has too much width for our purpose.  The API returns 32bit values after all.
			const DWORD gbufsize = GetGlyphOutlineW(hdc, wch, GGO_GRAY8_BITMAP, &gm, 0, NULL, &mat2);
			void *gbuf = malloc(gbufsize * 2);
			const DWORD widthBytes = (gm.gmBlackBoxX + 3) / 4 * 4;

			// I have no clue why gbufsize not necessarily equal to widthBytes * gm.gmBlackBoxY.
			// Sometimes gbufsize > widthBytes * gm.gmBlackBoxY.  In that case, trust widthBytes (which was
			// deduced from gm.gmBlackBoxX) and divide gbufsize by it to obtain height.
			// It is necessary for some glyphs that have components at the bottom.
			const DWORD height = gbufsize / widthBytes;

			// Though if we know there's no content in padded bytes, glTexSubImage2D will fail unless those
			// padded bytes are in the array range of the image. Kinda silly workaround for NVIDIA introduced bug.
			if(GLDTW < sx + gm.gmptGlyphOrigin.x + widthBytes){
				sx = 0;
				sy += maxheight;
			}
			gc.c = gck;
			gc.x0 = sx;
			gc.x1 = gc.x0 + wid;
			sx = gc.x1;
			gc.y0 = sy + 1;
			gc.y1 = gc.y0 + tm.tmHeight;

			// The space character (' ') does not require memory to rasterize. In that case, we do not mess around glyph caching.
			if(gbufsize){
				GetGlyphOutlineW(hdc, wch, GGO_GRAY8_BITMAP, &gm, gbufsize, gbuf, &mat2);

				// Pixel stores do not work
//				glPixelStorei(GL_PACK_ALIGNMENT, 4);
//				glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

				for(int y = 0; y < height; y++) for(int x = 0; x < gm.gmBlackBoxX; x++)
					((GLubyte*)gbuf)[x + y * widthBytes] = ((GLubyte*)gbuf)[x + y * widthBytes] * 255 / 64;

				// Due to a bug in GeForce GTS250 OpenGL driver, we just cannot pass gm.gmBlackBoxX to 5th argument (width), but rather round it up to multiple of 4 bytes.
				// Excess pixels in the 4 byte boundary will overwritten, but they're always right side of the subimage, meaning no content have chance to be overwritten.
				// The bug is that lower end of the subimage is not correctly transferred but garbage pixels, which implies that count of required bytes transferred is incorrectly calculated.
				glTexSubImage2D(GL_TEXTURE_2D, 0, gc.x0 + gm.gmptGlyphOrigin.x, gc.y0 + (size - gm.gmptGlyphOrigin.y), widthBytes, height, GL_ALPHA, GL_UNSIGNED_BYTE, gbuf);

				// Another workaround is to replace the whole texture image with buffered image in client memory every time a character gets texturized, but it's terribly inefficient.
//				glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, GLDTW, GLDTW, 0, GL_ALPHA, GL_UNSIGNED_BYTE, backbuf);

				free(gbuf);

				if(maxheight < tm.tmHeight)
					maxheight = tm.tmHeight;
			}
			listmap[gck] = gc;
#if 0
			if(FILE *fp = fopen("cache/fontcache.bmp", "wb")){
				BITMAPFILEHEADER fh;
				BITMAPINFO *bmi = malloc(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD) + GLDTW * GLDTW);
				((char*)&fh.bfType)[0] = 'B';
				((char*)&fh.bfType)[1] = 'M';
				fh.bfSize = sizeof(BITMAPFILEHEADER) +  sizeof backbuf;
				fh.bfReserved1 = fh.bfReserved2 = 0;
				fh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);
				bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER), //DWORD      biSize;
				bmi->bmiHeader.biWidth = GLDTW, //LONG       biWidth;
				bmi->bmiHeader.biHeight = -GLDTW, //LONG       biHeight;
				bmi->bmiHeader.biPlanes = 1, // WORD       biPlanes;
				bmi->bmiHeader.biBitCount = 8, //WORD       biBitCount;
				bmi->bmiHeader.biCompression = BI_RGB, //DWORD      biCompression;
				bmi->bmiHeader.biSizeImage = 0, //DWORD      biSizeImage;
				bmi->bmiHeader.biXPelsPerMeter = 0, //LONG       biXPelsPerMeter;
				bmi->bmiHeader.biYPelsPerMeter = 0, //LONG       biYPelsPerMeter;
				bmi->bmiHeader.biClrUsed = 0, //DWORD      biClrUsed;
				bmi->bmiHeader.biClrImportant = 0; //DWORD      biClrImportant;
				for(i = 0; i < 256; i++)
					bmi->bmiColors[i].rgbRed = bmi->bmiColors[i].rgbGreen = bmi->bmiColors[i].rgbBlue = i;
				fwrite(&fh,sizeof(BITMAPFILEHEADER), 1, fp);
				fwrite(bmi,sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD), 1, fp);
				fwrite(backbuf, sizeof backbuf, 1, fp);
				fclose(fp);
			}
#endif
		}
		i += nc;
	}

//	glPushMatrix();
	glPushAttrib(GL_TEXTURE_BIT);
	glEnable(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR/*/GL_NEAREST*/);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR/*/GL_NEAREST*/);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
	{
		GLfloat envc[4] = {0,0,0,0};
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, envc);
	}
	glBegin(GL_QUADS);
	int sumx = 0;
	for(i = 0; i < wsz && s[i];){
		int nc = 1;
		wchar_t wch = wstr[i];
/*		wchar_t wch = (unsigned char)s[i];
		while(nc < 3 && 0x80 <= (unsigned char)s[i + nc - 1] && (unsigned char)s[i + nc - 1] <= 0xfe){
			nc++;
			MultiByteToWideChar(CP_UTF8, 0, &s[i], nc, &wch, 1);
		}*/
		GlyphCache gc = listmap[GlyphCacheKey(wch, size)];
		glTexCoord2d((double)gc.x0 / GLDTW, (double)gc.y0 / GLDTW); glVertex2i(sumx, -(gc.y1 - gc.y0));
		glTexCoord2d((double)gc.x1 / GLDTW, (double)gc.y0 / GLDTW); glVertex2i(sumx + gc.x1 - gc.x0, -(gc.y1 - gc.y0));
		glTexCoord2d((double)gc.x1 / GLDTW, (double)gc.y1 / GLDTW); glVertex2i(sumx + gc.x1 - gc.x0, 0);
		glTexCoord2d((double)gc.x0 / GLDTW, (double)gc.y1 / GLDTW); glVertex2i(sumx, 0);
		sumx += gc.x1 - gc.x0;
		i += nc;
	}
	glEnd();

	delete[] wstr;

	glPopAttrib();
//	glPopMatrix();
	glTranslatef(sumx, 0.f, 0.f);
	glBindTexture(GL_TEXTURE_2D, oldtex);
	return sumx;
}

int glwPutTextureString(const char *s, int size){
	return glwPutTextureStringN(s, strlen(s), size);
}

int glwGetSizeTextureStringN(const char *s, long n, int isize){
	HDC hdc = InitGlwHDC(isize);
	size_t wsz = MultiByteToWideChar(CP_UTF8, 0, s, n, NULL, 0);
	wchar_t *wstr = new wchar_t[wsz];
	MultiByteToWideChar(CP_UTF8, 0, s, n, wstr, wsz);
	SIZE size;
	GetTextExtentPoint32W(hdc, wstr, wsz, &size);
	delete[] wstr;
	return size.cx;
}

int glwGetSizeTextureString(const char *s, int size){
	return glwGetSizeTextureStringN(s, strlen(s), size == -1 ? GLwindow::getFontHeight() : size);
}



/** String font is a fair problem. Normally glBitmap is liked to print
  strings, but it's so slow, because bitmap is not cached in video memory.

   Texture fonts, on the other hand, could be very fast in hardware
  accelerated video rendering device, but they require some transformation
  prior to printing, whose load could defeat benefit of caching.

   Finally we leave the decision to the user by providing it as a cvar. */
int r_texture_font = 1;
static double s_raspo[4];

/// Sets the raster position for outputting string with glwprintf().
void glwpos2d(double x, double y){
	glRasterPos2d(x, y);
	s_raspo[0] = x;
	s_raspo[1] = y-2;
}

/// UI strings are urged to be printed by this function.
int glwprintf(const char *f, ...){
	static char buf[512]; /* it's not safe but unlikely to be reached */
	int ret;
	va_list ap;
	va_start(ap, f);

	/* Unluckily, snprintf is not part of the standard. */
#ifdef _WIN32
	ret = _vsnprintf(buf, sizeof buf, f, ap);
#else
	ret = vsprintf(buf, f, ap);
#endif

	if(r_texture_font){
/*		double raspo[4];
		glGetDoublev(GL_CURRENT_RASTER_POSITION, raspo);*/
		glPushMatrix();
		glTranslated(s_raspo[0], s_raspo[1] + 2, 0.);
		glScaled(GLwindow::glwfontscale, GLwindow::glwfontscale, 1.);
		s_raspo[0] += glwPutTextureString(buf);
		glPopMatrix();
	}
	else
		gldPutString(buf);
	va_end(ap);
	return ret;
}

/// Returns width of the formatted string drawn if it is passed to glwprintf().
int glwsizef(const char *f, ...){
	static char buf[512]; /* it's not safe but unlikely to be reached */
	int ret;
	va_list ap;
	if(!f)
		return 0;
	va_start(ap, f);

	/* Unluckily, snprintf is not part of the standard. */
#ifdef _WIN32
	ret = _vsnprintf(buf, sizeof buf, f, ap);
#else
	ret = vsprintf(buf, f, ap);
#endif

	if(r_texture_font)
		ret = glwGetSizeTextureString(buf);
	else
		ret = 10 * strlen(buf);
	va_end(ap);
	return ret;
}

/// Prints multi-line string, each line separated by \\n.
int glwPutStringML(const char *s, int size){
	while(const char *p = strchr(s, '\n')){
		int xs = glwPutTextureStringN(s, p - s, size);
		glTranslatef(-xs, size, 0.f);
		s = p + 1;
	}
	return glwPutTextureStringN(s, strlen(s), size);
}

/// Gets dimensions of multiline string.
/// \param retxsize Pointer to variable that stores returned size along x axis. This is widest value of
///                 all lines. Can be NULL to be ignored.
/// \param retysize Pointer to variable that stores returned size along y axis. This is effectively
///                 base height times number of '\\n' in the string, except the last empty line.
///                 Can be NULL to be ignored.
void glwGetSizeStringML(const char *s, int size, int *retxsize, int *retysize){
	int xs;
	int xsize = 0;
	int ysize = 0;
	while(const char *p = strchr(s, '\n')){
		xs = glwGetSizeTextureStringN(s, p - s, size);
		ysize += size;
		if(xsize < xs)
			xsize = xs;
		s = p + 1;
	}
	xs = glwGetSizeTextureStringN(s, strlen(s), size);
	if(xsize < xs)
		xsize = xs;

	// Trailing empty line is excluded from draw area.
	if(xs)
		ysize += size;

	if(retxsize) *retxsize = xsize;
	if(retysize) *retysize = ysize;
}













// is it sufficient?
static int vrc_fontscale(void *pv){
	double *v = (double*)pv;
	if(*v < 0.00001){
		*v = 0.00001;
		return 1;
	}
	else
		return 0;
}

int glwInit(){
	CvarAddVRC("g_font_scale", &GLwindow::glwfontscale, cvar_double, vrc_fontscale);
	glwtip = new GLWtip(application.clientGame);
	return 0;
}
