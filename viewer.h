#ifndef VIEWER_H
#define VIEWER_H
#ifdef __cplusplus
extern "C"{
#endif
#include <clib/c.h>
#include <clib/amat4.h>
#include <clib/avec3.h>
#ifdef __cplusplus
}
#include <cpplib/vec3.h>
#include <cpplib/mat4.h>
#include <cpplib/quat.h>
#include <cpplib/gl/cullplus.h>
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
	Quatd qrot; // Rotation expressed in quaternion
	Mat4d rot, irot, relrot, relirot;
	Vec3d pos, pyr, velo;
	double velolen;
	double fov, ar;
	double dynamic_range; /* experimental; to simulate high dynamic range */
	double viewtime; // not a physical time, just for blinking lights
	double dt; // Delta time of this drawing frame
	const CoordSys *cs;
	GLcull *gc; /* current culling information */
	GLcull **gclist; /* list of glculls in order of z-slices */
	int ngclist; /* number of z-slices which can change in occasions */
	int zslice; /* index of gc into gclist */
	int relative; /* whether effect of relativity cannot be ignored */
	int detail; /* wireframe */
	int mousex, mousey; // Indicates mouse cursor position in viewport coordinates.
	struct viewport vp;
	void frustum(double n, double f){
		int w = vp.w, h = vp.h, m = vp.m;
		double l = -n * fov * w / m, t = n * fov * h / m, r = n * fov * w / m, b = -n * fov * h / m;
		glFrustum(l, r, b, t, n, f);
	}
};
#endif

#endif
