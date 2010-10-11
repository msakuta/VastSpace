/** \file
 * \brief TexSphere class definition. */
#ifndef TEXSPHERE_H
#define TEXSPHERE_H

#include "stellar_file.h"
#include "draw/ring-draw.h"


/// Astrobj drawn as a textured sphere
class TexSphere : public Astrobj{
	const char *texname, *cloudtexname, *ringtexname, *ringbacktexname;
	unsigned int texlist, cloudtexlist; // should not really be here
	double ringmin, ringmax, ringthick;
	double atmodensity;
	double oblateness;
	float atmohor[4];
	float atmodawn[4];
	int ring;

	// OpenGL texture units
	AstroRing astroRing;
public:
	typedef Astrobj st;
	TexSphere(){}
	TexSphere(const char *name, CoordSys *cs);
	virtual ~TexSphere();
	const char *classname()const;
	static const ClassRegister<TexSphere> classRegister;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	bool readFile(StellarContext &, int argc, char *argv[]);
	void draw(const Viewer *);
	virtual double atmoScatter(const Viewer &vw)const;
	virtual bool sunAtmosphere(const Viewer &vw)const;
};

/// Identical to Astrobj but ClassId
class Satellite : public Astrobj{
public:
	static const ClassRegister<Satellite> classRegister;
	Satellite(){}
	Satellite(const char *name, CoordSys *cs) : Astrobj(name, cs){}
};

#endif
