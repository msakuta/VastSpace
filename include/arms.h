/** \file
 * \brief Definition of ArmBase derived classes.
 */
#ifndef ARMS_H
#define ARMS_H
//#include "glwindow.h"
#include "Entity.h"
#include "war.h"
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include <cpplib/dstring.h>


class ArmBase;

/// \brief A class that describes a hard point on a vehicle.
///
/// It consists of relative position and orientation of the hardpoint
/// from the origin of base vehicle.
struct EXPORT hardpoint_static : public Serializable{
	typedef Serializable st;
	virtual const char *classname()const;
	static const unsigned classid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);

	Vec3d pos; /* base position relative to object origin */
	Quatd rot; /* base rotation */
	const char *name;
	unsigned flagmask;
	hardpoint_static(Game *game) : st(game){}
	hardpoint_static(Vec3d &apos, Quatd &arot, const char *aname, unsigned aflagmask) :
	pos(apos), rot(arot), name(aname), flagmask(aflagmask){}
	static hardpoint_static *load(const char *fname, int &num);
};

/// \brief A class to prevent memory free error in an extension module.
///
/// It' merely a std::vector STL instance, but defined in the main executable
/// to free the contents in the destructor compiled with the same compiler as
/// the constructor's.
///
/// If we link the program with the standard C++ DLL rather than the static library,
/// this won't be necessary. But we need the static one for a reason; portability.
///
/// We could ensure the portability by distributing runtime DLL along with the
/// program, but I don't want it now.
class EXPORT HardPointList : public std::vector<hardpoint_static*>{
};


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
