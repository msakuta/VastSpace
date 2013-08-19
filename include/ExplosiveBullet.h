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
	ExplosiveBullet(Entity *owner, float life, double damage) :
		st(owner, life, damage){}
	const char *classname()const override;
	void drawtra(WarDraw *wd)override;

protected:
	void bulletDeathEffect(int hitground, const struct contact_info *ci)override;
};

#endif
