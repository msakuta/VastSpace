#ifndef SERIAL_H
#define SERIAL_H
#include "export.h"
#include "dstring.h"
#include <map>
#include <vector>
#include <iostream>




class Serializable;
typedef const char *ClassId;
typedef std::map<gltestp::dstring, Serializable *(*)()> CtorMap;
typedef std::map<const Serializable*, unsigned> SerializeMap;
typedef std::vector<Serializable*> UnserializeMap;

class SerializeContext;
class UnserializeContext;


/** \brief Base class of all serializable objects.
 *
 * Serializable objects can be saved to and loaded from a file.
 * 
 * It will be able to synchronize between server and client when game over network
 * is implemented.
 *
 * It is also handled in Squirrel codes.
 */
class EXPORT Serializable{
public:
	/// Virtual destructor defined to make all derived classes have default destructor.
	virtual ~Serializable();

	/** \brief Returns class name.
	 *
	 * Returned string storage must be static, which means lives as long as the program.
	 */
	virtual const char *classname()const = 0;

	virtual void dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &));

	void map(SerializeContext &sc);

	/// \brief Serialize this object into given stream in the context.
	///
	/// It effectively adds class identification information (i.e. class name) prior to calling serialize virtual function.
	void packSerialize(SerializeContext &sc);

	/// \brief Unserialize this object from a stream.
	///
	/// Size and class identification is checked.
	void packUnserialize(UnserializeContext &sc);

	/// \brief Serialize this object into given stream in the context.
	///
	/// It effectively adds class identification information and object identification prior to calling serialize virtual function.
	void idPackSerialize(SerializeContext &sc);

	/// \brief Unserialize this object from a stream.
	///
	/// Size and class identification is checked.
	void idPackUnserialize(UnserializeContext &sc);

	/// \brief Binds a class ID with its constructor.
	///
	/// Derived classes must register themselves to unserialize.
	static unsigned registerClass(ClassId id, Serializable *(*constructor)());

	/// \brief Unbind name and class.
	///
	/// Note that if the same name is registered from multiple modules, they compare different as
	/// pointers to these names that reside in separate modules.
	static void unregisterClass(ClassId id);

	/// \brief Returns global constructor map.
	///
	/// Registered classes via registerClass reside in this map.
	static CtorMap &ctormap();

	/// \brief Template helper function that generate global constructor of given class.
	template<class T> static Serializable *Conster();

	/// Clear visit list used to eliminate duplicate visits.
	void clearVisitList()const;

protected:
	/// \brief Serialize this object using given serialize context.
	///
	/// Derived classes override this function to define how to serialize themselves.
	virtual void serialize(SerializeContext &sc);

	/// \brief Restore this object using given context.
	///
	/// Derived classes override this function to define how to restore themselves from serialized stream.
	virtual void unserialize(UnserializeContext &usc);

	Serializable() : visit_list(NULL){}
	mutable Serializable *visit_list; ///< Visit list for object network diving. Must be initially NULL and NULLified after use.
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