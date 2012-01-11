#ifndef ENTITYCOMMAND_H
#define ENTITYCOMMAND_H
/** \file
 * \brief Definition of EntityCommand and its subclasses. */
#include "export.h"
#include "CoordSys.h"
#include <set>
#include <map>
#include <cpplib/vec3.h>
#include <squirrel.h>

class Entity;

typedef const char *EntityCommandID;


/// Basic set of members necessary to operate EntityCommand offspring.
/// Constructors are necessary in addition to this.
#define COMMAND_BASIC_MEMBERS(name,base) \
	typedef base st;\
	static EntityCommandRegister<name> commandRegister;\
	static EntityCommandID sid;\
	virtual EntityCommandID id()const;\
	virtual bool derived(EntityCommandID)const;\

/// Define a EntityCommand-derived class in the main executable.
#define DERIVE_COMMAND(name,base) DERIVE_COMMAND_FULL(name,base,EXPORT,{},)

/// Define a EntityCommand-derived class with additional member variable defined in the main executable.
#define DERIVE_COMMAND_ADD(name,base,addmember) DERIVE_COMMAND_FULL(name,base,EXPORT,,addmember)

/// Define a EntityCommand-derived class in a extension module.
#define DERIVE_COMMAND_EXT(name,base) DERIVE_COMMAND_FULL(name,base,,{},)

/// Define a EntityCommand-derived class with additional member variable defined in a extension module.
#define DERIVE_COMMAND_EXT_ADD(name,base,addmember) DERIVE_COMMAND_FULL(name,base,,,addmember)

/// Define a EntityCommand-derived class with all options available.
#define DERIVE_COMMAND_FULL(name,base,linkage,sqconstructor,addmember) struct linkage name : public base{\
	COMMAND_BASIC_MEMBERS(name,base)\
	name(){}\
	name(HSQUIRRELVM v, Entity &) sqconstructor;\
	addmember;\
}

/// Implement basic members of EntityCommand offspring.
#define IMPLEMENT_COMMAND(name,idname) const char *name::sid = idname;\
	EntityCommandRegister<name> name::commandRegister(EntityCommandCreator<name>, EntityCommandDeletor<name>);\
	EntityCommandID name::id()const{return sid;}\
	bool name::derived(EntityCommandID aid)const{if(aid==sid)return true;else return st::derived(aid);}

struct EntityCommand;

typedef EntityCommand *EntityCommandCreatorFunc(HSQUIRRELVM, Entity &); ///< Type of EntityCommand-derived classes' constructor.
typedef void EntityCommandDeleteFunc(void *); ///< Type of EntityCommand-derived classes' destructor.
typedef void EntityCommandSqFunc(HSQUIRRELVM, Entity &); ///< Type of EntityCommand-derived classes' method to apply.

/// Static data type for a EntityCommand-derived class.
struct EntityCommandStatic{
	EntityCommandCreatorFunc *newproc; ///< Constructor for the class object. It's not really used.
	EntityCommandDeleteFunc *deleteproc; ///< Destructor for the class object. It's not really used.
	EntityCommandSqFunc *sq_command; ///< Squirrel binding for the class. Construct, pass, destroy in the single function.
	EntityCommandStatic(EntityCommandCreatorFunc a = NULL, EntityCommandDeleteFunc b = NULL, EntityCommandSqFunc c = NULL) : newproc(a), deleteproc(b), sq_command(c){}
};

/// A template to generate squirrel bindings automatically.
///
/// You can instanciate this function template for any Command to implement custom codes.
template<typename Command>
void EntityCommandSq(HSQUIRRELVM v, Entity &e){
	Command com(v, e);
	e.command(&com);
}

template<typename Command>
EntityCommand *EntityCommandCreator(HSQUIRRELVM v, Entity &e){
	return new Command(v, e);
}

template<typename Command>
void EntityCommandDeletor(void *pv){
	delete reinterpret_cast<Command*>(pv);
}

/** \brief Base class for all Entity commands.
//
// The Entity commands are short-lived, small object that deliver messages to Entities.
// The virtual mechanism of EntityCommand enables fast, safe, straightforward and extensible
// tree structure of commands.
//
// Entity class do not need to know all types of commands to receive one, only derived relevant
// class objects do.
//
// Note that this class object's storage duration is assumed to be one frame long at most.
// Containing an EntityCommand object or its derived objects within a long-lived objects,
// such as Entity itself, is prohibited. Writing into a file is of course NG.
// This is important convention because it can contain pointers to dynamic objects, which
// can get deleted at random occasion, that means they need serialization.
//
// To eliminate dangling pointers, we keep objects live until end of each frame, but the best
// practice is to create, pass and delete an EntityCommand object at the same place.
// If you really need to remember an EntityCommand for later use, I recommend using Squirrel
// function call to express a command.
 */
