/** \file
 * \brief OpenGL-friendly 4x4 matrix template class.
 */
#ifndef CPPLIB_MAT2_H
#define CPPLIB_MAT2_H
#include "mathcommon.h"
#include <assert.h>
#include "vec2.h"

namespace cpplib{


/** \brief 2x2 matrix class with its component type as template argument.
 *
 * Collaborates with Vec2.
 */
template<class T> class Mat2{
	typedef cpplib::Mat2<T> tt;
	T a[4];
public:
	typedef T intype[4]; ///< Internal object type
	typedef cpplib::Vec2<T> Vec2;
	Mat2(){}
	Mat2(T a, T b, T c, T d);
	Mat2(Vec2 x, Vec2 y);
	tt transpose()const;
	tt &scalein(T sx, T sy);
	tt inverse()const;

	/// vector product with fourth element of the given vector be assumed as 1.
	Vec2 operator*(const Vec2 &o)const;

	/// product with transpose matrix, useful for obtaining inverse rotation without making another matrix to transpose.
	Vec2 tvp(const Vec2 &o)const;

	Vec2 vp(const Vec2 &o)const;

	tt mul(const tt &o)const;

	/// Retrieve i-th vertical vector in the matrix.
	Vec2 &vec2(int i);
	const Vec2 &vec2(int i)const{return const_cast<tt*>(this)->vec2(i);}

	tt rot(T y)const;
	tt operator*(const tt &o)const{
		return mul(o);
	}

	operator const T*()const{return a;}		operator T*(){return a;}

	/// Cast to T* do the job, but range is not checked.
	T operator[](int i)const{assert(0 <= i && i < 4); return a[i];}
	T &operator[](int i){assert(0 <= i && i < 4); return a[i];}

	/// Multi-dimensional suffix operator
	T elem(int i, int j)const{assert(0 <= i && i < 2 && 0 <= j && j < 2); return a[i * 2 + j];}
	T &elem(int i, int j){assert(0 <= i && i < 2 && 0 <= j && j < 2); return a[i * 2 + j];}

	/// \brief Converting elements to a given type requires explicit call.
	template<typename T2> Mat2<T2> cast()const;

	/// Array to Class pointer converter
	static tt &atoc(T *a){return *reinterpret_cast<tt*>(a);}
};

typedef Mat2<double> Mat2d;
typedef Mat2<float> Mat2f;
typedef Mat2<int> Mat2i;




//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------

template<typename T> inline Mat2<T>::Mat2(T a, T b, T c, T d){
	this->a[0] = a;
	this->a[1] = b;
	this->a[2] = c;
	this->a[3] = d;
}

/// Constructor making matrix with 2 vertical vectors.
template<typename T> inline Mat2<T>::Mat2(Vec2 x, Vec2 y){
	*reinterpret_cast<Vec2*>(&a[0]) = x;
	*reinterpret_cast<Vec2*>(&a[2]) = y;
}

template<typename T> inline Mat2<T> Mat2<T>::transpose()const{
	tt mr;
	const T *ma = a;
	mr[0]=ma[0],mr[2]=ma[1];
	mr[1]=ma[2],mr[3]=ma[3];
	return mr;
}

template<typename T> inline Mat2<T> &Mat2<T>::scalein(T sx, T sy){
	T *mr = a;
	(mr)[0]*=(sx),(mr)[2]*=(sy);
	(mr)[1]*=(sx),(mr)[3]*=(sy);
	return *this;
}

template<typename T> inline Vec2<T> &Mat2<T>::vec2(int i){
	if(0 <= i && i < 2)
		return *reinterpret_cast<Vec2*>(&a[i*2]);
	else
		throw mathcommon::RangeCheck();
}

template<typename T> inline Vec2<T> Mat2<T>::operator*(const Vec2 &o)const{
	return vp(o);
}

template<typename T> inline Vec2<T> Mat2<T>::vp(const Vec2 &o)const{
	const T *vb = o;
	return Vec2(
		a[0] * vb[0] + a[1] * vb[1],
		a[2] * vb[0] + a[3] * vb[1]
	);
}

template<typename T> inline Vec2<T> Mat2<T>::tvp(const Vec2 &o)const{
	const T *vb = o;
	return Vec2(
		a[0] * vb[0] + a[2] * vb[1],
		a[1] * vb[0] + a[3] * vb[1]
	);
}

template<typename T> inline Mat2<T> Mat2<T>::mul(const tt &o)const{
	return tt(
		vp(o.vec2(0)),
		vp(o.vec2(1)));
}

template<typename T> inline Mat2<T> Mat2<T>::rot(T y)const{
	double sz = sin(y), cz = cos(y);
	const T *ma = a;
	tt mr;
	(mr)[0] = (ma)[0]*cz+(ma)[2]*(sz), (mr)[2] = (ma)[0]*-sz+(ma)[2]*cz;
	(mr)[1] = (ma)[1]*cz+(ma)[3]*(sz), (mr)[3] = (ma)[1]*-sz+(ma)[3]*cz;
	return mr;
}

template<typename T> inline Mat2<T> Mat2<T>::inverse()const{
	T det = a[0] * a[3] - a[1] * a[2];
	assert(det != T(0));
	T idet = 1. / det;
	return tt(a[3] * idet, -a[2] * idet, -a[1] * idet, a[0] * idet);
}

/// This is nested template function.
template<typename T> template<typename T2> inline Mat2<T2> Mat2<T>::cast()const{
	return Mat2<T2>(
		Vec2(0).cast<T2>(),
		Vec2(1).cast<T2>());
}

}
using namespace cpplib;

#endif
