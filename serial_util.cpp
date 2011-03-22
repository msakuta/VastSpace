#include "serial_util.h"
extern "C"{
#include <clib/rseq.h>
}
#include <sstream>
#include <stdlib.h>
#include <string.h>
using namespace std;


SerializeStream &StdSerializeStream::operator<<(const Vec3d &v){
	base << "(";
	*this << v[0] << v[1] << v[2];
	base << ")";
	return *this;
}

SerializeStream &StdSerializeStream::operator<<(const Quatd &v){
	base << "(";
	*this << v.i() << v.j() << v.k() << v.re();
	base << ")";
	return *this;
}

SerializeStream &StdSerializeStream::operator<<(const struct ::random_sequence &rs){
	base << "(";
	*this << rs.w << rs.z;
	base << ")";
	return *this;
}

SerializeStream &StdSerializeStream::operator<<(int a){ base << a << " "; return *this; }
SerializeStream &StdSerializeStream::operator<<(unsigned a){ base << a << " "; return *this; }
SerializeStream &StdSerializeStream::operator<<(long a){ base << a << " "; return *this; }
SerializeStream &StdSerializeStream::operator<<(unsigned long a){ base << a << " "; return *this; }
SerializeStream &StdSerializeStream::operator<<(bool a){ base << (int)a << " "; return *this; }
SerializeStream &StdSerializeStream::operator<<(float a){ base << a << " "; return *this; }
SerializeStream &StdSerializeStream::operator<<(double a){ base << a << " "; return *this; }
SerializeStream &StdSerializeStream::operator<<(const char *a){
	base.put('"');
	for(; *a; a++){
		if(*a == '"' || *a == '\\')
			base.put('\\');
		base.put(*a);
	}
	base.put('"');
	return *this;
}
SerializeStream &StdSerializeStream::operator<<(const Serializable *p){ base << sc->map[p] << " "; return *this; }




class StdSerializeSubStream : public StdSerializeStream{
	StdSerializeStream &parent;
	std::ostringstream src;
public:
	StdSerializeSubStream(StdSerializeStream &aparent) :
		parent(aparent),
		src(),
		StdSerializeStream(src, aparent.sc){}
	~StdSerializeSubStream();
};

StdSerializeSubStream::~StdSerializeSubStream(){
	unsigned len = src.str().length();
	parent << len;
	parent.base << src.str();
}

StdSerializeStream::~StdSerializeStream(){
}

SerializeStream *StdSerializeStream::substream(){
	StdSerializeSubStream *ret = new StdSerializeSubStream(*this);
	return ret;
}

void StdSerializeStream::join(tt *o){
	base << std::endl;
}




BinSerializeStream::~BinSerializeStream(){
	::free(buf);
}
template<typename T>
inline SerializeStream &BinSerializeStream::write(T a){
	buf = (unsigned char*)::realloc(buf, size += sizeof a);
	*(T*)&buf[size - sizeof a] = a;
	return *this;
}

SerializeStream &BinSerializeStream::operator <<(int a){return write(a);}
SerializeStream &BinSerializeStream::operator <<(unsigned a){return write(a);}
SerializeStream &BinSerializeStream::operator <<(long a){return write(a);}
SerializeStream &BinSerializeStream::operator <<(unsigned long a){return write(a);}
SerializeStream &BinSerializeStream::operator <<(bool a){return write(a);}
SerializeStream &BinSerializeStream::operator <<(float a){return write(a);}
SerializeStream &BinSerializeStream::operator <<(double a){return write(a);}

SerializeStream &BinSerializeStream::operator <<(const char *a){
	size_t len = ::strlen(a) + 1;
	buf = (unsigned char*)::realloc(buf, size += len);
	::memcpy(&buf[size - len], a, len);
	return *this;
}

SerializeStream &BinSerializeStream::operator<<(const std::string &a){
	size_t len = a.length() + 1;
	buf = (unsigned char*)::realloc(buf, size += len);
	::memcpy(&buf[size - len], a.c_str(), len);
	return *this;
}

SerializeStream &BinSerializeStream::operator <<(const Serializable *p){return write(sc->map[p]);}

SerializeStream &BinSerializeStream::operator<<(const Vec3d &v){
	return *this << v[0] << v[1] << v[2];
}

SerializeStream &BinSerializeStream::operator<<(const Quatd &v){
	return *this << v.i() << v.j() << v.k() << v.re();
}

SerializeStream &BinSerializeStream::operator<<(const struct ::random_sequence &rs){
	return *this << rs.w << rs.z;
}

SerializeStream &BinSerializeStream::write(const BinSerializeStream &o){
	buf = (unsigned char*)::realloc(buf, size += o.size);
	::memcpy(&buf[size - o.size], o.buf, o.size);
	return *this;
}

SerializeStream *BinSerializeStream::substream(){
	return new BinSerializeStream(sc);
}

void BinSerializeStream::join(tt *o){
	*this << (unsigned long)((BinSerializeStream*)o)->size;
	write(*(BinSerializeStream*)o);
}






class StdUnserializeSubStream : public StdUnserializeStream{
	std::istringstream src;
public:
	StdUnserializeSubStream(std::string &s, StdUnserializeStream &aparent) :
		src(s),
		StdUnserializeStream(src, aparent.usc){}
};

UnserializeStream &StdUnserializeStream::consume(const char *cstr){
	size_t len = ::strlen(cstr);
	for(size_t i = 0; i < len; i++){
		char c = base.get();
		if(c != cstr[i])
			throw FormatException();
	}
	return *this;
}

