#include "sqadapt.h"
#include "Application.h"
#include "cmd.h"
#include "cmd_int.h"
#include "Universe.h"
#include "Entity.h"
#include "Player.h"
#include "btadapt.h"
#include "EntityCommand.h"
#include "Docker.h"
#include "astro.h"
#include "TexSphere.h"
#include "Game.h"
#include "serial_util.h"
#include "glw/glwindow.h"
#include "glw/GLWmenu.h"
#include "glw/message.h"
#include "glw/GLWentlist.h"
#include "ClientMessage.h"
extern "C"{
#include <clib/timemeas.h>
#include <clib/gl/gldraw.h>
}
#include <cpplib/vec3.h>
#include <squirrel.h>
#include <sqstdio.h>
#include <sqstdaux.h>
#include <sqstdmath.h>
//#include <../sqplus/sqplus.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef _WIN32
#include <dlfcn.h>
#endif

#ifndef NDEBUG
#define verify(a) assert(a)
#else
#define verify(a) (a)
#endif

extern "C"{
#include <clib/c.h>
}

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.h>
#include <BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.h>

static Player *player;
#define pl (*player)
extern GLwindow *glwlist;
extern struct Client client;

/*DECLARE_INSTANCE_TYPE(Player)

namespace SqPlus{
template<>
void Push<const CoordSys>(SQVM *v, const CoordSys *cs){
	Push(v, (SQUserPointer)cs);
}
}*/

template<> const SQChar *const SQIntrinsic<Quatd>::classname = _SC("Quatd");
template<> const SQUserPointer *const SQIntrinsic<Quatd>::typetag = &tt_Quatd;

template<> const SQChar *const SQIntrinsic<Vec3d>::classname = _SC("Vec3d");
template<> const SQUserPointer *const SQIntrinsic<Vec3d>::typetag = &tt_Vec3d;

static const SQChar *CONSOLE_COMMANDS = _SC("console_commands");

int sqa_console_command(int argc, char *argv[], int *retval){
	HSQUIRRELVM &v = sqa::g_sqvm;
	sqa::StackReserver sr(v);
	sq_pushroottable(v); // root
	sq_pushstring(v, CONSOLE_COMMANDS, -1); // root "console_commands"
	if(SQ_FAILED(sq_get(v, -2))) // root table
		return 0;
	sq_pushstring(v, argv[0], -1); // root table argv[0]
	if(SQ_FAILED(sq_get(v, -2))) // root table closure
		return 0;

	switch(sq_gettype(v, -1)){
	case OT_CLOSURE:

		// Pass all arguments as strings (no conversion is done beforehand).
		sq_pushroottable(v);
		for(int i = 1; i < argc; i++)
			sq_pushstring(v, argv[i], -1);

		// It's no use examining returned value in that case calling the function iteslf fails.
		if(SQ_FAILED(sq_call(v, argc, SQTrue, SQTrue)))
			return 0;
		break;

	case OT_ARRAY:
		// Retrieve function packed into an array.
		sq_pushinteger(v, 0);
		if(SQ_FAILED(sq_get(v, -2))) // [closure] closure
			return -1;
		sq_pushroottable(v); // [closure] closure root

		// Pack all arguments into an array
		sq_newarray(v, argc-1); // [closure] closure root [null, ...]
		for(int i = 1; i < argc; i++){
			sq_pushinteger(v, i); // [closure] closure root [...] i
			sq_pushstring(v, argv[i], -1); // [closure] closure root [...] i arg[i]
			sq_set(v, -3);
		}
		if(SQ_FAILED(sq_call(v, 2, SQTrue, SQTrue))) // [closure] closure root [...]
			return 0;
		break;

	default:
		return sq_throwerror(v, _SC("The console command is not a function"));
	}

	// Assume returned value integer.
	SQInteger retint;
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
const SQUserPointer tt_Vec3d = const_cast<char*>("Vec3d");
const SQUserPointer tt_Quatd = const_cast<char*>("Quatd");
const SQUserPointer tt_Entity = const_cast<char*>("Entity");
const SQUserPointer tt_GLwindow = const_cast<char*>("GLwindow");
const SQUserPointer tt_Game = const_cast<char*>("Game");


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


static SQInteger sqf_set_cvar(HSQUIRRELVM v){
	SQInteger nargs = sq_gettop(v); //number of arguments
	SQObjectType ty = sq_gettype(v, 1);
	if(3 <= nargs && sq_gettype(v, 2) == OT_STRING && sq_gettype(v, 3) == OT_STRING){
		const SQChar *args[3];
		args[0] = _SC("set");
		sq_getstring(v, 2, &args[1]);
		sq_getstring(v, 3, &args[2]);
		cmd_set(3, const_cast<char**>(args));
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

/// A function that enables Squirrel codes to register console commands.
/// Command line arguments are passed as function parameters.
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
			return sq_throwerror(v, _SC("Could not allocate console_commands"));
	}

	sq_push(v, 2); // root table name
	sq_push(v, 3); // root table name closure
	sq_newslot(v, -3, SQFalse); // root table
	return 0;
}

/// This version packs arguments to an array before calling Squirrel function.
/// The array-packed arguments are occasionally useful when variable arguments are not appropriate.
/// To make the designation of calling convention whether to use array-packed arguments,
/// the function is enclosed inside an array.
/// This is not very smart, but array-packed arguments are expected to be rare, so
/// it's acceptable load to introduce extra indirection layer.
static SQInteger sqf_register_console_command_a(HSQUIRRELVM v){
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
			return sq_throwerror(v, _SC("Could not allocate console_commands"));
	}

	sq_push(v, 2); // root table name
	sq_newarray(v, 1);
	sq_pushinteger(v, 0); // root table name [nil] 0
	sq_push(v, 3); // root table name [nil] 0 closure
	sq_set(v, -3); // root table name [closure]
	sq_newslot(v, -3, SQFalse); // root table
	return 0;
}

