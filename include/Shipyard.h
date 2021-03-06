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
#include "Model-forward.h"

class Shipyard;

class EXPORT ShipyardDocker : public Docker{
public:
	typedef Docker st;
	ShipyardDocker(Game *game) : st(game){}
	ShipyardDocker(Shipyard *ae = NULL);
	static const unsigned classid;
	const char *classname()const;
	virtual void dock(Dockable *);
	virtual void dockque(Dockable *);
	virtual bool undock(Dockable *);
	virtual Vec3d getPortPos(Dockable *)const;
	virtual Quatd getPortRot(Dockable *)const;
	Shipyard *getent();
};

class GetFaceInfoCommand;

class EXPORT Shipyard : public Warpable, public Builder{
public:
	typedef Warpable st; st *pst(){return static_cast<st*>(this);}

	Shipyard(Game *game) : st(game), docker(NULL), Builder(), clientDead(false){init();}
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
	virtual double getHealth()const;
	virtual double getMaxHealth()const;
	virtual double getHitRadius()const;
	virtual int takedamage(double damage, int hitpart);
	virtual double maxenergy()const;
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void anim(double dt);
	virtual void clientUpdate(double);
	virtual Props props()const;
	bool command(EntityCommand *)override;
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual void drawOverlay(wardraw_t *);
	virtual int popupMenu(PopupMenu &list);
	virtual int armsCount()const;
	virtual ArmBase *armsGet(int i);
	virtual Builder *getBuilderInt();
	virtual Docker *getDockerInt();
	virtual Entity *toEntity(){return this;}
	virtual const ManeuverParams &getManeuve()const;
	int tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn) override;

	bool startBuild();
	bool finishBuild();

	static bool modelHitPart(const Entity *e, Model *model, GetFaceInfoCommand &);
	static int modelTraceHit(const Entity *e, const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn, const Model *model);


protected:
	ArmBase **turrets;
	ShipyardDocker *docker;
//	Builder *builder;
	WeakPtr<Entity> undockingFrigate;
	WeakPtr<Entity> dockingFrigate;
	WeakPtr<Entity> buildingCapital; ///< A capital ship that is under construction.

	double doorphase[2];

	bool clientDead; ///< A flag indicating the death effects are performed in the client.

	EntityCommand *respondingCommand;

	static hardpoint_static *hardpoints;
	static int nhardpoints;

	static const ManeuverParams mymn;
	static double modelScale;
	static double hitRadius;
	static double defaultMass;
	static double maxHealthValue;
	static HitBoxList hitboxes;
	static std::vector<Navlight> navlights;

	static GLuint disp;

	static Model *getModel();
	static Motion *motions[2];

	bool cancelBuild(int index, bool recalc_time);
	virtual void doneBuild(Entity *);

	bool buildBody();
	short bbodyGroup()const;
	short bbodyMask()const;
	HitBoxList *getTraceHitBoxes()const;

	void dyingEffects(double dt);
	void deathEffects();

	friend class ShipyardDocker;
};

struct EXPORT SetBuildPhaseCommand : public EntityCommand{
	COMMAND_BASIC_MEMBERS(SetBuildPhaseCommand, EntityCommand);
	SetBuildPhaseCommand(double phase = 0.) : phase(phase){}
	SetBuildPhaseCommand(HSQUIRRELVM, Entity&){}
	double phase;
};

struct EXPORT GetFaceInfoCommand : public EntityCommand{
	COMMAND_BASIC_MEMBERS(GetFaceInfoCommand, EntityCommand);
	GetFaceInfoCommand(int hitpart = 0, Vec3d pos = Vec3d(0,0,0)) : hitpart(hitpart), pos(pos){}
	GetFaceInfoCommand(HSQUIRRELVM, Entity&){}
	Vec3d pos;
	Vec3d retNormal;
	Vec3d retPos;
	int hitpart;
	bool retPosHit; ///< Indicate whether input position projected onto the face plane is inside the face.
};



//-----------------------------------------------------------------------------
//    Inline Implementation
//-----------------------------------------------------------------------------

inline ShipyardDocker::ShipyardDocker(Shipyard *ae) : st(ae){}

inline Shipyard *ShipyardDocker::getent(){return static_cast<Shipyard*>(e);}

#endif
