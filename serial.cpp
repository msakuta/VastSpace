#include "serial.h"
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
	bss.BinSerializeStream::BinSerializeStream(sc2);
//	sss.StdSerializeStream::StdSerializeStream(ss, sc2);
	serialize(sc2);
//	sc.o << ss.str().size() << " " << ss.str() << "\n";
	sc.o << bss.getsize();
	((BinSerializeStream&)sc.o).write(bss);
}

void Serializable::packUnserialize(UnserializeContext &sc){
	char *buf;
	unsigned size;
	sc.i >> size;
/*	if(sc.i.eof() || sc.i.fail())
		return;
	sc.i >> " ";*/
	buf = new char[size + 1];
	sc.i.read(buf, size);
/*	buf[size] = '\0';
	char *p = ::strchr(buf, ' ');
	if(!p || ::strncmp(buf, classname(), p - buf))
		throw std::exception("Class name doen't match.");
	std::istringstream ss(std::string(p, size - (p - buf) + 1));*/
/*	std::istringstream ss(std::string(buf, size));
	StdUnserializeStream sus(ss);*/
	BinUnserializeStream bus;
	UnserializeContext sc2(bus, sc.cons, sc.map);
	bus.BinUnserializeStream::BinUnserializeStream((unsigned char*)buf, size, sc2);
//	sus.StdUnserializeStream::StdUnserializeStream(ss, sc2);
	sc2.i >> classname();
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
