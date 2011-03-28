#ifndef CPPLIB_QUAT_H
#define CPPLIB_QUAT_H
#include "vec3.h"
#include <assert.h>
#include <math.h> // sqrt
#include <float.h> // FLT_EPSILON, DBL_EPSILON
/** \file
 * \brief OpenGL-friendly Quaternion implementation template class.
 */

namespace cpplib{

template<typename T> class Vec3;
template<typename T> class Mat4;

/// Quaternion class with its component type as template argument.
template<typename T> class Quat{
	typedef Quat<T> tt; ///< This type
	T a[4];
public:
	typedef T intype[4]; ///< Internal type
	typedef cpplib::Vec3<T> Vec3;
	typedef cpplib::Mat4<T> Mat4;
	Quat(){}
	Quat(T x, T y, T z, T w){a[0] = x, a[1] = y, a[2] = z, a[3] = w;}
	Quat(const Vec3 &o){a[0] = o[0], a[1] = o[1], a[2] = o[2], a[3] = 0;}
	void clear(){a[0] = a[1] = a[2] = a[3] = 0;} ///< Assign zero to all elements
	const T &re()const{return a[3];} T &re(){return a[3];} ///< Returns real part
	const T &i()const{return a[0];} T &i(){return a[0];} ///< Returns imaginary 'i' part
	const T &j()const{return a[1];} T &j(){return a[1];} ///< Returns imaginary 'j' part
	const T &k()const{return a[2];} T &k(){return a[2];} ///< Returns imaginary 'k' part
	tt scale(T s)const{
		return tt(a[0] * s, a[1] * s, a[2] * s, a[3] * s);
	}
	tt &scalein(T s){
		a[0] *= s; a[1] *= s; a[2] *= s; a[3] *= s;
		return *this;
	}
	T len()const{
		return ::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2] + a[3] * a[3]);
	}
	T slen()const{
		return a[0] * a[0] + a[1] * a[1] + a[2] * a[2] + a[3] * a[3];
	}
	tt norm()const{
		double s = len();
		return tt(a[0]/s,a[1]/s,a[2]/s,a[3]/s);
	}
	tt &normin(){
		double s = len(); a[0] /= s, a[1] /= s, a[2] /= s, a[3] /= s;
		return *this;
	}
	tt mul(const tt &o)const; ///< Multiplication
	tt imul(const tt &o)const{
		return mul(o.cnj());
	}
	tt operator*(const tt &o)const{return mul(o);}
	double sp(const tt &o)const{return a[0] * o.a[0] + a[1] * o.a[1] + a[2] * o.a[2] + a[3] * o.a[3];}
	T &operator[](int i){
		assert(0 <= i && i < 4);
		return a[i];
	}
	const T &operator[](int i)const{
		assert(0 <= i && i < 4);
		return a[i];
	}
	tt &operator*=(const tt &o);
	operator const intype*()const{
		return &a;
	}
	operator const intype&()const{
		return a;
	}
	operator intype*(){
		return &a;
	}
	operator intype&(){
		return a;
	}
	operator Vec3&(){
		return *reinterpret_cast<Vec3*>(this);
	}
	tt &addin(const tt &o){
		a[0] += o.a[0], a[1] += o.a[1], a[2] += o.a[2], a[3] += o.a[3];
		return *this;
	}
	tt add(const tt &o)const{
		return tt(a[0] + o.a[0], a[1] + o.a[1], a[2] + o.a[2], a[3] + o.a[3]);
	}
	tt &operator+=(const tt &o){return addin(o);}
	tt operator+(const tt &o)const{return add(o);}
	bool operator==(const tt &o)const;
	bool operator!=(const tt &o)const{return !operator==(o);}
	tt cnj()const{
		return tt(-a[0], -a[1], -a[2], a[3]);
	}
	Vec3 trans(const Vec3 &src)const; ///< Transforms given vector
	Vec3 itrans(const Vec3 &src)const{
		return cnj().trans(src);
	}
	tt quatrotquat(const Vec3 &v)const; ///< Creates a quaternion rotated by given angular velocity times delta-time

	static tt rotation(T p, T sx, T sy, T sz);
	static tt rotation(T p, const Vec3 &vec);
	tt rotate(T p, T sx, T sy, T sz)const;
	tt rotate(T p, const Vec3 &vec)const;

	Mat4 tomat4()const{
		return Mat4(trans(Vec3(1, 0, 0)), trans(Vec3(0, 1, 0)), trans(Vec3(0, 0, 1)));
	}
	static tt direction(const Vec3 &dir);
	static tt slerp(const tt &q, const tt &r, const double t); ///< Spheric Linear Interpolation

	/// Converting elements to a given type requires explicit call.
	template<typename T2> Quat<T2> cast()const;

	/// Square root of epsilon. This value depends on T, so we explicitly instanciate it as necessary.
	static T sqrtepsilon(){return ::sqrt(DBL_EPSILON);}
};

// Type definitions for common element types.
typedef Quat<double> Quatd;
typedef Quat<float> Quatf;
typedef Quat<int> Quati;

// Constant value definitions. This conversion operator trick was not necessary back in clib aquat_t.
class quat_0_t{ public: operator const Quatd &()const; };
extern const quat_0_t quat_0;
class quat_u_t{ public: operator const Quatd &()const; };
extern const quat_u_t quat_u;



//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------

