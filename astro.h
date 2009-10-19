#ifndef ASTRO_H
#define ASTRO_H
#include "coordsys.h"
#include <clib/aquat.h>
#include <clib/avec3.h>
#include <clib/colseq/color.h>

/* Codes relating astronautics.
  This header describes data structures on how astronomic objects behave.
  This header does not care about graphical aspects of objects.
  Refer astrodraw.h for this purpose. */

/* astrobj::flags shares with coordsys::flags, so use bits only over 16*/
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

class Astrobj : public CoordSys{
public:

	double mass;
	Quatd orbit_axis;
	double orbit_radius;
	Astrobj *orbit_home;
	double orbit_phase; /* store phase information for precise position calculation */
	double eccentricity; /* orbital element */
	float absmag; /* Absolute Magnitude */
	COLOR32 basecolor; /* rough approximation of apparent color */

	Astrobj(const char *name, CoordSys *cs);
//	void init(const char *name, CoordSys *cs);

	virtual const char *classname()const;
	void planet_anim(double dt);
	bool readFile(StellarContext &, int argc, char *argv[]);
};


#endif
