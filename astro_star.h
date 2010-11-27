#ifndef ASTRO_STAR_H
#define ASTRO_STAR_H
#include "astro.h"

#define AO_DRAWAIRCOLONA 0x1000
class Star : public Astrobj{
protected:
	int aircolona; /* a flag indicating the colona is already drawn by the time some nearer planet's air is drawn. */
public:
	typedef Astrobj st;
	enum SpectralType{
		Unknown, O, B, A, F, G, K, M, Num_SpectralType
	} spect;
	Star(){}
	Star(const char *name, CoordSys *cs);
	virtual const char *classname()const;
	static const ClassRegister<Star> classRegister;
	virtual void predraw(const Viewer*);
	virtual void draw(const Viewer*);
	virtual bool readFile(StellarContext &sc, int argc, char *argv[]);
	virtual bool readFileEnd(StellarContext &sc);

	static SpectralType nameToSpectral(const char *);
	static const char *spectralToName(SpectralType);
	static Vec3<float> spectralHSB(SpectralType);
	static Vec3<float> spectralRGB(SpectralType);
};

#endif

