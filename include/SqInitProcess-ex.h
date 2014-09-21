/** \file
 * \brief Extra SqInitProcess derived classes for convenience.
 */
#ifndef SQINITPROCESS_EX_H
#define SQINITPROCESS_EX_H

#include "SqInitProcess.h"
#include <cpplib/vec3.h>

#include <vector>

struct hardpoint_static;

/// This module tend to pollute the name space.
namespace sqinitproc{

typedef std::vector<gltestp::dstring> StringList;

/// \brief Processes a string value in a Squirrel script.
class EXPORT StringListProcess : public SqInitProcess{
public:
	StringList &value;
	const SQChar *name;
	bool mandatory;
	StringListProcess(StringList &value, const char *name, bool mandatory = true) : value(value), name(name), mandatory(mandatory){}
	virtual void process(HSQUIRRELVM)const;
};

/// \brief Processes single Vec3d with given variable name.
class EXPORT Vec3dProcess : public SqInitProcess{
public:
	Vec3d &vec;
	const SQChar *name;
	bool mandatory;
	Vec3dProcess(Vec3d &vec, const SQChar *name, bool mandatory = true) : vec(vec), name(name), mandatory(mandatory){}
	virtual void process(HSQUIRRELVM)const;
};

/// \brief Processes a list of Vec3d in a Squirrel script.
class EXPORT Vec3dListProcess : public SqInitProcess{
public:
	std::vector<Vec3d> &value;
	const SQChar *name;
	bool mandatory;
	Vec3dListProcess(std::vector<Vec3d> &value, const SQChar *name, bool mandatory = true) : value(value), name(name), mandatory(mandatory){}
	void process(HSQUIRRELVM)const override;
};



class EXPORT HardPointProcess : public SqInitProcess{
public:
	std::vector<hardpoint_static*> &hardpoints;
	HardPointProcess(std::vector<hardpoint_static*> &hardpoints) : hardpoints(hardpoints){}
	virtual void process(HSQUIRRELVM)const;
};

/// \brief Processes Squirrel callback function.
class EXPORT SqCallbackProcess : public SqInitProcess{
public:
	HSQOBJECT &value;
	const SQChar *name;
	bool mandatory;
	SqCallbackProcess(HSQOBJECT &value, const SQChar *name, bool mandatory = true) : value(value), name(name), mandatory(mandatory){}
	void process(HSQUIRRELVM v)const override;
};

/// Returns Squirrel's null object.
inline HSQOBJECT sq_nullobj(){
	HSQOBJECT ret;
	sq_resetobject(&ret);
	return ret;
}


}
using namespace sqinitproc;

#endif
