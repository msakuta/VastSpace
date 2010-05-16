#include "sqadapt.h"
#include "cmd.h"
#include "Universe.h"
#include "Entity.h"
#include "Player.h"
#include <squirrel.h>
#include <sqstdio.h>
#include <sqstdaux.h>
//#include <../sqplus/sqplus.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern "C"{
#include <clib/c.h>
}

extern Universe universe;
extern Player pl;

/*DECLARE_INSTANCE_TYPE(Player)

namespace SqPlus{
template<>
void Push<const CoordSys>(SQVM *v, const CoordSys *cs){
	Push(v, (SQUserPointer)cs);
}
}*/

namespace sqa{

HSQUIRRELVM g_sqvm;
static HSQUIRRELVM &v = g_sqvm;

static SQUserPointer registerTypeTag(){
	static SQUserPointer counter = (SQUserPointer)1;
	return (SQUserPointer)(*(unsigned long*)&counter)++;
}

static const SQUserPointer tt_Vec3 = registerTypeTag();
static const SQUserPointer tt_Vec3d = registerTypeTag();
static const SQUserPointer tt_Quatd = registerTypeTag();


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


void call_foo(HSQUIRRELVM v, int n,float f,const SQChar *s)
{
	int top = sq_gettop(v); //saves the stack size before the call
	sq_pushroottable(v); //pushes the global table
	sq_pushstring(v,_SC("foo"),-1);
	if(SQ_SUCCEEDED(sq_get(v,-2))) { //gets the field 'foo' from the global table
		sq_pushroottable(v); //push the 'this' (in this case is the global table)
		sq_pushinteger(v,n); 
		sq_pushfloat(v,f);
		sq_pushstring(v,s,-1);
		sq_call(v,4,0,0); //calls the function 
	}
	sq_settop(v,top); //restores the original stack size
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

static SQInteger sqf_addent(HSQUIRRELVM v){
	if(sq_gettop(v) < 3)
		return SQ_ERROR;
	Universe *p;
	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	
	Vec3d pos = vec3_000;
	if(OT_INSTANCE == sq_gettype(v, -1)){
		SQUserPointer typetag;
		sq_gettypetag(v, -1, &typetag);
		if(typetag == tt_Vec3){
			sq_pushstring(v, _SC("a"), -1); // this classname vec3 "a"
			sq_get(v, -2); // this classname vec3 vec3.a

			for(int i = 0; i < 3; i++){
				SQFloat f;
				sq_pushinteger(v, i); // this classname vec3 vec3.a i
				sq_get(v, -2); // this classname vec3 vec3.a vec3.a[i]
				sq_getfloat(v, -1, &f);
				sq_poptop(v); // this classname vec3 vec3.a
				pos[i] = f;
			}
		}
		else if(typetag == tt_Vec3d){
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
		pt->pos = pos;
/*		pt->race = 5 < argc ? atoi(argv[5]) : 0;*/
	}
	else
		sq_throwerror(v, cpplib::dstring("addent: Unknown entity class name: %s") << arg);

	sq_pushroottable(v);
	sq_pushstring(v, _SC("Entity"), -1);
	sq_get(v, -2);
	sq_createinstance(v, -1);
	sq_setinstanceup(v, -1, pt);
	return 1;
}

static SQInteger sqf_name(HSQUIRRELVM v){
	CoordSys *p;
	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	sq_pushstring(v, p->name, -1);
	return 1;
}

template<typename Class>
static SQInteger sqf_get(HSQUIRRELVM v){
	Class *p;
	const SQChar *wcs;
	sq_getstring(v, -1, &wcs);
	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!strcmp(wcs, _SC("pos"))){
		sq_pushroottable(v);
		sq_pushstring(v, _SC("Vec3d"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		sq_pushstring(v, _SC("a"), -1);
		sq_get(v, -2);
		Vec3d *vec;
		sq_getuserdata(v, -1, (SQUserPointer*)&vec, NULL);
		*vec = p->pos;
		sq_pop(v, 1);
		return 1;
	}
	else if(!strcmp(wcs, _SC("rot"))){
		sq_pushroottable(v);
		sq_pushstring(v, _SC("Quatd"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		sq_pushstring(v, _SC("a"), -1);
		sq_get(v, -2);
		Quatd *q;
		sq_getuserdata(v, -1, (SQUserPointer*)&q, NULL);
		*q = p->rot;
		sq_pop(v, 1);
		return 1;
	}
	else if(!strcmp(wcs, _SC("classname"))){
		sq_pushstring(v, p->classname(), -1);
		return 1;
	}
	sq_pushnull(v);
	return 1;
}

template<typename Class>
static SQInteger sqf_set(HSQUIRRELVM v){
	if(sq_gettop(v) < 3)
		return SQ_ERROR;
	Class *p;
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!strcmp(wcs, _SC("pos"))){
		SQUserPointer tt;
		if(OT_INSTANCE == sq_gettype(v, 3) && !SQ_FAILED(sq_gettypetag(v, 3, &tt)) && tt == tt_Vec3d){
			sq_pushstring(v, _SC("a"), -1);
			sq_get(v, 3);
			Vec3d *vec;
			sq_getuserdata(v, -1, (SQUserPointer*)&vec, NULL);
			p->pos = *vec;
			return 1;
		}
		else
			return SQ_ERROR;
	}
	else if(!strcmp(wcs, _SC("rot"))){
		SQUserPointer tt;
		if(OT_INSTANCE == sq_gettype(v, 3) && !SQ_FAILED(sq_gettypetag(v, 3, &tt)) && tt == tt_Quatd){
			sq_pushstring(v, _SC("a"), -1);
			sq_get(v, 3);
			Quatd *q;
			sq_getuserdata(v, -1, (SQUserPointer*)&q, NULL);
			p->rot = *q;
			return 1;
		}
		else
			return SQ_ERROR;
	}
	sq_pushnull(v);
	return 1;
}

static SQInteger sqf_Entity_get(HSQUIRRELVM v){
	Entity *p;
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!strcmp(wcs, _SC("race"))){
		sq_pushinteger(v, p->race);
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
	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!strcmp(wcs, _SC("race"))){
		if(SQ_FAILED(sq_getinteger(v, 3, &p->race)))
			return SQ_ERROR;
		return 0;
	}
	else
		return sqf_set<Entity>(v);
}

static SQInteger sqf_Universe_get(HSQUIRRELVM v){
	Universe *p;
	const SQChar *wcs;
	sq_getstring(v, -1, &wcs);
	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!strcmp(wcs, _SC("timescale"))){
		sq_pushfloat(v, p->timescale);
		return 1;
	}
	else if(!strcmp(wcs, _SC("global_time"))){
		sq_pushfloat(v, p->global_time);
		return 1;
	}
	else
		return sqf_get<Universe::st>(v);
}


static SQInteger sqf_child(HSQUIRRELVM v){
	CoordSys *p;
	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
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
		sq_setinstanceup(v, -1, p->children);
//	}
	return 1;
}

static SQInteger sqf_next(HSQUIRRELVM v){
	CoordSys *p;
	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!p->next){
		sq_pushnull(v);
		return 1;
	}
	sq_pushroottable(v);
	sq_pushstring(v, _SC("CoordSys"), -1);
	sq_get(v, -2);
	sq_createinstance(v, -1);
	sq_setinstanceup(v, -1, p->next);
	return 1;
}

static SQInteger sqf_getpath(HSQUIRRELVM v){
	CoordSys *p;
	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	sq_pushstring(v, p->getpath(), -1);
	return 1;
}

static SQInteger sqf_findcspath(HSQUIRRELVM v){
	CoordSys *p;
	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	const SQChar *s;
	sq_getstring(v, -1, &s);
	CoordSys *cs = p->findcspath(s);
	if(cs){
		sq_pushroottable(v);
		sq_pushstring(v, _SC("CoordSys"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		sq_setinstanceup(v, -1, cs);
		return 1;
	}
	sq_pushnull(v);
	return 1;
}

template<typename Class, typename MemberType, MemberType member>
SQInteger sqf_get_member(HSQUIRRELVM v){
	Class *p;
	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	sq_pushstring(v, p->*member, -1);
	return 1;
}

template<typename Class, typename MemberType, MemberType member>
SQInteger sqf_func(HSQUIRRELVM v){
	Class *p;
	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	sq_pushstring(v, (p->*member)(), -1);
	return 1;
}

SQInteger sqf_getcs(HSQUIRRELVM v){
	Player *p;
	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(p->cs){
		sq_pushroottable(v);
		sq_pushstring(v, _SC("CoordSys"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		sq_setinstanceup(v, -1, const_cast<CoordSys*>(p->cs));
		return 1;
	}
	sq_pushnull(v);
	return 1;
}

template<typename Class>
SQInteger sqf_getpos(HSQUIRRELVM v){
	Class *p;
	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	sq_pushroottable(v);
	sq_pushstring(v, _SC("Vec3d"), -1);
	sq_get(v, -2);
	sq_createinstance(v, -1);
	sq_pushstring(v, _SC("a"), -1);
	sq_get(v, -2);
	Vec3d *vec;
	if(SQ_FAILED(sq_getuserdata(v, -1, (SQUserPointer*)&vec, NULL)))
		return SQ_ERROR;
	*vec = p->pos;
	sq_poptop(v);
	return 1;
}

static SQInteger sqf_Vec3_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	sq_pushstring(v, _SC("a"), -1);
	sq_newarray(v, 0);
	for(int i = 0; i < 3; i++){
		SQFloat f;
		if(i + 2 <= argc && !SQ_FAILED(sq_getfloat(v, i + 2, &f)))
			sq_pushfloat(v, f);
		else
			sq_pushfloat(v, 0.f);
		sq_arrayappend(v, -2);
	}
	sq_set(v, 1);
	return 0;
}

static SQInteger sqf_Vec3d_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	sq_pushstring(v, _SC("a"), -1);
	Vec3d &vec = *(Vec3d*)sq_newuserdata(v, sizeof(Vec3d));
	for(int i = 0; i < 3; i++){
		SQFloat f;
		if(i + 2 <= argc && !SQ_FAILED(sq_getfloat(v, i + 2, &f)))
			vec[i] = f;
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
	sq_pushstring(v, _SC("a"), -1);
	sq_get(v, 1);
	Vec3d *vec;
	sq_getuserdata(v, -1, (SQUserPointer*)&vec, NULL);
	sq_pushstring(v, cpplib::dstring() << "[" << (*vec)[0] << "," << (*vec)[1] << "," << (*vec)[2] << "]", -1);
	return 1;
}

static SQInteger sqf_Quatd_tostring(HSQUIRRELVM v){
	sq_pushstring(v, _SC("a"), -1);
	sq_get(v, 1);
	Quatd *q;
	sq_getuserdata(v, -1, (SQUserPointer*)&q, NULL);
	sq_pushstring(v, cpplib::dstring() << "[" << (*q)[0] << "," << (*q)[1] << "," << (*q)[2] << "," << (*q)[3] << "]", -1);
	return 1;
}

static SQInteger sqf_Quatd_normin(HSQUIRRELVM v){
	sq_pushstring(v, _SC("a"), -1);
	sq_get(v, 1);
	Quatd *q;
	sq_getuserdata(v, -1, (SQUserPointer*)&q, NULL);
	q->normin();
	sq_pop(v, sq_gettop(v) - 1);
	return 1;
}


void sqa_init(){
//    SquirrelVM::Init();
//	v = SquirrelVM::GetVMPtr();
	v = sq_open(1024);

	sqstd_seterrorhandlers(v);

	sq_setprintfunc(v, sqf_print); //sets the print function

	register_global_func(v, sqf_set_cvar, _SC("set_cvar"));
	register_global_func(v, sqf_get_cvar, _SC("get_cvar"));
	register_global_func(v, sqf_cmd, _SC("cmd"));

    sq_pushroottable(v); //push the root table(were the globals of the script will be stored)

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
	sq_createslot(v, -3);

	// Define class Vec3
	sq_pushstring(v, _SC("Vec3"), -1);
	sq_newclass(v, SQFalse);
	sq_settypetag(v, -1, tt_Vec3);
	sq_pushstring(v, _SC("constructor"), -1);
	sq_newclosure(v, sqf_Vec3_constructor, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("a"), -1);
	sq_newarray(v, 0);
	for(int i = 0; i < 3; i++){
		sq_pushfloat(v, 0.f);
		sq_arrayappend(v, -2);
	}
	sq_newslot(v, -3, SQFalse);
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
	sq_pushstring(v, _SC("normin"), -1);
	sq_newclosure(v, sqf_Quatd_normin, 0);
	sq_createslot(v, -3);
	sq_createslot(v, -3);

	// Define class CoordSys
	sq_pushstring(v, _SC("CoordSys"), -1);
	sq_newclass(v, SQFalse);
	sq_pushstring(v, _SC("name"), -1);
	sq_newclosure(v, sqf_name, 0);
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
	sq_newclosure(v, sqf_get<CoordSys>, 0);
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
	sq_pushstring(v, _SC("classname"), -1);
	sq_newclosure(v, sqf_func<Entity, const SQChar *(Entity::*)() const, &Entity::classname>, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_get"), -1);
	sq_newclosure(v, sqf_Entity_get, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_set"), -1);
	sq_newclosure(v, sqf_Entity_set, 0);
	sq_createslot(v, -3);
	sq_createslot(v, -3);

	// Define instance of class Universe
	sq_pushstring(v, _SC("universe"), -1); // this "universe"
	sq_pushstring(v, _SC("Universe"), -1); // this "universe" "Universe"
	sq_get(v, -3); // this "universe" Universe
	sq_createinstance(v, -1); // this "universe" Universe Universe-instance
	sq_setinstanceup(v, -1, &universe); // this "universe" Universe Universe-instance
	sq_remove(v, -2); // this "universe" Universe-instance
	sq_createslot(v, -3); // this

	// Define class Player
/*	SqPlus::SQClassDef<Player>(_SC("Player")).
		func(&Player::classname, _SC("classname")).
		func(&Player::getcs, _SC("cs"));*/
	sq_pushstring(v, _SC("Player"), -1);
	sq_newclass(v, SQFalse);
	sq_pushstring(v, _SC("classname"), -1);
	sq_newclosure(v, sqf_func<Player, const SQChar *(Player::*)() const, &Player::classname>, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("getcs"), -1);
	sq_newclosure(v, sqf_getcs, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("getpos"), -1);
	sq_newclosure(v, sqf_getpos<Player>, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_get"), -1);
	sq_newclosure(v, sqf_get<Player>, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_set"), -1);
	sq_newclosure(v, sqf_set<Player>, 0);
	sq_createslot(v, -3);
	sq_createslot(v, -3);

	sq_pushstring(v, _SC("player"), -1); // this "player"
	sq_pushstring(v, _SC("Player"), -1); // this "player" "Player"
	sq_get(v, 1); // this "player" Player
	sq_createinstance(v, -1); // this "player" Player Player-instance
	sq_setinstanceup(v, -1, &pl); // this "player" Player Player-instance
	sq_remove(v, -2); // this "player" Player-instance
	sq_createslot(v, 1); // this

	sq_poptop(v); // Pop the root table.
}

void sqa_anim0(){
	sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
	if(SQ_SUCCEEDED(sqstd_dofile(v, _SC("scripts/init.nut"), 0, 1))) // also prints syntax errors if any 
	{
//		call_foo(v,1,2.5,_SC("teststring"));
	}
	else
		CmdPrintf("scripts/init.nut failed.");
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
