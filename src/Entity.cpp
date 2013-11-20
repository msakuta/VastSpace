/** \file
 * \brief Implements Entity class and its collaborative classes.
 */
#include "Entity.h"
#include "Application.h"
#include "EntityCommand.h"
#include "Game.h"
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
#include "btadapt.h"
#include "sqadapt.h"
#include "stellar_file.h"
#include "glw/PopupMenu.h"
#include <btBulletDynamicsCommon.h>
#include <sstream>
#include <iostream>
#include <fstream>

#define DEBUG_ENTERFIELD 0
#define DEBUG_TRANSIT_CS 0


Entity::Entity(Game *game) : st(game), w(NULL), bbody(NULL){}

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
	bbody(NULL),
	enteringField(false)
{
	// Assume Object IDs in the upper half of the value range reserved for
	// the client side objects.
	if(UINT_MAX / 2 < id){
		// Client side objects should be remembered by the Game object for deleting.
		Game::ObjSet *objSet = game->getClientObjSet();
		if(objSet){
			objSet->insert(this);
		}
	}

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
	assert(UINT_MAX / 2 < id || game->isClientDeleting() || game->isServer());
	if(w)
		leaveField(w);
	if(bbody){
/*		WarSpace *ws;
		if(w && (ws = *w) && ws->bdw)
			ws->bdw->removeRigidBody(bbody);*/
		delete bbody;
		bbody = NULL;
	}
	if(game->isServer()){
		if(game->sqvm){
			HSQUIRRELVM v = game->sqvm;
			StackReserver sr(v);

			// Invoke the delete hook that may be defined in the Squirrel VM.
			sq_pushroottable(v);
			sq_pushstring(v, _SC("hook_delete_Entity"), -1);
			if(SQ_SUCCEEDED(sq_get(v, -2))){
				sq_pushroottable(v);
				sq_pushobj(v, this);
				if(SQ_FAILED(sq_call(v, 2, SQFalse, SQTrue)))
					CmdPrint("Squirrel error: hook_delete_Entity is not a function"); // root func
			}
			sq_poptop(v);
		}
	}
}

void Entity::init(){
	health = getMaxHealth();
}


Entity *Entity::create(const char *cname, WarField *w){
	EntityStatic *ctor = NULL;;
	for(EntityCtorMap::iterator it = entityCtorMap().begin(); it != entityCtorMap().end(); it++) if(!strcmp(it->first, cname)){
		ctor = it->second;
		break;
	}
	if(!ctor){
/*		HSQUIRRELVM v = w->game->sqvm;
		do{
			StackReserver sr(v);
			sq_pushstring(v, "classRegister", -1);
			if(SQ_FAILED(sq_get(v, 1)))
				break;
			sq_pushstring(v, cname, -1);
			if(SQ_FAILED(sq_get(v, -2)))
				break;

		}while(0);*/
		return NULL;
	}
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

Entity *Entity::EntityStaticBase::stcreate(Game *game){
	return NULL;
}

void Entity::EntityStaticBase::destroy(Entity *){
}

const SQChar *Entity::EntityStaticBase::sq_classname(){
	return _SC("Entity");
}

static SQInteger sqf_Entity_constructor(HSQUIRRELVM v){
	SQUserPointer squp;
	if(SQ_FAILED(sq_getinstanceup(v, 1, &squp, NULL)))
		return sq_throwerror(v, _SC("No instance UP"));
	new(squp) WeakPtr<Entity>;
	return 0;
}

/// \brief The release hook of Entity that clears the weak pointer.
///
/// \param size is always 0?
static SQInteger sqh_release(SQUserPointer p, SQInteger size){
	((WeakPtr<Entity>*)p)->~WeakPtr<Entity>();
	return 1;
}

static SQInteger sqf_Entity_tostring(HSQUIRRELVM v){
	Entity *p = Entity::sq_refobj(v, 1);

	// It's not uncommon that a customized Squirrel code accesses a destroyed object.
	if(!p){
		sq_pushstring(v, _SC("(deleted)"), -1);
		return 1;
	}

	sq_pushstring(v, gltestp::dstring() << "{" << p->classname() << ":" << p->getid() << "}", -1);
	return 1;
}

void Entity::sq_pushobj(HSQUIRRELVM v, Entity *e){
	sq_pushroottable(v);

	// Using literal "Entity" here works to some extent, but we should create an instance of
	// derived class (in Squirrel VM) instead of always creating Entity.
	// It makes difference if you add a function in the derived class.
	// It doesn't make difference in _get or _set metamethods, because they are virtual in C++
	// way, which means derived methods are always called.
	// It means you must override classname() to return Squirrel class name in order to enable
	// added functions in derived class in Squirrel scripts.
	sq_pushstring(v, e ? e->classname() : "Entity", -1);

	if(SQ_FAILED(sq_get(v, -2)))
		throw SQFError("Something's wrong with Entity class definition.");
	if(SQ_FAILED(sq_createinstance(v, -1)))
		throw SQFError("Something's wrong with Entity class instance.");
	SQUserPointer p;
	if(SQ_FAILED(sq_getinstanceup(v, -1, &p, NULL)) || !p)
		throw SQFError("Something's wrong with Squirrel Class Instace of Entity.");
	new(p) WeakPtr<Entity>(e);
	sq_setreleasehook(v, -1, sqh_release);
	sq_remove(v, -2); // Remove Class
	sq_remove(v, -2); // Remove root table
}

Entity *Entity::sq_refobj(HSQUIRRELVM v, SQInteger idx){
	SQUserPointer up;

	// It's valid to have a null object assigned to an Entity reference.
	if(sq_gettype(v, idx) == OT_NULL)
		return NULL;

	// If the instance does not have a user pointer, it's a serious exception that might need some codes fixed.
	if(SQ_FAILED(sq_getinstanceup(v, idx, &up, NULL)) || !up)
		throw SQFError("Something's wrong with Squirrel Class Instace of Entity.");
	return *(WeakPtr<Entity>*)up;
}

SQInteger Entity::sqf_Entity_get(HSQUIRRELVM v){
	try{
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);

	// Retrieve Entity pointer
	Entity *p = Entity::sq_refobj(v, 1);

	// The "alive" property is always accessible even though the actual object is destroyed.
	if(!strcmp(wcs, _SC("alive"))){
		sq_pushbool(v, p ? SQTrue : SQFalse);
		return 1;
	}

	// It's not uncommon that a customized Squirrel code accesses a destroyed object.
	if(!p)
		return sq_throwerror(v, _SC("The object being accessed is destructed in the game engine"));

		return p->sqGet(v, wcs);
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.what());
	}
}

