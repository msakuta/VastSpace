/** \file
 * \brief OpenGL-friendly 4x4 matrix template class.
 */
#ifndef CPPLIB_MAT4_H
#define CPPLIB_MAT4_H
#include "mathcommon.h"
#include <assert.h>
#include "vec3.h"
#include "vec4.h"
#include "mat3.h"

namespace cpplib{

template<class T> class Quat;

/** \brief 4x4 matrix class with its component type as template argument.
 *
 * Collaborates with Vec3, Vec4, Quatd and Mat3.
 */
template<class T> class Mat4{
	typedef cpplib::Mat4<T> tt;
	T a[16];
public:
	typedef T intype[16]; ///< Internal object type
	typedef cpplib::Vec3<T> Vec3;
	typedef cpplib::Vec4<T> Vec4;
	typedef cpplib::Quat<T> Quat;
	typedef cpplib::Mat3<T> Mat3;
	Mat4(){}
	Mat4(Vec3 x, Vec3 y, Vec3 z, Vec3 w = Vec3(0,0,0)){
		*reinterpret_cast<Vec3*>(&a[0]) = x; a[3] = 0;
		*reinterpret_cast<Vec3*>(&a[4]) = y; a[7] = 0;
		*reinterpret_cast<Vec3*>(&a[8]) = z; a[11] = 0;
		*reinterpret_cast<Vec3*>(&a[12]) = w; a[15] = 1;
	}
	Mat4(Vec4 x, Vec4 y, Vec4 z, Vec4 w = Vec4(0,0,0,1)){
		*reinterpret_cast<Vec4*>(&a[0]) = x;
		*reinterpret_cast<Vec4*>(&a[4]) = y;
		*reinterpret_cast<Vec4*>(&a[8]) = z;
		*reinterpret_cast<Vec4*>(&a[12]) = w;
	}
	Mat4(const Mat3 &m){
		*reinterpret_cast<Vec3*>(&a[0]) = m.vec3(0); a[3] = 0;
		*reinterpret_cast<Vec3*>(&a[4]) = m.vec3(0); a[7] = 0;
		*reinterpret_cast<Vec3*>(&a[8]) = m.vec3(0); a[11] = 0;
		a[12] = a[13] = a[14] = 0; a[15] = 1;
	}
	tt transpose()const{
		tt mr;
		const T *ma = a;
		(mr)[0]=(ma)[0],(mr)[4]=(ma)[1],(mr)[8]=(ma)[2],(mr)[12]=(ma)[3],
		(mr)[1]=(ma)[4],(mr)[5]=(ma)[5],(mr)[9]=(ma)[6],(mr)[13]=(ma)[7],
		(mr)[2]=(ma)[8],(mr)[6]=(ma)[9],(mr)[10]=(ma)[10],(mr)[14]=(ma)[11],
		(mr)[3]=(ma)[12],(mr)[7]=(ma)[13],(mr)[11]=(ma)[14],(mr)[15]=(ma)[15];
		return mr;
	}
	tt &scalein(T sx, T sy, T sz){
		T *mr = a;
		(mr)[0]*=(sx),(mr)[4]*=(sy),(mr)[8]*=(sz),
		(mr)[1]*=(sx),(mr)[5]*=(sy),(mr)[9]*=(sz),
		(mr)[2]*=(sx),(mr)[6]*=(sy),(mr)[10]*=(sz),
		(mr)[3]*=(sx),(mr)[7]*=(sy),(mr)[11]*=(sz);
		return *this;
	}

	// Translation
	tt &translatein(T sx, T sy, T sz);
	tt &translatein(const Vec3 &sv);
	tt translate(T sx, T sy, T sz)const;
	tt translate(const Vec3 &sv)const;

	// vector product with fourth element of the given vector be assumed as 1.
	Vec3 vp3(const Vec3 &o)const{
		Vec3 ret;
		T *vr = ret;
		const T *ma = a, *vb = o;
		(vr)[0]=(ma)[0]*(vb)[0]+(ma)[4]*(vb)[1]+(ma)[8]*(vb)[2]+(ma)[12];
		(vr)[1]=(ma)[1]*(vb)[0]+(ma)[5]*(vb)[1]+(ma)[9]*(vb)[2]+(ma)[13];
		(vr)[2]=(ma)[2]*(vb)[0]+(ma)[6]*(vb)[1]+(ma)[10]*(vb)[2]+(ma)[14];
		return ret;
	}

	/// delta vector product, ignoring fourth column.
	Vec3 dvp3(const Vec3 &o)const{
		Vec3 ret;
		T *vr = ret;
		const T *ma = a, *vb = o;
		(vr)[0]=(ma)[0]*(vb)[0]+(ma)[4]*(vb)[1]+(ma)[8]*(vb)[2];
		(vr)[1]=(ma)[1]*(vb)[0]+(ma)[5]*(vb)[1]+(ma)[9]*(vb)[2];
		(vr)[2]=(ma)[2]*(vb)[0]+(ma)[6]*(vb)[1]+(ma)[10]*(vb)[2];
		return ret;
	}

	/// product with transpose matrix, useful for obtaining inverse rotation without making another matrix to transpose.
	Vec3 tvp3(const Vec3 &o)const{
		Vec3 ret;
		T *vr = ret;
		const T *ma = a, *vb = o;
		(vr)[0]=(ma)[0]*(vb)[0]+(ma)[1]*(vb)[1]+(ma)[2]*(vb)[2]+(ma)[3];
		(vr)[1]=(ma)[4]*(vb)[0]+(ma)[5]*(vb)[1]+(ma)[6]*(vb)[2]+(ma)[7];
		(vr)[2]=(ma)[8]*(vb)[0]+(ma)[9]*(vb)[1]+(ma)[10]*(vb)[2]+(ma)[11];
		return ret;
	}

	/// inverse delta vector product for rotational matrix.
	Vec3 tdvp3(const Vec3 &o)const{
		Vec3 ret;
		T *vr = ret;
		const T *ma = a, *vb = o;
		(vr)[0]=(ma)[0]*(vb)[0]+(ma)[1]*(vb)[1]+(ma)[2]*(vb)[2];
		(vr)[1]=(ma)[4]*(vb)[0]+(ma)[5]*(vb)[1]+(ma)[6]*(vb)[2];
		(vr)[2]=(ma)[8]*(vb)[0]+(ma)[9]*(vb)[1]+(ma)[10]*(vb)[2];
		return ret;
	}

	Vec4 vp(const Vec4 &o)const{
		Vec4 ret;
		T *vr = ret;
		const T *ma = a, *vb = o;
		(vr)[0]=(ma)[0]*(vb)[0]+(ma)[4]*(vb)[1]+(ma)[8]*(vb)[2]+(ma)[12]*(vb)[3];
		(vr)[1]=(ma)[1]*(vb)[0]+(ma)[5]*(vb)[1]+(ma)[9]*(vb)[2]+(ma)[13]*(vb)[3];
		(vr)[2]=(ma)[2]*(vb)[0]+(ma)[6]*(vb)[1]+(ma)[10]*(vb)[2]+(ma)[14]*(vb)[3];
		(vr)[3]=(ma)[3]*(vb)[0]+(ma)[7]*(vb)[1]+(ma)[11]*(vb)[2]+(ma)[15]*(vb)[3];
		return ret;
	}
	tt mul(const tt &o)const{
		tt ret;
		T *mr = ret.a;
		const T *ma = this->a, *mb = o.a;
		return tt(
			vp(&(mb)[0]),
			vp(&(mb)[4]),
			vp(&(mb)[8]),
			vp(&(mb)[12]));
	}

	/// Retrieve i-th vertical vector in the matrix, without the last element.
	Vec3 &vec3(int i);
	const Vec3 &vec3(int i)const{return const_cast<tt*>(this)->vec3(i);}

	/// Retrieve i-th vertical vector in the matrix, with the last element.
	Vec4 &vec4(int i);
	const Vec4 &vec4(int i)const{return const_cast<tt*>(this)->vec4(i);}

	tt rotx(T p)const{
		double sp = sin(p), cp = cos(p);
		const T *ma = a;
		tt mr;
		(mr)[0] = (ma)[0], (mr)[4] = (ma)[4]*cp+(ma)[8]*sp, (mr)[8] = -(ma)[4]*sp+(ma)[8]*cp, (mr)[12] = (ma)[12],
		(mr)[1] = (ma)[1], (mr)[5] = (ma)[5]*cp+(ma)[9]*sp, (mr)[9] = -(ma)[5]*sp+(ma)[9]*cp, (mr)[13] = (ma)[13],
		(mr)[2] = (ma)[2], (mr)[6] = (ma)[6]*cp+(ma)[10]*sp, (mr)[10] = -(ma)[6]*sp+(ma)[10]*cp, (mr)[14] = (ma)[14],
		(mr)[3] = (ma)[3], (mr)[7] = (ma)[7]*cp+(ma)[11]*sp, (mr)[11] = -(ma)[7]*sp+(ma)[11]*cp, (mr)[15] = (ma)[15];
		return mr;
	}
	tt roty(T y)const{
		double sy = sin(y), cy = cos(y);
		const T *ma = a;
		tt mr;
		(mr)[0] = (ma)[0]*cy+(ma)[8]*(-sy), (mr)[4] = (ma)[4], (mr)[8] = (ma)[0]*sy+(ma)[8]*cy, (mr)[12] = (ma)[12],
		(mr)[1] = (ma)[1]*cy+(ma)[9]*(-sy), (mr)[5] = (ma)[5], (mr)[9] = (ma)[1]*sy+(ma)[9]*cy, (mr)[13] = (ma)[13],
		(mr)[2] = (ma)[2]*cy+(ma)[10]*(-sy), (mr)[6] = (ma)[6], (mr)[10] = (ma)[2]*sy+(ma)[10]*cy, (mr)[14] = (ma)[14],
		(mr)[3] = (ma)[3]*cy+(ma)[11]*(-sy), (mr)[7] = (ma)[7], (mr)[11] = (ma)[3]*sy+(ma)[11]*cy, (mr)[15] = (ma)[15];
		return mr;
	}
	tt rotz(T y)const{
		double sz = sin(y), cz = cos(y);
		const T *ma = a;
		tt mr;
		(mr)[0] = (ma)[0]*cz+(ma)[4]*(sz), (mr)[4] = (ma)[0]*-sz+(ma)[4]*cz, (mr)[8] = (ma)[8], (mr)[12] = (ma)[12],
		(mr)[1] = (ma)[1]*cz+(ma)[5]*(sz), (mr)[5] = (ma)[1]*-sz+(ma)[5]*cz, (mr)[9] = (ma)[9], (mr)[13] = (ma)[13],
		(mr)[2] = (ma)[2]*cz+(ma)[6]*(sz), (mr)[6] = (ma)[2]*-sz+(ma)[6]*cz, (mr)[10] = (ma)[10], (mr)[14] = (ma)[14],
		(mr)[3] = (ma)[3]*cz+(ma)[7]*(sz), (mr)[7] = (ma)[3]*-sz+(ma)[7]*cz, (mr)[11] = (ma)[11], (mr)[15] = (ma)[15];
		return mr;
	}
	tt operator*(const tt &o)const{
		return mul(o);
	}

	/// Vector products. The operand vector must be placed at right of '*'.
	Vec3 operator*(const Vec3 &o)const{
		return vp3(o);
	}
	Vec4 operator*(const Vec4 &o)const{
		return vp(o);
	}

	operator const T*()const{return a;}		operator T*(){return a;}

	/// cast to T* do the job, but range is not checked.
	T operator[](int i)const{assert(0 <= i && i < 16); return a[i];}
	T &operator[](int i){assert(0 <= i && i < 16); return a[i];}

	/// cast to T(*[16]) do not cope with cast to T*, so here is a unary operator to explicitly do this.
	intype *operator ~(){return &a;} const intype *operator ~()const{return &a;}

	/// Array to Class pointer converter
	static tt &atoc(T *a){return *reinterpret_cast<tt*>(a);}

	Mat3 tomat3()const;

	/// Converting elements to a given type requires explicit call.
	template<typename T2> Mat4<T2> cast()const;
};

typedef Mat4<double> Mat4d;

extern const Mat4d mat4_u;



//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------

template<typename T> inline Vec3<T> &Mat4<T>::vec3(int i){
	if(0 <= i && i < 4)
		return *reinterpret_cast<Vec3*>(&a[i*4]);
	else
		throw mathcommon::RangeCheck();
}

template<typename T> inline Vec4<T> &Mat4<T>::vec4(int i){
	if(0 <= i && i < 4)
		return *reinterpret_cast<Vec4*>(&a[i*4]);
	else
		throw mathcommon::RangeCheck();
}

template<typename T> inline Mat4<T> &Mat4<T>::translatein(T sx, T sy, T sz){
	T *mr = a;
	(mr)[12]+=(mr)[0]*(sx)+(mr)[4]*(sy)+(mr)[8]*(sz),
	(mr)[13]+=(mr)[1]*(sx)+(mr)[5]*(sy)+(mr)[9]*(sz),
	(mr)[14]+=(mr)[2]*(sx)+(mr)[6]*(sy)+(mr)[10]*(sz),
	(mr)[15]+=(mr)[3]*(sx)+(mr)[7]*(sy)+(mr)[11]*(sz);
	return *this;
}

template<typename T> inline Mat4<T> &Mat4<T>::translatein(const Vec3 &sv){
	return translatein(sv[0], sv[1], sv[2]);
}

template<typename T> inline Mat4<T> Mat4<T>::translate(T sx, T sy, T sz)const{
	tt ret(*this);
	return ret.translatein(sx, sy, sz);
}

template<typename T> inline Mat4<T> Mat4<T>::translate(const Vec3 &sv)const{
	tt ret(*this);
	return ret.translatein(sv);
}

template<typename T> inline Mat3<T> Mat4<T>::tomat3()const{
	return Mat3(vec3(0), vec3(1), vec3(2));
}

template<typename T> template<typename T2> inline Mat4<T2> Mat4<T>::cast()const{
	return Mat4<T2>(
		vec4(0).cast<T2>(),
		vec4(1).cast<T2>(),
		vec4(2).cast<T2>(),
		vec4(3).cast<T2>());
}

}
using namespace cpplib;

#endif
