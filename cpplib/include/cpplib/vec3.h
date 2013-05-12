#ifndef CPPLIB_VEC3_H
#define CPPLIB_VEC3_H
/** \file
 * \brief Definition of simple vector in 3-dimension.
 *
 * It also declares constants of vectors and friend operators that
 * enable vector arithmetics of arbitrary order.
 */
#include <assert.h>
#include <math.h>

/// cpplib is common and compact library for C++.
namespace cpplib{

template<typename T> class Vec4;

/** \brief Vector in 3-dimension with arbitrary elements.
 *
 * Collaborates with Vec4, Quatd, Mat3 and Mat4.
 */
template<typename T> class Vec3{
	T a[3];
public:
	typedef Vec3<T> tt; ///< This Type
	typedef T intype[3]; ///< Internal type.
	Vec3(){} ///< The default constructor does nothing to allocated memory area, which means uninitialized object.
	Vec3(const T o[3]){a[0] = o[0], a[1] = o[1], a[2] = o[2];} ///< Initialize from an array of size 3.
	Vec3(T x, T y, T z){a[0] = x, a[1] = y, a[2] = z;} ///< Specify individual elements.
	void clear(){a[0] = a[1] = a[2] = 0;} ///< Assigns zero to all elements.
	Vec3<T> &addin(const Vec3<T> &o); ///< Adds given vector into this object.
	Vec3<T> add(const Vec3<T> &o)const; ///< Calculates sum of two vectors.
	T len()const{
		return ::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
	} ///< Calculates length of this vector. \return Length (norm) of this vector.
	T slen()const{
		return a[0] * a[0] + a[1] * a[1] + a[2] * a[2];
	} ///< Calculates squared length of this vector. This operation is faster than len() because no square root is required. \return Squared length (norm) of this vector.
	tt norm()const{
		double s = len();
		return tt(a[0]/s,a[1]/s,a[2]/s);
	} ///< Creates a normalized version of this vector. \return Newly created vector with the same direction of this and length of 1.
	tt &normin(){
		double s = len(); a[0] /= s, a[1] /= s, a[2] /= s;
		return *this;
	} ///< Normalizes this vector. \return This vector.
	tt operator-()const{return tt(-a[0], -a[1], -a[2]);} ///< Unary minus operator inverts elements. \return Newly created vector with negated elements of this.
	tt &operator+=(const tt &o){return addin(o);} ///< Adds given vector into this object. Same as addin().
	tt operator+(const tt &o)const{return add(o);} ///< Calculates sum of two vectors. Same as add().
	tt &operator-=(const tt &o){return addin(-o);} ///< Subtracts this vector by given vector. Same as addin(-o).
	tt operator-(const tt &o)const{return add(-o);} ///< Calculates difference of two vectors. Same as add(-o).
	tt &scalein(T s){
		a[0] *= s, a[1] *= s, a[2] *= s;
		return *this;
	} ///< Scale this vector by given scalar. \return This vector
	tt scale(T s)const{
		tt ret;
		((ret.a)[0]=(a)[0]*(s),(ret.a)[1]=(a)[1]*(s),(ret.a)[2]=(a)[2]*(s));
		return ret;
	} ///< Calculate scaled version of this vector by given scalar. \return Newly created vector
	T sp(const tt &o)const{
		return a[0] * o.a[0] + a[1] * o.a[1] + a[2] * o.a[2];
	} ///< Calculate scalar (dot) product with another vector.
	tt vp(const tt &o)const{
		return tt(a[1]*o.a[2]-a[2]*o.a[1], a[2]*o.a[0]-a[0]*o.a[2], a[0]*o.a[1]-a[1]*o.a[0]);
	} ///< Calculate vector (cross) product with another vector.
	tt &operator*=(const T s){return scalein(s);} ///< scalein().
	tt operator*(const T s)const{return scale(s);} ///< scale().
	tt &operator/=(const T s); ///< \brief Divide by a scalar.
	tt operator/(const T s)const; ///< \brief Create divided version of this by given scalar.
	friend tt operator*(const T s, const tt &o){return o * s;} ///< Binary multiply operator with a scalar as the first argument need to be global function.
	operator T*(){return a;} operator const T*()const{return a;} ///< Cast to array of T.
	bool operator==(const tt &o)const{return a[0] == o.a[0] && a[1] == o.a[1] && a[2] == o.a[2];} ///< Compares two vectors.
	bool operator!=(const tt &o)const{return !operator==(o);} ///< Compares two vectors.

	/// cast to T* do the job, but range is not checked.
	T operator[](int i)const{assert(0 <= i && i < 3); return a[i];}
	T &operator[](int i){assert(0 <= i && i < 3); return a[i];}

	/// cast to T(*[3]) do not cope with cast to T*, so here is a unary operator to explicitly do this.
	intype *operator ~(){return &a;} const intype *operator ~()const{return &a;}

	/// Converting elements to a given type requires explicit call.
	template<class T2> Vec3<T2> cast()const{return Vec3<T2>(static_cast<T2>(a[0]),static_cast<T2>(a[1]),static_cast<T2>(a[2]));}

	/// Expands this vector to Vec4 by appending fourth element of zero.
	operator Vec4<T>()const{return Vec4<T>(a[0],a[1],a[2],0);}

	/// \brief Array to Class pointer converter.
	///
	/// Use this if you want to pretend an array of scalar as a Vec3 without creating temporary object.
	static tt &atoc(T *a){return *reinterpret_cast<tt*>(a);}
};

typedef Vec3<double> Vec3d; ///< Type definition for double vector.
typedef Vec3<float> Vec3f; ///< Type definition for float vector.
typedef Vec3<int> Vec3i; ///< Type definition for int vector.

extern const Vec3d vec3_000; ///< Null vector constant. Note that this object may not be initialized when referred by another initialization code.
extern const Vec3d vec3_100; ///< (1,0,0) vector constant. Note that this object may not be initialized when referred by another initialization code.
extern const Vec3d vec3_010; ///< (0,1,0) vector constant. Note that this object may not be initialized when referred by another initialization code.
extern const Vec3d vec3_001; ///< (0,0,1) vector constant. Note that this object may not be initialized when referred by another initialization code.


//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------


/// \param o The other object to add.
/// \return This object after addition is performed.
template<typename T>
inline Vec3<T> &Vec3<T>::addin(const Vec3<T> &o){
	a[0] += o.a[0], a[1] += o.a[1], a[2] += o.a[2];
	return *this;
}

/// \param o The other object to add.
/// \return Newly created object of sum of this and o.
template<typename T>
inline Vec3<T> Vec3<T>::add(const Vec3<T> &o)const{
	return tt(a[0] + o.a[0], a[1] + o.a[1], a[2] + o.a[2]);
}

/// Division is not equivalent to scale with inverse in case of integral component type.
template<typename T>
inline Vec3<T> &Vec3<T>::operator/=(const T s){
	a[0] /= s, a[1] /= s, a[2] /= s; return *this;
}

/// Division is not equivalent to scale with inverse in case of integral component type.
template<typename T>
inline Vec3<T> Vec3<T>::operator/(const T s)const{
	return Vec3<T>(a[0] / s, a[1] / s, a[2] / s);
}

}

using namespace cpplib;

#endif
