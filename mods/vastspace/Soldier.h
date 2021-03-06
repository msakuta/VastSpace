/** \file
 * \brief Definition of Soldier class.
 */
#ifndef SOLDIER_H
#define SOLDIER_H
#include "Autonomous.h"
#include "war.h"
#include "arms.h"
#include "judge.h"
#include "EntityCommand.h"
#include "EntityRegister.h"
#include "SqInitProcess-ex.h"
#include "draw/OpenGLState.h"
extern "C"{
#include <clib/mathdef.h>
}

struct Model;
class Motion;
struct MotionPose;

struct GetGunPosCommand;

/// \biref Parameters for firearms
struct FirearmStatic{
	int maxAmmoValue;
	double shootCooldownValue;
	double bulletSpeedValue;
	double bulletDamageValue;
	double bulletVarianceValue;
	double aimFovValue;
	double shootRecoilValue;
	HSQOBJECT sqShoot;
	gltestp::dstring modelName;
	gltestp::dstring overlayIconFile;

	// Following variables are not configurable from Squirrel source,
	// just automatically set by the program.
	Model *model;
	OpenGLState::weak_ptr<bool> modelInit;
	GLuint overlayIcon;

	FirearmStatic();
};

/// \brief The abstract class for personal firearms.
class Firearm : public ArmBase{
public:
	typedef ArmBase st;

	Firearm(Game *game) : st(game){}
	Firearm(Entity *abase, const hardpoint_static *hp) : st(abase, hp){}
	void shoot(double dt);
	void reload();
	virtual int maxammo()const = 0;
	virtual double shootCooldown()const = 0;
	virtual double bulletSpeed()const = 0;
	virtual double bulletDamage()const = 0;
	virtual double bulletLifeTime()const{return 5.;}
	virtual double bulletVariance()const{return 0.01;}
	virtual double aimFov()const{return 0.7;} ///< Magnitude of zoom when aiming expressed in FOV.
	virtual double shootRecoil()const{return M_PI / 64.;}
	virtual HSQOBJECT getSqShoot()const{return sq_nullobj();} ///< Returns shoot event handler for this firearm
	virtual FirearmStatic *getFirearmStatic()const{return NULL;}

protected:
	SQInteger sqGet(HSQUIRRELVM v, const SQChar *name)const override;
	SQInteger sqSet(HSQUIRRELVM v, const SQChar *name)override;
};


/// \breif The infantryman with firearms and a spacesuit equipped.
class Soldier : public Autonomous{
public:
	typedef Autonomous st;

	static EntityRegister<Soldier> entityRegister;
	static const unsigned classid;
	virtual const char *classname()const;

	Soldier(Game *);
	Soldier(WarField *);
	~Soldier();
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void setPosition(const Vec3d *, const Quatd *, const Vec3d *, const Vec3d *);
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual void control(const input_t *inputs, double dt);
	virtual bool buildBody();
	virtual void draw(WarDraw *);
	virtual void drawtra(WarDraw *);
	virtual void drawHUD(WarDraw *);
	virtual void drawOverlay(WarDraw *);
	virtual double getHitRadius()const;
	virtual double getMaxHealth()const;
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;
	virtual void onBulletHit(const Bullet *, int hitpart);
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal); // return nonzero on hit
	virtual int takedamage(double damage, int hitpart);
	virtual Props props()const;
	virtual int armsCount()const;
	virtual ArmBase *armsGet(int);
	virtual bool command(EntityCommand *com);
	virtual const ManeuverParams &getManeuve()const;
	virtual HitBoxList *getTraceHitBoxes()const{return &hitboxes;}

	static double getModelScale(){return modelScale;}

	static Model *model;
	static Motion *motions[];

