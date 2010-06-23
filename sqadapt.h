#ifndef SQADAPT_H
#define SQADAPT_H
// Squirrel library adapter for gltestplus project.
#include <stddef.h>

#ifdef __cplusplus
extern "C"
#endif
int sqa_console_command(int argc, char *argv[], int *retval);

#ifdef __cplusplus

#include <squirrel.h>
#include <cpplib/vec3.h>
#include <cpplib/quat.h>



class Serializable;
class Entity;

/// Namespace for Squirrel adaption functions and classes.
namespace sqa{

void sqa_init();
void sqa_anim0();
void sqa_anim(double dt);
void sqa_delete_Entity(Entity *);
void sqa_exit();

/// Register a Squirrel native function to the given VM.
SQInteger register_global_func(HSQUIRRELVM v,SQFUNCTION f,const SQChar *fname);

/// Global Squirrel VM.
extern HSQUIRRELVM g_sqvm;

/// Type tags for intrinsic types.
extern const SQUserPointer tt_Vec3d, tt_Quatd, tt_Entity;

bool sqa_newobj(HSQUIRRELVM v, Serializable *o, SQInteger instanceindex = -3);
bool sqa_refobj(HSQUIRRELVM v, SQUserPointer* o, SQRESULT *sr = NULL, int idx = 1);
void sqa_deleteobj(HSQUIRRELVM v, Serializable *o);

/// Any recoverable errors in Squirrel VM is thrown and inherits this class.
struct SQFError{
	SQFError(SQChar *a = NULL) : description(a){}
	const SQChar *what()const;
	SQChar *description;
};
struct SQFArgumentError : SQFError{ SQFArgumentError() : SQFError("Argument error"){} };
struct SQIntrinsicError : SQFError{};
struct TypeMatch : SQIntrinsicError{};
struct NoIndex : SQIntrinsicError{};
struct NoUserData : SQIntrinsicError{};
struct NoCreateInstance : SQIntrinsicError{};

/// Adapter for Squirrel class instances that behave like an intrinsic type.
template<typename Class>
class SQIntrinsic{
	static const SQChar *const classname;
	static const SQUserPointer *const typetag;
public:
	Class value;
	Class *pointer; // Holds pointer to Squirrel-managed memory obtained by last sq_getuserdata call.

	SQIntrinsic() : pointer(NULL){}
	SQIntrinsic(Class &initValue) : value(initValue), pointer(NULL){}

	void push(HSQUIRRELVM v){
		sq_pushroottable(v); // ... root
		sq_pushstring(v, classname, -1); // ... root "Quatd"
		if(SQ_FAILED(sq_get(v, -2))) throw NoIndex(); // ... root Quatd
		if(SQ_FAILED(sq_createinstance(v, -1))) // ... root Quatd Quatd-instance
			throw NoCreateInstance();
		sq_remove(v, -2); // ... root Quatd-instance
		sq_remove(v, -2); // ... Quatd-instance
		sq_pushstring(v, _SC("a"), -1); // ... Quatd-instance "a"
		pointer = (Class*)sq_newuserdata(v, sizeof value); // ... Quatd-instance "a" {ud}
		*pointer = value;
		if(SQ_FAILED(sq_set(v, -3)))
			throw NoIndex();
	}

	// Gets a variable at index idx from Squirrel stack
	void getValue(HSQUIRRELVM v, int idx = -1){
#ifndef NDEBUG
		assert(OT_INSTANCE == sq_gettype(v, idx));
		SQUserPointer tt;
		sq_gettypetag(v, idx, &tt);
		assert(tt == *typetag);
#endif
		sq_pushstring(v, _SC("a"), -1); // ... "a"
		if(SQ_FAILED(sq_get(v, idx))) // ... a
			throw NoIndex();
		if(SQ_FAILED(sq_getuserdata(v, -1, (SQUserPointer*)&pointer, NULL)))
			throw NoUserData();
		value = *pointer;
		sq_poptop(v);
	}

	// Sets a variable at index idx in Squirrel stack
	void setValue(HSQUIRRELVM v, int idx = -1){
#ifndef NDEBUG
		assert(OT_INSTANCE == sq_gettype(v, idx));
		SQUserPointer tt;
		sq_gettypetag(v, idx, &tt);
		assert(tt == *typetag);
#endif
		sq_pushstring(v, _SC("a"), -1); // ... "a"
		if(SQ_FAILED(sq_get(v, idx))) // ... a
			throw NoIndex();
		if(SQ_FAILED(sq_getuserdata(v, -1, (SQUserPointer*)&pointer, NULL)))
			throw NoUserData();
		*pointer = value;
		sq_poptop(v);
	}
};

template<> const SQChar *const SQIntrinsic<Quatd>::classname = _SC("Quatd");
template<> const SQUserPointer *const SQIntrinsic<Quatd>::typetag = &tt_Quatd;

template<> const SQChar *const SQIntrinsic<Vec3d>::classname = _SC("Vec3d");
template<> const SQUserPointer *const SQIntrinsic<Vec3d>::typetag = &tt_Vec3d;

typedef SQIntrinsic<Quatd> SQQuatd;
typedef SQIntrinsic<Vec3d> SQVec3d;


template<typename Class>
SQInteger sqf_get(HSQUIRRELVM v){
	Class *p;
	const SQChar *wcs;
	sq_getstring(v, -1, &wcs);
	SQRESULT sr;
	if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
		return sr;
	if(!strcmp(wcs, _SC("classname"))){
		sq_pushstring(v, p->classname(), -1);
		return 1;
	}
	sq_pushnull(v);
	return 1;
}

template<typename Class>
SQInteger sqf_set(HSQUIRRELVM v){
	if(sq_gettop(v) < 3)
		return SQ_ERROR;
	Class *p;
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	SQRESULT sr;
	if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
		return sr;
	return 0;
}



}

using namespace sqa;

#endif // __cplusplus

#endif
