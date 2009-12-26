#include "serial.h"
#include "cmd.h"
#include "serial_util.h"
#include <string>
#include <sstream>

UnserializeContext::UnserializeContext(UnserializeStream &ai, CtorMap &acons, UnserializeMap &amap)
: i(ai), cons(acons), map(amap){
}


void Serializable::serialize(SerializeContext &sc){
	sc.o << classname();
}

void Serializable::unserialize(UnserializeContext &usc){
}

void Serializable::packSerialize(SerializeContext &sc){
	SerializeStream *ss = sc.o.substream();
	SerializeContext sc2(*ss, sc);
	serialize(sc2);
	sc.o.join(ss);
	delete ss;
}

void Serializable::packUnserialize(UnserializeContext &sc){
	unsigned size;
	sc.i >> size;
	UnserializeStream *us = sc.i.substream(size);
	UnserializeContext sc2(*us, sc.cons, sc.map);
	us->usc = &sc2;
	sc2.i >> classname();
	unserialize(sc2);
	delete us;
}

unsigned Serializable::registerClass(std::string name, Serializable *(*constructor)()){
	if(ctormap().find(name) != ctormap().end())
		CmdPrintf(cpplib::dstring("WARNING: Duplicate class name: ") << name.c_str());
	ctormap()[name] = constructor;
	return ctormap().size();
}

CtorMap &Serializable::ctormap(){
	static CtorMap ictormap;
	return ictormap;
}
