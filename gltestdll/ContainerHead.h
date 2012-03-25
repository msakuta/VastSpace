#ifndef CONTAINERHEAD_H
#define CONTAINERHEAD_H

#include "Frigate.h"
#include "EntityCommand.h"


/// A class with similar concept of EntityController, but issues EntityCommands only.
struct EntityAI : Serializable{
	EntityAI(Game *game) : Serializable(game){}

	/// Called when this controller has to control an Entity.
	virtual bool control(Entity *, double dt) = 0;

	/// Called on destructor of the Entity being controlled.
	///
	/// Not all controller classes need to be destroyed, but some do. For example, the player could control
	/// entities at his own will, but not going to be deleted everytime it switches controlled object.
	/// On the other hand, Individual AI may be bound to and is destined to be destroyed with the entity.
	virtual bool unlink(Entity *);
};


class Island3;

/// \brief A civilian ship transporting containers.
///
/// This ship routinely transports goods such as fuel, air, meal, etc in the background to add
/// the game a bit of taste of space era.
///
/// The ship can have variable number of containers. Random number and types of containers are generated
/// for variation.
/// Participating containers are connected linearly, sandwitched by head and tail module.
/// The head module is merely a terminator, while the tail module has cockpit and thrust engines.
///
/// The base class is Frigate, which is usually for military ships.
class ContainerHead : public Frigate{
public:
	typedef Frigate st;
	static const int maxcontainers = 6;
	enum ContainerType{gascontainer, hexcontainer, Num_ContainerType};
	ContainerType containers[maxcontainers];
	int ncontainers; ///< Count of containers connected.
protected:
	EntityAI *ai;
	float undocktime;
	Entity *docksite;
	Entity *leavesite;
	struct tent3d_fpol *pf[3]; ///< Trailing smoke
	static const double sufscale;
public:
	ContainerHead(Game *game) : st(game){init();}
	ContainerHead(WarField *w);
	ContainerHead(Entity *docksite);
	~ContainerHead();
	void init();
	const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	static EntityRegister<ContainerHead> entityRegister;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	void dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &));
	virtual const char *dispname()const;
	virtual void anim(double);
	virtual void postframe();
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void enterField(WarField *);
	virtual void leaveField(WarField *);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual void drawOverlay(wardraw_t *);
	virtual double maxhealth()const;
	virtual Props props()const;
	virtual bool command(EntityCommand *);
	virtual bool dock(Docker*);
	virtual bool undock(Docker*);
	virtual void post_warp();
	static Entity *create(WarField *w, Builder *);
	btRigidBody *get_bbody(){return bbody;}
	enum sship_task_ch{sship_dockqueque = num_sship_task};
protected:
	bool buildBody();
};


struct TransportResourceCommand : EntityCommand{
	COMMAND_BASIC_MEMBERS(TransportResourceCommand, EntityCommand);
	TransportResourceCommand(){}
	TransportResourceCommand(HSQUIRRELVM v, Entity &e);
	TransportResourceCommand(int gases, int solids) : gases(gases), solids(solids){}
	int gases;
	int solids;
};


DERIVE_COMMAND_EXT_ADD(DockToCommand, EntityCommand, Entity *deste);


struct DockAI : EntityAI{
	enum Phase{Dockque2, Dockque, Dock, num_Phase} phase;
	Entity *docksite;
	DockAI(Game *game) : EntityAI(game){}
	DockAI(Entity *ch, Entity *docksite = NULL);
	const char *classname()const;
	void serialize(SerializeContext &sc);
	void unserialize(UnserializeContext &usc);
	bool control(Entity *ch, double dt);
	bool unlink(Entity *);
protected:
	static const unsigned classid;
};

struct TransportAI : EntityAI{
	enum Phase{Undock, Avoid, Warp, WarpWait, Dockque2, Dockque, Dock, num_Phase} phase;
	Entity *docksite;
	Entity *leavesite;
	DockAI dockAI;
	TransportAI(Game *game) : EntityAI(game), dockAI(game){}
	TransportAI(Entity *ch, Entity *leavesite);
	const char *classname()const;
	void serialize(SerializeContext &sc);
	void unserialize(UnserializeContext &usc);
	bool control(Entity *ch, double dt);
	bool unlink(Entity *);
protected:
	Vec3d dest(Entity *ch);
	void findIsland3(CoordSys *root, std::vector<Entity *> &ret)const;
	static const unsigned classid;
};


#endif
