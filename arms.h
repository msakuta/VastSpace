#ifndef ARMS_H
#define ARMS_H
//#include "glwindow.h"
#include "entity.h"
#include "war.h"
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include <cpplib/dstring.h>


class ArmBase;

struct hardpoint_static : public Serializable{
	typedef Serializable st;
	virtual const char *classname()const;
	static const unsigned classid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);

	Vec3d pos; /* base position relative to object origin */
	Quatd rot; /* base rotation */
	const char *name;
	unsigned flagmask;
	hardpoint_static(){}
	hardpoint_static(Vec3d &apos, Quatd &arot, const char *aname, unsigned aflagmask) :
	pos(apos), rot(arot), name(aname), flagmask(aflagmask){}
	static hardpoint_static *load(const char *fname, int &num);
};

class ArmBase : public Entity{
public:
	typedef Entity st;
	Entity *base;
	Entity *target;
	const hardpoint_static *hp;
	int ammo;
	ArmBase(){}
	ArmBase(Entity *abase, const hardpoint_static *ahp) : base(abase), target(NULL), hp(ahp), ammo(0){}
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext&));
	virtual void draw(wardraw_t *wd) = 0;
	virtual void anim(double dt) = 0;
	virtual void postframe();
	virtual double hitradius()const;
	virtual Entity *getOwner();
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;
	virtual Props props()const;
	virtual cpplib::dstring descript()const;
	void align();
};

class MTurret : public ArmBase{
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

	MTurret(){}
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
	virtual double hitradius()const;
	virtual Props props()const;
	virtual cpplib::dstring descript()const;
	virtual bool command(EntityCommand *);

	virtual float reloadtime()const;
	virtual double bulletspeed()const;
	virtual float bulletlife()const;
protected:
	virtual void tryshoot();
};

class GatlingTurret : public MTurret{
public:
	typedef MTurret st;
	GatlingTurret();
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

class LTurretBase : public MTurret{
public:
	LTurretBase(){}
	LTurretBase(Entity *abase, const hardpoint_static *hp) : MTurret(abase, hp){}
};

class LTurret : public LTurretBase{
	float blowback, blowbackspeed;
public:
	typedef LTurretBase st;
	LTurret() : blowback(0), blowbackspeed(0){}
	LTurret(Entity *abase, const hardpoint_static *hp) : st(abase, hp), blowback(0), blowbackspeed(0){}
	virtual const char *classname()const;
	static const unsigned classid;
//	virtual void serialize(SerializeContext &sc);
//	virtual void unserialize(UnserializeContext &sc);
	virtual double hitradius()const;
	virtual void anim(double dt);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual float reloadtime()const;
	virtual float bulletlife()const;
	virtual void tryshoot();
	virtual double findtargetproc(const Entity *pb, const hardpoint_static *hp, const Entity *pt2);
};

class LMissileTurret : public LTurretBase{
	const Entity *targets[6]; // Targets already locked up
	float deploy; // Deployment factor.
protected:
	virtual int wantsFollowTarget()const;
public:
	typedef LTurretBase st;
	static const double bscale;
	LMissileTurret();
	LMissileTurret(Entity *abase, const hardpoint_static *hp);
	~LMissileTurret();
	virtual const char *classname()const;
	static const unsigned classid;
//	virtual void serialize(SerializeContext &sc);
//	virtual void unserialize(UnserializeContext &sc);
	virtual double hitradius()const;
	virtual void anim(double dt);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual double bulletspeed()const;
	virtual float reloadtime()const;
	virtual void tryshoot();
	virtual double findtargetproc(const Entity *pb, const hardpoint_static *hp, const Entity *pt2);
};


#endif