SQInteger Entity::sqGet(HSQUIRRELVM v, const SQChar *name)const{
	if(!strcmp(name, _SC("cs"))){
		CoordSys::sq_pushobj(v, const_cast<CoordSys*>(w->cs));
		return 1;
	}
	// Intrinsic aggregate types such as Vec3d and Quatd are intentionally excluded from property access
	// because I thought it could lead to counter-intuitive results in compound assignment operators.
	// For instance, what if we write "Entity.pos += Vec3d(1,0,0)" in a Squirrel script?
	// If it's expanded like "Entity.pos = Entity.pos + Vec3d(1,0,0)", there's no problem.
	// But we cannot be sure that it's not expanded like "local v = Entity.pos; v += Vec3d(1,0,0)".
	// It seems that the former is the case, so we can revive the property accessors for those member
	// variables.  Method-formed accessors (like getpos()) are still provided for compatibility.
	else if(!scstrcmp(name, _SC("pos"))){
		SQVec3d r(pos);
		r.push(v);
		return 1;
	}
	else if(!scstrcmp(name, _SC("velo"))){
		SQVec3d r(velo);
		r.push(v);
		return 1;
	}
	else if(!scstrcmp(name, _SC("rot"))){
		SQQuatd r(rot);
		r.push(v);
		return 1;
	}
	else if(!scstrcmp(name, _SC("omg"))){
		SQVec3d r(omg);
		r.push(v);
		return 1;
	}
	else if(!strcmp(name, _SC("race"))){
		sq_pushinteger(v, race);
		return 1;
	}
/*	else if(!strcmp(wcs, _SC("next"))){
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
	}*/
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
	else if(!strcmp(name, _SC("dockedEntList"))){
		const Docker *d = getDocker();
		if(!d){
			sq_pushnull(v);
			return 1;
		}
		const WarField::EntityList &el = d->el;
		sq_newarray(v, el.size()); // root Entity array
		int idx = 0;
		for(WarField::EntityList::const_iterator it = el.begin(); it != el.end(); it++) if(*it){
			Entity *e = *it;
			sq_pushinteger(v, idx); // root Entity array idx
			Entity::sq_pushobj(v, e); // root Entity array idx instance
			sq_set(v, -3); // root Entity array
			idx++;
	//		sq_setinstanceup(v, -1, p->w->el);
		}
		return 1;
	}
	else if(!strcmp(name, _SC("docker"))){
		// Returns a Docker object if available.
		const Docker *d = getDocker();
		if(!d){
			sq_pushnull(v);
			return 1;
		}
		sq_pushroottable(v);
		sq_pushstring(v, _SC("Docker"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		d->sq_pushobj(v, const_cast<Docker*>(d));
		return 1;
	}
	else if(!strcmp(name, _SC("builder"))){
		const Builder *b = getBuilder();
		if(!b){
			sq_pushnull(v);
			return 1;
		}
		// Just push itself, since we multiple inherited Builder.
		sq_push(v, 1);
		return 1;
	}
	else if(!strcmp(name, _SC("enemy"))){
		SQUserPointer o;
		if(!this || !enemy){
			sq_pushnull(v);
			return 1;
		}
		Entity::sq_pushobj(v, enemy);
		return 1;
	}
	else if(!strcmp(name, _SC("health"))){
		SQUserPointer o;
		if(!this){
			sq_pushnull(v);
			return 1;
		}
		sq_pushfloat(v, SQFloat(getHealth()));
		return 1;
	}
	else if(!scstrcmp(name, _SC("maxhealth"))){
		SQUserPointer o;
		if(!this){
			sq_pushnull(v);
			return 1;
		}
		sq_pushfloat(v, SQFloat(getMaxHealth()));
		return 1;
	}
	else if(!strcmp(name, _SC("classname"))){
		sq_pushstring(v, classname(), -1);
		return 1;
	}
	else if(!strcmp(name, _SC("id"))){
		sq_pushinteger(v, getid());
		return 1;
	}
	else{
		// Try to delegate getter method to Builder.
		// I don't have an idea how to achieve this without knowledge of descendants.
		if(const Builder *builder = getBuilder()){
			SQInteger ret = builder->sq_get(v, name);
			if(ret != 0)
				return ret;
		}
		return sq_throwerror(v, gltestp::dstring("The index \"") << name << _SC("\" was not found in members of \"") << classname() << _SC("\" in a query"));
	}
}

SQInteger Entity::sqf_Entity_set(HSQUIRRELVM v){
	if(sq_gettop(v) < 3)
		return SQ_ERROR;

	// Retrieve Entity pointer
	Entity *p = Entity::sq_refobj(v, 1);

	// It's not uncommon that a customized Squirrel code accesses a destroyed object.
	if(!p)
		return sq_throwerror(v, _SC("The object being accessed is destructed in the game engine"));

	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);

	return p->sqSet(v, wcs);
}

SQInteger Entity::sqSet(HSQUIRRELVM v, const SQChar *name){
	if(!strcmp(name, _SC("race"))){
		SQInteger retint;
		if(SQ_FAILED(sq_getinteger(v, 3, &retint)))
			return SQ_ERROR;
		race = int(retint);
		return 0;
	}
	else if(!scstrcmp(name, _SC("pos"))){
		SQVec3d r;
		r.getValue(v);
		pos = r.value;
		return 0;
	}
	else if(!scstrcmp(name, _SC("velo"))){
		SQVec3d r;
		r.getValue(v);
		velo = r.value;
		return 0;
	}
	else if(!scstrcmp(name, _SC("rot"))){
		SQQuatd r;
		r.getValue(v);
		rot = r.value;
		return 0;
	}
	else if(!scstrcmp(name, _SC("omg"))){
		SQVec3d r;
		r.getValue(v);
		omg = r.value;
		return 0;
	}
	else if(!strcmp(name, _SC("enemy"))){
		Entity *o = Entity::sq_refobj(v, 3);
		enemy = o;
		return 1;
	}
	else if(!scstrcmp(name, _SC("health"))){
		SQFloat f;
		if(SQ_FAILED(sq_getfloat(v, 3, &f)))
			return sq_throwerror(v, _SC("Non-arithmetic value is being assigned to health"));
		health = f;
		return 1;
	}
	else{
		// Try to delegate setter method to Builder.
		if(Builder *builder = getBuilder()){
			SQInteger ret = builder->sq_set(v, name);
			if(ret != SQ_ERROR)
				return ret;
		}
		return sq_throwerror(v, gltestp::dstring("The index \"") << name << _SC("\" was not found in members of \"") << classname() << _SC("\" in an assignment"));
	}
}

static SQInteger sqf_Entity_command(HSQUIRRELVM v){
	try{
		Entity *p = Entity::sq_refobj(v);
		if(!p) // It should be valid to send a command to destroyed object; it's just ignored.
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

		// We shouldn't call this function at all!
		// You should have a parent object (CoordSys or Docker from Squirrel VM's perspective) whenever you want to
		// create an Entity within. Use the parent's addent method to avoid leaks.
		assert(0);

/*		if(game->player && game->player->cs && !game->player->cs->w){
			const_cast<CoordSys*>(game->player->cs)->w = new WarSpace(game);
			const_cast<CoordSys*>(game->player->cs)->w->cs = const_cast<CoordSys*>(game->player->cs);
		}*/

		Entity *pt = Entity::create(classname, NULL);

		if(!pt)
			return sq_throwerror(v, _SC("Undefined Entity class name"));
		Entity::sq_pushobj(v, pt);
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.description);
	}
	return 1;
}

