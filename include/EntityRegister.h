#ifndef ENTITYREGISTER_H
#define ENTITYREGISTER_H
#include "Entity.h"

/// \brief A class template to automatically create registration of Entity.
///
/// This version cannot create instance from a Squirrel script.  "NC" stands for
/// Non-constructible.
template<typename T> class Entity::EntityRegisterNC : public Entity::EntityStatic{
	const SQChar *m_sq_classname;
	ClassId m_classid;
public:
	virtual ClassId classid(){ return m_classid; }
	virtual Entity *create(WarField *w){ return NULL; } ///< Prohibit creation of the Entity
	virtual Entity *stcreate(Game *game){ return new T(game); }
	virtual void destroy(Entity *p){ delete p; }
	virtual const SQChar *sq_classname(){ return m_sq_classname; }
	/// Called from sq_define(), override or instantiate to add member functions.
	virtual void sq_defineInt(HSQUIRRELVM v){}
	virtual bool sq_define(HSQUIRRELVM v){
		sq_pushstring(v, sq_classname(), -1);
		sq_pushstring(v, T::st::entityRegister.sq_classname(), -1);
		sq_get(v, 1);
		sq_newclass(v, SQTrue);
		sq_settypetag(v, -1, SQUserPointer(m_classid));
		sq_setclassudsize(v, -1, sq_udsize); // Set space for the weak pointer
		sq_defineInt(v);
		sq_createslot(v, -3);
		return true;
	}
public:
	virtual EntityStatic *st(){ return &T::st::entityRegister; }
	EntityRegisterNC(const SQChar *classname) : EntityStatic(classname), m_sq_classname(classname), m_classid(classname){
		Entity::registerEntity(classname, this);
	}
	EntityRegisterNC(const SQChar *classname, ClassId classid) : EntityStatic(classname), m_sq_classname(classname), m_classid(classid){
		Entity::registerEntity(classname, this);
	}
};

/// \brief Registers a Entity which is able to be created from a Squirrel script.
template<typename T> class Entity::EntityRegister : public Entity::EntityRegisterNC<T>{
public:
	EntityRegister(const SQChar *classname) : EntityRegisterNC<T>(classname){}
	virtual Entity *create(WarField *w){ return new T(w); } ///< Allow creation of the Entity
	virtual void sq_defineInt(HSQUIRRELVM v){}
};

#endif
