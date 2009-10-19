#ifndef ENTITY_H
#define ENTITY_H
#include "war.h"
#include <cpplib/vec3.h>
#include <cpplib/quat.h>

class Entity{
public:
	Entity() : pos(vec3_000), velo(vec3_000), rot(quat_u), omg(vec3_000){}
	virtual const char *idname()const;
	virtual const char *classname()const;

	Entity *next;
	Vec3d pos;
	Vec3d velo;
	Vec3d omg;
	Quatd rot; /* rotation expressed in quaternion */
	double mass; /* [kg] */
	double moi;  /* moment of inertia, [kg m^2] should be a tensor */
	double turrety, barrelp;
	double desired[2];
	double health;
	double cooldown, cooldown2;
	Entity *selectnext; /* selection list */
	Entity *enemy;
	int race;
	int shoots, shoots2, kills, deaths;
	input_t inputs;
	char active;
	char weapon;
};


#endif
