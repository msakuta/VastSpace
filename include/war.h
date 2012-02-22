#ifndef WAR_H
#define WAR_H
/// \brief Definition of WarField, WarSpace and its companion classes.
#include "serial_util.h"
#include "Observable.h"
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

class Player;
class Viewer;
class CoordSys;
class Entity;
class Bullet;



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
	typedef std::list<WeakPtr<Entity> > EntityList;

	/// \brief An internal class that unites behaviors of several types of ObservePtr's.
	///
	/// It's used as an iterator type for the Entity list.
	/// Because ObservePtr resolves the containing object statically, we cannot write a
	/// single 'for' loop to iterate all the objects in an Entity list without help of
	/// class like this. The type of the pointer to the first element of the list is
	/// different from the rest of the pointers in the list.
	/// A template function is another way to do it, but it will increase code size
	/// with little optimization.
#if 1
	typedef WeakPtr<Entity> &UnionPtr;
#else
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
#endif
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
	EntityList &entlist(){return el;}
	Player *getPlayer();
	virtual operator WarSpace*();
	virtual operator Docker*();
	virtual bool sendMessage(Message &);
	virtual bool unlink(Observable *);
	template<Entity *WarField::*list> int countEnts()const;
	int countBullets()const;

	CoordSys *cs; ///< redundant pointer to indicate belonging coordinate system
	Player *pl; ///< Player pointer
	EntityList el; ///< Local Entity list
	EntityList bl; ///< bullet list
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

inline SerializeStream &operator<<(SerializeStream &us, const WarField::EntityList &p){
	us << unsigned(p.size());
	for(WarField::EntityList::const_iterator it = p.begin(); it != p.end(); it++)
		us << *it;
	return us;
}

inline UnserializeStream &operator>>(UnserializeStream &us, WarField::EntityList &p){
	unsigned c;
	p.clear();
	us >> c;
	for(unsigned i = 0; i < c; i++){
		Entity *e;
		us >> e;
		p.push_back(e);
	}
	return us;
}

#endif
