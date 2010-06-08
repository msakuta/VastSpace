#include "sqadapt.h"
#include "cmd.h"
#include "Universe.h"
#include "Entity.h"
#include "Player.h"
#include "btadapt.h"
#include "EntityCommand.h"
#include "Scarry.h"
#include <squirrel.h>
#include <sqstdio.h>
#include <sqstdaux.h>
#include <sqstdmath.h>
//#include <../sqplus/sqplus.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef NDEBUG
#define verify(a) assert(a)
#else
#define verify(a) (a)
#endif

extern "C"{
#include <clib/c.h>
#include <clib/timemeas.h>
}

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.h>
#include <BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.h>

extern Universe universe;
extern Player pl;

/*DECLARE_INSTANCE_TYPE(Player)

namespace SqPlus{
template<>
void Push<const CoordSys>(SQVM *v, const CoordSys *cs){
	Push(v, (SQUserPointer)cs);
}
}*/

static const SQChar *CONSOLE_COMMANDS = _SC("console_commands");

int ::sqa_console_command(int argc, char *argv[], int *retval){
	HSQUIRRELVM &v = sqa::g_sqvm;
	sq_pushroottable(v); // root
	sq_pushstring(v, CONSOLE_COMMANDS, -1); // root "console_commands"
	if(SQ_FAILED(sq_get(v, -2))){ // root table
		sq_poptop(v);
		return 0;
	}
	sq_pushstring(v, argv[0], -1); // root table argv[0]
	if(SQ_FAILED(sq_get(v, -2))){ // root table closure
		sq_pop(v, 2);
		return 0;
	}

	// Pass all arguments as strings (no conversion is done beforehand).
	sq_pushroottable(v);
	for(int i = 1; i < argc; i++)
		sq_pushstring(v, argv[i], -1);

	// It's no use examining returned value in that case calling the function iteslf fails.
	if(SQ_FAILED(sq_call(v, argc, SQTrue, SQFalse)))
		return 0;

	// Assume returned value integer.
	int retint;
	if(SQ_SUCCEEDED(sq_getinteger(v, -1, &retint))){
		*retval = retint;
		return 2;
	}
	else{
		*retval = 0;
		return 1;
	}
}

