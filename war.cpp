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
	for(Entity *pe = el; pe; pe = pe->next){
		try{
			pe->anim(dt);
		}
		catch(std::exception e){
			fprintf(stderr, __FILE__"(%d) Exception in %s::anim(): %s\n", __LINE__, pe->classname(), e.what());
		}
		catch(...){
			fprintf(stderr, __FILE__"(%d) Exception in %s::anim(): ?\n", __LINE__, pe->classname());
		}
		if(pl && !pl->chase && (pe->pos - pl->pos).slen() < .002 * .002)
			pl->chase = pe;
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
		if(!e->w){
			pl->unlink(e);
			delete e;
		}
		else
			e->w->addent(e);
	}
	else
		pe = &(*pe)->next;
}

void WarField::draw(wardraw_t *wd){
	try{
		for(Entity *e = el; e; e = e->next) if(e->w == this)
			e->draw(wd);
	}
	catch(std::exception e){
			fprintf(stderr, __FILE__"(%d) Exception %s\n", __LINE__, e.what());
	}
	catch(...){
			fprintf(stderr, __FILE__"(%d) Exception ?\n", __LINE__);
	}
}

void WarField::drawtra(wardraw_t *wd){
	for(Entity *e = el; e; e = e->next) if(e->w == this)
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

Vec3d WarField::accel(const Vec3d &srcpos, const Vec3d &srcvelo)const{
	return Vec3d(0,0,0);
}


Entity *WarField::addent(Entity *e){
	e->w = this;
	e->next = el;
	return el = e;
}
