#ifndef SCARRY_H
#define SCARRY_H
#include "Warpable.h"
#include "Builder.h"
#include "arms.h"
#include "glw/glwindow.h"
#include "Docker.h"

#define SCARRY_BUILDQUESIZE 8
#define SCARRY_SCALE .0010
#define SCARRY_BAYCOOL 2.

class ScarryDocker : public Docker{
public:
	ScarryDocker(Entity *ae = NULL) : st(ae){}
	typedef Docker st;
	static const unsigned classid;
	const char *classname()const;
	virtual bool undock(Dockable *);
	virtual Vec3d getPortPos()const;
	virtual Quatd getPortRot()const;
};

int cmd_build(int argc, char *argv[], void *pv);

#if 0
class Scarry : public Warpable, public Builder{
public:
	typedef Warpable st; st *pst(){return static_cast<st*>(this);}

	Scarry() : docker(new ScarryDocker(this)){init();}
	Scarry(WarField *w);
	~Scarry();
	void init();
	virtual const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	static EntityRegister<Scarry> entityRegister;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &));
	virtual const char *dispname()const;
	virtual double getMaxHealth()const;
	virtual double getHitRadius()const;
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
	double ru;

	ArmBase **turrets;
	ScarryDocker *docker;

	static hardpoint_static *hardpoints;
	static int nhardpoints;

	static const maneuve mymn;
	static hitbox hitboxes[];
	static const int nhitboxes;

	virtual void doneBuild(Entity *);
};
#endif

#if 0
/* Active Radar or Hyperspace Sonar.
   AR costs less energy but has limitation in speed (light), while
  HS propagates on hyperspace field where limitation of speed do not exist.
*/
struct hypersonar{
	avec3_t pos;
	double life;
	double rad;
	double speed;
	struct coordsys *cs;
	struct hypersonar *next;
	int type; /* 0 - Active Radar, 1 - Hyperspace Sonar. */
} *g_hsonar;
#endif


#endif
