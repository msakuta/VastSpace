/** \file
 * \brief Definition of Apache class.
 */
#ifndef APACHE_H
#define APACHE_H
#include "Model-forward.h"
#include "ModelEntity.h"


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
	void drawHUD(WarDraw *)override;
	void drawCockpit(WarDraw *)override;
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
	short bbodyGroup()const override{return 1<<2;}
	short bbodyMask()const override{return 1<<2;}

	double rotor, rotoromega, tailrotor, rotoraxis[2], crotoraxis[2], gun[2];
	double throttle, feather, tail;
	double cooldown;
	double cooldown2;
/*	apachearms arms[numof(arms_s)];*/
//	arms_t arms[7];
//	shieldw_t *sw;
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
	static Vec3d cockpitOfs;
	static HitBoxList hitboxes;

	static const HitBoxList &getHitBoxes(){return hitboxes;}
	bool buildBody();
	int shootChainGun(double dt);
};



#endif
