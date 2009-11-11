#include "war.h"
#include "entity.h"

WarField::WarField() : el(NULL){
}

void WarField::anim(double dt){
	for(Entity *e = el; e; e = e->next)
		e->anim(dt);
}

void WarField::draw(wardraw_t *wd){
	for(Entity *e = el; e; e = e->next)
		e->draw(wd);
}

bool WarField::pointhit(const Vec3d &pos, const Vec3d &velo, double dt, struct contact_info*)const{
	return false;
}

Entity *WarField::addent(Entity *e){
	e->next = el;
	return el = e;
}
