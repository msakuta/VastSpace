/* \file
 * \brief Definition of Zeta Gundam class.
 */
#ifndef GUNDAM_ZETA_H
#define GUNDAM_ZETA_H

#include "StaticBind.h"
#include "Frigate.h"
#include "ModelEntity.h"
#include "mqo.h"
#include "ysdnmmot.h"
#include "libmotion.h"
#include "msg/GetCoverPointsMessage.h"


struct btCompoundShape;

/// ReZEL
class ZetaGundam : public ModelEntity{
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

	Vec3d aac; /* angular acceleration */
	double thrusts[3][2]; /* 3 pairs of thrusters, 2 directions each */
	double throttle;
	double fuel;
	double cooldown;
	Vec3d dest;
	float vulcancooldown;
	float vulcanSoundCooldown;
	float fcloak;
	float heat;
	float fwaverider; ///< Phase of transformation to wave rider
	float fweapon; ///< Phase of weapon switch
	struct Tefpol *pf; ///< Trailing smoke
	Docker *mother; ///< Mother ship that will be returned to when out of fuel
	int hitsound;
	int paradec;
	int magazine; ///< remaining bullet count in the magazine to shoot before reloading
	int weapon; ///< Armed weapon id
	int submagazine; ///< Remaining rounds in sub-arm
	int vulcanmag; ///< Vulcan magazine
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
	float freload; ///< Reload phase
	float fonfeet; ///< Factor of on-feetness
	float walkphase; ///< Walk phase
	float jumptime; ///< Time left for jump
	float coverRight; ///< Factor
	double aimdir[2]; ///< Pitch and yaw. They're different from twist and pitch in that they represent manipulator
	ZetaGundam *formPrev; ///< previous member in the formation
	Attitude attitude;
	Vec3d thrusterDirs[numof(thrusterDir)]; ///< Direction vector of the thrusters
	double thrusterPower[numof(thrusterDir)]; ///< Output power of the thrusters
	CoverPoint coverPoint; ///< Cover point

	static const int motionCount = 1;

	/// Set of MotionPoses produced by interpolating motions.
	///
	/// std::vector would do the same thing, but we know maximum count of MotionPoses,
	/// so we can avoid heap memory allocations.
	class MotionPoseSet{
		MotionPose *a[motionCount];
		int n;
	public:
		MotionPoseSet() : n(0){}
		~MotionPoseSet(){
		}
		MotionPose &operator[](int i){return *a[i];}
		void push_back(MotionPose *p){
			a[n++] = p;
		}
		int getn()const{return n;}
		void clear(){
			for(int i = 0; i < n; i++)
				delete a[i];
			n = 0;
		}
	};

	MotionPoseSet poseSet; ///< Last motion pose set
	double motion_time[motionCount];
	double motion_amplitude[motionCount];

	static double modelScale;
	static double defaultMass;
	static double maxHealthValue;
	static HSQOBJECT sqPopupMenu;
	static HSQOBJECT sqCockpitView;
	static const avec3_t gunPos[2];
	static Model *model;
	static Motion *motions[motionCount];
	static btCompoundShape *shape;
	static btCompoundShape *waveRiderShape;
	void getMotionTime(double (&motion_time)[numof(motions)], double (&motion_amplitude)[numof(motions)]);
	MotionPoseSet &motionInterpolate();
	void motionInterpolateFree(MotionPoseSet &set);
	void shootRifle(double dt);
	void shootShieldBeam(double dt);
	void shootVulcan(double dt);
	void shootMegaBeam(double dt);
	bool findEnemy(); // Finds the nearest enemy
	void steerArrival(double dt, const Vec3d &target, const Vec3d &targetvelo, double speedfactor = 5., double minspeed = 0.);
	bool cull(Viewer &)const;
	double nlipsFactor(Viewer &)const;
	Entity *findMother();
	Quatd aimRot()const;
	double coverFactor()const{return min(coverRight, 1.);}
	void reloadRifle();
	static SQInteger sqf_get(HSQUIRRELVM);
	friend class EntityRegister<ZetaGundam>;
	static StaticBindDouble rotationSpeed;
	static StaticBindDouble maxAngleSpeed;
	static StaticBindDouble deathSmokeFreq;
	static StaticBindDouble bulletSpeed;
	static StaticBindDouble walkSpeed;
	static StaticBindDouble airMoveSpeed;
	static StaticBindDouble torqueAmount;
	static StaticBindDouble floorProximityDistance;
	static StaticBindDouble floorTouchDistance;
	static StaticBindDouble standUpTorque;
	static StaticBindDouble standUpFeedbackTorque;
	static StaticBindDouble maxFuel;
	static StaticBindDouble fuelRegenRate;
	static StaticBindDouble randomVibration;
	static StaticBindDouble cooldownTime;
	static StaticBindDouble reloadTime;
	static StaticBindDouble rifleDamage;
	static StaticBindInt rifleMagazineSize;
	static StaticBindDouble shieldBeamCooldownTime;
	static StaticBindDouble shieldBeamReloadTime;
	static StaticBindDouble shieldBeamDamage;
	static StaticBindInt shieldBeamMagazineSize;
	static StaticBindDouble vulcanCooldownTime;
	static StaticBindDouble vulcanReloadTime;
	static StaticBindDouble vulcanDamage;
	static StaticBindInt vulcanMagazineSize;

	void init();
public:
	ZetaGundam(Game *game);
	ZetaGundam(WarField *aw);
	virtual const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	static EntityRegister<ZetaGundam> entityRegister;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual double getMaxHealth()const override;
	virtual void enterField(WarField *);
	virtual void control(const input_t *inputs, double dt);
	virtual void anim(double dt);
	virtual void draw(WarDraw *);
	virtual void drawtra(WarDraw *);
	virtual void drawHUD(WarDraw *);
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

	static gltestp::dstring modPath(){return "gundam/";}

	static HitBoxList hitboxes;
	static GLuint overlayDisp;
	static hitbox waveRiderHitboxes[];
	static const int nWaveRiderHitboxes;
//	static Entity *create(WarField *w, Builder *mother);
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

protected:
	SQInteger sqGet(HSQUIRRELVM v, const SQChar *name)const override;
	SQInteger sqSet(HSQUIRRELVM v, const SQChar *name)override;
	HSQOBJECT getSqPopupMenu()override{ return sqPopupMenu; }
	HSQOBJECT getSqCockpitView()const override{ return sqCockpitView; }

	SQInteger boolSetter(HSQUIRRELVM v, const SQChar *name, bool &value);

private:
	Vec3d evelo;
#if PIDAIM_PROFILE
	Vec3d epos;
	Vec3d iepos;
#endif
};

#endif
