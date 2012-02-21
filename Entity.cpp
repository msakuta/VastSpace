/** \file
 * \brief Implements Entity class and its collaborative classes.
 */
#include "Entity.h"
#include "Application.h"
#include "EntityCommand.h"
extern "C"{
#include <clib/aquat.h>
}
#include "Beamer.h"
#include "judge.h"
#include "Player.h"
#include "cmd.h"
#include "Scarry.h"
#include "RStation.h"
//#include "glw/glwindow.h"
#include "serial_util.h"
#include "Destroyer.h"
#include "btadapt.h"
#include "sqadapt.h"
#include "stellar_file.h"
#include <btBulletDynamicsCommon.h>
#include <sstream>
#include <iostream>



Entity::Entity(WarField *aw) :
	st(aw ? aw->getGame() : NULL),
	pos(vec3_000),
	velo(vec3_000),
	omg(vec3_000),
	rot(quat_u),
	mass(1e3),
	moi(1e1),
	enemy(NULL),
	w(aw),
	inputs(),
	health(1),
	race(0),
	otflag(0),
	controller(NULL),
	bbody(NULL)
{
//	The entity must be constructed before being contained by some surroundings.
//  This generic problem is fairly difficult whether the object should be constructed before being assigned to its container.
//  Objects like Entities in this program are always tied to a WarField and assumed valid only if containing WarField is defined.
//  Then, should we assign an Entity to a WarField in its construction? Previous answer is yes, and our answer is no later on.
//  The reason is that the Entity-derived object's vft is not pointing to derived class's vft here.
//  The user must call WarField::addent() explicitly to let an object attend to a WarField.
//	if(aw)
//		aw->addent(this);
}

Entity::~Entity(){
	if(w)
		leaveField(w);
	if(bbody){
/*		WarSpace *ws;
		if(w && (ws = *w) && ws->bdw)
			ws->bdw->removeRigidBody(bbody);*/
		delete bbody;
	}
	if(game->isServer()){
		sqa_delete_Entity(this);
		sqa_deleteobj(game->sqvm, this);
	}
//	if(controller && controller->unlink(this))
//		controller = NULL;
}

void Entity::init(){
	health = maxhealth();
}


Entity *Entity::create(const char *cname, WarField *w){
	EntityStatic *ctor = NULL;;
	for(EntityCtorMap::iterator it = entityCtorMap().begin(); it != entityCtorMap().end(); it++) if(!strcmp(it->first, cname)){
		ctor = it->second;
		break;
	}
	if(!ctor)
		return NULL;
	Entity *e = ctor->create(w);
	if(e && w){
		w->addent(e);
	}
	return e;
}

Entity::EntityStaticBase Entity::entityRegister;

Entity::EntityStaticBase::EntityStaticBase() : EntityStatic("Entity"){
}

ClassId Entity::EntityStaticBase::classid(){
	return "Entity";
}

Entity *Entity::EntityStaticBase::create(WarField *){
	return NULL;
}

Entity *Entity::EntityStaticBase::stcreate(){
	return NULL;
}

void Entity::EntityStaticBase::destroy(Entity *){
}

const SQChar *Entity::EntityStaticBase::sq_classname(){
	return _SC("Entity");
}

static SQInteger sqf_Entity_tostring(HSQUIRRELVM v){
	SQUserPointer o;
	SQRESULT sr;
	if(!sqa_refobj(v, &o, NULL, 1)){
		sq_pushstring(v, _SC("(deleted)"), -1);
		return 1;
	}
	Entity *p = (Entity*)o;
	if(p){
		sq_pushstring(v, gltestp::dstring() << "{" << p->classname() << ":" << p->getid() << "}", -1);
	}
	else
		return sq_throwerror(v, _SC("Object is deleted"));
	return 1;
}