static SQInteger sqf_Entity_kill(HSQUIRRELVM v){
	try{
		Entity *p = Entity::sq_refobj(v);
		if(!p)
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

/// Update method accessible from Squirrel scripts.
static SQInteger sqf_Entity_update(HSQUIRRELVM v){
	try{
		Entity *p = Entity::sq_refobj(v);
		if(!p)
			return 0;
		SQFloat dt;
		if(SQ_FAILED(sq_getfloat(v, 2, &dt)))
			return sq_throwerror(v, _SC("Type mismatch for the second argument of update()"));

		// We should really rename this to update() though its range of influence is so large.
		p->anim(dt);
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.description);
	}
	return 1;
}

/// \brief Returns Squirrel closure handle to define popup menu item for this Entity dynamically.
///
/// Inherited classes of Entity can hard-code their own popup menu items, but also add script
/// generated ones defined in the initialization script files.  In such case, user just override
/// this function to return Squirrel object handle to a Squirrel function.
HSQOBJECT Entity::getSqPopupMenu(){
	HSQOBJECT ret;
	sq_resetobject(&ret);
	return ret;
}

/// \brief Returns Squirrel closure handle to define cockpit view position and orientation.
HSQOBJECT Entity::getSqCockpitView()const{
	HSQOBJECT ret;
	sq_resetobject(&ret);
	return ret;
}

void setpos(Entity *e, const Vec3d &v){
	e->setPosition(&v);
}

void setvelo(Entity *e, const Vec3d &v){
	e->setPosition(NULL, NULL, &v);
}

void setrot(Entity *e, const Quatd &v){
	e->setPosition(NULL, &v);
}

void setomg(Entity *e, const Vec3d &v){
	e->setPosition(NULL, NULL, NULL, &v);
}

/// \brief Squirrel closure to set Entity::bbody related parameters.
/// \param Class can be Entity
/// \param MType can be Vec3d or Quatd.
/// \param setter function to set value natively.
/// \param sq_refobj function to return an object contained in a Squirrel instance.
template<typename Class, typename MType, void setter(Class *p, const MType &), Class *sq_refobj(HSQUIRRELVM v, SQInteger idx)>
SQInteger sqf_setintrinsic3(HSQUIRRELVM v){
	try{
		Class *p = sq_refobj(v, 1);
		SQIntrinsic<MType> r;
		r.getValue(v, 2);
		setter(p, r.value);
		return 0;
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.what());
	}
}



