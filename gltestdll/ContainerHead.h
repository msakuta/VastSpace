#ifndef CONTAINERHEAD_H
#define CONTAINERHEAD_H

#include "Frigate.h"

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
protected:
	static const int maxcontainers = 6;
	enum ContainerType{gascontainer, hexcontainer, Num_ContainerType};
	ContainerType containers[maxcontainers];
	int ncontainers; ///< Count of containers connected.
	float undocktime;
	CoordSys *docksite;
	static const double sufscale;
public:
	ContainerHead(){init();}
	ContainerHead(WarField *w);
	ContainerHead(CoordSys *docksite);
	void init();
	const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid, entityid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual void anim(double);
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual double maxhealth()const;
	virtual Props props()const;
	virtual bool command(EntityCommand *);
	virtual bool undock(Docker*);
	virtual void post_warp();
	static Entity *create(WarField *w, Builder *);
	btRigidBody *get_bbody(){return bbody;}
protected:
	void findIsland3(CoordSys *root, std::vector<CoordSys *> &ret)const;
};

#endif
