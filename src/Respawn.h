#ifndef RESPAWN_H
#define RESPAWN_H
#include "war.h"
#include "Entity.h"

class Respawn : public Entity{
	double timer;
	double interval;
	cpplib::dstring creator;
	int count;
public:
	typedef Entity st;
	Respawn(){}
	Respawn(WarField *w, double interval, double initial_phase, int max_count, const char *childClassName);
	static const unsigned classid;
	virtual const char *classname()const;
	virtual double hitradius()const;
	virtual bool isTargettable()const;
	virtual void anim(double dt);
};

#endif

