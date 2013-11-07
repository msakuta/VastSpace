/** \file
 * \brief Definition of Apache class.
 */
#ifndef APACHE_H
#define APACHE_H
#include "Aerial.h"
#include "Model-forward.h"
#include "Motion-forward.h"
#include "arms.h"
#include "SqInitProcess-ex.h"


/// \brief AH-64 Apache attack helicopter
///
/// http://en.wikipedia.org/wiki/Boeing_AH-64_Apache
class Apache : public Aerial{
public:
	typedef Aerial st;
	static EntityRegister<Apache> entityRegister;
	Apache(Game *game) : st(game){}
	Apache(WarField *w);
	void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const override;
	void control(const input_t *inputs, double dt)override;
	void anim(double dt)override;
	void draw(WarDraw *wd)override;
	void drawtra(WarDraw *wd)override;
	void drawHUD(WarDraw *)override;
	void drawCockpit(WarDraw *)override;
	void drawOverlay(WarDraw *)override;
	int takedamage(double damage, int hitpart)override;
	const char *idname()const override{return "apache";}
	const char *classname()const override{return "Apache";}
	const char *dispname()const override{return "AH-64 Apache";}
	void start_control();
	void end_control();
	int tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn)override;
	bool isTargettable()const override{return true;}
	bool isSelectable()const override{return true;}
	double getHitRadius()const override{return 0.02;}
	double getMaxHealth()const override{return maxHealthValue;}

	static gltestp::dstring modPath(){return "surface/";}
protected:
	SQInteger sqGet(HSQUIRRELVM, const SQChar *)const override;
	SQInteger sqSet(HSQUIRRELVM, const SQChar *)override;
	short bbodyGroup()const override{return 1<<2;}
	short bbodyMask()const override{return 1<<2;}

	double rotor, rotoromega, tailrotor, rotoraxis[2], crotoraxis[2], gun[2];
	double throttle, feather, tail;
	double cooldown;
	double cooldown2;
	typedef ObservableList<ArmBase> ArmList;
	ArmList arms;
	int ammo_chaingun;

	void init();
	void find_enemy_logic();

	static Model *model;
	static double modelScale;
	static double defaultMass;
	static double maxHealthValue;
	static double rotorAxisSpeed;
	static double mainRotorLiftFactor; ///< How strong the main rotor's lift is
	static double tailRotorLiftFactor; ///< How strong the tail rotor's lift is
	static double featherSpeed; ///< Speed of feathering angle change
	static double tailRotorSpeed; ///< Speed factor of the tail rotor relative to the main rotor.
	static double chainGunCooldown; ///< Shoot repeat cycle time
	static double chainGunMuzzleSpeed; ///< Speed of shot projectile at the gun muzzle, which may decrease as it travels through air.
	static double chainGunDamage; ///< Bullet damage for each chain gun round.
	static double chainGunVariance; ///< Chain gun variance (inverse accuracy) in cosine angles.
	static double chainGunLife; ///< Time before shot bullet disappear
	static double hydraDamage;
	static HSQOBJECT sqFire;
	static HSQOBJECT sqQueryAmmo;
	static Vec3d cockpitOfs;
	static HitBoxList hitboxes;
	static Vec3d hudPos;
	static double hudSize;
	static GLuint overlayDisp;
	static std::vector<hardpoint_static*> hardpoints;
	static StringList defaultArms;
	static StringList weaponList;

	WingList &getWings()const override{return WingList();}
	HitBoxList &getHitBoxes()const override{return hitboxes;}
	double getThrustStrength()const override{return 0.;}
	btCompoundShape *getShape(){return NULL;}
	bool buildBody();
	int shootChainGun(double dt);
	void shoot(double dt);
	void gunMotion(MotionPose *mp); ///< \param mp must be an array having at least 2 elements
	bool isCockpitView(int chasecam)const override{return chasecam == 0 || chasecam == 4 && !lastMissile;}
	gltestp::dstring getWeaponName()const override{return weaponList[weapon];}
	int getAmmo()const override{return getAmmoFromSQ(sqQueryAmmo);}

	friend class HydraRocketLauncher;
};



#endif
