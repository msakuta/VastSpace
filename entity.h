#ifndef ENTITY_H
#define ENTITY_H
#include "serial.h"
#include "war.h"
#include "popup.h"
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include <cpplib/dstring.h>
#include <vector>
#include <list>

class Bullet;
class Warpable;
class ArmBase;
class Builder;
class Docker;

class Entity : public Serializable{
public:
	typedef Serializable st;
	typedef Entity Dockable;
	typedef std::vector<cpplib::dstring> Props;
	Entity(WarField *aw = NULL);
	static Entity *create(const char *cname, WarField *w);
	virtual const char *idname()const;
	virtual const char *classname()const;
	virtual const char *dispname()const; // display name
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &));
	virtual double maxhealth()const;
	virtual void anim(double dt);
	virtual void postframe(); // gives an opportunity to clear pointers to objects being destroyed.
	virtual void control(const input_t *inputs, double dt);
	virtual unsigned analog_mask();
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual int numCockpits()const;
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
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
	virtual const ArmBase *armsGet(int index)const;
	virtual void attack(Entity *target);
	virtual Props props()const;
	virtual double getRU()const;
	virtual bool dock(Docker*);  // Returns if dockable for its own decision. Docking is so common operation that inheriting a class for that barely makes sense.
	virtual bool undock(Docker*); // Returns if undockable for its own reason.
	virtual void dockCommand(Docker* = NULL); // Instructs that try to a docker.

	void transform(Mat4d &mat){
		mat = Mat4d(mat4_u).translatein(pos) * rot.tomat4();
	}
	void transit_cs(CoordSys *destcs); // transit to a CoordSys from another, keeping absolute position and velocity.
	Entity *getUltimateOwner();

	Vec3d pos;
	Vec3d velo;
	Vec3d omg;
	Quatd rot; /* rotation expressed in quaternion */
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
	WarField *w; // belonging WarField, NULL means being bestroyed. Assigning another WarField marks it to transit to new CoordSys.
	int otflag;
//	char weapon;

	// Handles calls to virtual functions of a NULL ofject
	Builder *getBuilder(){return this ? this->getBuilderInt() : NULL;}
	Docker *getDocker(){return this ? this->getDockerInt() : NULL;}

	// Display a window that tells information about selected entity.
	static int cmd_property(int argc, char *argv[], void *pv);

protected:
	typedef std::map<std::string, Entity *(*)(WarField*)> EntityCtorMap;
	virtual void init();
	virtual Docker *getDockerInt();
	virtual Builder *getBuilderInt();
	static unsigned registerEntity(std::string name, Entity *(*)(WarField *));
	static EntityCtorMap &entityCtorMap();
	template<class T> static Entity *Constructor(WarField *w){
		return new T(w);
	};
};

inline Entity *Entity::getUltimateOwner(){
	Entity *ret;
	for(ret = this; ret && ret->getOwner(); ret = ret->getOwner());
	return ret;
}

struct GLwindowState;
void entity_popup(Entity *pt, GLwindowState &ws, int selectchain);

int estimate_pos(Vec3d &ret, const Vec3d &pos, const Vec3d &velo, const Vec3d &srcpos, const Vec3d &srcvelo, double speed, const WarField *w);

#endif
