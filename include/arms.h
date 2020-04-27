/** \file
 * \brief Definition of ArmBase derived classes.
 */
#ifndef ARMS_H
#define ARMS_H
//#include "glwindow.h"
#include "HardpointStatic.h"
#include "Entity.h"
#include "war.h"
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include <cpplib/dstring.h>


class ArmBase;


/// \brief Base class for all equippable modules on an Entity.
class EXPORT ArmBase : public Entity{
public:
	typedef Entity st;
	WeakPtr<Entity> base; ///< Mounted platform object. Can be destroyed before this ArmBase itself.
	WeakPtr<Entity> target; ///< Targetted enemy.
	const hardpoint_static *hp;
	int ammo;
	bool online; ///< Online-ness is different thing from active-ness. Offline ArmBases do not function but graphics preset.
	ArmBase(Game *game) : st(game){}
	ArmBase(Entity *abase, const hardpoint_static *ahp) : st(abase->w), base(abase), target(NULL), hp(ahp), ammo(0), online(true){}
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext&));
	virtual void draw(wardraw_t *wd) = 0;
	virtual void anim(double dt) = 0;
	virtual void clientUpdate(double dt);
	virtual double getHitRadius()const;
	virtual Entity *getOwner();
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;
	virtual Props props()const;
	virtual gltestp::dstring descript()const;
	void align();
};

#endif
