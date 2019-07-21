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
///
/// We do not define virtual destructor because the user should not delete an
/// Observer-derived object via Observer pointer, but more concrete pointer.
/// We may revisit this specification some time.
class EXPORT Observer{
public:
	/// \brief Called on destructor of the Observable being watched.
	/// \param o The observed object being destroyed.
	/// \returns True if handled.
	///
	/// Derived classes must override this function to define how to forget about
	/// the watched object.
	virtual bool unlink(const Observable *o);

	/// \brief The function that receives event messages sent from the observed object.
	/// \param o The observed object that invoked the event.
	/// \param e The event object.
	/// \returns True if handled.
	virtual bool handleEvent(const Observable *o, ObserveEvent &e);
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
	/// \brief The type for the internal list object.
	typedef std::map<Observer*, int> ObserverList;
	~Observable();
	void addObserver(Observer *)const;
	void removeObserver(Observer *)const;
	void addWeakPtr(WeakPtrBase *)const;
	void removeWeakPtr(WeakPtrBase *)const;
	void notifyEvent(ObserveEvent &e)const;
protected:
	/// \brief The list of observers that observing this object.
	///
	/// It's qualified as mutable because the observer can have a const-pointer to this object,
	/// in which case we should manage this list from the observer's perspective.
	mutable ObserverList observers;
	mutable WeakPtrBase* weakPtrHead = nullptr;
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
class EXPORT WeakPtrBase{
	/// Prohibit using the copy constructor.
	/// One must initialize the object with raw pointer as the argument, or the pointed Observable will miss counting.
	WeakPtrBase(WeakPtrBase &){}
protected:
	Observable *ptr = nullptr;
	WeakPtrBase* next = nullptr;
	WeakPtrBase(Observable *o = nullptr) : ptr(o){
		if(o)
			o->addWeakPtr(this);
	}
	virtual ~WeakPtrBase(){
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
	virtual bool unlink(const Observable *o){
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
class WeakPtr : public WeakPtrBase{
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

	// The following equality operators are not really necessary for logic.
	// They are just provided for performance reasons (prevents WeakPtr
	// instantiation in conditional expressions).
	bool operator==(const WeakPtr &o)const{
		return ptr == o.ptr;
	}
	bool operator!=(const WeakPtr &o)const{
		return !operator==(o);
	}
	friend bool operator==(P *a, const WeakPtr &b){
		return a == b.ptr;
	}
	friend bool operator!=(P *a, const WeakPtr b){
		return !(a == b);
	}
	friend bool operator==(const WeakPtr &a, P *b){
		return a.ptr == b;
	}
	friend bool operator!=(const WeakPtr &a, P *b){
		return !(a == b);
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
	typedef typename std::set<T*>::reverse_iterator reverse_iterator;
	typedef typename std::set<T*>::const_reverse_iterator const_reverse_iterator;
	~ObservableSet(){
		clear();
	}
	iterator begin(){return set_.begin();}
	iterator end(){return set_.end();}
	const_iterator begin()const{return set_.begin();}
	const_iterator end()const{return set_.end();}
	reverse_iterator rbegin(){return set_.rbegin();}
	reverse_iterator rend(){return set_.rend();}
	const_reverse_iterator rbegin()const{return set_.rbegin();}
	const_reverse_iterator rend()const{return set_.rend();}
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
		for(iterator it = set_.begin(); it != set_.end(); ++it)
			(*it)->removeObserver(this);
		set_.clear();
	}
	bool unlink(const Observable *o){
		iterator it = set_.find(static_cast<T*>(const_cast<Observable*>(o)));
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
/// A list (actually, vector) counterpart of ObservableSet.
/// Sometimes we want to use an ordered list of Observables that manages elements automatically.
template<typename T>
class ObservableList : public Observer{
	std::vector<T*> list_;
	ObservableList &operator=(ObservableList&){} ///< Prohibit copy construction.
public:
	typedef typename std::vector<T*>::iterator iterator;
	typedef typename std::vector<T*>::const_iterator const_iterator;
	typedef typename std::vector<T*>::reverse_iterator reverse_iterator;
	typedef typename std::vector<T*>::const_reverse_iterator const_reverse_iterator;
	~ObservableList(){
		clear();
	}
	iterator begin(){return list_.begin();}
	iterator end(){return list_.end();}
	const_iterator begin()const{return list_.begin();}
	const_iterator end()const{return list_.end();}
	reverse_iterator rbegin(){return list_.rbegin();}
	reverse_iterator rend(){return list_.rend();}
	const_reverse_iterator rbegin()const{return list_.rbegin();}
	const_reverse_iterator rend()const{return list_.rend();}
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
		for(iterator it = list_.begin(); it != list_.end(); ++it)
			(*it)->removeObserver(this);
		list_.clear();
	}
	bool unlink(const Observable *o){
		for(iterator it = list_.begin(); it != list_.end(); ++it){
			if(*it == o){
				// Once we encounter the reference we want to clear, the vector becomes
				// invalid.
				list_.erase(it);
				break; // TODO: clear multiple references to the same object in a list?
			}
		}
		return true;
	}
};


/// \brief A map of Observables that automatically removes expired elements.
/// \param T is key type, must be a subclass of and be static_cast-able from Observable.
/// \param R is value type, can be anything.
///
/// All occasions of item insertion are overridden so that all pointers to
/// Observables are managed to prevent dangling pointers.
///
/// \sa ObservableList, ObservableSet
template<typename T, typename R>
class ObservableMap : public Observer{
public:
	typedef std::map<T*,R> Map;
	typedef typename Map::iterator iterator;
	typedef typename Map::const_iterator const_iterator;
	typedef typename Map::reverse_iterator reverse_iterator;
	typedef typename Map::const_reverse_iterator const_reverse_iterator;
	typedef typename Map::value_type value_type;
	typedef typename Map::size_type size_type;
	~ObservableMap(){
		clear();
	}
	iterator begin(){return map.begin();}
	iterator end(){return map.end();}
	const_iterator begin()const{return map.begin();}
	const_iterator end()const{return map.end();}
	reverse_iterator rbegin(){return map.rbegin();}
	reverse_iterator rend(){return map.rend();}
	const_reverse_iterator rbegin()const{return map.rbegin();}
	const_reverse_iterator rend()const{return map.rend();}
	size_t size()const{return map.size();}
	bool empty()const{return map.empty();}
	iterator find(T *o){return map.find(o);}
	void insert(const value_type &v){
		v.first->addObserver(this);
		map.insert(v);
	}

	// We do not necessarily "own" the elements, so we can return non-const pointer
	// of elements even if this pointer is const.
	R &operator[](T* i){
		if(map.find(i) == map.end()){
			R ret = R();
			insert(value_type(i, ret));
		}
		return map[i];
	}

	void erase(iterator &it){
		if(it != map.end()){
			it->first->removeObserver(this);
			map.erase(it);
		}
	}
	size_type erase(T* t){
		// gcc does not allow unnamed temporary objects passed as reference.
		iterator it = find(t);
		// Now we have to return the value in VC9. The variable 'it' is not valid after erase().
		size_type ret = it != end();
		erase(it);
		return ret;
	}
	void clear(){
		for(iterator it = map.begin(); it != map.end(); ++it)
			it->first->removeObserver(this);
		map.clear();
	}

	virtual bool unlink(const Observable *o){
		iterator it = map.find(static_cast<T*>(o));
		if(it != map.end())
			map.erase(it);
		return true;
	}
protected:
	Map map;
private:
	ObservableMap &operator=(ObservableMap&){} ///< Prohibit copy construction.
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
