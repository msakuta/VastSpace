/// \file
/// \brief Declares GLWmessage
///
/// Depends on Squirrel.h so separated from glwindow.h.
#ifndef GLW_MESSAGE_H
#define GLW_MESSAGE_H
#include "glwindow.h"
#include <cpplib/dstring.h>
#include <squirrel.h>

/** \brief Small window that displays text for a period of time.
 *
 * You can pass event handler to response on destruction of this window.
 */
class GLWmessage : public GLwindow{
public:
	typedef GLwindow st;
	GLWmessage(const char *messagestring, double timer = 0., HSQUIRRELVM v = NULL, const char *onDestroy = NULL);
	GLWmessage(const char *messagestring, double timer, HSQUIRRELVM v, HSQOBJECT &hoOnDestroy);
	~GLWmessage();
	virtual void draw(GLwindowState &ws, double);
	virtual void anim(double dt);
	virtual bool focusable()const{return false;} ///< Message window needs not focusable.

	static void sq_define(HSQUIRRELVM v);

private:
	bool resized; ///< Internal flag that remember whether this window is sized to fit the string.
	dstring string; ///< Message string.
	dstring onDestroy; ///< Console command string executed on closing this window.
	double timer; ///< Time to destroy.
	HSQUIRRELVM v;
	HSQOBJECT hoOnDestroy; ///< Squirrel function or closure executed on closing this window.
	static SQInteger sqf_constructor(HSQUIRRELVM v);
};


#endif
