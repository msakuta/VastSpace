/** \file
 * \brief Definition of ExplosiveBullet class.
 */
#ifndef EXPLOSIVEBULLET_H
#define EXPLOSIVEBULLET_H
#include "Bullet.h"

/// \brief A Bullet with explosive charge.
class EXPORT ExplosiveBullet : public Bullet{
public:
	typedef Bullet st;
	static EntityRegister<ExplosiveBullet> entityRegister;
	ExplosiveBullet(Game *game) : st(game){}
	ExplosiveBullet(WarField *w);
	ExplosiveBullet(Entity *owner, float life, double damage, double splashRadius = 0, bool friendlySafe = false) :
		st(owner, life, damage), splashRadius(splashRadius), friendlySafe(friendlySafe){}
	const char *classname()const override;
	void drawtra(WarDraw *wd)override;

protected:
	SQInteger sqGet(HSQUIRRELVM v, const SQChar *name)const override;
	SQInteger sqSet(HSQUIRRELVM v, const SQChar *name)override;
	bool bulletHit(Entity *pt, WarSpace *ws, otjEnumHitSphereParam &param)override;
	void bulletDeathEffect(int hitground, const struct contact_info *ci)override;

	double splashRadius; ///< Splash radius
	bool friendlySafe; ///< If this explosion can hurt friendly Entities.
};

#endif
