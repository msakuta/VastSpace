#include "serial.h"
#include "serial_util.h"
#include <string>

UnserializeContext::UnserializeContext(std::istream &ai, CtorMap &acons, UnserializeMap &amap)
: i(ai, *this), cons(acons), map(amap){
}


void Serializable::serialize(SerializeContext &sc){
	sc.o << classname();
}

void Serializable::unserialize(UnserializeContext &usc){
}

unsigned Serializable::registerClass(std::string name, Serializable *(*constructor)()){
	ctormap()[name] = constructor;
	return ctormap().size();
}

CtorMap &Serializable::ctormap(){
	static CtorMap ictormap;
	return ictormap;
}
