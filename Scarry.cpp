#include "scarry.h"
#include "judge.h"
extern "C"{
#include <clib/mathdef.h>
}

/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289


#define SCARRY_MAX_HEALTH 200000
#define SCARRY_MAX_SPEED .03
#define SCARRY_ACCELERATE .01
#define SCARRY_MAX_ANGLESPEED (.005 * M_PI)
#define SCARRY_ANGLEACCEL (.002 * M_PI)

const char *Scarry::idname()const{return "scarry";}
const char *Scarry::classname()const{return "Space Carrier";}

double Scarry::maxhealth()const{
	return 200000;
}

double Scarry::hitradius(){
	return 1.;
}

double Scarry::maxenergy()const{
	return 5e8;
}

void Scarry::anim(double dt){
	st::anim(dt);
}

Entity::Props Scarry::props()const{
	Props ret = st::props();
	ret.push_back(cpplib::dstring("?: "));
	return ret;
}

int Scarry::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	Scarry *p = this;
	double sc[3];
	double best = dt, retf;
	int reti = 0, i, n;
#if 0
	if(0 < p->shieldAmount){
		if(jHitSpherePos(pos, BEAMER_SHIELDRAD + rad, src, dir, dt, ret, retp))
			return 1000; /* something quite unlikely to reach */
	}
#endif
	for(n = 0; n < nhitboxes; n++){
		Vec3d org;
		Quatd rot;
		org = this->rot.itrans(hitboxes[n].org) + this->pos;
		rot = this->rot * hitboxes[n].rot;
		for(i = 0; i < 3; i++)
			sc[i] = hitboxes[n].sc[i] + rad;
		if((jHitBox(org, sc, rot, src, dir, 0., best, &retf, retp, retn)) && (retf < best)){
			best = retf;
			if(ret) *ret = retf;
			reti = i + 1;
		}
	}
	return reti;
}


const Scarry::maneuve Scarry::mymn = {
	SCARRY_ACCELERATE, /* double accel; */
	SCARRY_MAX_SPEED, /* double maxspeed; */
	SCARRY_ANGLEACCEL, /* double angleaccel; */
	SCARRY_MAX_ANGLESPEED, /* double maxanglespeed; */
	5e8, /* double capacity; */
	1e6, /* double capacitor_gen; */
};

const Warpable::maneuve &Scarry::getManeuve()const{return mymn;}

struct hitbox Scarry::hitboxes[] = {
/*	hitbox(Vec3d(0., 0., -.02), Quatd(0,0,0,1), Vec3d(.015, .015, .075)),
	hitbox(Vec3d(.025, -.015, .02), Quatd(0,0, -SIN15, COS15), Vec3d(.0075, .002, .02)),
	hitbox(Vec3d(-.025, -.015, .02), Quatd(0,0, SIN15, COS15), Vec3d(.0075, .002, .02)),
	hitbox(Vec3d(.0, .03, .0325), Quatd(0,0,0,1), Vec3d(.002, .008, .010)),*/
	hitbox(Vec3d(0., 0., .050), Quatd(0,0,0,1), Vec3d(.3, .2, .500)),
/*	{{0.105, 0., .0}, {0,0,0,1}, {.100, .2, .050}},*/
	hitbox(Vec3d(-.105, 0., -.700), Quatd(0,0,0,1), Vec3d(.095, .020, .250)),
	hitbox(Vec3d( .100, 0., -.640), Quatd(0,0,0,1), Vec3d(.100, .060, .180)),
	hitbox(Vec3d( .100, .0, -.600), Quatd(0,0,0,1), Vec3d(.040, .260, .040)),
};
const int Scarry::nhitboxes = numof(Scarry::hitboxes);

