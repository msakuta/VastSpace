#ifndef TANK_H
#define TANK_H
#include "war.h"
#include "Entity.h"
#include "btadapt.h"
extern "C"{
#include <clib/c.h>
}

struct hitbox;

class Tank : public Entity{
public:
	typedef Entity st;
	static const unsigned classid;
	static EntityRegister<Tank> entityRegister;

	Tank(Game *game);
	Tank(WarField *);
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
	double hitradius()const;
	bool isTargettable()const;

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
protected:
	int shootcannon(double phi, double theta, double variance, Vec3d &mpos, int *mposa);
	int tryshoot(int rot, Vec3d &epos, double phi0, double variance, Vec3d *mpos, int *mposa);
	void find_enemy_logic();
	void vehicle_drive(double dt, Vec3d *points, int npoints);
	int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal, const hitbox *, int);
};

#endif
