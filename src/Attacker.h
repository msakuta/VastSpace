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
protected:
	AttackerDocker *docker;
	ArmBase **turrets;
	float engineHeat; ///< Integration of direction & PL_W

	static double modelScale;
	static double defaultMass;
	static ManeuverParams maneuverParams;
	static std::vector<hardpoint_static*> hardpoints;
	static std::vector<hitbox> hitboxes;
	static GLuint overlayDisp;
	static std::vector<Navlight> navlights;
public:
	typedef Warpable st;
	const char *classname()const;
	static const unsigned classid;
	static EntityRegister<Attacker> entityRegister;
	Attacker(Game *game);
	Attacker(WarField *);
	~Attacker();
	void static_init();
	void init();
	virtual void dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &));
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual void draw(wardraw_t *);
	virtual void drawtra(WarDraw *);
	virtual void drawOverlay(wardraw_t *);
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual bool command(EntityCommand *com);
	virtual double hitradius()const;
	virtual double maxhealth()const;
	virtual double maxenergy()const;
	virtual ArmBase *armsGet(int);
	virtual int armsCount()const;
	virtual const ManeuverParams &getManeuve()const;
	virtual Docker *getDockerInt();
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn);
	virtual int takedamage(double damage, int hitpart);
	virtual short bbodyGroup()const;
protected:
	bool buildBody();
};

/// A companion class of Attacker.
class AttackerDocker : public Docker{
public:
	int nextport;
	AttackerDocker(Game *game) : Docker(game){}
	AttackerDocker(Entity *ae = NULL) : st(ae), nextport(0){}
	typedef Docker st;
	static const unsigned classid;
	const char *classname()const;
	virtual bool undock(Dockable *);
	virtual Vec3d getPortPos(Dockable *)const;
	virtual Quatd getPortRot(Dockable *)const;
};

