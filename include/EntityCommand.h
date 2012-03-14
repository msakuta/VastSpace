#ifndef ENTITYCOMMAND_H
#define ENTITYCOMMAND_H
/** \file
 * \brief Definition of EntityCommand and its subclasses. */
#include "export.h"
#include "CoordSys.h"
#include "ClientMessage.h"
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
struct SerializableCommand;

typedef EntityCommand *EntityCommandCreatorFunc(); ///< Type of EntityCommand-derived classes' constructor.
typedef void EntityCommandDeleteFunc(void *); ///< Type of EntityCommand-derived classes' destructor.
typedef void EntityCommandSqFunc(HSQUIRRELVM, Entity &); ///< Type of EntityCommand-derived classes' method to apply.
typedef void EntityCommandUnserializeFunc(UnserializeContext &, Entity &); ///< Unserializes and sends the command to the Entity.

/// \brief Static data type for a EntityCommand-derived class.
struct EXPORT EntityCommandStatic{
	/// \brief Constructor for the class object.
	/// 
	/// The object is 'raw' constructed. That means the content is not really initialized.
	/// Typically, the object is unserialized to initialize later.
	EntityCommandCreatorFunc *newproc;

	EntityCommandDeleteFunc *deleteproc; ///< Destructor for the class object. It's not really used.
	EntityCommandSqFunc *sq_command; ///< Squirrel binding for the class. Construct, pass, destroy in the single function.
	EntityCommandUnserializeFunc *unserializeFunc;
	EntityCommandStatic(EntityCommandCreatorFunc a = NULL, EntityCommandDeleteFunc b = NULL,
		EntityCommandSqFunc c = NULL, EntityCommandUnserializeFunc d = NULL) : newproc(a), deleteproc(b), sq_command(c), unserializeFunc(d){}
};


struct SerializableCommand;

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

	/// \brief Returns pointer to SerializableCommand if the class really derives it.
	virtual SerializableCommand *toSerializable();

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
	EntityCommandRegister(EntityCommandCreatorFunc a, EntityCommandDeleteFunc b);
};

/// \brief Serializable EntityCommand.
///
/// Although EntityCommand is not allowed to be stored to inter-frame storage (i.e. lives longer than
/// the span of a frame), there's a necessity of serialization when one wants to send an
/// EntityCommand to the server. But not all EntityCommands do need it, so we insert a intermediate
/// class that guarantee derived classes are serializable.
struct EXPORT SerializableCommand : EntityCommand{
	typedef EntityCommand st;

	/// \brief Default construction is allowed.
	SerializableCommand(){}

	/// \brief Returns this object.
	virtual SerializableCommand *toSerializable();

	virtual void serialize(SerializeContext &sc);

	virtual void unserialize(UnserializeContext &usc);
};


/// \brief A client message that encapsulates EntityCommand (precisely, SerializableCommand).
struct EXPORT CMEntityCommand : ClientMessage{
	typedef ClientMessage st;
	static CMEntityCommand s;
	void interpret(ServerClient &sc, UnserializeStream &uss);
	void send(Entity *e, SerializableCommand &com);
protected:
	CMEntityCommand() : st("EntityCommand"){}
};


//-----------------------------------------------------------------------------
//  Function template definitions
//-----------------------------------------------------------------------------


/// \brief A template to generate 'raw' constructor for EntityCommand.
template<typename Command>
EntityCommand *EntityCommandCreator(){
	return new Command();
}

/// \brief A template to destroy an EntityCommand generated by EntityCommandCreator().
///
/// Probably manually invoke delete operator might be simpler.
template<typename Command>
void EntityCommandDeletor(void *pv){
	delete reinterpret_cast<Command*>(pv);
}

/// \brief A template to generate squirrel bindings automatically.
///
/// You can instantiate this function template for any Command to implement custom codes.
template<typename Command>
void EntityCommandSq(HSQUIRRELVM v, Entity &e){
	Command com(v, e);
	e.command(&com);
	if(SerializableCommand *scom = com.toSerializable()){
		CMEntityCommand::s.send(&e, *scom);
	}
}

