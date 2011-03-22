#ifndef WAR_H
#define WAR_H
#include "war.h"
#include "Entity.h"

template<Entity *WarField::*list> inline int WarField::countEnts()const{
	int ret = 0;
	for(Entity *p = this->*list; p; p = p->next)
		ret++;
	return ret;
}


#endif
