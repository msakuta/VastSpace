#ifndef MSG_MESSAGE_H
#define MSG_MESSAGE_H
#include "export.h"
#include <squirrel.h>
#include <cpplib/vec3.h>
#include <stddef.h>
#include <vector>

// Forward declaration
struct Message;

/// Type to identify message classes.
/// No two message classes shall share this value.
typedef const char *MessageID;

/// Macro to define a message class deriving base.
#define DERIVE_MESSAGE(name,base) struct EXPORT name : public base{\
	typedef base st;\
	static MessageRegister<name> messageRegister;\
	static MessageID sid;\
	virtual MessageID id()const;\
	virtual bool derived(MessageID)const;\
	name(){}\
	name(HSQUIRRELVM v, Entity &e) : st(v,e){}\
	friend class MessageRegister<name>;\
}

/// Macro to implement a message's member variables and functions.
#define IMPLEMENT_MESSAGE(name,idname) MessageID name::sid = idname;\
	MessageRegister<name> name::messageRegister(MessageCreator<name>, MessageDeletor<name>);\
	MessageID name::id()const{return sid;}\
	bool name::derived(MessageID aid)const{if(aid==sid)return true;else return st::derived(aid);}


typedef Message *MessageCreatorFunc(HSQUIRRELVM, Entity &);

/// Static data structure for Messages.
struct MessageStatic{
	MessageCreatorFunc *newproc;
	void (*deleteproc)(void*);
	MessageStatic(MessageCreatorFunc a = NULL, void b(void*) = NULL) : newproc(a), deleteproc(b){}
};

template<typename MessageDerived> struct MessageRegister : public MessageStatic{
	MessageRegister(MessageCreatorFunc a, void b(void*)) : MessageStatic(a, b){
		MessageDerived::registerMessage(MessageDerived::sid, this);
	}
};

template<typename Command>
Message *MessageCreator(HSQUIRRELVM v, Entity &e){
	return new Command(v, e);
}

template<typename Command>
void MessageDeletor(void *pv){
	delete reinterpret_cast<Command*>(pv);
}


/// Base class for all messages.
struct EXPORT Message{
	/// Constructor map. The key must be a pointer to a static string, which lives as long as the program.
	static std::map<const char *, MessageStatic*, bool (*)(const char *, const char *)> &ctormap();

	/** \brief Returns unique ID for this class.
	 *
	 * The returned pointer never be dereferenced without debugging purposes,
	 * it is just required to point the same address for all the instances but
	 * never coincides between different classes.
	 * A static const string of class name is ideal for this returned vale.
	 */
	virtual MessageID id()const = 0;

	/** \brief Derived or exact class returns true.
	 *
	 * Returns whether the given message ID is the same as this object's class's or its derived classes.
	 */
	virtual bool derived(MessageID)const;

	Message(){}

	/// In a Squirrel execution context, an object which will this command be sent to is known.
	Message(HSQUIRRELVM v, Entity &e){}

	/// Derived classes use this utility to register class.
	static int registerMessage(const char *name, MessageStatic *stat){
		ctormap()[name] = stat;
		return 0;
	}
};

/// Template function to test whether given message is of given type.
///
/// \return non-NULL if matched
template<typename CmdType>
CmdType *InterpretMessage(Message &com){
	return com.id() == CmdType::sid ? &static_cast<CmdType&>(com) : NULL;
}

/// Template function to test whether given message is offspring of given type.
///
/// \return non-NULL if argument object's class derives template argument.
template<typename CmdType>
CmdType *InterpretDerivedMessage(Message &com){
	return com.derived(CmdType::sid) ? &static_cast<CmdType&>(com) : NULL;
}


#endif
