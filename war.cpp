#include "war.h"
#include "entity.h"
#include "player.h"
#include "coordsys.h"
#include "astro.h"

WarField::WarField(CoordSys *acs) : el(NULL), cs(acs), pl(NULL){
	for(CoordSys *root = cs; root; root = root->parent){
		Universe *u = root->toUniverse();
		if(u){
			pl = u->ppl;
			break;
		}
	}
	tell = NewTeline3D(2048, 128, 128);
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
		try{
			e->anim(dt);
		}
		catch(std::exception e){
			fprintf(stderr, "Exception %s\n", e.what());
		}
		if(pl && !pl->chase && (e->pos - pl->pos).slen() < .002 * .002)
			pl->chase = e;
	}
	AnimTeline3D(tell, dt);
}

void WarField::postframe(){
	for(Entity *e = el; e; e = e->next)
		e->postframe();
}

void WarField::endframe(){
	for(Entity **pe = &el; *pe;) if((*pe)->w != this){
		Entity *e = *pe;
		*pe = e->next;
		if(!e->w)
			delete e;
		else
			e->w->addent(e);
	}
	else
		pe = &(*pe)->next;
}

void WarField::draw(wardraw_t *wd){
	for(Entity *e = el; e; e = e->next)
		e->draw(wd);
}

void WarField::drawtra(wardraw_t *wd){
	for(Entity *e = el; e; e = e->next)
		e->drawtra(wd);
	tent3d_line_drawdata dd;
	*(Vec3d*)(dd.viewdir) = -wd->vw->rot.vec3(2);
	*(Vec3d*)dd.viewpoint = wd->vw->pos;
	*(Mat4d*)(dd.invrot) = wd->vw->irot;
	dd.fov = wd->vw->fov;
	dd.pgc = NULL;
	*(Quatd*)dd.rot = wd->vw->qrot;
	DrawTeline3D(tell, &dd);
}

bool WarField::pointhit(const Vec3d &pos, const Vec3d &velo, double dt, struct contact_info*)const{
	return false;
}

Entity *WarField::addent(Entity *e){
	e->w = this;
	e->next = el;
	return el = e;
}
