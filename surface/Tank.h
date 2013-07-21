/** \file
 * \brief Definition of Tank and its projectiles.
 */
#ifndef TANK_H
#define TANK_H
#include "war.h"
#include "ModelEntity.h"
#include "judge.h"
#include "btadapt.h"
#include "Bullet.h"
extern "C"{
#include <clib/c.h>
}

class Model;

/// \brief Armor-Piercing Fin-Stabilized Discarding Sabot projectile.
///
/// This is the Tank's main gun round.  It's almost the same as normal Bullet
/// with high speed and splash damage, but with customized appearance.
/// Specifically, solid model is drawn for the projectile, although it's
/// so high speed only allow it to be seen in stop motion.
class APFSDS : public Bullet{
public:
	typedef Bullet st;
	APFSDS(Game *game) : st(game){}
	APFSDS(Entity *owner, float life, double damage) : st(owner, life, damage){}
	void draw(WarDraw*)override;
	void drawtra(WarDraw*)override;
};

/// \brief The base class for vehicles that can run on land terrain.
class LandVehicle : public ModelEntity{
public:
	typedef ModelEntity st;

	LandVehicle(Game *game) : st(game){}
	LandVehicle(WarField *w) : st(w){}
	void addRigidBody(WarSpace*)override;
	virtual void anim(double dt);
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal); // return nonzero on hit
	void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	bool isTargettable()const override{return true;}
	bool isSelectable()const override{return true;}

	double steer; /* Steering direction, positive righthand */
	double wheelspeed;
	double wheelangle;
	double turrety, barrelp;
	double desired[2];
	double cooldown, cooldown2;
	double mountpy[2];
	int ammo[3]; /* main gun, coaxial gun, mounted gun */
	char muzzle; /* muzzleflash status */
	int weapon;

/*	dBodyID body;
	dGeomID geom;*/

	/* graphical debugging vars */
    static const Vec3d points[];
//	Vec3d forces[numof(points)];
//	Vec3d normals[numof(points)];

	static gltestp::dstring modPath(){return "surface/";}

protected:

	short bbodyGroup()const override{return 1<<2;}
	short bbodyMask()const override{return 1<<2;}

	virtual HitBoxList &getHitBoxes()const = 0;
	virtual double getTopSpeed()const = 0;
	virtual double getBackSpeed()const = 0;
	virtual double getLandOffset()const{return 0.0010;}
	virtual bool buildBody();
	virtual void aiControl(double dt, const Vec3d &normal){}

	void find_enemy_logic();
	void vehicle_drive(double dt, Vec3d *points, int npoints);
};

/// \brief JSDF's type 90 main buttle tank.
class Tank : public LandVehicle{
public:
	typedef LandVehicle st;
	static const unsigned classid;
	static EntityRegister<Tank> entityRegister;

	Tank(Game *game);
	Tank(WarField *);
	const char *idname()const override;
	const char *classname()const override;
	void control(const input_t *inputs, double dt)override;
	void anim(double dt)override;
	int takedamage(double damage, int hitpart)override;
	void draw(wardraw_t *)override;
	void drawtra(wardraw_t *)override;
	double getHitRadius()const;
	double getMaxHealth()const override{return maxHealthValue;}
protected:

	double sightCheckTime;
	bool sightCheck;

	static Model *model;
	static double modelScale;
	static double defaultMass;
	static double maxHealthValue;
	static double topSpeed;
	static double backSpeed;
	static double mainGunCooldown;
	static double mainGunMuzzleSpeed;
	static double mainGunDamage;
	static double turretYawSpeed;
	static double barrelPitchSpeed;
	static double barrelPitchMin;
	static double barrelPitchMax;
	static double sightCheckInterval;
	static HitBoxList hitboxes;

	HitBoxList &getHitBoxes()const override{return hitboxes;}
	double getTopSpeed()const override{return topSpeed;}
	double getBackSpeed()const override{return backSpeed;}
	void aiControl(double dt, const Vec3d &normal)override;

	void init();
	int shootcannon(double dt);
	int tryshoot(double dt);
	Vec3d tankMuzzlePos(Vec3d *nh = NULL)const;
	void deathEffects();

	static void gib_draw(const struct tent3d_line_callback*, const struct tent3d_line_drawdata*, void *private_data);
};

/// \brief M3 half track
class M3Truck : public LandVehicle{
public:
	typedef LandVehicle st;
	static const unsigned classid;
	static EntityRegister<M3Truck> entityRegister;

	M3Truck(Game *game) : st(game){init();}
	M3Truck(WarField *);
	const char *idname()const override;
	const char *classname()const override;
	void anim(double dt)override;
	int takedamage(double damage, int hitpart)override;
	void draw(wardraw_t *)override;
	void drawCockpit(WarDraw *)override;
	void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const override;
	double getHitRadius()const{return 0.02;}
	double getMaxHealth()const override{return maxHealthValue;}

protected:
	static Model *model;
	static double modelScale;
	static double landOffset;
	static double defaultMass;
	static double maxHealthValue;
	static double topSpeed;
	static double backSpeed;
	static double turretCooldown;
	static double turretMuzzleSpeed;
	static double turretDamage;
	static double turretYawSpeed;
	static double barrelPitchSpeed;
	static double barrelPitchMin;
	static double barrelPitchMax;
	static std::vector<Vec3d> cameraPositions;
	static HitBoxList hitboxes;

	HitBoxList &getHitBoxes()const override{return hitboxes;}
	double getTopSpeed()const override{return topSpeed;}
	double getBackSpeed()const override{return backSpeed;}
	double getLandOffset()const override{return landOffset;}
	void aiControl(double dt, const Vec3d &normal)override;

	void init();
	bool tryshoot(double dt);
	Vec3d turretMuzzlePos(Vec3d *nh = NULL)const;
	void deathEffects();
};

#endif
