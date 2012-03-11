#ifndef SHIPYARD_H
#define SHIPYARD_H
#include "Builder.h"
#include "Warpable.h"
#include "arms.h"
#include "glw/glwindow.h"
#include "Docker.h"


class EXPORT ShipyardDocker : public Docker{
public:
	ShipyardDocker(Entity *ae = NULL) : st(ae){}
	typedef Docker st;
	static const unsigned classid;
	const char *classname()const;
	virtual bool undock(Dockable *);
	virtual Vec3d getPortPos()const;
	virtual Quatd getPortRot()const;
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
	virtual void anim(double dt);
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

	static hardpoint_static *hardpoints;
	static int nhardpoints;

	static const maneuve mymn;
	static hitbox hitboxes[];
	static const int nhitboxes;

	virtual void doneBuild(Entity *);
};

#endif
