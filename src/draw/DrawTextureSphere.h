/** \file
 * \brief Definition of DrawTexSphere and DrawTextureSpheroid classes.
 */
#ifndef DRAWTEXTURESPHERE_H
#define DRAWTEXTURESPHERE_H
#include "RoundAstrobj.h"
#include "CoordSys-find.h"
#include "judge.h"

#include <functional>


/// \brief A class that draw textured sphere.
///
/// Arguments on how to draw the sphere are passed via member functions.
/// The caller must invoke draw() method to get it working.
class DrawTextureSphere{
protected:
	Astrobj *a;
	const Viewer *vw;
	const Vec3d sunpos;
	const Vec3d apos;
	std::vector<FindBrightestAstrobj::ResultSet> lightingStars;
	Vec4f m_mat_diffuse;
	Vec4f m_mat_ambient;
	GLuint *ptexlist;
	Mat4d m_texmat;
	const char *m_texname;
	const std::vector<RoundAstrobj::Texture> *m_textures;
	double m_rad;
	int m_flags;
	GLuint m_shader;
	bool m_drawint;
	int m_ncuts;
	int m_nfinecuts;
	int m_nffinecuts;
	AstroRing *m_ring;
	double m_ringmin, m_ringmax;
	Quatd m_cloudRotation;
	Vec3f m_noisePos;
	void useShader();
	void stopShader();
public:
	typedef DrawTextureSphere tt;
	tt(Astrobj *a, const Viewer *vw, const Vec3d &sunpos) : a(a), vw(vw), sunpos(sunpos)
		, apos(vw->cs->tocs(a->pos, a->parent))
		, m_mat_diffuse(1,1,1,1), m_mat_ambient(.5, .5, .5, 1.)
		, ptexlist(NULL)
		, m_texmat(mat4_u)
		, m_textures(NULL)
		, m_rad(0), m_flags(0), m_shader(0), m_drawint(false)
		, m_ncuts(32), m_nfinecuts(256), m_nffinecuts(2048)
		, m_ring(NULL)
		, m_ringmin(0.), m_ringmax(0.)
		, m_cloudRotation(quat_u)
		, m_noisePos(0,0,0){}
	tt &astro(RoundAstrobj *a){this->a = a; return *this;}
	tt &viewer(const Viewer *vw){this->vw = vw; return *this;}
	tt &mat_diffuse(const Vec4f &a){this->m_mat_diffuse = a; return *this;}
	tt &mat_ambient(const Vec4f &a){this->m_mat_ambient = a; return *this;}
	tt &texlist(GLuint *a){ptexlist = a; return *this;}
	tt &texmat(const Mat4d &a){m_texmat = a; return *this;}
	tt &texname(const char *a){m_texname = a; return *this;}
	tt &textures(const std::vector<RoundAstrobj::Texture> &atextures){m_textures = &atextures; return *this;}
	tt &rad(double a){m_rad = a; return *this;}
	tt &flags(int a){m_flags = a; return *this;}
	tt &shader(GLuint a){m_shader = a; return *this;}
	tt &drawint(bool a){m_drawint = a; return *this;}
	tt &ncuts(int a){m_ncuts = a; m_nfinecuts = a * 8; m_nffinecuts = a * 64; return *this;}
	tt &ring(AstroRing *a){m_ring = a; return *this;}
	tt &ringRange(double ringmin, double ringmax){m_ringmin = ringmin; m_ringmax = ringmax; return *this;}
	tt &cloudRotation(const Quatd &acloudrot){m_cloudRotation = acloudrot; return *this;}
	tt &noisePos(const Vec3f &apos){m_noisePos = apos; return *this;}
	tt &lightingStar(const std::vector<FindBrightestAstrobj::ResultSet> &a){lightingStars = a; return *this;}
	virtual bool draw();

	static const GLenum cubetarget[6]; ///< The target OpenGL texture units for each direction of the cube.
	static const Quatd cubedirs[6]; ///< The orientations corresponding to cubetarget. Note that rolling matters.

	/// \brief Converts Mercator projection texture to cube map.
	///
	/// Cube has 6 faces, so returning textures are also 6. These returned textures are cached in cache folder for
	/// later execution.
	///
	/// \param name The name of cached textures.
	/// \param raw The source texture mapped in Mercator projection.
	/// \param cacheload The buffer for returned textures. The buffer must have at least 6 elements.
	/// \param flags DrawTextureSphere flags.
	/// \param heightmap Returning buffer for heightmaps
	static GLuint ProjectSphereCube(const char *name, const BITMAPINFO *raw, BITMAPINFO *cacheload[6], unsigned flags,
		BITMAPINFO *heightmap[6]);

	/// \brief Reads an Mercator projection texture in a JPEG file and pass it to ProjectSphereCube().
	/// \param fname The file name of JPEG file.
	/// \param flags DrawTextureSphere flags.
	/// \param heightmap Returning buffer for heightmaps
	GLuint ProjectSphereCubeImage(const char *fname, int flags)const;

protected:
	/// \brief Sets up lights from lightingStars
	void setupLight();
	/// <summary> Draw simple shape if apparent size of this celestial body is so small </summary>
	/// <returns> True if it was small enough to draw simplified version; derived classes should not draw further if so </returns>
	bool drawSimple();
};

