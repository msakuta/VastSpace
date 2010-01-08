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
	virtual ~Serializable(); // Virtual destructor defined to make all derived classes have default destructor.
	virtual const char *classname()const = 0; // returned string storage must be static
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &usc);
	virtual void dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &));	// Serialize the graph recursively.
	void map(SerializeContext &sc);
	void packSerialize(SerializeContext &sc);
	void packUnserialize(UnserializeContext &sc);
	static unsigned registerClass(std::string name, Serializable *(*)());
	static CtorMap &ctormap();
	template<class T> static Serializable *Conster();
	void clearVisitList()const;
protected:
	Serializable() : visit_list(NULL){}
	mutable Serializable *visit_list; // Visit list for object network diving. Must be initially NULL and NULLified after use.
};

template<class T> inline Serializable *Serializable::Conster(){
	return new T();
};

inline void Serializable::clearVisitList()const{
	const Serializable *p = this;
	while(p){
		const Serializable *next = p->visit_list;
		p->visit_list = NULL;
		p = next;
	};
}

#endif
