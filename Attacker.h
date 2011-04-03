/** \file
 * \brief Defines Attacker and AttackerDocker.
 */
#include "war.h"
#include "Warpable.h"
#include "Docker.h"
#include "arms.h"

class AttackerDocker;

/// A heavy assault ship with docking bay.
class Attacker : public Warpable{
	AttackerDocker *docker;
	ArmBase **turrets;
	bool justLoaded; ///< A flag indicates this object is just loaded from a save file.
	static hardpoint_static *hardpoints;
	static int nhardpoints;
public:
	typedef Warpable st;
	static hitbox hitboxes[];
	static const unsigned nhitboxes;
	const char *classname()const;
	static const unsigned classid, entityid;
	Attacker();
	Attacker(WarField *);
	~Attacker();
	virtual void dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &));
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	void static_init();
	virtual void anim(double dt);
	virtual void draw(wardraw_t *);
	virtual void drawOverlay(wardraw_t *);
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual bool command(EntityCommand *com);
	virtual double hitradius()const;
	virtual double maxhealth()const;
	virtual double maxenergy()const;
	virtual ArmBase *armsGet(int);
	virtual int armsCount()const;
	virtual const maneuve &getManeuve()const;
	virtual Docker *getDockerInt();
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn);
	virtual int takedamage(double damage, int hitpart);
	virtual void enterField(WarField *);

	static void onBeginTexture(void*);
	static void onEndTexture(void*);
protected:
	void buildBody();
};

/// A companion class of Attacker.
class AttackerDocker : public Docker{
public:
	int nextport;
	AttackerDocker(Entity *ae = NULL) : st(ae), nextport(0){}
	typedef Docker st;
	static const unsigned classid;
	const char *classname()const;
	virtual bool undock(Dockable *);
	virtual Vec3d getPortPos()const;
	virtual Quatd getPortRot()const;
};