bool Entity::EntityStaticBase::sq_define(HSQUIRRELVM v){
	// Define class Entity
	sq_pushstring(v, _SC("Entity"), -1);
	sq_newclass(v, SQFalse);
	sq_settypetag(v, -1, SQUserPointer(sq_classname()));
	sq_setclassudsize(v, -1, sizeof(WeakPtr<Entity>));
	register_closure(v, _SC("constructor"), sqf_Entity_constructor);
	register_closure(v, _SC("getpos"), sqf_getintrinsic2<Entity, Vec3d, membergetter<Entity, Vec3d, &Entity::pos>, sq_refobj >);
	register_closure(v, _SC("setpos"), sqf_setintrinsic3<Entity, Vec3d, setpos, sq_refobj>);
	register_closure(v, _SC("getvelo"), sqf_getintrinsic2<Entity, Vec3d, membergetter<Entity, Vec3d, &Entity::velo>, sq_refobj >);
	register_closure(v, _SC("setvelo"), sqf_setintrinsic3<Entity, Vec3d, setvelo, sq_refobj>);
	register_closure(v, _SC("getrot"), sqf_getintrinsic2<Entity, Quatd, membergetter<Entity, Quatd, &Entity::rot>, sq_refobj >);
	register_closure(v, _SC("setrot"), sqf_setintrinsic3<Entity, Quatd, setrot, sq_refobj>);
	register_closure(v, _SC("getomg"), sqf_getintrinsic2<Entity, Vec3d, membergetter<Entity, Vec3d, &Entity::omg>, sq_refobj >);
	register_closure(v, _SC("setomg"), sqf_setintrinsic3<Entity, Vec3d, setomg, sq_refobj>);
	register_closure(v, _SC("_get"), sqf_Entity_get);
	register_closure(v, _SC("_set"), sqf_Entity_set);
	register_closure(v, _SC("_tostring"), sqf_Entity_tostring, 1);
	register_closure(v, _SC("command"), sqf_Entity_command);
	register_closure(v, _SC("create"), sqf_Entity_create);
	register_closure(v, _SC("kill"), sqf_Entity_kill);
	register_closure(v, _SC("update"), sqf_Entity_update);
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
	//
	// Update: The above statements are correct to some extent, but fail to consider initialization order
	// fiasco induced by constructor of global variables.  We must trace inheritance tree to ensure the
	// super class is defined first, but we cannot do that here.  The correct timing to define Squirrel
	// classes is just after an extension module is loaded, so the procedure is moved to sqadapt.cpp.
/*	if(sqa::g_sqvm){
		sq_pushroottable(g_sqvm);
		ctor->sq_define(sqa::g_sqvm);
		sq_poptop(g_sqvm);
	}*/

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
	// DO NOT change order or insert other variables before w and pos,
	// they're assumed in that position in Game::idUnmap().
	sc.o << w;
	sc.o << pos;
	sc.o << velo;
	sc.o << omg;
	sc.o << rot;
	sc.o << mass;
	sc.o << moi;
	sc.o << health;
//	sc.o << next /*<< selectnext*/;
	sc.o << enemy;
	sc.o << race;
	sc.o << controller;
	sc.o << otflag;
}

