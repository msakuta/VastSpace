#ifndef SERIAL_H
#define SERIAL_H
#include <map>
#include <vector>
#include <iostream>

class Serializable;
typedef std::map<std::string, Serializable *(*)()> CtorMap;

class SerializeContext{
public:
	SerializeContext(std::ostream &ao) : o(ao){}
	std::ostream &o;
	std::map<const Serializable*, unsigned> map;
};

class UnserializeContext{
public:
	UnserializeContext(std::istream &ai, CtorMap &acons, std::vector<Serializable*> &amap) : i(ai), cons(acons), map(amap){}
	std::istream &i;
	CtorMap &cons;
	std::vector<Serializable*> &map;
};

// Base class of all serializable objects.
class Serializable{
public:
	virtual const char *classname()const = 0; // returned string storage must be static
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &usc);
	static unsigned registerClass(std::string name, Serializable *(*)());
	static CtorMap &ctormap();
	template<class T> static Serializable *Conster();
};

template<class T> inline Serializable *Serializable::Conster(){
	return new T();
};

#endif
