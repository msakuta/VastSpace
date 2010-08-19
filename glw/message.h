#ifndef GLW_MESSAGE_H
#define GLW_MESSAGE_H
#include "glwindow.h"
#include <cpplib/dstring.h>

/** \brief Small window that displays text for a period of time.
 *
 * You can pass event handler to response on destruction of this window.
 */
class GLWmessage : public GLwindow{
	bool resized;
	dstring string;
	dstring onDestroy;
	double timer;
public:
	typedef GLwindow st;
	GLWmessage(const char *messagestring, double timer = 0., const char *onDestroy = NULL);
	~GLWmessage();
	virtual void draw(GLwindowState &ws, double);
	virtual void anim(double dt);
	virtual bool focusable()const{return false;}
};


#endif
