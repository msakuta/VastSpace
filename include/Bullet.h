/** \file
 * \brief Definition of Bullet class.
 */
#ifndef BULLET_H
#define BULLET_H
#include "Entity.h"
#include "EntityRegister.h"

struct otjEnumHitSphereParam;
struct contact_info;

/// \brief An Entity representing traveling projectile, damages another Entity if hit.
///
/// This object is a point object; it has no size.
/// But derived classes are not necessarily point Entities.
///
/// Extension modules can derive and define new type of projectiles, such as guided missiles.
///
/// Default appearance is plain orange beam, which has width depending on damage.
class EXPORT Bullet : public Entity{
public:
	typedef Entity st;
protected:
	double damage; ///< Damaging amount
	float life; ///< Entity is removed after this value runs out
	float runlength; ///< Run length since creation. Note that it's not age after creation.
	WeakPtr<Entity> owner; ///< Owner of this Entity. If this projectile kills something, this event is reported to the owner.
	bool grav; ///< Flag to enable gravity.
	bool active; ///< Client-side flag indicating if this object is alive in the client.
public:
	Bullet(Game *game) : st(game), active(true){}
	Bullet(WarField *w);
	Bullet(Entity *owner, float life, double damage);
	static EntityRegister<Bullet> entityRegister;
	EntityStatic &getStatic()const override;
	static const unsigned classid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual double getHitRadius()const;
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual void postframe();
	virtual void drawtra(wardraw_t *);
	virtual Entity *getOwner();
	virtual bool isTargettable()const;

	double getDamage()const{return damage;}
protected:
	virtual SQInteger sqGet(HSQUIRRELVM v, const SQChar *name)const override;
	virtual SQInteger sqSet(HSQUIRRELVM v, const SQChar *name)override;

	virtual void bulletDeathEffect(int hitground, const contact_info *ci);
	virtual bool bulletHit(Entity *pt, WarSpace *ws, otjEnumHitSphereParam &param);
	void explosionEffect(const contact_info *ci);
};


#endif
