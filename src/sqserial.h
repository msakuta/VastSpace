/** \file
 * \brief A private header associated with both Squirrel VM and Serializable objects.
 */
#ifndef SQSERIAL_H
#define SQSERIAL_H
#include "serial.h"
#include "Observable.h"
#include <squirrel.h>

/// \brief Tries to find a Serializable object if it is already defined in the VM and
///        pushes it.
///
/// If it cannot find the designated object, it calls the callback function to push a new
/// instance of the object.
/// This extra layer of indirection can be a bit of overhead, but we want the same object
/// to be defined as the same instance also in Squirrel VM.
/// It mainly influences the behavior of equality operators (==,!=), which return false
/// if we create distinct instances when the same object is pushed twice.
/// A bonus is that we won't create multiple instances of WeakPtr, which allocates entry
/// to the referenced object's Observer table.  Memory usage efficiency will be improved.
///
/// This function is designed to accept various Serializable-derived classes by
/// callback function customization, because it's shared by at least CoordSys and Entity.
///
/// \param v The Squirrel VM.
/// \param s The object to be pushed.  Its SerializableId is checked for duplicates.
/// \param create A callback function to create a new instance of desired object and
///               pushes it to the stack if it's not already there.  It should preserve
///               the stack, which means no other objects should be left pushed to the
///               stack after the callback is finished.
void sqserial_findobj(HSQUIRRELVM v, Serializable *s, void create(HSQUIRRELVM v, Serializable *cs));

extern const SQChar *objectsTableName;

/// \brief A WeakPtr for use with Squirrel's class instance.
template<typename T>
class SqSerialPtr : public WeakPtr<T>{
	HSQUIRRELVM v;
public:
	SqSerialPtr(HSQUIRRELVM v, T *o) : WeakPtr<CoordSys>(o), v(v){}
	~SqSerialPtr(){}
	bool unlink(const Observable *o)override{
		// Do not invoke unlink() before getid()
		SerializableId id = static_cast<T*>(*this)->getid();
		// Do not invoke sq_deleteslot() before unlink()
		bool ret = WeakPtr<T>::unlink(o);
		do{
			sq_pushroottable(v); // root
			sq_pushstring(v, objectsTableName, -1); // root str
			if(SQ_FAILED(sq_get(v, -2))) // root table
				break;
			sq_pushinteger(v, id); // root {} id
			// This deleteslot in turn deletes this SqSerialPtr object.
			sq_deleteslot(v, -2, SQFalse);
		} while(0);
		return ret;
	}
};


#endif
