/// \file
/// \brief Declares GLWchat
///
/// Depends on Squirrel.h so separated from glwindow.h.
#ifndef GLW_GLWCHAT_H
#define GLW_GLWCHAT_H
#include "glwindow.h"
#include <windows.h>
#include <set>

/** \brief Window that displays chat text log and entry text box.
 */
class GLWchat : public GLwindowSizeable{
public:
	typedef GLwindow st;
	GLWchat();
	~GLWchat();
	virtual void changeExtent();
	virtual void draw(GLwindowState &ws, double);
	virtual void anim(double dt);
	virtual int mouse(GLwindowState &ws, int button, int state, int mx, int my);

	void append(dstring);

	static bool sq_define(HSQUIRRELVM v);
	static std::set<GLWchat*> wndlist;

private:
	HWND hEdit; ///< The Window handle of the OS.
	std::vector<dstring> buf; ///< Message string.
	int scrollpos; ///< Current scroll position in the vertical scroll bar.
	static SQInteger sqf_constructor(HSQUIRRELVM v);
	int scrollrange();
};


#endif
