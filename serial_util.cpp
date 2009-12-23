#include "serial_util.h"
extern "C"{
#include <clib/rseq.h>
}
using namespace std;


SerializeStream &SerializeStream::operator<<(const Vec3d &v){
	return *this << "(" << v[0] << " " << v[1] << " " << v[2] << ")";
}

SerializeStream &SerializeStream::operator<<(const Quatd &v){
	return *this << "(" << v.i() << " " << v.j() << " " << v.k() << " " << v.re() << ")";
}

SerializeStream &SerializeStream::operator<<(const struct ::random_sequence &rs){
	return *this << "(" << rs.w << " " << rs.z << ")";
}





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

