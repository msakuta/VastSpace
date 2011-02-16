#ifndef CPPLIB_GL_CULLPLUS_H
#define CPPLIB_GL_CULLPLUS_H
extern "C"{
#include <clib/gl/cull.h>
}
#include "../vec3.h"
#include "../mat4.h"

class GLcull{
	double fov; ///< field of view
	double znear; ///< near clipping plane is sometimes necesarry
	double zfar; ///< the distance of z far clipping plane
	Vec3d viewpoint; ///< The eye position
	Vec3d viewdir; ///< The eye direction
	Mat4d invrot; ///< inverse of rotation of modelview transformation
	Mat4d trans; ///< projection * modelview transformation; used for culling
	int width, height, res; ///< viewport characteristics
	bool ortho; ///< orthogonal flag
public:
	GLcull(double fov, const Vec3d &viewpoint, const Mat4d &invrot, double znear, double zfar); ///< Frustum constructor
	GLcull(const Vec3d &viewpoint, const Mat4d &invrot, double width, double znear, double zfar); ///< Orthogonal constructor
	bool cullFrustum(const Vec3d &pos, double rad)const;
	bool cullNear(const Vec3d &pos, double rad)const;
	bool cullFar(const Vec3d &pos, double rad)const;
	double scale(const Vec3d &pos)const;
	operator glcull()const;

	// accessors
	double getFov()const{return fov;}
	double getNear()const{return znear;}
	double getFar()const{return zfar;}
	Mat4d getInvrot()const{return invrot;}
	bool isOrtho()const{return ortho;}
};

#endif
