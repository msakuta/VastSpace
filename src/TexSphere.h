/** \file
 * \brief TexSphere class definition. */
#ifndef TEXSPHERE_H
#define TEXSPHERE_H

#include "stellar_file.h"
#ifndef DEDICATED
#include "draw/ring-draw.h"
#endif
#include <squirrel.h>


/// Astrobj drawn as a textured sphere
class TexSphere : public Astrobj{
protected:
	cpplib::dstring texname, cloudtexname;
	cpplib::dstring ringtexname, ringbacktexname;
	unsigned int texlist, cloudtexlist; // should not really be here
	double ringmin, ringmax, ringthick;
	double atmodensity;
	double oblateness;
	float atmohor[4];
	float atmodawn[4];
	int ring;
	std::vector<cpplib::dstring> vertexShaderName, fragmentShaderName;
#ifndef DEDICATED
	GLuint shader;
	bool shaderGiveup; ///< Flag whether compilation of shader has been given up, to prevent the compiler to try the same code in vain.
	/// Cloud sphere is separate geometry than the globe itself, so shaders and extra textures must be allocated separately.
#endif
	std::vector<cpplib::dstring> cloudVertexShaderName, cloudFragmentShaderName;
#ifndef DEDICATED
	GLuint cloudShader;
#endif
	bool cloudShaderGiveup; ///< Flag whether compilation of shader has been given up.
	double cloudHeight; ///< In kilometers
	double cloudPhase;

	Vec3d noisePos; ///< Position in noise space for ocean noise. Only in the client.

#ifndef DEDICATED
	/// OpenGL texture units
	AstroRing astroRing;
#endif

	void updateInt(double dt);
public:
	/// drawTextureSphere flags
	enum DTS{
		DTS_ADD = 1<<0,
		DTS_NODETAIL = 1<<1,
		DTS_ALPHA = 1<<2,
		DTS_NORMALMAP = 1<<3,
		DTS_NOGLOBE = 1<<4,
		DTS_LIGHTING = 1<<5
	};

	struct Texture{
		cpplib::dstring uniformname;
		cpplib::dstring filename;
#ifndef DEDICATED
		mutable GLuint list;
		mutable GLint shaderLoc;
#endif
		bool cloudSync;
		int flags; ///< drawTextureSphere flags
		Texture() :
#ifndef DEDICATED
			list(0), shaderLoc(-2),
#endif
			cloudSync(false), flags(false){}
	};
	typedef Astrobj st;
	typedef TexSphere tt;
	TexSphere(Game *game);
	TexSphere(const char *name, CoordSys *cs);
	virtual ~TexSphere();
	static const ClassRegister<TexSphere> classRegister;
	virtual const Static &getStatic()const{return classRegister;}
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual bool readFile(StellarContext &, int argc, const char *argv[]);
	void draw(const Viewer *);
	virtual double atmoScatter(const Viewer &vw)const;
	virtual double getAtmosphericScaleHeight()const{return atmodensity;}
	virtual bool sunAtmosphere(const Viewer &vw)const;
	double getAmbientBrightness(const Viewer &vw)const;
	typedef std::vector<Texture>::const_iterator TextureIterator;
	TextureIterator beginTextures()const{return textures.begin();} ///< Iterates custom texture units for shaders.
	TextureIterator endTextures()const{return textures.end();} ///< End of iteration.
	Quatd cloudRotation()const{return rot.rotate(cloudPhase, 0, 1, 0);}
	static bool sq_define(HSQUIRRELVM v);
private:
	std::vector<Texture> textures;
	const PropertyMap &propertyMap()const;
	friend class DrawTextureSphere;
	friend class DrawTextureSpheroid;
};


/// Identical to Astrobj but ClassId
class Satellite : public TexSphere{
public:
	typedef TexSphere st;
	static const ClassRegister<Satellite> classRegister;
	Satellite(Game *game) : st(game){}
	Satellite(const char *name, CoordSys *cs) : st(name, cs){}
};

#endif
