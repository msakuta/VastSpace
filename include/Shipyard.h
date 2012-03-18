#ifndef SHIPYARD_H
#define SHIPYARD_H
/** \file
 * \brief Definition of Shipyard class.
 */
#include "Builder.h"
#include "Warpable.h"
#include "arms.h"
#include "glw/glwindow.h"
#include "Docker.h"
#include "libmotion.h"

class Shipyard;

class EXPORT ShipyardDocker : public Docker{
public:
	ShipyardDocker(Shipyard *ae = NULL);
	typedef Docker st;
	static const unsigned classid;
	const char *classname()const;
	virtual void dock(Dockable *);
	virtual void dockque(Dockable *);
	virtual bool undock(Dockable *);
	virtual Vec3d getPortPos(Dockable *)const;
	virtual Quatd getPortRot(Dockable *)const;
	Shipyard *getent();
};

class EXPORT Shipyard : public Warpable, public Builder{
public:
	typedef Warpable st; st *pst(){return static_cast<st*>(this);}

	Shipyard() : docker(new ShipyardDocker(this)), Builder(this->Entity::w){init();}
	Shipyard(WarField *w);
	~Shipyard();
	void init();
	virtual const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	static EntityRegister<Shipyard> entityRegister;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &));
	virtual const char *dispname()const;
	virtual double maxhealth()const;
	virtual double hitradius()const;
	virtual double maxenergy()const;
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void enterField(WarField *target);
	virtual void leaveField(WarField *w);
	virtual void anim(double dt);
	virtual void clientUpdate(double);
	virtual Props props()const;
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual int popupMenu(PopupMenu &list);
	virtual int tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn);
	virtual int armsCount()const;
	virtual ArmBase *armsGet(int i);
	virtual double getRU()const;
	virtual Builder *getBuilderInt();
	virtual Docker *getDockerInt();
	virtual const maneuve &getManeuve()const;

protected:
	ArmBase **turrets;
	ShipyardDocker *docker;
//	Builder *builder;
	WeakPtr<Entity> undockingFrigate;
	WeakPtr<Entity> dockingFrigate;

	double doorphase[2];

	static hardpoint_static *hardpoints;
	static int nhardpoints;

	static const maneuve mymn;
	static hitbox hitboxes[];
	static const int nhitboxes;

	static Motion *motions[2];

	virtual void doneBuild(Entity *);

	bool buildBody();

	friend class ShipyardDocker;
};


//-----------------------------------------------------------------------------
//    Inline Implementation
//-----------------------------------------------------------------------------

inline ShipyardDocker::ShipyardDocker(Shipyard *ae) : st(ae){}

inline Shipyard *ShipyardDocker::getent(){return static_cast<Shipyard*>(e);}

#endif
