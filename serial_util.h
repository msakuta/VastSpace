#ifndef SERIAL_UTIL_H
#define SERIAL_UTIL_H
#include <cpplib/dstring.h>
#include <cpplib/vec3.h>
#include <cpplib/vec4.h>
#include <cpplib/quat.h>
#include <iostream>

std::ostream &operator<<(std::ostream &o, Vec3d &v);

template<typename T> std::ostream &operator<<(std::ostream &o, Vec4<T> &v){
	o << "(" << v[0] << " " << v[1] << " " << v[2] << " " << v[3] << ")";
	return o;
}

std::ostream &operator<<(std::ostream &o, Quatd &v);

std::istream &operator>>(std::istream &o, const char *cstr);

std::istream &operator>>(std::istream &o, Vec3d &v);

template<typename T> std::istream &operator>>(std::istream &o, Vec4<T> &v){
	o >> "(" >> v[0] >> " " >> v[1] >> " " >> v[2] >> " " >> v[3] >> ")";
	return o;
}

std::istream &operator>>(std::istream &o, Quatd &v);

cpplib::dstring readUntil(std::istream &in, char c);

char *strnewdup(const char *src, size_t len);

char *strmdup(const char *src, size_t len);

#endif
