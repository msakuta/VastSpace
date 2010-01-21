#ifndef DESTROYER_H
#define DESTROYER_H
#include "Warpable.h"
#include "arms.h"

class Destroyer : public Warpable{
protected:
	ArmBase **turrets;
	static hardpoint_static *hardpoints;
	static int nhardpoints;
	static struct hitbox hitboxes[];
	static const int nhitboxes;
public:
	typedef Warpable st;
	Destroyer(){}
	Destroyer(WarField *w);
	void init();
	static const unsigned classid;
	virtual const char *classname()const;
	virtual const char *dispname()const;
	virtual double hitradius();
	virtual void anim(double dt);
	virtual void postframe();
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn);
	virtual void draw(wardraw_t *wd);
	virtual double maxhealth()const;
	virtual double maxenergy()const;
	const maneuve &getManeuve()const;
};

#endif
