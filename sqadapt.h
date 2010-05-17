#ifndef SQADAPT_H
#define SQADAPT_H
// Squirrel library adapter for gltestplus project.
#include <stddef.h>
#include <squirrel.h>
#include <cpplib/vec3.h>
#include <cpplib/quat.h>

class Serializable;
class Entity;

namespace sqa{

void sqa_init();
void sqa_anim0();
void sqa_anim(double dt);
void sqa_delete_Entity(Entity *);
void sqa_exit();

SQInteger register_global_func(HSQUIRRELVM v,SQFUNCTION f,const SQChar *fname);

extern HSQUIRRELVM g_sqvm;

extern const SQUserPointer tt_Vec3;
extern const SQUserPointer tt_Vec3d;
extern const SQUserPointer tt_Quatd, tt_Entity;

bool sqa_newobj(HSQUIRRELVM v, Serializable *o);
bool sqa_refobj(HSQUIRRELVM v, SQUserPointer* o, SQRESULT *sr = NULL, int idx = 1);
void sqa_deleteobj(HSQUIRRELVM v, Serializable *o);

// Adapter for Squirrel class instances that behave like an intrinsic type.
template<typename Class>
class SQIntrinsic{
	static const SQChar *const classname;
	static const SQUserPointer *const typetag;
public:
	Class value;
	Class *pointer; // Holds pointer to Squirrel-managed memory obtained by last sq_getuserdata call.

	SQIntrinsic() : pointer(NULL){}
	SQIntrinsic(Class &initValue) : value(initValue), pointer(NULL){}

	bool newValue(HSQUIRRELVM v){
		sq_pushroottable(v); // ... root
		sq_pushstring(v, classname, -1); // ... root "Quatd"
		sq_get(v, -2); // ... root Quatd
		sq_createinstance(v, -1); // ... root Quatd Quatd-instance
		sq_remove(v, -2); // ... root Quatd-instance
		sq_remove(v, -2); // ... Quatd-instance
		sq_pushstring(v, _SC("a"), -1); // ... Quatd-instance "a"
		sq_newuserdata(v, sizeof value); // ... Quatd-instance "a" {ud}
		if(SQ_FAILED(sq_getuserdata(v, -1, (SQUserPointer*)&pointer, NULL)))
			return false;
		*pointer = value;
		sq_set(v, -3);
		return true;
	}

	// Gets a variable at index idx from Squirrel stack
	bool getValue(HSQUIRRELVM v, int idx = -1){
#ifndef NDEBUG
		assert(OT_INSTANCE == sq_gettype(v, idx));
		SQUserPointer tt;
		sq_gettypetag(v, idx, &tt);
		assert(tt == *typetag);
#endif
		sq_pushstring(v, _SC("a"), -1); // ... "a"
		if(SQ_FAILED(sq_get(v, idx))) // ... a
			return false;
		if(SQ_FAILED(sq_getuserdata(v, -1, (SQUserPointer*)&pointer, NULL)))
			return false;
		value = *pointer;
		sq_poptop(v);
	}

	// Sets a variable at index idx in Squirrel stack
	bool setValue(HSQUIRRELVM v, int idx = -1){
#ifndef NDEBUG
		assert(OT_INSTANCE == sq_gettype(v, idx));
		SQUserPointer tt;
		sq_gettypetag(v, idx, &tt);
		assert(tt == *typetag);
#endif
		sq_pushstring(v, _SC("a"), -1); // ... "a"
		if(SQ_FAILED(sq_get(v, idx))) // ... a
			return false;
		if(SQ_FAILED(sq_getuserdata(v, -1, (SQUserPointer*)&pointer, NULL)))
			return false;
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
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!strcmp(wcs, _SC("pos"))){
		SQVec3d(p->pos).setValue(v, -1);
		return 1;
	}
	else if(!strcmp(wcs, _SC("rot"))){
		SQQuatd q;
		q.value = p->rot;
		q.setValue(v);
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
SQInteger sqf_set(HSQUIRRELVM v){
	if(sq_gettop(v) < 3)
		return SQ_ERROR;
	Class *p;
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	SQRESULT sr;
	if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
		return sr;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!strcmp(wcs, _SC("pos"))){
		SQQuatd q;
		q.getValue(v, 3);
		p->pos= q.value;
	}
	else if(!strcmp(wcs, _SC("rot"))){
		SQQuatd q;
		q.getValue(v, 3);
		p->rot = q.value;
	}
	return 0;
}

}

using namespace sqa;

#endif
