#ifndef ENTITY_H
#define ENTITY_H
#include "war.h"
#include <cpplib/vec3.h>
#include <cpplib/quat.h>

class Bullet;
class Warpable;
class Entity{
public:
	Entity(WarField *aw = NULL);
	static Entity *create(const char *cname);
	virtual const char *idname()const;
	virtual const char *classname()const;
	virtual double maxhealth()const;
	virtual void anim(double dt);
	virtual void postframe(); // gives an opportunity to clear pointers to objects being destroyed.
	virtual void control(input_t *inputs, double dt) = 0;
	virtual unsigned analog_mask() = 0;
	virtual void draw(wardraw_t *) = 0;
	virtual void drawtra(wardraw_t *) = 0;
	virtual double hitradius() = 0;
	virtual void bullethit(const Bullet *);
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal); // return nonzero on hit
	virtual int takedamage(double damage, int hitpart); /* return 0 on death */
	virtual int popupMenu(char ***const, int **keys, char ***cmds, int *num);
	virtual Warpable *toWarpable();

	void transform(Mat4d &mat){
		mat = Mat4d(mat4_u).translatein(pos) * rot.tomat4();
	}
	void transit_cs(CoordSys *destcs); // transit to a CoordSys from another, keeping absolute position and velocity.

	Vec3d pos;
	Vec3d velo;
	Vec3d omg;
	Quatd rot; /* rotation expressed in quaternion */
	double mass; /* [kg] */
	double moi;  /* moment of inertia, [kg m^2] should be a tensor */
//	double turrety, barrelp;
//	double desired[2];
	double health;
//	double cooldown, cooldown2;
	Entity *next;
	Entity *selectnext; /* selection list */
	Entity *enemy;
	int race;
//	int shoots, shoots2, kills, deaths;
	input_t inputs;
	WarField *w; // belonging WarField, NULL means being bestroyed. Assigning another WarField marks it to transit to new CoordSys.
//	char weapon;

protected:
	void init();
};

struct GLwindowState;
void entity_popup(Entity *pt, GLwindowState &ws, int selectchain);

#endif
