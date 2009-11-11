#ifndef ENTITY_H
#define ENTITY_H
#include "war.h"
#include <cpplib/vec3.h>
#include <cpplib/quat.h>

class Entity{
public:
	Entity(WarField *aw = NULL) : pos(vec3_000), velo(vec3_000), omg(vec3_000), rot(quat_u), mass(1e3), moi(1e1), health(100), enemy(NULL), w(aw), inputs(){
		if(aw)
			aw->addent(this);
	}
	static Entity *create(const char *cname);
	virtual const char *idname()const;
	virtual const char *classname()const;
	virtual void anim(double dt);
	virtual void control(input_t *inputs, double dt) = 0;
	virtual unsigned analog_mask() = 0;
	virtual void draw(wardraw_t *) = 0;
	virtual void drawtra(wardraw_t *) = 0;

	void transform(Mat4d &mat){
		mat = Mat4d(mat4_u).translatein(pos) * rot.tomat4();
	}

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
	WarField *w;
//	char weapon;
};


#endif