/// \brief A template to generate unserialize and send function that just ignores EntityCommand.
/// \sa EntityCommandUnserialize
///
/// \param Command ignored, but necessary to match overloaded template functions.
///
/// TODO: The EntiyCommand instance is created though no operation is performed.
template<typename Command>
void EntityCommandUnserializeInt(UnserializeContext &, Entity &, EntityCommand &){
	// Just ignore
}

/// \brief A template to generate unserialize and send function for SerializableCommand automatically.
/// \sa EntityCommandUnserialize
template<typename Command>
void EntityCommandUnserializeInt(UnserializeContext &usc, Entity &e, SerializableCommand &com){
	com.unserialize(usc);
	e.command(&com);
}

/// \brief The "entry" function for unserialization and execution of a SerializableCommand.
///
/// First, we want to implement "unserialize and send" operation gathered into a function.
/// Using template functions, we can allocate the command object on to the stack, so we do not cost
/// heap memory at all.
///
/// This function is overloaded to change implementation, based on actual type.
/// I have considered using template argument to switch the two implementations, but the standard
/// forbids partial instantiation of function template.
/// We can, of course, use template class with static function to partially instantiate, but it's
/// less straightforward and intuitive compared to function overloading. (Probably less portable too.)
///
/// The idea behind this is inspired by Haskell's pattern matching. The user (who defines EntityCommand-
/// or SerializableCommand-derived classes) needs not to care about whether the base class is
/// serializable or not, except the serializable() and unserializable() methods.
/// Normally, this kind of polymorphism is achieved with virtual function overriding, but in this case
/// we do not have an instance of the EntityCommand, so some static way is needed.
/// Function template and its specialization are suitable for such metaprogramming.
template<typename Command>
void EntityCommandUnserialize(UnserializeContext &usc, Entity &e){
	EntityCommandUnserializeInt<Command>(usc, e, Command());
}



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



//-----------------------------------------------------------------------------
//  Individual EntityCommand definitions
//-----------------------------------------------------------------------------



DERIVE_COMMAND(HaltCommand, SerializableCommand);

struct EXPORT AttackCommand : public SerializableCommand{
	COMMAND_BASIC_MEMBERS(AttackCommand, EntityCommand);
	AttackCommand(){}
	AttackCommand(HSQUIRRELVM v, Entity &e);
	std::set<Entity*> ents;
	virtual void serialize(SerializeContext &);
	virtual void unserialize(UnserializeContext &);
};

DERIVE_COMMAND(ForceAttackCommand, AttackCommand);

struct EXPORT MoveCommand : public SerializableCommand{
	COMMAND_BASIC_MEMBERS(MoveCommand, EntityCommand);
	MoveCommand(){}
	MoveCommand(HSQUIRRELVM v, Entity &e);
	Vec3d destpos;
	virtual void serialize(SerializeContext &);
	virtual void unserialize(UnserializeContext &);
};

DERIVE_COMMAND(ParadeCommand, EntityCommand);
DERIVE_COMMAND(DeltaCommand, EntityCommand);
DERIVE_COMMAND(DockCommand, SerializableCommand);

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

struct EXPORT RemainDockedCommand : public SerializableCommand{
	COMMAND_BASIC_MEMBERS(RemainDockedCommand, EntityCommand);
	RemainDockedCommand(bool a = true) : enable(a){}
	RemainDockedCommand(HSQUIRRELVM v, Entity &e);
	bool enable;
	virtual void serialize(SerializeContext &);
	virtual void unserialize(UnserializeContext &);
};





//-----------------------------------------------------------------------------
//  EntityCommandRegister inline implementation
//-----------------------------------------------------------------------------

/// \brief Constructs an EntityCommandRegister template class.
///
/// Placed here because CMEntityCommand must be defined.
template<typename CommandDerived>
inline EntityCommandRegister<CommandDerived>::EntityCommandRegister(EntityCommandCreatorFunc a, EntityCommandDeleteFunc b)
	: EntityCommandStatic(a, b, EntityCommandSq<CommandDerived>, EntityCommandUnserialize<CommandDerived>)
{
	EntityCommand::registerEntityCommand(CommandDerived::sid, this);
}

#endif
