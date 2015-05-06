/** \file
 * \brief RoundAstrobj class definition. */
#ifndef ROUNDASTROBJ_H
#define ROUNDASTROBJ_H

#include "stellar_file.h"
#ifndef DEDICATED
#include "draw/ring-draw.h"
#endif
#include <squirrel.h>


/// \brief Astrobj with solid (roughly) round shape
///
/// Planets, natural satellites, asteroids, comets and stars all fall in
/// this category.  (Technically, stars have their own class Star.)
///
/// This class represents almost all solid celestial objects in space,
/// although it's not necessarily solid, since stars, gaseous planets and
/// oceanic planets have fluid surface.  The shape can be not round,
/// since asteroids and small satellites have significant ridges and valleys.
///
/// This class had been named TexSphere because it had been drawn as a textured
/// sphere.  The old name came to disagree with nature since it's not
/// necessarily a sphere nor textured.
class EXPORT RoundAstrobj : public Astrobj{
public:
	typedef Astrobj st;
	typedef gltestp::dstring String;
	typedef std::vector<String> StringList;
protected:
	String texname, cloudtexname;
	String ringtexname, ringbacktexname;
	unsigned int texlist, cloudtexlist; // should not really be here
	double ringmin, ringmax, ringthick;
	double atmodensity;
	double oblateness;
	float atmohor[4];
	float atmodawn[4];
	float albedo; ///< Albedo of this celestial body, (0,1)
	int ring;
	StringList vertexShaderName, fragmentShaderName;
#ifndef DEDICATED
	GLuint shader;
	bool shaderGiveup; ///< Flag whether compilation of shader has been given up, to prevent the compiler to try the same code in vain.
	/// Cloud sphere is separate geometry than the globe itself, so shaders and extra textures must be allocated separately.
#endif
	StringList cloudVertexShaderName, cloudFragmentShaderName;
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

	double terrainNoiseHeight;
	double terrainNoisePersistence;
	double terrainNoiseLODRange;
	int terrainNoiseLODs;
	int terrainNoiseOctaves;
	int terrainNoiseBaseLevel;
	bool terrainNoiseEnable;

	void updateInt(double dt);
public:
	/// drawTextureSphere flags
	enum DTS{
		DTS_ADD = 1<<0,
		DTS_NODETAIL = 1<<1,
		DTS_ALPHA = 1<<2,
		DTS_HEIGHTMAP = 1<<3,
		DTS_NORMALMAP = 1<<4,
		DTS_NOGLOBE = 1<<5,
		DTS_LIGHTING = 1<<6
	};

	struct Texture{
		String uniformname;
		String filename;
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
	typedef RoundAstrobj tt;
	RoundAstrobj(Game *game);
	RoundAstrobj(const char *name, CoordSys *cs);
	virtual ~RoundAstrobj();
	static const ClassRegister<RoundAstrobj> classRegister;
	virtual const Static &getStatic()const{return classRegister;}
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual bool readFile(StellarContext &, int argc, const char *argv[]);
	void anim(double dt)override;
	void draw(const Viewer *);
	void drawSolid(const Viewer *);
	virtual double atmoScatter(const Viewer &vw)const;
	virtual double getAtmosphericScaleHeight()const{return atmodensity;}
	virtual bool sunAtmosphere(const Viewer &vw)const;
	double getAmbientBrightness(const Viewer &vw)const;
	typedef std::vector<Texture>::const_iterator TextureIterator;
	TextureIterator beginTextures()const{return textures.begin();} ///< Iterates custom texture units for shaders.
	TextureIterator endTextures()const{return textures.end();} ///< End of iteration.
	Quatd cloudRotation()const{return rot.rotate(cloudPhase, 0, 1, 0);}
	static bool sq_define(HSQUIRRELVM v);
	double getTerrainHeight(const Vec3d &basepos)const;

	/// Terrain modifier that artificially affect randomly generated terrain.
	/// Currently, only circular flat area can be specified, but it could be various shapes.
	struct TerrainMod{
		Vec3d pos;
		double radius;
		double falloff;
	};

	/// A set of TerrainMods.  All modifications are applied to the same RoundAstrobj instance.
	typedef std::vector<TerrainMod> TerrainMods;

	/// A map type that can get TerrainMods from corresponding RoundAstrobj's SerializableId without RoundAstrobj instance.
	typedef std::map<SerializableId, TerrainMods> TerrainModMap;

	/// Global storage for storing TerrainMods for all RoundAstrobjs.  This is required for
	/// parallelize terrain generation by threads and prevent access to deleted RoundAstrobjs from these threads.
	/// If RoundAstrobjs are created and destroyed regularly, TerrainMods will leak memory.  Probably we could use
	/// reference counter scheme to delete finished objects.
	static TerrainModMap terrainModMap;

protected:
	virtual void updateAbsMag(double dt); ///< Update absolute magnitude of this celestial body by other light sources
	static double getTerrainHeightInt(const Vec3d &basepos, int octaves, double persistence, double aheight, const TerrainMods &tmods);
private:
	std::vector<Texture> textures;
#ifndef DEDICATED
	BITMAPINFO *heightmap[6];
#endif
	/// Member reference to global TerrainMods object.  The object is not deleted even if this RoundAstrobj is destroyed.
	TerrainMods &tmods;

	const PropertyMap &propertyMap()const;
	friend class DrawTextureSphere;
	friend class DrawTextureSpheroid;
	template<StringList RoundAstrobj::*memb> static SQInteger slgetter(HSQUIRRELVM v, const CoordSys *);
	template<StringList RoundAstrobj::*memb> static SQInteger slsetter(HSQUIRRELVM v, CoordSys *);
	friend class DrawTextureCubeEx;
};


/// Identical to Astrobj but ClassId
class EXPORT Satellite : public RoundAstrobj{
public:
	typedef RoundAstrobj st;
	static const ClassRegister<Satellite> classRegister;
	Satellite(Game *game) : st(game){}
	Satellite(const char *name, CoordSys *cs) : st(name, cs){}
};

#endif
