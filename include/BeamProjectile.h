/** \file
 * \brief Definition of BeamProjectile, a beam-looking projectile that travels quick and leave a tracer.
 */
#ifndef BEAMPROJECTILE_H
#define BEAMPROJECTILE_H
#include "Bullet.h"
#include "tefpol3d.h"

/// \brief A fast bolt of particle beam with trailing beam effect.
class EXPORT BeamProjectile : public Bullet{
public:
	typedef Bullet st;
	static const unsigned classid;
	Tefpol *pf;
	BeamProjectile(Game *game);
	BeamProjectile(Entity *owner, float life, double damage, double radius = 10., Vec4<unsigned char> col = Vec4<unsigned char>(127,127,255,255), const color_sequence &cs = cs_beamtrail, double getHitRadius = 0.);
	~BeamProjectile();
	virtual const char *classname()const;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void enterField(WarField *);
	virtual void leaveField(WarField *);
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual void drawtra(wardraw_t *);
	virtual double getHitRadius()const;

	const static color_sequence cs_beamtrail;
protected:
	double radius;
	double m_hitradius;
	Vec4<unsigned char> col;
	double bands; ///< Remaining expanding disc effect (bands) this projectile can generate.
	const color_sequence *cs;
	static const int maxBands = 10; ///< Maximum number of band effects a projectile can generate.
};

#endif
