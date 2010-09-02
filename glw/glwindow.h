#ifndef GLWINDOW_H
#define GLWINDOW_H
#include "../serial.h"
#include "popup.h"
extern "C"{
#include <clib/c.h>
#include <string.h>
}
/** \file
 * \brief A header for OpenGL window system.
 *
 * OpenGL window system is somewhat like Windows' one, it has messaging mechanism
 * to notify and response events. The difference is that it does not have a message
 * queue like Windows. All messages are sent from calling thread immediately.
 */

/* flags */
#define GLW_CLOSE		1 ///< Presense of close button (x) at titlebar.
#define GLW_SIZEABLE	2 ///< Whether this window is sizeable.
#define GLW_COLLAPSABLE 4 ///< Collapsable window has a button on its titlebar to toggle collapsed state.
#define GLW_COLLAPSE	8 ///< Whether this window is collapsed.
#define GLW_PINNABLE	0x10 ///< Pinnable window has a button on its titlebar to toggle pinned state.
#define GLW_PINNED		0x20 ///< Pinned window cannot be moved nor focused.
#define GLW_SIZEPROP	0x40 ///< proportional size change, i.e. ratio is reserved
#define GLW_POPUP		0x80 ///< hidden as soon as defocused
#define GLW_TIP			0x100 ///< hidden as soon as the cursor moves out
#define GLW_INVISIBLE   0x200 ///< default (false) is visible
#define GLW_TODELETE	0x8000 ///< Being deleted by the end of the frame

// Namespaces does not solve all the problems.
/// Rectangle object like Windows RECT structure.
struct GLWrect{
	long x0;
	long y0;
	long x1;
	long y1;
	GLWrect(long ax0, long ay0, long ax1, long ay1) : x0(ax0), y0(ay0), x1(ax1), y1(ay1){}

	/// Returns true if given point is included in this rectangle.
	bool include(long x, long y)const{return x0 <= x && x <= x1 && y0 <= y && y <= y1;}
	long width()const{return x1 - x0;}
	long height()const{return y1 - y0;}

	/// Move this rectangle's top left corner to given point without changing size.
	GLWrect &move(long x, long y){x1 += x - x0; x0 = x; y1 += y - y0; y0 = y; return *this;}

	/// Returns copy of this rectangle that moved to given point.
	GLWrect moved(long x, long y)const{return GLWrect(*this).move(x, y);}

	/// Move this rectangle's bottom right corner to given point without changing size.
	GLWrect &movebr(long x, long y){x0 += x - x1; x1 = x; y0 += y - y1; y1 = y; return *this;}

	/// Returns copy of this rectangle that moved to given point.
	GLWrect movedbr(long x, long y)const{return GLWrect(*this).movebr(x, y);}

	/// Move this rectangle relatively by given values.
	GLWrect &rmove(long dx, long dy){x0 += dx; x1 += dx; y0 += dy; y1 += dy; return *this;}

	/// Returns copy of this rectangle relatively moved by given values.
	GLWrect rmoved(long dx, long dy){return GLWrect(*this).rmove(dx, dy);}
};

//struct viewport;
/// Represents OpenGL status for drawing windows.
struct GLwindowState{
	int w, h, m; // viewport size
	int mx, my; // mouse position
	int mousex, mousey; // mouse position in client coordinates of the window being drawn.
	void set(int vp[4]){ // no virtual allowed!
		w = vp[2] - vp[0], h = vp[3] - vp[1], m = MAX(w, h);
	}
};




/** \brief Base class of all OpenGL window system elemets.
 *
 * OpenGL window system is associated with an OpenGL viewport.
 *
 * All drawable OpenGL window system elements should derive this class.
 */
class GLelement : public Serializable{
public:
	GLelement() : xpos(0), ypos(0), width(100), height(100), flags(0){}
	void setExtent(const GLWrect &r){xpos = r.x0; ypos = r.y0; width = r.x1 - r.x0; height = r.y1 - r.y0;}
	void setVisible(bool f){if(!f) flags |= GLW_INVISIBLE; else flags &= ~GLW_INVISIBLE;}
	bool getVisible()const{return !(flags & GLW_INVISIBLE);}
protected:
	int xpos, ypos;
	int width, height;
	unsigned int flags;
};

/** \brief Overlapped window in OpenGL window system.
 *
 * This class object can have titlebar and some utility buttons in non-client area.
 *
 * Windows that draw something into client area should derive this class to override draw member function.
 *
 * GLwindow is also bound to Squirrel class object.
 */
