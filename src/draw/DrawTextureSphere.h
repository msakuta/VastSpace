/** \file
 * \brief Definition of DrawTexSphere and DrawTextureSpheroid classes.
 */
#ifndef DRAWTEXTURESPHERE_H
#define DRAWTEXTURESPHERE_H
#include "../TexSphere.h"
#include "judge.h"


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
	Vec4f m_mat_diffuse;
	Vec4f m_mat_ambient;
	GLuint *ptexlist;
	Mat4d m_texmat;
	const char *m_texname;
	const std::vector<TexSphere::Texture> *m_textures;
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
	tt &astro(TexSphere *a){this->a = a; return *this;}
	tt &viewer(const Viewer *vw){this->vw = vw; return *this;}
	tt &mat_diffuse(const Vec4f &a){this->m_mat_diffuse = a; return *this;}
	tt &mat_ambient(const Vec4f &a){this->m_mat_ambient = a; return *this;}
	tt &texlist(GLuint *a){ptexlist = a; return *this;}
	tt &texmat(const Mat4d &a){m_texmat = a; return *this;}
	tt &texname(const char *a){m_texname = a; return *this;}
	tt &textures(const std::vector<TexSphere::Texture> &atextures){m_textures = &atextures; return *this;}
	tt &rad(double a){m_rad = a; return *this;}
	tt &flags(int a){m_flags = a; return *this;}
	tt &shader(GLuint a){m_shader = a; return *this;}
	tt &drawint(bool a){m_drawint = a; return *this;}
	tt &ncuts(int a){m_ncuts = a; m_nfinecuts = a * 8; m_nffinecuts = a * 64; return *this;}
	tt &ring(AstroRing *a){m_ring = a; return *this;}
	tt &ringRange(double ringmin, double ringmax){m_ringmin = ringmin; m_ringmax = ringmax; return *this;}
	tt &cloudRotation(const Quatd &acloudrot){m_cloudRotation = acloudrot; return *this;}
	tt &noisePos(const Vec3f &apos){m_noisePos = apos; return *this;}
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
	static GLuint ProjectSphereCube(const char *name, const BITMAPINFO *raw, BITMAPINFO *cacheload[6], unsigned flags);

	/// \brief Reads an Mercator projection texture in a JPEG file and pass it to ProjectSphereCube().
	/// \param fname The file name of JPEG file.
	/// \param flags DrawTextureSphere flags.
	static GLuint ProjectSphereCubeImage(const char *fname, int flags);

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
	tt(TexSphere *a, const Viewer *vw, const Vec3d &sunpos) : st(a, vw, sunpos)
		, m_oblateness(0.){}
	tt &oblateness(double a){m_oblateness = a; return *this;}
	virtual bool draw();
};


#endif
