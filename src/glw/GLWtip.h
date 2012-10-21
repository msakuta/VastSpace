/** \file
 * \brief GLWtip class definition.
 * \sa glwindow.h
 */
#ifndef GLW_GLWTIP_H
#define GLW_GLWTIP_H
#include "glw/glwindow.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <gl/gl.h>
#include <cpplib/dstring.h>

/// The tip window. It floats over all GLwindows to describe what the element (e.g. button) means
/// to the player.
class GLWtip : public GLwindow{
public:
	cpplib::dstring tips;
	GLelement *parent;
	GLWtip() : st(), tips(NULL), parent(NULL){
		xpos = -100;
		ypos = -100;
		width = -100;
		height = -100;
		setVisible(false); // Initially invisible
		glwAppend(this); // Append after the object is constructed.
	}
	virtual bool focusable()const{return false;}
	virtual void draw(GLwindowState &ws, double t){
		if(!tips)
			return;
		GLWrect r = clientRect();
//		glwpos2d(r.x0, r.y0 + fontheight);
		glPushMatrix();
		glTranslated(r.x0, r.y0 + getFontHeight() + 2, 0.);
		glScaled(glwfontscale, glwfontscale, 1.);
		glwPutStringML(tips);
		glPopMatrix();
	}
};

extern GLWtip *glwtip;

#endif
