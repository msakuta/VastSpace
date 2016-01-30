/* \file
 * \brief Forward declaration header for non-drawing codes, namely RoundAstrobj
 */
#ifndef DRAWTEXTURESPHERE_FORWARD_H
#define DRAWTEXTURESPHERE_FORWARD_H
#include "dstring.h"
#include "serial.h"
#include <cpplib/vec3.h>

#ifdef _WIN32
#include <windows.h>
#endif
#ifndef DEDICATED
#include <GL/gl.h>
#endif

#include <vector>
#include <map>

class DrawTextureSphere;

/// DrawTextureSphere related structures and constants
namespace DTS{

/// drawTextureSphere flags
enum DrawTextureSphereFlags{
	DTS_ADD = 1<<0, ///< Deprecated; used with fixed pipelines for city lighting
	DTS_NODETAIL = 1<<1,
	DTS_ALPHA = 1<<2,
	DTS_HEIGHTMAP = 1<<3,
	DTS_NORMALMAP = 1<<4,
	DTS_NORMALIZE = 1<<5
};

struct Texture{
	gltestp::dstring uniformname;
	gltestp::dstring filename;
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
		cloudSync(false), flags(0){}
};

typedef std::vector<Texture> TextureList;

/// Structure to share terrain noise parameters among DrawTextureCubeEx and its users.
/// Since the number of parameters have increased, it's easier to handle all the parameters
/// in a structure.
struct TerrainNoise{
	bool enable;
	double height; ///< Noise's height scale
	double mapHeight; ///< Texture height map's maximum height
	double persistence;
	double lodRange;
	int lods;
	int octaves;
	int baseLevel;
	int zBufLODs;

	TerrainNoise() :
		enable(false),
		height(1000.),
		mapHeight(1000.),
		persistence(0.65),
		lodRange(3.),
		lods(3),
		octaves(7),
		baseLevel(0),
		zBufLODs(3){
	}
};

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

}
#endif
