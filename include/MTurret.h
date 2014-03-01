/** \file
 * \brief Definition of MTurret and its derived classes.
 */
#ifndef MTURRET_H
#define MTURRET_H
#include "arms.h"

/// \brief Medium-sized gun turret for spaceships.
class EXPORT MTurret : public ArmBase{
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

	static EntityRegister<MTurret> entityRegister;
	MTurret(Game *game) : st(game){}
	MTurret(Entity *abase, const hardpoint_static *hp);
	EntityStatic &getStatic()const{return entityRegister;}
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual void control(const input_t *, double dt);
	virtual void anim(double dt);
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



//-----------------------------------------------------------------------------
//    Implementation for MTurret
//-----------------------------------------------------------------------------


/// Instantiate the constructor for MTurret to prevent creating Entity alone.
template<> Entity *Entity::EntityRegister<MTurret>::create(WarField *){
	return NULL;
}

#endif
