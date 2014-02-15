/* \file
 * \brief Definition of Mobile Suit abstract class.
 */
#ifndef GUNDAM_MOBILESUIT_H
#define GUNDAM_MOBILESUIT_H

#include "StaticBind.h"
#include "Frigate.h"
#include "ModelEntity.h"
#include "mqo.h"
#include "ysdnmmot.h"
#include "libmotion.h"
#include "msg/GetCoverPointsMessage.h"


struct btCompoundShape;

/// \brief The base class for Mobile Suits
class MobileSuit : public ModelEntity{
public:
	typedef ModelEntity st;
protected:
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
		Jump, ///< Jump over some obstacles
		JumpForward, ///< Move forward after jumping over obstacles.
		Rest, ///< Rest a while to wait energy to regain.
		CoverRight,
		num_sceptor_task
	};
	enum Attitude{
		Passive,
		Aggressive
	};
	static const Vec3d thrusterDir[7];

	/// \brief The parameters for a weapon type.
	struct WeaponParams{
		const char *name;
		float coolDownTime;
		float reloadTime;
		int magazineSize;
		int maxAmmo;
	};

	/// \brief Status for a weapon type.
	///
	/// Each MobileSuit instance have a set of this struct to keep track of
	/// weapon status.
	struct WeaponStatus{
		float cooldown;
		float reload;
		int magazine;
		int ammo;
	};

	std::vector<WeaponStatus> weaponStatus;

	Vec3d aac; /* angular acceleration */
	double thrusts[3][2]; /* 3 pairs of thrusters, 2 directions each */
	double throttle;
	double fuel;
	Vec3d dest;
	float vulcanSoundCooldown;
	float fcloak;
	float heat;
	float fwaverider; ///< Phase of transformation to wave rider
	float fweapon; ///< Phase of weapon switch
	struct Tefpol *pf; ///< Trailing smoke
	Docker *mother; ///< Mother ship that will be returned to when out of fuel
	int hitsound;
	int paradec;
	int weapon; ///< Armed weapon id
	Task task;
	bool standingUp; ///< Standing up against acceleration
	bool docked, returning, away, cloak, forcedEnemy;
	bool waverider;
	bool stabilizer;
	float reverser; ///< Thrust reverser position, approaches to 1 when throttle is negative.
	float muzzleFlash[3]; ///< Muzzle flashes for each arms.
	float twist; ///< Twist value (integration of angular velocity around Y axis)
	float pitch; ///< Pitch value (integration of angular velocity around X axis)
	float fsabre; ///< Sabre swing phase
	float integral[2]; ///< integration of pitch-yaw space of relative target position
	float fonfeet; ///< Factor of on-feetness
	float walkphase; ///< Walk phase
	float jumptime; ///< Time left for jump
	float coverRight; ///< Factor
	double aimdir[2]; ///< Pitch and yaw. They're different from twist and pitch in that they represent manipulator
	MobileSuit *formPrev; ///< previous member in the formation
	Attitude attitude;
	Vec3d thrusterDirs[numof(thrusterDir)]; ///< Direction vector of the thrusters
	double thrusterPower[numof(thrusterDir)]; ///< Output power of the thrusters
	CoverPoint coverPoint; ///< Cover point

	static double defaultMass;
	static double maxHealthValue;
	static HSQOBJECT sqPopupMenu;
	static HSQOBJECT sqCockpitView;
	static const avec3_t gunPos[2];
	virtual void shootRifle(double dt) = 0;
	virtual void shootShieldBeam(double dt) = 0;
	virtual void shootVulcan(double dt) = 0;
	bool findEnemy(); // Finds the nearest enemy
	void steerArrival(double dt, const Vec3d &target, const Vec3d &targetvelo, double speedfactor = 5., double minspeed = 0.);
	bool cull(Viewer &)const;
	double nlipsFactor(Viewer &)const;
	Entity *findMother();
	Quatd aimRot()const;
	double coverFactor()const{return min(coverRight, 1.);}
	static SQInteger sqf_get(HSQUIRRELVM);

	static StaticBindDouble deathSmokeFreq;
	static StaticBindDouble bulletSpeed;
	static StaticBindDouble walkSpeed;
	static StaticBindDouble airMoveSpeed;
	static StaticBindDouble torqueAmount;
	static StaticBindDouble floorProximityDistance;
	static StaticBindDouble floorTouchDistance;
	static StaticBindDouble standUpTorque;
	static StaticBindDouble standUpFeedbackTorque;
	static StaticBindDouble randomVibration;

public:
	MobileSuit(Game *game);
	MobileSuit(WarField *aw);
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual double getMaxHealth()const override;
	virtual void control(const input_t *inputs, double dt);
	virtual void anim(double dt);
	void drawHUD(WarDraw *wd)override;
	virtual bool solid(const Entity*)const;
	virtual int takedamage(double damage, int hitpart);
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;
	virtual Dockable *toDockable();
	virtual double getHitRadius()const;
	virtual Props props()const;
	virtual bool undock(Docker *);
	virtual bool command(EntityCommand *);

	static gltestp::dstring modPath(){return "gundam/";}

	static int cmd_dock(int argc, char *argv[], void *);
	static int cmd_parade_formation(int argc, char *argv[], void*);
	static void smokedraw(const struct tent3d_line_callback *p, const struct tent3d_line_drawdata *dd, void *private_data);
	static double pid_ifactor;
	static double pid_pfactor;
	static double pid_dfactor;
	static double motionInterpolateTime;
	static double motionInterpolateTimeAverage;
	static int motionInterpolateTimeAverageCount;
	static HSQUIRRELVM sqvm;

	void init();
protected:
	virtual double maxfuel()const = 0;
	virtual double getFuelRegenRate()const = 0;
	virtual int getWeaponCount()const{return 0;}
	virtual bool getWeaponParams(int weapon, WeaponParams &param)const{return false;}
	virtual double getRotationSpeed()const = 0;
	virtual double getMaxAngleSpeed()const = 0;
	virtual const HitBoxList &getHitBoxes()const = 0;
	virtual btCompoundShape *getShape()const = 0;
	virtual btCompoundShape *getWaveRiderShape()const{return NULL;}
	virtual Model *getModel()const = 0;
	virtual double getModelScale()const = 0;
	Vec3d getDelta()const;
	virtual void AIUpdate(double dt, bool floorTouched);
	SQInteger sqGet(HSQUIRRELVM v, const SQChar *name)const override;
	SQInteger sqSet(HSQUIRRELVM v, const SQChar *name)override;
	HSQOBJECT getSqPopupMenu()override{ return sqPopupMenu; }
	HSQOBJECT getSqCockpitView()const override{ return sqCockpitView; }

	SQInteger boolSetter(HSQUIRRELVM v, const SQChar *name, bool &value);

	void reloadWeapon();

	void drawBar(double f, double left, double top, double width, double height)const; ///< Draw a percentage bar
	bool isDrawHealthBar()const; ///< Whether the health bar should be drawn as overlay

private:
	mutable Vec3d pDelta; ///< Private delta
	mutable double pDeltaTime; ///< Global time when pDelta was updated
	Vec3d evelo;
#if PIDAIM_PROFILE
	Vec3d epos;
	Vec3d iepos;
#endif
};

#endif
