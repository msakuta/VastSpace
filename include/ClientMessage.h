#ifndef CLIENTMESSAGE_H
#define CLIENTMESSAGE_H
/** \file
 * \brief Definition of ClientMessage class.
 */
#include "export.h"
#include <squirrel.h>
#include <map>


class Application;

/// \brief The Client Messages are sent from the client to the server, to ask something the client wants to interact with the
/// server world.
///
/// The overriders should be sigleton class, which means only one instance of that class is permitted.
///
/// The overriders registers themselves with the constructor of ClientMessage and de-register in the destructor.
struct EXPORT ClientMessage{

	/// Type for the constructor map.
	typedef std::map<dstring, ClientMessage*> CtorMap;

	/// Constructor map. The key must be a pointer to a static string, which lives as long as the program.
	static CtorMap &ctormap();

	/// The virtual function that defines how to interpret serialized stream.
	virtual void interpret(ServerClient &sc, UnserializeStream &uss) = 0;

	/// The virtual function that defines how to interpret serialized stream.
	virtual bool sq_send(Application &, HSQUIRRELVM v)const;

protected:
	/// The id (name) of this ClientMessage automatically sent and matched.
	dstring id;

	ClientMessage(dstring id);
	virtual ~ClientMessage();

	void send(Application &, const void *, size_t);
};


#endif
