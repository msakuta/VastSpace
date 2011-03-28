#ifndef SQADAPT_H
#define SQADAPT_H
/// \file
/// \brief Squirrel library adapter for gltestplus project.
#include "export.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
#endif
int sqa_console_command(int argc, char *argv[], int *retval);

#ifdef __cplusplus

#include <squirrel.h>
#include <string.h>
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include <cpplib/dstring.h>


class Serializable;
class Entity;

/// Namespace for Squirrel adaption functions and classes.
namespace sqa{

void sqa_init(HSQUIRRELVM *v = NULL);
void sqa_anim0();
void sqa_anim(double dt);
void sqa_delete_Entity(Entity *);
void sqa_exit();

/// Register a Squirrel native function to the given VM.
void register_global_func(HSQUIRRELVM v,SQFUNCTION f,const SQChar *fname);
void register_global_var(HSQUIRRELVM v, int var, const SQChar *vname);

/// Register a Squirrel closure bound to the top of the stack. Useful with defining class methods.
bool register_closure(HSQUIRRELVM v, const SQChar *fname, SQFUNCTION f, SQInteger nparams = 0, const SQChar *params = NULL);

/// Register a Squirrel scripted closure bound to the top of the stack.
bool register_code_func(HSQUIRRELVM v, const SQChar *fname, const SQChar *code, bool nested = false);

/// Global Squirrel VM.
extern HSQUIRRELVM g_sqvm;

/// Type tags for intrinsic types.
extern EXPORT const SQUserPointer tt_Vec3d;
extern EXPORT const SQUserPointer tt_Quatd;
extern EXPORT const SQUserPointer tt_Entity;
extern EXPORT const SQUserPointer tt_GLwindow;

/// \param instanceindex The index of Squirrel class instance that is to be assigned.
bool sqa_newobj(HSQUIRRELVM v, Serializable *o, SQInteger instanceindex = -1);
bool sqa_refobj(HSQUIRRELVM v, SQUserPointer* o, SQRESULT *sr = NULL, int idx = 1, bool throwError = true);
void sqa_deleteobj(HSQUIRRELVM v, Serializable *o);

/// Translate given string with Squirrel defined translation function.
::cpplib::dstring sqa_translate(const SQChar *);

/// Any recoverable errors in Squirrel VM is thrown and inherits this class.
struct SQFError{
	SQFError(const SQChar *a = NULL) : description(a){}
	const SQChar *what()const;
	const SQChar *description;
};
struct SQFArgumentError : SQFError{ SQFArgumentError() : SQFError(_SC("Argument error")){} };
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
		if(SQ_FAILED(sq_get(v, idx < 0 ? idx - 1 : idx))) // ... a
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

/// "Class" can be CoordSys, Player or Entity. "MType" can be Vec3d or Quatd. "member" can be pos or rot, respectively.
/// I think I cannot write this logic shorter.
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

template<typename Class, typename MType, MType Class::*member>
inline MType membergetter(Class *p){
	return p->*member;
}

template<typename Class, typename MType, MType Class::*member>
inline void membersetter(Class p, MType &newvalue){
	p->*member = newvalue;
}

template<typename Class, typename MType, MType (Class::*member)()const>
inline MType accessorgetter(Class *p){
	return (p->*member)();
}

template<typename Class, typename MType, void (Class::*member)(MType)>
inline void accessorsetter(Class p, MType &newvalue){
	(p->*member)(newvalue);
}

#if 1

/// This template function is rather straightforward, but produces similar instances rapidly.
/// Probably classes with the same member variables should share base class, but it's not always feasible.
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
	else if(!strcmp(wcs, _SC("alive"))){
		SQUserPointer o;
		sq_pushbool(v, sqa_refobj(v, &o, NULL, 1, false));
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
	return SQ_ERROR;
}


/// A class to automatically restores Squirrel stack depth at the destruction of this object.
class StackReserver{
	SQInteger initial; ///< Initial depth of the stack.
	HSQUIRRELVM v;
public:
	StackReserver(HSQUIRRELVM v) : v(v), initial(sq_gettop(v)){}
	~StackReserver(){
		SQInteger end = sq_gettop(v);
		if(initial < end)
			sq_pop(v, end - initial);
	}
};


}

using namespace sqa;

#endif // __cplusplus

#endif
