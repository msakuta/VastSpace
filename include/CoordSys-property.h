#ifndef COORDSYS_PROPERTY_H
#define COORDSYS_PROPERTY_H
#include "CoordSys.h"

struct CoordSys::PropertyEntry{
	typedef SQInteger (*GetCallback)(HSQUIRRELVM, const CoordSys*);
	typedef SQInteger (*SetCallback)(HSQUIRRELVM, CoordSys*);
	PropertyEntry(GetCallback get = NULL, SetCallback set = NULL) : get(get), set(set){}
	GetCallback get;
	SetCallback set;
};

template<typename T, T CoordSys::*memb>
SQInteger tgetter(HSQUIRRELVM v, const CoordSys *cs){
	SQIntrinsic<T>(cs->*memb).push(v);
	return SQInteger(1);
}

template<typename T, T CoordSys::*memb>
SQInteger tsetter(HSQUIRRELVM v, CoordSys *cs){
	SQIntrinsic<T> q;
	q.getValue(v, 3);
	cs->*memb = q.value;
	return SQInteger(0);
}

#endif
