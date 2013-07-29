/** \file
 * \brief OpenGL-friendly 4x4 matrix template class.
 */
#ifndef CPPLIB_MAT3_H
#define CPPLIB_MAT3_H
#include "mathcommon.h"
#include <assert.h>
#include "vec3.h"

namespace cpplib{

template<class T> class Quat;

/** \brief 3x3 matrix class with its component type as template argument.
 *
 * Collaborates with Vec3, Quatd and Mat4.
 */
template<class T> class Mat3{
	typedef cpplib::Mat3<T> tt;
	T a[9];
public:
	typedef T intype[9]; ///< Internal object type
	typedef cpplib::Vec3<T> Vec3;
	typedef cpplib::Quat<T> Quat;
	Mat3(){}
	Mat3(Vec3 x, Vec3 y, Vec3 z);
	tt transpose()const;
	tt &scalein(T sx, T sy, T sz);
	tt inverse()const;

	/// vector product.
	Vec3 vp(const Vec3 &o)const;

	/// product with transpose matrix, useful for obtaining inverse rotation without making another matrix to transpose.
	Vec3 tvp3(const Vec3 &o)const;

	tt mul(const tt &o)const;

	/// Retrieve i-th vertical vector in the matrix.
	Vec3 &vec3(int i);
	const Vec3 &vec3(int i)const{return const_cast<tt*>(this)->vec3(i);}

	tt rotx(T p)const;
	tt roty(T y)const;
	tt rotz(T y)const;
	tt operator*(const tt &o)const{
		return mul(o);
	}

	/// Vector product. The operand vector must be placed at right of '*'.
	Vec3 operator*(const Vec3 &o)const{
		return vp(o);
	}

	operator const T*()const{return a;}		operator T*(){return a;}

	/// Cast to T* do the job, but range is not checked.
	T operator[](int i)const{assert(0 <= i && i < 9); return a[i];}
	T &operator[](int i){assert(0 <= i && i < 9); return a[i];}

	/// Multi-dimensional suffix operator
	T elem(int i, int j)const{assert(0 <= i && i < 3 && 0 <= j && j < 3); return a[i * 3 + j];}
	T &elem(int i, int j){assert(0 <= i && i < 3 && 0 <= j && j < 3); return a[i * 3 + j];}

	/// \brief Converting elements to a given type requires explicit call.
	template<typename T2> Mat3<T2> cast()const;

	/// Array to Class pointer converter
	static tt &atoc(T *a){return *reinterpret_cast<tt*>(a);}
};

typedef Mat3<double> Mat3d;
typedef Mat3<float> Mat3f;
typedef Mat3<int> Mat3i;

// Unit transformations initialized by first call.
extern const Mat3d &mat3d_u();
extern const Mat3f &mat3f_u();
extern const Mat3i &mat3i_u();



//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------

/// Constructor making matrix with 3 vertical vectors.
template<typename T> inline Mat3<T>::Mat3(Vec3 x, Vec3 y, Vec3 z){
	*reinterpret_cast<Vec3*>(&a[0]) = x;
	*reinterpret_cast<Vec3*>(&a[3]) = y;
	*reinterpret_cast<Vec3*>(&a[6]) = z;
}

template<typename T> inline Mat3<T> Mat3<T>::transpose()const{
	tt mr;
	const T *ma = a;
	(mr)[0]=(ma)[0],(mr)[3]=(ma)[1],(mr)[6]=(ma)[2],
	(mr)[1]=(ma)[3],(mr)[4]=(ma)[4],(mr)[7]=(ma)[5],
	(mr)[2]=(ma)[6],(mr)[5]=(ma)[7],(mr)[8]=(ma)[8];
	return mr;
}

template<typename T> inline Mat3<T> &Mat3<T>::scalein(T sx, T sy, T sz){
	T *mr = a;
	(mr)[0]*=(sx),(mr)[3]*=(sy),(mr)[6]*=(sz),
	(mr)[1]*=(sx),(mr)[4]*=(sy),(mr)[7]*=(sz),
	(mr)[2]*=(sx),(mr)[5]*=(sy),(mr)[8]*=(sz);
	return *this;
}

template<typename T> inline Vec3<T> &Mat3<T>::vec3(int i){
	if(0 <= i && i < 3)
		return *reinterpret_cast<Vec3*>(&a[i*3]);
	else
		throw mathcommon::RangeCheck();
}

template<typename T> inline Vec3<T> Mat3<T>::vp(const Vec3 &o)const{
	Vec3 ret;
	T *vr = ret;
	const T *ma = a, *vb = o;
	(vr)[0]=(ma)[0]*(vb)[0]+(ma)[3]*(vb)[1]+(ma)[6]*(vb)[2];
	(vr)[1]=(ma)[1]*(vb)[0]+(ma)[4]*(vb)[1]+(ma)[7]*(vb)[2];
	(vr)[2]=(ma)[2]*(vb)[0]+(ma)[5]*(vb)[1]+(ma)[8]*(vb)[2];
	return ret;
}

template<typename T> inline Vec3<T> Mat3<T>::tvp3(const Vec3 &o)const{
	Vec3 ret;
	T *vr = ret;
	const T *ma = a, *vb = o;
	(vr)[0]=(ma)[0]*(vb)[0]+(ma)[1]*(vb)[1]+(ma)[2]*(vb)[2];
	(vr)[1]=(ma)[3]*(vb)[0]+(ma)[4]*(vb)[1]+(ma)[5]*(vb)[2];
	(vr)[2]=(ma)[6]*(vb)[0]+(ma)[7]*(vb)[1]+(ma)[8]*(vb)[2];
	return ret;
}

template<typename T> inline Mat3<T> Mat3<T>::mul(const tt &o)const{
	tt ret;
	T *mr = ret.a;
	const T *ma = this->a, *mb = o.a;
	return tt(
		vp(&(mb)[0]),
		vp(&(mb)[3]),
		vp(&(mb)[6]));
}

template<typename T> Mat3<T> Mat3<T>::rotx(T p)const{
	double sp = sin(p), cp = cos(p);
	const T *ma = a;
	tt mr;
	(mr)[0] = (ma)[0], (mr)[3] = (ma)[3]*cp+(ma)[6]*sp, (mr)[6] = -(ma)[3]*sp+(ma)[6]*cp,
	(mr)[1] = (ma)[1], (mr)[4] = (ma)[4]*cp+(ma)[7]*sp, (mr)[7] = -(ma)[4]*sp+(ma)[7]*cp,
	(mr)[2] = (ma)[2], (mr)[5] = (ma)[5]*cp+(ma)[8]*sp, (mr)[8] = -(ma)[5]*sp+(ma)[8]*cp;
	return mr;
}
template<typename T> Mat3<T> Mat3<T>::roty(T y)const{
	double sy = sin(y), cy = cos(y);
	const T *ma = a;
	tt mr;
	(mr)[0] = (ma)[0]*cy+(ma)[6]*(-sy), (mr)[3] = (ma)[3], (mr)[6] = (ma)[0]*sy+(ma)[6]*cy,
	(mr)[1] = (ma)[1]*cy+(ma)[7]*(-sy), (mr)[4] = (ma)[4], (mr)[7] = (ma)[1]*sy+(ma)[7]*cy,
	(mr)[2] = (ma)[2]*cy+(ma)[8]*(-sy), (mr)[5] = (ma)[5], (mr)[8] = (ma)[2]*sy+(ma)[8]*cy;
	return mr;
}
template<typename T> Mat3<T> Mat3<T>::rotz(T y)const{
	double sz = sin(y), cz = cos(y);
	const T *ma = a;
	tt mr;
	(mr)[0] = (ma)[0]*cz+(ma)[3]*(sz), (mr)[3] = (ma)[0]*-sz+(ma)[3]*cz, (mr)[6] = (ma)[6],
	(mr)[1] = (ma)[1]*cz+(ma)[4]*(sz), (mr)[4] = (ma)[1]*-sz+(ma)[4]*cz, (mr)[7] = (ma)[7],
	(mr)[2] = (ma)[2]*cz+(ma)[5]*(sz), (mr)[5] = (ma)[2]*-sz+(ma)[5]*cz, (mr)[8] = (ma)[8];
	return mr;
}

template<typename T> inline Mat3<T> Mat3<T>::inverse()const{
	T idet = 1. /
		( elem(0, 0) * elem(1, 1) * elem(2, 2)
		+ elem(0, 1) * elem(1, 2) * elem(2, 0)
		+ elem(0, 2) * elem(1, 0) * elem(2, 1)
		- elem(0, 0) * elem(1, 2) * elem(2, 1)
		- elem(0, 1) * elem(1, 0) * elem(2, 2)
		- elem(0, 2) * elem(1, 1) * elem(2, 0) );
	tt ret;
	for(int i = 0; i < 3; i++) for(int j = 0; j < 3; j++)
		ret.elem(i, j) = elem((i+1) % 3, (j+1) % 3) * elem((i+2) % 3, (j+2) % 3) - elem((i+1) % 3, (j+2) % 3) * elem((i+2) % 3, (j+1) % 3);
	return ret.scalein(idet, idet, idet);
}

/// This is nested template function.
template<typename T> template<typename T2> inline Mat3<T2> Mat3<T>::cast()const{
	return Mat3<T2>(
		vec3(0).cast<T2>(),
		vec3(1).cast<T2>(),
		vec3(2).cast<T2>());
}

}
using namespace cpplib;

#endif
