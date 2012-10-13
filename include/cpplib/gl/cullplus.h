/** \file
 * \brief Definition ofGLcull class.
 */
#ifndef CPPLIB_GL_CULLPLUS_H
#define CPPLIB_GL_CULLPLUS_H
extern "C"{
#include <clib/gl/cull.h>
}
#include "../vec3.h"
#include "../mat4.h"

/// \brief A class to deterimne if an object should be culled from viewing volume.
///
/// OpenGL's viewing volume is 6-sided frustum, not a cone. So we can cull objects nearer or farther than clipping planes.
/// The function cullFrustom() will do all necessary jobs, but you can also examine each clipping planes.
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
	bool cullFrustum(const Vec3d &pos, double rad)const; ///< Determine if given sphere should be culled in 6-sided frustum.
	bool cullNear(const Vec3d &pos, double rad)const; ///< Determine if given sphere should be culled by near clipping plane.
	bool cullFar(const Vec3d &pos, double rad)const; ///< Determine if given sphere should be culled by near clipping plane.
	bool cullCone(const Vec3d &pos, double rad)const; ///< Determine if given sphere should be culled by FOV's 4 sided cone.
	double scale(const Vec3d &pos)const; ///< Returns rough factor of zooming at a given point viewed fom the camera. Negative value means it's behind the camera.
	operator glcull()const;

	// accessors
	double getFov()const{return fov;}
	double getNear()const{return znear;}
	double getFar()const{return zfar;}
	Vec3d getViewpoint()const{return viewpoint;}
	Vec3d getViewdir()const{return viewdir;}
	Mat4d getInvrot()const{return invrot;}
	bool isOrtho()const{return ortho;}
};

#endif
