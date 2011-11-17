#ifndef STELLAR_FILE_H
#define STELLAR_FILE_H
/** \file
 * \breif Functions to load Stellar Structure Definition file. */

#include "CoordSys.h"
#include "astro.h"
#include <stdio.h>
#include <squirrel.h>

#define TELEPORT_TP     1
#define TELEPORT_WARP   2
#define TELEPORT_PORTAL 4 /* should really have different data structure. */

struct teleport/* : public Serializable*/{
/*	typedef Serializable st;
	virtual const char *classname()const;
	static const unsigned classid;*/
	teleport(CoordSys *cs, const char *name, int flags, const Vec3d &pos);
	~teleport();
	void init(CoordSys *cs, const char *name, int flags, const Vec3d &pos);

	void serialize(SerializeContext &sc);
	void unserialize(UnserializeContext &sc);

	CoordSys *cs;
	char *name;
	int flags;
	Vec3d pos;
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

class StellarStructureScanner;

/// Context object in the process of interpreting a stellar file.
struct StellarContext{
	const char *fname;
	CoordSys *root;
	FILE *fp;
	cpplib::dstring buf;
	long line;
	struct varlist *vl;
	HSQUIRRELVM v;
	StellarStructureScanner *scanner;
};


#endif
