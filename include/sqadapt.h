#ifndef SQADAPT_H
#define SQADAPT_H
/// \file
/// \brief Squirrel library adapter for gltestplus project.
#include "export.h"
#include "dstring.h"
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

#include <map>

class Game;
class Serializable;
class Entity;

/// Namespace for Squirrel adaption functions and classes.
namespace sqa{

void sqa_init(Game *game, HSQUIRRELVM *v = NULL);
void sqa_anim0(HSQUIRRELVM);
void sqa_anim(HSQUIRRELVM v, double dt);
void sqa_exit();
SQRESULT sqa_dofile(HSQUIRRELVM, const char *filename, SQInteger retval = 0, SQBool printerror = SQTrue);

/// Register a Squirrel native function to the given VM.
void register_global_func(HSQUIRRELVM v,SQFUNCTION f,const SQChar *fname);
void register_global_var(HSQUIRRELVM v, int var, const SQChar *vname);

/// Register a Squirrel const value.
EXPORT bool register_const(HSQUIRRELVM v, const SQChar *vname, SQInteger var);

/// Register a Squirrel static member constant.
EXPORT bool register_static(HSQUIRRELVM v, const SQChar *vname, SQInteger var);

/// Register a Squirrel closure bound to the top of the stack. Useful with defining class methods.
EXPORT bool register_closure(HSQUIRRELVM v, const SQChar *fname, SQFUNCTION f, SQInteger nparams = 0, const SQChar *params = NULL);

/// Register a Squirrel scripted closure bound to the top of the stack.
bool register_code_func(HSQUIRRELVM v, const SQChar *fname, const SQChar *code, bool nested = false);

/// \brief Type for the Squirrel VM initializer callbacks.
///
/// Not necessary to be a map.
typedef std::map<gltestp::dstring, bool (*)(HSQUIRRELVM)> SQDefineMap;

/// \brief Returns the map object to initialize the Squirrel VM in the Game object.
///
/// Global initialize codes can call this function without caring order of initialization.
SQDefineMap &defineMap();

/// \brief An utility class to register some function into defineMap().
///
/// Be sure to define a global (static) variable of this class to function.
class Initializer{
public:
	Initializer(gltestp::dstring name, bool sqf(HSQUIRRELVM)){
		defineMap()[name] = sqf;
	}
};

/// Global Squirrel VM.
extern HSQUIRRELVM g_sqvm;

/// Type tags for intrinsic types.
extern EXPORT const SQUserPointer tt_Vec3d;
extern EXPORT const SQUserPointer tt_Quatd;
extern EXPORT const SQUserPointer tt_Entity;
extern EXPORT const SQUserPointer tt_GLwindow;

/// Translate given string with Squirrel defined translation function.
EXPORT ::gltestp::dstring sqa_translate(const SQChar *);

/// \brief Any recoverable errors in Squirrel VM is thrown and inherits this class.
struct EXPORT SQFError{
	/// \brief Construct this object with a string expression.
	SQFError(::gltestp::dstring a = "") : description(a){}

	/// \brief Convert this error to text expression.
	virtual const SQChar *what()const{return description;}

	/// \brief The description text.
	///
	/// Formerly, it's type is plain pointer to a character (at the head of a string), but in that way
	/// we cannot store dynamically constructed string into this object without worrying about storage
	/// duration, because the string buffer is probably out of scope at the time this object is catched.
	/// The only method to avoid that situation without the aid of a dynamic string is to make the string
	/// buffer a static variable, which is dangerous in multithreaded environment.
	/// We simply gave up to avoid using dynamic memory and used the dynamic string.
	///
	/// What if the exception is generated due to memory shortage? Probably we cannot do much anyway,
	/// just let the program crash.
	///
	/// Pure C++ers would use std::string here, but we're dirty ones.
	/// We want the string to be refcounted and copy-on-writed.
	::gltestp::dstring description;
};
struct EXPORT SQFArgumentError : SQFError{ SQFArgumentError() : SQFError(_SC("Argument error")){} };
struct EXPORT SQIntrinsicError : SQFError{};
struct EXPORT TypeMatch : SQIntrinsicError{};
struct EXPORT NoIndex : SQIntrinsicError{};
struct EXPORT NoUserData : SQIntrinsicError{};
struct EXPORT NoInstanceUP : SQIntrinsicError{};
struct EXPORT NoCreateInstance : SQIntrinsicError{};

/// \brief Adapter for Squirrel class instances that behave like an intrinsic type, such as vectors and quaternions.
template<typename Class>
class SQIntrinsic{
	static EXPORT const SQChar *const classname;
	static EXPORT const SQUserPointer *const typetag;
public:
	Class value;
	Class *pointer; // Holds pointer to Squirrel-managed memory obtained by last sq_getuserdata call.

