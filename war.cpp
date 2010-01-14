#include "war.h"
#include "entity.h"
#include "player.h"
#include "coordsys.h"
#include "astro.h"
#include "judge.h"
#include "serial_util.h"

int WarSpace::g_otdrawflags = 0;

WarField::WarField(){}

WarField::WarField(CoordSys *acs) : cs(acs), el(NULL), bl(NULL), pl(NULL), realtime(0){
	init_rseq(&rs, 2426);
}

const char *WarField::classname()const{
	return "WarField";
}

const unsigned WarField::classid = registerClass("WarField", Conster<WarField>);

#if 0 // reference
	Player *pl;
	Entity *el; /* entity list */
	Entity *bl; /* bullet list */
	struct tent3d_line_list *tell, *gibs;
	struct tent3d_fpol_list *tepl;
	struct random_sequence rs;
	int effects; /* trivial; telines or tefpols generated by this frame: to control performance */
	double realtime;
	double soundtime;
	otnt *ot, *otroot, *ottemp;
	int ots, oti;
	CoordSys *cs; /* redundant pointer to indicate belonging coordinate system */
#endif

void WarField::serialize(SerializeContext &sc){
	Serializable::serialize(sc);
	sc.o << pl << el << bl << rs << realtime;
}

void WarField::unserialize(UnserializeContext &sc){
	Serializable::unserialize(sc);
	sc.i >> pl >> el >> bl >> rs >> realtime;
}

void WarField::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext&)){
	(this->*method)(sc);
	if(el)
		el->dive(sc, method);
	if(bl)
		bl->dive(sc, method);
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

#if 0
#define TRYBLOCK(a) {try{a;}catch(std::exception e){fprintf(stderr, __FILE__"(%d) Exception %s\n", __LINE__, e.what());}catch(...){fprintf(stderr, __FILE__"(%d) Exception ?\n", __LINE__);}}
#else
#define TRYBLOCK(a) (a);
#endif

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
	aaanim(dt, this, list[1]);
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

		// Player does not follow entities into WarField, which has no positional information.
		if(!e->w || !(WarSpace*)*e->w)
			pl->unlink(e);

		// Delete if actually NULL is assigned.
		if(!e->w)
			delete e;
		else
			e->w->addent(e);
	}
	else
		pe = &(*pe)->next;
}

void WarField::draw(wardraw_t *wd){}
void WarField::drawtra(wardraw_t *wd){}

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

struct tent3d_line_list *WarField::getTeline3d(){return NULL;}
struct tent3d_fpol_list *WarField::getTefpol3d(){return NULL;}
WarField::operator WarSpace*(){return NULL;}

Entity *WarField::addent(Entity *e){
	Entity **plist = e->isTargettable() ? &el : &bl;
	e->w = this;
	e->next = *plist;
	return *plist = e;
}






























const char *WarSpace::classname()const{
	return "WarSpace";
}


const unsigned WarSpace::classid = registerClass("WarSpace", Conster<WarSpace>);

#if 0 // reference
	Player *pl;
	Entity *el; /* entity list */
	Entity *bl; /* bullet list */
	struct tent3d_line_list *tell, *gibs;
	struct tent3d_fpol_list *tepl;
	struct random_sequence rs;
	int effects; /* trivial; telines or tefpols generated by this frame: to control performance */
	double realtime;
	double soundtime;
	otnt *ot, *otroot, *ottemp;
	int ots, oti;
	CoordSys *cs; /* redundant pointer to indicate belonging coordinate system */
#endif

void WarSpace::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << pl << effects << soundtime << cs;
}

void WarSpace::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> pl >> effects >> soundtime >> cs;
}

WarSpace::WarSpace() : tell(NewTeline3D(2048, 128, 128)), tepl(NewTefpol3D(2047, 128, 128)), ot(NULL), otroot(NULL), oti(0), ots(0){}

WarSpace::WarSpace(CoordSys *acs) : st(acs), tell(NewTeline3D(2048, 128, 128)), gibs(NULL), tepl(NewTefpol3D(2047, 128, 128)),
	ot(NULL), otroot(NULL), oti(0), ots(0),
	effects(0), soundtime(0)
{
	for(CoordSys *root = cs; root; root = root->parent){
		Universe *u = root->toUniverse();
		if(u){
			pl = u->ppl;
			break;
		}
	}
}

void WarSpace::anim(double dt){
	CoordSys *root = cs;
	for(; root; root = root->parent){
		Universe *u = root->toUniverse();
		if(u){
			pl = u->ppl;
			break;
		}
	}
	aaanim(dt, this, list[0]);
//	fprintf(stderr, "otbuild %p %p %p %d\n", this->ot, this->otroot, this->ottemp);
	TRYBLOCK(ot_build(this, dt));
	aaanim(dt, this, list[1]);
	TRYBLOCK(ot_check(this, dt));
	TRYBLOCK(AnimTeline3D(tell, dt));
	TRYBLOCK(AnimTefpol3D(tepl, dt));
}

void WarSpace::draw(wardraw_t *wd){
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

void WarSpace::drawtra(wardraw_t *wd){
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

struct tent3d_line_list *WarSpace::getTeline3d(){return tell;}
struct tent3d_fpol_list *WarSpace::getTefpol3d(){return tepl;}
WarSpace::operator WarSpace *(){return this;}
