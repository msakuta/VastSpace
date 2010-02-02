#ifndef MISSILE_H
#define MISSILE_H
#include "bullet.h"

class Missile : public Bullet{
public:
	typedef Bullet st;
	Missile(){}
	Missile(Entity *parent, float life, double damage) : st(parent, life, damage){}
	static const unsigned classid;
	virtual const char *classname()const;
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
};


#endif
