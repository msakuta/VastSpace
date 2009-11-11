#include "war.h"
#include "entity.h"
#include "player.h"
#include "coordsys.h"
#include "astro.h"

WarField::WarField(CoordSys *acs) : el(NULL), cs(acs), pl(NULL){
}

void WarField::anim(double dt){
	CoordSys *root = cs;
	for(; root; root = root->parent){
		Universe *u = root->toUniverse();
		if(u){
			pl = u->ppl;
			break;
		}
	}
	for(Entity *e = el; e; e = e->next){
		e->anim(dt);
		if(pl && !pl->chase && (e->pos - pl->pos).slen() < .002 * .002)
			pl->chase = e;
	}
}

void WarField::draw(wardraw_t *wd){
	for(Entity *e = el; e; e = e->next)
		e->draw(wd);
}

bool WarField::pointhit(const Vec3d &pos, const Vec3d &velo, double dt, struct contact_info*)const{
	return false;
}

Entity *WarField::addent(Entity *e){
	e->w = this;
	e->next = el;
	return el = e;
}
