/** \file
 * \brief Defines Docker class.
 */
#ifndef DOCKER_H
#define DOCKER_H
#include "war.h"
#include "Entity.h"
#ifndef DEDICATED
#include "glw/glwindow.h"
#endif
#include <squirrel.h>

/// The base class for those ships that can contain other ships.
/// It derives WarField for they have similarities about containing Entities, but Docker
/// does not maintain position of each Entity.
class EXPORT Docker : public WarField, public Observable{
public:
	typedef WarField st;
	typedef Entity::Dockable Dockable;
	enum ShipClass{
		Fighter, Frigate, Destroyer, num_ShipClass
	};

	Entity *e; // The pointed Entity is always resident when a Docker object exists. Propagating functions and destructor is always invoked from the Entity's.
	double baycool;
	EntityList undockque;
	int paradec[num_ShipClass];
	bool remainDocked;

	Docker(Entity *ae);
	~Docker();
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &));
	void anim(double dt);
	virtual void dock(Dockable *);
	bool postUndock(Dockable *); // Posts an entity to undock queue.
	int enumParadeC(enum ShipClass sc){return paradec[sc]++;}
	virtual Entity *addent(Entity*);
	virtual operator Docker*();
	virtual bool undock(Dockable *);
	virtual Vec3d getPortPos()const = 0; ///< Retrieves position to dock to
	virtual Quatd getPortRot()const = 0; ///< Retrieves rotation of the port


	static void init();

	/// Creates and pushes an Entity object to Squirrel stack.
	static void sq_pushobj(HSQUIRRELVM, Docker *);

	/// Returns an Entity object being pointed to by an object in Squirrel stack.
	static Docker *sq_refobj(HSQUIRRELVM, SQInteger idx = 1);

	static bool sq_define(HSQUIRRELVM v); ///< Define Squirrel class
protected:
	static SQInteger sqf_get(HSQUIRRELVM v);
	static SQInteger sqf_addent(HSQUIRRELVM v);
};

#ifndef DEDICATED
class GLWdock : public GLwindowSizeable{
public:
	typedef GLwindowSizeable st;
	GLWdock(const char *title, Docker *a) : st(title), docker(a), grouping(false){
		flags |= GLW_CLOSE | GLW_COLLAPSABLE;
		xpos = 100;
		ypos = 200;
		width = 250;
		height = 100;
	}
	virtual void draw(GLwindowState &ws, double t);
	virtual int mouse(GLwindowState &ws, int button, int state, int x, int y);
	virtual void postframe();

	Docker *docker;
	bool grouping;
};
#endif


#endif
