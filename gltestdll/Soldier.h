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
	virtual void draw(WarDraw *);
	virtual void drawtra(WarDraw *);
	virtual void drawHUD(WarDraw *);
	virtual void drawOverlay(WarDraw *);
	virtual double getHitRadius()const;
	virtual double maxhealth()const;
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;
	virtual void bullethit(const Bullet *);
	virtual Props props()const;
	virtual int armsCount()const;
	virtual ArmBase *armsGet(int);
	virtual bool command(EntityCommand *com);
	virtual const ManeuverParams &getManeuve()const;
	virtual std::vector<hitbox> *getTraceHitBoxes()const{return &hitboxes;}

	static double getModelScale(){return modelScale;}

	static Model *model;
	static Motion *motions[];

protected:
	void init();
	int shoot_infgun(double phi0, double theta0, double v, double damage, double variance, double t, Mat4d &gunmat);
	void swapWeapon();
	void reload();
	bool findEnemy();

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
	float damagephase;
	float shiftphase;
/*	int ammo;*/
	char state;
	char controlled;
	char reloading;
	char aiming;
	char muzzle;
	bool forcedEnemy;
	Firearm *arms[2];
	Vec3d hookpos;
	Vec3d hookvelo;
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

protected:
	int maxammo()const{return 20;}
	double shootCooldown()const{return 0.1;}
	double bulletSpeed()const{return 1.;}
	double bulletDamage()const{return 1.;}
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

protected:
	int maxammo()const{return 5;}
	double shootCooldown()const{return 1.5;}
	double bulletSpeed()const{return 3.;}
	double bulletDamage()const{return 3.;}
	double bulletVariance()const{return 0.001;}
};



struct GetGunPosCommand : public EntityCommand{
	COMMAND_BASIC_MEMBERS(GetGunPosCommand, EntityCommand);
	GetGunPosCommand(int gunId = 0) : gunId(gunId){}
	const int gunId;
	Vec3d pos;
	Quatd rot;
};



#endif
