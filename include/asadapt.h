/// \file
/// \brief AngelScript library adapter for gltestplus project.
#ifndef ASADAPT_H
#define ASADAPT_H
#include "export.h"
#include "dstring.h"
#include <stddef.h>

#include "../angelscript/angelscript/include/angelscript.h"


#include <map>

class Game;
class Serializable;
class Entity;

/// Namespace for AngelScript adaption functions and classes.
namespace asa{

EXPORT asIScriptEngine *ASAdaptInit(Game *game, asIScriptEngine *v = NULL);

}

using namespace asa;

#endif
