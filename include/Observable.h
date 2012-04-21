#ifndef OBSERVABLE_H
#define OBSERVABLE_H
/** \file
 * \brief Definition of Observer, Observable, ObservePtr, WeakPtrBase and WeakPtr classes.
 */
#include "serial_util.h"

#include <map>
#include <set>
#include <stddef.h> // offsetof
#include <squirrel.h>

class Observable;
class WeakPtrBase;
struct ObserveEvent;

/// \brief The object that can watch an Observable object for its destruction.
class EXPORT Observer{
public:
	/// \brief Called on destructor of the Entity being controlled.
	///
	/// Not all controller classes need to be destroyed, but some do. For example, the player could control
	/// entities at his own will, but not going to be deleted everytime it switches controlled object.
	/// On the other hand, Individual AI may be bound to and is destined to be destroyed with the entity.
	virtual bool unlink(Observable *);

	/// \brief The function that receives event messages sent from the observed object.
	/// \param o The observed object that invoked the event.
	/// \param e The event object.
	/// \returns True if handled.
	virtual bool handleEvent(Observable *o, ObserveEvent &e);
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
	~Observable();
	void addObserver(Observer *);
	void removeObserver(Observer *);
	void addWeakPtr(WeakPtrBase *);
	void removeWeakPtr(WeakPtrBase *);
	void notifyEvent(ObserveEvent &e);
protected:
	ObserverList observers;
};


/// \brief The type of value to identify ObserveEvent derived classes.
typedef const char *ObserveEventID;

/// \brief The event object type that can be sent from Observable to Observer.
///
/// The convention is that derived structs must have a static member variable named sid that
/// stores ObserveEventID uniquely indicate the struct type.
/// A string literal of the struct name is commonly used.
/// This rule is required to enable InterpretEvent and InterpretDerivedEvent to function properly.
struct ObserveEvent{
	/// Type of the constructor map.
	typedef std::map<ObserveEventID, ObserveEvent *(*)(HSQUIRRELVM, Observer &), bool (*)(ObserveEventID, ObserveEventID)> MapType;

	/// Constructor map. The key must be a pointer to a static string, which lives as long as the program.
	static MapType &ctormap();

	/** \brief Returns unique ID for this class.
	 *
	 * The returned pointer never be dereferenced without debugging purposes,
	 * it is just required to point the same address for all the instances but
	 * never coincides between different classes.
	 * A static const string of class name is ideal for this returned vale.
	 */
	virtual ObserveEventID id()const = 0;

	/** \brief Derived or exact class returns true.
	 *
	 * Returns whether the given event ID is the same as this object's class's or its derived classes.
	 */
	virtual bool derived(ObserveEventID)const;

	ObserveEvent(){}

	/// Derived classes use this utility to register class.
	static int registerObserveEvent(const char *name, ObserveEvent *(*ctor)(HSQUIRRELVM, Observer &)){
		ctormap()[name] = ctor;
		return 0;
	}
};

/// A template function that tests whether given event matches the template argument class.
///
/// Re-invention of RTTI, but expected faster.
template<typename EvtType>
EvtType *InterpretEvent(ObserveEvent &evt){
	return evt.id() == EvtType::sid ? static_cast<EvtType*>(&evt) : NULL;
}

