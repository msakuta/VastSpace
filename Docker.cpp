/** \file
 * \brief Implementation of Docker class.
 */
#ifndef DEDICATED
#include "Application.h"
#endif
#include "Docker.h"
#include "serial_util.h"
#include "Player.h"
#include "cmd.h"
#ifndef DEDICATED
#include "antiglut.h"
#endif
#include "sqadapt.h"
extern "C"{
#include <clib/c.h>
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



Docker::Docker(Entity *ae) : st(ae ? ae->getGame() : NULL), baycool(0), e(ae), remainDocked(false){
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
	for(EntityList::iterator it = el.begin(); it != el.end();){
		EntityList::iterator next = it;
		next++;
		if(*it)
			(*it)->anim(dt);
		it = next;
	}
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

	// If the Entity has been belonged to another WarField, notify TransitEvent
	// to clear old references.
	TransitEvent te(this);
	e->notifyEvent(te);

	e->w = this;
	addent(e);
}

bool Docker::postUndock(Dockable *e){
	for(EntityList::iterator it = el.begin(); it != el.end();){
		EntityList::iterator next = it;
		next++;
		if(*it == e){
			undockque.push_back(*it);
			el.erase(it);
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

/// \brief The release hook of Entity that clears the weak pointer.
static SQInteger sqh_release(SQUserPointer p, SQInteger){
	((WeakPtr<Docker>*)p)->~WeakPtr<Docker>();
	return 1;
}

void Docker::sq_pushobj(HSQUIRRELVM v, Docker *cs){
	sq_pushroottable(v);
	sq_pushstring(v, _SC("Docker"), -1);
	if(SQ_FAILED(sq_get(v, -2)))
		throw SQFError("Something's wrong with CoordSys class definition.");
	if(SQ_FAILED(sq_createinstance(v, -1)))
		throw SQFError("Couldn't create class Docker");
	SQUserPointer p;
	if(SQ_FAILED(sq_getinstanceup(v, -1, &p, NULL)))
		throw SQFError("Something's wrong with Squirrel Class Instace of CoordSys.");
	new(p) WeakPtr<Docker>(cs);
	sq_setreleasehook(v, -1, sqh_release);
	sq_remove(v, -2); // Remove Class
	sq_remove(v, -2); // Remove root table
}

Docker *Docker::sq_refobj(HSQUIRRELVM v, SQInteger idx){
	SQUserPointer up;
	// If the instance does not have a user pointer, it's a serious exception that might need some codes fixed.
	if(SQ_FAILED(sq_getinstanceup(v, idx, &up, NULL)) || !up)
		throw SQFError("Something's wrong with Squirrel Class Instace of CoordSys.");
	return *(WeakPtr<Docker>*)up;
}

static Initializer dockerInit("Docker", Docker::sq_define);

bool Docker::sq_define(HSQUIRRELVM v){
	static const SQChar *tt_Docker = "Docker";
	sq_pushstring(v, _SC("Docker"), -1);
	sq_newclass(v, SQFalse);
	sq_settypetag(v, -1, const_cast<SQChar*>(tt_Docker));
	sq_setclassudsize(v, -1, sizeof(WeakPtr<Docker>));
	register_closure(v, _SC("_get"), sqf_get);
	register_closure(v, _SC("addent"), sqf_addent);
	sq_createslot(v, -3);
	return true;
}

SQInteger Docker::sqf_get(HSQUIRRELVM v){
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	if(!strcmp(wcs, _SC("alive"))){
		SQUserPointer o;
		sq_pushbool(v, SQBool(!!sq_refobj(v)));
		return 1;
	}
	return SQ_ERROR;
}

SQInteger Docker::sqf_addent(HSQUIRRELVM v){
	try{
		if(sq_gettop(v) < 2)
			return SQ_ERROR;
		Docker *p = sq_refobj(v);
		Entity *pt = NULL;
		if(sq_gettype(v, 2) == OT_INSTANCE){
			pt = Entity::sq_refobj(v, 2);
			p->addent(pt);
		}
		else{
			const SQChar *arg;
			if(SQ_FAILED(sq_getstring(v, 2, &arg)))
				return SQ_ERROR;
			pt = Entity::create(arg, p);
		}

		// Return added Entity
		if(pt){
			Entity::sq_pushobj(v, pt);
			return 1;
		}
		else
			return SQ_ERROR;
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.what());
	}
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


static int cmd_dockmenu(int argc, char *argv[], void *pv){
	Application *app = (Application*)pv;
	Player *pl = app->clientGame ? app->clientGame->player : app->serverGame->player;
	if(!pl)
		return 0;
	if(pl->selected.empty() || !(*pl->selected.begin())->w)
		return 0;
	Docker *pb = (*pl->selected.begin())->getDocker();
	if(pb)
		glwAppend(new GLWdock("Dock", pb));
	return 0;
}

void Docker::init(){
	CmdAddParam("dockmenu", cmd_dockmenu, &application);
}

#else
void Docker::init(){}
#endif
