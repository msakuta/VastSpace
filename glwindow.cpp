#include "glwindow.h"
#include "cmd.h"
#include "antiglut.h"
#include "viewer.h"
extern "C"{
#include <clib/c.h>
#include <clib/GL/gldraw.h>
}
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define projection(e) glMatrixMode(GL_PROJECTION); e; glMatrixMode(GL_MODELVIEW);

// There is only one font size. It could be bad for some future.
static double glwfontscale = 1.;
#define glwfontwidth 8
#define glwfontheight 12
#define fontwidth (glwfontwidth * glwfontscale)
#define fontheight (glwfontheight * glwfontscale)

/*static glwindow glwsample = {
	100,100,100,100,"sample 1",NULL,0
}, glw0 = {
	150,180,200,100,"Window 0",&glwsample
};*/

glwindow *glwlist = NULL/*, *glwmini = NULL*/;
glwindow *glwfocus = NULL;
glwindow *glwdrag = NULL;
int glwdragpos[2] = {0};

GLwindow::GLwindow(const char *atitle){
	if(atitle){
		title = new char[strlen(atitle)+1];
		strcpy(title, atitle);
	}
	else
		title = NULL;
}

glwindow **glwAppend(glwindow *wnd){
	wnd->next = glwlist;
	glwlist = wnd;
	return &glwlist;
}

void glwActivate(glwindow **ppwnd){
	glwindow **last, *wnd = *ppwnd;
/*	for(last = &glwlist; *last; last = &(*last)->next);
	if(last != &(*ppwnd)->next){
		*last = *ppwnd;
		*ppwnd = wnd->next;
		wnd->next = NULL;
	}*/
	*ppwnd = wnd->next;
	wnd->next = glwlist;
	glwfocus = glwlist = wnd;
	glwfocus->flags &= ~GLW_COLLAPSE;
}

int r_window_scissor = 1;
static int s_minix;
//static const int r_titlebar_height = 12;
#define r_titlebar_height fontheight

