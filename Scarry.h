#ifndef SCARRY_H
#define SCARRY_H
#include "Warpable.h"
#include "arms.h"

#define SCARRY_BUILDQUESIZE 8
#define SCARRY_SCALE .0010
#define SCARRY_BAYCOOL 2.

class Scarry : public Warpable{
public:
	typedef Warpable st;

	Scarry(WarField *w);
	virtual const char *idname()const;
	virtual const char *classname()const;
	virtual double maxhealth()const;
	virtual double hitradius();
	virtual double maxenergy()const;
	virtual void anim(double dt);
	virtual Props props()const;
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual int tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn);
	virtual int armsCount()const;
	virtual const ArmBase *armsGet(int i)const;
	virtual const maneuve &getManeuve()const;

protected:
	ArmBase **turrets;
	static hardpoint_static *hardpoints;
	static int nhardpoints;

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
