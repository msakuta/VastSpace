/** \file
 * \brief Implementation of GLwindow and its subclasses.
 * Implements GLWbutton branch too.
 */
#include "glwindow.h"
#include "cmd.h"
#include "antiglut.h"
#include "viewer.h"
#include "material.h"
#include "player.h" // GLWmoveOrderButton
extern "C"{
#include <clib/c.h>
#include <clib/GL/gldraw.h>
}
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define projection(e) glMatrixMode(GL_PROJECTION); e; glMatrixMode(GL_MODELVIEW);

/// There is only one font size. It could be bad for some future.
static double glwfontscale = 1.;
#define fontwidth (GLwindow::glwfontwidth * glwfontscale)
#define fontheight (GLwindow::glwfontheight * glwfontscale)
double GLwindow::getFontWidth(){return fontwidth;}
double GLwindow::getFontHeight(){return fontheight;}

/// Window margin
const long margin = 4;


GLwindow *glwlist = NULL; ///< Global list of all GLwindows. 
GLwindow *glwfocus = NULL; ///< Focused window.
GLwindow *glwdrag = NULL; ///< Window being dragged.
int glwdragpos[2] = {0}; ///< Memory for starting position of dragging.

/** \brief Sets title string
 * \param title pointer to string to initialize the window's title. Can be freed after construction.
 */
GLwindow::GLwindow(const char *atitle) : modal(NULL), next(NULL){
	if(atitle){
		title = new char[strlen(atitle)+1];
		strcpy(title, atitle);
	}
	else
		title = NULL;
	next = glwlist;
	glwlist = this;
	glwActivate(&glwlist);
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
		glwfocus = wnd;
		glwfocus->flags &= ~GLW_COLLAPSE;
	}
}

/// Called when a window is changed its size.
void GLwindow::reshapeFunc(int w, int h){
	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);
	GLwindow *wnd;
	for(wnd = glwlist; wnd; wnd = wnd->next){
		GLWrect r = wnd->extentRect();

		// If a window is aligned to or crossing with border, snap it
		if(vp[2] - vp[0] <= r.r || w <= r.r)
			r.rmove(w - (vp[2] - vp[0]), 0);
		if(vp[3] - vp[1] <= r.b || h <= r.b)
			r.rmove(0, h - (vp[3] - vp[1]));
		wnd->setExtent(r);
	}
}

int r_window_scissor = 1; ///< Whether enable scissor tests to draw client area.
static int s_minix;
//static const int r_titlebar_height = 12;
#define r_titlebar_height fontheight

