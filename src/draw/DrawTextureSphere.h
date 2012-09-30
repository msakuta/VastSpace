/** \file
 * \brief Implementation of drawing methods of astronomical objects like Star and TexSphere.
 */
#ifndef DRAWTEXTURESPHERE_H
#define DRAWTEXTURESPHERE_H
#include "../TexSphere.h"
#include "judge.h"


/// drawTextureSphere flags
enum DTS{
	DTS_ADD = 1<<0,
	DTS_NODETAIL = 1<<1,
	DTS_ALPHA = 1<<2,
	DTS_NORMALMAP = 1<<3,
	DTS_NOGLOBE = 1<<4,
	DTS_LIGHTING = 1<<5
};

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
		, m_cloudRotation(quat_u){}
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
	virtual bool draw();
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
