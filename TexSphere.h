#ifndef TEXSPHERE_H
#define TEXSPHERE_H
/* TexSphere class definition. */

#include "stellar_file.h"


// Astrobj drawn as a textured sphere
class TexSphere : public Astrobj{
	const char *texname, *ringtexname;
	unsigned int texlist; // should not really be here
	double ringmin, ringmax, ringthick;
	double atmodensity;
	double oblateness;
	float atmohor[4];
	float atmodawn[4];
	int ring;

	// OpenGL texture units
	unsigned ringTex, ringShadowTex;
public:
	typedef Astrobj st;
	TexSphere(){}
	TexSphere(const char *name, CoordSys *cs);
	virtual ~TexSphere();
	const char *classname()const;
	static const unsigned classid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	bool readFile(StellarContext &, int argc, char *argv[]);
	void draw(const Viewer *);
	virtual double atmoScatter(const Viewer &vw)const;
	virtual bool sunAtmosphere(const Viewer &vw)const;
};

#endif
