#ifndef ASTRO_H
#define ASTRO_H
#include "coordsys.h"
#include <clib/aquat.h>
#include <clib/avec3.h>
#include <clib/colseq/color.h>

/* Codes relating astronautics. */

// OrbitCS::flags2
#define OCS_SHOWORBIT 1

// astrobj::flags shares with coordsys::flags, so use bits only over 16
#define AO_DELETE         0x00000002 /* marked as to be deleted, share with coordsys */
#define AO_PLANET         0x00010000
#define AO_GRAVITY        0x00020000
#define AO_ALWAYSSHOWNAME 0x00040000
#define AO_SPHEREHIT      0x00080000
#define AO_BLACKHOLE      0x00100000 /* Blackhole */
#define AO_STARGLOW       0x00200000 /* Star that emits light */
#define AO_ELLIPTICAL     0x00800000 /* Orbits close to circle are treated so for speed. */
#define AO_ATMOSPHERE     0x01000000 /* Atmosphere is existing. */
#define AO_ASTROBJ        0x80000000 /* Distinguish coordsys with astrobj by this flag. */

typedef void (*astrobj_draw_proc)(struct astrobj *, const struct viewer *);

struct StellarContext;

// CoordSys of orbital motion
class OrbitCS : public CoordSys{
public:
	double orbit_rad;
	Astrobj *orbit_home;
	Quatd orbit_axis;
	double orbit_phase;
	double eccentricity; /* orbital element */
	int flags2;
public:
	typedef CoordSys st;
	OrbitCS(const char *path, CoordSys *root);
	virtual const char *classname()const;
	virtual void anim(double dt);
	virtual bool readFileStart(StellarContext &);
	virtual bool readFile(StellarContext &, int argc, char *argv[]);
	virtual bool readFileEnd(StellarContext &);
	virtual OrbitCS *toOrbitCS();

private:
	int enable;
	double inclination, loan, aop;
};

// Astronomical object. Usually orbits some other object.
class Astrobj : public OrbitCS{
public:
	typedef OrbitCS st;

	double mass;
	float absmag; /* Absolute Magnitude */
	COLOR32 basecolor; /* rough approximation of apparent color */

	Astrobj(const char *name, CoordSys *cs);
//	void init(const char *name, CoordSys *cs);

	virtual const char *classname()const;
	void planet_anim(double dt);
	virtual bool readFile(StellarContext &, int argc, char *argv[]);
	virtual Astrobj *toAstrobj(){ return this; }
	virtual double atmoScatter(const Viewer &vw)const{ return 0.; }
	virtual bool sunAtmosphere(const Viewer &vw)const{ return false; }

	Astrobj *findBrightest()const;
};

class Universe : public CoordSys{
public:
	typedef CoordSys st;
	Universe(){flags = CS_ISOLATED | CS_EXTENT;}
	const char *classname()const;
	void draw(const Viewer *);
};

#endif
