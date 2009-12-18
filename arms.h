#ifndef ARMS_H
#define ARMS_H
#include "glwindow.h"
#include "entity.h"
#include "war.h"
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include <cpplib/dstring.h>


class ArmBase;

struct hardpoint_static{
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
	ArmBase(Entity *abase, const hardpoint_static *ahp) : base(abase), target(NULL), hp(ahp), ammo(0){}
	virtual void draw(wardraw_t *wd) = 0;
	virtual void anim(double dt) = 0;
	virtual void postframe();
	virtual double hitradius();
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
	void findtarget(Entity *pb, const hardpoint_static *hp);
public:
	typedef ArmBase st;
	float cooldown;
	float py[2]; // pitch and yaw
	float mf; // muzzle flash time
	bool forceEnemy;
	MTurret(Entity *abase, const hardpoint_static *hp);
	virtual const char *idname()const;
	virtual const char *classname()const;
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual void control(const input_t *, double dt);
	virtual void anim(double dt);
	virtual void postframe();
	virtual double hitradius();
	virtual void attack(Entity *target);
	virtual std::vector<cpplib::dstring> props()const;
	virtual cpplib::dstring descript()const;

	virtual float reloadtime()const;
protected:
	virtual void tryshoot();
};

class GatlingTurret : public MTurret{
public:
	typedef MTurret st;
	GatlingTurret(Entity *abase, const hardpoint_static *hp);
	virtual const char *idname()const;
	virtual const char *classname()const;
	virtual void anim(double dt);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *w);
	virtual float reloadtime()const;
protected:
	virtual void tryshoot();
	static const Vec3d barrelpos;
	float barrelrot;
	float barrelomg;
};


#endif
