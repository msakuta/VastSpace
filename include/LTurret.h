/** \file
 * \brief Definition of LTurretBase derived classes.
 */
#ifndef LTURRET_H
#define LTURRET_H
#include "arms.h"

class LTurretBase : public MTurret{
public:
	LTurretBase(Game *game) : MTurret(game){}
	LTurretBase(Entity *abase, const hardpoint_static *hp) : MTurret(abase, hp){}
};

class LTurret : public LTurretBase{
	float blowback, blowbackspeed;
public:
	typedef LTurretBase st;
	LTurret(Game *game) : st(game), blowback(0), blowbackspeed(0){}
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
protected:
	void shootEffect(Bullet *pz, const Vec3d &direction);
};

class LMissileTurret : public LTurretBase{
	const Entity *targets[6]; // Targets already locked up
	float deploy; // Deployment factor.
protected:
	virtual int wantsFollowTarget()const;
public:
	typedef LTurretBase st;
	static const double bscale;
	LMissileTurret(Game *game);
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
