#ifndef APPLICATION_H
#define APPLICATION_H
/** \file
 * \brief Definition of Application, ClientApplication and ServerApplication class.
 */

#include "Server.h"
#include "Game.h"

/// \brief The base class of singleton object of various application types.
///
/// The application has different properties and methods depending on whether
/// it is the client or the dedicated server, but we want to treat it somehow uniformly.
/// So we made it a class and constructed class hierarchy to achieve this.
class EXPORT Application{
public:
	enum GameMode{
		ClientBit = 0x01,
		ServerBit = 0x02,

		InactiveGame   = 0x00, ///< no game is progressing
		StandaloneGame = 0x03, ///< single player
		ClientGame     = 0x01, ///< Join another game
		ServerGame     = 0x02  ///< Host game
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
	/// If it's a standalone client, this object is a copy of the object indicated by serverGame.
	Game *clientGame;

	/// Embedded server thread
	ServerThreadHandle server;

	/// \brief The connection socket.
	///
	/// Only meaningful in Client, but placed here to simplify codes that'll need reinterpret_cast otherwise.
	SOCKET con;

	/// Parameters required for starting a Server.
	ServerParams serverParams;

	/// Login name for joining a remote game.
	gltestp::dstring loginName;

	/// The file pointer to write chat logs.
	FILE *logfile;

	/// \brief Initialize the application.
	/// \param vmInit Called just after serverGame or clientGame's Squirrel VM is set up.
	///               Intended for platform dependent VM initialization, namely debug hooks.
	void init(void vmInit(HSQUIRRELVM) = NULL);

	bool parseArgs(int argc, char *argv[]);

	/// \brief Try to start a server game.
	/// \param game The game object to host as a server.
	/// \param port Port the server listen to.
	/// \returns False if failed to start server.
	bool hostgame(Game *game, int port);

	/// \brief Starts a standalone game with the given game object.
	bool standalone(Game *game);

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

#ifndef DEDICATED

/// \brief Representation of Client in the client process.
///
/// Only available in Windows client.
class EXPORT ClientApplication : public Application{
public:

	/// \biref Client registry in the Client process.
	struct ClientClient{
		gltestp::dstring name;
		struct sockaddr_in tcp;
	};
	typedef std::vector<ClientClient> ClientList;

	struct JoinGameData{
		char addr[256];
		gltestp::dstring name;
		u_short port;
		FILE **plogfile;
	};

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

	size_t recvbytes;

	int mousedelta[2];

	ClientApplication();
	~ClientApplication();
	bool joinGame(const char *host, int port);
	bool startGame();
	void display_func();
	void mouse_func(int button, int state, int x, int y);
	void signalMessage(const char *text);
	void errorMessage(const char *str);

protected:
	JoinGameData data;
	SOCKET s;
};

EXPORT extern ClientApplication application;

#else

/// \brief Representation of a dedicated server process.
///
/// Only available in Linux dedicated server.
class DedicatedServerApplication : public Application{

};

extern DedicatedServerApplication application;

#endif

#endif
