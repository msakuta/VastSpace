extern "C"{
#include <clib/aquat.h>
}
#include "entity.h"
#include "beamer.h"

template<class T> Entity *Constructor(){
	return new T();
};

static const char *ent_name[] = {
	"beamer",
};
static Entity *(*const ent_creator[])() = {
	Constructor<Beamer>,
};

Entity *Entity::create(const char *cname){
	int i;
	for(i = 0; i < numof(ent_name); i++) if(!strcmp(ent_name[i], cname)){
		Entity *pt;
		pt = ent_creator[i]();
		return pt;
	}
	return NULL;
}

const char *Entity::idname()const{
	return "entity";
}

const char *Entity::classname()const{
	return "Entity";
}

void Entity::anim(double){}