static SQInteger sqf_timemeas(HSQUIRRELVM v){
	sq_push(v, 1);
	timemeas_t tm;
	TimeMeasStart(&tm);
	if(SQ_FAILED(sq_call(v, 1, SQTrue, SQTrue)))
		return SQ_ERROR;
	sq_newtable(v);
	sq_pushstring(v, _SC("time"), -1);
	sq_pushfloat(v, SQFloat(TimeMeasLap(&tm)));
	sq_newslot(v, -3, SQFalse);
	sq_pushstring(v, _SC("result"), -1);
	sq_push(v, -3);
	sq_newslot(v, -3, SQFalse);
	return 1;
}

/// Returns whether this program is built with debug profile.
static SQInteger sqf_debugBuild(HSQUIRRELVM v){
	sq_pushbool(v,
#ifndef NDEBUG
		SQTrue
#else
		SQFalse
#endif
		);
	return 1;
}

/// Returns if the build is for x64 target.
static SQInteger sqf_x64Build(HSQUIRRELVM v){
	sq_pushbool(v,
#ifdef _WIN64
		SQTrue
#else
		SQFalse
#endif
		);
	return 1;
}



/// Time measurement class constructor.
static SQInteger sqf_TimeMeas_construct(HSQUIRRELVM v){
#if 1
	timemeas_t *p = new timemeas_t;
	TimeMeasStart(p);
	sq_setinstanceup(v, 1, p);
	return 0;
#else
	sq_pushstring(v, _SC("a"), -1);
	timemeas_t &tm = *(timemeas_t*)sq_newuserdata(v, sizeof(timemeas_t));
	sq_set(v, -3);
	TimeMeasStart(&tm);
	return 0;
#endif
}

/// Time measurement class lap time accessor.
static SQInteger sqf_TimeMeas_lap(HSQUIRRELVM v){
#if 1
	timemeas_t *p;
	if(SQ_FAILED(sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL)))
		return SQ_ERROR;
	sq_pushfloat(v, SQFloat(TimeMeasLap(p)));
	return 1;
#else
	timemeas_t *p;
	sq_pushstring(v, _SC("a"), -1);
	sq_get(v, 1);
	if(SQ_FAILED(sq_getuserdata(v, -1, (SQUserPointer*)&p, NULL)))
		return SQ_ERROR;
	sq_pushfloat(v, TimeMeasLap(p));
	return 1;
