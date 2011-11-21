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
};


#endif
