#ifndef APPLICATION_H
#define APPLICATION_H
/** \file
 * \brief Definition of Client class.
 */

#include "Server.h"
#include "Game.h"

/// \brief The base class of singleton object of various application types.
///
/// The application has different properties and methods depending on whether
/// it is the client or the dedicated server, but we want to treat it somehow uniformly.
/// So we made it a class and constructed class hierarchy to achieve this.
class Application{
public:
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

	/// The game mode
	volatile enum GameMode mode;

	/// \brief The server game object.
	///
	/// Available only if the game is running in a dedicated server or a standalone client.
	Game *serverGame;

	/// \brief The client's game, a copy of the server in the client process.
	///
	/// Available only if the game is running in a standalone or remote client.
	/// If it's a standalone client, this object is a copy of the object indicated by pg.
	Game *clientGame;

	/// Embedded server thread
	ServerThreadHandle server;

	/// \brief The connection socket.
	///
	/// Only meaningful in Client, but placed here to simplify codes that'll need reinterpret_cast otherwise.
	SOCKET con;

	bool isClient;

	/// Parameters required for starting a Server.
	ServerParams serverParams;

	/// Login name for joining a remote game.
	gltestp::dstring loginName;

	/// The file pointer to write chat logs.
	FILE *logfile;

	void init(bool isClient);

	bool parseArgs(int argc, char *argv[]);

	/// \brief Try to start a server game.
	/// \param game The game object to host as a server.
	/// \param port Port the server listen to.
	/// \returns False if failed to start server.
	bool hostgame(Game *game, int port);

	/// \brief Send chat message to the server.
	void sendChat(const char *buf);

	/// \brief Called when the application receives a message from the server object, typically a chat message.
	///
	/// The default is to output to console. Override in the derived classes to implement the method.
	virtual void signalMessage(const char *text);

	/// \brief Called when the application needs to report something bad to the user.
	///
	/// The default is to write to stdout. Override in the derived classes to implement the method.
	virtual void errorMessage(const char *);

protected:
	/// Make this singleton by protecting the constructor.
	Application();
	~Application();
};

#ifdef _WIN32

/// \brief Representation of Client in the client process.
///
/// Only available in Windows client.
class ClientApplication : public Application{
public:

	/// \biref Client registry in the Client process.
	struct ClientClient{
		gltestp::dstring name;
		struct sockaddr_in tcp;
	};
	typedef std::vector<ClientClient> ClientList;

	HWND w;
	HANDLE hDrawThread; // the drawing thread
//	HANDLE hWaveThread; // is it necessary??
//	HANDLE hDrawEvent;
	HANDLE hGameMutex; /* game data storage must be mutually exclusive */
//	void *offbuf;
	ClientList ad;
	unsigned thisad;
//	int mousemode;
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
	std::list<std::vector<unsigned char> > recvbuf; ///< Receive buffer queue. Mutex locked by hGameMutex.

	ClientApplication();
	~ClientApplication();
	int joingame(const char *host, int port);
	void display_func();
	void mouse_func(int button, int state, int x, int y);
	void signalMessage(const char *text);
	void errorMessage(const char *str);
};

extern ClientApplication application;

#else

/// \brief Representation of a dedicated server process.
///
/// Only available in Linux dedicated server.
class DedicatedServerApplication : public Application{

};

extern DedicatedServerApplication application;

#endif

/// \brief The Client Messages are sent from the client to the server, to ask something the client wants to interact with the
/// server world.
///
/// The overriders should be sigleton class, which means only one instance of that class is permitted.
///
/// The overriders registers themselves with the constructor of ClientMessage and de-register in the destructor.
struct ClientMessage{

	/// Type for the constructor map.
	typedef std::map<dstring, ClientMessage*> CtorMap;

	/// Constructor map. The key must be a pointer to a static string, which lives as long as the program.
	static CtorMap &ctormap();

	/// The virtual function that defines how to interpret serialized stream.
	virtual void interpret(ServerClient &sc, UnserializeStream &uss) = 0;

protected:
	/// The id (name) of this ClientMessage automatically sent and matched.
	dstring id;

	ClientMessage(dstring id);
	virtual ~ClientMessage();

	void send(Application &, const void *, size_t);
};


#endif
