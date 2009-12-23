#include "serial_util.h"
extern "C"{
#include <clib/rseq.h>
}
using namespace std;


UnserializeStream &UnserializeStream::operator>>(const char *cstr){
	size_t len = ::strlen(cstr);
	for(size_t i = 0; i < len; i++){
		char c = base.get();
		if(c != cstr[i])
			throw std::exception("Format error");
	}
	return *this;
}

UnserializeStream &UnserializeStream::operator>>(random_sequence &rs){
	*this >> "(" >> rs.w >> " " >> rs.z >> ")";
	return *this;
}

UnserializeStream &UnserializeStream::operator>>(Vec3d &v){
	*this >> "(" >> v[0] >> " " >> v[1] >> " " >> v[2] >> ")";
	return *this;
}

UnserializeStream &UnserializeStream::operator>>(Quatd &v){
	*this >> "(" >> v.i() >> " " >> v.j() >> " " >> v.k() >> " " >> v.re() >> ")";
	return *this;
}









std::ostream &operator<<(std::ostream &o, Vec3d &v){
	o << "(" << v[0] << " " << v[1] << " " << v[2] << ")";
	return o;
}

std::ostream &operator<<(std::ostream &o, const Quatd &v){
	o << "(" << v.i() << " " << v.j() << " " << v.k() << " " << v.re() << ")";
	return o;
}

ostream &operator<<(ostream &o, const struct ::random_sequence &rs){
	o << "(" << rs.w << " " << rs.z << ")";
	return o;
}

std::istream &operator>>(std::istream &o, const char *cstr){
	size_t len = ::strlen(cstr);
	for(size_t i = 0; i < len; i++){
		char c = o.get();
		if(c != cstr[i])
			throw std::exception("Format error");
	}
	return o;
}

std::istream &operator>>(std::istream &o, Vec3d &v){
	o >> "(" >> v[0] >> " " >> v[1] >> " " >> v[2] >> ")";
	return o;
}

std::istream &operator>>(std::istream &o, Quatd &v){
	o >> "(" >> v.i() >> " " >> v.j() >> " " >> v.k() >> " " >> v.re() >> ")";
	return o;
}

std::istream &operator>>(std::istream &o, struct random_sequence &rs){
	o >> "(" >> rs.w >> " " >> rs.z >> ")";
	return o;
}

cpplib::dstring readUntil(UnserializeStream &in, char c){
	cpplib::dstring ret;
	do{
		char c = in.get();
		if(c == ')')
			break;
		ret << c;
	}while(true);
	in.unget();
	return ret;
}

cpplib::dstring readUntil(std::istream &in, char c){
	cpplib::dstring ret;
	do{
		char c = in.get();
		if(c == ')')
			break;
		ret << c;
	}while(true);
	in.unget();
	return ret;
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

