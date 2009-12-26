#include "serial_util.h"
extern "C"{
#include <clib/rseq.h>
}
#include <sstream>
using namespace std;


SerializeStream &StdSerializeStream::operator<<(const Vec3d &v){
	return *this << "(" << v[0] << " " << v[1] << " " << v[2] << ")";
}

SerializeStream &StdSerializeStream::operator<<(const Quatd &v){
	return *this << "(" << v.i() << " " << v.j() << " " << v.k() << " " << v.re() << ")";
}

SerializeStream &StdSerializeStream::operator<<(const struct ::random_sequence &rs){
	return *this << "(" << rs.w << " " << rs.z << ")";
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
SerializeStream &BinSerializeStream::operator <<(unsigned long a){return write(a);}
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








UnserializeStream &StdUnserializeStream::operator>>(const char *cstr){
	size_t len = ::strlen(cstr);
	for(size_t i = 0; i < len; i++){
		char c = base.get();
		if(c != cstr[i])
			throw std::exception("Format error");
	}
	return *this;
}

UnserializeStream &StdUnserializeStream::operator>>(random_sequence &rs){
	*this >> "(" >> rs.w >> " " >> rs.z >> ")";
	return *this;
}

UnserializeStream &StdUnserializeStream::operator>>(Vec3d &v){
	*this >> "(" >> v[0] >> " " >> v[1] >> " " >> v[2] >> ")";
	return *this;
}

UnserializeStream &StdUnserializeStream::operator>>(Quatd &v){
	*this >> "(" >> v.i() >> " " >> v.j() >> " " >> v.k() >> " " >> v.re() >> ")";
	return *this;
}

UnserializeStream &StdUnserializeStream::operator >>(cpplib::dstring &a){
	do{
		char c = base.get();
		if(c == '\\')
			c = base.get();
		else if(c == ')')
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
	std::istringstream iss(str);
	StdUnserializeStream *ret = new StdUnserializeStream(iss);
	return ret;
}










template<typename T> UnserializeStream &BinUnserializeStream::read(T &a){
	if(size < sizeof a)
		throw std::exception("Input stream shortage");
	::memcpy(&a, src, sizeof a);
	src += sizeof a;
	size -= sizeof a;
	return *this;
}

BinUnserializeStream::BinUnserializeStream(const unsigned char *asrc, size_t asize, UnserializeContext *ausc) : src(asrc), size(asize), tt(ausc){
}

UnserializeStream &BinUnserializeStream::read(char *s, std::streamsize ssize){
	if(size < (size_t)ssize)
		throw std::exception("Input stream shortage");
	::memcpy(s, src, ssize);
	src += ssize;
	size -= ssize;
	return *this;
}

bool BinUnserializeStream::eof()const{return !size;}
UnserializeStream &BinUnserializeStream::operator>>(int &a){return read(a);}
UnserializeStream &BinUnserializeStream::operator>>(unsigned &a){return read(a);}
UnserializeStream &BinUnserializeStream::operator>>(unsigned long &a){return read(a);}
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
		throw std::exception("Input stream shortage");
	if(::strncmp(reinterpret_cast<const char*>(src), a, len))
		throw std::exception("Format unmatch");
	src += len;
	size -= len;
	return *this;
}

UnserializeStream &BinUnserializeStream::operator>>(cpplib::dstring &a){
	char c;
	while(c = get()){
		if(c == -1)
			throw std::exception("Input stream shortage");
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

/*cpplib::dstring readUntil(UnserializeStream &in, char c){
	cpplib::dstring ret;
	do{
		char c = in.get();
		if(c == ')')
			break;
		ret << c;
	}while(true);
	in.unget();
	return ret;
}*/

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

