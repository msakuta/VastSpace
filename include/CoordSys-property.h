#ifndef COORDSYS_PROPERTY_H
#define COORDSYS_PROPERTY_H

struct CoordSys::PropertyEntry{
	PropertyEntry(SQInteger (*get)(HSQUIRRELVM) = NULL, SQInteger (*set)(HSQUIRRELVM) = NULL) : get(get), set(set){}
	SQInteger (*get)(HSQUIRRELVM);
	SQInteger (*set)(HSQUIRRELVM);
};

template<typename T, T CoordSys::*memb>
SQInteger tgetter(HSQUIRRELVM v){
	SQIntrinsic<T>(CoordSys::sq_refobj(v)->*memb).push(v);
	return SQInteger(1);
}

template<typename T, T CoordSys::*memb>
SQInteger tsetter(HSQUIRRELVM v){
	SQIntrinsic<T> q;
	q.getValue(v, 3);
	CoordSys::sq_refobj(v)->*memb = q.value;
	return SQInteger(0);
}

#endif
