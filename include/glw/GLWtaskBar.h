/** \file
 * \brief A header for OpenGL window system.
 *
 * OpenGL window system is somewhat like Windows' one, it has messaging mechanism
 * to notify and response events. The difference is that it does not have a message
 * queue like Windows. All messages are sent from calling thread immediately.
 */
#ifndef GLW_GLWTASKBAR_H
#define GLW_GLWTASKBAR_H
#include "glwindow.h"

/// \brief The window class for the task bar.
class GLWtaskBar : public GLwindowSizeable{
public:
	typedef GLwindowSizeable st;
	GLWtaskBar(Game *game);

	int mouse(GLwindowState &, int button, int state, int x, int y);
	void mouseEnter(GLwindowState &ws);
	void mouseLeave(GLwindowState &ws);

	bool addButton(GLWbutton *button, int pos = -1);

	static bool sq_define(HSQUIRRELVM v);

protected:
	ObservableList<GLWbutton> buttons;
	int xbuttonsize, ybuttonsize;
	static SQInteger sqf_constructor(HSQUIRRELVM v);
	void drawInt(GLwindowState &vp, double t, int mousex, int mousey, int, int);
};

#endif
