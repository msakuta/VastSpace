/** \file
 * \brief Defines F-15 Eagle fighter class.
 */
#ifndef SURFACE_F15_H
#define SURFACE_F15_H

#include "Aerial.h"
#include "arms.h"

/// \brief McDonnell Douglas F-15 Eagle
///
/// \see http://en.wikipedia.org/wiki/McDonnell_Douglas_F-15_Eagle
class F15 : public Aerial{
public:
	typedef Aerial st;
	static EntityRegister<F15> entityRegister;

	F15(Game *game);
	F15(WarField *aw);
	const char *idname()const override{return "f15";}
	const char *classname()const override{return "F15";}
	void anim(double dt)override;
	void draw(WarDraw *wd)override;
	void drawtra(WarDraw *wd)override;
	void drawHUD(WarDraw *)override;
	void drawCockpit(WarDraw *)override;
	void drawOverlay(WarDraw *)override;
	void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const override;
	int takedamage(double damage, int hitpart)override;
	double getHitRadius()const override{return hitRadius;}
	double getMaxHealth()const override{return maxHealthValue;}

	static double getModelScale(){return modelScale;}

protected:
	SQInteger sqGet(HSQUIRRELVM, const SQChar *)const override;
	SQInteger sqSet(HSQUIRRELVM, const SQChar *)override;

	Tefpol *pf;
	std::vector<Tefpol*> vapor;
	Vec3d destPos; ///< Destination position
	typedef ObservableList<ArmBase> ArmList;
	ArmList arms;

	static Model *model;
	static double modelScale;
	static double hitRadius;
	static double defaultMass; ///< Dry mass?
	static double maxHealthValue;
	static HSQOBJECT sqSidewinderFire;
	static HSQOBJECT sqQueryAmmo;
	static HitBoxList hitboxes;
	static NavlightList navlights;
	static double thrustStrength;
	static WingList wings0;
	static std::vector<Vec3d> wingTips;
	static std::vector<Vec3d> gunPositions;
	static Vec3d gunDirection;
	static double shootCooldown;
	static double bulletSpeed;
	static CameraPosList cameraPositions;
	static Vec3d hudPos;
	static double hudSize;
	static GLuint overlayDisp;
	static bool debugWings;
	static std::vector<hardpoint_static*> hardpoints;
	static std::vector<Vec3d> engineNozzles;
	static StringList defaultArms;
	static StringList weaponList;
	static gltestp::dstring flyingSoundFile;
	static gltestp::dstring flyingHiSoundFile;

	WingList &getWings()const override{return wings0;}
	HitBoxList &getHitBoxes()const override{return hitboxes;}
	void shoot(double dt)override;
	double getThrustStrength()const override{return thrustStrength;}
	bool isCockpitView(int chasecam)const override{
		return cameraPositions[chasecam].type == CameraPos::Type::Cockpit
			|| cameraPositions[chasecam].type == CameraPos::Type::MissileTrack && !lastMissile;
	}
	gltestp::dstring getWeaponName()const override{return weaponList[weapon];}
	int getAmmo()const override{return getAmmoFromSQ(sqQueryAmmo);}
	gltestp::dstring getFlyingSoundFile()const override{return flyingSoundFile;}
	gltestp::dstring getFlyingHiSoundFile()const override{return flyingHiSoundFile;}
	static SQInteger sqf_debugWings(HSQUIRRELVM);
	btCompoundShape *getShape()override;

	void init();

	friend EntityRegister<F15>;
};

#endif
