#ifndef ASSAULT_H
#define ASSAULT_H
#include "Frigate.h"

class Assault : public Frigate{
protected:
	static suf_t *sufbase;
	union{
		struct{
			ArmBase *turret0, *turret1, *turret2, *turret3;
		};
		ArmBase *turrets[4];
	};
	static const hardpoint_static hardpoints[];
public:
	typedef Frigate st;
	Assault(WarField *w);
	const char *idname()const;
	const char *classname()const;
	virtual void anim(double);
	virtual void postframe();
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual double maxhealth()const;
	virtual int armsCount()const;
	virtual const ArmBase *armsGet(int index)const;
	virtual void attack(Entity *target);
	friend class GLWarms;
};

#endif
