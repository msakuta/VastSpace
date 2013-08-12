/** \file
 * \brief Definition of Apache class.
 */
#ifndef APACHE_H
#define APACHE_H
#include "ModelEntity.h"
#include "Model-forward.h"
#include "Motion-forward.h"
#include "Bullet.h"
#include "arms.h"


/// \brief AH-64 Apache attack helicopter
///
/// http://en.wikipedia.org/wiki/Boeing_AH-64_Apache
class Apache : public ModelEntity{
public:
	typedef ModelEntity st;
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
	int weapon;
	int ammo_chaingun;
	int hellfires;
	int aim9; /* AIM-9 Sidewinder */
	int hydras;
	int contact; /* contact state, not really necessary */
	char muzzle; /* muzzle flash status */
	char brk;
	char navlight; /* switch of navigation lights for flight in dark */

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
	static HSQOBJECT sqHydraFire;
	static Vec3d cockpitOfs;
	static HitBoxList hitboxes;
	static GLuint overlayDisp;
	static std::vector<hardpoint_static*> hardpoints;

	static const HitBoxList &getHitBoxes(){return hitboxes;}
	bool buildBody();
	int shootChainGun(double dt);
	int shootHydraRocket(double dt);
	void gunMotion(MotionPose *mp); ///< \param mp must be an array having at least 2 elements

	friend class HydraRocketLauncher;
};

/// \brief Hydra-70 unguided rocket.
class HydraRocket : public Bullet{
public:
	typedef Bullet st;
	static EntityRegister<HydraRocket> entityRegister;
	HydraRocket(Game *game) : st(game), pf(NULL), fuel(3.){}
	HydraRocket(WarField *w);
	HydraRocket(Entity *owner, float life, double damage) :
		st(owner, life, damage), pf(NULL), fuel(3.){}
	void anim(double dt)override;
	void clientUpdate(double dt)override;
	void draw(WarDraw *wd)override;
	void drawtra(WarDraw *wd)override;
	void enterField(WarField*)override;
	void leaveField(WarField*)override;

protected:
	static const double modelScale;

	Tefpol *pf;
	double fuel;

	void commonUpdate(double dt);
	void bulletDeathEffect(int hitground, const struct contact_info *ci)override;
};

/// \brief M261, a Hydra-70 rocket launcher mounted on attack helicopters.
class HydraRocketLauncher : public ArmBase{
public:
	typedef ArmBase st;
	HydraRocketLauncher(Game *game) : st(game), cooldown(0.){}
	HydraRocketLauncher(Entity *base, const hardpoint_static *hp) : st(base, hp), cooldown(0.){}
	void anim(double dt)override;
	void draw(WarDraw *)override;
protected:
	double cooldown;
};

#endif