namespace sqa{

HSQUIRRELVM g_sqvm;
static HSQUIRRELVM &v = g_sqvm;

// The Squirrel type tags can be any pointer that never overlap.
const SQUserPointer tt_Vec3d = "Vec3d";
const SQUserPointer tt_Quatd = "Quatd";
const SQUserPointer tt_Entity = "Entity";


static void sqf_print(HSQUIRRELVM v, const SQChar *s, ...) 
{ 
	va_list arglist; 
	va_start(arglist, s);
//	vwprintf(s, arglist);
	char cstr[1024];
	vsprintf(cstr, s, arglist);
	CmdPrint(cstr);
	va_end(arglist); 
} 


extern "C" int cmd_set(int argc, const char *argv[]);

static SQInteger sqf_set_cvar(HSQUIRRELVM v){
	SQInteger nargs = sq_gettop(v); //number of arguments
	SQObjectType ty = sq_gettype(v, 1);
	if(3 <= nargs && sq_gettype(v, 2) == OT_STRING && sq_gettype(v, 3) == OT_STRING){
		const SQChar *args[3];
		args[0] = _SC("set");
		sq_getstring(v, 2, &args[1]);
		sq_getstring(v, 3, &args[2]);
		cmd_set(3, const_cast<const char **>(args));
	}
	return 0;
}

static SQInteger sqf_get_cvar(HSQUIRRELVM v){
	SQInteger nargs = sq_gettop(v); //number of arguments
	if(2 == nargs && sq_gettype(v, 2) == OT_STRING){
		const SQChar *arg;
		sq_getstring(v, 2, &arg);
		const SQChar *ret = CvarGetString(arg);
		if(!ret){
			sq_pushnull(v);
			return 1;
		}
		size_t retlen = strlen(ret) + 1;
		sq_pushstring(v, ret, -1);
		delete[] ret;
		return 1;
	}
	else
		return SQ_ERROR;
}

static SQInteger sqf_cmd(HSQUIRRELVM v){
	SQInteger nargs = sq_gettop(v); //number of arguments
	if(2 != nargs)
		return SQ_ERROR;
	if(sq_gettype(v, 2) == OT_STRING){
		const SQChar *arg;
		sq_getstring(v, 2, &arg);
		CmdExec(arg);
	}
	return 0;
}

static SQInteger sqf_register_console_command(HSQUIRRELVM v){
	const SQChar *name;
	sq_getstring(v, 2, &name);
	sq_pushroottable(v); // root
	sq_pushstring(v, CONSOLE_COMMANDS, -1); // root "console_commands"

	// If "console_commands" table does not exist in the root table, create an empty one.
	if(SQ_FAILED(sq_get(v, -2))){ // root table
		sq_newtable(v); // root table
		sq_pushstring(v, CONSOLE_COMMANDS, -1); // root table "console_commands"
		sq_push(v, -2); // root table "console_commands" table
		if(SQ_FAILED(sq_createslot(v, -4))) // root table
			;
	}

	sq_push(v, 2); // root table name
	sq_push(v, 3); // root table name closure
	sq_newslot(v, -3, SQFalse); // root table
	return 0;
}

static SQInteger sqf_addent(HSQUIRRELVM v){
	if(sq_gettop(v) < 3)
		return SQ_ERROR;
	CoordSys *p;
	SQRESULT sr;
	if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
		return sr;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	
	Vec3d pos = vec3_000;
	if(OT_INSTANCE == sq_gettype(v, -1)){
		SQUserPointer typetag;
		sq_gettypetag(v, -1, &typetag);
		if(typetag == tt_Vec3d){
			Vec3d *pvec;
			sq_pushstring(v, _SC("a"), -1);
			sq_get(v, -2); // this classname vec3 vec3.a
			if(!SQ_FAILED(sq_getuserdata(v, -1, (SQUserPointer*)&pvec, NULL)))
				pos = *pvec;
		}
	}
	sq_pop(v, 2); // this classname

	const SQChar *arg;
	if(SQ_FAILED(sq_getstring(v, 2, &arg)))
		return SQ_ERROR;

	extern Player *ppl;
	WarField *&w = p->w;
	if(!w)
		w = new WarSpace(p)/*spacewar_create(cs, ppl)*/;
	Entity *pt = Entity::create(arg, w);
	if(pt){
		pt->setPosition(&pos);
/*		pt->pos = pos;
		if(pt->bbody){
			btTransform trans;
			trans.setIdentity();
			trans.setOrigin(btvc(pos));
			pt->bbody->setCenterOfMassTransform(trans);
		}*/
/*		pt->race = 5 < argc ? atoi(argv[5]) : 0;*/
	}
	else
		sq_throwerror(v, cpplib::dstring("addent: Unknown entity class name: %s") << arg);

	sq_pushroottable(v);
	sq_pushstring(v, _SC("Entity"), -1);
	sq_get(v, -2);
	sq_createinstance(v, -1);
	sqa_newobj(v, pt);
	return 1;
}

static SQInteger sqf_name(HSQUIRRELVM v){
	CoordSys *p;
	SQRESULT sr;
	if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
		return sr;
	sq_pushstring(v, p->name, -1);
	return 1;
}


static SQInteger sqf_Entity_get(HSQUIRRELVM v){
	Entity *p;
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	SQRESULT sr;
	if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
		return sr;
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
	else if(!strcmp(wcs, _SC("selectnext"))){
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
	}
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
		if(SQ_FAILED(sq_getinteger(v, 3, &p->race)))
			return SQ_ERROR;
		return 0;
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

		timemeas_t tm;
		TimeMeasStart(&tm);
		printf("Command: %lg\n", TimeMeasLap(&tm));
		EntityCommandCreatorFunc *func = EntityCommand::ctormap[s];

		if(func){
			EntityCommand *com = func(v, *p);
			p->command(com);
			delete com;
		}
		else
			return sq_throwerror(v, _SC("Undefined Entity Command"));
/*
	if(!strcmp(s, _SC("Halt"))){
		p->command(&HaltCommand(v, *p));
	}
	else if(!strcmp(s, _SC("Move"))){
		p->command(&MoveCommand(v, *p));
	}
	else if(!strcmp(s, _SC("Dock"))){
		p->command(&DockCommand(v, *p));
	}
	else if(!strcmp(s, _SC("Parade"))){
		p->command(&ParadeCommand(v, *p));
	}
	else if(!strcmp(s, _SC("SetAggressive"))){
		p->command(&SetAggressiveCommand(v, *p));
	}
	else if(!strcmp(s, _SC("SetPassive"))){
		p->command(&SetPassiveCommand(v, *p));
	}
	else if(!strcmp(s, _SC("Warp"))){
		p->command(&WarpCommand(v, *p));
	}
	else if(!strcmp(s, _SC("RemainDocked"))){
		p->command(&RemainDockedCommand(v, *p));
	}
	return 0;*/
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.description);
	}
	return 0;
}