/// A template function that tests whether given event derives the template argument class.
///
/// Re-invention of RTTI, but expected faster.
template<typename EvtType>
EvtType *InterpretDerivedEvent(ObserveEvent &evt){
	return evt.derived(EvtType::sid) ? static_cast<EvtType*>(&evt) : NULL;
}





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
class EXPORT WeakPtrBase : public Observer{
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
	/// This operator overloading is necessary to correctly handle assignment expression.
	WeakPtrBase &operator=(WeakPtrBase &o){
		return operator=(o.ptr);
	}
	bool unlink(Observable *o){
		if(ptr == o)
			ptr = NULL;
		return true;
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
	bool operator<(const WeakPtr &o)const{
		return ptr < o.ptr;
	}
};


/// \brief A set of Observables that automatically removes expired elements.
/// \param T is element type, must be a subclass of and be static_cast-able from Observable.
///
/// All occasions of item insertion are overridden so that all pointers to
/// Observables are managed to prevent dangling pointer.
///
/// I thought it could publicly inherit std::set<T*>, but it would introduce so many
/// ways to add or remove elements that overriding them all would be a pain.
/// Instead, we limited the ways to access the contents for changing to ensure
/// the pointers to be managed.
///
/// Basically, this class can be used much like std::set, for it resembles std::set
/// in methods such as begin() and end(). Actually, most methods just delegates
/// to internal set object.
template<typename T>
class ObservableSet : public Observer{
	std::set<T*> set_;
	ObservableSet &operator=(ObservableSet&){} ///< Prohibit copy construction.
public:
	typedef typename std::set<T*>::iterator iterator;
	typedef typename std::set<T*>::const_iterator const_iterator;
	~ObservableSet(){
		clear();
	}
	iterator begin(){return set_.begin();}
	iterator end(){return set_.end();}
	const_iterator begin()const{return set_.begin();}
	const_iterator end()const{return set_.end();}
	size_t size()const{return set_.size();}
	bool empty()const{return set_.empty();}
	iterator find(T *o){return set_.find(o);}
	void insert(T *o){
		iterator it = set_.find(o);
		if(it == set_.end())
			o->addObserver(this);
		set_.insert(o);
	}

	// The types iterator and const_iterator seem to have the same
	// signature in gcc, so defining both causes conflict.
	// I'm not sure leaving const_iterator version is correct choice,
	// but it should be easy to cast to non-const version.
/*	void erase(iterator &it){
		if(it != set_.end()){
			(*it)->removeObserver(this);
			set_.erase(it);
		}
	}*/
	void erase(const_iterator &it){
		if(it != set_.end()){
			(*it)->removeObserver(this);
			set_.erase(it);
		}
	}
	void clear(){
		for(iterator it = set_.begin(); it != set_.end(); it++)
			(*it)->removeObserver(this);
		set_.clear();
	}
	bool unlink(Observable *o){
		iterator it = set_.find(static_cast<T*>(o));
		if(it != set_.end())
			set_.erase(it);
		return true;
	}
};


/// \brief A list of Observables that automatically removes expired elements.
/// \param T is element type, must be a subclass of and be static_cast-able from Observable.
///
/// All occasions of item insertion are overridden so that all pointers to
/// Observables are managed to prevent dangling pointer.
///
/// A list (actually, vector) conterpart of ObservableSet.
/// Sometimes we want to use an ordered list of Observables that manages elements automatically.
template<typename T>
class ObservableList : public Observer{
	std::vector<T*> list_;
	ObservableList &operator=(ObservableList&){} ///< Prohibit copy construction.
public:
	typedef typename std::vector<T*>::iterator iterator;
	typedef typename std::vector<T*>::const_iterator const_iterator;
	~ObservableList(){
		clear();
	}
	iterator begin(){return list_.begin();}
	iterator end(){return list_.end();}
	const_iterator begin()const{return list_.begin();}
	const_iterator end()const{return list_.end();}
	size_t size()const{return list_.size();}
	bool empty()const{return list_.empty();}
	iterator find(T *o){return list_.find(o);}
	void push_back(T *o){
		o->addObserver(this);
		list_.push_back(o);
	}

	// We do not necessarily "own" the elements, so we can return non-const pointer
	// of elements even if this pointer is const.
	T *operator[](int i)const{
		return list_[i];
	}

	void erase(iterator &it){
		if(it != list_.end()){
			(*it)->removeObserver(this);
			list_.erase(it);
		}
	}
	void clear(){
		for(iterator it = list_.begin(); it != list_.end(); it++)
			(*it)->removeObserver(this);
		list_.clear();
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


#endif
