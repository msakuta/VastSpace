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
	Entity *docksite;
	Entity *leavesite;
	struct tent3d_fpol *pf[3]; ///< Trailing smoke
	static const double sufscale;
public:
	SpacePlane(){init();}
	SpacePlane(WarField *w);
	SpacePlane(Entity *docksite);
	void init();
	const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid, entityid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
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
	virtual bool undock(Docker*);
	virtual void post_warp();
	static Entity *create(WarField *w, Builder *);
	btRigidBody *get_bbody(){return bbody;}
	enum sship_task_ch{sship_dockqueque = num_sship_task};
};


#endif
