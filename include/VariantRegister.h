/** \file
 * \brief Defines Entity::VariantRegister class template.
 *
 * Entity.h is included by so many sources that modifying it a bit makes the build process
 * take so much time.
 * So we separated definition of relatively rarely used class template definition to
 * another header.
 */
#ifndef VARIANTREGISTER_H
#define VARIANTREGISTER_H
#include "Entity.h"

/// \brief Constructor list for variants.
typedef std::map<gltestp::dstring, std::vector<gltestp::dstring> > ArmCtors;


/// \brief A class register object that defines variant of Assault.
///
/// Note that this class is, unlike EntityRegister, not singleton class, which means there could be multiple variants.
template<typename T> class Entity::VariantRegister : public Entity::EntityRegister<T>{
public:
	gltestp::dstring classname;
	const SQChar *variant;
	VariantRegister(gltestp::dstring classname, const SQChar *variant) :
		EntityRegister<T>(classname), classname(classname), variant(variant){}
	Entity *create(WarField *w){ return new T(w, variant); }
};


/// \brief Squirrel initializer entry that registers variant configurations in terms of equipments.
class EXPORT VariantProcess : public SqInitProcess{
public:
	const SQChar *name;
	ArmCtors &value;
	VariantProcess(ArmCtors &value, const char *name) : value(value), name(name){}
	virtual void process(HSQUIRRELVM)const;
};


#endif
