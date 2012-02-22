#ifndef OBSERVABLE_H
#define OBSERVABLE_H
/// \brief Definition of Observer, Observable, ObservePtr, WeakPtrBase and WeakPtr classes.
#include "serial_util.h"

#include <map>
#include <set>
#include <stddef.h> // offsetof

class Observable;
class WeakPtrBase;

/// \brief The object that can watch an Observable object for its destruction.
class EXPORT Observer{
public:
	/// \brief Called on destructor of the Entity being controlled.
	///
	/// Not all controller classes need to be destroyed, but some do. For example, the player could control
	/// entities at his own will, but not going to be deleted everytime it switches controlled object.
	/// On the other hand, Individual AI may be bound to and is destined to be destroyed with the entity.
	virtual bool unlink(Observable *);
};


/// \brief The object that can notify its observers when it is deleted.
///
/// It internally uses a list of pairs of Observer and its reference count.
/// The reference count is necessary for those Observers who has multiple
/// pointers that can point to the same Observable.
///
/// It's the Observer's responsibility to remove itself from the list when
/// the Observer itself is deleted.
///
/// The observer list would be obsolete soon.
class EXPORT Observable{
public:
	typedef std::map<Observer*, int> ObserverList;
	typedef std::set<WeakPtrBase*> WeakPtrList;
	~Observable();
	void addObserver(Observer *);
	void removeObserver(Observer *);
	void addWeakPtr(WeakPtrBase *);
	void removeWeakPtr(WeakPtrBase *);
protected:
	ObserverList observers;
	WeakPtrList ptrs;
};

/// \brief The class template to generate automatic pointers that adds the
///  containing object to referred Observable's Observer list.
///
/// It's a kind of smart pointers that behaves like a plain pointer, but
/// assigning a pointer to this object implicitly adds reference to the
/// pointed object, and setting another pointer value removes the
/// reference.
///
/// It costs no additional memory usage compared to plain pointer, because
/// it refers to its containing object by statically subtracting address.
/// It is implemented with template instantiation, which I'm afraid is less
/// portable, especially for Linux gcc.
///
/// Another problem in practice is that ObservePtr requires separate type
/// for each occurrence in a class members. For this purpose, the second
/// template parameter is provided to just distinguish those occurrences.
///
/// Also it's suspicious that it worths saving a little memory space in
/// exchange with code complexity, in terms of speed optimization.
///
/// At least it works on Windows with VC.
///
/// Either way, the big difference is that it obsoletes the postframe() and
/// even endframe(). An Observable object can delete itself as soon as it
/// wants to, and the containing pointer list adjusts the missing link with
/// the object's destructor.
/// It should eliminate the overhead for checking delete flag for all the
/// objects in the list each frame.
///
/// TODO: Notify other events, such as WarField transition.
template<typename T, int I, typename TP>
class EXPORT ObservePtr{
	TP *ptr;
	size_t ofs();
	T *getThis(){
		return (T*)((char*)this - ofs());
	}
public:
	ObservePtr(TP *o = NULL) : ptr(o){
		if(o)
			o->addObserver(getThis());
	}
	~ObservePtr(){
		if(ptr)
			ptr->removeObserver(getThis());
	}
	ObservePtr &operator=(TP *o){
		if(ptr)
			ptr->removeObserver(getThis());
		if(o)
			o->addObserver(getThis());
		ptr = o;
		return *this;
	}
	void unlinkReplace(TP *o){
		if(o)
			o->addObserver(getThis());
		ptr = o;
	}
	operator TP *()const{
		return ptr;
	}
	TP *operator->()const{
		return ptr;
	}
	operator bool()const{
		return !!ptr;
	}
};

/// \brief A base class for the weak pointer to an Observable object.
///
/// You cannot create an instance of this class.
/// Use the WeakPtr class template to create a type for your needs.
///
/// It registers itself to the Observable's pointer list which will
/// be cleared when the object is destroyed.
///
/// That's a re-invention of wheels.
/// Probably we should use boost library or C++11's std::weak_ptr.
class EXPORT WeakPtrBase{
	/// Prohibit using the copy constructor.
	/// One must initialize the object with raw pointer as the argument, or the pointed Observable will miss counting.
	WeakPtrBase(WeakPtrBase &){}
protected:
	Observable *ptr;
	WeakPtrBase(Observable *o = NULL) : ptr(o){
		if(o)
			o->addWeakPtr(this);
	}
	~WeakPtrBase(){
		if(ptr)
			ptr->removeWeakPtr(this);
	}
	WeakPtrBase &operator=(Observable *o){
		if(ptr)
			ptr->removeWeakPtr(this);
		if(o)
			o->addWeakPtr(this);
		ptr = o;
		return *this;
	}
	void unlink(Observable *o){
		if(ptr == o)
			ptr = NULL;
	}
	friend class Observable;
};

/// \brief A weak pointer to an Observable-derived object.
///
/// The template parameter class must not virtually inherit Observable, since
/// the internal Observable pointer downcasts to the parameter's class statically.
///
/// If you really need a pointer to a class object that inherits Observable
/// virtually, you can rewrite this class to use dynamic_cast, or you can
/// define another base class that has a virtual function to return the Observable
/// class pointer that is retrieved from a member pointer of derived class.
template<typename P>
class EXPORT WeakPtr : public WeakPtrBase{
public:
	WeakPtr(P *o = NULL) : WeakPtrBase(o){}
	WeakPtr(WeakPtr &o) : WeakPtrBase(static_cast<Observable*>(o)){} ///< Prevent the default copy constructor from being called.
	WeakPtr &operator=(P *o){
		return static_cast<WeakPtr&>(WeakPtrBase::operator=(o));
	}
	operator P *()const{
		return static_cast<P*>(ptr);
	}
	P *operator->()const{
		return static_cast<P*>(ptr);
	}
	operator bool()const{
		return !!ptr;
	}
};




//-----------------------------------------------------------------------------
//    Inline Implementation
//-----------------------------------------------------------------------------

template<typename P>
inline SerializeStream &operator<<(SerializeStream &us, const WeakPtr<P> &p){
	P *a = p;
	us << a;
	return us;
}

template<typename P>
inline UnserializeStream &operator>>(UnserializeStream &us, WeakPtr<P> &p){
	P *a;
	us >> a;
	p = a;
	return us;
}


template<typename T, int I, typename TP>
inline SerializeStream &operator<<(SerializeStream &us, const ObservePtr<T,I,TP> &p){
	TP *a = p;
	us << a;
	return us;
}

template<typename T, int I, typename TP>
inline UnserializeStream &operator>>(UnserializeStream &us, ObservePtr<T,I,TP> &p){
	TP *a;
	us >> a;
	p = a;
	return us;
}

#endif