void Entity::unserialize(UnserializeContext &sc){
	WarField *oldwf = w;

	st::unserialize(sc);
	// DO NOT change order or insert other variables before w and pos,
	// they're assumed in that position in Game::idUnmap().
	sc.i >> w;
	sc.i >> pos;
	sc.i >> velo;
	sc.i >> omg;
	sc.i >> rot;
	sc.i >> mass;
	sc.i >> moi;
	sc.i >> health;
//	sc.i >> next /*>> selectnext*/;
	sc.i >> enemy;
	sc.i >> race;
	sc.i >> controller;
	sc.i >> otflag;

	// Postprocessing calls leave/enterField according to frame deltas.
	if(oldwf != w){
		if(oldwf)
			leaveField(oldwf);
		if(w)
			enteringField = true;
//			enterField(w);
	}
}

void Entity::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	st::dive(sc, method);
	if(controller)
		controller->dive(sc, method);
/*	if(next)
		next->dive(sc, method);*/
}

double Entity::getHealth()const{return health;}

double Entity::getMaxHealth()const{return 100.;}

/// \brief Called when this Entity is entering a WarField, just after adding to Entity list in the WarField.
///
/// You can use this function as an opportunity to response to WarField::addent().
///
/// This function is also called when creating an Entity.
///
/// \param aw The WarField to which this Entity is entering.
void Entity::enterField(WarField *aw){
	WarSpace *ws = *aw;
	if(ws){
		addRigidBody(ws);
	}

	// Prevent duplicate addition to the Bullet Dynamics World by clearing the flag, regardless of reason.
	enteringField = false;

#if DEBUG_ENTERFIELD
	std::ofstream of("debug.log", std::ios_base::app);
	of << game->universe->global_time << ": enterField: " << (game->isServer()) << " {" << classname() << ":" << id << "} to " << aw->cs->getpath()
		<< " with bbody " << bbody << std::endl;
#endif
}

/// \brief Protected virtual function for adding rigid body to the WarSpace.
///
/// Derived classes can override to implement custom mask values and such.
///
/// \param ws The WarSpace to add the rigid body to.
void Entity::addRigidBody(WarSpace *ws){
	if(ws->bdw && bbody){
		ws->bdw->addRigidBody(bbody);
		bbody->activate(); // Is it necessary?
	}
}

/// \brief Called when this Entity is leaving a WarField, after removing from Entity list in the WarField.
///
/// Added for symmetry with enterField().
///
/// This function is also called when deleting an Entity.
///
/// \param ws The WarField from which this Entity is leaving.
void Entity::leaveField(WarField *aw){
	if(!aw)
		return;
	WarSpace *ws = *aw;
	if(ws)
		removeRigidBody(ws);
#if DEBUG_ENTERFIELD
	std::ofstream of("debug.log", std::ios_base::app);
	of << game->universe->global_time << ": leaveField: " << (game->isServer()) << " {" << classname() << ":" << id << "} from " << aw->cs->getpath()
		<< " with bbody " << bbody << std::endl;
#endif
}

