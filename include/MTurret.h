/** \file
 * \brief Definition of MTurret and its derived classes.
 */
#ifndef MTURRET_H
#define MTURRET_H
#include "arms.h"
#include "Model-forward.h"

/// \brief Medium-sized gun turret for spaceships.
class EXPORT MTurret : public ArmBase{
protected:
	void findtarget(Entity *pb, const hardpoint_static *hp, const Entity *ignore_list[] = NULL, int nignore_list = 0);
	virtual double findtargetproc(const Entity *pb, const hardpoint_static *hp, const Entity *pt2); // returns precedence factor
	virtual bool checkTargetRange(double phi, double theta)const;
	virtual int wantsFollowTarget()const; // Returns whether the turret should aim at target, 0 - no aim, 1 - yaw only, 2 - yaw and pitch, default 2.
	static double modelScale;
	static double hitRadius;
	static double turretVariance;
	static double turretIntolerance;
	static double rotateSpeed;
	static double manualRotateSpeed;
	static gltestp::dstring modelFile;
	static gltestp::dstring turretObjName;
	static gltestp::dstring barrelObjName;
	static gltestp::dstring muzzleObjName;
public:
	typedef ArmBase st;
	float cooldown;
	float py[2]; // pitch and yaw
	float mf; // muzzle flash time
	bool forceEnemy;

	static EntityRegisterNC<MTurret> entityRegister;
	MTurret(Game *game) : st(game){init();}
	MTurret(Entity *abase, const hardpoint_static *hp);
	void init();
	EntityStatic &getStatic()const override;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual void control(const input_t *, double dt);
	virtual void anim(double dt);
	virtual double getHitRadius()const;
	virtual Props props()const;
	virtual gltestp::dstring descript()const;
	virtual bool command(EntityCommand *);

	virtual float getShootInterval()const;
	virtual double getBulletSpeed()const;
	virtual float getBulletLife()const;
protected:
	virtual double getTurretVariance()const{return turretVariance;}
	virtual double getTurretIntolerance()const{return turretIntolerance;}
	virtual double getRotateSpeed()const{return rotateSpeed;}
	virtual double getManualRotateSpeed()const{return manualRotateSpeed;}
	virtual void tryshoot();
};

/// \brief Middle-sized gatling gun turret for spaceships.
class EXPORT GatlingTurret : public MTurret{
protected:
	static double modelScale;
	static double hitRadius;
	static double turretVariance;
	static double turretIntolerance;
	static double rotateSpeed;
	static double manualRotateSpeed;
	static double bulletDamage;
	static double bulletLife;
	static double shootInterval;
	static double bulletSpeed;
	static int magazineSize;
	static double reloadTime;
	static double muzzleFlashDuration;
	static double barrelRotSpeed;
	static Vec3d shootOffset;
	static gltestp::dstring modelFile;
	static gltestp::dstring turretObjName;
	static gltestp::dstring barrelObjName;
	static gltestp::dstring barrelRotObjName;
	static gltestp::dstring muzzleObjName;

	static Model *model;
	static int turretIndex;
	static int barrelIndex;
	static int barrelRotIndex;
	static int muzzleIndex;
public:
	typedef MTurret st;
	static EntityRegisterNC<GatlingTurret> entityRegister;
	GatlingTurret(Game *game);
	GatlingTurret(Entity *abase, const hardpoint_static *hp);
	void init();
	EntityStatic &getStatic()const override;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual void anim(double dt);
	double getHitRadius()const override{return hitRadius;}
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *w);
	float getShootInterval()const override;
	double getBulletSpeed()const override;
	float getBulletLife()const override;
protected:
	virtual double getTurretVariance()const{return turretVariance;}
	virtual double getTurretIntolerance()const{return turretIntolerance;}
	virtual double getRotateSpeed()const{return rotateSpeed;}
	virtual double getManualRotateSpeed()const{return manualRotateSpeed;}
	virtual void tryshoot();
	float barrelrot;
	float barrelomg;
};



#endif
