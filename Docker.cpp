#include "Scarry.h"
#include "judge.h"
#include "serial_util.h"
#include "Player.h"
#ifndef DEDICATED
#include "antiglut.h"
#endif
//#include "sceptor.h"
//#include "Beamer.h"
#include "sqadapt.h"
//#include "glw/popup.h"
extern "C"{
#include <clib/mathdef.h>
}

/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289
#define SQRT2P2 (M_SQRT2/2.)



Docker::Docker(Entity *ae) : st(NULL), baycool(0), e(ae), remainDocked(false){
	for(int i = 0; i < numof(paradec); i++)
		paradec[i] = 0;
	if(ae && ae->w && ae->w->cs)
		cs = ae->w->cs;
}

Docker::~Docker(){
	// Docked entities die with their base.
	for(WarField::EntityList::iterator it = el.begin(); it != el.end(); it++){
		WarField::EntityList::iterator next = it;
		next++;
		delete *it;
		it = next;
	}
	for(EntityList::iterator it = undockque.begin(); it != undockque.end(); it++){
		EntityList::iterator next = it;
		it++;
		delete e;
		it = next;
	}
}

void Docker::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << e;
	sc.o << baycool;
	sc.o << undockque;
	for(int i = 0; i < numof(paradec); i++)
		sc.o << paradec[i];
	sc.o << remainDocked;
}

void Docker::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> e;
	sc.i >> baycool;
	sc.i >> undockque;
	for(int i = 0; i < numof(paradec); i++)
		sc.i >> paradec[i];
	sc.i >> remainDocked;
}

void Docker::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	// Do NOT call st::dive here because this class is a virtual branch class.
	(this->*method)(sc);
	for(EntityList::iterator it = el.begin(); it != el.end(); it++) if(*it)
		(*it)->dive(sc, method);
	for(EntityList::iterator it = undockque.begin(); it != undockque.end(); it++) if(*it)
		(*it)->dive(sc, method);
}

void Docker::anim(double dt){
/*	if(!remainDocked && el){
		Dockable **end = &undockque;
		for(; *end; end = &(*end)->next);
		*end = el;
		el = NULL;
	}*/
	for(EntityList::iterator it = el.begin(); it != el.end(); it++)
		if(*it)
			(*it)->anim(dt);
	while(!undockque.empty() && baycool < dt){
		if(!undock(undockque.front()))
			break;
//		undockque = next ? next->toDockable() : NULL;
	}

	if(baycool < dt){
		baycool = 0.;
	}
	else
		baycool -= dt;
}

void Docker::dock(Dockable *e){
	e->dock(this);
	if(e->w)
		e->w = this;
	else{
		e->w = this;
		addent(e);
	}
/*	e->next = el;
	el = e;*/
}

bool Docker::postUndock(Dockable *e){
	for(EntityList::iterator it = el.begin(); it != el.end();){
		EntityList::iterator next = it;
		next++;
		if(*it == e){
			el.erase(it);
			undockque.push_back(*it);
//			Dockable **qend = &undockque;
//			if(*qend) for(; *qend; qend = reinterpret_cast<Dockable**>(&(*qend)->next)); // Search until end
//			*qend = e;
//			e->next = NULL; // Appending to end means next is equal to NULL
			return true;
		}
		it = next;
	}
	return false;
}

Entity *Docker::addent(Entity *e){
	e->w = this;
//	e->next = el;
	el.push_back(e);
	return e;
}

Docker::operator Docker *(){return this;}

bool Docker::undock(Dockable *e){
	EntityList::iterator it = undockque.begin();
	while(it != undockque.end()){
		if(*it == e)
			it = undockque.erase(it);
		else
			it++;
	}
	e->w = this->e->w;
	this->e->w->addent(e);
	e->undock(this);
	return true;
}