void GLwindow::drawInt(GLwindowState &gvp, double t, int wx, int wy, int ww, int wh){
//	GLwindow *wnd = this;
	int s_mousex = gvp.mousex, s_mousey = gvp.mousey;
	char buf[128];
	GLint vp[4];
	int w = gvp.w, h = gvp.h, m = gvp.m, mi, i, x;
	GLubyte alpha;
	int border;
	double left, bottom;

	glGetIntegerv(GL_VIEWPORT, vp);
	mi = MIN(h, w);
	left = -(double)w / m;
	bottom = -(double)h / m;

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
		int ofs = i - 1, k = i ? 0 : 2;
		col = cols[(k) % 4];
		glColor4ub(col * !modal, col, col * (glwfocus == this), alpha);
		glBegin(GL_LINES);
		glVertex2d(wx - ofs, wy - ofs);
		glVertex2d(wx + ww + ofs, wy - ofs);
		col = cols[(k + 1) % 4];
		glColor4ub(col * !modal, col, col * (glwfocus == this), alpha);
		glVertex2d(wx + ww + ofs, wy - ofs);
		glVertex2d(wx + ww + ofs, wy + wh + ofs);
		col = cols[(k + 2) % 4];
		glColor4ub(col * !modal, col, col * (glwfocus == this), alpha);
		glVertex2d(wx + ww + ofs, wy + wh + ofs);
		glVertex2d(wx - ofs, wy + wh + ofs);
		col = cols[(k + 3) % 4];
		glColor4ub(col * !modal, col, col * (glwfocus == this), alpha);
		glVertex2d(wx - ofs, wy + wh + ofs), glVertex2d(wx - ofs, wy - ofs);
		glEnd();
	}
	if(!(flags & GLW_COLLAPSE) && r_window_scissor){
		glPushAttrib(GL_SCISSOR_BIT);
		glScissor(xpos, gvp.h - (ypos + height), width, height);
		glEnable(GL_SCISSOR_TEST);
	}
	if(!(flags & GLW_COLLAPSE)){
		gvp.mousex = gvp.mx - xpos;
		gvp.mousey = gvp.my - ypos - fontheight;
		draw(gvp, t);
	}
	if((flags & (GLW_COLLAPSE | GLW_SIZEABLE)) == GLW_SIZEABLE){
		glColor4ub(255, 255, 255 * (glwfocus == this), 255);
		glBegin(GL_LINE_LOOP);
		glVertex2d(xpos + width - 2, ypos + height - 6);
		glVertex2d(xpos + width - 2, ypos + height - 2);
		glVertex2d(xpos + width - 6, ypos + height - 2);
		glEnd();
	}
	if(!(flags & GLW_COLLAPSE) && (title || flags & (GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE))){
		glColor4ub(255,255, 255 * (glwfocus == this),255);
		glBegin(GL_LINES);
		glVertex2d(wx, wy + fontheight);
		glVertex2d(wx + ww, wy + fontheight);
		x = width - fontheight;
		if(flags & GLW_CLOSE){
			glVertex2i(xpos + width - fontheight, ypos);
			glVertex2i(xpos + width - fontheight, ypos + fontheight);
			glVertex2i(xpos + width - 2, ypos + 2);
			glVertex2i(xpos + width - (fontheight - 2), ypos + fontheight - 2);
			glVertex2i(xpos + width - (fontheight - 2), ypos + 2);
			glVertex2i(xpos + width - 2, ypos + fontheight - 2);
			x -= fontheight;
		}
		if(flags & GLW_COLLAPSABLE){
			glVertex2i(xpos + x, ypos);
			glVertex2i(xpos + x, ypos + fontheight);
			glVertex2i(xpos + x + 2, ypos + fontheight - 2);
			glVertex2i(xpos + x + fontheight - 2, ypos + fontheight - 2);
			if(flags & GLW_COLLAPSE){
				glVertex2i(xpos + x + fontheight - 2, ypos + fontheight - 2);
				glVertex2i(xpos + x + fontheight - 2, ypos + 2);
				glVertex2i(xpos + x + fontheight - 2, ypos + 2);
				glVertex2i(xpos + x + 2, ypos + 2);
				glVertex2i(xpos + x + 2, ypos + 2);
				glVertex2i(xpos + x + 2, ypos + fontheight - 2);
			}
			x -= fontheight;
		}
		if(flags & GLW_PINNABLE){
			glVertex2i(xpos + x, ypos);
			glVertex2i(xpos + x, ypos + fontheight);
			glVertex2i(xpos + x + fontheight / 2, ypos + 2);
			glVertex2i(xpos + x + 2, ypos + fontheight / 2);
			glVertex2i(xpos + x + 2, ypos + fontheight / 2);
			glVertex2i(xpos + x + fontheight / 2, ypos + fontheight - 2);
			glVertex2i(xpos + x + fontheight / 2, ypos + fontheight - 2);
			glVertex2i(xpos + x + fontheight - 2, ypos + fontheight / 2);
			glVertex2i(xpos + x + fontheight - 2, ypos + fontheight / 2);
			glVertex2i(xpos + x + fontheight / 2, ypos + 2);
			if(flags & GLW_PINNED){
				glVertex2i(xpos + x + fontheight / 2 - 2, ypos + fontheight / 2 - 2);
				glVertex2i(xpos + x + fontheight / 2 + 2, ypos + fontheight / 2 + 2);
				glVertex2i(xpos + x + fontheight / 2 + 2, ypos + fontheight / 2 - 2);
				glVertex2i(xpos + x + fontheight / 2 - 2, ypos + fontheight / 2 + 2);
			}
			x -= fontheight;
		}
		glEnd();
	}
	else
		x = width;
	if(title){
		glColor4ub(255,255,255,255);
		glwpos2d(wx, wy + fontheight);
		glwprintf("%.*s", (int)(x / fontwidth), title);
	}
	if(!(flags & GLW_COLLAPSE) && r_window_scissor){
		glPopAttrib();
	}
	projection(glPopMatrix());
}

/* using recursive call to do reverse rendering order, for reversing message process instead
  is more costly. */
void GLwindow::glwDraw(GLwindowState &vp, double t, int *minix){
	if(!this)
		return;

	next->glwDraw(vp, t, minix);

	if(!(flags & GLW_COLLAPSE))
		drawInt(vp, t, xpos, ypos, width, height);
}

/* Minimized windows never overlap, so drawing order can be arbitrary,
  making recursive call unnecessary. */
