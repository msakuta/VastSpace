#ifndef SERIAL_H
#define SERIAL_H
#include "export.h"
#include "dstring.h"
#include <map>
#include <vector>
#include <iostream>


/// This type should match the definition of Serializable::Id.
typedef unsigned SerializableId;

class Serializable;
typedef const char *ClassId;
typedef std::map<gltestp::dstring, Serializable *(*)()> CtorMap;
typedef std::map<const Serializable*, SerializableId> SerializeMap;
typedef std::map<SerializableId, Serializable*> UnserializeMap;

class SerializeContext;
class UnserializeContext;
class Game;
class ServerGame;


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
	typedef SerializableId Id;

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

	Id getid()const{return id;}

	/// \brief Returns the game object this Serializable object belongs to.
	///
	/// Provided because there could be multiple Game instances in a process.
	/// The overhead caused by an almost-redundant pointer is assumed negligible,
	/// because it does not need to be serialized.
	///
	/// In fact, we have been able to make it without this pointer for hundreds of
	/// Subversion revisions, which means it's not really necessary.
	/// But we came to think that we need an universal way to instantly query an object
	/// is in a client or server game object. We also want to know if an object being
	/// constructed is really created or just allocated for further unserialization.
	/// If it's the former, we must fetch the id via game->nextId().
	///
	/// Normally the game reference in a Serializable object should be set in the constructor and
	/// never be altered. For this purpose, a C++ reference is suitable, but Serializable object
	/// cannot contain reference member because it must be 'raw' created before unserialized.
	/// In that case, the constructor is invoked with no arguments.
	Game *getGame()const{return game;}

protected:
	/// \brief Serialize this object using given serialize context.
	///
	/// Derived classes override this function to define how to serialize themselves.
	virtual void serialize(SerializeContext &sc);

	/// \brief Restore this object using given context.
	///
	/// Derived classes override this function to define how to restore themselves from serialized stream.
	virtual void unserialize(UnserializeContext &usc);

	Serializable(Game *game = NULL);
	mutable Serializable *visit_list; ///< Visit list for object network diving. Must be initially NULL and NULLified after use.
	Game *game; ///< The game object this Serializable object belongs to.
	Id id; ///< The number shared among server and clients to identify the object regardless of memory address.

	friend class Game;
	friend class ServerGame;
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
