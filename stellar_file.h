#ifndef STELLAR_FILE_H
#define STELLAR_FILE_H
/* function to load stellar information file. */

#include "coordsys.h"
#include "astro.h"
#include <stdio.h>

#define TELEPORT_TP     1
#define TELEPORT_WARP   2
#define TELEPORT_PORTAL 4 /* should really have different data structure. */

extern struct teleport{
	CoordSys *cs;
	char *name;
	int flags;
	avec3_t pos;
} *tplist;
extern int ntplist;

#define OCS_SHOWORBIT 1

class OrbitCS : public CoordSys{
	double orbit_rad;
	Astrobj *orbit_home;
	Quatd orbit_axis;
	double orbit_phase;
	double eccentricity; /* orbital element */
	int flags2;
public:
	OrbitCS(const char *path, CoordSys *root);
	void anim(double dt);
	void draw(const Viewer *);
};

class Lagrange1CS : public CoordSys{
	Astrobj *objs[2];
public:
	Lagrange1CS(const char *path, CoordSys *root);
	void anim(double dt);
};

class TexSphere : public Astrobj{
	const char *texname;
	unsigned int texlist;
	double ringmin, ringmax, ringthick;
	double atmodensity;
	float atmohor[4];
	float atmodawn[4];
	int ring;
};

Astrobj *new_astrobj(const char *name, CoordSys *cs);
Astrobj *satellite_new(const char *name, CoordSys *cs);
Astrobj *texsphere_new(const char *name, CoordSys *cs);
Astrobj *blackhole_new(const char *name, CoordSys *cs);
Astrobj *add_astrobj(Astrobj *a);
Astrobj *findastrobj(const char *name);

int StellarFileLoad(const char *fname, CoordSys *);


void sun_draw(const struct astrobj *, const struct viewer *, int relative);
void saturn_draw(const struct astrobj *a, const struct viewer *vw, int relative);
void blackhole_draw(const struct astrobj *, const struct viewer*, int relative);


struct StellarContext{
	const char *fname;
	CoordSys *root;
	FILE *fp;
	char *buf;
	long line;
	struct varlist *vl;
};


#endif
