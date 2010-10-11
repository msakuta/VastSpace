#ifndef ASTRO_STAR_H
#define ASTRO_STAR_H
#include "astro.h"

#define AO_DRAWAIRCOLONA 0x1000
class Star : public Astrobj{
	int aircolona; /* a flag indicating the colona is already drawn by the time some nearer planet's air is drawn. */
public:
	typedef Astrobj st;
	Star(){}
	Star(const char *name, CoordSys *cs);
	virtual const char *classname()const;
	static const ClassRegister<Star> classRegister;
	virtual void predraw(const Viewer*);
	virtual void draw(const Viewer*);
};

#endif

