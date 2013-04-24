/** \file
 * \brief A header for buttons in OpenGL window system.
 */
#ifndef GLW_GLWBUTTONS_H
#define GLW_GLWBUTTONS_H
#include "glwindow.h"


/// The simplest display element in GLwindow system. This is base class
/// and all actual buttons inherit this class.
class GLWbutton : public GLelement{
public:
	typedef GLelement st;
	const char *classname()const;
	GLwindow *parent;
	const char *tipstring;
	float flashTime; ///< Remaining time for flashing effect in seconds
	bool depress;
	GLWbutton(Game *game) : st(game), parent(NULL), tipstring(NULL), flashTime(0.f), depress(false){}
	GLWrect extentRect()const{return GLWrect(xpos, ypos, xpos + width, ypos + height);}
	virtual void anim(double dt);
	virtual void draw(GLwindowState &, double) = 0;
	virtual int mouse(GLwindowState &ws, int button, int state, int x, int y);
	virtual void mouseEnter(GLwindowState &);
	virtual void mouseLeave(GLwindowState &);
	virtual void press() = 0;
	virtual ~GLWbutton(){}
protected:
	virtual SQInteger sqGet(HSQUIRRELVM v, const SQChar *)const;
	virtual SQInteger sqSet(HSQUIRRELVM v, const SQChar *);
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
	~GLWbuttonMatrix();
	virtual void anim(double dt);
	virtual void draw(GLwindowState &,double);
	virtual bool focusable()const;
	virtual int mouse(GLwindowState &, int button, int state, int x, int y);
	virtual void mouseEnter(GLwindowState &);
	virtual void mouseLeave(GLwindowState &);
	bool addButton(GLWbutton *button, int x = -1, int y = -1);
	static GLWbuttonMatrix *sq_refobj(HSQUIRRELVM, SQInteger idx = 1);
	GLWbutton *getButton(int index);
	GLWbutton *getButton(int x, int y);
};




//-----------------------------------------------------------------------------
//     GLWbuttonMatrix Inline Implementation
//-----------------------------------------------------------------------------

inline GLWbutton *GLWbuttonMatrix::getButton(int index){
	return 0 <= index && index < xbuttons * ybuttons ? buttons[index % xbuttons + index / ybuttons * xbuttons] : NULL;
}

inline GLWbutton *GLWbuttonMatrix::getButton(int x, int y){
	return 0 <= x && x < xbuttons && 0 <= y && y < ybuttons ? buttons[x + y / ybuttons * xbuttons] : NULL;
}


#endif
