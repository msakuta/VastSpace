#ifndef SERVER_H
#define SERVER_H
/** \file
 * \brief Definition of Server class and related classes and functions.
 */

#include "dstring.h"
#include "serial_util.h"
#include "BinDiff.h"
#include <stdio.h>

#include <list>
#include <vector>


#define MAX_HOSTNAME 128

#define FRAMETIME 50 /* millisecs */

/// The string identifying protocol itself.
#define PROTOCOL_SIGNATURE "GLTESTPLUS "

/** protocol version. If major version is different, no communication can
  be made. As for minor, I don't know. */
#define PROTOCOL_MAJOR 2
#define PROTOCOL_MINOR 0

/// The default port for the protocol.
#define PROTOCOL_PORT 6668

/// Print Mutex debugging information
#define muprintf

/* note that this header file is designed to be compatible among
  Windows platforms and others, typically GNU Linux. So it's here
  to get issues solved. */
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
typedef HANDLE event_t;
typedef UINT timer_t;
typedef HANDLE thread_t;
#	ifdef NDEBUG
typedef HANDLE mutex_t;
#	else
typedef struct{
	HANDLE m;
	int locks;
	const char *file; // deadlock detecter
	int line;
	DWORD thread;
} mutex_t;
#	endif
#define socketty() socket(AF_INET, SOCK_STREAM, 0)
#	ifdef NDEBUG
#define mutex_init(pm) (*(pm)=NULL)
#define mutex_isnull(pm) (!*(pm))
#define create_mutex(pm) (*(pm) = CreateMutex(NULL, FALSE, NULL))
#define delete_mutex(pm) CloseHandle(*(pm))
#define lock_mutex(pm) (WAIT_OBJECT_0 == WaitForSingleObject(*(pm), INFINITE))
#define unlock_mutex(pm) ReleaseMutex(*(pm))
#	else
#include <assert.h>
#define mutex_init(pm) ((pm)->m = NULL)
#define mutex_isnull(pm) (!(pm)->m)
#define create_mutex(pm) ((pm)->locks = 0, (pm)->m = CreateMutex(NULL, FALSE, NULL), (pm)->file = "", (pm)->line = 0, (pm)->thread = 0)
#define delete_mutex(pm) (CloseHandle((pm)->m), (pm)->m = NULL)
#define lock_mutex(pm) (WAIT_OBJECT_0 == WaitForSingleObject((pm)->m, INFINITE) && (assert(!(pm)->locks), (pm)->locks++, (pm)->file = __FILE__, (pm)->line = __LINE__, (pm)->thread = GetCurrentThreadId(), 1))
#define unlock_mutex(pm) (assert((pm)->locks == 1), (pm)->locks--, ReleaseMutex((pm)->m), (pm)->file = "", (pm)->line = 0, (pm)->thread = 0)
#	endif
#else /* linux */
#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define WINAPI
#define WSAGetLastError() errno
#define WSACleanup()
#define FreeConsole()
#define socketty() socket(PF_INET, SOCK_STREAM, 0)
#define closesocket(s) close(s)

/* pthread's mutex */
typedef struct mutex_s{
	pthread_mutex_t m;
	int init;
} mutex_t;
#define mutex_init(pm) ((pm)->init = 0)
#define mutex_isnull(pm) (!(pm)->init)
#define create_mutex(pm) (muprintf("miniting %p "__FILE__"(%d)\n", pm, __LINE__), pthread_mutex_init(&(pm)->m, NULL), ((pm)->init = 1))
#define delete_mutex(pm) (pthread_mutex_destroy(&(pm)->m) && !((pm)->init = 0))
bool LockMutex(mutex_t *pm, const char *source = NULL, int line = 0);
#define lock_mutex(pm) LockMutex(pm, __FILE__, __LINE__)
#define unlock_mutex(pm) (muprintf("munlocking %p "__FILE__"(%d)\n", pm, __LINE__), !pthread_mutex_unlock(&(pm)->m))

/*#define Sleep sleep*/
typedef struct thread_s{
	pthread_t p;
	int init;
} thread_t;
typedef unsigned long DWORD;
typedef void *LPVOID;
typedef int SOCKET;
typedef struct sockaddr SOCKADDR;

/* Events in Windows have effectively the same functionality as pairs of a condition variable
  and a mutex. */
typedef struct event_s{
	pthread_cond_t c;
	mutex_t m;
} event_t;

/* Linux timers are per-process, variable is dummy */
//typedef int timer_t;

#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (-1)
#endif

#ifdef _WIN32
static DWORD dummyThreadId;
#define thread_null(pt) (*(pt)=NULL)
#define thread_create(ret,proc,data) (*(ret)=CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(proc), (LPVOID)(data), 0, &dummyThreadId))
#define thread_join(pt) (WAIT_OBJECT_0 == WaitForSingleObject(*(pt), INFINITE) && (CloseHandle(*(pt)), 1))
#define thread_isvalid(t) (*(t))
#else
#define thread_null(pt) ((pt)->init = 0)
#define thread_create(ret,proc,data) (!pthread_create(&(ret)->p, NULL, (void*(*)(void*))proc, data) && ((ret)->init = 1))
#define thread_join(pt) (!pthread_join((pt)->p, NULL) && !((pt)->init = 0))
#define thread_isvalid(t) ((t)->init)
#endif


