/** \file
 * \brief Definition of Apache class.
 */
#ifndef APACHE_H
#define APACHE_H
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
	void drawCockpit(WarDraw *)override;
	void anim(double dt)override;
	void draw(WarDraw *wd)override;
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

	static double modelScale;
	static double defaultMass;
	static double maxHealthValue;
	static double rotorAxisSpeed;
	static Vec3d cockpitOfs;
	static HitBoxList hitboxes;

	static const HitBoxList &getHitBoxes(){return hitboxes;}
};



#endif
