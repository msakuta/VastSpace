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
	hardpoint_static(Vec3d &apos, Quatd &arot, const char *aname, unsigned aflagmask) :
	pos(apos), rot(arot), name(aname), flagmask(aflagmask){}
};

class ArmBase : public Entity{
public:
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
	MTurret(Entity *abase, const hardpoint_static *hp);
	virtual const char *idname()const;
	virtual const char *classname()const;
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual void control(const input_t *, double dt);
	virtual void anim(double dt);
	virtual void postframe();
	virtual double hitradius();
	virtual cpplib::dstring descript()const;
protected:
	void tryshoot();
};

#if 0
const struct arms_static_info{
	const char *name;
	enum armsclass cls;
	unsigned flags;
	int maxammo;
	void (*draw)(arms_t *, ...);
	void (*drawtra)(arms_t *, ...);
	void (*anim)(arms_t *, entity_t *pt, struct hardpoint_static *hp, warf_t *w, double dt, int trigger);
	double cooldown;
	double emptyweight;
	double ammoweight;
	double emptyprice;
	double ammoprice;
	double ammodamage; /* To calculate damage rate */
} arms_static[];

glwindow *ArmsShowWindow(entity_t *creator(warf_t *), double baseprice, size_t armsoffset, arms_t *ret, struct hardpoint_static *hardpoints, int count);
#endif


#endif