protected:
	SQInteger sqGet(HSQUIRRELVM v, const SQChar *name)const override;
	SQInteger sqSet(HSQUIRRELVM v, const SQChar *name)override;

	void init();
	int shoot_infgun(double phi0, double theta0, double v, double damage, double variance, double t, Mat4d &gunmat);
	void swapWeapon();
	void reload();
	bool findEnemy();
	double getFov()const;
	Vec3d getHookPos();

	void hookHitEffect(const otjEnumHitSphereParam &param);
	void drawHookAndTether(WarDraw *);
	bool initModel();
	bool interpolate(MotionPose *mp);
	bool getGunPos(GetGunPosCommand &);

	double pitch;
	double cooldown2;
	double walkphase;
	double swapphase;
	double reloadphase;
	double kick[2], kickvelo[2];
	double integral[2];
	Vec3d hurtdir;
	float damagephase;
	float shiftphase;
	float aimphase;
	float hurt;
/*	int ammo;*/
	char state;
	char controlled;
	char reloading;
	char muzzle;
	bool aiming;
	bool forcedEnemy;
	bool forcedRot; ///< Forced set rotation from the server
	Firearm *arms[2];
	Vec3d hookpos;
	Vec3d hookvelo;
	int hookhitpart;
	int standPart;
	WeakPtr<Entity> hookedEntity;
	WeakPtr<Entity> standEntity;
	bool hookshot;
	bool hooked;
	bool hookretract;

	static HitBoxList hitboxes;
	static double modelScale;
	static double hitRadius;
	static double defaultMass;
	static double maxHealthValue;
	static double hookSpeed;
	static double hookRange;
	static double hookPullAccel;
	static double hookStopRange;
	static ManeuverParams maneuverParams;
	static GLuint overlayDisp;
	static double muzzleFlashRadius[2];
	static Vec3d muzzleFlashOffset[2];
};



/// \brief M16A1 assault rifle. It's silly to see it in space.
class Rifle : public Firearm{
public:
	typedef Firearm st;

	static EntityRegisterNC<Rifle> entityRegister;

	Rifle(Game *game) : st(game){init();}
	Rifle(Entity *abase, const hardpoint_static *hp, FirearmStatic *fs);
	EntityStatic &getStatic()const override{return entityRegister;}
	void anim(double dt){}
	void draw(WarDraw *);

	static void s_init();

	typedef std::map<gltestp::dstring, FirearmStatic> FirearmDefs;
	static FirearmDefs firearmDefs;

protected:
	int maxammo()const{return fs->maxAmmoValue;}
	double shootCooldown()const{return fs->shootCooldownValue;}
	double bulletSpeed()const{return fs->bulletSpeedValue;}
	double bulletDamage()const{return fs->bulletDamageValue;}
	double bulletVariance()const{return fs->bulletVarianceValue;}
	double aimFov()const{return fs->aimFovValue;}
	double shootRecoil()const{return fs->shootRecoilValue;}
	HSQOBJECT getSqShoot()const override{return fs->sqShoot;}
	FirearmStatic *getFirearmStatic()const override{return fs;}

	FirearmStatic *fs;
	static FirearmStatic defaultFS;
};




struct GetGunPosCommand : public EntityCommand{
	COMMAND_BASIC_MEMBERS(GetGunPosCommand, EntityCommand);
	GetGunPosCommand(int gunId = 0) : gunId(gunId){}
	const int gunId;
	Vec3d pos;
	Quatd rot;
	Quatd gunRot; ///< A Soldier can have recoil, which makes the gun to diverse the aim.
};

struct HookPosWorldToLocalCommand : public EntityCommand{
	COMMAND_BASIC_MEMBERS(HookPosWorldToLocalCommand, EntityCommand);
	HookPosWorldToLocalCommand(int hitpart = 0, const Vec3d *srcpos = NULL) : supported(false), hitpart(hitpart), srcpos(srcpos){}
	bool supported; ///< If the queried Entity supports this EntitiyCommand, 
	const int hitpart;
	const Vec3d *srcpos;
	Vec3d pos;
	Quatd rot;
};

struct HookPosLocalToWorldCommand : public EntityCommand{
	COMMAND_BASIC_MEMBERS(HookPosLocalToWorldCommand, EntityCommand);
	HookPosLocalToWorldCommand(int hitpart = 0, const Vec3d *srcpos = NULL) : supported(false), hitpart(hitpart), srcpos(srcpos){}
	bool supported;
	const int hitpart;
	const Vec3d *srcpos;
	Vec3d pos;
	Quatd rot;
};



#endif
