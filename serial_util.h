#ifndef SERIAL_UTIL_H
#define SERIAL_UTIL_H
#include "serial.h"
#include <cpplib/dstring.h>
#include <cpplib/vec3.h>
#include <cpplib/vec4.h>
#include <cpplib/quat.h>
#include <iostream>
#include <string>

class UnserializeStream{
	std::istream &base; // cannot simply derive, for the stream can be either ifstream or istringstream but neither declare virtual.
	UnserializeContext &usc; // redundant pointer to point each other, but useful to automate serialization syntax.
public:
	typedef UnserializeStream tt;

	// abase must live as long as UnserializeContext does.
	UnserializeStream(std::istream &abase, UnserializeContext &ausc) : base(abase), usc(ausc){
	}
	char get(){ return base.get(); }
	void unget(){ base.unget(); }
	bool eof(){ return base.eof(); }
	void getline(std::string &s){ std::getline(base, s); }
	tt &operator>>(int &a){ base.operator>>(a); return *this; }
	tt &operator>>(unsigned &a){ base.operator>>(a); return *this; }
	tt &operator>>(unsigned long &a){ base.operator>>(a); return *this; }
	tt &operator>>(float &a){ base.operator>>(a); return *this; }
	tt &operator>>(double &a){ base.operator>>(a); return *this; }
	tt &operator>>(Vec3d &v);
	tt &operator>>(Quatd &v);
	tt &operator>>(struct random_sequence &rs);
	tt &operator>>(const char *a);
	// Note that this template function is not thread-safe.
	template<class T> tt &operator>>(T *&p){
		unsigned id;
		base >> id;
		p = static_cast<T*>(usc.map[id]);
		return *this;
	}
};

std::ostream &operator<<(std::ostream &o, Vec3d &v);

template<typename T> std::ostream &operator<<(std::ostream &o, Vec4<T> &v){
	o << "(" << v[0] << " " << v[1] << " " << v[2] << " " << v[3] << ")";
	return o;
}

std::ostream &operator<<(std::ostream &o, const Quatd &v);

std::ostream &operator<<(std::ostream &o, const struct random_sequence &rs);

std::istream &operator>>(std::istream &o, const char *cstr);

std::istream &operator>>(std::istream &o, Vec3d &v);

template<typename T> UnserializeStream &operator>>(UnserializeStream &o, Vec4<T> &v){
	o >> "(" >> v[0] >> " " >> v[1] >> " " >> v[2] >> " " >> v[3] >> ")";
	return o;
}

std::istream &operator>>(std::istream &o, Quatd &v);

std::istream &operator>>(std::istream &o, struct random_sequence &rs);

cpplib::dstring readUntil(std::istream &in, char c);
cpplib::dstring readUntil(UnserializeStream &in, char c);

char *strnewdup(const char *src, size_t len);

char *strmdup(const char *src, size_t len);


class UnserializeContext{
public:
	UnserializeContext(std::istream &ai, CtorMap &acons, UnserializeMap &amap);
	UnserializeStream i;
	CtorMap &cons;
	UnserializeMap &map;
};

#endif