static SQInteger sqf_Entity_get(HSQUIRRELVM v){
	Entity *p;
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	SQRESULT sr;
	if(!sqa_refobj(v, (SQUserPointer*)&p, &sr)){
		if(!strcmp(wcs, _SC("alive"))){
			sq_pushbool(v, SQFalse);
			return 1;
		}
		return sr;
	}
	if(!p)
		return -1;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!strcmp(wcs, _SC("race"))){
		sq_pushinteger(v, p->race);
		return 1;
	}
	else if(!strcmp(wcs, _SC("next"))){
		if(!p->next){
			sq_pushnull(v);
			return 1;
		}
		sq_pushroottable(v);
		sq_pushstring(v, _SC("Entity"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		sqa_newobj(v, p->next);
//		sq_setinstanceup(v, -1, p->next);
		return 1;
	}
/*	else if(!strcmp(wcs, _SC("selectnext"))){
		if(!p->selectnext){
			sq_pushnull(v);
			return 1;
		}
		sq_pushroottable(v);
		sq_pushstring(v, _SC("Entity"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		sqa_newobj(v, p->selectnext);
//		sq_setinstanceup(v, -1, p->next);
		return 1;
	}*/
	else if(!strcmp(wcs, _SC("dockedEntList"))){
		Docker *d = p->getDocker();
		if(!d || !d->el){
			sq_pushnull(v);
			return 1;
		}
		sq_pushroottable(v);
		sq_pushstring(v, _SC("Entity"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		sqa_newobj(v, d->el);
//		sq_setinstanceup(v, -1, p->next);
		return 1;
	}
	else if(!strcmp(wcs, _SC("docker"))){
		Docker *d = p->getDocker();
		if(!d){
			sq_pushnull(v);
			return 1;
		}
		sq_pushroottable(v);
		sq_pushstring(v, _SC("Docker"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		sqa_newobj(v, d);
		return 1;
	}
	else if(!strcmp(wcs, _SC("enemy"))){
		SQUserPointer o;
		if(!p || !p->enemy){
			sq_pushnull(v);
			return 1;
		}
		sq_pushroottable(v);
		sq_pushstring(v, _SC("Entity"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		sqa_newobj(v, p->enemy);
		return 1;
	}
	else if(!strcmp(wcs, _SC("health"))){
		SQUserPointer o;
		if(!p){
			sq_pushnull(v);
			return 1;
		}
		sq_pushfloat(v, SQFloat(p->health));
		return 1;
	}
	else
		return sqf_get<Entity>(v);
}

static SQInteger sqf_Entity_set(HSQUIRRELVM v){
	if(sq_gettop(v) < 3)
		return SQ_ERROR;
	Entity *p;
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	if(!sqa_refobj(v, (SQUserPointer*)&p))
		return SQ_ERROR;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!strcmp(wcs, _SC("race"))){
		SQInteger retint;
		if(SQ_FAILED(sq_getinteger(v, 3, &retint)))
			return SQ_ERROR;
		p->race = int(retint);
		return 0;
	}
	else if(!strcmp(wcs, _SC("enemy"))){
		SQUserPointer o;
		SQRESULT sr;
		if(!sqa_refobj(v, &o, &sr, 3))
			return sr;
		p->enemy = (Entity*)(o);
//		p->enemy->addObserver(p);
		return 1;
	}
	else
		return sqf_set<Entity>(v);
}

static SQInteger sqf_Entity_command(HSQUIRRELVM v){
	try{
		Entity *p;
		if(!sqa_refobj(v, (SQUserPointer*)&p))
			return 0;
		const SQChar *s;
		sq_getstring(v, 2, &s);

		EntityCommandStatic *ecs = EntityCommand::ctormap()[s];

		if(ecs && ecs->sq_command){
			ecs->sq_command(v, *p);
		}
		else
			return sq_throwerror(v, _SC("Entity::command(): Undefined Entity Command"));
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.description);
	}
	return 0;
}

static SQInteger sqf_Entity_create(HSQUIRRELVM v){
	try{
		const SQChar *classname;
		sq_getstring(v, 2, &classname);

		Entity *pt = Entity::create(classname, NULL);

		if(!pt)
			return sq_throwerror(v, _SC("Undefined Entity class name"));
		sq_pushroottable(v);
		sq_pushstring(v, _SC("Entity"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		sqa_newobj(v, pt);
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.description);
	}
	return 1;
}

static SQInteger sqf_Entity_kill(HSQUIRRELVM v){
	try{
		Entity *p;
		if(!sqa_refobj(v, (SQUserPointer*)&p))
			return 0;
		const SQChar *s;
		if(p->getGame()->isServer())
			delete p;
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.description);
	}
	return 1;
}

bool Entity::EntityStaticBase::sq_define(HSQUIRRELVM v){
	// Define class Entity
	sq_pushstring(v, _SC("Entity"), -1);
	sq_newclass(v, SQFalse);
	sq_settypetag(v, -1, SQUserPointer(sq_classname()));
	sq_pushstring(v, _SC("ref"), -1);
	sq_pushnull(v);
	sq_newslot(v, -3, SQFalse);
	register_closure(v, _SC("getpos"), sqf_getintrinsic<Entity, Vec3d, membergetter<Entity, Vec3d, &Entity::pos> >);
	register_closure(v, _SC("setpos"), sqf_setintrinsic<Entity, Vec3d, &Entity::pos>);
	register_closure(v, _SC("getrot"), sqf_getintrinsic<Entity, Quatd, membergetter<Entity, Quatd, &Entity::rot> >);
	register_closure(v, _SC("setrot"), sqf_setintrinsic<Entity, Quatd, &Entity::rot>);
	register_closure(v, _SC("_get"), sqf_Entity_get);
	register_closure(v, _SC("_set"), sqf_Entity_set);
	register_closure(v, _SC("_tostring"), sqf_Entity_tostring, 1);
	register_closure(v, _SC("command"), sqf_Entity_command);
	register_closure(v, _SC("create"), sqf_Entity_create);
	register_closure(v, _SC("kill"), sqf_Entity_kill);
	sq_createslot(v, -3);
	return true;
}

Entity::EntityStatic *Entity::EntityStaticBase::st(){
	return NULL;
}


unsigned Entity::registerEntity(ClassId name, EntityStatic *ctor){
	EntityCtorMap &ctormap = entityCtorMap();
	if(ctormap.find(name) != ctormap.end())
		CmdPrintf(cpplib::dstring("WARNING: Duplicate class name: ") << name);
	ctormap[name] = ctor;

	// If an extension module is loaded and a Entity derived class is newly defined through this function,
	// the Squirrel VM is already initialized to load definition of classes.
	// Therefore, extension module classes must define themselves in Squirrel code as they register.
	if(sqa::g_sqvm){
		sq_pushroottable(g_sqvm);
		ctor->sq_define(sqa::g_sqvm);
		sq_poptop(g_sqvm);
	}

	return ctormap.size();
}

Entity::EntityStatic::EntityStatic(ClassId classname){
}

Entity::EntityCtorMap &Entity::entityCtorMap(){
	static EntityCtorMap ictormap;
	return ictormap;
}


const char *Entity::idname()const{
	return "entity";
}

const char *Entity::classname()const{
	return "Entity";
}

const char *Entity::dispname()const{
	return classname();
}

#if 0 // reference
	Vec3d pos;
	Vec3d velo;
	Vec3d omg;
	Quatd rot; /* rotation expressed in quaternion */
	double mass; /* [kg] */
	double moi;  /* moment of inertia, [kg m^2] should be a tensor */
//	double turrety, barrelp;
//	double desired[2];
	double health;
//	double cooldown, cooldown2;
	Entity *next;
	Entity *selectnext; /* selection list */
	Entity *enemy;
	int race;
//	int shoots, shoots2, kills, deaths;
	input_t inputs;
	WarField *w; // belonging WarField, NULL means being bestroyed. Assigning another WarField marks it to transit to new CoordSys.
	int otflag;
#endif

void Entity::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << w;
	sc.o << pos;
	sc.o << velo;
	sc.o << omg;
	sc.o << rot;
	sc.o << mass;
	sc.o << moi;
	sc.o << health;
	sc.o << next /*<< selectnext*/;
	sc.o << enemy;
	sc.o << race;
	sc.o << otflag;
}

void Entity::unserialize(UnserializeContext &sc){
	WarField *oldwf = w;

	st::unserialize(sc);
	sc.i >> w;
	sc.i >> pos;
	sc.i >> velo;
	sc.i >> omg;
	sc.i >> rot;
	sc.i >> mass;
	sc.i >> moi;
	sc.i >> health;
	sc.i >> next /*>> selectnext*/;
	sc.i >> enemy;
	sc.i >> race;
	sc.i >> otflag;

	// Postprocessing calls leave/enterField according to frame deltas.
	if(oldwf != w){
		leaveField(oldwf);
		if(w)
			enterField(w);
	}
}

void Entity::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	st::dive(sc, method);
	if(controller)
		controller->dive(sc, method);
	if(next)
		next->dive(sc, method);
}


double Entity::maxhealth()const{return 100.;}

/// Called when this Entity is entering a WarField, just after adding to Entity list in the WarField.
///
/// This function is also called when creating an Entity.
void Entity::enterField(WarField *aw){
	WarSpace *ws = *aw;
	if(ws && ws->bdw && bbody){
		ws->bdw->addRigidBody(bbody);
		bbody->activate(); // Is it necessary?
	}
}

/// Called when this Entity is leaving a WarField, after removing from Entity list in the WarField.
///
/// This function is also called when deleting an Entity.
void Entity::leaveField(WarField *aw){
	if(!aw)
		return;
	WarSpace *ws = *aw;
	if(ws && ws->bdw && bbody)
		ws->bdw->removeRigidBody(bbody);
}

void Entity::setPosition(const Vec3d *apos, const Quatd *arot, const Vec3d *avelo, const Vec3d *aavelo){
	if(apos && apos != &pos) pos = *apos;
	if(arot && arot != &rot) rot = *arot;
	if(avelo && avelo != &velo) velo = *avelo;
	if(aavelo && aavelo != &omg) omg = *aavelo;

	if(bbody){
		if(apos && arot)
			bbody->setCenterOfMassTransform(btTransform(btqc(*arot), btvc(*apos)));
		else if(apos)
			bbody->setCenterOfMassTransform(btTransform(bbody->getOrientation(), btvc(*apos)));
		else if(arot)
			bbody->setCenterOfMassTransform(btTransform(btqc(*arot), bbody->getCenterOfMassPosition()));
		if(avelo)
			bbody->setLinearVelocity(btvc(*avelo));
		if(aavelo)
			bbody->setAngularVelocity(btvc(*aavelo));
	}
}
void Entity::getPosition(Vec3d *apos, Quatd *arot, Vec3d *avelo, Vec3d *aavelo)const{
	if(apos) *apos = pos;
	if(arot) *arot = rot;
	if(avelo) *avelo = velo;
	if(aavelo) *aavelo = omg;
}

/// \brief Updates the Entity.
///
/// This method is called periodically to advance simulation steps.
/// Entity-derived classes can override this method to define how to time-develop
/// themselves.
///
/// \param dt Delta-time of this frame.
/// \sa clientUpdate
void Entity::anim(double dt){dt;}

/// \brief Updates the Entity in the client side.
///
/// Almost all physics are calculated in server side, but the client needs to update
/// itself to maintain rendering states.
///
/// \param dt Delta-time of this frame.
void Entity::clientUpdate(double dt){dt;}

void Entity::postframe(){if(enemy && !enemy->w) enemy = NULL;}
void Entity::control(const input_t *i, double){inputs = *i;}
unsigned Entity::analog_mask(){return 0;}
void Entity::cockpitView(Vec3d &pos, Quatd &rot, int)const{pos = this->pos; rot = this->rot;}
int Entity::numCockpits()const{return 1;}
void Entity::draw(wardraw_t *){}
void Entity::drawtra(wardraw_t *){}
void Entity::drawHUD(wardraw_t *){}
void Entity::drawOverlay(wardraw_t *){}
bool Entity::solid(const Entity *)const{return true;} // Default is to check hits
void Entity::bullethit(const Bullet *){}
Entity *Entity::getOwner(){return NULL;}
bool Entity::isSelectable()const{return false;}
int Entity::armsCount()const{return 0;}
ArmBase *Entity::armsGet(int){return NULL;}
Entity::Props Entity::props()const{
	using namespace cpplib;
	Props ret;
//	std::stringstream ss;
//	ss << "Class: " << classname();
//	std::stringbuf *pbuf = ss.rdbuf();
//	ret.push_back(pbuf->str());
	ret.push_back(gltestp::dstring("Class: ") << classname());
	ret.push_back(gltestp::dstring("CoordSys: ") << w->cs->getpath());
	ret.push_back(gltestp::dstring("Health: ") << dstring(health) << "/" << dstring(maxhealth()));
	ret.push_back(gltestp::dstring("Race: ") << dstring(race));
	ret.push_back(gltestp::dstring("Controller: ") << dstring(controller ? controller->classname() : "(None)"));
	return ret;
}
double Entity::getRU()const{return 0.;}
Builder *Entity::getBuilderInt(){return NULL;}
Docker *Entity::getDockerInt(){return NULL;}
bool Entity::dock(Docker*){return false;}
bool Entity::undock(Docker*){return false;}
bool Entity::command(EntityCommand *){return false;}

bool Entity::unlink(Observable*o){
	if(enemy == o)
		enemy.unlinkReplace(NULL);
	if(next == o)
		next.unlinkReplace(next->next);
	return true;
}

int Entity::tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *fret, Vec3d *retp, Vec3d *retnormal){
	Vec3d retpos;
	bool bret = !!jHitSpherePos(pos, this->hitradius() + rad, start, dir, 1., fret, &retpos);
	if(bret && retp)
		*retp = retpos;
	if(bret && retnormal)
		*retnormal = (retpos - pos).normin();
	return bret;
//	(hitpart = 0, 0. < (sdist = -VECSP(delta, &mat[8]) - rad))) && sdist < best
}

int Entity::takedamage(double damage, int hitpart){
	if(health <= damage){
		health = 0;
		w = NULL;
		return 0;
	}
	health -= damage;
	return 1;
}

// This function's purpose is unclear.
void Entity::bullethole(sufindex, double, const Vec3d &pos, const Quatd &rot){}

int Entity::popupMenu(PopupMenu &list){
	return 0;
}

Warpable *Entity::toWarpable(){return NULL;}
Entity::Dockable *Entity::toDockable(){return NULL;}

void Entity::transit_cs(CoordSys *cs){
	Mat4d mat;
	if(w == cs->w)
		return;

//	leaveField(w);

	// Transform position to target CoordSys
	Vec3d pos = cs->tocs(this->pos, w->cs);
	Vec3d velo = cs->tocsv(this->velo, this->pos, w->cs);
	this->pos = pos;
	this->velo = velo;
	if(!cs->w){
		cs->w = new WarSpace(cs);
	}

	Quatd q1 = w->cs->tocsq(cs);
	this->rot = q1 * this->rot;

	Player *pl = w->getPlayer();
	if(pl && pl->chase == this){
		pl->transit_cs(cs);
	}

	if(bbody){
/*		WarSpace *ws = *w;
		if(ws)
			ws->bdw->removeRigidBody(bbody);
		WarSpace *ws2 = *cs->w;
		if(ws2)
			ws2->bdw->addRigidBody(bbody);*/
		bbody->setWorldTransform(btTransform(btqc(rot), btvc(pos)));
		bbody->setLinearVelocity(btvc(velo));
		bbody->setAngularVelocity(btvc(omg));
	}

//	cs->w->addent(this);
	w = cs->w;

}



void Entity::Props::push_back(gltestp::dstring &s){
	if(nstr < numof(str))
		str[nstr++] = s;
}

Entity::Props::iterator Entity::Props::begin(){
	return str;
}

Entity::Props::iterator Entity::Props::end(){
	return &str[nstr];
}



/* estimate target position based on distance and approaching speed */
int estimate_pos(Vec3d &ret, const Vec3d &pos, const Vec3d &velo, const Vec3d &srcpos, const Vec3d &srcvelo, double speed, const WarField *w){
	double dist;
	Vec3d grv;
	double posy;
	double h = 0.;
	double h0 = 0.;
	if(w){
		Vec3d mid = (pos + srcpos) * .5;
		Vec3d evelo = (pos - srcpos).norm() * -speed;
		grv = w->accel(mid, evelo);
	}
	else
		VECNULL(grv);
	dist = (pos - srcpos).len();
	posy = pos[1] + (velo[1] + grv[1]) * dist / speed;
/*	(*ret)[0] = pos[0] + (velo[0] - srcvelo[0] + grv[0] * dist / speed / 2.) * dist / speed;
	(*ret)[1] = posy - srcvelo[1] * dist / speed;
	(*ret)[2] = pos[2] + (velo[2] - srcvelo[2] + grv[2] * dist / speed / 2.) * dist / speed;*/
	ret = pos + (velo - srcvelo + grv * dist / speed / 2.) * dist / speed;
	return 1;
}


static bool strless(const char *a, const char *b){
	return strcmp(a, b) < 0;
}

/// Using Constructor on First Use idiom, described in C++ FAQ lite.
EntityCommand::MapType &EntityCommand::ctormap(){
	static EntityCommand::MapType s(strless);
	return s;
}


bool EntityCommand::derived(EntityCommandID)const{return false;}
IMPLEMENT_COMMAND(HaltCommand, "Halt")
IMPLEMENT_COMMAND(AttackCommand, "Attack")

AttackCommand::AttackCommand(HSQUIRRELVM v, Entity &){
	int argc = sq_gettop(v);
	if(argc < 3)
		throw SQFArgumentError();
	Entity *ent;
	if(sqa_refobj(v, (SQUserPointer*)&ent, NULL, 3)){
		ents.insert(ent);
	}
}

IMPLEMENT_COMMAND(ForceAttackCommand, "ForceAttack")
IMPLEMENT_COMMAND(MoveCommand, "Move")
IMPLEMENT_COMMAND(ParadeCommand, "Parade")
IMPLEMENT_COMMAND(DeltaCommand, "Delta")
IMPLEMENT_COMMAND(DockCommand, "Dock")

IMPLEMENT_COMMAND(SetAggressiveCommand, "SetAggressive")
IMPLEMENT_COMMAND(SetPassiveCommand, "SetPassive")

IMPLEMENT_COMMAND(WarpCommand, "Warp")

IMPLEMENT_COMMAND(RemainDockedCommand, "RemainDocked")

RemainDockedCommand::RemainDockedCommand(HSQUIRRELVM v, Entity &){
	int argc = sq_gettop(v);
	if(argc < 3)
		throw SQFArgumentError();
	SQBool b;
	if(SQ_FAILED(sq_getbool(v, 3, &b)))
		throw SQFArgumentError();
	enable = b;
}

MoveCommand::MoveCommand(HSQUIRRELVM v, Entity &){
	int argc = sq_gettop(v);
	if(argc < 2)
		throw SQFArgumentError();
	SQUserPointer typetag;
	if(OT_INSTANCE != sq_gettype(v, 3) || (sq_gettypetag(v, 3, &typetag), typetag != tt_Vec3d))
		throw SQFError(_SC("Incompatible argument type"));
	sq_pushstring(v, _SC("a"), -1);
	if(SQ_FAILED(sq_get(v, 3)))
		throw SQFError(_SC("Corrupt Vec3d data"));
	Vec3d *pvec3;
	sq_getuserdata(v, -1, (SQUserPointer*)&pvec3, NULL);
	destpos = *pvec3;
}

WarpCommand::WarpCommand(HSQUIRRELVM v, Entity &e){
	int argc = sq_gettop(v);
	if(argc < 3)
		throw SQFArgumentError();
	const SQChar *destname;
	if(SQ_SUCCEEDED(sq_getstring(v, 3, &destname))){
		WarField *w = e.w;
		CoordSys *pa = NULL;
		CoordSys *pcs;
		double landrad;
		double dist, cost;
		extern coordsys *g_galaxysystem;
		Universe *u = w->cs->findcspath("/")->toUniverse();
		teleport *tp = u->ppl->findTeleport(destname, TELEPORT_WARP);
		if(tp){
			destpos = tp->pos;
			destcs = tp->cs;
		}
		else if(pa = w->cs->findcspath(destname)){
			destpos = vec3_000;
			destcs = pa;
		} 
		else
			throw SQFError();
	}
	else if(sqa_refobj(v, (SQUserPointer*)&destcs, NULL, 3)){
		destpos = vec3_000;
	}
	else
		throw SQFArgumentError();
	SQUserPointer typetag;
	if(OT_INSTANCE != sq_gettype(v, 4) || (sq_gettypetag(v, 4, &typetag), typetag != tt_Vec3d))
		throw SQFError(_SC("Incompatible argument type"));
	sq_pushstring(v, _SC("a"), -1);
	if(SQ_FAILED(sq_get(v, 4)))
		throw SQFError(_SC("Corrupt Vec3d data"));
	Vec3d *pvec3;
	sq_getuserdata(v, -1, (SQUserPointer*)&pvec3, NULL);
	destpos = *pvec3;
}
