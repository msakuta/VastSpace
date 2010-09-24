#include "serial.h"
#include "cmd.h"
#include "serial_util.h"
#include "sqadapt.h"
#include <string>
#include <sstream>

UnserializeContext::UnserializeContext(UnserializeStream &ai, CtorMap &acons, UnserializeMap &amap)
: i(ai), cons(acons), map(amap){
}


Serializable::~Serializable(){
	if(g_sqvm)
		sqa_deleteobj(g_sqvm, this);
}

void Serializable::serialize(SerializeContext &sc){
}

void Serializable::unserialize(UnserializeContext &usc){
}

/// \brief Serializes or maps an object graph, which means arbitrary network of serializable objects.
///
/// This function recursively enumerates all relevant objects of this object.
/// The second argument is called on all objects in the graph.
///
/// Objects with arbitrary pointers to each other forms a graph (see C++ FAQ lite), so we must take 2 passes
/// to fully serialize them.
///
/// Derived classes must override this function to dive into all member pointers to Serializable-derived class object.
///
/// \param method member pointer to execute on all nodes in Serializable tree. Can be serialize() or map().
///
void Serializable::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	(this->*method)(sc);
}

/// \brief Add this object to the map of pointer and ID for serialization.
///
/// Serialized stream identify objects by ID numbers, not pointers in memory. Therefore, all objects must
/// translate themselves into an ID prior to serialization.
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

/// \param name name of the class.
/// \param constructor function that creates the object of class.
/// class constructor cannot be passed directly, use Conster() template function to generate a function
/// that can be passed here.
/// \sa Conster(), ctormap()
unsigned Serializable::registerClass(std::string name, Serializable *(*constructor)()){
	if(Serializable::ctormap().find(name) != Serializable::ctormap().end())
		CmdPrintf(cpplib::dstring("WARNING: Duplicate class name: ") << name.c_str());
	Serializable::ctormap()[name] = constructor;
	return Serializable::ctormap().size();
}

/// Using construction on first use idiom so it's safe to call from other initialization functions.
CtorMap &Serializable::ctormap(){
	static CtorMap ictormap;
	return ictormap;
}
