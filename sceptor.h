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

class Sceptor : public Entity{
public:
	typedef Entity st;
protected:
	enum task;
	Vec3d aac; /* angular acceleration */
	double thrusts[3][2]; /* 3 pairs of thrusters, 2 directions each */
	double throttle;
	double fuel;
	double cooldown;
	Vec3d dest;
	float fcloak;
	float heat;
	struct tent3d_fpol *pf;
//	scarry_t *mother;
	int hitsound;
	int paradec;
	enum task task;
	bool docked, returning, away, cloak;
	void shootDualGun(double dt);
public:
	Sceptor(WarField *aw);
	virtual const char *idname()const;
	virtual const char *classname()const;
	virtual double maxhealth()const;
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void anim(double dt);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *w);
	virtual int takedamage(double damage, int hitpart);
	virtual void postframe();
	virtual bool isTargettable()const;
	virtual double hitradius();
	virtual int popupMenu(char ***const, int **, char ***cmds, int *num);
};


#endif