class GLwindow : public GLelement{
public:
	typedef GLwindow st;

	// Constants
	static const int glwfontwidth = 8;
	static const int glwfontheight = 12;
	static double glwfontscale;
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
	static void glwPostFrame();
	static void glwEndFrame();
	static friend GLwindow **glwFindPP(GLwindow *);
	template<class C> static GLwindow **findpp(GLwindow **root, C compar);
	template<const char *title> static bool namecmp(const GLwindow *w){
		return !strcmp(w->title, title);
	}
	class TitleCmp;

	// Object methods
	const char *getTitle()const{return title;} ///< \return Title string of this window
	void setTitle(const char *newTitle);
	void mouseDrag(int x, int y);
	int getX()const{return xpos;}
	int getY()const{return ypos;}
	virtual const char *classname()const;
	virtual GLWrect clientRect()const; ///< Get client rectangle
	virtual GLWrect extentRect()const; ///< Something like GetWindowRect
	virtual GLWrect adjustRect(const GLWrect &client)const; ///< Something like AdjustClientRect

	/// Returns whether this window can have a focus to input from keyboards
	virtual bool focusable()const;

	/// \brief Mouse event handler.
	/// Derived classes can override to define mouse responses.
	virtual int mouse(GLwindowState &ws, int key, int state, int x, int y);

	/// Called when the mouse pointer enters the window.
	virtual void mouseEnter(GLwindowState &ws); 

	/// Called when the mouse pointer leaves the window.
	virtual void mouseLeave(GLwindowState &ws);

	/// Key event function, returns nonzero if processed
	virtual int key(int key);

	/// Event handler for special keys like page up/down.
	virtual int specialKey(int key);

	/// Called every frame.
	virtual void anim(double dt);

	/// Called when a frame is about to end. Derived classes should assign NULL to member pointers
	/// that point to objects being destroyed.
	virtual void postframe();
	static void glwpostframe();
	GLwindow *getNext(){return next;} ///< \brief Getter for next member.
	void setVisible(bool f);
	void setClosable(bool f){if(f) flags |= GLW_CLOSE; else flags &= ~GLW_CLOSE;}
	void setCollapsable(bool f){if(f) flags |= GLW_COLLAPSABLE; else flags &= ~GLW_COLLAPSABLE;}
	void setPinned(bool f);
	void setPinnable(bool f){if(f) flags |= GLW_PINNABLE; else flags &= ~GLW_PINNABLE;}
	bool isDestroying()const{return !!(flags & GLW_TODELETE);} ///< Is being destroyed. All referrers must be aware of this.
	bool getVisible()const{return !(flags & GLW_INVISIBLE);} ///< Inverting logic
	bool getClosable()const{return !!(flags & GLW_CLOSE);}
	bool getCollapsable()const{return !!(flags & GLW_COLLAPSABLE);}
	bool getPinned()const{return !!(flags & GLW_PINNED);}
	bool getPinnable()const{return !!(flags & GLW_PINNABLE);}
	static GLwindow *getCaptor(){return captor;}
	void postClose(){ flags |= GLW_TODELETE; }
protected:
	GLwindow(const char *title = NULL);
	char *title;

	/// Next window in window list. Early windows are drawn over later windows in the list.
	GLwindow *next;

	/// The window that must be closed prior to proceed this window's process.
	GLwindow *modal;

	/// Derived classes can override to describe drawing in client area.
	virtual void draw(GLwindowState &ws, double t);

	/// Destructor method, NULL permitted.
	virtual ~GLwindow();

	static int mouseFuncNC(GLwindow **ppwnd, GLwindowState &ws, int key, int state, int x, int y, int minix, int miniy);

	/// The window which the mouse pointer floating over at the last frame.
	static GLwindow *lastover;
	static GLwindow *captor; ///< Mouse captor
private:
	void drawInt(GLwindowState &vp, double t, int mousex, int mousey, int, int);
	void glwFree();
};

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

/// Inverting logic
inline void GLwindow::setVisible(bool f){
	if(!f){
		flags |= GLW_INVISIBLE;
		if(glwfocus == this) // defocus from hiding window
			glwfocus = NULL;
	}
	else flags &= ~GLW_INVISIBLE;
}


