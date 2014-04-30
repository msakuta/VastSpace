#ifndef ASTRO_STAR_H
#define ASTRO_STAR_H
#include "astro.h"

class Star : public Astrobj{
protected:
	int aircorona; /* a flag indicating the corona is already drawn by the time some nearer planet's air is drawn. */
public:
	static const int AO_DRAWAIRCORONA = 0x1000;
	typedef Astrobj st;
	typedef Star tt;
	enum SpectralType{
		Unknown, O, B, A, F, G, K, M, Num_SpectralType
	} spect;
	float subspect; ///< Fractional sub-spectral type is permitted
	Star(Game *game) : st(game){}
	Star(const char *name, CoordSys *cs);
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const Static &getStatic()const{return classRegister;}
	static bool sq_define(HSQUIRRELVM);
	static const ClassRegister<Star> classRegister;
	virtual void predraw(const Viewer*);
	virtual void draw(const Viewer*);
	virtual bool readFile(StellarContext &sc, int argc, const char *argv[]);
	virtual bool readFileEnd(StellarContext &sc);
	PropertyMap &propertyMap()const override;

	double appmag(const Vec3d &pos, const CoordSys &cs)const; ///< Apparent magnitude

	static SpectralType nameToSpectral(const char *, float *subspect = NULL);
	static const char *spectralToName(SpectralType);
	static Vec3<float> spectralHSB(SpectralType);
	static Vec3<float> spectralRGB(SpectralType);
	static void drawsuncorona(Astrobj *a, const Viewer *vw);
protected:
	static SQInteger sqf_get(HSQUIRRELVM v);
};

#endif

