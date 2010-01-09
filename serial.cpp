#include "serial.h"
#include "cmd.h"
#include "serial_util.h"
#include <string>
#include <sstream>

UnserializeContext::UnserializeContext(UnserializeStream &ai, CtorMap &acons, UnserializeMap &amap)
: i(ai), cons(acons), map(amap){
}


Serializable::~Serializable(){}

void Serializable::serialize(SerializeContext &sc){
}

void Serializable::unserialize(UnserializeContext &usc){
}

void Serializable::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	(this->*method)(sc);
}

void Serializable::map(SerializeContext &sc){
	SerializeMap &cm = sc.map;
	if(cm.find(this) == cm.end()){
		unsigned id = cm.size();
		cm[this] = id;
	}
}

void Serializable::packSerialize(SerializeContext &sc){
	if(visit_list) // avoid duplicate objects
		return;
	visit_list = sc.visit_list; // register itself as visited
	sc.visit_list = this;
/*	if(sc.visits.find(this) != sc.visits.end()){
		return; // avoid duplicate objects
	}
	sc.visits.insert(this);*/
	SerializeStream *ss = sc.o.substream();
	SerializeContext sc2(*ss, sc);
	sc2.o << classname();
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