bool StdUnserializeStream::eof()const{ return base.eof(); }
bool StdUnserializeStream::fail()const{ return base.fail(); }
UnserializeStream &StdUnserializeStream::read(char *s, std::streamsize size){ base.read(s, size); return *this; }
UnserializeStream &StdUnserializeStream::operator>>(int &a){ base.operator>>(a); if(!fail()) consume(" "); return *this; }
UnserializeStream &StdUnserializeStream::operator>>(unsigned &a){ base.operator>>(a); if(!fail()) consume(" "); return *this; }
UnserializeStream &StdUnserializeStream::operator>>(long &a){ base.operator>>(a); if(!fail()) consume(" "); return *this; }
UnserializeStream &StdUnserializeStream::operator>>(unsigned long &a){ base.operator>>(a); if(!fail()) consume(" "); return *this; }
UnserializeStream &StdUnserializeStream::operator>>(bool &a){ int i; base.operator>>(i); if(!fail()) consume(" "); a = !!i; return *this; }
UnserializeStream &StdUnserializeStream::operator>>(float &a){ base.operator>>(a); if(!fail()) consume(" "); return *this; }
UnserializeStream &StdUnserializeStream::operator>>(double &a){ base.operator>>(a); if(!fail()) consume(" "); return *this; }

UnserializeStream &StdUnserializeStream::operator>>(const char *cstr){
	// eat until the first double-quote
	while(char c = base.get()) if(c == '"')
		break;
	else if(c == -1)
		return *this;

	consume(cstr);

	// eat the terminating double-quote
	if(base.get() != '"')
		throw FormatException();
	return *this;
}

UnserializeStream &StdUnserializeStream::operator>>(random_sequence &rs){
	consume("(");
	*this >> rs.w >> rs.z;
	consume(")");
	return *this;
}

UnserializeStream &StdUnserializeStream::operator>>(Vec3d &v){
	consume("(");
	*this >> v[0] >> v[1] >> v[2];
	consume(")");
	return *this;
}

UnserializeStream &StdUnserializeStream::operator>>(Quatd &v){
	consume("(");
	*this >> v.i() >> v.j() >> v.k() >> v.re();
	consume(")");
	return *this;
}

UnserializeStream &StdUnserializeStream::operator >>(cpplib::dstring &a){
	// eat until the first double-quote
	while(char c = base.get()) if(c == '"')
		break;
	else if(c == -1)
		return *this;

	do{
		char c = base.get();
		if(c == '\\')
			c = base.get();
		else if(c == '"')
			break;
		a << c;
	}while(true);
	return *this;
}

UnserializeStream *StdUnserializeStream::substream(size_t size){
	char *buf = new char[size+1];
	base.read(buf, size);
	std::string str(buf, size);
	delete[] buf;
	StdUnserializeStream *ret = new StdUnserializeSubStream(str, *this);
	return ret;
}










template<typename T> UnserializeStream &BinUnserializeStream::read(T &a){
	if(size < sizeof a)
		throw InputShortageException();
	::memcpy(&a, src, sizeof a);
	src += sizeof a;
	size -= sizeof a;
	return *this;
}

BinUnserializeStream::BinUnserializeStream(const unsigned char *asrc, size_t asize, UnserializeContext *ausc) : src(asrc), size(asize), tt(ausc){
}

UnserializeStream &BinUnserializeStream::read(char *s, std::streamsize ssize){
	if(size < (size_t)ssize)
		throw InputShortageException();
	::memcpy(s, src, ssize);
	src += ssize;
	size -= ssize;
	return *this;
}

bool BinUnserializeStream::eof()const{return !size;}
bool BinUnserializeStream::fail()const{return !size;}
UnserializeStream &BinUnserializeStream::operator>>(int &a){return read(a);}
UnserializeStream &BinUnserializeStream::operator>>(unsigned &a){return read(a);}
UnserializeStream &BinUnserializeStream::operator>>(long &a){return read(a);}
UnserializeStream &BinUnserializeStream::operator>>(unsigned long &a){return read(a);}
UnserializeStream &BinUnserializeStream::operator>>(bool &a){return read(a);}
UnserializeStream &BinUnserializeStream::operator>>(float &a){return read(a);}
UnserializeStream &BinUnserializeStream::operator>>(double &a){return read(a);}

UnserializeStream &BinUnserializeStream::operator>>(random_sequence &rs){
	*this >> rs.w >> rs.z;
	return *this;
}

UnserializeStream &BinUnserializeStream::operator>>(Vec3d &v){
	*this >> v[0] >> v[1] >> v[2];
	return *this;
}

UnserializeStream &BinUnserializeStream::operator>>(Quatd &v){
	*this >> v.i() >> v.j() >> v.k() >> v.re();
	return *this;
}

UnserializeStream &BinUnserializeStream::operator>>(const char *a){
	size_t len = ::strlen(a) + 1;
	if(size < len)
		throw InputShortageException();
	if(::strncmp(reinterpret_cast<const char*>(src), a, len))
		throw FormatException();
	src += len;
	size -= len;
	return *this;
}

UnserializeStream &BinUnserializeStream::operator>>(cpplib::dstring &a){
	char c;
	while(c = get()){
		if(c == -1)
			throw InputShortageException();
		a << c;
	}
	return *this;
}

UnserializeStream *BinUnserializeStream::substream(size_t size){
	const unsigned char *retsrc = src;

	// advance pointers
	src += size;
	this->size -= size;

	return new BinUnserializeStream(retsrc, size, usc);
}

char BinUnserializeStream::get(){
	if(size == 0)
		return -1; // EOF
	size--;
	return *src++;
}








char *strnewdup(const char *src, size_t len){
	char *ret = new char[len + 1];
	::strncpy(ret, src, len + 1);
	ret[len] = '\0';
	return ret;
}

char *strmdup(const char *src, size_t len){
	char *ret = (char*)malloc(len + 1);
	::strncpy(ret, src, len + 1);
	ret[len] = '\0';
	return ret;
}

