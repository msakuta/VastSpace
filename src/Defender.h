/** \file
 * \brief Definition of Defender class.
 *
 * Also declares DeployCommand, acceptable by Defender.
 */
#ifndef DEFENDER_H
#define DEFENDER_H

#include "Autonomous.h"
#include "EntityCommand.h"
#include "CoordSys.h"
#include "war.h"
#include "arms.h"
#include "shield.h"
#include "tefpol3d.h"
//#include "mqo.h"
extern "C"{
#include <clib/avec3.h>
//#include <clib/suf/sufdraw.h>
}

#define PIDAIM_PROFILE 0

class Model;
struct Motion;
struct MotionPose;

/// \brief Space Defender (middle fighter)
///
/// A space fighter that sit tight and guard settlements.
class Defender : public Autonomous{
public:
	typedef Autonomous st;
protected:
	static double modelScale;
	static double defaultMass;
	static HitBoxList hitboxes;
	static GLuint overlayDisp;
	static Vec3d gunPos;
	static ManeuverParams maneuverParams;

	enum Task{
		Idle = sship_idle,
		Undock = sship_undock,
		Undockque = sship_undockque,
		Dock = sship_dock,
		Dockque = sship_dockque,
		Moveto = sship_moveto,
		Parade = sship_parade,
		DeltaFormation = sship_delta,
		Attack = sship_attack,
		Away = sship_away,
		Auto, // Automatically targets near enemy or parade with mother.
		Deploy,
		Dodge0, Dodge1, Dodge2, Dodge3,
		NumTask
	};
/*	enum Attitude{
		Passive,
		Aggressive
	};*/
	Vec3d aac; /* angular acceleration */
	double thrusts[3][2]; /* 3 pairs of thrusters, 2 directions each */
	double throttle;
	double fuel;
	double cooldown;
	float fcloak;
	float heat;
	Tefpol *pf[4]; ///< Trailing smoke
	Docker *mother; ///< Mother ship that will be returned to when out of fuel
	int hitsound;
	int paradec;
	Task task;
	bool docked, returning, away, cloak, forcedEnemy;
	float reverser; ///< Thrust reverser position, approaches to 1 when throttle is negative.
	float mf; ///< trivial muzzle flashes
	float integral[2]; ///< integration of pitch-yaw space of relative target position
	float fdeploy; ///< Factor of deployment
	float fdodge; ///< Factor where dodging movement
	float frotate; ///< Phase of barrel rotation, 1. == quarter of a round
	static const float rotateTime; ///< Time taken to rotate the barrel.
//	Sceptor *formPrev; ///< previous member in the formation
//	Attitude attitude;
	static Model *model;
	static Motion *motions[2];
	static gltestp::dstring engineNames[4];

	void init();
	void shoot(double dt);
	bool findEnemy(); // Finds the nearest enemy
	void steerArrival(double dt, const Vec3d &target, const Vec3d &targetvelo, double speedfactor = 5., double minspeed = 0.);
	bool cull(Viewer &)const;
	double nlipsFactor(Viewer &)const;
	Entity *findMother();
	void headTowardEnemy(double dt, const Vec3d &enemyPosition);
	static double reloadTime(){return 5.;} ///< Time between shoots
	static double bulletSpeed(){return 5000.;} ///< Velocity of projectile shot
	bool isDeployed()const{return task == Deploy || Dodge0 <= task && task <= Dodge3;}
public:
	Defender(Game *game);
	Defender(WarField *aw);
	~Defender();
	virtual const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	static EntityRegister<Defender> entityRegister;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual double getMaxHealth()const;
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void enterField(WarField *);
	virtual void leaveField(WarField *);
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual void drawOverlay(wardraw_t *);
	virtual bool solid(const Entity*)const;
	virtual int takedamage(double damage, int hitpart);
	virtual void postframe();
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;
	virtual Dockable *toDockable(){return this;}
	virtual double getHitRadius()const;
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal);
	virtual int popupMenu(PopupMenu &);
	virtual Props props()const;
	virtual bool undock(Docker *);
	virtual bool command(EntityCommand *);
	virtual double maxfuel()const;
	virtual bool buildBody();
	virtual short bbodyMask()const;
	virtual const ManeuverParams &getManeuve()const;
	Mat4d legTransform(int legIndex)const; ///< Returns transformation matrix for a given leg.
	MotionPose *getPose(MotionPose (&mpbuf)[2])const; ///< Returns the current MotionPoses.
private:
};

/// \brief Command that tells a Defender to transform itself to deployed state.
///
/// Deployed Defender has better performance over combat, but cannot move forward nor desired directions.
DERIVE_COMMAND(DeployCommand, SerializableCommand);

/// \brief Command that tells a Defender to quit deploying.
/// \sa DeployCommand
DERIVE_COMMAND(UndeployCommand, SerializableCommand);


#endif
