#include "serial.h"
#include "serial_util.h"
#include <string>
#include <sstream>

UnserializeContext::UnserializeContext(std::istream &ai, CtorMap &acons, UnserializeMap &amap)
: i(ai, *this), cons(acons), map(amap){
}


void Serializable::serialize(SerializeContext &sc){
	sc.o << classname();
}

void Serializable::unserialize(UnserializeContext &usc){
}

void Serializable::packSerialize(SerializeContext &sc){
	std::stringstream ss;
	SerializeContext sc2(ss, sc);
	serialize(sc2);
	sc.o << ss.str().size() << " " << ss.str() << "\n";
}

void Serializable::packUnserialize(UnserializeContext &sc){
	char *buf;
	unsigned size;
	sc.i >> size;
	if(sc.i.eof() || sc.i.fail())
		return;
	sc.i >> " ";
	buf = new char[size + 1];
	sc.i.read(buf, size);
	buf[size] = '\0';
	char *p = ::strchr(buf, ' ');
	if(!p || ::strncmp(buf, classname(), p - buf))
		throw std::exception("Class name doen't match.");
	std::istringstream ss(std::string(p, size - (p - buf) + 1));
	UnserializeContext sc2(ss, sc.cons, sc.map);
	unserialize(sc2);
	delete[] buf;
}

unsigned Serializable::registerClass(std::string name, Serializable *(*constructor)()){
	ctormap()[name] = constructor;
	return ctormap().size();
}

CtorMap &Serializable::ctormap(){
	static CtorMap ictormap;
	return ictormap;
}
