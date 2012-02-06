#ifndef SERIAL_UTIL_H
#define SERIAL_UTIL_H
/** \file
 * \brief Defines classes necessary to perform serialization of Serializable classes.
 */
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
#include <set>
#include <exception>


class FormatException : public std::exception{
public:
	FormatException(){}
	const char *what()const throw(){return "FormatException";}
};

class InputShortageException : public std::exception{
public:
	InputShortageException(){}
	const char *what()const throw(){return "InputShortageException";}
};

class ClassNotFoundException : public std::exception{
public:
	ClassNotFoundException(){}
	const char *what()const throw(){return "ClassNotFoundException";}
};


/// \brief The abstract base class for serialization stream classes.
///
/// It's much like std::stream, but interprets various objects in specific ways.
class SerializeStream{
public:
	SerializeContext *sc; // redundant pointer to point each other, but useful to automate serialization syntax.
	typedef SerializeStream tt;
	SerializeStream(SerializeContext *asc = NULL) : sc(asc){}
	virtual ~SerializeStream(){}
	virtual tt &operator<<(int a) = 0;
	virtual tt &operator<<(unsigned a) = 0;
	virtual tt &operator<<(long a) = 0;
	virtual tt &operator<<(unsigned long a) = 0;
	virtual tt &operator<<(bool a) = 0;
	virtual tt &operator<<(float a) = 0;
	virtual tt &operator<<(double a) = 0;
	virtual tt &operator<<(const char *a) = 0;
	virtual tt &operator<<(const Serializable *p) = 0;
	virtual tt &operator<<(const Vec3d &v) = 0;
	virtual tt &operator<<(const Quatd &v) = 0;
	virtual tt &operator<<(const random_sequence &v) = 0;
	template<typename T> tt &operator<<(const Vec4<T> &v);
	virtual tt *substream() = 0;
	virtual void join(tt *) = 0;
};

/// \brief Serialization class that converts whole object "web" into a sequence of text.
/// \sa StdUnserializeStream
class StdSerializeStream : public SerializeStream{
protected:
	std::ostream &base; // cannot simply derive, for the stream can be either ifstream or istringstream but neither declare virtual.
public:
	typedef SerializeStream tt;

	StdSerializeStream(std::ostream &abase, SerializeContext *asc = NULL) : base(abase), tt(asc){}
	~StdSerializeStream();
	virtual tt &operator<<(int a);
	virtual tt &operator<<(unsigned a);
	virtual tt &operator<<(long a);
	virtual tt &operator<<(unsigned long a);
	virtual tt &operator<<(bool a);
	virtual tt &operator<<(float a);
	virtual tt &operator<<(double a);
	virtual tt &operator<<(const char *a);
	virtual tt &operator<<(const Serializable *p);
	virtual tt &operator<<(const Vec3d &v);
	virtual tt &operator<<(const Quatd &v);
	virtual tt &operator<<(const random_sequence &v);
	virtual tt *substream();
	virtual void join(tt *);

	friend class StdSerializeSubStream;
};

/// \brief Serialization class that converts objects into binary stream.
/// \sa BinUnserializeStream
class BinSerializeStream : public SerializeStream{
public:
	typedef SerializeStream tt;

	BinSerializeStream(SerializeContext *asc = NULL) : buf(NULL), size(0), tt(asc){}
	~BinSerializeStream();
	virtual tt &operator<<(int a);
	virtual tt &operator<<(unsigned a);
	virtual tt &operator<<(long a);
	virtual tt &operator<<(unsigned long a);
	virtual tt &operator<<(bool a);
	virtual tt &operator<<(float a);
	virtual tt &operator<<(double a);
	virtual tt &operator<<(const char *a);
	virtual tt &operator<<(const std::string &a);
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
	SerializeContext(SerializeStream &ao, Serializable *&avisit_list) : o(ao), visit_list(avisit_list), trunk(true){}
	SerializeContext(SerializeStream &ao, const SerializeContext &copy_src) : o(ao), visit_list(copy_src.visit_list), trunk(true){}
	SerializeStream &o;
//	SerializeMap &map;
	Serializable *&visit_list;
	bool trunk; // Whether processing node in class inheritance graph is trunk (base virtual class) or branch (no base serialization).
};

