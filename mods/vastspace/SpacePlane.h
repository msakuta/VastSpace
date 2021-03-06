#ifndef SPACEPLANE_H
#define SPACEPLANE_H

#include "Frigate.h"
#include "ContainerHead.h"
#include "EntityCommand.h"

class Island3;

/// \brief A civilian ship transporting passengers.
///
/// This ship routinely transports passengers in the background to add
/// the game a bit of taste of space era.
///
/// The ship can have variable number of containers. Random number and types of containers are generated
/// for variation.
/// Participating containers are connected linearly, sandwitched by head and tail module.
/// The head module is merely a terminator, while the tail module has cockpit and thrust engines.
///
/// The base class is Frigate, which is usually for military ships.
class SpacePlane : public Frigate{
public:
	typedef Frigate st;
protected:
	EntityAI *ai;
	float undocktime;
	int people;
	std::vector<Tefpol*> pf; ///< Trailing smoke
	float engineHeat;
	static std::vector<Vec3d> engines;
	static double modelScale;
	static double hitRadius;
	static double defaultMass;
	static double maxHealthValue;
	static GLuint overlayDisp;
public:
	SpacePlane(Game *game) : st(game){init();}
	SpacePlane(WarField *w);
	SpacePlane(Entity *docksite);
	~SpacePlane();
	void init();
	const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	static EntityRegister<SpacePlane> entityRegister;
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
	double getHitRadius()const override{return hitRadius;}
	virtual double getMaxHealth()const;
	virtual Props props()const;
	virtual bool command(EntityCommand *);
	virtual bool dock(Docker*);
	virtual bool undock(Docker*);
	static Entity *create(WarField *w, Builder *);
protected:
	bool buildBody();
};

struct TransportPeopleCommand : EntityCommand{
	COMMAND_BASIC_MEMBERS(TransportPeopleCommand, EntityCommand);
	TransportPeopleCommand(){}
	TransportPeopleCommand(HSQUIRRELVM v, Entity &e);
	TransportPeopleCommand(int people) : people(people){}
	int people;
};


#endif
