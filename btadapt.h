/** \file
 * \brief The Bullet dynamics library adaptor for cpplib.
 *
 * Contains classes to convert cpplib math classes to Bullet classes, or vice versa.
 */
#ifndef BTADAPT_H
#define BTADAPT_H
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#ifdef _WIN32
#pragma warning ( push )
#pragma warning ( disable : 4244 ) // The Bullet library gets conversion warnings if btScalar == double.
#include <btBulletDynamicsCommon.h>
#pragma warning ( pop )
#endif


/// Intermediate class that can convert a Vec3d to btVector3, or vice versa.
class btvc : public Vec3d{
public:
	btvc(const btVector3 &bv):Vec3d(bv.x(), bv.y(), bv.z()){}
	btvc(const Vec3d &v):Vec3d(v){}
	operator btVector3()const{return btVector3((*this)[0], (*this)[1], (*this)[2]);}
};


/// Intermediate class that can convert a Quatd to btQuaternion, or vice versa.
class btqc : public Quatd{
public:
	btqc(const btQuaternion &bv):Quatd(bv.x(), bv.y(), bv.z(), bv.w()){}
	btqc(const Quatd &v):Quatd(v){}
	operator btQuaternion()const{return btQuaternion((*this)[0], (*this)[1], (*this)[2], (*this)[3]);}
};


#endif
