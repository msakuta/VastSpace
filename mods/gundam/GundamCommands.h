/** \file
 * \brief Definition of common commands specific to Gundam extension module.
 */
#ifndef GUNDAM_GUNDAMCOMMANDS_H
#define GUNDAM_GUNDAMCOMMANDS_H

#include "EntityCommand.h"

DERIVE_COMMAND_EXT_ADD(TransformCommand, EntityCommand, int formid);

DERIVE_COMMAND_EXT_ADD(WeaponCommand, EntityCommand, int weaponid);

DERIVE_COMMAND_EXT_ADD(StabilizerCommand, EntityCommand, int stabilizer);

DERIVE_COMMAND_EXT_ADD(GetCoverCommand, EntityCommand, int v);

DERIVE_COMMAND_EXT(ReloadCommand, EntityCommand);

#endif