static SQInteger sqf_CoordSys_get(HSQUIRRELVM v){
	CoordSys *p;
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	if(!sqa_refobj(v, (SQUserPointer*)&p))
		return SQ_ERROR;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!strcmp(wcs, _SC("entlist"))){
		if(!p->w || !p->w->el){
			sq_pushnull(v);
			return 1;
		}
		sq_pushroottable(v);
		sq_pushstring(v, _SC("Entity"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		sqa_newobj(v, p->w->el);
//		sq_setinstanceup(v, -1, p->w->el);
		return 1;
	}
	else
		return sqf_get<CoordSys>(v);
}

static SQInteger sqf_Universe_get(HSQUIRRELVM v){
	Universe *p;
	const SQChar *wcs;
	sq_getstring(v, -1, &wcs);
	if(!sqa_refobj(v, (SQUserPointer*)&p))
		return SQ_ERROR;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!strcmp(wcs, _SC("timescale"))){
		sq_pushfloat(v, p->timescale);
		return 1;
	}
	else if(!strcmp(wcs, _SC("global_time"))){
		sq_pushfloat(v, p->global_time);
		return 1;
	}
	else
		return sqf_CoordSys_get(v);
}


static SQInteger sqf_child(HSQUIRRELVM v){
	CoordSys *p;
	if(!sqa_refobj(v, (SQUserPointer*)&p))
		return SQ_ERROR;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!p->children){
		sq_pushnull(v);
		return 1;
	}
/*	int n;
	CoordSys *cs = p->children;
	for(n = 0; cs; n++, cs = cs->next){*/
		sq_pushroottable(v);
		sq_pushstring(v, _SC("CoordSys"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
	sqa_newobj(v, p->children);
//		sq_setinstanceup(v, -1, p->children);
//	}
	return 1;
}

static SQInteger sqf_next(HSQUIRRELVM v){
	CoordSys *p;
	if(!sqa_refobj(v, (SQUserPointer*)&p))
		return SQ_ERROR;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!p->next){
		sq_pushnull(v);
		return 1;
	}
	sq_pushroottable(v);
	sq_pushstring(v, _SC("CoordSys"), -1);
	sq_get(v, -2);
	sq_createinstance(v, -1);
	sqa_newobj(v, p->next);
//	sq_setinstanceup(v, -1, p->next);
	return 1;
}

static SQInteger sqf_getpath(HSQUIRRELVM v){
	CoordSys *p;
	if(!sqa_refobj(v, (SQUserPointer*)&p))
		return SQ_ERROR;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	sq_pushstring(v, p->getpath(), -1);
	return 1;
}

