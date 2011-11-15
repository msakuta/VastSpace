/** \file
 * \brief Definition of Entity class, root of many actual object classes in the program.
 */
#ifndef ENTITY_H
#define ENTITY_H
#include "serial.h"
#include "war.h"
//#include "glw/popup.h"
#include "dstring.h"
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include <cpplib/dstring.h>
#include <vector>
#include <squirrel.h>



class btRigidBody;

class Bullet;
class Warpable;
class ArmBase;
class Builder;
class Docker;
struct EntityCommand;
class PopupMenu;

/// An abstract class to implement Player or AI controlling in the common way.
class EXPORT EntityController : public Serializable{
public:
	/// Called when this controller has to control an Entity.
	virtual bool control(Entity *, double dt) = 0;

	/// Called on destructor of the Entity being controlled.
	///
	/// Not all controller classes need to be destroyed, but some do. For example, the player could control
	/// entities at his own will, but not going to be deleted everytime it switches controlled object.
	/// On the other hand, Individual AI may be bound to and is destined to be destroyed with the entity.
	virtual bool unlink(Entity *);
};

/// Primary object in the space. Many object classes derive this.
/// Serializable and accessible from Squirrel codes.
class EXPORT Entity : public Serializable{
public:
	typedef Serializable st;
	typedef Entity Dockable;
	class EXPORT Props{
		gltestp::dstring str[32];
		int nstr;
	public:
		typedef gltestp::dstring *iterator;
		Props() : nstr(0){}
		void push_back(gltestp::dstring &);
		iterator begin();
		iterator end();
	};
//	typedef std::vector<gltestp::dstring> Props;
	Entity(WarField *aw = NULL);
	~Entity();
	static Entity *create(const char *cname, WarField *w);
	virtual const char *idname()const;
	virtual const char *classname()const;
	virtual const char *dispname()const; // display name
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &));
	virtual double maxhealth()const;
	virtual void enterField(WarField *enteringField); // Opportunity to response to WarField::addent.
	virtual void leaveField(WarField *leavingField); // Finally, we need this.
	virtual void setPosition(const Vec3d *pos, const Quatd *rot = NULL, const Vec3d *velo = NULL, const Vec3d *avelo = NULL); // Arguments can be NULL
	virtual void getPosition(Vec3d *pos, Quatd *rot = NULL, Vec3d *velo = NULL, Vec3d *avelo = NULL)const; // Arguments can be NULL
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual void postframe(); // gives an opportunity to clear pointers to objects being destroyed.
	virtual void control(const input_t *inputs, double dt);
	virtual unsigned analog_mask();
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual int numCockpits()const;
	virtual void draw(wardraw_t *); ///< Called for drawing opaque parts of this object.
	virtual void drawtra(wardraw_t *); ///< Called for drawing transparent parts of this object. Not Z-buffered nor lightened.
	virtual void drawHUD(wardraw_t *); ///< Called if this Entity is cockpit-viewed.  Drawn over everything but GLwindows.
	virtual void drawOverlay(wardraw_t *); ///< Called everytime, but not model-transformed and drawn over everything but GUI.
	virtual bool solid(const Entity *)const; // Sometimes hit check must be suppressed to prevent things from stacking. Hit check is enabled only if both objects claims solidity each other.
	virtual double hitradius()const = 0; // The object's outermost hitting sphere radius, used for collision checking and object scale estimation.
	virtual void bullethit(const Bullet *);
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal); // return nonzero on hit
	virtual int takedamage(double damage, int hitpart); /* return 0 on death */
	virtual void bullethole(sufindex, double rad, const Vec3d &pos, const Quatd &rot);
	virtual int popupMenu(PopupMenu &list);
	virtual Warpable *toWarpable();
	virtual Dockable *toDockable();
	virtual Entity *getOwner();
	virtual bool isTargettable()const = 0;
	virtual bool isSelectable()const;
	virtual int armsCount()const;
	virtual ArmBase *armsGet(int index);
	const ArmBase *armsGet(int index)const{return const_cast<Entity*>(this)->armsGet(index);}
	virtual Props props()const;
	virtual double getRU()const;
	virtual bool dock(Docker*);  // Returns if dockable for its own decision. Docking is so common operation that inheriting a class for that barely makes sense.
	virtual bool undock(Docker*); // Returns if undockable for its own reason.
	virtual bool command(EntityCommand *); // A general-purpose command dispatcher. Can have arbitrary argument via virtual class.

	// Assigns transformation matrix to argument object.
	void transform(Mat4d &mat)const{
		mat = Mat4d(mat4_u).translatein(pos) * rot.tomat4();
	}
	void transit_cs(CoordSys *destcs); // transit to a CoordSys from another, keeping absolute position and velocity.
	Entity *getUltimateOwner();

	Vec3d pos; // Position vector
	Vec3d velo; // Linear velocity vector
	Vec3d omg; // Angular velocity vector
	Quatd rot; // rotation expressed in quaternion
	double mass; /* [kg] */
	double moi;  /* moment of inertia, [kg m^2] should be a tensor */
