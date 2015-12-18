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
	DTS_ADD = 1<<0,
	DTS_NODETAIL = 1<<1,
	DTS_ALPHA = 1<<2,
	DTS_HEIGHTMAP = 1<<3,
	DTS_NORMALMAP = 1<<4,
	DTS_NOGLOBE = 1<<5,
	DTS_LIGHTING = 1<<6,
	DTS_NORMALIZE = 1<<7
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
