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
	virtual const ManeuverParams &getManeuve()const;

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

#endif
