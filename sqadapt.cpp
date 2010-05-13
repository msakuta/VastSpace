#include "sqadapt.h"
#include "cmd.h"
#include "Universe.h"
#include <squirrel.h>
#include <sqstdio.h>
#include <sqstdaux.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern "C"{
#include <clib/c.h>
}

extern Universe universe;


namespace sqa{

HSQUIRRELVM g_sqvm;
static HSQUIRRELVM &v = g_sqvm;

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

#if 0
static SQInteger sqf_addent(HSQUIRRELVM v){
	extern Player *ppl;
	WarField *w;
	Entity *pt;
	if(this->w)
		w = this->w;
	else
		w = this->w = new WarSpace(this)/*spacewar_create(cs, ppl)*/;
	pt = Entity::create(argv[1], w);
	if(pt){
		pt->pos[0] = 2 < argc ? calc3(&argv[2], sc.vl, NULL) : 0.;
		pt->pos[1] = 3 < argc ? calc3(&argv[3], sc.vl, NULL) : 0.;
		pt->pos[2] = 4 < argc ? calc3(&argv[4], sc.vl, NULL) : 0.;
		pt->race = 5 < argc ? atoi(argv[5]) : 0;
	}
	else
		printf("%s(%ld): addent: Unknown entity class name: %s\n", sc.fname, sc.line, argv[1]);
	return true;
}
#endif

static SQInteger sqf_name(HSQUIRRELVM v){
	sq_pushstring(v, _SC("Universe"), -1);
	return 1;
}

static SQInteger sqf_Universe_get(HSQUIRRELVM v){
	Universe *p;
	const SQChar *wcs;
	sq_getstring(v, -1, &wcs);
	sq_pushstring(v, _SC("ptr"), -1); // this "ptr"
	sq_get(v, 1);
	if(SQ_FAILED(sq_getuserpointer(v, -1, (SQUserPointer*)&p))) // this (ptr)
		return SQ_ERROR;
	if(!strcmp(wcs, _SC("timescale"))){
		sq_pushfloat(v, p->timescale);
		return 1;
	}
	else if(!strcmp(wcs, _SC("global_time"))){
		sq_pushfloat(v, p->global_time);
		return 1;
	}
	sq_pushnull(v);
	return 1;
}


void sqa_init(){

	sqstd_seterrorhandlers(v);

	sq_setprintfunc(v, sqf_print); //sets the print function

	register_global_func(v, sqf_set_cvar, _SC("set_cvar"));
	register_global_func(v, sqf_get_cvar, _SC("get_cvar"));
	register_global_func(v, sqf_cmd, _SC("cmd"));

    sq_pushroottable(v); //push the root table(were the globals of the script will be stored)

	// Define class Universe
	sq_pushstring(v, _SC("Universe"), -1);
	sq_newclass(v, SQFalse);
	sq_pushstring(v, _SC("name"), -1);
	sq_newclosure(v, sqf_name, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("_get"), -1);
	sq_newclosure(v, sqf_Universe_get, 0);
	sq_createslot(v, -3);
	sq_pushstring(v, _SC("ptr"), -1);
	sq_pushuserpointer(v, &universe);
	sq_createslot(v, -3);
	sq_createslot(v, -3);

	// Define instance of class Universe
	sq_pushstring(v, _SC("universe"), -1); // this "universe"
	sq_pushstring(v, _SC("Universe"), -1); // this "universe" "Universe"
	sq_get(v, -3); // this "universe" Universe
	sq_createinstance(v, -1); // this "universe" Universe Universe-instance
	sq_remove(v, -2); // this "universe" Universe-instance
	sq_createslot(v, -3); // this

//	SqPlus::SQClassDef<Universe>(_SC("Universe")).
//		func(&Universe::classname, _SC("classname"));

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
