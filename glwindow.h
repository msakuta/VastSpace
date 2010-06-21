#ifndef GLWINDOW_H
#define GLWINDOW_H
#include "serial.h"
#include "popup.h"
extern "C"{
#include <clib/c.h>
#include <string.h>
}

/* flags */
#define GLW_CLOSE		1
#define GLW_SIZEABLE	2
#define GLW_COLLAPSABLE 4
#define GLW_COLLAPSE	8
#define GLW_PINNABLE	0x10
#define GLW_PINNED		0x20
#define GLW_SIZEPROP	0x40 /* proportional size change, i.e. ratio is reserved */
#define GLW_POPUP		0x80 /* hidden as soon as defocused */
#define GLW_TIP			0x100 /* hidden as soon as the cursor moves out */
#define GLW_TODELETE	0x8000

// Namespaces does not solve all the problems.
struct GLWrect{
	long l;
	long t;
	long r;
	long b;
	GLWrect(long al, long at, long ar, long ab) : l(al), t(at), r(ar), b(ab){}
	bool include(long x, long y)const{return l <= x && x <= r && t <= y && y <= b;}
	long width()const{return r - l;}
	long height()const{return b - t;}
	GLWrect &move(long x, long y){r += x - l; l = x; b += y - t; t = y; return *this;}
	GLWrect moved(long x, long y)const{return GLWrect(*this).move(x, y);}
	GLWrect &movebr(long x, long y){l += x - r; r = x; t += y - b; b = y; return *this;}
	GLWrect movedbr(long x, long y)const{return GLWrect(*this).movebr(x, y);}
	GLWrect &rmove(long dx, long dy){l += dx; r += dx; t += dy; b += dy; return *this;}
	GLWrect rmoved(long dx, long dy){return GLWrect(*this).rmove(dx, dy);}
};

//struct viewport;
struct GLwindowState{
	int w, h, m; // viewport size
	int mx, my; // mouse position
	int mousex, mousey; // mouse position in client coordinates of the window being drawn.
	void set(int vp[4]){ // no virtual allowed!
		w = vp[2] - vp[0], h = vp[3] - vp[1], m = MAX(w, h);
	}
};







typedef class GLwindow : public Serializable{
public:
	typedef GLwindow st;

	// Constants
	static const int glwfontwidth = 8;
	static const int glwfontheight = 12;
	static double getFontWidth();
	static double getFontHeight();

	// Global methods
	static int mouseFunc(int button, int state, int x, int y, GLwindowState &gvp);
	friend GLwindow **glwAppend(GLwindow *wnd);
	friend void glwActivate(GLwindow **ppwnd);
	static void reshapeFunc(int w, int h);
	void glwDraw(GLwindowState &, double t, int *);
	void glwDrawMinimized(GLwindowState &gvp, double t, int *pp);
	void glwAnim(double dt);
	static friend GLwindow **glwFindPP(GLwindow *);
	template<class C> static GLwindow **findpp(GLwindow **root, C compar);
	template<const char *title> static bool namecmp(const GLwindow *w){
		return !strcmp(w->title, title);
	}
	class TitleCmp;

	// Object methods
	const char *getTitle()const{return title;}
	void setTitle(const char *newTitle);
	void mouseDrag(int x, int y);
	int getX()const{return xpos;}
	int getY()const{return ypos;}
	virtual const char *classname()const;
	virtual GLWrect clientRect()const; // Get client rectangle
	virtual GLWrect extentRect()const; // GetWindowRect
	virtual GLWrect adjustRect(const GLWrect &client)const; // AdjustClientRect
	virtual bool focusable()const; // Returns wheter this window can have a focus to input from keyboards
	void setExtent(const GLWrect &r){xpos = r.l; ypos = r.t; width = r.r - r.l; height = r.b - r.t;}
	virtual int mouse(GLwindowState &ws, int key, int state, int x, int y);
	virtual void mouseEnter(GLwindowState &ws); // Called when the mouse pointer enters the window.
	virtual void mouseLeave(GLwindowState &ws); // Called when the mouse pointer leaves the window.
	virtual int key(int key); /* returns nonzero if processed */
	virtual int specialKey(int key); // Special keys like page up/down
	virtual void anim(double dt);
	virtual void postframe();
	static void glwpostframe();
	GLwindow *getNext(){return next;}
	void setPinned(bool f);
	void setPinnable(bool f){if(f) flags |= GLW_PINNABLE; else flags &= ~GLW_PINNABLE;}
	bool getPinned()const{return flags & GLW_PINNED;}
	bool getPinnable()const{return flags & GLW_PINNABLE;}
protected:
	GLwindow(const char *title = NULL);
	int xpos, ypos;
	int width, height;
	char *title;
	GLwindow *next;
	unsigned int flags;
	GLwindow *modal; /* The window that must be closed prior to proceed this window's process. */
	virtual void draw(GLwindowState &ws, double t);
	virtual ~GLwindow(); /* destructor method, NULL permitted */
private:
	void drawInt(GLwindowState &vp, double t, int mousex, int mousey, int, int);
	void glwFree();
} glwindow;