/// \brief Protected virtual function for removing rigid body from the WarSpace.
///
/// Derived classes can override to customize something corresponding to addRigidBody().
///
/// \param ws The WarSpace to remove the rigid body from.
void Entity::removeRigidBody(WarSpace *ws){
	if(ws->bdw && bbody)
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
void Entity::beginControl(){
	HSQUIRRELVM v = game->sqvm;
	StackReserver sr(v);
	sq_pushroottable(v);
	sq_pushstring(v, _SC("beginControl"), -1);
	if(SQ_FAILED(sq_get(v, -2)))
		return;
	sq_pushstring(v, classname(), -1);
	if(SQ_FAILED(sq_get(v, -2)))
		return;
	sq_pushroottable(v);
	sq_call(v, 1, SQFalse, SQTrue);
	return;
}
void Entity::control(const input_t *i, double){inputs = *i;}
void Entity::endControl(){
	HSQUIRRELVM v = game->sqvm;
	StackReserver sr(v);
	sq_pushroottable(v);
	sq_pushstring(v, _SC("endControl"), -1);
	if(SQ_FAILED(sq_get(v, -2)))
		return;
	sq_pushstring(v, classname(), -1);
	if(SQ_FAILED(sq_get(v, -2)))
		return;
	sq_pushroottable(v);
	sq_call(v, 1, SQFalse, SQTrue);
	return;
}
unsigned Entity::analog_mask(){return 0;}

/// \brief Returns cockpit position and orientation when the camera is chasing.
///
/// By default, this function try to run a Squirrel function returned by getSqCockpitView(),
/// and loads position and rotation from its returned table, although this behavior can
/// be overridden in derived classes.
void Entity::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	HSQOBJECT sqCockpitView = getSqCockpitView();
	int ret = 0;
	if(sq_isnull(sqCockpitView)){
		pos = this->pos;
		rot = this->rot;
		return;
	}
	try{ // Call Squirrel defined cockpit view position logic.
		// One may concern the overhead to call Squirrel function, but
		// this method is called at most once a frame, which does not
		// impact much of execution time dominated by drawing methods.
		HSQUIRRELVM v = game->sqvm;
		StackReserver sr(v);

		// Call cockpitView((root), this, seatid) -> {pos, rot}
		sq_pushobject(v, sqCockpitView);
		sq_pushroottable(v);
		Entity::sq_pushobj(v, const_cast<Entity*>(this)); // Squirrel cannot handle const qualifier!
		sq_pushinteger(v, seatid);
		if(SQ_FAILED(sq_call(v, 3, SQTrue, SQTrue)))
			throw SQFError("Exception in sq_call");

		sq_pushstring(v, _SC("pos"), -1); // func table "pos"
		if(SQ_FAILED(sq_get(v, -2)))
			throw SQFError("Couldn't get pos");
		SQVec3d sqv;
		sqv.getValue(v);
		sq_poptop(v); // func table

		sq_pushstring(v, _SC("rot"), -1); // func array array[i] "proc"
		if(SQ_FAILED(sq_get(v, -2)))
			throw SQFError("Couldn't get rot");
		SQQuatd sqq;
		sqq.getValue(v);
		sq_poptop(v); // func table

		sq_poptop(v); // func

		pos = sqv.value;
		rot = sqq.value;
	}
	catch(SQFError &e){
		CmdPrint(gltestp::dstring("Entity::cockpitView Error: ") << e.what());
		pos = this->pos;
		rot = this->rot;
	}
}

int Entity::numCockpits()const{return 1;}
void Entity::draw(WarDraw *){}
void Entity::drawtra(WarDraw *){}
void Entity::drawHUD(WarDraw *){}
void Entity::drawCockpit(WarDraw *){}
void Entity::drawOverlay(WarDraw *){}
bool Entity::solid(const Entity *)const{return true;} // Default is to check hits
/// \param b The Bullet object that hit me.
/// \param hitpart The hit part ID of this object.
void Entity::onBulletHit(const Bullet *b, int hitpart){}
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
	ret.push_back(gltestp::dstring("Health: ") << dstring(getHealth()) << "/" << dstring(getMaxHealth()));
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

bool Entity::unlink(const Observable*){
	return true;
}

int Entity::tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *fret, Vec3d *retp, Vec3d *retnormal){
	Vec3d retpos;
	bool bret = !!jHitSpherePos(pos, this->getHitRadius() + rad, start, dir, 1., fret, &retpos);
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
		return 0;
	}
	health -= damage;
	return 1;
}

// This function's purpose is unclear.
void Entity::bullethole(sufindex, double, const Vec3d &pos, const Quatd &rot){}