inline SerializeStream &SerializeStream::operator<<(const Serializable *p){
//	return *this << sc->map[p];
	return *this << p->getid();
}









/// \brief The abstract base class for unserialization stream classes.
///
/// It's much like std::stream, but distinct class tree from SerializeStream.
class UnserializeStream{
public:
	UnserializeContext *usc; ///< redundant pointer to point each other, but useful to automate serialization syntax.
	typedef UnserializeStream tt;
	UnserializeStream(UnserializeContext *ausc = NULL) : usc(ausc){}
	virtual ~UnserializeStream(){}
	virtual bool eof()const = 0;
	virtual bool fail()const = 0;
	virtual tt &read(char *s, std::streamsize size) = 0;
	virtual tt &operator>>(int &a) = 0;
	virtual tt &operator>>(unsigned &a) = 0;
	virtual tt &operator>>(long &a) = 0;
	virtual tt &operator>>(unsigned long &a) = 0;
	virtual tt &operator>>(bool &a) = 0;
	virtual tt &operator>>(float &a) = 0;
	virtual tt &operator>>(double &a) = 0;
	virtual tt &operator>>(Vec3d &v) = 0;
	virtual tt &operator>>(Quatd &v) = 0;
	virtual tt &operator>>(struct random_sequence &rs) = 0;
	virtual tt &operator>>(const char *a) = 0;
	virtual tt &operator>>(cpplib::dstring &a) = 0;
	virtual tt *substream(size_t size) = 0;
	template<class T> tt &operator>>(T *&p);
};

/// \brief Unserialization counterparts of StdSerializeStream.
class StdUnserializeStream : public UnserializeStream{
protected:
	std::istream &base; // cannot simply derive, for the stream can be either ifstream or istringstream but neither declare virtual.
	tt &consume(const char *);
public:
	// abase must live as long as UnserializeContext does.
	StdUnserializeStream(std::istream &abase, UnserializeContext *ausc = NULL) : base(abase), tt(ausc){}
	virtual bool eof()const;
	virtual bool fail()const;
	virtual tt &read(char *s, std::streamsize size);
	virtual tt &operator>>(int &a);
	virtual tt &operator>>(unsigned &a);
	virtual tt &operator>>(long &a);
	virtual tt &operator>>(unsigned long &a);
	virtual tt &operator>>(bool &a);
	virtual tt &operator>>(float &a);
	virtual tt &operator>>(double &a);
	virtual tt &operator>>(Vec3d &v);
	virtual tt &operator>>(Quatd &v);
	virtual tt &operator>>(struct random_sequence &rs);
	virtual tt &operator>>(const char *a);
	virtual tt &operator>>(cpplib::dstring &a);
	virtual tt *substream(size_t size);
};

/// \brief Unserialization counterpart of BinSerializeSteram.
class BinUnserializeStream : public UnserializeStream{
public:
	BinUnserializeStream(const unsigned char *src = NULL, size_t size = 0, UnserializeContext *ausc = NULL);
	virtual bool eof()const;
	virtual bool fail()const;
	virtual tt &read(char *s, std::streamsize size);
	virtual tt &operator>>(int &a);
	virtual tt &operator>>(unsigned &a);
	virtual tt &operator>>(long &a);
	virtual tt &operator>>(unsigned long &a);
	virtual tt &operator>>(bool &a);
	virtual tt &operator>>(float &a);
	virtual tt &operator>>(double &a);
	virtual tt &operator>>(Vec3d &v);
	virtual tt &operator>>(Quatd &v);
	virtual tt &operator>>(struct random_sequence &rs);
	virtual tt &operator>>(const char *a);
	virtual tt &operator>>(cpplib::dstring &a);
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


template<class T> UnserializeStream &UnserializeStream::operator>>(T *&p){
	SerializableId id;
	*this >> id;
	p = static_cast<T*>(usc->map[id]);
	return *this;
}

#endif
