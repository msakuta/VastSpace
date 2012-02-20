#ifndef WAR_H
#define WAR_H
/// \brief Definition of WarField, WarSpace and its companion classes.
#include "serial_util.h"
#include "tent3d.h"
extern "C"{
#include <clib/colseq/color.h>
#include <clib/avec3.h>
#include <clib/amat3.h>
#include <clib/suf/suf.h>
#include <clib/rseq.h>
}
#include <cpplib/RandomSequence.h>
#include <cpplib/gl/cullplus.h>
#include <map>
#include <stddef.h> // offsetof

class Player;
class Viewer;
class CoordSys;
class Entity;
class Bullet;

class Observable;

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
class EXPORT Observable{
public:
	typedef std::map<Observer*, int> ObserverList;
	~Observable();
	void addObserver(Observer *);
	void removeObserver(Observer *);
protected:
	ObserverList observers;
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
		return ptr;
	}
};


/// \brief The object to contain input data from the player.
///
/// All player or AI's input should be expressed in this structure.
struct EXPORT input_t{
	unsigned press; /* bitfield for buttons being pressed */
	unsigned change; /* and changing state */
	int start; /* if the controlling is beggining */
	int end; /* if the controlling is terminating */
	double analog[4]; /* analog inputs, first and second elements are probably mouse movements. */
	input_t() : press(0), change(0), start(0), end(0){
		analog[0] = analog[1] = analog[2] = analog[3] = 0;
	}
};

struct contact_info{
	Vec3d normal;
	Vec3d velo; /* base velocity of colliding object */
	double depth; /* penetration depth */
};
struct otnt;

typedef struct WarDraw wardraw_t;
struct tent3d_line_list;

class WarSpace;
class Docker;
struct Message;

/// \brief A virtual world that can contain multiple Entities inside.
///
/// It does not necessarily have real 3-D space but can be merely a list of Entities.
/// The latter one is used for storing small ships inside bases or carriers.
///
/// It's usually tied together with a CoordSys.
class EXPORT WarField : public Serializable, public Observer{
public:

	/// \brief An internal class that unites behaviors of several types of ObservePtr's.
	///
	/// It's used as an iterator type for the Entity list.
	/// Because ObservePtr resolves the containing object statically, we cannot write a
	/// single 'for' loop to iterate all the objects in an Entity list without help of
	/// class like this. The type of the pointer to the first element of the list is
	/// different from the rest of the pointers in the list.
	/// A template function is another way to do it, but it will increase code size
	/// with little optimization.
	class UnionPtr{
		union{
			ObservePtr<WarField,0,Entity> *p0;
			ObservePtr<WarField,1,Entity> *p1;
			ObservePtr<Entity,0,Entity> *p2;
		};
		enum Type{P0,P1,P2} type;
	public:
		operator Entity*(){
			switch(type){
				case P0: return (*p0); break;
				case P1: return (*p1); break;
				case P2: return (*p2); break;
				default: assert(0); return NULL; break;
			}
		}
		Entity *operator->(){
			return operator Entity*();
		}
		UnionPtr(ObservePtr<WarField,0,Entity> &p0) : p0(&p0), type(P0){}
		UnionPtr(ObservePtr<WarField,1,Entity> &p1) : p1(&p1), type(P1){}
		UnionPtr(ObservePtr<Entity,0,Entity> &p2) : p2(&p2), type(P2){}
	};
	WarField();
	WarField(CoordSys *cs);
	virtual const char *classname()const;
	static const unsigned classid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void dive(SerializeContext &, void (Serializable::*)(SerializeContext &));
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual void postframe(); ///< Gives an opportunity to clear pointers to objects being destroyed
	virtual void endframe(); ///< Actually destructs invalid objects.
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual void drawOverlay(wardraw_t *);
	virtual bool pointhit(const Vec3d &pos, const Vec3d &velo, double dt, struct contact_info*)const;
	virtual Vec3d accel(const Vec3d &srcpos, const Vec3d &srcvelo)const;
	virtual double war_time()const;
	virtual struct tent3d_line_list *getTeline3d();
	virtual struct tent3d_fpol_list *getTefpol3d();
	Entity *addent(Entity *);
	Entity *entlist(){return el;}
	Player *getPlayer();
	virtual operator WarSpace*();
	virtual operator Docker*();
	virtual bool sendMessage(Message &);
	virtual bool unlink(Observable *);
	template<Entity *WarField::*list> int countEnts()const;
	int countBullets()const;

	CoordSys *cs; ///< redundant pointer to indicate belonging coordinate system
	Player *pl; ///< Player pointer
	ObservePtr<WarField,0,Entity> el; ///< Local Entity list
	ObservePtr<WarField,1,Entity> bl; ///< bullet list
	RandomSequence rs; ///< The pseudo-random number sequence generator local to this WarField.
	double realtime; ///< Time accumulator for this WarField. Some WarFields (or CoordSys') could have different progression of time.
};

class btRigidBody;
class btDiscreteDynamicsWorld;

/// \brief A real space that can contain multiple Entities and can simulate rigid-body dynamics.
///
/// All Entities belong to a WarField, but not necessarily WarSpace. Even though, majority of Entities
/// belong to a WarSpace.
///
/// The WarSpace class could be named "World" in normal gaming terminology, but the notable thing is that
/// there could be multiple instances of the class in a game.
/// WarSpace is a world in such ways that no two objects in separate WarSpaces could never directly interact.
class EXPORT WarSpace : public WarField{
	void init();
public:
	typedef WarField st;
	WarSpace();
	WarSpace(CoordSys *cs);
	virtual const char *classname()const;
	static const unsigned classid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
//	virtual void dive(SerializeContext &, void (Serializable::*)(SerializeContext &));
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual void endframe();
	virtual bool unlink(Observable *o);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual void drawOverlay(wardraw_t *);
	virtual struct tent3d_line_list *getTeline3d();
	virtual struct tent3d_fpol_list *getTefpol3d();
	virtual operator WarSpace*();
	virtual Quatd orientation(const Vec3d &pos)const;
	virtual btRigidBody *worldBody();

	struct tent3d_line_list *tell, *gibs;
	struct tent3d_fpol_list *tepl;
	int effects; /* trivial; telines or tefpols generated by this frame: to control performance */
	double soundtime;
	otnt *ot, *otroot;
	int ots, oti;

	btDiscreteDynamicsWorld *bdw;

	static int g_otdrawflags;
};


//-----------------------------------------------------------------------------
//     Implementation
//-----------------------------------------------------------------------------

template<>
inline size_t ObservePtr<WarField,0,Entity>::ofs(){return offsetof(WarField, el);}
template<>
inline size_t ObservePtr<WarField,1,Entity>::ofs(){return offsetof(WarField, bl);}


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
