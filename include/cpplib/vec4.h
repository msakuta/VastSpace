#ifndef CPPLIB_VEC4_H
#define CPPLIB_VEC4_H
/** \file
 * \brief Definition of simple vector in 4-dimension.
 *
 * It also declares constants of vectors and friend operators that
 * enable vector arithmetics of arbitrary order.
 */
#include <math.h>
#include <assert.h>

namespace cpplib{

template<typename T> class Vec3;

/** \brief Vector in 4-dimension with arbitrary elements.
 *
 * Collaborates with Vec3, Quatd and Mat4.
 */
template<typename T> class Vec4{
	typedef Vec4<T> tt;
	T a[4];
public:
	typedef T intype[4];
	Vec4(){}
	Vec4(const T o[4]){a[0] = o[0], a[1] = o[1], a[2] = o[2], a[3] = o[3];}
	Vec4(T x, T y, T z, T w){a[0] = x, a[1] = y, a[2] = z, a[3] = w;}
	void clear(){a[0] = a[1] = a[2] = a[3] = 0;} // assign zero element
	tt &addin(const tt &o){
		a[0] += o.a[0], a[1] += o.a[1], a[2] += o.a[2], a[3] += o.a[3];
		return *this;
	}
	tt add(const tt &o)const{
		return tt(a[0] + o.a[0], a[1] + o.a[1], a[2] + o.a[2], a[3] + o.a[3]);
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
	tt operator-()const{return tt(-a[0], -a[1], -a[2], -a[3]);}
	tt &operator+=(const tt &o){return addin(o);}
	tt operator+(const tt &o)const{return add(o);}
	tt &operator-=(const tt &o){return addin(-o);}
	tt operator-(const tt &o)const{return add(-o);}
	tt &scalein(T s){
		a[0] *= s, a[1] *= s, a[2] *= s, a[3] *= s;
		return *this;
	}
	tt scale(T s)const{
		tt ret = *this;
		ret.scalein(s);
		return ret;
	}
	T sp(const tt &o)const{
		return a[0] * o.a[0] + a[1] * o.a[1] + a[2] * o.a[2] + a[3] * o.a[3];
	}
	tt &operator*=(const T s){return scalein(s);}
	tt operator*(const T s)const{return scale(s);}
	tt &operator/=(const T s); ///< \brief Divide by a scalar.
	tt operator/(const T s)const; ///< \brief Create divided version of this by given scalar.
	friend tt operator*(const T s, const tt &o){return o * s;}
	operator T*(){return a;} operator const T*()const{return a;}
	bool operator==(const tt &o)const{return a[0] == o.a[0] && a[1] == o.a[1] && a[2] == o.a[2] && a[3] == o.a[3];}
	bool operator!=(const tt &o)const{return !operator==(o);}

	/// cast to T* do the job, but range is not checked.
	T operator[](int i)const{assert(0 <= i && i < 4); return a[i];}
	T &operator[](int i){assert(0 <= i && i < 4); return a[i];}

	/// cast to T(*[4]) do not cope with cast to T*, so here is a unary operator to explicitly do this.
	intype *operator ~(){return &a;} const intype *operator ~()const{return &a;}

	/// Converting elements to a given type requires explicit call.
	template<typename T2> Vec4<T2> cast()const;
	operator Vec3<T>()const{return Vec3<T>(a[0],a[1],a[2]);}

	/// Array to Class pointer converter
	static tt &atoc(T *a){return *reinterpret_cast<tt*>(a);}
};

typedef Vec4<double> Vec4d; ///< Type definition for double vector.
typedef Vec4<float> Vec4f; ///< Type definition for float vector.
typedef Vec4<int> Vec4i; ///< Type definition for int vector.

extern const Vec4d &vec4_0001();
extern const Vec4d &vec4_0010();
extern const Vec4d &vec4_0100();
extern const Vec4d &vec4_1000();




//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------

/// Division is not equivalent to scale with inverse in case of integral component type.
template<typename T>
inline Vec4<T> &Vec4<T>::operator/=(const T s){
	a[0] /= s, a[1] /= s, a[2] /= s, a[3] /= s; return *this;
}

/// Division is not equivalent to scale with inverse in case of integral component type.
template<typename T>
inline Vec4<T> Vec4<T>::operator/(const T s)const{
	return Vec4<T>(a[0] / s, a[1] / s, a[2] / s, a[3] / s);
}


template<typename T> template<typename T2> Vec4<T2> Vec4<T>::cast()const{
	return Vec4<T2>(
		static_cast<T2>(a[0]),
		static_cast<T2>(a[1]),
		static_cast<T2>(a[2]),
		static_cast<T2>(a[3]));
}

}

using namespace cpplib;

#endif
