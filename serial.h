#ifndef SERIAL_H
#define SERIAL_H
#include <map>
#include <vector>
#include <iostream>

class Serializable;
typedef std::map<std::string, Serializable *(*)()> CtorMap;
typedef std::map<const Serializable*, unsigned> SerializeMap;
typedef std::vector<Serializable*> UnserializeMap;

class SerializeContext;
class UnserializeContext;


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