template<typename T> inline	Quat<T> Quat<T>::mul(const tt &o)const{
	const T *qa = this->a, *qb = o.a;
	return tt((qa)[1]*(qb)[2]-(qa)[2]*(qb)[1]+(qa)[0]*(qb)[3]+(qa)[3]*(qb)[0],\
		(qa)[2]*(qb)[0]-(qa)[0]*(qb)[2]+(qa)[1]*(qb)[3]+(qa)[3]*(qb)[1],\
		(qa)[0]*(qb)[1]-(qa)[1]*(qb)[0]+(qa)[2]*(qb)[3]+(qa)[3]*(qb)[2],\
		-(qa)[0]*(qb)[0]-(qa)[1]*(qb)[1]-(qa)[2]*(qb)[2]+(qa)[3]*(qb)[3]);
}


template<typename T> inline Quat<T> &Quat<T>::operator*=(const tt &o){
	// There should be certainly better algorithm out there, but I rely on the optimizer for this one.
	*this = *this * o;
	return *this;
}

template<typename T> inline bool Quat<T>::operator ==(const tt &o)const{
	return a[0] == o.a[0] && a[1] == o.a[1] && a[2] == o.a[2] && a[3] == o.a[3];
}

template<typename T> inline Vec3<T> Quat<T>::trans(const Vec3 &src)const{
	tt r, qr;
	tt qc, qret;
	qc = cnj();
	r = src;
	qr = mul(r);
	qret = qr * qc;
	return Vec3(qret[0], qret[1], qret[2]);
}

template<typename T> inline Quat<T> Quat<T>::quatrotquat(const Vec3 &v)const{
	tt q, qr;
	q[0] = v[0];
	q[1] = v[1];
	q[2] = v[2];
	q[3] = 0;
	qr = q * *this;
	qr += *this;
	return qr.norm();
}

/// \brief Returns rotation represented as a quaternion, defined by angle and vector.
///
/// Note that the vector must be normalized.
/// \param p Angle in radians
/// \param sx,sy,sz Components of axis vector, must be normalized
template<typename T> inline Quat<T> Quat<T>::rotation(T p, T sx, T sy, T sz){
	T len = sin(p / 2.);
	return Quatd(len * sx, len * sy, len * sz, cos(p / 2.));
}

/// \brief Returns rotation represented as a quaternion, defined by angle and vector.
///
/// Note that the vector must be normalized.
/// \param p Angle in radians
/// \param vec Axis vector of rotation, must be normalized
template<typename T> inline Quat<T> Quat<T>::rotation(T p, const Vec3 &vec){
	return rotation(p, vec[0], vec[1], vec[2]);
}

/// \brief Returns rotated version of this quaternion with given angle around vector components.
///
/// Note that the vector must be normalized.
/// \param p Angle in radians
/// \param sx,sy,sz Components of axis vector, must be normalized
template<typename T> inline Quat<T> Quat<T>::rotate(T p, T sx, T sy, T sz)const{
	return *this * rotation(p, sx, sy, sz);
}

/// \brief Returns rotated version of this quaternion with given angle around vector.
///
/// Note that the vector must be normalized.
/// \param p Angle in radians
/// \param vec Axis vector of rotation, must be normalized
template<typename T> inline Quat<T> Quat<T>::rotate(T p, const Vec3 &vec)const{
	return rotate(p, vec[0], vec[1], vec[2]);
}


///< Creates a quaternion representing rotation towards given direction.
template<typename T> inline Quat<T> Quat<T>::direction(const Vec3 &dir){
	static const T epsilon = sqrtepsilon();
	double p;
	tt ret;
	Vec3 dr = dir.norm();

	/* half-angle formula of trigonometry replaces expensive tri-functions to square root */
	ret[3] = ::sqrt((dr[2] + 1) / 2) /*cos(acos(dr[2]) / 2.)*/;

	if(1 - epsilon < ret[3]){
		ret.a[0] = ret.a[1] = ret.a[2] = 0;
	}
	else if(ret[3] < epsilon){
		ret.a[0] = ret.a[2] = 0;
		ret.a[1] = 1;
	}
	else{
		Vec3d v = vec3_001.vp(dr);
		p = sqrt(1 - ret[3] * ret[3]) / (v.len());
		ret[0] = v[0] * p;
		ret[1] = v[1] * p;
		ret[2] = v[2] * p;
	}
	return ret;
}

template<typename T> inline Quat<T> Quat<T>::slerp(const typename Quat<T>::tt &q, const typename Quat<T>::tt &r, const double t){
	tt ret;
	double qr = q.a[0] * r.a[0] + q.a[1] * r.a[1] + q.a[2] * r.a[2] + q.a[3] * r.a[3];
	double ss = 1.0 - qr * qr;
  
	if (ss == 0.0) {
		return q;
	}
	else if(q == r){
		return q;
	}
	else {
		double sp = ::sqrt(ss);
		double ph = ::acos(qr);
		double pt = ph * t;
		double t1 = ::sin(pt) / sp;
		double t0 = ::sin(ph - pt) / sp;

		// Long path case
		if(qr < 0)
			t1 *= -1;

		return tt(
			q.a[0] * t0 + r.a[0] * t1,
			q.a[1] * t0 + r.a[1] * t1,
			q.a[2] * t0 + r.a[2] * t1,
			q.a[3] * t0 + r.a[3] * t1);
	}
}


template<typename T> template<typename T2> inline Quat<T2> Quat<T>::cast()const{
	return Quat<T2>(
		static_cast<T2>(a[0]),
		static_cast<T2>(a[1]),
		static_cast<T2>(a[2]),
		static_cast<T2>(a[3]));
}

/// Instantiation of sqrtepsilon in case of float.
template<> float Quat<float>::sqrtepsilon(){return ::sqrt(FLT_EPSILON);}

}

using namespace cpplib;

#endif