/// Draw a GLwindow with its non-client frame.
void GLwindow::drawInt(GLwindowState &gvp, double t, int wx, int wy, int ww, int wh){
//	GLwindow *wnd = this;
	int s_mousex = gvp.mx, s_mousey = gvp.my;
	char buf[128];
	GLint vp[4];
	int w = gvp.w, h = gvp.h, m = gvp.m, mi, i, x;
	GLubyte alpha;
	int border;
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
	border = (flags & (GLW_POPUP | GLW_COLLAPSE) ? 2 : 3)/* + !!(wnd->flags & GLW_SIZEABLE)*/;
	alpha = flags & GLW_PINNED ? wx <= s_mousex && s_mousex < wx + ww && wy <= s_mousey && s_mousey < wy + wh ? 63 : 31 : 255;
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
		if(title || flags & (GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE))
			r.t -= fontheight + margin;
		glPushAttrib(GL_SCISSOR_BIT);
		glScissor(r.l - 1, gvp.h - (r.b + 1), r.r - r.l + 2, r.b - r.t + 2);
		glEnable(GL_SCISSOR_TEST);
	}
	if(!(flags & GLW_COLLAPSE)){
		gvp.mousex = gvp.mx - cr.l;
		gvp.mousey = gvp.my - cr.t;
		draw(gvp, t);
	}

	// Collapsed or pinned window cannot be sized.
	if((flags & (GLW_COLLAPSE | GLW_PINNED | GLW_SIZEABLE)) == GLW_SIZEABLE){
		glColor4ub(255, 255, 255 * (glwfocus == this), 255);
		glBegin(GL_LINE_LOOP);
		glVertex2d(cr.r - 2, cr.b - 10);
		glVertex2d(cr.r - 2, cr.b - 2);
		glVertex2d(cr.r - 10, cr.b - 2);
		glEnd();
	}

	// System command buttons. Do not draw if pinned and mouse pointer is afar.
	if(32 < alpha && !(flags & GLW_COLLAPSE) && (title || flags & (GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE))){
		glColor4ub(255,255, 255 * (glwfocus == this), alpha);
		glBegin(GL_LINES);
		glVertex2d(wx, wy + titleheight);
		glVertex2d(wx + ww, wy + titleheight);
		x = width - titleheight;
		if(flags & GLW_CLOSE){
			glVertex2i(xpos + width - titleheight, ypos + margin);
			glVertex2i(xpos + width - titleheight, ypos + titleheight);
			glVertex2i(xpos + width - margin - 2, ypos + margin + 2);
			glVertex2i(xpos + width - (titleheight - 2), ypos + titleheight - 2);
			glVertex2i(xpos + width - (titleheight - 2), ypos + margin + 2);
			glVertex2i(xpos + width - margin - 2, ypos + titleheight - 2);
			x -= titleheight;
		}
		if(flags & GLW_COLLAPSABLE){
			glVertex2i(xpos + x, ypos + margin);
			glVertex2i(xpos + x, ypos + titleheight);
			glVertex2i(xpos + x + 2, ypos + titleheight - 2);
			glVertex2i(xpos + x + titleheight - 2, ypos + titleheight - 2);
			if(flags & GLW_COLLAPSE){
				glVertex2i(xpos + x + titleheight - 2, ypos + titleheight - 2);
				glVertex2i(xpos + x + titleheight - 2, ypos + 2);
				glVertex2i(xpos + x + titleheight - 2, ypos + 2);
				glVertex2i(xpos + x + 2, ypos + 2);
				glVertex2i(xpos + x + 2, ypos + 2);
				glVertex2i(xpos + x + 2, ypos + titleheight - 2);
			}
			x -= titleheight;
		}
		if(flags & GLW_PINNABLE){
			glVertex2i(xpos + x, ypos + margin);
			glVertex2i(xpos + x, ypos + titleheight);
			glVertex2i(xpos + x + titleheight / 2, ypos + 2);
			glVertex2i(xpos + x + 2, ypos + titleheight / 2);
			glVertex2i(xpos + x + 2, ypos + titleheight / 2);
			glVertex2i(xpos + x + titleheight / 2, ypos + titleheight - 2);
			glVertex2i(xpos + x + titleheight / 2, ypos + titleheight - 2);
			glVertex2i(xpos + x + titleheight - 2, ypos + titleheight / 2);
			glVertex2i(xpos + x + titleheight - 2, ypos + titleheight / 2);
			glVertex2i(xpos + x + titleheight / 2, ypos + 2);
			if(flags & GLW_PINNED){
				glVertex2i(xpos + x + titleheight / 2 - 2, ypos + titleheight / 2 - 2);
				glVertex2i(xpos + x + titleheight / 2 + 2, ypos + titleheight / 2 + 2);
				glVertex2i(xpos + x + titleheight / 2 + 2, ypos + titleheight / 2 - 2);
				glVertex2i(xpos + x + titleheight / 2 - 2, ypos + titleheight / 2 + 2);
			}
			x -= titleheight;
		}
		glEnd();
	}
	else
		x = width;
	if(32 < alpha && title){
		glColor4ub(255,255,255,255);
		if(flags & GLW_COLLAPSE)
			glwpos2d(wx, wy + fontheight);
		else
			glwpos2d(wx + margin, wy + titleheight);
		glwprintf("%.*s", (int)(x / fontwidth), title);
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
		ww = wnd->title ? strlen(wnd->title) * fontwidth + 2 : 80;
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

void GLwindow::glwFree(){
	GLwindow *wnd = this;
	GLwindow **ppwnd, *wnd2;
	if(wnd == glwfocus)
		glwfocus = glwfocus->flags & GLW_POPUP || wnd->next && wnd->next->flags & GLW_COLLAPSE ? NULL : wnd->next;
	if(wnd == lastover)
		lastover = NULL;
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
	if(title)
		delete[] title;
	if(atitle){
		title = new char[strlen(atitle)+1];
		strcpy(title, atitle);
	}
	else
		title = NULL;
}

const char *GLwindow::classname()const{return "GLwindow";}
GLWrect GLwindow::clientRect()const{return GLWrect(xpos + margin, (title || flags & (GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE)) ? ypos + margin + fontheight : ypos + margin, xpos + width - margin, ypos + height - margin);}
GLWrect GLwindow::extentRect()const{return GLWrect(xpos, ypos, xpos + width, ypos + height);}
GLWrect GLwindow::adjustRect(const GLWrect &r)const{return GLWrect(r.l - margin, (title || flags & (GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE)) ? r.t - margin - fontheight : r.t - margin, r.r + margin, r.b + margin);}
/// Derived classes can override the default attribute.
bool GLwindow::focusable()const{return true;}
/// Draws client area.
/// \param t Global time
void GLwindow::draw(GLwindowState &,double){}

/** Mouse clicks and drags can be handled in this function.
 * \param key can be GLUT_LEFT_BUTTON or GLUT_RIGHT_BUTTON
 * \param state can be GLUT_DOWN, GLUT_UP, GLUT_KEEP_DOWN or GLUT_KEEP_UP.
 * The latter 2 is called when the mouse moves.
 * \param x Mouse position
 * \param y Mouse position
 * \return 0 if this window does not process the mouse event, otherwise consume the event.
 * \sa mouseFunc(), mouseDrag(), mouseEnter() and mouseLeave()
 */
int GLwindow::mouse(GLwindowState&,int,int,int,int){return 0;}

void GLwindow::mouseEnter(GLwindowState&){} ///< Derived classes can override to define mouse responses.
void GLwindow::mouseLeave(GLwindowState&){} ///< Derived classes can override to define mouse responses.
/// \param key Key code of inputs from the keyboard. Printable keys are passed as its ASCII code.
/// \return 0 if this window does not process the key event, otherwise consume the event.
int GLwindow::key(int key){return 0;}
/// \param key Virtual key code of inputs from the keyboard.
int GLwindow::specialKey(int){return 0;}
/// \param dt frametime of this frame.
void GLwindow::anim(double){}
void GLwindow::postframe(){}
GLwindow::~GLwindow(){
	if(title)
		delete[] title;
}

GLwindow *GLwindow::lastover = NULL;
GLwindow *GLwindow::captor = NULL;

static int messagecount = 0;

/// Mouse event handler for non-client region of the window.
int GLwindow::mouseFuncNC(GLwindow **ppwnd, GLwindowState &ws, int button, int state, int x, int y, int minix, int miniy){
	GLwindow *wnd = *ppwnd;
	int wx, wy, ww, wh;
	int nowheel = !(button == GLUT_WHEEL_UP || button == GLUT_WHEEL_DOWN);
	int titleheight = fontheight + margin;
	int ret = 0, killfocus;
	if(state == GLUT_KEEP_DOWN || state == GLUT_KEEP_UP){
		bool caught = false;
		if(captor == wnd || wnd->extentRect().include(x, y)){
			if(lastover && lastover != wnd){
				GLWrect cr = lastover->clientRect();
				ws.mousex = ws.mx - cr.l;
				ws.mousey = ws.my - cr.t;
				lastover->mouseLeave(ws);
				wnd->mouseEnter(ws);
				messagecount++;
			}
			lastover = wnd;
			caught = true;
		}
		else if(lastover == wnd){ // Mouse leaves
			GLWrect cr = wnd->clientRect();
			ws.mousex = ws.mx - cr.l;
			ws.mousey = ws.my - cr.t;
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
		ws.mousex = ws.mx - cr.l;
		ws.mousey = ws.my - cr.t;
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
		ww = wnd->title ? strlen(wnd->title) * fontwidth + 2 : 80;
		if(ww < ws.w && ws.w < minix + ww)
			wx = minix = 2, wy = miniy -= r_titlebar_height + 2;
		else
			wx = minix, wy = miniy;
		wh = r_titlebar_height;
		minix += ww + 2;
	}
	else
		wx = wnd->xpos, wy = wnd->ypos, ww = wnd->width, wh = wnd->height;
	if((nowheel || glwfocus == wnd) && wx <= x && x <= wx + ww && wy <= y && y <= wy + wh){
		int sysx = wnd->width - titleheight - (wnd->flags & GLW_CLOSE ? titleheight : 0);
		int pinx = sysx - (wnd->flags & GLW_COLLAPSABLE ? titleheight : 0);

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

		if(wnd->flags & GLW_COLLAPSE || wnd->flags & GLW_COLLAPSABLE && wnd->xpos + sysx <= x && x <= wnd->xpos + sysx + titleheight && wnd->ypos <= y && y <= wnd->ypos + titleheight){
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
						glwfocus = NULL;
				}
				else
					glwActivate(ppwnd);
				return 1;
//					break;
			}
			return 1;
//				break;
		}
		else if(wnd->flags & GLW_CLOSE && wnd->xpos + wnd->width - titleheight <= x && x <= wnd->xpos + wnd->width && wnd->ypos <= y && y <= wnd->ypos + titleheight){
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
		else if(wnd->flags & GLW_PINNABLE && wnd->xpos + pinx <= x && x <= wnd->xpos + pinx + titleheight && wnd->ypos <= y && y <= wnd->ypos + titleheight){
			ret = 1;
			killfocus = 0;
			if(nowheel && state == GLUT_UP){
				wnd->flags ^= GLW_PINNED;
				if(wnd->flags & GLW_PINNED && wnd == glwfocus)
					glwfocus = NULL;
				return 1;
//					break;
			}
			return 1;
//				break;
		}
		else if(wnd->xpos <= x && x <= wnd->xpos + wnd->width - (titleheight * !!(wnd->flags & GLW_CLOSE)) && wnd->ypos <= y && y <= wnd->ypos + titleheight){
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
		else if(!(wnd->flags & GLW_COLLAPSE) && (captor == wnd || glwfocus == wnd)){
			GLWrect cr = wnd->clientRect();
			wnd->mouse(ws, button, state, x - cr.l, y - cr.t);
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
				wnd->mouse(ws, button, state, x - cr.l, y - cr.t);
			}

			if(!(wnd->flags & GLW_PINNED) && glwfocus != wnd){
				glwfocus = wnd;
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



const int glwMenuAllAllocated[] = {1};
const char glwMenuSeparator[] = "-";

void GLwindowMenu::draw(GLwindowState &ws, double t){
	GLWrect r = clientRect();
	int mx = ws.mousex, my = ws.mousey;
	GLwindowMenu *p = this;
	GLwindow *wnd = this;
	int i, len, maxlen = 1;
	int ind = 0 <= my && 0 <= mx && mx <= width ? (my) / fontheight : -1;
	MenuItem *item = menus->get();
	for(i = 0; i < count; i++, item = item->next) if(item->isSeparator()){
		glBegin(GL_LINES);
		glVertex2d(r.l + fontwidth / 2, r.t + (0 + i) * fontheight + 6);
		glVertex2d(r.r - fontwidth / 2, r.t + (0 + i) * fontheight + 6);
		glEnd();
	}
	else{
		if(i == ind && item->cmd){
			glColor4ub(0,0,255,128);
			glBegin(GL_QUADS);
			glVertex2d(r.l + 1, r.t + (0 + i) * fontheight);
			glVertex2d(r.r    , r.t + (0 + i) * fontheight);
			glVertex2d(r.r    , r.t + (1 + i) * fontheight);
			glVertex2d(r.l + 1, r.t + (1 + i) * fontheight);
			glEnd();
		}
		glColor4ub(255,255,255,255);
		glwpos2d(r.l, r.t + (1 + i) * fontheight);
		glwprintf(item->title);
		len = item->title.len();
		if(maxlen < len)
			maxlen = len;
	}
//	wnd->w = maxlen * fontwidth + 2;
}

int GLwindowMenu::mouse(GLwindowState &, int button, int state, int x, int y){
	int ind = (y) / fontheight;
	if(button == GLUT_LEFT_BUTTON && state == GLUT_UP && 0 <= ind && ind < count){
		MenuItem *item = menus->get();
		for(; 0 < ind; ind--)
			item = item->next;
		if(!item->cmd)
			return 0;
		CmdExec(item->cmd);
		if((flags & (GLW_CLOSE | GLW_POPUP)) && !(flags & GLW_PINNED))
			flags |= GLW_TODELETE;
		return 1;
	}
	return 0;
}

int GLwindowMenu::key(int key){
	MenuItem *item = menus->get();
	for(; item; item = item->next) if(item->key == key){
		CmdExec(item->cmd);
		if(flags & GLW_CLOSE && !(flags & GLW_PINNED))
			flags |= GLW_TODELETE;
		return 1;
	}
	return 0;
}

GLwindowMenu::~GLwindowMenu(){
}

/// \brief Constructs a GLwindowMenu object of given items.
/// \param name Title string.
/// \param count Number of items.
/// \param menutitles Pointer to array of strings for displaying menu items. \param keys Pointer to array of keyboard shortcuts. \param cmd Pointer to array of strings for console commands that are executed when menu item is selected.
/// \param stickey ???
/// \return Constructed GLwindowMenu object.
/// \relates GLwindowMenu
GLwindowMenu *glwMenu(const char *name, int count, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky){
	return new GLwindowMenu(name, count, menutitles, keys, cmd, sticky);
}

/// \brief Constructs a GLwindowMenu object of given items.
/// \param name Title string.
/// \param list PopupMenu object that contains list of menu items.
/// \params flags ???
/// \relates GLwindowMenu
GLwindowMenu *glwMenu(const char *name, const PopupMenu &list, unsigned flags){
	return new GLwindowMenu(name, list, flags);
}

/// \brief Constructs a GLwindowMenu object of given items.
/// \par It's effectively same as the glwMenu function.
/// \param name Title string.
/// \param count Number of items.
/// \param menutitles Pointer to array of strings for displaying menu items. \param keys Pointer to array of keyboard shortcuts. \param cmd Pointer to array of strings for console commands that are executed when menu item is selected.
/// \param stickey ???
/// \return Constructed GLwindowMenu object.
GLwindowMenu::GLwindowMenu(const char *title, int acount, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky) : st(title), count(acount), menus(NULL){
	GLwindow *ret = this;
	int i, len, maxlen = 0;
//	menus = (glwindowmenuitem*)malloc(count * sizeof(*menus));
	xpos = !sticky * 50;
	ypos = 75;
	width = 150;
	height = (1 + count) * fontheight;
	title = NULL;
	flags = (sticky ? 0 : GLW_CLOSE | GLW_PINNABLE) | GLW_COLLAPSABLE;
	modal = NULL;

	/* To ensure no memory leaks, we need to allocate all strings though if it's obvious
	  that the given parameters are static const. The caller also must disallocate
	  the parameters if they are allocated memory.
	   This feels ridiculous, but no other way can clearly ensure memory safety. For
	  short-lived programs, memory leaks wouldn't be a big problem, but this work is
	  going to have sustained life. */
	menus = new PopupMenu;
	for(i = 0; i < count; i++){
		if(menutitles[i] == glwMenuSeparator){
			menus->appendSeparator();
		}
		else{
			menus->append(menutitles[i], keys ? keys[i] : '\0', cmd && cmd[i] ? cmd[i] : "");
			len = ::strlen(menutitles[i]);
			if(maxlen < len)
				maxlen = len;
		}
	}
	height = (1 + count) * fontheight;

/*	if(ret->w < maxlen * fontwidth + 2)
		ret->w = maxlen * fontwidth + 2;*/
}

GLwindowMenu::GLwindowMenu(const char *title, const PopupMenu &list, unsigned aflags) : st(title), count(list.count()), menus(new PopupMenu(list)){
	flags = aflags & (GLW_CLOSE | GLW_SIZEABLE | GLW_COLLAPSABLE | GLW_PINNABLE); // filter the bits
	width = 150;
	for(MenuItem *mi = menus->get(); mi; mi = mi->next) if(width < mi->title.len() * fontwidth)
		width = mi->title.len() * fontwidth;
	height = (1 + count) * fontheight;
}

/// \brief Adds an item to the menu.
/// Window size is expanded.
GLwindowMenu *GLwindowMenu::addItem(const char *title, int key, const char *cmd){
	menus->append(title, key, cmd);
	GLWrect cr = clientRect();
	cr.b = cr.t + ++count * fontheight;
	cr = adjustRect(cr);
	height = cr.b - cr.t;
	return this;
}

int GLwindowMenu::cmd_addcmdmenuitem(int argc, char *argv[], void *p){
	GLwindowMenu *wnd = (GLwindowMenu*)p;
	if(argc < 3){
		CmdPrint("addcmdmenuitem title command [key]");
		return 0;
	}
	if(!wnd){
		int key = argc < 4 ? 0 : argv[3][0];
		wnd = glwMenu("Command Menu", 0, NULL, NULL, NULL, 1);
		return 0;
	}
	wnd->addItem(argv[1], argc < 4 ? 0 : argv[3][0], argv[2]);
	return 0;
}

class GLwindowPopup : public GLwindowMenu{
public:
	typedef GLwindowMenu st;
	GLwindowPopup(const char *title, int count, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky, GLwindowState &gvp)
		: st(title, count, menutitles, keys, cmd, sticky){
		init(gvp);
	}
	GLwindowPopup(const char *title, GLwindowState &gvp, const PopupMenu &list)
		: st(title, list, 0){
		init(gvp);
	}
	void init(GLwindowState &gvp){
		flags &= ~(GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE);
		flags |= GLW_POPUP;
		POINT point;
		GetCursorPos(&point);
		ScreenToClient(WindowFromDC(wglGetCurrentDC()), &point);
		this->xpos = point.x + this->width < gvp.w ? point.x : gvp.w - this->width;
		this->ypos = point.y + this->height < gvp.h ? point.y : gvp.h - this->height;
	}
	~GLwindowPopup(){
		if(glwfocus == this)
			glwfocus = NULL;
	}
};

GLwindow *glwPopupMenu(GLwindowState &gvp, int count, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky){
	GLwindow *ret;
	GLwindowMenu *p;
	int i, len, maxlen = 0;
	ret = new GLwindowPopup(NULL, count, menutitles, keys, cmd, sticky, gvp);
	return ret;
}

GLwindowMenu *glwPopupMenu(GLwindowState &gvp, const PopupMenu &list){
	return new GLwindowPopup(NULL, gvp, list);
}








#define GLWSIZEABLE_BORDER 10



GLwindowSizeable::GLwindowSizeable(const char *title) : st(title){
	flags |= GLW_SIZEABLE;
	ratio = 1.;
	sizing = 0;
	minw = minh = 40;
	maxw = maxh = 1000;
}

int GLwindowSizeable::mouse(GLwindowState &, int button, int state, int x, int y){
	if(y < 12)
		return 0;
	GLWrect r = clientRect();
	x += r.l;
	y += r.t;

	// Pinned window should not be able to change size.
	if(button == GLUT_LEFT_BUTTON && !getPinned()){
		int edgeflags = 0;
		edgeflags |= r.r - GLWSIZEABLE_BORDER < x && x < r.r + GLWSIZEABLE_BORDER && r.t <= y && y < r.b;
		edgeflags |= (r.b - GLWSIZEABLE_BORDER < y && y < r.b + GLWSIZEABLE_BORDER && r.l <= x && x < r.r) << 1;
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
		if(sizing & 1 && minw <= x && x < maxw){
			GLWrect wr = extentRect();
			wr.r = x + GLWSIZEABLE_BORDER;
			setExtent(wr);
/*			this->width = x;
			if(this->flags & GLW_SIZEPROP)
				this->height = ratio * x;*/
		}
		if(sizing & 2 && minh <= y + 12 && y + 12 < maxh){
			GLWrect wr = extentRect();
			wr.b = y + GLWSIZEABLE_BORDER;
			setExtent(wr);
//			this->height = y + 12;
/*			if(this->flags & GLW_SIZEPROP)
				this->width = 1. / ratio * this->height;*/
		}
		return 1;
	}
	return 0;
}












class GLWtip : public GLwindow{
public:
	const char *tips;
	GLWbutton *parent;
	GLWtip() : st(), tips(NULL), parent(NULL){
		xpos = -100;
		ypos = -100;
		width = -100;
		height = -100;
	}
	virtual bool focusable()const{return false;}
	virtual void draw(GLwindowState &ws, double t){
		if(!tips)
			return;
		GLWrect r = clientRect();
		glwpos2d(r.l, r.t + fontheight);
		glwprintf(tips);
	}
};

static GLWtip *glwtip = new GLWtip();





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
		GLWrect localrect = GLWrect(xpos, ypos - fontheight - 3 * margin, xpos + fontwidth * strlen(tipstring) + 3 * margin, ypos);
		GLWrect parentrect = parent->clientRect();
		localrect.rmove(parentrect.l, parentrect.t);

		// Adjust rect to fit in the screen. No care is taken if tips window is larger than the screen.
		if(ws.w < localrect.r)
			localrect.rmove(ws.w - localrect.r, 0);
		if(ws.h < localrect.b)
			localrect.rmove(0, ws.h - localrect.b);

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

GLWcommandButton::GLWcommandButton(const char *filename, const char *command, const char *tips){
	xpos = ypos = 0;
	width = height = 32;
	texname = CallCacheBitmap(filename, filename, NULL, NULL);
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
	if(!texname)
		return;
	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT);
	glColor4ub(mod,mod,mod,255);
	glCallList(texname);
	glBegin(GL_QUADS);
	glTexCoord2i(0,0); glVertex2i(xpos, ypos);
	glTexCoord2i(1,0); glVertex2i(xpos + width, ypos);
	glTexCoord2i(1,1); glVertex2i(xpos + width, ypos + height);
	glTexCoord2i(0,1); glVertex2i(xpos, ypos + height);
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



GLWstateButton::GLWstateButton(const char *filename, const char *filename1, const char *tips){
	xpos = ypos = 0;
	width = height = 32;
	texname = CallCacheBitmap(filename, filename, NULL, NULL);
	texname1 = CallCacheBitmap(filename1, filename1, NULL, NULL);
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
	GLubyte mod = depress ? 127 : 255;
	GLuint texname = state() ? this->texname : this->texname1;
	if(!texname)
		return;
	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT);
	glColor4ub(mod,mod,mod,255);
	glCallList(texname);
	glBegin(GL_QUADS);
	glTexCoord2i(0,0); glVertex2i(xpos, ypos);
	glTexCoord2i(1,0); glVertex2i(xpos + width, ypos);
	glTexCoord2i(1,1); glVertex2i(xpos + width, ypos + height);
	glTexCoord2i(0,1); glVertex2i(xpos, ypos + height);
	glEnd();
	glPopAttrib();
}




bool GLWtoggleCvarButton::state()const{
	return var;
}

void GLWtoggleCvarButton::press(){
	var = !var;
}



bool GLWmoveOrderButton::state()const{
	return pl->moveorder;
}

void GLWmoveOrderButton::press(){
	pl->cmd_moveorder(0, NULL, pl);
}



GLWbuttonMatrix::GLWbuttonMatrix(int x, int y, int xsize, int ysize) : st("Button Matrix"), xbuttons(x), ybuttons(y), xbuttonsize(xsize), ybuttonsize(ysize), buttons(new (GLWbutton*[x * y])){
	flags |= GLW_COLLAPSABLE | GLW_CLOSE | GLW_PINNABLE;
	GLWrect r = adjustRect(GLWrect(0, 0, x * xsize, y * ysize));
	width = r.r - r.l;
	height = r.b - r.t;
	memset(buttons, 0, x * y * sizeof(GLWbutton*));
}


void GLWbuttonMatrix::draw(GLwindowState &ws, double dt){
	GLWrect r = clientRect();
	glPushMatrix();
	glTranslatef(r.l, r.t, 0);
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

int GLWbuttonMatrix::mouse(GLwindowState &ws, int button, int state, int mousex, int mousey){
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






















#define GLDTW 1024
#define GLDTH 1024

struct GlyphCache{
	int x0, y0, x1, y1;
	wchar_t c;
};

static int glwPutTextureStringN(const char *s, int n){
	static int init = 0;
	static GLuint fonttex = 0;
	GLint oldtex;
	int i;
	static std::map<wchar_t, GlyphCache> listmap;
	static int sx = 0, sy = 0;
	static HDC hdc;
	static void *buf;
	static BITMAPINFO *bmi;

	glGetIntegerv(GL_TEXTURE_2D, &oldtex);
	if(!init){
		int y, x;
		init = 1;

		HWND hw = GetDesktopWindow();
		HDC hwdc = GetDC(hw);
		hdc = CreateCompatibleDC(hwdc);
		ReleaseDC(hw,hwdc);
		HFONT hf = CreateFont(GLwindow::glwfontheight, fontwidth, 0, 0, FW_DONTCARE, 0, 0, 0, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, /*DEFAULT_PITCH | FF_DONTCARE/*/FF_MODERN | FIXED_PITCH, NULL);
		SelectObject(hdc, hf);

		bmi = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
		bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER), //DWORD      biSize;
		bmi->bmiHeader.biWidth = 1, //LONG       biWidth;
		bmi->bmiHeader.biHeight = 1, //LONG       biHeight;
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

		glGenTextures(1, &fonttex);
		glBindTexture(GL_TEXTURE_2D, fonttex);
/*		for(y = 0; y < GLDTW - GLDTH; y++) for(x = 0; x < GLDTW; x++){
			buf[y][x][0] = buf[y][x][1] = 127;
		}
		for(y = 0; y < GLDTH; y++) for(x = 0; x < GLDTW; x++){
			buf[y + GLDTW - GLDTH][x][0] = 255; buf[y + GLDTW - GLDTH][x][1] = (0x1 & (font8x10[y * 16 + x / 8] >> (7 - (x) % 8)) ? 0 : 255);
		}*/
		glTexImage2D(GL_TEXTURE_2D, 0, 2, GLDTW, GLDTW, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
	}

	glBindTexture(GL_TEXTURE_2D, fonttex);

	for(i = 0; i < n;){
		int nc = 1;
		wchar_t wch;
		wch = (unsigned char)s[i];
		while(nc < 3 && 0x80 <= (unsigned char)s[i + nc - 1] && (unsigned char)s[i + nc - 1] <= 0xfe){
			nc++;
			MultiByteToWideChar(CP_UTF8, 0, &s[i], nc, &wch, 1);
		}
		if(listmap.find(wch) == listmap.end()){
			TEXTMETRIC tm;
			GetTextMetrics(hdc, &tm);
			GlyphCache gc;
			int wid;
			GetCharWidth32W(hdc, wch, wch, &wid);
			if(GLDTW < sx + wid){
				sx = 0;
				sy += tm.tmHeight;
			}
			gc.c = wch;
			gc.x0 = sx;
			gc.x1 = gc.x0 + wid;
			sx = gc.x1;
			gc.y0 = 0;
			gc.y1 = gc.y0 + tm.tmHeight;

			bmi->bmiHeader.biWidth = gc.x1 - gc.x0; //LONG       biWidth;
			bmi->bmiHeader.biHeight = gc.y1 - gc.y0; //LONG       biHeight;
			bmi->bmiHeader.biSizeImage = 0;
			HBITMAP hbm = CreateDIBSection(hdc, (BITMAPINFO*)bmi, DIB_RGB_COLORS, &buf, NULL, 0);
			if(!hbm)
				printf("%d\n", GetLastError());
			HGDIOBJ holdbm = SelectObject(hdc, hbm);
			SetTextColor(hdc, RGB(255,255,255));
			SetBkColor(hdc, RGB(0,0,0));
			TextOutW(hdc, 0, 0, &wch, 1);
			GdiFlush();
			glTexSubImage2D(GL_TEXTURE_2D, 0, gc.x0, gc.y0, gc.x1 - gc.x0, gc.y1 - gc.y0, GL_ALPHA, GL_UNSIGNED_BYTE, buf);
			listmap[wch] = gc;
			SelectObject(hdc, holdbm);
			DeleteObject(hbm);
		}
		i += nc;
	}

	glPushMatrix();
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
	for(i = 0; i < n && s[i];){
		int nc = 1;
		wchar_t wch = (unsigned char)s[i];
		while(nc < 3 && 0x80 <= (unsigned char)s[i + nc - 1] && (unsigned char)s[i + nc - 1] <= 0xfe){
			nc++;
			MultiByteToWideChar(CP_UTF8, 0, &s[i], nc, &wch, 1);
		}
		GlyphCache gc = listmap[wch];
		glTexCoord2d((double)gc.x0 / GLDTW, (double)gc.y0 / GLDTW); glVertex2i(sumx, 0);
		glTexCoord2d((double)gc.x1 / GLDTW, (double)gc.y0 / GLDTW); glVertex2i(sumx + gc.x1 - gc.x0, 0);
		glTexCoord2d((double)gc.x1 / GLDTW, (double)gc.y1 / GLDTW); glVertex2i(sumx + gc.x1 - gc.x0, gc.y1 - gc.y0);
		glTexCoord2d((double)gc.x0 / GLDTW, (double)gc.y1 / GLDTW); glVertex2i(sumx, gc.y1 - gc.y0);
		sumx += gc.x1 - gc.x0;
/*		glTexCoord2d(0, 1); glVertex2i(i,0);
		glTexCoord2d(1, 1); glVertex2i(i+1,0);
		glTexCoord2d(1, 0); glVertex2i(i+1,1);
		glTexCoord2d(0, 0); glVertex2i(i,1);*/
		i += nc;
	}
	glEnd();
/*	glLoadIdentity();
	glBegin(GL_QUADS);
		glTexCoord2d(0, 0); glVertex2i(0,0);
		glTexCoord2d(1, 0); glVertex2i(+1,0);
		glTexCoord2d(1, 1); glVertex2i(+1,1);
		glTexCoord2d(0, 1); glVertex2i(0,1);
	glEnd();*/
	glPopAttrib();
	glPopMatrix();
	glTranslatef(n, 0.f, 0.f);
	glBindTexture(GL_TEXTURE_2D, oldtex);
	return sx;
}

int glwPutTextureString(const char *s){
	return glwPutTextureStringN(s, strlen(s));
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
		glScaled(glwfontscale, -glwfontscale, 1.);
		s_raspo[0] += glwPutTextureString(buf);
		glPopMatrix();
	}
	else
		gldPutString(buf);
	va_end(ap);
	return ret;
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
	CvarAddVRC("g_font_scale", &glwfontscale, cvar_double, vrc_fontscale);
	return 0;
}