	SQIntrinsic() : pointer(NULL){}
	SQIntrinsic(const Class &initValue) : value(initValue), pointer(NULL){}

	/// \brief Pushes the object to Squirrel stack.
	void push(HSQUIRRELVM v){
		sq_pushroottable(v); // ... root
		sq_pushstring(v, classname, -1); // ... root "Quatd"
		if(SQ_FAILED(sq_get(v, -2))) throw NoIndex(); // ... root Quatd
		if(SQ_FAILED(sq_createinstance(v, -1))) // ... root Quatd Quatd-instance
			throw NoCreateInstance();
		sq_remove(v, -2); // ... root Quatd-instance
		sq_remove(v, -2); // ... Quatd-instance
		if(SQ_FAILED(sq_getinstanceup(v, -1, (SQUserPointer*)&pointer, NULL))) // ... Quatd-instance "a" {ud}
			throw NoInstanceUP();
		*pointer = value;
	}

	/// \brief Gets a variable at index idx from Squirrel stack
	void getValue(HSQUIRRELVM v, int idx = -1){
#ifndef NDEBUG
		assert(OT_INSTANCE == sq_gettype(v, idx));
		SQUserPointer tt;
		sq_gettypetag(v, idx, &tt);
		assert(tt == *typetag);
#endif
		if(SQ_FAILED(sq_getinstanceup(v, idx, (SQUserPointer*)&pointer, NULL)))
			throw NoInstanceUP();
		value = *pointer;
	}

	/// \brief Sets a variable at index idx in Squirrel stack
	void setValue(HSQUIRRELVM v, int idx = -1){
#ifndef NDEBUG
		assert(OT_INSTANCE == sq_gettype(v, idx));
		SQUserPointer tt;
		sq_gettypetag(v, idx, &tt);
		assert(tt == *typetag);
#endif
		if(SQ_FAILED(sq_getinstanceup(v, -1, (SQUserPointer*)&pointer, NULL)))
			throw NoInstanceUP();
		*pointer = value;
		sq_poptop(v);
	}
};


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


/// "Class" can be Player. "MType" can be Vec3d or Quatd. "member" can be pos or rot, respectively.
/// I think I cannot write this logic shorter.
template<typename Class, typename MType, MType getter(Class *p), Class *sq_refobj(HSQUIRRELVM v, SQInteger idx)>
SQInteger sqf_getintrinsic2(HSQUIRRELVM v){
	try{
		Class *p = sq_refobj(v, 1);
		SQIntrinsic<MType> r;
		r.value = getter(p);
		r.push(v);
		return 1;
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.what());
	}
}

/// This template function is rather straightforward, but produces similar instances rapidly.
/// Probably classes with the same member variables should share base class, but it's not always feasible.
template<typename Class, typename MType, MType Class::*member, Class *sq_refobj(HSQUIRRELVM v, SQInteger idx)>
static SQInteger sqf_setintrinsic2(HSQUIRRELVM v){
	try{
		Class *p = sq_refobj(v, 1);
		SQIntrinsic<MType> r;
		r.getValue(v, 2);
		p->*member = r.value;
		return 0;
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.what());
	}
}

template<typename Class, typename MType, void (Class::*member)(const MType &), Class *sq_refobj(HSQUIRRELVM v, SQInteger idx)>
static SQInteger sqf_setintrinsica2(HSQUIRRELVM v){
	try{
		Class *p = sq_refobj(v, 1);
		SQIntrinsic<MType> r;
		r.getValue(v, 2);
		(p->*member)(r.value);
		return 0;
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.what());
	}
}

typedef SQIntrinsic<Quatd> SQQuatd;
typedef SQIntrinsic<Vec3d> SQVec3d;





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
