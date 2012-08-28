#ifndef GETFOV_H
#define GETFOV_H

#include "EntityCommand.h"

struct EXPORT GetFovCommand : EntityCommand{
	COMMAND_BASIC_MEMBERS(GetFovCommand, EntityCommand);
	double fov;
};

#endif
