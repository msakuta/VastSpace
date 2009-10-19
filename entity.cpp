extern "C"{
#include <clib/aquat.h>
}
#include "entity.h"

const char *Entity::idname()const{
	return "entity";
}

const char *Entity::classname()const{
	return "Entity";
}
