#ifndef VIEWER_H
#define VIEWER_H
#ifdef __cplusplus
extern "C"{
#endif
#include <clib/c.h>
#include <clib/amat4.h>
#include <clib/avec3.h>
#include <clib/gl/cull.h>
#ifdef __cplusplus
}
#include <cpplib/vec3.h>
#include <cpplib/mat4.h>
#include <cstring>
#endif
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

/* This header is so small but Viewer structure is frequently referred
  and its worth making a header dedicated for this (in means of compilation
  speed). */

#ifdef __cplusplus
class CoordSys;
#endif

struct viewport{
	int w, h, m;
#ifdef __cplusplus
	void set(int vp[4]){ // no virtual allowed!
		w = vp[2] - vp[0], h = vp[3] - vp[1], m = MAX(w, h);
	}
#endif
};

#ifdef __cplusplus
class Viewer{
public:
	Viewer(){
		std::memset(this, 0, sizeof *this);
		fov = 1.; // Field of View
	}
	Mat4d rot, irot, relrot, relirot;
	Vec3d pos, pyr, velo;
	double velolen;
	double fov, ar;
	double dynamic_range; /* experimental; to simulate high dynamic range */
	CoordSys *cs;
	struct glcull *gc; /* current culling information */
	struct glcull **gclist; /* list of glculls in order of z-slices */
	int ngclist; /* number of z-slices which can change in occasions */
	int zslice; /* index of gc into gclist */
	int relative; /* whether effect of relativity cannot be ignored */
	int detail; /* wireframe */
	struct viewport vp;
	void frustum(double n, double f){
		int w = vp.w, h = vp.h, m = vp.m;
		double l = -n * fov * w / m, t = n * fov * h / m, r = n * fov * w / m, b = -n * fov * h / m;
		glFrustum(l, r, b, t, n, f);
	}
};
#endif

#endif
