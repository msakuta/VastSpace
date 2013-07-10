#ifndef TANK_H
#define TANK_H
#include "war.h"
#include "ModelEntity.h"
#include "judge.h"
#include "btadapt.h"
extern "C"{
#include <clib/c.h>
}


class Tank : public ModelEntity{
public:
	typedef ModelEntity st;
	static const unsigned classid;
	static EntityRegister<Tank> entityRegister;

	Tank(Game *game);
	Tank(WarField *);
	void addRigidBody(WarSpace*)override;
	void control(WarField *, const input_t *inputs, double dt);
	const char *idname()const;
	const char *classname()const;
	virtual void control(const input_t *inputs, double dt);
	virtual void anim(double dt);
	void postframe();
	void draw(wardraw_t *);
	void drawtra(wardraw_t *);
//	virtual void bullethit(const Bullet *);
//	int getrot(warf_t*, double (*)[16]);
	int takedamage(double damage, int hitpart);
	void gib_draw(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata*, void *pv);
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal); // return nonzero on hit
	void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	double getHitRadius()const;
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

	static double defaultMass;
	static double topSpeed;
	static double backSpeed;
	static double mainGunCooldown;
	static double mainGunMuzzleSpeed;
	static double mainGunDamage;
	static HitBoxList hitboxes;

	HitBoxList &getHitBoxes()const{return hitboxes;}
	short bbodyGroup()const override{return 1<<2;}
	short bbodyMask()const override{return 1<<2;}

	bool buildBody();
	void init();
	int shootcannon(double dt);
	int tryshoot(double dt);
	void find_enemy_logic();
	void vehicle_drive(double dt, Vec3d *points, int npoints);
	int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal, const HitBox *, int);
	Vec3d tankMuzzlePos(Vec3d *nh = NULL)const;
};

#endif