void GLwindow::glwDrawMinimized(GLwindowState &gvp, double t, int *pp){
	GLwindow *wndy = this;
	glwindow *wnd;
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

void GLwindow::glwAnim(double dt){
	for(GLwindow *wnd = glwlist; wnd; wnd = wnd->next)
		wnd->anim(dt);
}

glwindow **glwFindPP(glwindow *wnd){
	glwindow **ret;
	for(ret = &glwlist; *ret; ret = &(*ret)->next) if(*ret == wnd)
		return ret;
	return NULL;
}

void GLwindow::glwFree(){
	GLwindow *wnd = this;
	glwindow **ppwnd, *wnd2;
	if(wnd == glwfocus)
		glwfocus = glwfocus->flags & GLW_POPUP ? NULL : wnd->next;
	for(wnd2 = glwlist; wnd2; wnd2 = wnd2->next) if(wnd2->modal == wnd)
		wnd2->modal = NULL;
	for(ppwnd = &glwlist; *ppwnd; ppwnd = &(*ppwnd)->next) if(*ppwnd == wnd)
		*ppwnd = wnd->next;
/*	*ppwnd = wnd->next;*/
	delete wnd;
}

void GLwindow::draw(GLwindowState &,double){}
int GLwindow::mouse(GLwindowState&,int,int,int,int){return 0;}
int GLwindow::key(int key){return 0;}
void GLwindow::anim(double){}
GLwindow::~GLwindow(){
	if(title)
		delete[] title;
}

int GLwindow::mouseFunc(int button, int state, int x, int y, GLwindowState &gvp){
	int ret = 0, killfocus;
	glwindow **ppwnd, *wnd;
	int minix = 2, miniy = gvp.h - r_titlebar_height - 2;
	int nowheel = !(button == GLUT_WHEEL_UP || button == GLUT_WHEEL_DOWN);
	for(ppwnd = &glwlist; *ppwnd;){
		int wx, wy, ww, wh;
		wnd = *ppwnd;
		if(wnd->flags & GLW_COLLAPSE){
			ww = wnd->title ? strlen(wnd->title) * fontwidth + 2 : 80;
			if(ww < gvp.w && gvp.w < minix + ww)
				wx = minix = 2, wy = miniy -= r_titlebar_height + 2;
			else
				wx = minix, wy = miniy;
			wh = r_titlebar_height;
			minix += ww + 2;
		}
		else
			wx = wnd->xpos, wy = wnd->ypos, ww = wnd->width, wh = wnd->height;
		if((nowheel || glwfocus == wnd) && wx <= x && x <= wx + ww && wy <= y && y <= wy + wh){
			int sysx = wnd->width - fontheight - (wnd->flags & GLW_CLOSE ? fontheight : 0);
			int pinx = sysx - fontheight;

			/* Window with modal window will never receive mouse messages. */
			if(wnd->modal){
				if(nowheel){
					glwActivate(glwFindPP(wnd->modal));
					ret = 1;
					killfocus = 0;
				}
				break;
			}

			if(wnd->flags & GLW_COLLAPSE || wnd->flags & GLW_COLLAPSABLE && wnd->xpos + sysx <= x && x <= wnd->xpos + sysx + fontheight && wnd->ypos <= y && y <= wnd->ypos + fontheight){
				ret = 1;
				killfocus = 0;
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
					break;
				}
				break;
			}
			else if(wnd->flags & GLW_CLOSE && wnd->xpos + wnd->width - fontheight <= x && x <= wnd->xpos + wnd->width && wnd->ypos <= y && y <= wnd->ypos + fontheight){
				ret = 1;
				if(nowheel && state == GLUT_UP){
					wnd->glwFree();
					break;
				}
				killfocus = 0;
				break;
			}
			else if(wnd->flags & GLW_PINNABLE && wnd->xpos + pinx <= x && x <= wnd->xpos + pinx + fontheight && wnd->ypos <= y && y <= wnd->ypos + fontheight){
				ret = 1;
				killfocus = 0;
				if(nowheel && state == GLUT_UP){
					wnd->flags ^= GLW_PINNED;
					break;
				}
				break;
			}
			else if(wnd->xpos <= x && x <= wnd->xpos + wnd->width - (fontheight * !!(wnd->flags & GLW_CLOSE)) && wnd->ypos <= y && y <= wnd->ypos + fontheight){
				if(!nowheel)
					break;
				glwfocus = wnd;
				glwActivate(ppwnd);
				killfocus = 0;
				ret = 1;
				if(button != GLUT_LEFT_BUTTON)
					break;
				glwdrag = wnd;
				glwdragpos[0] = x - wnd->xpos;
				glwdragpos[1] = y - wnd->ypos;
				break;
			}
			else if(!(wnd->flags & GLW_COLLAPSE) && glwfocus == wnd)
				wnd->mouse(gvp, button, state, x - wnd->xpos, y - wnd->ypos - fontheight);
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
				glwfocus = wnd;
				glwActivate(ppwnd);
				killfocus = 0;
			}
			ret = 1;
			break;
		}
		else if(wnd->flags & GLW_POPUP){
			wnd->glwFree();
			continue;
		}
		ppwnd = &(*ppwnd)->next;
	}
	return ret;
}

