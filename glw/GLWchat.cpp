/** \file
 * \brief Implementation of GLWmessage.
 */
#include "cmd.h"
#include "../Client.h"
#include "sqadapt.h"
#include "glw/GLWchat.h"
#include "antiglut.h"
extern "C"{
#include <clib/c.h>
#include <clib/GL/gldraw.h>
}
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


#define fontwidth (GLwindow::glwfontwidth * glwfontscale)
#define fontheight (GLwindow::glwfontheight * glwfontscale)

namespace sqa{
typedef std::map<dstring, bool (*)(HSQUIRRELVM)> SQDefineMap;

extern SQDefineMap &defineMap();
}

struct GLWchatDefiner{
	GLWchatDefiner(){
		sqa::defineMap()["GLWchat"] = GLWchat::sq_define;
	}
};

static GLWchatDefiner definer;

std::set<GLWchat*> GLWchat::wndlist;

extern HWND hWndApp;

static LRESULT WINAPI CALLBACK EditWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT (WINAPI CALLBACK *DefEditWndProc)(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/** \brief Constructs a chat window.
 */
GLWchat::GLWchat() : GLwindowSizeable("Chat Window"), scrollpos(0){
	wndlist.insert(this);
	hEdit = CreateWindow("Edit", "", WS_CHILD | WS_VISIBLE, 100, 100, 300, int(fontheight), hWndApp, NULL, GetModuleHandle(NULL), NULL);
	DefEditWndProc = (LRESULT (WINAPI CALLBACK *)(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam))GetWindowLongPtr(hEdit, GWLP_WNDPROC);
	SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)EditWndProc);
}

GLWchat::~GLWchat(){
	wndlist.erase(this);
	DestroyWindow(hEdit);
}

inline int GLWchat::scrollrange(){
	return int((buf.size() + 1) * fontheight - clientRect().height());
}

void GLWchat::changeExtent(){
	GLWrect r = clientRect();
	MoveWindow(hEdit, r.x0, r.y1 - int(fontheight), r.width(), int(fontheight), TRUE);
	GLwindowSizeable::changeExtent();
}

void GLWchat::draw(GLwindowState &ws, double){
	GLWrect r = clientRect();

	glPushAttrib(GL_SCISSOR_BIT);
	glScissor(r.x0, ws.h - r.y1 + GLsizei(fontheight), r.width() - 10, r.height() - GLsizei(fontheight));
	glEnable(GL_SCISSOR_TEST);
	glColor4f(1,1,1,1);
	for(int i = 0; i < buf.size(); i++){
		glwpos2d(r.x0, r.y0 + (i + 1) * fontheight - scrollpos);
		glwprintf(buf[i]);
	}
	glPopAttrib();

	if(0 < scrollrange()){
		if(scrollrange() < scrollpos)
			scrollpos = scrollrange();
		glwVScrollBarDraw(this, r.x1 - 10, r.y0, 10, r.height() - int(fontheight), scrollrange(), scrollpos);
	}
	else
		scrollpos = 0;

	glColor4f(1,1,0,1);
	glBegin(GL_LINE_LOOP);
	glVertex2d(r.x0, r.y1 - fontheight);
	glVertex2d(r.x1, r.y1 - fontheight);
	glVertex2d(r.x1, r.y1);
	glVertex2d(r.x0, r.y1);
	glEnd();
}

void GLWchat::anim(double dt){
}

int GLWchat::mouse(GLwindowState &ws, int button, int state, int mx, int my){
	GLWrect r = clientRect();

	if(r.width() - 10 < mx && button == GLUT_LEFT_BUTTON && (state == GLUT_DOWN || state == GLUT_KEEP_DOWN || state == GLUT_UP) && 0 < scrollrange()){
		int iy = glwVScrollBarMouse(this, mx, my, r.width() - 10, 0, 10, r.height() - int(fontheight), scrollrange(), scrollpos);
		if(0 <= iy)
			scrollpos = iy;
		return 1;
	}
	return 0;
}

void GLWchat::append(dstring str){
	buf.push_back(str);

	// If a new string is added, force scrolling down to the bottom to notice the player that there's a new message.
	if(0 < scrollrange())
		scrollpos = scrollrange();
}

/// Define class GLWmessage for Squirrel
bool GLWchat::sq_define(HSQUIRRELVM v){
	sq_pushstring(v, _SC("GLWchat"), -1);
	sq_pushstring(v, _SC("GLwindow"), -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	register_closure(v, _SC("constructor"), sqf_constructor);
	sq_createslot(v, -3);
	return true;
}

/// Squirrel constructor
SQInteger GLWchat::sqf_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);

	GLWchat *p = new GLWchat();

	if(!sqa_newobj(v, p, 1))
		return SQ_ERROR;
	glwAppend(p);
	return 0;
}


static LRESULT WINAPI CALLBACK EditWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
	switch(message){
		case WM_CHAR:
			if(wParam == VK_RETURN){
				extern Client client;
				char buf[256];
				wchar_t wbuf[128];
				GetWindowText(hWnd, buf, sizeof buf);
				if(!buf[0])
					return 0;
				MultiByteToWideChar(CP_THREAD_ACP, 0, buf, -1, wbuf, sizeof wbuf);
				WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, sizeof buf, NULL, NULL);
				client.sendChat(buf);
				SetWindowText(hWnd, "");
				return 0;
			}
			if(wParam == VK_ESCAPE){
				SetForegroundWindow(hWndApp);
				return 0;
			}
	}
	return DefEditWndProc(hWnd, message, wParam, lParam);
}

