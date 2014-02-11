/* \file
 * \brief Definition of ReZEL class.
 */
#ifndef GUNDAM_REZEL_H
#define GUNDAM_REZEL_H

#include "StaticBind.h"
#include "Frigate.h"
#include "MobileSuit.h"
#include "mqo.h"
#include "ysdnmmot.h"
#include "libmotion.h"
#include "msg/GetCoverPointsMessage.h"


struct btCompoundShape;

/// ReZEL
class ReZEL : public MobileSuit{
public:
	typedef MobileSuit st;
protected:

	static const int motionCount = 16;

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
	bool cull(Viewer &)const;
	double nlipsFactor(Viewer &)const;
	Quatd aimRot()const;
	double coverFactor()const{return min(coverRight, 1.);}
	void reloadRifle();
	static SQInteger sqf_get(HSQUIRRELVM);
	friend class EntityRegister<ReZEL>;
	static StaticBindDouble rotationSpeed;
	static StaticBindDouble maxAngleSpeed;
	static StaticBindDouble maxFuel;
	static StaticBindDouble fuelRegenRate;
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
	ReZEL(Game *game);
	ReZEL(WarField *aw);
	virtual const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	static EntityRegister<ReZEL> entityRegister;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual double getMaxHealth()const override;
	virtual void enterField(WarField *);
	virtual void draw(WarDraw *);
	virtual void drawtra(WarDraw *);
	virtual void drawHUD(WarDraw *);
	virtual void drawOverlay(wardraw_t *);
	virtual bool solid(const Entity*)const;
	virtual int takedamage(double damage, int hitpart);
	virtual double getHitRadius()const;
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal);

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
	double maxfuel()const override;
	double getFuelRegenRate()const override{return fuelRegenRate;}
	int getVulcanMagazineSize()const override{return vulcanMagazineSize;}
	int getRifleMagazineSize()const override{return rifleMagazineSize;}
	double getRotationSpeed()const override{return rotationSpeed;}
	double getMaxAngleSpeed()const override{return maxAngleSpeed;}
	int getReloadTime()const override{return reloadTime;}
	const HitBoxList &getHitBoxes()const override{return hitboxes;}
	btCompoundShape *getShape()const override{return shape;}
	btCompoundShape *getWaveRiderShape()const override{return waveRiderShape;}
	Model *getModel()const override{return model;}
	double getModelScale()const override{return modelScale;}
	HSQOBJECT getSqPopupMenu()override{ return sqPopupMenu; }
	HSQOBJECT getSqCockpitView()const override{ return sqCockpitView; }
};


#endif
