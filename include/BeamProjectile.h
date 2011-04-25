/** \file
 * \brief Definition of BeamProjectile, a beam-looking projectile that travels quick and leave a tracer.
 */
#ifndef BEAMPROJECTILE_H
#define BEAMPROJECTILE_H
#include "Bullet.h"

class EXPORT BeamProjectile : public Bullet{
public:
	typedef Bullet st;
	struct tent3d_fpol *pf;
	BeamProjectile() : cs(NULL){}
	BeamProjectile(Entity *owner, float life, double damage, double radius = .01, Vec4<unsigned char> col = Vec4<unsigned char>(127,127,255,255), const color_sequence &cs = cs_beamtrail);
	virtual void enterField(WarField *);
	virtual void leaveField(WarField *);
	virtual void anim(double dt);
	virtual void drawtra(wardraw_t *);
	virtual double hitradius()const;

	const static color_sequence cs_beamtrail;
protected:
	double radius;
	Vec4<unsigned char> col;
	const color_sequence *cs;
};

#endif
