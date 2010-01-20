#ifndef DESTROYER_H
#define DESTROYER_H
#include "Warpable.h"
#include "arms.h"

class Destroyer : public Warpable{
protected:
	ArmBase **turrets;
	static hardpoint_static *hardpoints;
	static int nhardpoints;
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
	virtual void draw(wardraw_t *wd);
	virtual double maxhealth()const;
	virtual double maxenergy()const;
	const maneuve &getManeuve()const;
};

#endif
