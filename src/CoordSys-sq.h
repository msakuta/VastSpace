/** \file CoordSys-sq.h
 * \brief CoordSys private header for Squirrel bindings
 */
#ifndef COORDSYS_SQ_H
#define COORDSYS_SQ_H
#include "CoordSys.h"
#include "sqserial.h"

template<typename T>
SQInteger sq_CoordSysConstruct(HSQUIRRELVM v){
	const SQChar *name;
	if(SQ_FAILED(sq_getstring(v, 2, &name)))
		return sq_throwerror(v, _SC("Argument is not convertible to string in CoordSys.constructor"));
	CoordSys *parent = CoordSys::sq_refobj(v, 3);
	if(!parent)
		return sq_throwerror(v, _SC("Argument is not convertible to CoordSys in CoordSys.constructor"));
	T *a = new T(name, parent);
	sqserial_findobj(v, a, [](HSQUIRRELVM v, Serializable *s){
		sq_push(v, 1);
		SQUserPointer p;
		if(SQ_FAILED(sq_getinstanceup(v, -1, &p, NULL)))
			throw SQFError("Something's wrong with Squirrel Class Instace of CoordSys.");
		new(p) SqSerialPtr<CoordSys>(v, static_cast<CoordSys*>(s));
		sq_setreleasehook(v, -1, [](SQUserPointer p, SQInteger size){
			((SqSerialPtr<CoordSys>*)p)->~SqSerialPtr<CoordSys>();
			return SQInteger(1);
		});
	});
	return SQInteger(0);
}

#endif