/// \brief Gets list of menu items for right click popup menu.
///
/// By default, it tries to run a Squirrel function and loads menu items from
/// its returned array, although this behavior can be overridden in derived
/// classes.
int Entity::popupMenu(PopupMenu &list){
#ifndef DEDICATED // This function would never be called in dedicated server.
	HSQOBJECT sqPopupMenu = getSqPopupMenu();
	int ret = 0;
	if(sq_isnull(sqPopupMenu))
		return ret;
	try{ // Load the popup menu titles and functions from a Squirrel callback closure.
		HSQUIRRELVM v = game->sqvm;
		StackReserver sr(v);
		sq_pushobject(v, sqPopupMenu);
		sq_pushroottable(v);
		Entity::sq_pushobj(v, this);
		if(SQ_FAILED(sq_call(v, 2, SQTrue, SQTrue)))
			throw SQFError("Exception in sq_call");

		SQInteger size = sq_getsize(v, -1); // func array
		for(SQInteger i = 0; i < size; i++){
			sq_pushinteger(v, i); // func array i
			if(SQ_FAILED(sq_get(v, -2)))
				throw SQFError(gltestp::dstring() << "Couldn't get array[" << int(i) << "]");

			// If the table contains a member named separator, make it a solid separator.
			sq_pushstring(v, _SC("separator"), -1); // func array array[i] "separator"
			if(SQ_SUCCEEDED(sq_get(v, -2))){
				list.appendSeparator();
				sq_poptop(v); // func array array[i]
				sq_poptop(v); // func array
				continue;
			}

			sq_pushstring(v, _SC("title"), -1); // func array array[i] "title"
			const SQChar *title;
			if(SQ_FAILED(sq_get(v, -2)) || SQ_FAILED(sq_getstring(v, -1, &title)))
				throw SQFError("Couldn't get title");
			sq_poptop(v); // func array array[i]

			sq_pushstring(v, _SC("proc"), -1); // func array array[i] "proc"
			HSQOBJECT sqProc;
			bool procDefined = false;
			if(SQ_SUCCEEDED(sq_get(v, -2))){
				procDefined = SQ_SUCCEEDED(sq_getstackobj(v, -1, &sqProc));
				sq_poptop(v); // func array array[i]
			}

			sq_pushstring(v, _SC("cmd"), -1); // func array array[i] "cmd"
			const SQChar *sqCmd;
			bool cmdDefined = false;
			if(SQ_SUCCEEDED(sq_get(v, -2))){
				cmdDefined = SQ_SUCCEEDED(sq_getstring(v, -1, &sqCmd));
				sq_poptop(v); // func array array[i]
			}

			sq_pushstring(v, _SC("key"), -1); // func array array[i] "key"
			const SQChar *sqKey = NULL;
			if(SQ_SUCCEEDED(sq_get(v, -2))){
				sq_getstring(v, -1, &sqKey);
				sq_poptop(v); // func array array[i]
			}

			sq_pushstring(v, _SC("unique"), -1); // func array array[i] "unique"
			SQBool sqUnique = SQTrue;
			if(SQ_SUCCEEDED(sq_get(v, -2))){
				sq_getbool(v, -1, &sqUnique);
				sq_poptop(v); // func array array[i]
			}

			if(procDefined){ // callback process is preceded over command string.
				PopupMenuItem *item = new PopupMenuItemClosure(sqProc, v);
				item->title = title;
				item->key = sqKey ? sqKey[0] : 0;
				list.append(item, sqUnique != SQFalse);
			}
			else if(cmdDefined){
				list.append(title, sqKey ? sqKey[0] : 0, sqCmd, sqUnique != SQFalse);
			}
			else
				throw SQFError(gltestp::dstring() << "Couldn't get neither proc nor cmd in table " << int(i));

			sq_poptop(v); // func array
		}
	}
	catch(SQFError &e){
		CmdPrint(gltestp::dstring("Entity::popupMenu Error: ") << e.what());
	}
	return ret;
#else
	return 0;
#endif
}

Warpable *Entity::toWarpable(){return NULL;}
Entity::Dockable *Entity::toDockable(){return NULL;}

void Entity::transit_cs(CoordSys *cs){
#if DEBUG_TRANSIT_CS // The nice thing about C++ is that #if directives need not to be placed at all return statements.
	std::ofstream of("debug.log", std::ios_base::app);
	of << game->universe->global_time << ": transit_cs start: " << game->isServer() << " {" << classname() << ":" << id << "} from " << w->cs->getpath() << " to " << cs->getpath() << std::endl;
	struct FuncExit{
		Entity *e;
		CoordSys *cs;
		~FuncExit(){
			std::ofstream of("debug.log", std::ios_base::app);
			of << e->game->universe->global_time << ": transit_cs end: " << e->game->isServer() << " {" << e->classname() << ":" << e->getid() << "}" << std::endl;
		}
	} fe;
	fe.e = this;
	fe.cs = cs;
#endif
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

	// Even in the process of transition, at any moment no two WarFields own the
	// same Entity.
	WarField *oldw = w;
	w->unlink(this);
	WarField::EntityList &el = w->entlist();
	w = NULL;
	leaveField(oldw);
	cs->w->addent(this);

}

/// \brief Just call anim().
void Entity::callServerUpdate(double dt){
	anim(dt);
}

