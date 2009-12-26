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
//	std::stringstream ss;
//	StdSerializeStream sss(ss);
	BinSerializeStream bss;
	SerializeContext sc2(bss, sc);
	bss.sc = &sc2;
//	sss.StdSerializeStream::StdSerializeStream(ss, sc2);
	serialize(sc2);
//	sc.o << ss.str().size() << " " << ss.str() << "\n";
	sc.o << bss.getsize();
	((BinSerializeStream&)sc.o).write(bss);
}

void Serializable::packUnserialize(UnserializeContext &sc){
	unsigned size;
	sc.i >> size;
/*	if(sc.i.eof() || sc.i.fail())
		return;
	sc.i >> " ";*/
/*	std::istringstream ss(std::string(buf, size));
	StdUnserializeStream sus(ss);*/
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
