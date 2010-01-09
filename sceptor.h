#ifndef SCEPTOR_H
#define SCEPTOR_H

#include "beamer.h"
#include "coordsys.h"
#include "war.h"
#include "arms.h"
#include "shield.h"
extern "C"{
#include <clib/avec3.h>
#include <clib/suf/sufdraw.h>
}

class Builder;
class Docker;

class Dockable : public Entity{
public:
	typedef Entity st;
	Dockable(){}
	Dockable(WarField *aw) : st(aw){}
	virtual void dock(Docker *);
	virtual void undock(Docker *);
protected:
	Docker *docker;
	friend class Docker;
};

// Space Interceptor (small fighter)
class Sceptor : public Dockable{
public:
	typedef Dockable st;
protected:
	enum Task;
	Vec3d aac; /* angular acceleration */
	double thrusts[3][2]; /* 3 pairs of thrusters, 2 directions each */
	double throttle;
	double fuel;
	double cooldown;
	Vec3d dest;
	float fcloak;
	float heat;
	struct tent3d_fpol *pf;
	Docker *mother; // Mother ship
	int hitsound;
	int paradec;
	Task task;
	bool docked, returning, away, cloak;
	float mf; // trivial muzzle flashes

	static const avec3_t gunPos[2];
	void shootDualGun(double dt);
	bool findEnemy(); // Finds the nearest enemy
public:
	Sceptor();
	Sceptor(WarField *aw);
	virtual const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual double maxhealth()const;
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void anim(double dt);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *w);
	virtual int takedamage(double damage, int hitpart);
	virtual void postframe();
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;
	virtual double hitradius();
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal);
	virtual int popupMenu(PopupMenu &);
	virtual Props props()const;
	virtual void undock(Docker *);
	virtual double maxfuel()const;
	static hitbox hitboxes[];
	static const int nhitboxes;
	static Entity *create(WarField *w, Builder *mother);
};


#endif