template<class C> inline GLwindow **GLwindow::findpp(GLwindow **root, C compar){
	for(GLwindow **pp = root; *pp; pp = &(*pp)->next) if((*compar)(*pp))
		return pp;
	return NULL;
}

class GLwindow::TitleCmp{
	const char *title;
public:
	TitleCmp(const char *a) : title(a){}
	bool operator()(const GLwindow *w){
		return w->title && !strcmp(w->title, title);
	}
};

extern GLwindow *glwlist;
extern GLwindow *glwfocus;
extern GLwindow *glwdrag;
extern int glwdragpos[2];


/* UI strings are urged to be printed by this function. */
void glwpos2d(double x, double y);
int glwprintf(const char *f, ...);
void glwVScrollBarDraw(glwindow *wnd, int x0, int y0, int w, int h, int range, int iy);
void glwHScrollBarDraw(glwindow *wnd, int x0, int y0, int w, int h, int range, int ix);
int glwVScrollBarMouse(glwindow *wnd, int mousex, int mousey, int x0, int y0, int w, int h, int range, int iy);
int glwHScrollBarMouse(glwindow *wnd, int mousex, int mousey, int x0, int y0, int w, int h, int range, int ix);

class GLwindowMenu : public GLwindow{
public:
	int count;
#if 0
	struct glwindowmenuitem{
		const char *title;
		int key;
		const char *cmd;
//		int allocated; /* whether this item need to be freed */
	} *menus;
#else
	typedef PopupMenuItem MenuItem;
	PopupMenu *menus;
#endif
	GLwindowMenu(const char *title, int count, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky);
	GLwindowMenu(const char *title, const PopupMenu &, unsigned flags = 0);
	void draw(GLwindowState &,double);
	int mouse(GLwindowState &ws, int button, int state, int x, int y);
	int key(int key);
	~GLwindowMenu();
	GLwindowMenu *addItem(const char *title, int key, const char *cmd);
	static int cmd_addcmdmenuitem(int argc, char *argv[], void *p);
};

extern const int glwMenuAllAllocated[];
extern const char glwMenuSeparator[];
GLwindowMenu *glwMenu(const char *title, int count, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky);
GLwindowMenu *glwMenu(const char *title, const PopupMenu &, unsigned flags);
glwindow *glwPopupMenu(GLwindowState &, int count, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky);
GLwindowMenu *glwPopupMenu(GLwindowState &, const PopupMenu &);


class GLwindowSizeable : public glwindow{
protected:
	float ratio; /* ratio of window size if it is to be reserved */
	int sizing; /* edge flags of changing borders */
	int minw, minh, maxw, maxh;
public:
	GLwindowSizeable(const char *title);
	int mouse(GLwindowState &ws, int button, int state, int x, int y);
};

class GLWbutton{
public:
	int xpos, ypos;
	int width, height;
	GLwindow *parent;
	GLWrect extentRect()const{return GLWrect(xpos, ypos, xpos + width, ypos + height);}
	virtual void draw(GLwindowState &, double) = 0;
	virtual int mouse(GLwindowState &ws, int button, int state, int x, int y) = 0;
	virtual void mouseEnter(GLwindowState &);
	virtual void mouseLeave(GLwindowState &);
	virtual ~GLWbutton(){}
};

class GLWcommandButton : public GLWbutton{
public:
	unsigned texname;
	const char *command;
	const char *tipstring;
	bool depress;
	GLWcommandButton(const char *filename, const char *command, const char *tips = NULL);
	virtual void draw(GLwindowState &, double);
	virtual int mouse(GLwindowState &, int button, int state, int x, int y);
	virtual void mouseLeave(GLwindowState &);
	virtual ~GLWcommandButton();
};

class GLWbuttonMatrix : public GLwindow{
	GLWbutton **buttons;
public:
	int xbuttons, ybuttons;
	int xbuttonsize, ybuttonsize;
	GLWbuttonMatrix(int x, int y, int xsize = 32, int ysize = 32);
	virtual void draw(GLwindowState &,double);
	virtual int mouse(GLwindowState &, int button, int state, int x, int y);
	virtual void mouseEnter(GLwindowState &);
	virtual void mouseLeave(GLwindowState &);
	bool addButton(GLWbutton *button, int x = -1, int y = -1);
};


inline void GLwindow::glwpostframe(){
	for(GLwindow *glw = glwlist; glw; glw = glw->next)
		glw->postframe();
}



// Implementation



inline void GLwindow::setPinned(bool f){
	if(f){
		flags |= GLW_PINNED;
		if(this == glwfocus)
			glwfocus = NULL;
	}
	else
		flags &= ~GLW_PINNED;
}

#endif
