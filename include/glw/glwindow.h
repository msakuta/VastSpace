#ifndef GLW_GLWINDOW_H
#define GLW_GLWINDOW_H
#include "export.h"
#include "serial.h"
#include "dstring.h"
#include "Observable.h"
extern "C"{
#include <clib/c.h>
#include <string.h>
}
#include <squirrel.h>
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
	bool include(long x, long y)const{return x0 <= x && x < x1 && y0 <= y && y < y1;}
	long width()const{return x1 - x0;}
	long height()const{return y1 - y0;}
	long hcenter()const{return (x0 + x1) / 2;} ///< Horizontal Center
	long vcenter()const{return (y0 + y1) / 2;} ///< Vertical Center

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

	/// Expand this rectangle with given pixels to four directions.
	/// \param v Can be negative.
	GLWrect &expand(long v){x0 -= v; x1 += v; y0 -= v; y1 += v; return *this;}

	/// Returns expanded version of this rectangle by a given value.
	GLWrect expanded(long v)const{return GLWrect(*this).expand(v);}
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
class EXPORT GLelement : public Serializable, public Observable{
public:
	/// Obsolete; the version with two args is recommended, the game object should be initialized.
	GLelement() : xpos(0), ypos(0), width(100), height(100), flags(0), onChangeExtent(NULL){}

	/// The constructor that can initialize the game object pointer.
	GLelement(Game *game, const char *title = NULL) : Serializable(game), xpos(0), ypos(0), width(100), height(100), flags(0), onChangeExtent(NULL){}

	void rawSetExtent(const GLWrect &r); ///< Changes the extent rectangle without invoking the event handler.
	virtual void changeExtent(); ///< Invokes event handler that is called when the size of this element is changed.
	void setExtent(const GLWrect &r); ///< Changes the extent rectangle. onChangeExtent event handler may be called.
	virtual GLWrect extentRect()const; ///< Something like GetWindowRect
	void setVisible(bool f){if(!f) flags |= GLW_INVISIBLE; else flags &= ~GLW_INVISIBLE;}
	bool getVisible()const{return !(flags & GLW_INVISIBLE);}

	/// Creates and pushes an Entity object to Squirrel stack.
	static void sq_pushobj(HSQUIRRELVM, GLelement *);

	/// Returns an Entity object being pointed to by an object in Squirrel stack.
	static GLelement *sq_refobj(HSQUIRRELVM, SQInteger idx = 1);

	/// Assign a GLelement-derived object to the instance on top of Squirrel stack instead of pushing.
	static void sq_assignobj(HSQUIRRELVM v, GLelement *, SQInteger idx = 1);

	/// Define this class for Squirrel VM.
	static bool sq_define(HSQUIRRELVM v);

	/// The type tag for this class in Squirrel VM.
	static const SQUserPointer tt_GLelement;

protected:
	int xpos, ypos;
	int width, height;
	unsigned int flags;
	void (*onChangeExtent)(GLelement *); ///< The event handler that is called whenever the element size is changed.
	virtual SQInteger sqGet(HSQUIRRELVM v, const SQChar *)const; ///< The internal method to get properties, can be overridden.
	static SQInteger sqf_get(HSQUIRRELVM v);
	virtual SQInteger sqSet(HSQUIRRELVM v, const SQChar *); ///< The internal method to set properties, can be overridden.
	static SQInteger sqf_set(HSQUIRRELVM v);
	static SQInteger sqf_close(HSQUIRRELVM v);
	static SQInteger sqh_release(SQUserPointer p, SQInteger);
};

/** \brief Overlapped window in OpenGL window system.
 *
 * This class object can have titlebar and some utility buttons in non-client area.
 *
 * Windows that draw something into client area should derive this class to override draw member function.
 *
 * GLwindow is also bound to Squirrel class object.
 */
class EXPORT GLwindow : public GLelement{
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
	EXPORT friend GLwindow **glwAppend(GLwindow *wnd);
	EXPORT friend void glwActivate(GLwindow **ppwnd);
	static void reshapeFunc(int w, int h);
	void glwDraw(GLwindowState &, double t, int *);
	void glwDrawMinimized(GLwindowState &gvp, double t, int *pp);
	void glwAnim(double dt);
	static void glwPostFrame();
	static void glwEndFrame();
	friend GLwindow **glwFindPP(GLwindow *);
	template<class C> static GLwindow **findpp(GLwindow **root, C compar);
	class TitleCmp;
	static int glwMouseCursorState(int mousex, int mousey);
	virtual int mouseCursorState(int mousex, int mousey)const;

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

	/// \brief Opportunity to override default mouse behavior on non-client area.
	/// \param x,y mouse position in screen coordinates.
	/// \return If this method processes the event and the default reaction is to be suppressed.
	virtual bool mouseNC(GLwindowState &ws, int key, int state, int x, int y){return false;}

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
	GLwindow *getNext()const{return next;} ///< \brief Getter for next member.
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
	static GLwindow *getLastover(){return lastover;} ///< You can refer to but cannot alter lastover window.
	static GLwindow *getCaptor(){return captor;} ///< You can refer to but cannot alter captor window.
	static GLwindow *getDragStart(){return dragstart;}
	void postClose(){ flags |= GLW_TODELETE; }

	/// Creates and pushes an Entity object to Squirrel stack.
	static void sq_pushobj(HSQUIRRELVM, GLwindow *);

	/// Returns an Entity object being pointed to by an object in Squirrel stack.
	static GLwindow *sq_refobj(HSQUIRRELVM, SQInteger idx = 1);

	/// Define this class for Squirrel VM.
	static bool sq_define(HSQUIRRELVM v);

	/// The type tag for this class in Squirrel VM.
	static const SQUserPointer tt_GLwindow;