static int snapborder(int x0, int w0, int x1, int w1){
	int snapdist = 10;
	if(x1 - snapdist < x0 && x0 < x1 + snapdist)
		return x1;
	if(x1 - snapdist < x0 + w0 && x0 + w0 < x1 + snapdist)
		return x1 - w0 - 4;
	if(x1 + w1 - snapdist < x0 + w0 && x0 + w0 < x1 + w1 + snapdist)
		return x1 + w1 - w0;
	if(x1 + w1 - snapdist < x0 && x0 < x1 + w1 + snapdist)
		return x1 + w1 + 4;
	return x0;
}

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





















void glwVScrollBarDraw(glwindow *wnd, int x0, int y0, int w, int h, int range, int iy){
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

int glwVScrollBarMouse(glwindow *wnd, int mousex, int mousey, int x0, int y0, int w, int h, int range, int iy){
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
	int mx = ws.mousex, my = ws.mousey;
	GLwindowMenu *p = this;
	GLwindow *wnd = this;
	int i, len, maxlen = 1;
	int ind = 0 <= my && 0 <= mx && mx <= width ? (my) / fontheight : -1;
	height = (count + 1) * fontheight;
	for(i = 0; i < count; i++) if(menus[i].title == glwMenuSeparator){
		glBegin(GL_LINES);
		glVertex2d(xpos + fontwidth / 2, ypos + (1 + i) * fontheight + 6);
		glVertex2d(xpos + width - fontwidth / 2, ypos + (1 + i) * fontheight + 6);
		glEnd();
	}
	else{
		if(i == ind && menus[ind].cmd){
			glColor4ub(0,0,255,128);
			glBegin(GL_QUADS);
			glVertex2d(xpos + 1, ypos + (1 + i) * fontheight);
			glVertex2d(xpos + width, ypos + (1 + i) * fontheight);
			glVertex2d(xpos + width, ypos + (2 + i) * fontheight);
			glVertex2d(xpos + 1, ypos + (2 + i) * fontheight);
			glEnd();
		}
		glColor4ub(255,255,255,255);
		glwpos2d(xpos, ypos + (2 + i) * fontheight);
		glwprintf(menus[i].title);
		len = strlen(menus[i].title);
		if(maxlen < len)
			maxlen = len;
	}
//	wnd->w = maxlen * fontwidth + 2;
}

int GLwindowMenu::mouse(GLwindowState &, int button, int state, int x, int y){
	int ind = (y) / fontheight;
	if(button == GLUT_LEFT_BUTTON && state == GLUT_UP && 0 <= ind && ind < count && menus[ind].cmd){
		CmdExec(menus[ind].cmd);
		if((flags & (GLW_CLOSE | GLW_POPUP)) && !(flags & GLW_PINNED))
			flags |= GLW_TODELETE;
		return 1;
	}
	return 0;
}

int GLwindowMenu::key(int key){
	int i;
	for(i = 0; i < count; i++) if(menus[i].key == key){
		CmdExec(menus[i].cmd);
		if(flags & GLW_CLOSE && !(flags & GLW_PINNED))
			flags |= GLW_TODELETE;
		return 1;
	}
	return 0;
}

GLwindowMenu::~GLwindowMenu(){
	int i;
	for(i = 0; i < count; i++) if(menus[i].title != glwMenuSeparator){
		if(menus[i].title)
			free((void*)menus[i].title);
		if(menus[i].cmd)
			free((void*)menus[i].cmd);
	}
	free((void*)menus);
}

GLwindowMenu *glwMenu(const char *name, int count, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky){
	return new GLwindowMenu(name, count, menutitles, keys, cmd, sticky);
}

GLwindowMenu::GLwindowMenu(const char *title, int acount, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky) : st(title), count(acount){
	glwindow *ret = this;
	int i, len, maxlen = 0;
	menus = (glwindowmenuitem*)malloc(count * sizeof(*menus));
	xpos = !sticky * 50;
	ypos = 75;
	width = 150;
	height = (1 + count) * fontheight;
	title = NULL;
	next = glwlist;
	glwlist = ret;
	glwActivate(&glwlist);
	flags = (sticky ? 0 : GLW_CLOSE | GLW_PINNABLE) | GLW_COLLAPSABLE;
	modal = NULL;

	/* To ensure no memory leaks, we need to allocate all strings though if it's obvious
	  that the given parameters are static const. The caller also must disallocate
	  the parameters if they are allocated memory.
	   This feels ridiculous, but no other way can clearly ensure memory safety. For
	  short-lived programs, memory leaks wouldn't be a big problem, but this work is
	  going to have sustained life. */
	for(i = 0; i < count; i++){
		if(menutitles[i] == glwMenuSeparator){
			menus[i].title = glwMenuSeparator;
			menus[i].key = 0;
			menus[i].cmd = NULL;
		}
		else{
			menus[i].title = (char*)malloc(strlen(menutitles[i]) + 1);
			strcpy((char*)menus[i].title, menutitles[i]);
			menus[i].key = keys ? keys[i] : 0;
			if(cmd && cmd[i]){
				menus[i].cmd = (char*)malloc(strlen(cmd[i]) + 1);
				strcpy((char*)menus[i].cmd, cmd[i]);
			}
			else
				menus[i].cmd = NULL;
			len = strlen(menutitles[i]);
			if(maxlen < len)
				maxlen = len;
		}
	}

/*	if(ret->w < maxlen * fontwidth + 2)
		ret->w = maxlen * fontwidth + 2;*/
}

GLwindowMenu *GLwindowMenu::addItem(const char *title, int key, const char *cmd){
	if(!this)
		return NULL;
/*	if((*ppwnd)->draw != glwmenudraw){
		printf("glwMenuAddItem: given window is not a menu window.\n");
		return NULL;
	}*/
	count++;
	menus = (glwindowmenuitem*)realloc(menus, count * sizeof(*menus));
	height = (count + 1) * fontheight;
	width = MAX(width, strlen(title) * fontwidth + 2);
	menus[count - 1].title = (char*)malloc(strlen(title) + 1);
	strcpy((char*)menus[count - 1].title, title);
	menus[count - 1].key = key;
	menus[count - 1].cmd = (char*)malloc(strlen(cmd) + 1);
	strcpy((char*)menus[count - 1].cmd, cmd);
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
		flags &= ~(GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE);
		flags |= GLW_POPUP;
		{
			POINT point;
			GetCursorPos(&point);
			ScreenToClient(WindowFromDC(wglGetCurrentDC()), &point);
			this->xpos = point.x + this->width < gvp.w ? point.x : gvp.w - this->width;
			this->ypos = point.y + this->height < gvp.h ? point.y : gvp.h - this->height;
		}
	}
	~GLwindowPopup(){
		if(glwfocus == this)
			glwfocus = NULL;
	}
};

