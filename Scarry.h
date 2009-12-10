#ifndef SCARRY_H
#define SCARRY_H
#include "beamer.h"

class Scarry : public Warpable{
public:
	typedef Warpable st;

	Scarry(WarField *w) : st(w){init();}
	virtual const char *idname()const;
	virtual const char *classname()const;
	virtual double maxhealth()const;
	virtual double hitradius();
	virtual double maxenergy()const;
	virtual void anim(double dt);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual int tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn);
	virtual const maneuve &getManeuve()const;

protected:
	static const maneuve mymn;
	static hitbox hitboxes[];
	static const int nhitboxes;
};

#if 0
/* Active Radar or Hyperspace Sonar.
   AR costs less energy but has limitation in speed (light), while
  HS propagates on hyperspace field where limitation of speed do not exist.
*/
struct hypersonar{
	avec3_t pos;
	double life;
	double rad;
	double speed;
	struct coordsys *cs;
	struct hypersonar *next;
	int type; /* 0 - Active Radar, 1 - Hyperspace Sonar. */
} *g_hsonar;
#endif


#endif
