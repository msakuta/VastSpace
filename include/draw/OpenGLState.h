#ifndef DRAW_OPENGLSTATE_H
#define DRAW_OPENGLSTATE_H
/** \file
 * \brief Definition of OpenGLState class.
 */

#include "export.h"

#include <set>

/// \brief A class that accumulates OpenGL status variables such as texture units or shader program objects.
///
/// It can remember arbitrary class objects and frees them at once.
class EXPORT OpenGLState{
	class weak_ptr_base{
	public:
		virtual void destroy() = 0;
	};
public:
	template<typename T> class weak_ptr : public weak_ptr_base{
		T *p;
	public:
		weak_ptr(T *ap = NULL) : p(ap){}
		~weak_ptr(){delete p;}
		void destroy(){delete p; p = NULL;}
		T *create(OpenGLState &o){
			weak_ptr::~weak_ptr();
			p = new T;
			o.add(this);
			return p;
		}
		operator T*(){return p;}
		T *operator->(){return p;}
	};
	friend void *operator new(size_t size, OpenGLState &);
	void *add(weak_ptr_base *);
	~OpenGLState();

protected:
	std::set<weak_ptr_base *> objs;
};

/// Current OpenGL status vector. Re-createing this object will force all OpeGL resources to be reallocated.
EXPORT extern OpenGLState *openGLState;

#endif
