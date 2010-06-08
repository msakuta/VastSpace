#ifndef ENTITYCOMMAND_H
#define ENTITYCOMMAND_H
#include <set>
#include <map>
#include <cpplib/vec3.h>
#include <squirrel.h>

class Entity;

typedef const char *EntityCommandID;

#define DERIVE_COMMAND(name,base) struct name : public base{\
	typedef base st;\
	static int construction_dummy;\
	static EntityCommandID sid;\
	virtual EntityCommandID id()const;\
	virtual bool derived(EntityCommandID)const;\
	name(){}\
	name(HSQUIRRELVM v, Entity &e) : st(v,e){}\
}

#define IMPLEMENT_COMMAND(name,idname) const char *name::sid = idname;\
	int name::construction_dummy = registerEntityCommand(idname, EntityCommandCreator<name>);\
	EntityCommandID name::id()const{return sid;}\
	bool name::derived(EntityCommandID aid)const{if(aid==sid)return true;else return st::derived(aid);}

struct EntityCommand;

typedef EntityCommand *EntityCommandCreatorFunc(HSQUIRRELVM, Entity &);

template<typename Command>
EntityCommand *EntityCommandCreator(HSQUIRRELVM v, Entity &e){
	return new Command(v, e);
}

// Base class for all Entity commands.
// The Entity commands are short-lived, small object that deliver messages to Entities.
// The virtual mechanism of EntityCommand enables fast, safe, straightforward and extensible
// tree structure of commands.
// Entity class do not need to know all types of commands to receive one, only derived relevant
// class objects do.
// Note that this class object's storage duration is assumed to be one frame long at most.
// Containing an EntityCommand object or its derived objects within a long-lived objects,
// such as Entity itself, is prohibited. Writing into a file is of course NG.
// This is important convention because it can contain pointers to dynamic objects, which
// can get deleted at random occasion, that means they need serialization.
// To eliminate dangling pointers, we keep objects live until end of each frame, but the best
// practice is to create, pass and delete an EntityCommand object at the same place.
// If you really need to remember an EntityCommand for later use, I recommend using Squirrel
// function call to express a command.
struct EntityCommand{
	// Constructor map. The key must be a pointer to a static string, which lives as long as the program.
	static std::map<const char *, EntityCommandCreatorFunc*, bool (*)(const char *, const char *)> ctormap;

	// The returned pointer never be dereferenced without debugging purposes,
	// it is just required to point the same address for all the instances but
	// never coincides between different classes.
	// A static const string of class name is ideal for this returned vale.
	virtual EntityCommandID id()const = 0;
	virtual bool derived(EntityCommandID)const;

	EntityCommand(){}

	// In a Squirrel execution context, an Entity which will this command be sent to is known.
	EntityCommand(HSQUIRRELVM v, Entity &e){}

protected:
	// Derived classes use this utility to register class.
	static int registerEntityCommand(const char *name, EntityCommandCreatorFunc ctor){
		ctormap[name] = ctor;
		return 0;
	}
};

template<typename CmdType>
CmdType *InterpretCommand(EntityCommand *com){
	return com->id() == CmdType::sid ? static_cast<CmdType*>(com) : NULL;
}

template<typename CmdType>
CmdType *InterpretDerivedCommand(EntityCommand *com){
	return com->derived(CmdType::sid) ? static_cast<CmdType*>(com) : NULL;
}


DERIVE_COMMAND(HaltCommand, EntityCommand);

struct AttackCommand : public EntityCommand{
	typedef EntityCommand st;
	static int construction_dummy;
	static EntityCommandID sid;
	virtual EntityCommandID id()const;
	virtual bool derived(EntityCommandID)const;
	AttackCommand(){}
	AttackCommand(HSQUIRRELVM v, Entity &e) : st(v, e){}
	std::set<Entity*> ents;
};

DERIVE_COMMAND(ForceAttackCommand, AttackCommand);

struct MoveCommand : public EntityCommand{
	typedef EntityCommand st;
	static int construction_dummy;
	static EntityCommandID sid;
	virtual EntityCommandID id()const;
	virtual bool derived(EntityCommandID)const;
	MoveCommand(){}
	MoveCommand(HSQUIRRELVM v, Entity &e);
	Vec3d destpos;
};

DERIVE_COMMAND(ParadeCommand, EntityCommand);
DERIVE_COMMAND(DeltaCommand, EntityCommand);
DERIVE_COMMAND(DockCommand, EntityCommand);

DERIVE_COMMAND(SetAggressiveCommand, EntityCommand);
DERIVE_COMMAND(SetPassiveCommand, EntityCommand);

struct WarpCommand : public MoveCommand{
	typedef MoveCommand st;
	static int construction_dummy;
	static EntityCommandID sid;
	virtual EntityCommandID id()const;
	virtual bool derived(EntityCommandID)const;
	WarpCommand(){}
	WarpCommand(HSQUIRRELVM v, Entity &e);

	// There can be a discussion on whether warp destination is designated by a path or
	// a pointer. Finally, we choose a pointer because the entity commands are impulsive
	// by specification, which means they should be generated, passed and consumed in a
	// single frame, so we do not need to warry about pointed object's lifetime.
	// Also, it can be less efficient to convert between path string and pointer when
	// one that sent the command already know the destination system via pointer.
	CoordSys *destcs;
};

struct RemainDockedCommand : public EntityCommand{
	typedef EntityCommand st;
	static int construction_dummy;
	static EntityCommandID sid;
	virtual EntityCommandID id()const;
	virtual bool derived(EntityCommandID)const;
	RemainDockedCommand(bool a = true) : enable(a){}
	RemainDockedCommand(HSQUIRRELVM v, Entity &e);
	bool enable;
};

#endif
