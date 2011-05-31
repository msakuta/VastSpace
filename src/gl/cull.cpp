#include "cpplib/gl/cullplus.h"
#include "cpplib/vec3.h"
#include "cpplib/mat4.h"
#if _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <stdlib.h>
#include <math.h>

#define SQRT2 1.4142135623730950488016887242097

GLcull::GLcull(double afov, const Vec3d &viewpoint, const Mat4d &invrot, double znear, double zfar){
	fov = afov;
	this->viewpoint = viewpoint;
	this->invrot = invrot;
	this->znear = znear;
	this->zfar = zfar;

	/* initiate the matrix for culling */
	{
		Mat4d modelmat, persmat;
		modelmat = invrot.transpose();
		modelmat.translatein(-viewpoint[0], -viewpoint[1], -viewpoint[2]);

		persmat[0] = 1. / fov;
		persmat[1] = 0;
		persmat[2] = 0;
		persmat[3] = 0;
		persmat[4] = 0;
		persmat[5] = 1. / fov;
		persmat[6] = 0;
		persmat[7] = 0;
		persmat[8] = 0;
		persmat[9] = 0;
		persmat[10] = - (zfar + znear) / (zfar - znear);
		persmat[11] = -1;
		persmat[12] = 0;
		persmat[13] = 0;
		persmat[14] = 2 * zfar * znear / (zfar - znear);
		persmat[15] = 0;

		/* TODO: aspect ratio other than 1 causes troubles determining intersection against
		  sphere geometries unless we assign fake ratio here. */
		persmat[14] = 0.; /* assign 0 to this element to make negative coords to be culled */
		trans = persmat * modelmat;
	}
	{
		Vec3d nh0(0., 0., -1.);
		this->viewdir = invrot.vp3(nh0);
	}
	{
		int viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		width = viewport[2];
		height = viewport[3];
		res = width < height ? height : width;
	}
	ortho = false;
}

GLcull::GLcull(const Vec3d &viewpoint, const Mat4d &invrot, double width, double znear, double zfar){
	fov = 1.;
	this->viewpoint = viewpoint;
	this->invrot = invrot;
	this->znear = znear;
	this->zfar = zfar;

	/* initiate the matrix for culling */
	{
		Mat4d modelmat, persmat;
		glGetDoublev(GL_MODELVIEW_MATRIX, modelmat);
//		modelmat.vec3(3).clear();
//		modelmat.translatein(-viewpoint[0], -viewpoint[1], -viewpoint[2]);
		glGetDoublev(GL_PROJECTION_MATRIX, persmat);

		trans = persmat * modelmat;
	}
	{
		int viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		width = viewport[2];
		height = viewport[3];
		res = width < height ? height : width;
	}
	ortho = true;
}

// Examine if an object can be culled by near clipping plane.
// Silly VC won't allow me to use "near" for being a keyword.
bool GLcull::cullNear(const Vec3d &pos, double rad)const{
	Vec3d delta;
	delta = pos - viewpoint;
	if(viewdir.sp(delta) < znear - rad)
		return 1;
	else
		return 0;
}

// Examine if an object can be culled by far clipping plane.
// Silly VC won't allow me to use "far" for being a keyword.
bool GLcull::cullFar(const Vec3d &pos, double rad)const{
	Vec3d delta;
	delta = pos - viewpoint;
	if(zfar + rad < viewdir.sp(delta))
		return 1;
	else
		return 0;
}

// Examine if an object can be culled by viewing volume.
bool GLcull::cullFrustum(const Vec3d &pos, double rad)const{
	Vec3d dst, vec, delta;
	double sp;
	if(ortho)
		return false;
	delta = pos - viewpoint;
	sp = viewdir.sp(delta);
	if(zfar + rad < sp)
		return true;
	if(sp < znear - rad)
		return true;
	dst = pos;
	dst += viewdir * SQRT2 * rad / fov;
	vec = trans.vp3(dst);
	if(vec[2] < 0. || vec[0] < -vec[2] || vec[2] < vec[0] || vec[1] < -vec[2] || vec[2] < vec[1])
		return true;
	return false;
}

/* returned value can be negative! */
double GLcull::scale(const Vec3d &pos)const{
	if(ortho)
		return res;
	Vec3d delta = pos - viewpoint;
	double divisor = delta.sp(viewdir) * fov;
	return divisor != 0. ? 1. / divisor * res : HUGE_VAL;
}

GLcull::operator glcull()const{
	glcull ret;
	ret.fov = fov;
	ret.znear = znear;
	ret.zfar = zfar;
	*(Vec3d*)ret.viewpoint = viewpoint;
	*(Vec3d*)ret.viewdir = viewdir;
	*(Mat4d*)ret.invrot = invrot;
	*(Mat4d*)ret.trans = trans;
	ret.width = width;
	ret.height = height;
	ret.res = res;
	ret.ortho = ortho;
	return ret;
}



