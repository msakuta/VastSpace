#ifndef SQADAPT_H
#define SQADAPT_H
// Squirrel library adapter for gltestplus project.
#include <stddef.h>
#include <squirrel.h>

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

}

using namespace sqa;

#endif