	const SQChar *sq_classname();

protected:
	/// Obsolete; the version with two args is recommended, the game object should be initialized.
	GLwindow(const char *title = NULL);

	/// The constructor that can initialize the game object pointer.
	GLwindow(Game *game, const char *title = NULL);

	/// The title string of this window.
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
	static GLwindow *dragstart; ///< \brief The wnidow dragging is began on.

	virtual SQInteger sqGet(HSQUIRRELVM v, const SQChar *)const;
	virtual SQInteger sqSet(HSQUIRRELVM v, const SQChar *);
	static SQInteger sqf_close(HSQUIRRELVM v);

	static const int GLWSIZEABLE_BORDER;
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

/// \brief Initializes the GLwindow system.
extern int glwInit();

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
EXPORT int glwPutTextureStringN(const char *s, int n, int size);
EXPORT int glwPutTextureString(const char *s, int size = GLwindow::getFontHeight());
EXPORT int glwGetSizeTextureStringN(const char *s, long n, int isize);
EXPORT int glwGetSizeTextureString(const char *s, int size = -1);
EXPORT int glwPutStringML(const char *s, int size = GLwindow::getFontHeight());
EXPORT void glwGetSizeStringML(const char *s, int size = GLwindow::getFontHeight(), int *retxsize = NULL, int *retysize = NULL);
EXPORT void glwpos2d(double x, double y);
EXPORT int glwprintf(const char *f, ...);
EXPORT int glwsizef(const char *f, ...);
EXPORT void glwVScrollBarDraw(GLwindow *wnd, int x0, int y0, int w, int h, int range, int iy);
EXPORT void glwHScrollBarDraw(GLwindow *wnd, int x0, int y0, int w, int h, int range, int ix);
EXPORT int glwVScrollBarMouse(GLwindow *wnd, int mousex, int mousey, int x0, int y0, int w, int h, int range, int iy);
EXPORT int glwHScrollBarMouse(GLwindow *wnd, int mousex, int mousey, int x0, int y0, int w, int h, int range, int ix);



/// Base class for sizeable windows.
class EXPORT GLwindowSizeable : public GLwindow{
protected:
	float ratio; /* ratio of window size if it is to be reserved */
	int sizing; /* edge flags of changing borders */
	int minw, minh, maxw, maxh;
public:
	/// Obsolete; the version with two args is recommended, the game object should be initialized.
	GLwindowSizeable(const char *title);

	/// The constructor that can initialize the game object pointer.
	GLwindowSizeable(Game *game, const char *title = NULL);

	int mouseCursorState(int mousex, int mousey)const;
	int mouse(GLwindowState &ws, int button, int state, int x, int y);
	bool mouseNC(GLwindowState &ws, int button, int state, int x, int y);
};

/// The simplest display element in GLwindow system. This is base class
/// and all actual buttons inherit this class.
class GLWbutton : public GLelement{
public:
	typedef GLelement st;
	const char *classname()const;
	GLwindow *parent;
	const char *tipstring;
	bool depress;
	GLWbutton(Game *game) : st(game), parent(NULL), tipstring(NULL), depress(false){}
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
	typedef GLWbutton st;
	gltestp::dstring texname;
	const char *command;
	GLWcommandButton(Game *game, const char *filename, const char *command, const char *tips = NULL);
	virtual void draw(GLwindowState &, double);
//	virtual int mouse(GLwindowState &, int button, int state, int x, int y);
	virtual void mouseLeave(GLwindowState &);
	virtual void press();
	virtual ~GLWcommandButton();
};

/// 2-State button.
class GLWstateButton : public GLWbutton{
public:
	typedef GLWbutton st;
	gltestp::dstring texname, texname1;
	const char *command;
	GLWstateButton(Game *game, const char *filename, const char *filename1, const char *tips = NULL);
	virtual ~GLWstateButton();
	virtual void draw(GLwindowState &, double);
//	virtual int mouse(GLwindowState &, int button, int state, int x, int y);
	virtual void mouseLeave(GLwindowState &);
	virtual bool state()const = 0; ///< Override to define depressed state.
	virtual void press() = 0;
};

/// 2-state button that is bound to a boolean cvar.
class GLWtoggleCvarButton : public GLWstateButton{
public:
	gltestp::dstring var;
	GLWtoggleCvarButton(Game *game, const char *filename, const char *filename1, gltestp::dstring cvar, const char *tip = NULL) :
		GLWstateButton(game, filename, filename1, tip), var(cvar){}
	virtual bool state()const;
	virtual void press();
};

/// A window with GLWbuttons in matrix.
class GLWbuttonMatrix : public GLwindow{
	GLWbutton **buttons;
public:
	int xbuttons, ybuttons;
	int xbuttonsize, ybuttonsize;
	GLWbuttonMatrix(Game *game, int x, int y, int xsize = 32, int ysize = 32);
	virtual void draw(GLwindowState &,double);
	virtual int mouse(GLwindowState &, int button, int state, int x, int y);
	virtual void mouseEnter(GLwindowState &);
	virtual void mouseLeave(GLwindowState &);
	bool addButton(GLWbutton *button, int x = -1, int y = -1);
	static GLWbuttonMatrix *sq_refobj(HSQUIRRELVM, SQInteger idx = 1);
};





//-----------------------------------------------------------------------------
//     Implementation
//-----------------------------------------------------------------------------


inline void GLelement::rawSetExtent(const GLWrect &r){
	xpos = r.x0; ypos = r.y0; width = r.x1 - r.x0; height = r.y1 - r.y0;
}

inline void GLelement::setExtent(const GLWrect &r){
	rawSetExtent(r);
	changeExtent();
}

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