/* UI strings are urged to be printed by this function. */
void glwpos2d(double x, double y);
int glwprintf(const char *f, ...);
int glwsizef(const char *f, ...);
void glwVScrollBarDraw(GLwindow *wnd, int x0, int y0, int w, int h, int range, int iy);
void glwHScrollBarDraw(GLwindow *wnd, int x0, int y0, int w, int h, int range, int ix);
int glwVScrollBarMouse(GLwindow *wnd, int mousex, int mousey, int x0, int y0, int w, int h, int range, int iy);
int glwHScrollBarMouse(GLwindow *wnd, int mousex, int mousey, int x0, int y0, int w, int h, int range, int ix);

/// Window that lists menus. Menu items are bound to console commands.
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
	virtual GLwindowMenu *addItem(const char *title, int key, const char *cmd);
	static int cmd_addcmdmenuitem(int argc, char *argv[], void *p);
	static GLwindowMenu *newBigMenu();
};

extern const int glwMenuAllAllocated[];
extern const char glwMenuSeparator[];
GLwindowMenu *glwMenu(const char *title, int count, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky);
GLwindowMenu *glwMenu(const char *title, const PopupMenu &, unsigned flags);
GLwindow *glwPopupMenu(GLwindowState &, int count, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky);
GLwindowMenu *glwPopupMenu(GLwindowState &, const PopupMenu &);


/// Base class for sizeable windows.
class GLwindowSizeable : public GLwindow{
protected:
	float ratio; /* ratio of window size if it is to be reserved */
	int sizing; /* edge flags of changing borders */
	int minw, minh, maxw, maxh;
public:
	GLwindowSizeable(const char *title);
	int mouse(GLwindowState &ws, int button, int state, int x, int y);
};

/// The simplest display element in GLwindow system. This is base class
/// and all actual buttons inherit this class.
class GLWbutton : public GLelement{
public:
	const char *classname()const;
	GLwindow *parent;
	const char *tipstring;
	bool depress;
	GLWbutton() : parent(NULL), tipstring(NULL), depress(false){}
	GLWrect extentRect()const{return GLWrect(xpos, ypos, xpos + width, ypos + height);}
	virtual void draw(GLwindowState &, double) = 0;
	virtual int mouse(GLwindowState &ws, int button, int state, int x, int y);
	virtual void mouseEnter(GLwindowState &);
	virtual void mouseLeave(GLwindowState &);
	virtual void press() = 0;
	virtual ~GLWbutton(){}
};

/// Impulsive command buttons that activates when mouse button is pressed down and released.
class GLWcommandButton : public GLWbutton{
public:
	unsigned texname;
	const char *command;
	GLWcommandButton(const char *filename, const char *command, const char *tips = NULL);
	virtual void draw(GLwindowState &, double);
//	virtual int mouse(GLwindowState &, int button, int state, int x, int y);
	virtual void mouseLeave(GLwindowState &);
	virtual void press();
	virtual ~GLWcommandButton();
};

/// 2-State button.
class GLWstateButton : public GLWbutton{
public:
	unsigned texname, texname1;
	const char *command;
	GLWstateButton(const char *filename, const char *filename1, const char *tips = NULL);
	virtual ~GLWstateButton();
	virtual void draw(GLwindowState &, double);
//	virtual int mouse(GLwindowState &, int button, int state, int x, int y);
	virtual void mouseLeave(GLwindowState &);
	virtual bool state()const = 0; ///< Override to define depressed state.
	virtual void press() = 0;
};

class GLWtoggleCvarButton : public GLWstateButton{
public:
	int &var;
	GLWtoggleCvarButton(const char *filename, const char *filename1, int &cvar, const char *tip = NULL) :
		GLWstateButton(filename, filename1, tip), var(cvar){}
	virtual bool state()const;
	virtual void press();
};

class Player;
class GLWmoveOrderButton : public GLWstateButton{
public:
	Player *pl;
	GLWmoveOrderButton(const char *filename, const char *filename1, Player *apl, const char *tip = NULL) :
		GLWstateButton(filename, filename1, tip), pl(apl){}
	virtual bool state()const;
	virtual void press();
};

/// A window with GLWbuttons in matrix.
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





// Implementation

/// Runs postframe for all windows. Also takes care of lastover pointer.
inline void GLwindow::glwpostframe(){
	for(GLwindow *glw = glwlist; glw; glw = glw->next)
		glw->postframe();

	// Window being destroyed will cause dangling pointer with mouse pointer over it.
	if(lastover && lastover->flags & GLW_TODELETE)
		lastover = NULL;
	if(captor && captor->flags & GLW_TODELETE)
		captor = NULL;
}

/// Pinned window cannot be focused.
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
