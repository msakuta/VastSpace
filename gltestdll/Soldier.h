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
extern "C"{
#include <clib/mathdef.h>
}

struct Model;
class Motion;
struct MotionPose;

struct GetGunPosCommand;

/// \brief The abstract class for personal firearms.
class Firearm : public ArmBase{
public:
	typedef ArmBase st;

	Firearm(Game *game) : st(game){}
	Firearm(Entity *abase, const hardpoint_static *hp) : st(abase, hp){}
	void shoot();
	void reload();
	virtual int maxammo()const = 0;
	virtual double shootCooldown()const = 0;
	virtual double bulletSpeed()const = 0;
	virtual double bulletDamage()const = 0;
	virtual double bulletLifeTime()const{return 5.;}
	virtual double bulletVariance()const{return 0.01;}
	virtual double aimFov()const{return 0.7;} ///< Magnitude of zoom when aiming expressed in FOV.
	virtual double shootRecoil()const{return M_PI / 64.;}
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
	virtual std::vector<hitbox> *getTraceHitBoxes()const{return &hitboxes;}

	static double getModelScale(){return modelScale;}

	/// \brief Retrieves root path for this extension module.
	static gltestp::dstring modPath(){return "gltestdll/";}

	static Model *model;
	static Motion *motions[];

protected:
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
	WeakPtr<Entity> hookedEntity;
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
class M16 : public Firearm{
public:
	typedef Firearm st;

	static const unsigned classid;

	M16(Game *game) : st(game){}
	M16(Entity *abase, const hardpoint_static *hp);
	const char *classname()const;
	void anim(double dt){}
	void draw(WarDraw *);

	static gltestp::dstring modPath(){return Soldier::modPath();}

protected:
	void init();
	int maxammo()const{return maxAmmoValue;}
	double shootCooldown()const{return shootCooldownValue;}
	double bulletSpeed()const{return bulletSpeedValue;}
	double bulletDamage()const{return bulletDamageValue;}
	double bulletVariance()const{return bulletVarianceValue;}
	double aimFov()const{return aimFovValue;}
	double shootRecoil()const{return shootRecoilValue;}

	static int maxAmmoValue;
	static double shootCooldownValue;
	static double bulletSpeedValue;
	static double bulletDamageValue;
	static double bulletVarianceValue;
	static double aimFovValue;
	static double shootRecoilValue;
};

/// \brief M40 sniper rifle. It's silly to see it in space.
class M40 : public Firearm{
public:
	typedef Firearm st;

	static const unsigned classid;

	M40(Game *game) : st(game){}
	M40(Entity *abase, const hardpoint_static *hp);
	const char *classname()const;
	void anim(double dt){}
	void draw(WarDraw *);

	static gltestp::dstring modPath(){return Soldier::modPath();}

protected:
	void init();
	int maxammo()const{return maxAmmoValue;}
	double shootCooldown()const{return shootCooldownValue;}
	double bulletSpeed()const{return bulletSpeedValue;}
	double bulletDamage()const{return bulletDamageValue;}
	double bulletVariance()const{return bulletVarianceValue;}
	double aimFov()const{return aimFovValue;}
	double shootRecoil()const{return shootRecoilValue;}

	static int maxAmmoValue;
	static double shootCooldownValue;
	static double bulletSpeedValue;
	static double bulletDamageValue;
	static double bulletVarianceValue;
	static double aimFovValue;
	static double shootRecoilValue;
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
