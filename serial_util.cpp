#include "serial_util.h"


std::ostream &operator<<(std::ostream &o, Vec3d &v){
	o << "(" << v[0] << " " << v[1] << " " << v[2] << ")";
	return o;
}

std::ostream &operator<<(std::ostream &o, Quatd &v){
	o << "(" << v[0] << " " << v[1] << " " << v[2] << " " << v[3] << ")";
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
	o >> "(" >> v[0] >> " " >> v[1] >> " " >> v[2] >> " " >> v[3] >> ")";
	return o;
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

