#ifndef MISSILE_H
#define MISSILE_H
#include "Bullet.h"
#include "tent3d.h"

class Missile : public Bullet{
	struct tent3d_fpol *pf;
	double integral; /* integrated term */
	double def[2]; /* integrated deflaction of each axes */
	double ft; /* fpol time */
	float fuel, throttle;
	bool centered; /* target must be centered before integration starts */
	static const float maxfuel;
	void steerHoming(double dt, const Vec3d &atarget, const Vec3d &targetvelo, double speedfactor, double minspeed);
	void unlinkTarget();
public:
	typedef Bullet st;
	Missile(Game *game) : st(game), pf(NULL), ft(0), fuel(maxfuel), throttle(0){}
	Missile(Entity *parent, float life, double damage, Entity *target = NULL);
	~Missile();
	static const unsigned classid;
	virtual const char *classname()const;
	virtual void anim(double dt);
	virtual void postframe();
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual double hitradius()const;
	static const double maxspeed;
	Missile *targetlist;
	static std::map<const Entity *, Missile *> targetmap;
};


#endif
