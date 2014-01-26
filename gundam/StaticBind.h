/* \file
 * \brief Definition of StaticBind class.
 */
#ifndef GUNDAM_STATICBIND_H
#define GUNDAM_STATICBIND_H

#include <squirrel.h>

/// The base class for Squirrel-bound static variables.
///
/// Derived classes implement abstruct methods to interact with Squirrel VM.
///
/// This method's overhead compared to plain variable is just a virtual function
/// table pointer for each instance.
class StaticBind{
public:
	virtual void push(HSQUIRRELVM v) = 0;
	virtual bool set(HSQUIRRELVM v) = 0;
};

/// Integer binding.
class StaticBindInt : public StaticBind{
	int value;
	virtual void push(HSQUIRRELVM v){
		sq_pushinteger(v, value);
	}
	virtual bool set(HSQUIRRELVM v){
		SQInteger sqi;
		bool ret = SQ_SUCCEEDED(sq_getinteger(v, -1, &sqi));
		if(ret)
			value = sqi;
		return ret;
	}
public:
	StaticBindInt(int a) : value(a){}
	operator int&(){return value;}
};

/// Double binding.
class StaticBindDouble : public StaticBind{
	double value;
	virtual void push(HSQUIRRELVM v){
		sq_pushfloat(v, value);
	}
	virtual bool set(HSQUIRRELVM v){
		SQFloat sqi;
		bool ret = SQ_SUCCEEDED(sq_getfloat(v, -1, &sqi));
		if(ret)
			value = sqi;
		return ret;
	}
public:
	StaticBindDouble(double a) : value(a){}
	operator double&(){return value;}
};
#endif
