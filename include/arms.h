/** \file
 * \brief Definition of ArmBase derived classes.
 */
#ifndef ARMS_H
#define ARMS_H
//#include "glwindow.h"
#include "Entity.h"
#include "war.h"
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include <cpplib/dstring.h>


class ArmBase;

/// \brief A class that describes a hard point on a vehicle.
///
/// It consists of relative position and orientation of the hardpoint
/// from the origin of base vehicle.
struct EXPORT hardpoint_static : public Serializable{
	typedef Serializable st;
	virtual const char *classname()const;
	static const unsigned classid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);

	Vec3d pos; /* base position relative to object origin */
	Quatd rot; /* base rotation */
	const char *name;
	unsigned flagmask;
	hardpoint_static(Game *game) : st(game){}
	hardpoint_static(Vec3d &apos, Quatd &arot, const char *aname, unsigned aflagmask) :
	pos(apos), rot(arot), name(aname), flagmask(aflagmask){}
	static hardpoint_static *load(const char *fname, int &num);
};

/// \brief A class to prevent memory free error in an extension module.
///
/// It' merely a std::vector STL instance, but defined in the main executable
/// to free the contents in the destructor compiled with the same compiler as
/// the constructor's.
///
/// If we link the program with the standard C++ DLL rather than the static library,
/// this won't be necessary. But we need the static one for a reason; portability.
///
/// We could ensure the portability by distributing runtime DLL along with the
/// program, but I don't want it now.
class EXPORT HardPointList : public std::vector<hardpoint_static*>{
};


/// \brief Base class for all equippable modules on an Entity.
class EXPORT ArmBase : public Entity{
public:
	typedef Entity st;
	Entity *base;
	Entity *target;
	const hardpoint_static *hp;
	int ammo;
	ArmBase(Game *game) : st(game){}
	ArmBase(Entity *abase, const hardpoint_static *ahp) : st(abase->w), base(abase), target(NULL), hp(ahp), ammo(0){}
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext&));
	virtual void draw(wardraw_t *wd) = 0;
	virtual void anim(double dt) = 0;
	virtual void postframe();
	virtual double getHitRadius()const;
	virtual Entity *getOwner();
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;
	virtual Props props()const;
	virtual cpplib::dstring descript()const;
	void align();
};

/// \brief Medium-sized gun turret for spaceships.
class EXPORT MTurret : public ArmBase{
	static suf_t *suf_turret;
	static suf_t *suf_barrel;
protected:
	void findtarget(Entity *pb, const hardpoint_static *hp, const Entity *ignore_list[] = NULL, int nignore_list = 0);
	virtual double findtargetproc(const Entity *pb, const hardpoint_static *hp, const Entity *pt2); // returns precedence factor
	virtual int wantsFollowTarget()const; // Returns whether the turret should aim at target, 0 - no aim, 1 - yaw only, 2 - yaw and pitch, default 2.
public:
	typedef ArmBase st;
	float cooldown;
	float py[2]; // pitch and yaw
	float mf; // muzzle flash time
	bool forceEnemy;

	MTurret(Game *game) : st(game){}
	MTurret(Entity *abase, const hardpoint_static *hp);
	virtual const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual void control(const input_t *, double dt);
	virtual void anim(double dt);
	virtual void postframe();
	virtual double getHitRadius()const;
	virtual Props props()const;
	virtual cpplib::dstring descript()const;
	virtual bool command(EntityCommand *);

	virtual float reloadtime()const;
	virtual double bulletspeed()const;
	virtual float bulletlife()const;
protected:
	virtual void tryshoot();
};

/// \brief Middle-sized gatling gun turret for spaceships.
class EXPORT GatlingTurret : public MTurret{
public:
	typedef MTurret st;
	GatlingTurret(Game *game);
	GatlingTurret(Entity *abase, const hardpoint_static *hp);
	virtual const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual void anim(double dt);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *w);
	virtual float reloadtime()const;
	virtual double bulletspeed()const;
protected:
	virtual void tryshoot();
	static const Vec3d barrelpos;
	float barrelrot;
	float barrelomg;
};



#endif
