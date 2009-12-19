#include "serial.h"
#include <string>


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
