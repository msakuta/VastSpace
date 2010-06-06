#ifndef ENTITYCOMMAND_H
#define ENTITYCOMMAND_H
#include <set>
#include <cpplib/vec3.h>

typedef const char *EntityCommandID;

#define DERIVE_COMMAND(name,base) struct name : public base{\
	typedef base st;\
	static EntityCommandID sid;\
	virtual EntityCommandID id()const;\
	virtual bool derived(EntityCommandID)const;\
}

#define IMPLEMENT_COMMAND(name,idname) const char *name::sid = idname;\
	EntityCommandID name::id()const{return sid;}\
	bool name::derived(EntityCommandID aid)const{if(aid==sid)return true;else return st::derived(aid);}

// Base class for all Entity commands.
struct EntityCommand{
	// The returned pointer never be dereferenced without debugging purposes,
	// it is just required to point the same address for all the instances but
	// never coincides between different classes.
	// A static const string of class name is ideal for this returned vale.
	virtual EntityCommandID id()const = 0;
	virtual bool derived(EntityCommandID)const;
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
	static EntityCommandID sid;
	virtual EntityCommandID id()const;
	virtual bool derived(EntityCommandID)const;
	std::set<Entity*> ents;
};

DERIVE_COMMAND(ForceAttackCommand, AttackCommand);

struct MoveCommand : public EntityCommand{
	typedef EntityCommand st;
	static EntityCommandID sid;
	virtual EntityCommandID id()const;
	virtual bool derived(EntityCommandID)const;
	Vec3d dest;
};

DERIVE_COMMAND(ParadeCommand, EntityCommand);
DERIVE_COMMAND(DeltaCommand, EntityCommand);
DERIVE_COMMAND(DockCommand, EntityCommand);

DERIVE_COMMAND(SetAggressiveCommand, EntityCommand);
DERIVE_COMMAND(SetPassiveCommand, EntityCommand);

struct WarpCommand : public MoveCommand{
	typedef MoveCommand st;
	static EntityCommandID sid;
	virtual EntityCommandID id()const;
	virtual bool derived(EntityCommandID)const;
	const char *destname;
};

struct RemainDockedCommand : public EntityCommand{
	typedef EntityCommand st;
	static EntityCommandID sid;
	virtual EntityCommandID id()const;
	virtual bool derived(EntityCommandID)const;
	RemainDockedCommand(bool a) : enable(a){}
	bool enable;
};

#endif
