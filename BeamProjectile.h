/** \file
 * \brief Definition of BeamProjectile, a beam-looking projectile that travels quick and leave a tracer.
 */
#ifndef BEAMPROJECTILE_H
#define BEAMPROJECTILE_H
#include "Bullet.h"

class BeamProjectile : public Bullet{
public:
	typedef Bullet st;
	struct tent3d_fpol *pf;
	BeamProjectile(){}
	BeamProjectile(Entity *owner, float life, double damage);
	virtual void enterField(WarField *);
	virtual void leaveField(WarField *);
	virtual void anim(double dt);
	virtual void drawtra(wardraw_t *);
	virtual double hitradius()const;
};

#endif
