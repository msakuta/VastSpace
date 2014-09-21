/** \file
 * \brief Definition of LTurretBase derived classes.
 */
#ifndef LTURRET_H
#define LTURRET_H
#include "MTurret.h"

class LTurretBase : public MTurret{
public:
	LTurretBase(Game *game) : MTurret(game){}
	LTurretBase(Entity *abase, const hardpoint_static *hp) : MTurret(abase, hp){}
};

class LTurret : public LTurretBase{
	float blowback, blowbackspeed;
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

	static Model *model;
	static int turretIndex;
	static int barrelIndex;
	static int muzzleIndex;
public:
	typedef LTurretBase st;
	static EntityRegisterNC<LTurret> entityRegister;
	LTurret(Game *game) : st(game), blowback(0), blowbackspeed(0){init();}
	LTurret(Entity *abase, const hardpoint_static *hp) : st(abase, hp), blowback(0), blowbackspeed(0){init();}
	void init();
	EntityStatic &getStatic()const override;
	const char *dispname()const override{return "Large Turret";}
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual double getHitRadius()const;
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	float getShootInterval()const override;
	float getBulletLife()const override;
	virtual void tryshoot();
	virtual double findtargetproc(const Entity *pb, const hardpoint_static *hp, const Entity *pt2);
protected:
	double getTurretVariance()const override;
	double getTurretIntolerance()const override;
	double getRotateSpeed()const override{return rotateSpeed;}
	double getManualRotateSpeed()const override{return manualRotateSpeed;}
	void shootTransform(Mat4d &mat, Quatd *qrot = NULL)const;
	void shootEffect(const Vec3d &bulletpos, const Vec3d &direction);
};

class LMissileTurret : public LTurretBase{
	const Entity *targets[6]; // Targets already locked up
	float deploy; // Deployment factor.

	static double modelScale;
	static double hitRadius;
	static double turretVariance;
	static double turretIntolerance;
	static double rotateSpeed;
	static double manualRotateSpeed;
	static gltestp::dstring modelFile;
	static gltestp::dstring deployMotionFile;

protected:
	virtual int wantsFollowTarget()const;
public:
	typedef LTurretBase st;
	static EntityRegisterNC<LMissileTurret> entityRegister;
	LMissileTurret(Game *game);
	LMissileTurret(Entity *abase, const hardpoint_static *hp);
	void init();
	virtual double getHitRadius()const;
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual void draw(WarDraw *);
	virtual void drawtra(WarDraw *);
	double getBulletSpeed()const override;
	float getShootInterval()const override;
	virtual void tryshoot();
	virtual double findtargetproc(const Entity *pb, const hardpoint_static *hp, const Entity *pt2);
protected:
	double getTurretVariance()const override{return turretVariance;}
	double getTurretIntolerance()const override{return turretIntolerance;}
	double getRotateSpeed()const override{return rotateSpeed;}
	double getManualRotateSpeed()const override{return manualRotateSpeed;}
};


#endif