static SQInteger sqf_findcspath(HSQUIRRELVM v){
	CoordSys *p;
	if(!sqa_refobj(v, (SQUserPointer*)&p))
		return SQ_ERROR;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	const SQChar *s;
	sq_getstring(v, -1, &s);
	CoordSys *cs = p->findcspath(s);
	if(cs){
		sq_pushroottable(v);
		sq_pushstring(v, _SC("CoordSys"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		sqa_newobj(v, cs);
//		sq_setinstanceup(v, -1, cs);
		return 1;
	}
	sq_pushnull(v);
	return 1;
}

static SQInteger sqf_getcs(HSQUIRRELVM v){
	Player *p;
	SQRESULT sr;
	if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
		return sr;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(p->cs){
		sq_pushroottable(v);
		sq_pushstring(v, _SC("CoordSys"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		sqa_newobj(v, const_cast<CoordSys*>(p->cs));
//		sq_setinstanceup(v, -1, const_cast<CoordSys*>(p->cs));
		return 1;
	}
	sq_pushnull(v);
	return 1;
}

template<typename Class, typename MType, MType Class::*member>
inline MType membergetter(Class *p){
	return p->*member;
}

template<typename Class, typename MType, MType Class::*member>
inline void membersetter(Class p, MType Class::*member, MType &newvalue){
	p->*member = newvalue;
}

template<typename Class, typename MType, MType (Class::*member)()const>
inline MType accessorgetter(Class *p){
	return (p->*member)();
}

template<typename Class, typename MType, void (Class::*member)(MType)>
inline void accessorsetter(Class p, MType Class::*member, MType &newvalue){
	(p->*member)(newvalue);
}

// "Class" can be CoordSys, Player or Entity. "MType" can be Vec3d or Quatd. "member" can be pos or rot, respectively.
// I think I cannot write this logic shorter.
template<typename Class, typename MType, MType getter(Class *p)>
SQInteger sqf_getintrinsic(HSQUIRRELVM v){
	try{
		Class *p;
		SQRESULT sr;
		if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
			return sr;
		SQIntrinsic<MType> r;
		r.value = getter(p);
		r.push(v);
		return 1;
	}
	catch(SQFError){
		return SQ_ERROR;
	}
}

#if 1

// This template function is rather straightforward, but produces similar instances rapidly.
// Probably classes with the same member variables should share base class, but it's not always feasible.
template<typename Class, typename MType, MType Class::*member>
static SQInteger sqf_setintrinsic(HSQUIRRELVM v){
	try{
		Class *p;
		SQRESULT sr;
		if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
			return sr;
		SQIntrinsic<MType> r;
		r.getValue(v, 2);
		p->*member = r.value;
		return 0;
	}
	catch(SQFError){
		return SQ_ERROR;
	}
}

template<typename Class, typename MType, void (Class::*member)(const MType &)>
static SQInteger sqf_setintrinsica(HSQUIRRELVM v){
	try{
		Class *p;
		SQRESULT sr;
		if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
			return sr;
		SQIntrinsic<MType> r;
		r.getValue(v, 2);
		(p->*member)(r.value);
		return 0;
	}
	catch(SQFError){
		return SQ_ERROR;
	}
}

#else

// This implementation is a bit dirty and requires unsigned long to have equal or greater size compared to pointer's,
// but actually reduces code size and probably execution time, by CPU caching effect.
template<typename MType>
static SQInteger sqf_setintrinsic_in(HSQUIRRELVM v, unsigned long offset){
	try{
		void *p;
		SQRESULT sr;
		if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
			return sr;
		SQIntrinsic<MType> r;
		r.getValue(v, 2);
		*(MType*)&((unsigned char*)p)[offset] = r.value;
		return 0;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

template<typename Class, typename MType, MType Class::*member>
SQInteger sqf_setintrinsic(HSQUIRRELVM v){
	return sqf_setintrinsic_in<MType>(v, (unsigned long)&(((Class*)NULL)->*member));
}
#endif

template<>
static SQInteger sqf_setintrinsic<Entity, Quatd, &Entity::rot>(HSQUIRRELVM v){
	try{
		Entity *p;
		SQRESULT sr;
		if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
			return sr;
		SQIntrinsic<Quatd> r;
		r.getValue(v, 2);
		p->setPosition(NULL, &r.value);
/*		p->rot = r.value;
		if(p->bbody){
			btTransform tra = p->bbody->getWorldTransform();
			tra.setRotation(btqc(p->rot));
			p->bbody->setWorldTransform(tra);
		}*/
		return 0;
	}
	catch(SQFError){
		return SQ_ERROR;
	}
}

template<>
static SQInteger sqf_setintrinsic<Entity, Vec3d, &Entity::pos>(HSQUIRRELVM v){
	try{
		Entity *p;
		SQRESULT sr;
		if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
			return sr;
		SQIntrinsic<Vec3d> r;
		r.getValue(v, 2);
		p->setPosition(&r.value);
/*		p->pos = r.value;
		if(p->bbody){
			btTransform tra = p->bbody->getWorldTransform();
			tra.setOrigin(btvc(p->pos));
			p->bbody->setCenterOfMassTransform(tra);
		}*/
		return 0;
	}
	catch(SQFError){
		return SQ_ERROR;
	}
}

static SQInteger sqf_Vec3d_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	sq_pushstring(v, _SC("a"), -1);
	Vec3d &vec = *(Vec3d*)sq_newuserdata(v, sizeof(Vec3d));
	for(int i = 0; i < 3; i++){
		if(i + 2 <= argc){
			SQFloat f;
			const SQChar *str;
			if(!SQ_FAILED(sq_getfloat(v, i + 2, &f)))
				vec[i] = f;
			else if(!SQ_FAILED(sq_getstring(v, i + 2, &str)))
				vec[i] = atof(str);
		}
		else
			vec[i] = 0.;
	}
	sq_set(v, 1);
	return 0;
}

static SQInteger sqf_Quatd_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	sq_pushstring(v, _SC("a"), -1);
	Quatd &vec = *(Quatd*)sq_newuserdata(v, sizeof(Quatd));
	for(int i = 0; i < 4; i++){
		SQFloat f;
		if(i + 2 <= argc && !SQ_FAILED(sq_getfloat(v, i + 2, &f)))
			vec[i] = f;
		else
			vec[i] = 0.;
	}
	sq_set(v, 1);
	return 0;
}

static SQInteger sqf_Vec3d_tostring(HSQUIRRELVM v){
	try{
		SQVec3d q;
		q.getValue(v, 1);
		sq_pushstring(v, cpplib::dstring() << "[" << q.value[0] << "," << q.value[1] << "," << q.value[2] << "]", -1);
		return 1;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

static SQInteger sqf_Vec3d_add(HSQUIRRELVM v){
	try{
		SQVec3d q;
		q.getValue(v, 1);
		SQVec3d o;
		o.getValue(v, 2);
		SQVec3d r(q.value + o.value);
		r.push(v);
		return 1;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

// What a nonsense method...
static SQInteger sqf_Vec3d_get(HSQUIRRELVM v){
	try{
		SQVec3d q;
		q.getValue(v, 1);
		const SQChar *s;
		sq_getstring(v, 2, &s);
		if(!s[0] || s[1])
			return SQ_ERROR;
		if('x' <= s[0] && s[0] <= 'z'){
			sq_pushfloat(v, q.value[s[0] - 'x']);
			return 1;
		}
		return SQ_ERROR;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

// What a nonsense method...
static SQInteger sqf_Vec3d_set(HSQUIRRELVM v){
	try{
		SQVec3d q;
		q.getValue(v, 1);
		const SQChar *s;
		sq_getstring(v, 2, &s);
		if(!s[0] || s[1])
			return SQ_ERROR;
		if('x' <= s[0] && s[0] <= 'z'){
			SQFloat f;
			sq_getfloat(v, 3, &f);
			(*q.pointer)[s[0] - 'x'] = f;
			return 0;
		}
		return SQ_ERROR;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

static SQInteger sqf_Vec3d_sp(HSQUIRRELVM v){
	try{
		SQVec3d q;
		q.getValue(v, 1);
		SQVec3d o;
		o.getValue(v, 2);
		sq_pushfloat(v, q.value.sp(o.value));
		return 1;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

static SQInteger sqf_Vec3d_vp(HSQUIRRELVM v){
	try{
		SQVec3d q;
		q.getValue(v, 1);
		SQVec3d o;
		o.getValue(v, 2);
		SQVec3d r(q.value.vp(o.value));
		r.push(v);
		return 1;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

static SQInteger sqf_Quatd_tostring(HSQUIRRELVM v){
	try{
		SQQuatd q;
		q.getValue(v, 1);
		sq_pushstring(v, cpplib::dstring() << "[" << q.value[0] << "," << q.value[1] << "," << q.value[2] << "," << q.value[3] << "]", -1);
		return 1;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

// What a nonsense method...
static SQInteger sqf_Quatd_get(HSQUIRRELVM v){
	try{
		SQQuatd q;
		q.getValue(v, 1);
		const SQChar *s;
		sq_getstring(v, 2, &s);
		if(!s[0] || s[1])
			return SQ_ERROR;
		if('x' <= s[0] && s[0] <= 'z' || s[0] == 'w'){
			sq_pushfloat(v, q.value[s[0] == 'w' ? 3 : s[0] - 'x']);
			return 1;
		}
		return SQ_ERROR;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

// What a nonsense method...
static SQInteger sqf_Quatd_set(HSQUIRRELVM v){
	try{
		SQQuatd q;
		q.getValue(v, 1);
		const SQChar *s;
		sq_getstring(v, 2, &s);
		if(!s[0] || s[1])
			return SQ_ERROR;
		if('x' <= s[0] && s[0] <= 'z' || s[0] == 'w'){
			SQFloat f;
			sq_getfloat(v, 3, &f);
			(*q.pointer)[s[0] == 'w' ? 3 : s[0] - 'x'] = f;
			return 0;
		}
		return SQ_ERROR;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

static SQInteger sqf_Quatd_normin(HSQUIRRELVM v){
	try{
		SQQuatd q;
		q.getValue(v, 1);
		q.pointer->normin();
		return 1;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

template<typename Type>
static SQInteger sqf_Intri_mul(HSQUIRRELVM v){
	try{
		SQIntrinsic<Type> q;
		q.getValue(v, 1);
		SQIntrinsic<Type> o;
		o.getValue(v, 2);
		SQIntrinsic<Type> r;
		r.value = q.value * o.value;
		r.push(v);
		return 1;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

static SQInteger sqf_Quatd_trans(HSQUIRRELVM v){
	try{
		SQQuatd q;
		q.getValue(v, 1);
		SQVec3d o;
		o.getValue(v, 2);
		SQVec3d r;
		r.value = q.value.trans(o.value);
		r.push(v);
		return 1;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

bool sqa_newobj(HSQUIRRELVM v, Serializable *o){
	sq_pushstring(v, _SC("ref"), -1);
	sq_pushregistrytable(v); // reg
	sq_pushstring(v, _SC("objects"), -1); // reg "objects"
	sq_get(v, -2); // reg objects
	sq_pushuserpointer(v, o);
	if(SQ_FAILED(sq_get(v, -2))){ // reg objects (o)
		sq_pushuserpointer(v, o); // reg objects (o)
		sq_newuserdata(v, sizeof o); // reg objects (o) {ud}
		Serializable **p;
		if(SQ_FAILED(sq_getuserdata(v, -1, (SQUserPointer*)&p, NULL))){
			printf("error\n");
			return false;
		}
		*p = o;
		sq_newslot(v, -3, SQFalse); // reg objects
		sq_pushuserpointer(v, o);
		verify(SQ_SUCCEEDED(sq_get(v, -2)));
	}
	sq_weakref(v, -1); // reg objects (o) &(o)
	sq_remove(v, -2); // reg objects &(o)
	sq_remove(v, -2); // reg &(o)
	sq_remove(v, -2); // &(o)
	sq_set(v, -3);
	return true;
}

bool sqa_refobj(HSQUIRRELVM v, SQUserPointer *o, SQRESULT *sr, int idx){
	sq_pushstring(v, _SC("ref"), -1);
	sq_get(v, idx);
	SQUserPointer *p;
	if(SQ_FAILED(sq_getuserdata(v, -1, (SQUserPointer*)&p, NULL))){
		if(sr)
			*sr = sq_throwerror(v, _SC("The object being accessed is destructed in the game engine"));
		return false;
	}
	*o = *p;
	sq_poptop(v);
	return true;
}

void sqa_deleteobj(HSQUIRRELVM v, Serializable *o){
	sq_pushregistrytable(v); // reg
	sq_pushstring(v, _SC("objects"), -1); // reg "objects"
	sq_get(v, -2); // reg objects
	sq_pushuserpointer(v, o); // reg objects (o)
	sq_deleteslot(v, -2, SQFalse); // reg objects
	sq_pop(v, 2);
}

static SQInteger sqf_reg(HSQUIRRELVM v){
	sq_pushregistrytable(v);
	return 1;
}


void sqa_init(){
//    SquirrelVM::Init();
//	v = SquirrelVM::GetVMPtr();
	v = sq_open(1024);

	sqstd_seterrorhandlers(v);

	sq_setprintfunc(v, sqf_print); //sets the print function

	// Set object table for weak referencing.
	sq_pushregistrytable(v);
	sq_pushstring(v, _SC("objects"), -1);
	sq_newtable(v);
	sq_newslot(v, -3, SQFalse);
	sq_poptop(v);

	register_global_func(v, sqf_set_cvar, _SC("set_cvar"));
	register_global_func(v, sqf_get_cvar, _SC("get_cvar"));
	register_global_func(v, sqf_cmd, _SC("cmd"));
	register_global_func(v, sqf_reg, _SC("reg"));

    sq_pushroottable(v); //push the root table(were the globals of the script will be stored)

	sqstd_register_iolib(v);
	sqstd_register_mathlib(v);

	// Define class Vec3d, native vector representation
	sq_pushstring(v, _SC("Vec3d"), -1);
	sq_newclass(v, SQFalse);
	sq_settypetag(v, -1, tt_Vec3d);
	sq_pushstring(v, _SC("constructor"), -1);
	sq_newclosure(v, sqf_Vec3d_constructor, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("a"), -1);
	*(Vec3d*)sq_newuserdata(v, sizeof(Vec3d)) = vec3_000;
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_tostring"), -1);
	sq_newclosure(v, sqf_Vec3d_tostring, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_add"), -1);
	sq_newclosure(v, sqf_Vec3d_add, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("sp"), -1);
	sq_newclosure(v, sqf_Vec3d_sp, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("vp"), -1);
	sq_newclosure(v, sqf_Vec3d_vp, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_get"), -1);
	sq_newclosure(v, sqf_Vec3d_get, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_set"), -1);
	sq_newclosure(v, sqf_Vec3d_set, 0);
	sq_createslot(v, -3);
	sq_createslot(v, -3);

	// Define class Quatd
	sq_pushstring(v, _SC("Quatd"), -1);
	sq_newclass(v, SQFalse);
	sq_settypetag(v, -1, tt_Quatd);
	sq_pushstring(v, _SC("constructor"), -1);
	sq_newclosure(v, sqf_Quatd_constructor, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("a"), -1);
	*(Quatd*)sq_newuserdata(v, sizeof(Quatd)) = quat_0;
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_tostring"), -1);
	sq_newclosure(v, sqf_Quatd_tostring, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_get"), -1);
	sq_newclosure(v, sqf_Quatd_get, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_set"), -1);
	sq_newclosure(v, sqf_Quatd_set, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("normin"), -1);
	sq_newclosure(v, sqf_Quatd_normin, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_mul"), -1);
	sq_newclosure(v, sqf_Intri_mul<Quatd>, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("trans"), -1);
	sq_newclosure(v, sqf_Quatd_trans, 0);
	sq_createslot(v, -3);
	sq_createslot(v, -3);

	// Define class CoordSys
	sq_pushstring(v, _SC("CoordSys"), -1);
	sq_newclass(v, SQFalse);
	sq_pushstring(v, _SC("ref"), -1);
	sq_pushnull(v);
	sq_newslot(v, -3, SQFalse);
	sq_pushstring(v, _SC("name"), -1);
	sq_newclosure(v, sqf_name, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("getpos"), -1);
	sq_newclosure(v, sqf_getintrinsic<CoordSys, Vec3d, membergetter<CoordSys, Vec3d, &CoordSys::pos> >, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("setpos"), -1);
	sq_newclosure(v, sqf_setintrinsic<CoordSys, Vec3d, &CoordSys::pos>, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("getrot"), -1);
	sq_newclosure(v, sqf_getintrinsic<CoordSys, Quatd, membergetter<CoordSys, Quatd, &CoordSys::rot> >, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("setrot"), -1);
	sq_newclosure(v, sqf_setintrinsic<CoordSys, Quatd, &CoordSys::rot>, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("child"), -1);
	sq_newclosure(v, sqf_child, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("next"), -1);
	sq_newclosure(v, sqf_next, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("getpath"), -1);
	sq_newclosure(v, sqf_getpath, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("findcspath"), -1);
	sq_newclosure(v, sqf_findcspath, 0);
	sq_setparamscheck(v, 2, _SC("xs"));
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("addent"), -1);
	sq_newclosure(v, sqf_addent, 0);
	sq_setparamscheck(v, 3, "xsx");
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_get"), -1);
	sq_newclosure(v, sqf_CoordSys_get, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_set"), -1);
	sq_newclosure(v, sqf_set<CoordSys>, 0);
	sq_createslot(v, -3);
	sq_createslot(v, -3);

	// Define class Universe
	sq_pushstring(v, _SC("Universe"), -1);
	sq_pushstring(v, _SC("CoordSys"), -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_pushstring(v, _SC("_get"), -1);
	sq_newclosure(v, sqf_Universe_get, 0);
	sq_createslot(v, -3);
	sq_createslot(v, -3);

	// Define class Entity
	sq_pushstring(v, _SC("Entity"), -1);
	sq_newclass(v, SQFalse);
	sq_settypetag(v, -1, tt_Entity);
	sq_pushstring(v, _SC("ref"), -1);
	sq_pushnull(v);
	sq_newslot(v, -3, SQFalse);
/*	sq_pushstring(v, _SC("classname"), -1);
	sq_newclosure(v, sqf_func<Entity, const SQChar *(Entity::*)() const, &Entity::classname>, 0);
	sq_createslot(v, -3);*/
	sq_pushstring(v, _SC("getpos"), -1);
	sq_newclosure(v, sqf_getintrinsic<Entity, Vec3d, membergetter<Entity, Vec3d, &Entity::pos> >, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("setpos"), -1);
	sq_newclosure(v, sqf_setintrinsic<Entity, Vec3d, &Entity::pos>, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("getrot"), -1);
	sq_newclosure(v, sqf_getintrinsic<Entity, Quatd, membergetter<Entity, Quatd, &Entity::rot> >, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("setrot"), -1);
	sq_newclosure(v, sqf_setintrinsic<Entity, Quatd, &Entity::rot>, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_get"), -1);
	sq_newclosure(v, sqf_Entity_get, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_set"), -1);
	sq_newclosure(v, sqf_Entity_set, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("command"), -1);
	sq_newclosure(v, sqf_Entity_command, 0);
	sq_createslot(v, -3);
	sq_createslot(v, -3);

	// Define instance of class Universe
	sq_pushstring(v, _SC("universe"), -1); // this "universe"
	sq_pushstring(v, _SC("Universe"), -1); // this "universe" "Universe"
	sq_get(v, -3); // this "universe" Universe
	sq_createinstance(v, -1); // this "universe" Universe Universe-instance
	sqa_newobj(v, &universe);
//	sq_setinstanceup(v, -1, &universe); // this "universe" Universe Universe-instance
	sq_remove(v, -2); // this "universe" Universe-instance
	sq_createslot(v, -3); // this

	// Define class Player
/*	SqPlus::SQClassDef<Player>(_SC("Player")).
		func(&Player::classname, _SC("classname")).
		func(&Player::getcs, _SC("cs"));*/
	sq_pushstring(v, _SC("Player"), -1);
	sq_newclass(v, SQFalse);
	sq_pushstring(v, _SC("ref"), -1);
	sq_pushnull(v);
	sq_newslot(v, -3, SQFalse);
/*	sq_pushstring(v, _SC("classname"), -1);
	sq_newclosure(v, sqf_func<Player, const SQChar *(Player::*)() const, &Player::classname>, 0);
	sq_createslot(v, -3);*/
/*	sq_pushstring(v, _SC("getcs"), -1);
	sq_newclosure(v, sqf_getcs, 0);
	sq_createslot(v, -3);*/
	sq_pushstring(v, _SC("getpos"), -1);
	sq_newclosure(v, sqf_getintrinsic<Player, Vec3d, accessorgetter<Player, Vec3d, &Player::getpos> >, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("setpos"), -1);
	sq_newclosure(v, sqf_setintrinsica<Player, Vec3d, &Player::setpos>, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("getvelo"), -1);
	sq_newclosure(v, sqf_getintrinsic<Player, Vec3d, accessorgetter<Player, Vec3d, &Player::getvelo> >, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("setvelo"), -1);
	sq_newclosure(v, sqf_setintrinsica<Player, Vec3d, &Player::setvelo>, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("getrot"), -1);
	sq_newclosure(v, sqf_getintrinsic<Player, Quatd, accessorgetter<Player, Quatd, &Player::getrot> >, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("setrot"), -1);
	sq_newclosure(v, sqf_setintrinsica<Player, Quatd, &Player::setrot>, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_get"), -1);
	sq_newclosure(v, &Player::sqf_get, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_set"), -1);
	sq_newclosure(v, &Player::sqf_set, 0);
	sq_createslot(v, -3);
	sq_createslot(v, -3);

	sq_pushstring(v, _SC("player"), -1); // this "player"
	sq_pushstring(v, _SC("Player"), -1); // this "player" "Player"
	sq_get(v, 1); // this "player" Player
	sq_createinstance(v, -1); // this "player" Player Player-instance
	sqa_newobj(v, &pl); // this "player" Player Player-instance
//	sq_setinstanceup(v, -1, &pl); // this "player" Player Player-instance
	sq_remove(v, -2); // this "player" Player-instance
	sq_createslot(v, 1); // this

	sq_pushstring(v, _SC("register_console_command"), -1);
	sq_newclosure(v, sqf_register_console_command, 0);
	sq_createslot(v, 1);

	if(SQ_SUCCEEDED(sqstd_dofile(v, _SC("scripts/init.nut"), 0, 1))) // also prints syntax errors if any 
	{
//		call_foo(v,1,2.5,_SC("teststring"));
	}
	else
		CmdPrintf("scripts/init.nut failed.");

	sq_poptop(v); // Pop the root table.
}

void sqa_anim0(){
	sq_pushroottable(v); // root
	sq_pushstring(v, _SC("init_Universe"), -1); // root "init_Universe"
	if(SQ_SUCCEEDED(sq_get(v, -2))){ // root closure
		sq_pushroottable(v);
		sq_call(v, 1, SQFalse, SQTrue);
		sq_poptop(v);
	}
//	sq_poptop(v);
}

void sqa_anim(double dt){
	sq_pushroottable(v);
	sq_pushstring(v, _SC("frameproc"), -1);
	if(SQ_SUCCEEDED(sq_get(v, -2))){
		sq_pushroottable(v);
		sq_pushfloat(v, dt); 
		sq_call(v, 2, SQFalse, SQTrue);
		sq_poptop(v); // pop the closure
	}
	sq_poptop(v);
}

void sqa_delete_Entity(Entity *e){
	sq_pushroottable(v);
	sq_pushstring(v, _SC("hook_delete_Entity"), -1);
	if(SQ_SUCCEEDED(sq_get(v, -2))){
		sq_pushroottable(v); // root func root
		sq_pushstring(v, _SC("Entity"), -1); // root func root "Entity"
		sq_get(v, -2); // root func root Entity
		sq_createinstance(v, -1); // root func root Entity Entity-instance
		sqa_newobj(v, e);
//		sq_setinstanceup(v, -1, e); // root func root Entity Entity-instance
		sq_remove(v, -2); // root func root Entity-instance
		sq_call(v, 2, SQFalse, SQTrue); // root func
		sq_poptop(v); // root
	}
	sq_poptop(v);
	sqa_deleteobj(v, e);
}

void sqa_exit(){
	sq_pop(v,1); //pops the root table

//	SquirrelVM::Shutdown();
	sq_close(v);
}


SQInteger register_global_func(HSQUIRRELVM v,SQFUNCTION f,const SQChar *fname)
{
    sq_pushroottable(v);
    sq_pushstring(v,fname,-1);
    sq_newclosure(v,f,0); //create a new function
    sq_createslot(v,-3); 
    sq_pop(v,1); //pops the root table    
	return 0;
}

}