#if 1
glwindow *glwPopupMenu(GLwindowState &gvp, int count, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky){
	glwindow *ret;
	GLwindowMenu *p;
	int i, len, maxlen = 0;
	ret = new GLwindowPopup(NULL, count, menutitles, keys, cmd, sticky, gvp);
	return ret;
}
#endif








#define GLWSIZEABLE_BORDER 6



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
	if(button == GLUT_LEFT_BUTTON){
		int edgeflags = 0;
		edgeflags |= this->width - GLWSIZEABLE_BORDER < x && x < this->width + GLWSIZEABLE_BORDER && 0 <= y && y < this->height;
		edgeflags |= (this->height - 12 - GLWSIZEABLE_BORDER < y && y < this->height - 12 + GLWSIZEABLE_BORDER && 0 <= x && x < this->width) << 1;
		if(state == GLUT_DOWN){
			if(edgeflags){
				sizing = edgeflags;
				return 1;
			}
		}
		else if(state == GLUT_UP){
			sizing = 0;
			return !!edgeflags;
		}
	}
	if(button == GLUT_LEFT_BUTTON && state == GLUT_KEEP_DOWN && sizing){
		if(sizing & 1 && minw <= x && x < maxw){
			this->width = x;
			if(this->flags & GLW_SIZEPROP)
				this->height = ratio * x;
		}
		if(sizing & 2 && minh <= y + 12 && y + 12 < maxh){
			this->height = y + 12;
			if(this->flags & GLW_SIZEPROP)
				this->width = 1. / ratio * this->height;
		}
		return 1;
	}
	return 0;
}





/* String font is a fair problem. Normally glBitmap is liked to print
  strings, but it's so slow, because bitmap is not cached in video memory.
   Texture fonts, on the other hand, could be very fast in hardware
  accelerated video rendering device, but they require some transformation
  prior to printing, whose load could defeat benefit of caching.
   Finally we leave the decision to the user by providing it as a cvar. */
int r_texture_font = 1;
static double s_raspo[4];

void glwpos2d(double x, double y){
	glRasterPos2d(x, y);
	s_raspo[0] = x;
	s_raspo[1] = y-2;
}

/* UI strings are urged to be printed by this function. */
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
		glTranslated(s_raspo[0], s_raspo[1], 0.);
		glScaled(8. * glwfontscale, -8. * glwfontscale, 1.);
		gldPutTextureString(buf);
		s_raspo[0] += strlen(buf) * 8 * glwfontscale;
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
