#include "war.h"
#include "entity.h"
#include "player.h"
#include "coordsys.h"
#include "astro.h"
#include "judge.h"

int WarField::g_otdrawflags = 0;

WarField::WarField(CoordSys *acs) : el(NULL), bl(NULL), cs(acs), pl(NULL), tell(NewTeline3D(2048, 128, 128)), tepl(NewTefpol3D(2047, 128, 128)), ot(NULL), otroot(NULL), ottemp(NULL){
	for(CoordSys *root = cs; root; root = root->parent){
		Universe *u = root->toUniverse();
		if(u){
			pl = u->ppl;
			break;
		}
	}
}

static Entity *WarField::*const list[2] = {&WarField::el, &WarField::bl};

void aaanim(double dt, WarField *w, Entity *WarField::*li){
	Player *pl = w->pl;
	for(Entity *pe = w->*li; pe; pe = pe->next){
		try{
			pe->anim(dt);
		}
		catch(std::exception e){
			fprintf(stderr, __FILE__"(%d) Exception in %p->%s::anim(): %s\n", __LINE__, pe, pe->idname(), e.what());
		}
		catch(...){
			fprintf(stderr, __FILE__"(%d) Exception in %p->%s::anim(): ?\n", __LINE__, pe, pe->idname());
		}
		if(pl && !pl->chase && (pe->pos - pl->pos).slen() < .002 * .002)
			pl->chase = pe;
	}
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
	aaanim(dt, this, list[0]);
	ot_build(this, dt);
	aaanim(dt, this, list[1]);
	AnimTeline3D(tell, dt);
	AnimTefpol3D(tepl, dt);
}

void WarField::postframe(){
	for(int i = 0; i < 2; i++)
	for(Entity *e = this->*list[i]; e; e = e->next)
		e->postframe();
}

void WarField::endframe(){
	for(int i = 0; i < 2; i++)
	for(Entity **pe = &(this->*list[i]); *pe;) if((*pe)->w != this){
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
	for(int i = 0; i < 2; i++)
	for(Entity *pe = this->*list[i]; pe; pe = pe->next) if(pe->w == this/* && wd->vw->zslice == (pl->chase && pl->mover == &Player::freelook && pl->chase->getUltimateOwner() == pe->getUltimateOwner() ? 0 : 1)*/){
		try{
			pe->draw(wd);
		}
		catch(std::exception e){
			fprintf(stderr, __FILE__"(%d) Exception in %p->%s::draw(): %s\n", __LINE__, pe, pe->idname(), e.what());
		}
		catch(...){
			fprintf(stderr, __FILE__"(%d) Exception in %p->%s::draw(): ?\n", __LINE__, pe, pe->idname());
		}
	}
}

void WarField::drawtra(wardraw_t *wd){
	for(int i = 0; i < 2; i++)
	for(Entity *pe = this->*list[i]; pe; pe = pe->next) if(pe->w == this/* && wd->vw->zslice == (pl->chase && pl->mover == &Player::freelook && pl->chase->getUltimateOwner() == pe->getUltimateOwner() ? 0 : 1)*/){
		try{
			pe->drawtra(wd);
		}
		catch(std::exception e){
			fprintf(stderr, __FILE__"(%d) Exception in %p->%s::drawtra(): %s\n", __LINE__, pe, pe->idname(), e.what());
		}
		catch(...){
			fprintf(stderr, __FILE__"(%d) Exception in %p->%s::drawtra(): ?\n", __LINE__, pe, pe->idname());
		}
	}

	tent3d_line_drawdata dd;
	*(Vec3d*)(dd.viewdir) = -wd->vw->rot.vec3(2);
	*(Vec3d*)dd.viewpoint = wd->vw->pos;
	*(Mat4d*)(dd.invrot) = wd->vw->irot;
	dd.fov = wd->vw->fov;
	dd.pgc = NULL;
	*(Quatd*)dd.rot = wd->vw->qrot;
	DrawTeline3D(tell, &dd);
	DrawTefpol3D(tepl, wd->vw->pos, &static_cast<glcull>(*wd->vw->gc));

	if(g_otdrawflags)
		ot_draw(this, wd);
}

bool WarField::pointhit(const Vec3d &pos, const Vec3d &velo, double dt, struct contact_info*)const{
	return false;
}

Vec3d WarField::accel(const Vec3d &srcpos, const Vec3d &srcvelo)const{
	return Vec3d(0,0,0);
}

double WarField::war_time()const{
	CoordSys *top = cs;
	for(; top->parent; top = top->parent);
	if(!top || !top->toUniverse())
		return 0.;
	return top->toUniverse()->global_time;
}


Entity *WarField::addent(Entity *e){
	Entity **plist = e->isTargettable() ? &el : &bl;
	e->w = this;
	e->next = *plist;
	return *plist = e;
}