class Game;
struct Server;
class Application;

struct ServerThreadHandle{
	SOCKET listener;
	thread_t thread;
	Server *sv;
};
#define ServerThreadHandleInit(p) ((p)->listener = INVALID_SOCKET, (p)->thread = NULL)
#define ServerThreadHandleValid(p) thread_isvalid(&(p).thread)

/// \brief The data given to initialize the server object.
struct ServerParams{
	gltestp::dstring hostname;
	unsigned short port;
	int maxclients;
	Application *app; ///< Reference to the Application object used to notify server events to the application.
};

/// \brief The object represents a client in the server process.
struct ServerClient{
	Server *sv; ///< Redundant reference to the server
	bool kicked; ///< Flag indicating this client is being kicked.
	short team; ///< Team id to filter chat messages.
	unsigned long meid; ///< id of the player entity
	SOCKET s; ///< keep connection on this socket
	mutex_t m; ///< the mutex object to the socket, to make some longer messages atomic
	thread_t t; ///< Thread handle for receive thread for this client.
	int id; ///< Id of this Client.
	gltestp::dstring name; ///< Name of this client, used for chatting.
	struct sockaddr_in tcp, udp;
	/* if tcp connection is established, no need to store the address, but udp needs its destination */
};



/// \brief The object to represent the server in the server process itself.
struct Server{
	class Game *pg;
//	struct GameAnimData ad;
	char *hostname;
	thread_t animThread;
	event_t hAnimEvent;
	timer_t timer;
	Application *app; ///< Reference to the Application object used to notify server events to the application.
	SOCKET listener;
	typedef std::list<ServerClient> ServerClientList;
	ServerClientList cl; ///< Client list
	int maxncl; ///< Maximum size of cl allowed
	ServerClient *scs; ///< serverclient server (fake client indicating the server)
	void (*command_proc)(ServerClient *, char *);
	volatile long terminating; /* the server is shutting down! */
	volatile long started; /* game started; no further users can log in */
	mutex_t mcl; /* client list's mutex object */
	mutex_t mg; /* Game object access */
	ServerParams std;
	FILE *logfp;
	unsigned char *sendbuf;
	size_t sendbufsiz;
	SyncBuf syncbuf;

	Server();
	~Server();
	void WaitModified();
	ServerClient &addServerClient();
protected:
	void SendWait(ServerClient *cl);
};

extern int StartServer(ServerParams *, struct ServerThreadHandle *);
extern void StopServer(ServerThreadHandle *);
extern void SendChatServer(ServerThreadHandle *, const char *buf);
extern void KickClientServer(ServerThreadHandle *, int clid);



/// \brief The serialize stream object that is used to generate compressed stream for sending from the server to the client.
///
/// Defined here in Server.h because the server needs it anyway.
class DiffSectionStream : public SerializeStream{
public:
	typedef SerializeStream tt;
	typedef SerializeStream st;
	Serializable::Id id;
	BinDiff bd;

	DiffSectionStream(SerializeContext *asc, Serializable::Id id, const unsigned char *src, patchsize_t size) : st(asc), id(id), bd(src, size){}

	virtual tt &operator<<(int a);
	virtual tt &operator<<(unsigned a);
	virtual tt &operator<<(long a);
	virtual tt &operator<<(unsigned long a);
	virtual tt &operator<<(bool a);
	virtual tt &operator<<(float a);
	virtual tt &operator<<(double a);
	virtual tt &operator<<(const char *a);
	virtual tt &operator<<(const std::string &a);
	virtual tt &operator<<(const Serializable *p);
	virtual tt &operator<<(const Vec3d &v);
	virtual tt &operator<<(const Quatd &v);
	virtual tt &operator<<(const random_sequence &v);
	virtual tt *substream(Serializable::Id id){return this;}
	virtual void join(tt *){}

	template<typename T> SerializeStream &write(T a);
};

/// \brief Serialization class that converts objects into binary stream.
/// \sa BinUnserializeStream
class SectBinSerializeStream : public BinSerializeStream{
public:
	typedef SerializeStream tt;
	typedef BinSerializeStream st;

	SectBinSerializeStream(SerializeContext *asc, SyncBuf &records) : st(asc), records(records){}
	virtual tt *substream(Serializable::Id id);
	virtual void join(tt *);
//	typedef std::vector<byte> Record;

	const SyncBuf &getRecords()const{return records;}

	const std::map<SerializableId, std::list<Patch> > getPatches()const{ return patches; }
protected:
	SyncBuf &records;
	std::map<SerializableId, std::list<Patch> > patches;
};

#endif
