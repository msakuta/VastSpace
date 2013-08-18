/** \file
 * \brief Defines A-10 Thunderbolt II
 */
#ifndef SURFACE_A10_H
#define SURFACE_A10_H

#include "Aerial.h"
#include "arms.h"
#include "SqInitProcess-ex.h"

/// \brief Fairchild A-10 Thunderbolt II
///
/// \see http://en.wikipedia.org/wiki/Fairchild_Republic_A-10_Thunderbolt_II
class A10 : public Aerial{
public:
	typedef Aerial st;
	static EntityRegister<A10> entityRegister;

	A10(Game *game);
	A10(WarField *aw);
	const char *idname()const override{return "A10";}
	const char *classname()const override{return "A10";}
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
	WeakPtr<Bullet> lastMissile; ///< Reference to last shot missile
	Vec3d destPos; ///< Destination position
	typedef ObservableList<ArmBase> ArmList;
	ArmList arms;

	static Model *model;
	static double modelScale;
	static double hitRadius;
	static double defaultMass; ///< Dry mass?
	static double maxHealthValue;
	static HSQOBJECT sqSidewinderFire;
	static HitBoxList hitboxes;
	static double thrustStrength;
	static WingList wings0;
	static std::vector<Vec3d> wingTips;
	static std::vector<Vec3d> gunPositions;
	static double shootCooldown;
	static double bulletSpeed;
	static std::vector<Vec3d> cameraPositions;
	static Vec3d hudPos;
	static double hudSize;
	static GLuint overlayDisp;
	static bool debugWings;
	static std::vector<hardpoint_static*> hardpoints;
	static StringList defaultArms;

	WingList &getWings()const override{return wings0;}
	HitBoxList &getHitBoxes()const override{return hitboxes;}
	void shoot(double dt)override;
	double getThrustStrength()const override{return thrustStrength;}
	bool isCockpitView(int chasecam)const{return chasecam == 0 || chasecam == cameraPositions.size() && !lastMissile;}
	static SQInteger sqf_debugWings(HSQUIRRELVM);

	void init();

	friend EntityRegister<A10>;
};

#endif
