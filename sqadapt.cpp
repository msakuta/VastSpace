#include "sqadapt.h"
#include "cmd.h"
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

HSQUIRRELVM g_sqvm;
static HSQUIRRELVM &v = g_sqvm;

static void sqf_print(HSQUIRRELVM v, const SQChar *s, ...) 
{ 
	va_list arglist; 
	va_start(arglist, s);
//	vwprintf(s, arglist);
	wchar_t wcstr[1024]; // The buffer size does not make sense
	vswprintf(wcstr, numof(wcstr), s, arglist);
	char cstr[1024];
	wcstombs(cstr, wcstr, sizeof cstr);
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
		const SQChar *wargs[2];
		int i;
		sq_getstring(v, 2, &wargs[0]);
		sq_getstring(v, 3, &wargs[1]);
		size_t arglen[2];
		char *args[3];
		args[0] = "set";
		for(i = 0; i < 2; i++){
			args[i+1] = new char[arglen[i] = wcslen(wargs[i]) * 2 + 1];
			wcstombs(args[i+1], wargs[i], arglen[i]);
		}
		cmd_set(3, const_cast<const char **>(args));
		delete[] args[1];
		delete[] args[2];
	}
	return 0;
}

static SQInteger sqf_get_cvar(HSQUIRRELVM v){
	SQInteger nargs = sq_gettop(v); //number of arguments
	if(2 == nargs && sq_gettype(v, 2) == OT_STRING){
		const SQChar *warg;
		sq_getstring(v, 2, &warg);
		size_t arglen;
		char *arg;
		arg = new char[arglen = wcslen(warg) * 2];
		wcstombs(arg, warg, arglen);
		const char *ret = CvarGetString(arg);
		delete[] arg;
		if(!ret){
			sq_pushnull(v);
			return 1;
		}
		size_t retlen = strlen(ret) + 1;
		SQChar *wret = new SQChar[retlen];
		mbstowcs(wret, ret, retlen);
		delete[] ret;
		sq_pushstring(v, wret, -1);
		delete[] wret;
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
		const SQChar *warg;
		sq_getstring(v, 2, &warg);
		size_t arglen;
		char *arg;
		arg = new char[arglen = wcslen(warg) * 2];
		wcstombs(arg, warg, arglen);
		CmdExec(arg);
		delete[] arg;
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

void sqa_init(){

	sqstd_seterrorhandlers(v);

	sq_setprintfunc(v, sqf_print); //sets the print function

	register_global_func(v, sqf_set_cvar, _SC("set_cvar"));
	register_global_func(v, sqf_get_cvar, _SC("get_cvar"));
	register_global_func(v, sqf_cmd, _SC("cmd"));

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

