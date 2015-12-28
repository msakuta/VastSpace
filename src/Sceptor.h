/** \file
 * \brief Definition of Sceptor, Space interceptor class.
 */
#ifndef SCEPTOR_H
#define SCEPTOR_H

#include "Autonomous.h"
#include "tent3d_forward.h"

#define PIDAIM_PROFILE 0

/// Space Interceptor (small fighter)
class Sceptor : public Autonomous{
public:
	typedef Autonomous st;
protected:
	static double modelScale;
	static double defaultMass;
	static double maxHealthValue;
	static double maxFuelValue;
	static double maxAngleAccel;
	static HitBoxList hitboxes;
	static GLuint overlayDisp;
	static HSQOBJECT sqPopupMenu;
	static HSQOBJECT sqCockpitView;
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
		num_sceptor_task
	};
	enum Attitude{
		Passive,
		Aggressive
	};
	Vec3d aac; /* angular acceleration */
	double thrusts[3][2]; /* 3 pairs of thrusters, 2 directions each */
	double throttle;
	double horizontalThrust;
	double verticalThrust;
	double fuel;
	double cooldown;
	float fcloak;
	float heat;
	Tefpol *pf; ///< Trailing smoke
	WeakPtr<Docker> mother; ///< Mother ship that will be returned to when out of fuel
	int hitsound;
	int paradec;
	int magazine; ///< remaining bullet count in the magazine to shoot before reloading
	Task task;
	bool docked, returning, away, cloak, forcedEnemy, active;
	float reverser; ///< Thrust reverser position, approaches to 1 when throttle is negative.
	float muzzleFlash; ///< trivial muzzle flashes
	float integral[2]; ///< integration of pitch-yaw space of relative target position
	Sceptor *formPrev; ///< previous member in the formation
	Attitude attitude;
	int thrustSid; ///< Sound ID for thrusters
	int thrustHiSid; ///< Sound ID of high frequency component for thrusters

	static const avec3_t gunPos[2];
	static EnginePosList enginePos, enginePosRev;
	void init();
	void shootDualGun(double dt);
	bool findEnemy(); // Finds the nearest enemy
	void steerArrival(double dt, const Vec3d &target, const Vec3d &targetvelo, double speedfactor = 5., double minspeed = 0.);
	bool cull(Viewer &)const;
	double nlipsFactor(Viewer &)const;
	Entity *findMother();
public:
	Sceptor(Game *game);
	Sceptor(WarField *aw);
	~Sceptor();
	virtual const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	static EntityRegister<Sceptor> entityRegister;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual double getMaxHealth()const;
	virtual void enterField(WarField *);
	virtual void leaveField(WarField *);
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	void drawHUD(WarDraw *)override;
	virtual void drawOverlay(wardraw_t *);
	virtual bool solid(const Entity*)const;
	virtual int takedamage(double damage, int hitpart);
	virtual void postframe();
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;
	virtual Dockable *toDockable();
	virtual double getHitRadius()const;
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal);
	virtual Props props()const;
	virtual bool undock(Docker *);
	virtual bool command(EntityCommand *);
	virtual double maxfuel()const;

	static int cmd_dock(int argc, char *argv[], void *);
	static int cmd_parade_formation(int argc, char *argv[], void*);
	static double pid_ifactor;
	static double pid_pfactor;
	static double pid_dfactor;
	static SQInteger sqf_setControl(HSQUIRRELVM v);
protected:
	bool left, right, up, down, rollCW, rollCCW, throttleUp, throttleDown;
	bool flightAssist;
	double targetThrottle; ///< The real throttle tries to approach to this value
	bool buildBody();
	short bbodyMask()const;
	SQInteger sqGet(HSQUIRRELVM v, const SQChar *name)const override;
	SQInteger sqSet(HSQUIRRELVM v, const SQChar *name)override;
	HSQOBJECT getSqPopupMenu()override;
	HSQOBJECT getSqCockpitView()const override;
private:
	Vec3d evelo;
#if PIDAIM_PROFILE
	Vec3d epos;
	Vec3d iepos;
#endif
};


#endif
