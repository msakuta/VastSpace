#ifndef HARDPOINT_STATIC_H
#define HARDPOINT_STATIC_H

#include "serial.h"

#include <cpplib/vec3.h>
#include <cpplib/quat.h>

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

#endif