void Docker::sq_define(HSQUIRRELVM v){
	static const SQChar *tt_Docker = "Docker";
	sq_pushstring(v, _SC("Docker"), -1);
	sq_newclass(v, SQFalse);
	sq_settypetag(v, -1, const_cast<SQChar*>(tt_Docker));
	sq_pushstring(v, _SC("ref"), -1);
	sq_pushnull(v);
	sq_newslot(v, -3, SQFalse);
	register_closure(v, _SC("_get"), sqf_get);
	register_closure(v, _SC("addent"), sqf_addent);
	sq_createslot(v, -3);
}

SQInteger Docker::sqf_get(HSQUIRRELVM v){
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	if(!strcmp(wcs, _SC("alive"))){
		SQUserPointer o;
		sq_pushbool(v, sqa_refobj(v, &o, NULL, 1, false));
		return 1;
	}
}

SQInteger Docker::sqf_addent(HSQUIRRELVM v){
	if(sq_gettop(v) < 2)
		return SQ_ERROR;
	Docker *p;
	SQRESULT sr;
	if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
		return sr;

/*	const SQChar *arg;
	if(SQ_FAILED(sq_getstring(v, 2, &arg)))
		return SQ_ERROR;

	extern Player *ppl;
	Entity *pt = Entity::create(arg, p);
	if(!pt)
		sq_throwerror(v, cpplib::dstring("addent: Unknown entity class name: %s") << arg);

	sq_pushroottable(v);
	sq_pushstring(v, _SC("Entity"), -1);
	sq_get(v, -2);
	sq_createinstance(v, -1);
	sqa_newobj(v, pt);*/
	Entity *pt;
	if(!sqa_refobj(v, (SQUserPointer*)&pt, &sr, 2))
		return 0;
	p->addent(pt);
	return 0;
}

#ifndef DEDICATED
int GLWdock::mouse(GLwindowState &ws, int mbutton, int state, int mx, int my){
	GLWdock *p = this;
	int ind, sel0, sel1, ix;
	int fonth = (int)getFontHeight();
	if(st::mouse(ws, mbutton, state, mx, my))
		return 1;
	ind = (my - 5 * 12) / 12;
	if(docker && (mbutton == GLUT_RIGHT_BUTTON || mbutton == GLUT_LEFT_BUTTON) && state == GLUT_UP || mbutton == GLUT_WHEEL_UP || mbutton == GLUT_WHEEL_DOWN){
		int num = 1, i;
		if(ind == -2)
			docker->remainDocked = !docker->remainDocked;
		if(ind == -1)
			grouping = !grouping;
		else if(grouping){
			std::map<cpplib::dstring, int> map;

			for(WarField::EntityList::iterator it = docker->el.begin(); it != docker->el.end(); it++) if(*it){
				Entity *e = (*it)->toDockable();
				if(e)
					map[e->dispname()]++;
			}
			for(std::map<cpplib::dstring, int>::iterator it = map.begin(); it != map.end(); it++) if(!ind--){
				for(WarField::EntityList::iterator it2 = docker->el.begin(); it2 != docker->el.end(); it2++){
					WarField::EntityList::iterator next = it2;
					next++;
					Entity *e = *it2;
					if(e && !strcmp(e->dispname(), it->first))
						docker->postUndock(e->toDockable());
					it2 = next;
				}
				break;
			}
		}
		else for(WarField::EntityList::iterator it = docker->el.begin(); it != docker->el.end(); it++) if(!ind--){
			Entity *e = *it;
			if(e)
				docker->postUndock(e->toDockable());
			break;
		}
		return 1;
	}
	return 0;
}

void GLWdock::postframe(){
	if(docker && !docker->e->w)
		docker = NULL;
}


int cmd_dockmenu(int argc, char *argv[], void *pv){
	Player &pl = *(Player*)pv;
	if(pl.selected.empty() || !(*pl.selected.begin())->w)
		return 0;
	Docker *pb = (*pl.selected.begin())->getDocker();
	if(pb)
		glwAppend(new GLWdock("Dock", pb));
	return 0;
}

#endif