//	double turrety, barrelp;
//	double desired[2];
	double health;
//	double cooldown, cooldown2;
	Entity *next;
	Entity *selectnext; /* selection list */
	Entity *enemy;
	int race;
//	int shoots, shoots2, kills, deaths;
	input_t inputs;
	EntityController *controller; /// The controller class, could be either a player or an AI.
	WarField *w; // belonging WarField, NULL means being bestroyed. Assigning another WarField marks it to transit to new CoordSys.
	int otflag;
//	char weapon;

	// Handles calls to virtual functions of a NULL object
	Builder *getBuilder(){return this ? this->getBuilderInt() : NULL;}
	Docker *getDocker(){return this ? this->getDockerInt() : NULL;}

	// Display a window that tells information about selected entity.
	static int cmd_property(int argc, char *argv[], void *pv);

	class EXPORT EntityStatic{
	public:
		virtual ClassId classid() = 0;
		virtual Entity *create(WarField*) = 0; ///< Allocate and construct a new object as a member of a WarField.
		virtual Entity *stcreate() = 0; ///< Allocate and construct empty object for use in unserialization.
		virtual void destroy(Entity*) = 0; ///< Destruct and deallocate
		virtual const SQChar *sq_classname() = 0; ///< Squirrel class name.
		virtual bool sq_define(HSQUIRRELVM) = 0; ///< Defines Squirrel bound class.
		virtual EntityStatic *st() = 0; ///< Super type
		EntityStatic(ClassId);
	};

	/// Entity class static list is typedefed in public privilege.
	typedef std::map<ClassId, EntityStatic*> EntityCtorMap;
	typedef const EntityCtorMap ConstEntityCtorMap;

	/// Public interface to query all list of Entity classes.
	static const EntityCtorMap &constEntityCtorMap(){return entityCtorMap();}

protected:
	btRigidBody *bbody;

	virtual void init();
	virtual Docker *getDockerInt();
	virtual Builder *getBuilderInt();
	static unsigned registerEntity(ClassId name, EntityStatic *st);
	static EntityCtorMap &entityCtorMap();

	template<class T> class EntityRegister : public EntityStatic{
		const SQChar *m_sq_classname;
		ClassId m_classid;
		virtual ClassId classid(){ return m_classid; }
		virtual Entity *create(WarField *w){ return new T(w); }
		virtual Entity *stcreate(){ return new T(); }
		virtual void destroy(Entity *p){ delete p; }
		virtual const SQChar *sq_classname(){ return m_sq_classname; }
		virtual bool sq_define(HSQUIRRELVM v){
			sq_pushstring(v, sq_classname(), -1);
			sq_pushstring(v, T::st::entityRegister.sq_classname(), -1);
			sq_get(v, 1);
			sq_newclass(v, SQTrue);
			sq_settypetag(v, -1, SQUserPointer(m_classid));
			sq_createslot(v, -3);
			return true;
		}
		virtual EntityStatic *st(){ return &T::st::entityRegister; }
	public:
		EntityRegister(const SQChar *classname) : EntityStatic(classname), m_sq_classname(classname), m_classid(classname){
			Entity::registerEntity(classname, this);
		}
		EntityRegister(const SQChar *classname, ClassId classid) : EntityStatic(classname), m_sq_classname(classname), m_classid(classid){
			Entity::registerEntity(classname, this);
		}
	};

	class EXPORT EntityStaticBase : public EntityStatic{
	public:
		EntityStaticBase();
		virtual ClassId classid();
		virtual Entity *create(WarField *w);
		virtual Entity *stcreate();
		virtual void destroy(Entity *p);
		virtual const SQChar *sq_classname();
		virtual bool sq_define(HSQUIRRELVM v);
		virtual EntityStatic *st();
	};
	static EntityStaticBase entityRegister;
};

inline Entity *Entity::getUltimateOwner(){
	Entity *ret;
	for(ret = this; ret && ret->getOwner(); ret = ret->getOwner());
	return ret;
}

struct GLwindowState;
void entity_popup(Entity *pt, GLwindowState &ws, int selectchain);

int EXPORT estimate_pos(Vec3d &ret, const Vec3d &pos, const Vec3d &velo, const Vec3d &srcpos, const Vec3d &srcvelo, double speed, const WarField *w);


#endif
