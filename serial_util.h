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
public:
	SerializeContext *sc; // redundant pointer to point each other, but useful to automate serialization syntax.
	typedef SerializeStream tt;
	SerializeStream(SerializeContext *asc = NULL) : sc(asc){}
	virtual ~SerializeStream(){}
	virtual tt &operator<<(int a) = 0;
	virtual tt &operator<<(unsigned a) = 0;
	virtual tt &operator<<(unsigned long a) = 0;
	virtual tt &operator<<(float a) = 0;
	virtual tt &operator<<(double a) = 0;
	virtual tt &operator<<(const char *a) = 0;
	virtual tt &operator<<(const std::string &a) = 0;
	virtual tt &operator<<(const Serializable *p) = 0;
	virtual tt &operator<<(const Vec3d &v) = 0;
	virtual tt &operator<<(const Quatd &v) = 0;
	virtual tt &operator<<(const random_sequence &v) = 0;
	template<typename T> tt &operator<<(const Vec4<T> &v);
	virtual tt *substream() = 0;
	virtual void join(tt *) = 0;
};

class StdSerializeStream : public SerializeStream{
protected:
	std::ostream &base; // cannot simply derive, for the stream can be either ifstream or istringstream but neither declare virtual.
public:
	typedef SerializeStream tt;

	StdSerializeStream(std::ostream &abase, SerializeContext *asc = NULL) : base(abase), tt(asc){}
	~StdSerializeStream();
	tt &operator<<(int a){ base.operator<<(a); return *this; }
	tt &operator<<(unsigned a){ base.operator<<(a); return *this; }
	tt &operator<<(unsigned long a){ base.operator<<(a); return *this; }
	tt &operator<<(float a){ base.operator<<(a); return *this; }
	tt &operator<<(double a){ base.operator<<(a); return *this; }
	tt &operator<<(const char *a){ base << a; return *this; }
	tt &operator<<(const std::string &a){ base << a; return *this; }
	virtual tt &operator<<(const Serializable *p){ base << p; return *this; };
	virtual tt &operator<<(const Vec3d &v);
	virtual tt &operator<<(const Quatd &v);
	virtual tt &operator<<(const random_sequence &v);
	virtual tt *substream();
	virtual void join(tt *);

	friend class StdSerializeSubStream;
};

class BinSerializeStream : public SerializeStream{
public:
	typedef SerializeStream tt;

	BinSerializeStream(SerializeContext *asc = NULL) : buf(NULL), size(0), tt(asc){}
	~BinSerializeStream();
	tt &operator<<(int a);
	tt &operator<<(unsigned a);
	tt &operator<<(unsigned long a);
	tt &operator<<(float a);
	tt &operator<<(double a);
	tt &operator<<(const char *a);
	tt &operator<<(const std::string &a);
	virtual tt &operator<<(const Serializable *p);
	virtual tt &operator<<(const Vec3d &v);
	virtual tt &operator<<(const Quatd &v);
	virtual tt &operator<<(const random_sequence &v);
	virtual tt *substream();
	virtual void join(tt *);

	tt &write(const BinSerializeStream &o);
	const void *getbuf()const{return buf;}
	size_t getsize()const{return size;}

protected:
	unsigned char *buf;
	size_t size;
	template<typename T> SerializeStream &write(T a);
};

class SerializeContext{
public:
	SerializeContext(SerializeStream &ao, SerializeMap &amap) : o(ao), map(amap){}
	SerializeContext(SerializeStream &ao, const SerializeContext &copy_src) : o(ao), map(copy_src.map){}
	SerializeStream &o;
	SerializeMap &map;
};

inline SerializeStream &SerializeStream::operator<<(const Serializable *p){
	return *this << sc->map[p];
}









class UnserializeStream{
public:
	UnserializeContext *usc; // redundant pointer to point each other, but useful to automate serialization syntax.
	typedef UnserializeStream tt;
	UnserializeStream(UnserializeContext *ausc = NULL) : usc(ausc){}
	virtual bool eof()const = 0;
	virtual tt &read(char *s, std::streamsize size) = 0;
	virtual tt &operator>>(int &a) = 0;
	virtual tt &operator>>(unsigned &a) = 0;
	virtual tt &operator>>(unsigned long &a) = 0;
	virtual tt &operator>>(float &a) = 0;
	virtual tt &operator>>(double &a) = 0;
	virtual tt &operator>>(Vec3d &v) = 0;
	virtual tt &operator>>(Quatd &v) = 0;
	virtual tt &operator>>(struct random_sequence &rs) = 0;
	virtual tt &operator>>(const char *a) = 0;
	virtual tt &operator>>(cpplib::dstring &a) = 0;
	virtual tt *substream(size_t size) = 0;
	template<class T> tt &operator>>(T *&p){
		unsigned id;
		*this >> id;
		p = static_cast<T*>(usc->map[id]);
		return *this;
	}
};

class StdUnserializeStream : public UnserializeStream{
	std::istream &base; // cannot simply derive, for the stream can be either ifstream or istringstream but neither declare virtual.
public:
	// abase must live as long as UnserializeContext does.
	StdUnserializeStream(std::istream &abase, UnserializeContext *ausc = NULL) : base(abase), tt(ausc){}
/*	char get(){ return base.get(); }
	tt &get(char *s, std::streamsize size){ base.get(s, size); return *this; }
	void unget(){ base.unget(); }
	bool fail()const{ return base.fail(); }
	bool eof()const{ return base.eof(); }
	void getline(std::string &s){ std::getline(base, s); }*/
	virtual bool eof()const{ return base.eof(); }
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
	tt &operator>>(cpplib::dstring &a);
	virtual tt *substream(size_t size);
};

class BinUnserializeStream : public UnserializeStream{
public:
	BinUnserializeStream(const unsigned char *src = NULL, size_t size = 0, UnserializeContext *ausc = NULL);
	virtual bool eof()const;
	tt &read(char *s, std::streamsize size);
	tt &operator>>(int &a);
	tt &operator>>(unsigned &a);
	tt &operator>>(unsigned long &a);
	tt &operator>>(float &a);
	tt &operator>>(double &a);
	tt &operator>>(Vec3d &v);
	tt &operator>>(Quatd &v);
	tt &operator>>(struct random_sequence &rs);
	tt &operator>>(const char *a);
	tt &operator>>(cpplib::dstring &a);
	virtual tt *substream(size_t size);

private:
	const unsigned char *src;
	size_t size;
	template<typename T> tt &read(T&);
	char get();
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
	UnserializeContext(UnserializeStream &ai, CtorMap &acons, UnserializeMap &amap);
	UnserializeStream &i;
	CtorMap &cons;
	UnserializeMap &map;
};

#endif
