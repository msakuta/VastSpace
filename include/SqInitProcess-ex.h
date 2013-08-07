/** \file
 * \brief Extra SqInitProcess derived classes for convenience.
 */
#ifndef SQINITPROCESS_EX_H
#define SQINITPROCESS_EX_H

#include "SqInitProcess.h"
#include <cpplib/vec3.h>

#include <vector>

/// \brief Processes a list of Vec3d in a Squirrel script.
class EXPORT Vec3dListProcess : public SqInitProcess{
public:
	std::vector<Vec3d> &value;
	const SQChar *name;
	bool mandatory;
	Vec3dListProcess(std::vector<Vec3d> &value, const SQChar *name, bool mandatory = true) : value(value), name(name), mandatory(mandatory){}
	void process(HSQUIRRELVM)const override;
};


#endif
