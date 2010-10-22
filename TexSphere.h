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
	std::vector<cpplib::dstring> vertexShaderName, fragmentShaderName;
	GLuint shader;
	bool shaderGiveup; ///< Flag whether compilation of shader has been given up, to prevent the compiler to try the same code in vain.
	/// Cloud sphere is separate geometry than the globe itself, so shaders and extra textures must be allocated separately.
	std::vector<cpplib::dstring> cloudVertexShaderName, cloudFragmentShaderName;
	GLuint cloudShader;
	bool cloudShaderGiveup; ///< Flag whether compilation of shader has been given up.
	double cloudHeight; ///< In kilometers
	double cloudPhase;

	/// OpenGL texture units
	AstroRing astroRing;
public:
	struct Texture{
		cpplib::dstring uniformname;
		cpplib::dstring filename;
		mutable GLuint list;
		bool cloudSync;
	};
	typedef Astrobj st;
	TexSphere();
	TexSphere(const char *name, CoordSys *cs);
	virtual ~TexSphere();
	const char *classname()const;
	static const ClassRegister<TexSphere> classRegister;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	bool readFile(StellarContext &, int argc, char *argv[]);
	virtual void anim(double dt);
	void draw(const Viewer *);
	virtual double atmoScatter(const Viewer &vw)const;
	virtual bool sunAtmosphere(const Viewer &vw)const;
	typedef std::vector<Texture>::const_iterator TextureIterator;
	TextureIterator beginTextures()const{return textures.begin();} ///< Iterates custom texture units for shaders.
	TextureIterator endTextures()const{return textures.end();} ///< End of iteration.
	Quatd cloudRotation()const{return rot.rotate(cloudPhase, 0, 1, 0);}
private:
	std::vector<Texture> textures;
};

/// Identical to Astrobj but ClassId
class Satellite : public Astrobj{
public:
	static const ClassRegister<Satellite> classRegister;
	Satellite(){}
	Satellite(const char *name, CoordSys *cs) : Astrobj(name, cs){}
};

#endif
