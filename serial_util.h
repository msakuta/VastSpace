#ifndef SERIAL_UTIL_H
#define SERIAL_UTIL_H
#include "serial.h"
extern "C"{
#include <clib/rseq.h>
}
#include <cpplib/dstring.h>
#include <cpplib/vec3.h>
#include <cpplib/vec4.h>
#include <cpplib/quat.h>
#include <iostream>
#include <string>


class SerializeStream{
	std::ostream &base; // cannot simply derive, for the stream can be either ifstream or istringstream but neither declare virtual.
	SerializeContext &sc; // redundant pointer to point each other, but useful to automate serialization syntax.
public:
	typedef SerializeStream tt;

	SerializeStream(std::ostream &abase, SerializeContext &asc) : base(abase), sc(asc){}
	tt &operator<<(int a){ base.operator<<(a); return *this; }
	tt &operator<<(unsigned a){ base.operator<<(a); return *this; }
	tt &operator<<(unsigned long a){ base.operator<<(a); return *this; }
	tt &operator<<(float a){ base.operator<<(a); return *this; }
	tt &operator<<(double a){ base.operator<<(a); return *this; }
	tt &operator<<(const char *a){ base << a; return *this; }
	tt &operator<<(const std::string &a){ base << a; return *this; }
	tt &operator<<(const Vec3d &v);
	tt &operator<<(const Quatd &v);
	template<typename T> tt &operator<<(const Vec4<T> &v);
	tt &operator<<(const random_sequence &v);
	tt &operator<<(const Serializable *p);
};

class SerializeContext{
public:
	SerializeContext(std::ostream &ao, SerializeMap &amap) : o(ao, *this), map(amap){}
	SerializeContext(std::ostream &ao, const SerializeContext &copy_src) : o(ao, *this), map(copy_src.map){}
	SerializeStream o;
	SerializeMap &map;
};

inline SerializeStream &SerializeStream::operator<<(const Serializable *p){
	return *this << sc.map[p];
}











class UnserializeStream{
	std::istream &base; // cannot simply derive, for the stream can be either ifstream or istringstream but neither declare virtual.
	UnserializeContext &usc; // redundant pointer to point each other, but useful to automate serialization syntax.
public:
	typedef UnserializeStream tt;

	// abase must live as long as UnserializeContext does.
	UnserializeStream(std::istream &abase, UnserializeContext &ausc) : base(abase), usc(ausc){}
	char get(){ return base.get(); }
	tt &get(char *s, std::streamsize size){ base.get(s, size); return *this; }
	void unget(){ base.unget(); }
	bool fail()const{ return base.fail(); }
	bool eof()const{ return base.eof(); }
	void getline(std::string &s){ std::getline(base, s); }
	tt &read(char *s, std::streamsize size){ base.read(s, size); return *this; }
	tt &operator>>(int &a){ base.operator>>(a); return *this; }
	tt &operator>>(unsigned &a){ base.operator>>(a); return *this; }
	tt &operator>>(unsigned long &a){ base.operator>>(a); return *this; }
	tt &operator>>(float &a){ base.operator>>(a); return *this; }
	tt &operator>>(double &a){ base.operator>>(a); return *this; }
	tt &operator>>(Vec3d &v);
	tt &operator>>(Quatd &v);
	tt &operator>>(struct random_sequence &rs);
	tt &operator>>(const char *a);
	template<class T> tt &operator>>(T *&p){
		unsigned id;
		base >> id;
		p = static_cast<T*>(usc.map[id]);
		return *this;
	}
};

std::ostream &operator<<(std::ostream &o, Vec3d &v);

template<typename T> SerializeStream &SerializeStream::operator<<(const Vec4<T> &v){
	return *this << "(" << v[0] << " " << v[1] << " " << v[2] << " " << v[3] << ")";
}

template<typename T> UnserializeStream &operator>>(UnserializeStream &o, Vec4<T> &v){
	o >> "(" >> v[0] >> " " >> v[1] >> " " >> v[2] >> " " >> v[3] >> ")";
	return o;
}

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
