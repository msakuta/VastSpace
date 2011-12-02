#ifndef CLIENT_H
#define CLIENT_H
/** \file
 * \brief Definition of Client class.
 */

#include "Server.h"
#include "Game.h"


/// \brief Representation of Client in the client process.
struct Client{
	enum GameMode{
		ChatBit   = 0x80, /* bitmask for game mode that allow chatting or logging */
		WaitBit   = 0x01, /* bitmask for waiting rooms */
		ServerBit = 0x04, /* server thread running (not standalone!) */

		InactiveGame   = 0x00, /* no game is progressing */
		StandaloneGame = 0x02, /* single player */
		ServerWaitGame = 0x84, /* hosting game over network, waiting clients to join */
		ServerGame     = 0x85, /* hosting game, running. no more clients can join */
		ClientWaitGame = 0x82, /* joinning game, waiting */
		ClientGame     = 0x83, /* playing game on remote server */
	};

	/// \biref Client registry in the Client process.
	struct ClientClient{
		gltestp::dstring name;
		unsigned ip;
		unsigned port;
	};

	HWND w;
	HANDLE hDrawThread; // the drawing thread
//	HANDLE hWaveThread; // is it necessary??
//	HANDLE hDrawEvent;
	HANDLE hGameMutex; /* game data storage must be mutually exclusive */
//	void *offbuf;
	volatile enum GameMode mode;
	std::vector<ClientClient> ad;
	unsigned thisad;
//	int mousemode;
	Game *pg; ///< The server game
	Game *clientGame; ///< The client's game, a copy of the server in the client process.
//	Game::DrawData dd;
//	Game::AnimData ad;
//	Game::squadid meid; // Squad ID for the player
//	FILE *logfile; // file pointer to the local log file, if any
//	mutex_t mlogfile; // critical section would be enough, or even nothing
//	bool pause;
	bool shutclient;
//	HWAVEOUT hwo;
//	CRITICAL_SECTION cswave;
//	MMRESULT timer;
	ServerThreadHandle server; /* embedded server thread */
	SOCKET con; /* connection */
//	Waiter *waiter;
	HANDLE hRecvThread;
//	HWND hChatbox;
//	HWND hChatwnd;
	HFONT hf;
//	DrawWaitData wd;
//	Button buttons[3];
//	char attr[3];
//	int ncl; /* number of allocated pvlist */
//	PictVertexList *pvlist;
//	PictVertex *pvroot, *pvfree, vertices[128];

	Client();
	~Client();
	void hostgame(Game *);
	int joingame();
	void display_func();
	void mouse_func(int button, int state, int x, int y);
};

/// Type to identify message classes.
/// No two message classes shall share this value.
typedef const char *ClientMessageID;

struct ClientMessage;

typedef ClientMessage *ClientMessageCreatorFunc(const dstring &);

/// Static data structure for Messages.
struct ClientMessageStatic{
//	ClientMessageCreatorFunc *newproc;
//	void (*deleteproc)(void*);
//	ClientMessageStatic(ClientMessageCreatorFunc a = NULL, void b(void*) = NULL) : newproc(a), deleteproc(b){}

	/// Type for the constructor map.
	typedef std::map<dstring, ClientMessageStatic*> CtorMap;

	/// Constructor map. The key must be a pointer to a static string, which lives as long as the program.
	static CtorMap &ctormap();

	virtual void interpret(ServerClient &sc, UnserializeStream &uss) = 0;
protected:
	dstring id;

	ClientMessageStatic(dstring id);
	~ClientMessageStatic();

	void send(Client &, const void *, size_t);
};

#if 0
template<typename ClientMessageDerived> struct ClientMessageRegister : public ClientMessageStatic{
	ClientMessageRegister(ClientMessageCreatorFunc a, void b(void*)) : ClientMessageStatic(a, b){
		ClientMessageDerived::registerMessage(ClientMessageDerived::sid, this);
	}
};

template<typename Command>
ClientMessage *ClientMessageCreator(const dstring &ds){
	return new Command(ds);
}

template<typename Command>
void ClientMessageDeletor(void *pv){
	delete pv;
}

/// \brief Message sent from the client over the network.
///
/// Base class for all messages.
struct EXPORT ClientMessage{
	/// Type for the constructor map.
	typedef std::map<const char *, ClientMessageStatic*, bool (*)(const char *, const char *)> CtorMap;

	/// Constructor map. The key must be a pointer to a static string, which lives as long as the program.
	static CtorMap &ctormap();

	/** \brief Returns unique ID for this class.
	 *
	 * The returned pointer never be dereferenced without debugging purposes,
	 * it is just required to point the same address for all the instances but
	 * never coincides between different classes.
	 * A static const string of class name is ideal for this returned vale.
	 */
	virtual ClientMessageID id()const = 0;

	/** \brief Derived or exact class returns true.
	 *
	 * Returns whether the given message ID is the same as this object's class's or its derived classes.
	 */
	virtual bool derived(ClientMessageID)const;

	virtual dstring encode()const;

	virtual void interpret(ServerClient &);

	ClientMessage(){}

	/// Derived classes use this utility to register class.
	static int registerMessage(const char *name, ClientMessageStatic *stat){
		ctormap()[name] = stat;
		return 0;
	}
};
#endif


#endif