/// \brief A class that draw textured sphere.
///
/// Arguments on how to draw the sphere are passed via member functions.
/// The caller must invoke draw() method to get it working.
class DrawTextureSpheroid : public DrawTextureSphere{
public:
	typedef DrawTextureSphere st;
	typedef DrawTextureSpheroid tt;
	double m_oblateness;
	tt(RoundAstrobj *a, const Viewer *vw, const Vec3d &sunpos) : st(a, vw, sunpos)
		, m_oblateness(0.){}
	tt &oblateness(double a){m_oblateness = a; return *this;}
	virtual bool draw();
};


class DrawTextureCubeEx : public DrawTextureSphere{
public:
	typedef DrawTextureSphere st;
	typedef DrawTextureCubeEx tt;

	tt(RoundAstrobj *a, const Viewer *vw, const Vec3d &sunpos) : st(a, vw, sunpos),
		m_lods(3), m_noiseHeight(1.), m_noisePersistence(0.65), m_noiseOctaves(6){}
	bool draw()override;
	tt &noiseLODRange(double v){m_noiseLODRange = v; return *this;}
	tt &noiseLODs(int v){m_lods = MIN(maxLods, v); return *this;}
	tt &noiseHeight(double v){m_noiseHeight = v; return *this;}
	tt &noisePersistence(double v){m_noisePersistence = v; return *this;}
	tt &noiseOctaves(int v){m_noiseOctaves = v; return *this;}
	tt &noiseBaseLevel(int v){m_noiseBaseLevel = v; return *this;}

	static const int maxLods = 5;
	static const int lodPatchSize = 4;
	static const int lodPatchSize2 = lodPatchSize * lodPatchSize;
	static const int maxPatchRatio = 4;

	static int getPatchSize(int lod = 0);
	static int getPatchSize2(int lod = 0){
		int v = getPatchSize(lod);
		return v * v;
	}

	static int getDivision(int lod = 0);

protected:
	struct BufferData;

	struct WorkerThread;
	typedef std::vector<DrawTextureCubeEx::WorkerThread> WorkerThreads;
	static WorkerThreads threads;

	struct TempVertex;
	typedef std::map<DrawTextureCubeEx::TempVertex, GLuint> VertexIndMap;

	struct SubBufferSetBase{
		GLuint pos; ///< Position vector buffer.
		GLuint nrm; ///< Normal vector buffer.
		GLuint tex; ///< Texture coordinates buffer.
		GLuint ind; ///< Index buffer into all three buffers above.
		unsigned count; ///< Count of vertices ind member contains.
	};

	struct SubBufferSet : SubBufferSetBase{
		WorkerThread *t;
		BufferData *pbd;
		bool ready; ///< This need not be std::atomic<bool>.

		SubBufferSet() : t(NULL), ready(false){}

		/// Data member to calculate base and count (should be protected?)
		GLuint subPatchIdx[maxPatchRatio][maxPatchRatio];
		GLuint subPatchCount[maxPatchRatio][maxPatchRatio];
	};

	/// Tuple indicating indices of direction, x and y
	typedef std::tuple<int,int,int> SubKey;

	typedef std::map<SubKey, SubBufferSet> SubBufs;

	/// \brief A set of vertex buffer object indices.
	struct BufferSet : SubBufferSetBase{

		/// Get base index of given LOD and direction ID
		unsigned getBase(int direction, int patch = 0){
			if(direction == 0 && patch == 0)
				return 0;
			else if(patch == 0)
				return baseIdx[direction-1][lodPatchSize2-1];
			else
				return baseIdx[direction][patch-1];
		}

		/// Get count of vertices in given LOD and direction ID
		unsigned getCount(int direction){
			assert(direction < numof(cubedirs));
			return baseIdx[direction][lodPatchSize2-1] - getBase(direction);
		}

		unsigned getPatchCount(int direction, int patch){
			assert(direction < numof(cubedirs) && patch < lodPatchSize2);
			return baseIdx[direction][patch] - getBase(direction, patch);
		}

		/// Data member to calculate base and count (should be protected?)
		GLuint baseIdx[numof(cubedirs)][lodPatchSize2];

		std::vector<SubBufs> subbufs;
	};

	typedef std::map<Astrobj*, BufferSet> BufferSets;

	static BufferSets bufsets;

	typedef std::function<double(const Vec3d &basepos, int ix, int iy)> HeightGetter;

	bool drawPatch(BufferSet &bufs, int direction, int lod, int px, int py);
	void enableBuffer(SubBufferSetBase &);

	static double height(const Vec3d &basepos, int octaves, double persistence, double aheight);
	static void point0(int divides, const Quatd &rot, BufferData &bd, int ix, int iy, HeightGetter &height);

	void compileVertexBuffers()const;

	SubBufs::iterator compileVertexBuffersSubBuf(BufferSet &bs, int lod, int direction, int ix, int iy);

	static void setVertexBuffers(const BufferData &bd, SubBufferSetBase &bs);

	int m_lods;
	double m_noiseHeight;
	double m_noisePersistence;
	double m_noiseLODRange;
	int m_noiseOctaves;
	int m_noiseBaseLevel;

	Quatd qrot;
};

#endif