/// \brief Calls clientUpdate() with preceding processes that are encapsulated in the class.
void Entity::callClientUpdate(double dt){
	// Check the flag that could be set in unserialize().
	if(enteringField){
		enterField(w);
		enteringField = false;
	}
	clientUpdate(dt);
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


//-----------------------------------------------------------------------------
//  EntityCommand implementation
//-----------------------------------------------------------------------------

static bool strless(const char *a, const char *b){
	return strcmp(a, b) < 0;
}

/// Using Constructor on First Use idiom, described in C++ FAQ lite.
EntityCommand::MapType &EntityCommand::ctormap(){
	static EntityCommand::MapType s(strless);
	return s;
}


bool EntityCommand::derived(EntityCommandID)const{return false;}

SerializableCommand *EntityCommand::toSerializable(){
	return NULL;
}

//-----------------------------------------------------------------------------
//  SerializableCommand implementation
//-----------------------------------------------------------------------------

SerializableCommand *SerializableCommand::toSerializable(){
	return this;
}

void SerializableCommand::serialize(SerializeContext &sc){
}

void SerializableCommand::unserialize(UnserializeContext &usc){
}

IMPLEMENT_COMMAND(HaltCommand, "Halt")
IMPLEMENT_COMMAND(AttackCommand, "Attack")

AttackCommand::AttackCommand(HSQUIRRELVM v, Entity &){
	int argc = sq_gettop(v);
	if(argc < 3)
		throw SQFArgumentError();
	Entity *ent = Entity::sq_refobj(v, 3);
	if(ent){
		ents.insert(ent);
	}
}

void AttackCommand::serialize(SerializeContext &sc){
	sc.o << unsigned(ents.size());
	for(std::set<Entity*>::iterator it = ents.begin(); it != ents.end(); ++it)
		sc.o << *it;
}

void AttackCommand::unserialize(UnserializeContext &usc){
	unsigned size;
	usc.i >> size;
	for(int i = 0; i < size; i++){
		Entity *pe;
		usc.i >> pe;
		ents.insert(pe);
	}
}

IMPLEMENT_COMMAND(ForceAttackCommand, "ForceAttack")
IMPLEMENT_COMMAND(MoveCommand, "Move")

void MoveCommand::serialize(SerializeContext &sc){
	sc.o << destpos;
}

void MoveCommand::unserialize(UnserializeContext &sc){
	sc.i >> destpos;
}

IMPLEMENT_COMMAND(ParadeCommand, "Parade")
IMPLEMENT_COMMAND(DeltaCommand, "Delta")
IMPLEMENT_COMMAND(DockCommand, "Dock")

IMPLEMENT_COMMAND(SetAggressiveCommand, "SetAggressive")
IMPLEMENT_COMMAND(SetPassiveCommand, "SetPassive")

IMPLEMENT_COMMAND(WarpCommand, "Warp")

void WarpCommand::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << destcs;
}

void WarpCommand::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> destcs;
}

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

void RemainDockedCommand::serialize(SerializeContext &sc){
	sc.o << enable;
}

void RemainDockedCommand::unserialize(UnserializeContext &sc){
	sc.i >> enable;
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
	Game *game = (Game*)sq_getforeignptr(v);
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
//		extern coordsys *g_galaxysystem;
		Player *player = game->player;
		teleport *tp = player ? player->findTeleport(destname, TELEPORT_WARP) : NULL;
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
	else if(destcs = CoordSys::sq_refobj(v, 3)){
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





ObserveEventID TransitEvent::sid = "TransitEvent";
ObserveEventID TransitEvent::id()const{return sid;}


//-----------------------------------------------------------------------------
//  CMRemainDockedCommand implementation
//-----------------------------------------------------------------------------

CMEntityCommand CMEntityCommand::s;

void CMEntityCommand::interpret(ServerClient &sc, UnserializeStream &uss){
	gltestp::dstring cmdname;
	uss >> cmdname;

	// First, find the EntityCommand static record of given name.
	// If it's not found, just ignore the message.
	EntityCommand::MapType::iterator it = EntityCommand::ctormap().find(cmdname);
	if(it == EntityCommand::ctormap().end())
		return;

	// Find the specified Entity. It can be destructed in the server.
	Entity *e;
	uss >> e;
	if(!e)
		return;

	// Only dispatch commands from those who have right privileges.
	Player *player = application.serverGame->players[sc.id];
	if(!player || player->race != e->race)
		return;

#if 1
	// Rest of the works are done in the function.
	it->second->unserializeFunc(*uss.usc, *e);
#else

	EntityCommand *com = it->second->newproc();
	assert(com);

	// Check if the EntityCommand is really a SerializableCommand.
	SerializableCommand *scom = com->toSerializable();
	if(!scom)
		return;

	// Unserialize from client stream.
	scom->unserialize(*uss.usc);

	// Actually sends the command to the Entity.
	e->command(scom);

	// Delete dynamically allocated command.
	it->second->deleteproc(scom);
#endif
}

void CMEntityCommand::send(Entity *e, SerializableCommand &com){
	// Safety net to avoid mistakenly tried SerializableCommand to send
	// from actually be sent.
	if(e->getGame()->isServer())
		return;
	std::stringstream ss;
	StdSerializeStream sss(ss);
	Serializable* visit_list = NULL;
	SerializeContext sc(sss, visit_list);
	sss.sc = &sc;
	sss << com.id();
	sss << e;
	com.serialize(sc);
	std::string str = ss.str();
	s.st::send(application, str.c_str(), str.size());
}
