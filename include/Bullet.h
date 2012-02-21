/** \file
 * \brief Definition of Bullet class.
 */
#ifndef BULLET_H
#define BULLET_H
#include "Entity.h"

/// \brief Exact implementation of a weak pointer to an Observable object.
///
/// That's a re-invention of wheels.
/// Deriving Observer class wouldn't be necessary.
/// Probably we should use boost library or C++11's std::weak_ptr.
template<typename TP>
class EXPORT ObservePtr2 : public Observer{
	TP *ptr;
public:
	ObservePtr2(TP *o = NULL) : ptr(o){
		if(o)
			o->addObserver(this);
	}
	~ObservePtr2(){
		if(ptr)
			ptr->removeObserver(this);
	}
	ObservePtr2 &operator=(TP *o){
		if(ptr)
			ptr->removeObserver(this);
		if(o)
			o->addObserver(this);
		ptr = o;
		return *this;
	}
	void unlinkReplace(TP *o){
		if(o)
			o->addObserver(this);
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
	bool unlink(Observable *o){
		if(o == ptr)
			ptr = NULL;
		return true;
	}
};


/// \brief An Entity representing traveling projectile, damages another Entity if hit.
///
/// This object is a point object; it has no size.
/// But derived classes are not necessarily point Entities.
///
/// Extension modules can derive and define new type of projectiles, such as guided missiles.
///
/// Default appearance is plain orange beam, which has width depending on damage.
class EXPORT Bullet : public Entity{
public:
	typedef Entity st;

	double damage; ///< Damaging amount
	float life; ///< Entity is removed after this value runs out
	float runlength; ///< Run length since creation. Note that it's not age after creation.
	ObservePtr2<Entity> owner; ///< Owner of this Entity. If this projectile kills something, this event is reported to the owner.
	bool grav; ///< Flag to enable gravity.

	Bullet(){}
	Bullet(Entity *owner, float life, double damage);
	virtual const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual double hitradius()const;
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual void postframe();
	virtual void drawtra(wardraw_t *);
	virtual Entity *getOwner();
	virtual bool isTargettable()const;
protected:
	void bulletkill(int hitground, const struct contact_info *ci);
};


//-----------------------------------------------------------------------------
//    Inline Implementation
//-----------------------------------------------------------------------------

template<typename TP>
inline SerializeStream &operator<<(SerializeStream &us, const ObservePtr2<TP> &p){
	TP *a = p;
	us << a;
	return us;
}

template<typename TP>
inline UnserializeStream &operator>>(UnserializeStream &us, ObservePtr2<TP> &p){
	TP *a;
	us >> a;
	p = a;
	return us;
}


#endif
