#ifndef MISSILE_H
#define MISSILE_H
#include "bullet.h"
extern "C"{
#include "tent3d.h"
}

class Missile : public Bullet{
	struct tent3d_fpol *pf;
public:
	typedef Bullet st;
	Missile() : pf(NULL){}
	Missile(Entity *parent, float life, double damage);
	static const unsigned classid;
	virtual const char *classname()const;
	virtual void anim(double dt);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
};


#endif