#endif
}
const SQUserPointer tt_TimeMeas = const_cast<char*>("TimeMeas");









static SQInteger sqf_Vec3d_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	SQUserPointer up;
	if(SQ_FAILED(sq_getinstanceup(v, 1, &up, tt_Vec3d)))
		return sq_throwerror(v, _SC("NoInstanceUP"));
	Vec3d &vec = *(Vec3d*)up;
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
	return 0;
}

static SQInteger sqf_Quatd_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	SQUserPointer up;
	if(SQ_FAILED(sq_getinstanceup(v, 1, &up, tt_Quatd)))
		return sq_throwerror(v, _SC("NoInstanceUP"));
	Quatd &vec = *(Quatd*)up;
	for(int i = 0; i < 4; i++){
		SQFloat f;
		if(i + 2 <= argc && !SQ_FAILED(sq_getfloat(v, i + 2, &f)))
			vec[i] = f;
		else
			vec[i] = 0.;
	}
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

/// It's too repetitive to write this for unary minus of Vec3d and conjugate of Quatd separately.
template<typename T, T (T::*proc)()const>
static SQInteger sqf_unary(HSQUIRRELVM v){
	try{
		SQIntrinsic<T> q;
		q.getValue(v, 1);
		// This local variable is necessary for gcc.
		T rv = (q.value.*proc)();
		SQIntrinsic<T> r(rv);
		r.push(v);
		return 1;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

/// It's too repetitive to write this for binary add, subtract, vector product of Vec3d and multiplication of Quatd separately.
template<typename T, T (T::*proc)(const T &)const, bool hasScalarproc, T (*scalarproc)(const T &, double)>
static SQInteger sqf_binary(HSQUIRRELVM v){
	try{
		SQIntrinsic<T> q;
		q.getValue(v, 1);
		T rv;
		if(hasScalarproc && sq_gettype(v, 2) & SQOBJECT_NUMERIC){
			SQFloat f;
			sq_getfloat(v, 2, &f);
			rv = scalarproc(q.value, f);
		}
		else{
			SQIntrinsic<T> o;
			o.getValue(v, 2);
			// This local variable is necessary for gcc.
			rv = (q.value.*proc)(o.value);
		}
		SQIntrinsic<T> r(rv);
		r.push(v);
		return 1;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

template<typename T, T (T::*proc)(double)>
T t_scalarproc(const T &a, double o){
	return (a.*proc)(o);
}

template<typename T>
T dummyproc(const T &a, double){
	return a;
}

template<typename T, T (T::*proc)(const T &)const>
static SQInteger sqf_binary(HSQUIRRELVM v){
	return sqf_binary<T, proc, false, dummyproc<T> >(v);
}

template<typename T, T (T::*proc)(double)const>
T scalarproc(const T &a, double o){
	return (a.*proc)(o);
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
			sq_pushfloat(v, SQFloat(q.value[s[0] - 'x']));
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

static SQInteger sqf_Vec3d_scale(HSQUIRRELVM v){
	try{
		SQFloat f;
		if(SQ_FAILED(sq_getfloat(v, -1, &f)))
			return SQ_ERROR;
		SQVec3d q;
		q.getValue(v, 1);
		SQVec3d o;
		o.value = q.value * f;
		o.push(v);
		return 1;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

static SQInteger sqf_Vec3d_divscale(HSQUIRRELVM v){
	try{
		SQFloat f;
		if(SQ_FAILED(sq_getfloat(v, -1, &f)))
			return SQ_ERROR;
		SQVec3d q;
		q.getValue(v, 1);
		SQVec3d o;
		o.value = q.value / f;
		o.push(v);
		return 1;
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
		sq_pushfloat(v, SQFloat(q.value.sp(o.value)));
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
			sq_pushfloat(v, SQFloat(q.value[s[0] == 'w' ? 3 : s[0] - 'x']));
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

/// It's too repetitive to write this for Vec3d and Quatd separately.
template<typename T>
static SQInteger sqf_normin(HSQUIRRELVM v){
	try{
		SQIntrinsic<T> q;
		q.getValue(v, 1);
		q.pointer->normin();
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

static SQInteger sqf_Quatd_direction(HSQUIRRELVM v){
	try{
		SQVec3d q;
		q.getValue(v, 2);
		SQQuatd r;
		r.value = Quatd::direction(q.value);
		r.push(v);
		return 1;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

static SQInteger sqf_Quatd_rotation(HSQUIRRELVM v){
	try{
		SQFloat f;
		if(SQ_FAILED(sq_getfloat(v, 2, &f)))
			return SQ_ERROR;
		SQVec3d q;
		q.getValue(v, 3);
		SQQuatd r;
		r.value = Quatd::rotation(f, q.value);
		r.push(v);
		return 1;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}



/// \return Translated string. It is dynamic string because the Squirrel VM does not promise
/// returned string pointer is kept alive untl returning this function.
gltestp::dstring sqa_translate(const SQChar *src){
	HSQUIRRELVM v = g_sqvm;
	const SQChar *str;
	sq_pushroottable(v); // root
	sq_pushstring(v, _SC("translate"), -1); // root "tlate"
	if(SQ_FAILED(sq_get(v, -2))){ // root tlate
		sq_poptop(v);
		return src;
	}
	sq_pushroottable(v); // root tlate root
	sq_pushstring(v, src, -1); // root tlate root "Deploy"
	sq_call(v, 2, SQTrue, SQTrue); // root tlate "..."
	if(SQ_FAILED(sq_getstring(v, -1, &str)))
		str = "Deploy";
	gltestp::dstring ret = str;
	sq_pop(v, 3);
	return ret;
}


static SQInteger sqf_reg(HSQUIRRELVM v){
	sq_pushregistrytable(v);
	return 1;
}

static SQInteger sqf_sendClientMessage(HSQUIRRELVM v){
	try{
		const SQChar *str;
		if(SQ_FAILED(sq_getstring(v, 2, &str)))
			return SQ_ERROR;
		ClientMessage::CtorMap::iterator it = ClientMessage::ctormap().find(str);
		if(it != ClientMessage::ctormap().end()){
			if(!it->second->sq_send(application, v))
				return sq_throwerror(v, _SC("The client message is not supported for Squirrel scripts."));
		}
		return 0;
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.what());
	}
}

#ifndef _WIN32
#define __stdcall
#endif

template<void (__stdcall *fp)()> SQInteger sqf_adapter0(HSQUIRRELVM v){
	fp();
	return 0;
}

/*template<void (__stdcall *const fp)(int)> SQInteger sqf_adapter1(HSQUIRRELVM v){
	int a0;
	if(SQ_FAILED(sq_getinteger(v, 2, &a0)))
		return sq_throwerror(v, _SC("The first argument must be integer"));
	fp(a0);
	return 0;
}*/

template<void (__stdcall *fp)(GLenum)> SQInteger sqf_adapter1(HSQUIRRELVM v){
	SQInteger a0;
	if(SQ_FAILED(sq_getinteger(v, 2, &a0)))
		return sq_throwerror(v, _SC("The first argument must be integer"));
	fp(GLenum(a0));
	return 0;
}

template<typename FP, FP fp> SQInteger sqf_adapter(HSQUIRRELVM v){
}

#ifdef _WIN32
SQInteger sqf_glVertex(HSQUIRRELVM v){
	try{
		SQVec3d sqv;
		sqv.getValue(v, 2);
		glVertex3dv(sqv.value);
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.description);
	}
	return 0;
}

SQInteger sqf_glTranslate(HSQUIRRELVM v){
	try{
		SQVec3d sqv;
		sqv.getValue(v, 2);
		gldTranslate3dv(sqv.value);
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.description);
	}
	return 0;
}

static SQInteger sqf_glRasterPos(HSQUIRRELVM v){
	try{
		SQVec3d sqv;
		sqv.getValue(v, 2);
		glRasterPos3dv(sqv.value);
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.description);
	}
	return 0;
}

static SQInteger sqf_gldprint(HSQUIRRELVM v){
	try{
		const SQChar *s;
		sq_getstring(v, 2, &s);
		gldprintf(s);
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.description);
	}
	return 0;
}
#endif

struct ModuleEntry{
	void *handle;
	int refs;
};

typedef std::map<cpplib::dstring, ModuleEntry> ModuleMap;

static ModuleMap modules;

static SQInteger sqf_loadModule(HSQUIRRELVM v){
	try{
		const SQChar *name;
		if(SQ_FAILED(sq_getstring(v, 2, &name)))
			return SQ_ERROR;
#ifdef _WIN32
		if(void *lib = LoadLibrary(name)){
#else
		if(void *lib = dlopen(name, RTLD_LAZY)){
#endif
			ModuleMap::iterator it = modules.find(name);
			if(it != modules.end())
				it->second.refs++;
			else{
				modules[name].handle = lib;
				modules[name].refs = 1;
			}
			sq_pushinteger(v, modules[name].refs);
		}
		else{
			CmdPrint(cpplib::dstring() << "loadModule(\"" << name << "\") Failed!");
			sq_pushinteger(v, 0);
		}
		return 1;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

static SQInteger sqf_unloadModule(HSQUIRRELVM v){
	try{
		const SQChar *name;
		if(SQ_FAILED(sq_getstring(v, 2, &name)))
			return SQ_ERROR;
#ifdef _WIN32
		HMODULE hm = GetModuleHandle(name);
		if(hm){
			FreeLibrary(hm);
			sq_pushinteger(v, modules[name].refs--);
		}
		else
			return sq_throwerror(v, _SC("Failed to unload library."));
#else
		ModuleMap::iterator it = modules.find(name);
		if(it != modules.end())
			dlclose(it->second.handle);
#endif
		return 1;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

void unloadAllModules(){
	for(ModuleMap::iterator it = modules.begin(); it != modules.end(); it++) for(int i = 0; i < it->second.refs; i++){
#ifdef _WIN32
		HMODULE hm = GetModuleHandle(it->first);
		if(hm)
			FreeLibrary(hm);
#else
		dlclose(it->second.handle);
#endif
	}
	modules.clear();
}

typedef std::set<bool (*)(HSQUIRRELVM)> SQDefineSet;

static void traceParent(HSQUIRRELVM v, SQDefineSet &clset, const CoordSys::Static &s){
	if(s.st)
		traceParent(v, clset, *s.st);
	if(clset.find(s.sq_define) != clset.end())
		return;
	s.sq_define(v);
	clset.insert(s.sq_define);
}

static void traceParent(HSQUIRRELVM v, SQDefineSet &clset, Entity::EntityStatic &s){
	if(s.st())
		traceParent(v, clset, *s.st());
//	if(clset.find(&s.sq_define) != clset.end())
//		return;
	s.sq_define(v);
//	clset.insert(s.sq_define);
}

SQDefineMap &defineMap(){
	static SQDefineMap s;
	return s;
}


void sqa_init(Game *game, HSQUIRRELVM *pv){
//    SquirrelVM::Init();
//	v = SquirrelVM::GetVMPtr();
	player = game->player;
	if(!pv)
		pv = &g_sqvm;
	HSQUIRRELVM &v = *pv = sq_open(1024);
	game->sqvm = v;

	sqstd_seterrorhandlers(v);

	sq_setprintfunc(v, sqf_print, sqf_print); //sets the print function

	register_global_func(v, sqf_set_cvar, _SC("set_cvar"));
	register_global_func(v, sqf_get_cvar, _SC("get_cvar"));
	register_global_func(v, sqf_cmd, _SC("cmd"));
	register_global_func(v, sqf_reg, _SC("reg"));
	register_global_func(v, sqf_loadModule, _SC("loadModule"));
	register_global_func(v, sqf_unloadModule, _SC("unloadModule"));
	register_global_func(v, sqf_timemeas, _SC("timemeas"));
	register_global_func(v, sqf_debugBuild, _SC("debugBuild"));
	register_global_func(v, sqf_x64Build, _SC("x64Build"));
#ifdef _WIN32
	register_global_func(v, sqf_adapter1<glBegin>, _SC("glBegin"));
	register_global_func(v, sqf_glVertex, _SC("glVertex"));
	register_global_func(v, sqf_adapter0<glEnd>, _SC("glEnd"));
	register_global_func(v, sqf_adapter0<glPushMatrix>, _SC("glPushMatrix"));
	register_global_func(v, sqf_adapter0<glPopMatrix>, _SC("glPopMatrix"));
	register_global_func(v, sqf_adapter0<glLoadIdentity>, _SC("glLoadIdentity"));
	register_global_func(v, sqf_glTranslate, _SC("glTranslate"));
	register_global_func(v, sqf_glRasterPos, _SC("glRasterPos"));
	register_global_func(v, sqf_gldprint, _SC("gldprint"));
	register_global_var(v, GL_LINES, _SC("GL_LINES"));
#endif

    sq_pushroottable(v); //push the root table(were the globals of the script will be stored)

	sqstd_register_iolib(v);
	sqstd_register_mathlib(v);

	// Define class TimeMeas
	sq_pushstring(v, _SC("TimeMeas"), -1);
	sq_newclass(v, SQFalse);
	sq_settypetag(v, -1, tt_TimeMeas);
	register_closure(v, _SC("constructor"), sqf_TimeMeas_construct);
	register_closure(v, _SC("lap"), sqf_TimeMeas_lap);
	sq_createslot(v, -3);

	// Define class Vec3d, native vector representation
	sq_pushstring(v, _SC("Vec3d"), -1);
	sq_newclass(v, SQFalse);
	sq_settypetag(v, -1, tt_Vec3d);
	sq_setclassudsize(v, -1, sizeof(Vec3d));
	register_closure(v, _SC("constructor"), sqf_Vec3d_constructor);
	register_closure(v, _SC("_tostring"), sqf_Vec3d_tostring);
	register_closure(v, _SC("_add"), sqf_binary<Vec3d, &Vec3d::operator+ >);
	register_closure(v, _SC("_sub"), sqf_binary<Vec3d, &Vec3d::operator- >);
	register_closure(v, _SC("_mul"), sqf_Vec3d_scale);
	register_closure(v, _SC("_div"), sqf_Vec3d_divscale);
	register_closure(v, _SC("_unm"), sqf_unary<Vec3d, &Vec3d::operator- >);
	register_closure(v, _SC("normin"), sqf_normin<Vec3d>);
	register_closure(v, _SC("norm"), sqf_unary<Vec3d, &Vec3d::norm>);
	register_closure(v, _SC("sp"), sqf_Vec3d_sp);
	register_closure(v, _SC("vp"), sqf_binary<Vec3d, &Vec3d::vp>);
	register_closure(v, _SC("_get"), sqf_Vec3d_get);
	register_closure(v, _SC("_set"), sqf_Vec3d_set);
	register_code_func(v, _SC("len"), _SC("return ::sqrt(this.sp(this));"));
	sq_createslot(v, -3);

	// Define class Quatd
	sq_pushstring(v, _SC("Quatd"), -1);
	sq_newclass(v, SQFalse);
	sq_settypetag(v, -1, tt_Quatd);
	sq_setclassudsize(v, -1, sizeof(Quatd));
	register_closure(v, _SC("constructor"), sqf_Quatd_constructor);
	register_closure(v, _SC("_tostring"), sqf_Quatd_tostring);
	register_closure(v, _SC("_get"), sqf_Quatd_get);
	register_closure(v, _SC("_set"), sqf_Quatd_set);
	register_closure(v, _SC("normin"), sqf_normin<Quatd>);
	register_closure(v, _SC("_mul"), sqf_binary<Quatd, &Quatd::operator*, true, scalarproc<Quatd, &Quatd::scale> >);
	register_closure(v, _SC("trans"), sqf_Quatd_trans);
	register_closure(v, _SC("cnj"), sqf_unary<Quatd, &Quatd::cnj>/*sqf_Quatd_cnj*/);
	register_closure(v, _SC("norm"), sqf_unary<Quatd, &Quatd::norm>);
	register_closure(v, _SC("direction"), sqf_Quatd_direction);
	register_closure(v, _SC("rotation"), sqf_Quatd_rotation);
	register_code_func(v, _SC("len"), _SC("return ::sqrt(x * x + y * y + z * z + w * w);"));
	sq_createslot(v, -3);

	// Define all CoordSys-derived classes.
	SQDefineSet clset;
	CoordSys::CtorMap::const_iterator it = CoordSys::ctormap().begin();
	for(; it != CoordSys::ctormap().end(); it++){
		traceParent(v, clset, *it->second);
	}

	// Define all Entity-derived classes.
	{
		Entity::EntityCtorMap::const_iterator it = Entity::constEntityCtorMap().begin();
		for(; it != Entity::constEntityCtorMap().end(); it++){
			traceParent(v, clset, *it->second);
		}
	}

	// Define class Game
	sq_pushstring(v, _SC("Game"), -1);
	sq_newclass(v, SQFalse);
	sq_settypetag(v, -1, tt_Game);
	sq_createslot(v, -3);

	// Define instance of class Game
	sq_pushstring(v, _SC("game"), -1); // this "game"
	sq_pushstring(v, _SC("Game"), -1); // this "game" "Game"
	sq_get(v, -3); // this "game" Game
	sq_createinstance(v, -1); // this "game" Game Game-instance
	sq_setinstanceup(v, -1, game); // this "game" Game Game-instance
	sq_remove(v, -2); // this "game" Game-instance
	sq_createslot(v, -3); // this

	// Register the game as the foreign pointer of the VM.
	sq_setforeignptr(v, game);

	// Define class Docker, leaving details to the class itself.
//	Docker::sq_define(v);

	// Define instance of class Universe
	if(game->universe){
		sq_pushstring(v, _SC("universe"), -1); // this "universe"
		CoordSys::sq_pushobj(v, game->universe);
		sq_createslot(v, -3); // this
	}

	// Define class Player
	Player::sq_define(v);

	if(game->player){
		sq_pushstring(v, _SC("player"), -1); // this "player"
		Player::sq_pushobj(v, game->player); // this "player" Player-instance
		sq_createslot(v, -3); // this
	}

	// Define class SquirrelShare
	SquirrelShare::sq_define(v);

	// Prevent duplicate instance of SquirrelShare.
	// The client will define one as it receives serialization stream.
	if(game->isServer())
		game->setSquirrelShare(new SquirrelShare(game));

	register_global_func(v, sqf_register_console_command, _SC("register_console_command"));
	register_global_func(v, sqf_register_console_command_a, _SC("register_console_command_a"));

	register_global_func(v, sqf_sendClientMessage, _SC("sendClientMessage"));

	// Execute initializations registered to defineMap.
	// This must be placed after all other base classes are defined.
	for(SQDefineMap::iterator it = defineMap().begin(); it != defineMap().end(); it++)
		it->second(v);

	sq_pushstring(v, _SC("stellar_file"), -1);
	sq_pushstring(v, _SC("space.dat"), -1);
	sq_createslot(v, 1);

	timemeas_t tm;
	TimeMeasStart(&tm);
	const SQChar *scriptFile = game->isServer() ? _SC("scripts/init.nut") : _SC("scripts/initClient.nut");
	if(SQ_SUCCEEDED(sqstd_dofile(v, scriptFile, 0, 1))) // also prints syntax errors if any 
	{
		double d = TimeMeasLap(&tm);
		CmdPrint(cpplib::dstring() << scriptFile << " total: " << d << " sec");
//		call_foo(v,1,2.5,_SC("teststring"));
	}
	else
		CmdPrint(cpplib::dstring(scriptFile) << " failed.");

	sq_poptop(v); // Pop the root table.
}

void sqa_anim0(HSQUIRRELVM v){
	sq_pushroottable(v); // root
	sq_pushstring(v, _SC("init_Universe"), -1); // root "init_Universe"
	if(SQ_SUCCEEDED(sq_get(v, -2))){ // root closure
		sq_pushroottable(v);
		sq_call(v, 1, SQFalse, SQTrue);
		sq_poptop(v);
	}
//	sq_poptop(v);
}

void sqa_anim(HSQUIRRELVM v, double dt){
	sq_pushroottable(v);
	sq_pushstring(v, _SC("frameproc"), -1);
	if(SQ_SUCCEEDED(sq_get(v, -2))){
		sq_pushroottable(v);
		sq_pushfloat(v, SQFloat(dt)); 
		sq_call(v, 2, SQFalse, SQTrue);
		sq_poptop(v); // pop the closure
	}
	sq_poptop(v);
}

void sqa_exit(){
	sq_pop(v,1); //pops the root table

//	SquirrelVM::Shutdown();
	sq_close(v);
	v = NULL; // To prevent other destructors from using dangling pointer.

	unloadAllModules();
}


void register_global_func(HSQUIRRELVM v,SQFUNCTION f,const SQChar *fname)
{
    sq_pushroottable(v);
    sq_pushstring(v,fname,-1);
    sq_newclosure(v,f,0); //create a new function
    sq_createslot(v,-3); 
    sq_pop(v,1); //pops the root table    
}

void register_global_var(HSQUIRRELVM v, int var, const SQChar *vname){
    sq_pushroottable(v);
    sq_pushstring(v,vname,-1);
    sq_pushinteger(v,var);
    sq_createslot(v,-3); 
    sq_pop(v,1); //pops the root table    
}

bool register_closure(HSQUIRRELVM v, const SQChar *fname, SQFUNCTION f, SQInteger nparams, const SQChar *params){
	StackReserver sr(v);
	sq_pushstring(v, fname, -1);
	sq_newclosure(v, f, 0);
	if(nparams)
		sq_setparamscheck(v, nparams, params);
	if(SQ_FAILED(sq_createslot(v, -3)))
		return false;
	return true;
}

bool register_code_func(HSQUIRRELVM v, const SQChar *fname, const SQChar *code, bool nested){
	StackReserver sr(v);
	sq_pushstring(v, fname, -1);
	if(SQ_FAILED(sq_compilebuffer(v, code, strlen(code), fname, SQFalse)))
		return false;
	if(nested){
		sq_pushroottable(v);
		if(SQ_FAILED(sq_call(v, 1, SQTrue, SQTrue)))
			return false;
		sq_remove(v, -2); // Remove generator function
	}
	if(SQ_FAILED(sq_createslot(v, -3)))
		return false;
	return true;
}





/// \brief ClientMessage to notify something from the client's Squirrel VM to the server's one.
struct CMSQ : public ClientMessage{
	typedef ClientMessage st;
	static CMSQ s;
	void interpret(ServerClient &sc, UnserializeStream &uss);
	static void send();
private:
	static bool sqf_define(HSQUIRRELVM);
	static SQInteger sqf_call(HSQUIRRELVM);
	CMSQ();
};

CMSQ CMSQ::s;

CMSQ::CMSQ() : st("SQ"){
	defineMap()[id] = sqf_define;
}

void CMSQ::send(){
	if(application.mode & application.ServerBit){
		HSQUIRRELVM v = application.serverGame->sqvm;
		StackReserver sr(v);
		sq_pushroottable(v);
		sq_pushstring(v, _SC("clientMessage"), -1);
		if(SQ_SUCCEEDED(sq_get(v, -2))){
			sq_pushroottable(v);
			sq_call(v, 1, SQFalse, SQFalse);
		}
	}
	else{
		s.st::send(application, "", 0);
	}
}

bool CMSQ::sqf_define(HSQUIRRELVM v){
	register_closure(v, _SC("CMSQ"), &sqf_call);
	return true;
}

void CMSQ::interpret(ServerClient &sc, UnserializeStream &uss){
	if(application.mode & application.ServerBit){
		HSQUIRRELVM v = application.serverGame->sqvm;
		StackReserver sr(v);
		sq_pushroottable(v);
		sq_pushstring(v, _SC("clientMessage"), -1);
		if(SQ_SUCCEEDED(sq_get(v, -2))){
			sq_pushroottable(v);
			sq_call(v, 1, SQFalse, SQFalse);
		}
	}
}

SQInteger CMSQ::sqf_call(HSQUIRRELVM v){
	try{
		send();
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}


}