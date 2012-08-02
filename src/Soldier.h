/** \file
 * \brief Definition of Soldier class.
 */
#ifndef SOLDIER_H
#define SOLDIER_H
#include "Autonomous.h"
#include "war.h"
#include "arms.h"

class Soldier : public Autonomous{
public:
	typedef Autonomous st;

	static EntityRegister<Soldier> entityRegister;
	static const unsigned classid;
	virtual const char *classname()const;

	Soldier(Game *);
	Soldier(WarField *);
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual void draw(WarDraw *);
	virtual void drawHUD(WarDraw *);
	virtual void drawOverlay(WarDraw *);
	virtual double hitradius()const;
	virtual double maxhealth()const;
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;
	virtual Props props()const;
	virtual const ManeuverParams &getManeuve()const;

	static double getModelScale(){return modelScale;}

protected:
	void init();
	int shoot_infgun(double phi0, double theta0, double v, double damage, double variance, double t, Mat4d &gunmat);
	void swapWeapon();
	void reload();
	bool findEnemy();

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
	ArmBase *arms[2];

	static double modelScale;
	static double defaultMass;
	static ManeuverParams maneuverParams;
	static GLuint overlayDisp;
};


class M16 : public ArmBase{
public:
	typedef ArmBase st;

	static EntityRegister<Soldier> entityRegister;
	static const unsigned classid;

	M16(Game *game) : st(game){}
	M16(Entity *abase, const hardpoint_static *hp);
	const char *classname()const;
	void anim(double dt){}
	void draw(WarDraw *);
	bool command(EntityCommand *);
protected:
	void shoot();
	int maxammo()const{return 20;}
};


#endif
