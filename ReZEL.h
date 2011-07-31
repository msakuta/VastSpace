#ifndef GUNDAM_REZEL_H
#define GUNDAM_REZEL_H

#include "Frigate.h"
#include "EntityCommand.h"
#include "mqo.h"
#include "ysdnmmot.h"
#include "libmotion.h"

/// The base class for Squirrel-bound static variables.
///
/// Derived classes implement abstruct methods to interact with Squirrel VM.
///
/// This method's overhead compared to plain variable is just a virtual function
/// table pointer for each instance.
class StaticBind{
public:
	virtual void push(HSQUIRRELVM v) = 0;
	virtual bool set(HSQUIRRELVM v) = 0;
};

/// Integer binding.
class StaticBindInt : public StaticBind{
	int value;
	virtual void push(HSQUIRRELVM v){
		sq_pushinteger(v, value);
	}
	virtual bool set(HSQUIRRELVM v){
		SQInteger sqi;
		bool ret = SQ_SUCCEEDED(sq_getinteger(v, -1, &sqi));
		if(ret)
			value = sqi;
		return ret;
	}
public:
	StaticBindInt(int a) : value(a){}
	operator int&(){return value;}
};

/// Double binding.
class StaticBindDouble : public StaticBind{
	double value;
	virtual void push(HSQUIRRELVM v){
		sq_pushfloat(v, value);
	}
	virtual bool set(HSQUIRRELVM v){
		SQFloat sqi;
		bool ret = SQ_SUCCEEDED(sq_getfloat(v, -1, &sqi));
		if(ret)
			value = sqi;
		return ret;
	}
public:
	StaticBindDouble(double a) : value(a){}
	operator double&(){return value;}
};


struct btCompoundShape;

/// ReZEL
class ReZEL : public Entity{
public:
	typedef Entity st;
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
	float fcloak;
	float heat;
	float fwaverider; ///< Phase of transformation to wave rider
	float fweapon; ///< Phase of weapon switch
	struct tent3d_fpol *pf; ///< Trailing smoke
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
	double aimdir[2]; ///< Pitch and yaw. They're different from twist and pitch in that they represent manipulator
	ReZEL *formPrev; ///< previous member in the formation
	Attitude attitude;
	Vec3d thrusterDirs[numof(thrusterDir)]; ///< Direction vector of the thrusters
	double thrusterPower[numof(thrusterDir)]; ///< Output power of the thrusters

	static const int motionCount = 13;

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
	

	static const double sufscale;
	static const avec3_t gunPos[2];
	static Model *model;
	static Motion *motions[motionCount];
	static btCompoundShape *shape;
	static btCompoundShape *waveRiderShape;
	void getMotionTime(double (*motion_time)[numof(motions)], double (*motion_amplitude)[numof(motions)] = NULL);
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
	static SQInteger sqf_get(HSQUIRRELVM);
	friend class EntityRegister<ReZEL>;
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
public:
	ReZEL();
	ReZEL(WarField *aw);
	virtual const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	static EntityRegister<ReZEL> entityRegister;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual double maxhealth()const;
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
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
	virtual double hitradius()const;
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal);
	virtual int popupMenu(PopupMenu &);
	virtual Props props()const;
	virtual bool undock(Docker *);
	virtual bool command(EntityCommand *);
	virtual double maxfuel()const;
	static hitbox hitboxes[];
	static const int nhitboxes;
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
private:
	Vec3d evelo;
#if PIDAIM_PROFILE
	Vec3d epos;
	Vec3d iepos;
#endif
};


struct TransformCommand : public EntityCommand{
	typedef EntityCommand st;
	static int construction_dummy;
	static EntityCommandID sid;
	virtual EntityCommandID id()const;
	virtual bool derived(EntityCommandID)const;
	TransformCommand(){}
	TransformCommand(HSQUIRRELVM v, Entity &e);
	int formid;
};

struct WeaponCommand : public EntityCommand{
	typedef EntityCommand st;
	static int construction_dummy;
	static EntityCommandID sid;
	virtual EntityCommandID id()const;
	virtual bool derived(EntityCommandID)const;
	WeaponCommand(){}
	WeaponCommand(HSQUIRRELVM v, Entity &e);
	int weaponid;
};

struct StabilizerCommand : public EntityCommand{
	typedef EntityCommand st;
	static int construction_dummy;
	static EntityCommandID sid;
	virtual EntityCommandID id()const;
	virtual bool derived(EntityCommandID)const;
	StabilizerCommand(){}
	StabilizerCommand(HSQUIRRELVM v, Entity &e);
	int stabilizer;
};

#endif