struct EXPORT EntityCommand{
	/// Type of the constructor map.
	typedef std::map<const char *, EntityCommandStatic*, bool (*)(const char *, const char *)> MapType;

	/// Constructor map. The key must be a pointer to a static string, which lives as long as the program.
	static MapType &ctormap();

	/** \brief Returns unique ID for this class.
	 *
	 * The returned pointer never be dereferenced without debugging purposes,
	 * it is just required to point the same address for all the instances but
	 * never coincides between different classes.
	 * A static const string of class name is ideal for this returned vale.
	 */
	virtual EntityCommandID id()const = 0;

	/** \brief Derived or exact class returns true.
	 *
	 * Returns whether the given Entity Command ID is the same as this object's class's or its derived classes.
	 */
	virtual bool derived(EntityCommandID)const;

	EntityCommand(){}

	/// In a Squirrel execution context, an Entity which will this command be sent to is known.
	EntityCommand(HSQUIRRELVM v, Entity &e){}

	/// Derived classes use this utility to register class.
	static int registerEntityCommand(const char *name, EntityCommandStatic *ctor){
		ctormap()[name] = ctor;
		return 0;
	}
};

/// A template class to implement EntityCommandStatic object for a given EntityCommand-derived class.
template<typename CommandDerived> struct EntityCommandRegister : public EntityCommandStatic{
	EntityCommandRegister(EntityCommandCreatorFunc a, EntityCommandDeleteFunc b)
		: EntityCommandStatic(a, b, EntityCommandSq<CommandDerived>)
	{
		EntityCommand::registerEntityCommand(CommandDerived::sid, this);
	}
};

/// A template function that tests whether given command matches the template argument class.
///
/// Re-invention of RTTI, but expected faster.
template<typename CmdType>
CmdType *InterpretCommand(EntityCommand *com){
	return com->id() == CmdType::sid ? static_cast<CmdType*>(com) : NULL;
}

/// A template function that tests whether given command derives the template argument class.
///
/// Re-invention of RTTI, but expected faster.
template<typename CmdType>
CmdType *InterpretDerivedCommand(EntityCommand *com){
	return com->derived(CmdType::sid) ? static_cast<CmdType*>(com) : NULL;
}


DERIVE_COMMAND(HaltCommand, EntityCommand);

struct EXPORT AttackCommand : public EntityCommand{
	COMMAND_BASIC_MEMBERS(AttackCommand, EntityCommand);
	AttackCommand(){}
	AttackCommand(HSQUIRRELVM v, Entity &e);
	std::set<Entity*> ents;
};

DERIVE_COMMAND(ForceAttackCommand, AttackCommand);

struct EXPORT MoveCommand : public EntityCommand{
	COMMAND_BASIC_MEMBERS(MoveCommand, EntityCommand);
	MoveCommand(){}
	MoveCommand(HSQUIRRELVM v, Entity &e);
	Vec3d destpos;
};

DERIVE_COMMAND(ParadeCommand, EntityCommand);
DERIVE_COMMAND(DeltaCommand, EntityCommand);
DERIVE_COMMAND(DockCommand, EntityCommand);

DERIVE_COMMAND(SetAggressiveCommand, EntityCommand);
DERIVE_COMMAND(SetPassiveCommand, EntityCommand);

struct EXPORT WarpCommand : public MoveCommand{
	COMMAND_BASIC_MEMBERS(WarpCommand, MoveCommand);
	WarpCommand(){}
	WarpCommand(HSQUIRRELVM v, Entity &e);

	/** There can be a discussion on whether warp destination is designated by a path or
	// a pointer. Finally, we choose a pointer because the entity commands are impulsive
	// by specification, which means they should be generated, passed and consumed in a
	// single frame, so we do not need to warry about pointed object's lifetime.
	// Also, it can be less efficient to convert between path string and pointer when
	// one that sent the command already know the destination system via pointer. */
	CoordSys *destcs;
};

struct EXPORT RemainDockedCommand : public EntityCommand{
	COMMAND_BASIC_MEMBERS(RemainDockedCommand, EntityCommand);
	RemainDockedCommand(bool a = true) : enable(a){}
	RemainDockedCommand(HSQUIRRELVM v, Entity &e);
	bool enable;
};

#endif
