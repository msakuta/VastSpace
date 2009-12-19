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

// Base class for Lagrange points
class LagrangeCS : public CoordSys{
public:
	typedef CoordSys st;
	LagrangeCS(){}
	LagrangeCS(const char *path, CoordSys *root);
	bool readFile(StellarContext &, int argc, char *argv[]);
protected:
	Astrobj *objs[2];
};

// L1 point of two celestial bodies
class Lagrange1CS : public LagrangeCS{
public:
	typedef LagrangeCS st;
	Lagrange1CS(){}
	Lagrange1CS(const char *path, CoordSys *root) : st(path, root){}
	const char *classname()const;
	static const unsigned classid;
	void anim(double dt);
};

// L2 point of two celestial bodies
class Lagrange2CS : public LagrangeCS{
public:
	typedef LagrangeCS st;
	Lagrange2CS(){}
	Lagrange2CS(const char *path, CoordSys *root) : st(path, root){}
	const char *classname()const;
	static const unsigned classid;
	void anim(double dt);
};

// Astrobj drawn as a textured sphere
class TexSphere : public Astrobj{
	const char *texname;
	unsigned int texlist; // should not really be here
	double ringmin, ringmax, ringthick;
	double atmodensity;
	float atmohor[4];
	float atmodawn[4];
	int ring;
public:
	typedef Astrobj st;
	TexSphere(){}
	TexSphere(const char *name, CoordSys *cs);
	const char *classname()const;
	static const unsigned classid;
	bool readFile(StellarContext &, int argc, char *argv[]);
	void draw(const Viewer *);
	virtual double atmoScatter(const Viewer &vw)const;
	virtual bool sunAtmosphere(const Viewer &vw)const;
};

Astrobj *new_astrobj(const char *name, CoordSys *cs);
Astrobj *satellite_new(const char *name, CoordSys *cs);
Astrobj *texsphere_new(const char *name, CoordSys *cs);
Astrobj *blackhole_new(const char *name, CoordSys *cs);
Astrobj *add_astrobj(Astrobj *a);

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
