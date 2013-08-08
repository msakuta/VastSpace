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
	ExplosiveBullet(Game *game) : st(game){}
	ExplosiveBullet(Entity *owner, float life, double damage) :
		st(owner, life, damage){}
	void drawtra(WarDraw *wd)override;

protected:
	void bulletDeathEffect(int hitground, const struct contact_info *ci)override;
};

#endif
