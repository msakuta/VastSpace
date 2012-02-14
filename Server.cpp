#include "Server.h"
#include "Game.h"
#include "serial_util.h"
#include "cmd.h"
#include "Application.h"
extern "C"{
#include <clib/dstr.h>
#include <clib/timemeas.h>
}
//#include <assert.h>
//#include <limits.h>
//#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <stdint.h>

#ifdef DEDICATED
#include <stdio.h>
#else
#include <stdio.h>
#include <stdarg.h>
/* TODO: security issues!! */
static int altprintf(const char *s, ...){
	va_list args;
	char buf[256];
	DWORD dw;
	int ret;
	HANDLE out;
	out = GetStdHandle(STD_OUTPUT_HANDLE);
	if(!out)
		return 0;
	va_start(args, s);
	ret = vsprintf(buf, s, args);
	WriteConsole(out, buf, (DWORD)strlen(buf), &dw, NULL);
	va_end(args);
	return ret;
}
#define printf altprintf
#endif

#define numof(a) (sizeof(a)/sizeof*(a))


static ServerClient *cl_add(ServerClient **root, mutex_t *m);
static void cl_freenode(Server::ServerClientList::iterator p);
static int cl_delp(ServerClient **root, ServerClient *dst, mutex_t *m);
static int cl_clean(Server::ServerClientList &root, mutex_t *m);
static int cl_destroy(Server::ServerClientList &root);

#ifdef _WIN32
#include <crtdbg.h>
typedef int socklen_t;
typedef char *sockopt_t;
#define MSG_NOSIGNAL 0
#define event_isnull(pe) (!*(pe))
#define event_create(pm) (*(pm)=CreateEvent(NULL, FALSE, FALSE, NULL))
#define event_set(pe) SetEvent(*(pe))
#define event_wait(pe) (WAIT_OBJECT_0 == WaitForSingleObject(*(pe), INFINITE))
#define event_delete(pe) CloseHandle(*(pe))
#define timer_init(timer) (*(timer)=NULL)
#define timer_set(timer,period,set) {\
	TIMECAPS tc;\
	timeGetDevCaps(&tc, sizeof tc);\
	*(timer) = (timer_t)timeSetEvent(FRAMETIME, tc.wPeriodMax, (LPTIMECALLBACK)*(set), (DWORD_PTR)0, TIME_PERIODIC | TIME_CALLBACK_EVENT_SET);\
}
#define timer_kill(timer) (TIMERR_NOERROR == timeKillEvent(*(timer)) && (*(timer) = NULL, 1))

bool TryLockMutex(mutex_t *pm, const char *file, int line){
	muprintf("mlocking %p %s(%d)\n", pm, file, line);
#ifdef NDEBUG
	DWORD ret = WaitForSingleObject(*pm, 0);
#else
	DWORD ret = WaitForSingleObject(pm->m, 0);
	if(WAIT_TIMEOUT == ret)
		return false;
	assert(!pm->locks);
	pm->locks++;
	pm->file = file;
	pm->line = line;
	pm->thread = GetCurrentThreadId();
#endif
	return WAIT_OBJECT_0 == ret;
}

#else /* LINUX */
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <signal.h>

typedef void *sockopt_t;

#define SD_BOTH SHUT_RDWR

#define event_isnull(pe) mutex_isnull(&(pe)->m)
#define event_create(pe) (pthread_cond_init(&(pe)->c, NULL), create_mutex(&(pe)->m))
#define event_set(pe) (lock_mutex(&(pe)->m), pthread_cond_signal(&(pe)->c), unlock_mutex(&(pe)->m))
inline bool event_wait(event_t *pe){
	lock_mutex(&(pe)->m);
	pthread_cond_wait(&(pe)->c, &(pe)->m.m);
	return unlock_mutex(&(pe)->m);
}
#define event_delete(pe) (pthread_cond_destroy(&(pe)->c), delete_mutex(&(pe)->m))

bool LockMutex(mutex_t *pm, const char *file, int line){
	muprintf("mlocking %p %s(%d)\n", pm, file, line);
	return !pthread_mutex_lock(&(pm)->m);
}

bool TryLockMutex(mutex_t *pm, const char *file, int line){
	muprintf("mlocking %p %s(%d)\n", pm, file, line);
	return !pthread_mutex_trylock(&pm->m);
}

#define timer_init(timer) (*(timer)=0)

/// Dummy signal handler that would never be called
void sigalrm(int){}

/// Use pthread timer in Linux. Linux kernel version must be 2.4 or later.
static bool timer_set(timer_t *timer, unsigned long period, event_t *set){

	struct sigaction sigact, oldsa;
	sigact.sa_handler = sigalrm;
	sigact.sa_flags = 0;
	sigemptyset(&sigact.sa_mask);
	if(sigaction(SIGUSR2, &sigact, &oldsa) == -1)
		return false;
	struct itimerspec it;
	it.it_interval.tv_sec = (period) / 1000;
	it.it_interval.tv_nsec = ((period) * 1000000) % 1000000000;
	it.it_value = it.it_interval;
	struct sigevent se;
	se.sigev_notify = SIGEV_SIGNAL;
	se.sigev_signo = SIGUSR2;
	if(timer_create(CLOCK_REALTIME, &se, timer) == -1)
		return false;
	if(timer_settime(*timer, 0, &it, NULL) == -1)
		return false;
//	signal(SIGALRM, sigalrm);
	return true;
}

static void timer_kill(timer_t *timer){
	timer_delete(*timer);
}

#endif

#define trylock_mutex(pm) TryLockMutex(pm, __FILE__, __LINE__)

#define setflush(s) {\
	int nd = 1;\
	socklen_t size = sizeof nd;\
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (sockopt_t)&nd, size);\
}
#define unsetflush(s) {\
	int nd = 0;\
	socklen_t size = sizeof nd;\
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (sockopt_t)&nd, size);\
}

static void WaitCommand(ServerClient *, char *);
static void GameCommand(ServerClient *, char *);
static DWORD WINAPI ClientThread(LPVOID lpv);
double server_lastdt = 0.;


Server::Server() : sendbuf(NULL), sendbufsiz(0){
	logfp = fopen("server.log", "wb");
}

Server::~Server(){
	delete[] sendbuf;
	fclose(logfp);
}

/// \brief Sends the update stream to a given client.
/// \param cl The client handle to send to.
/// \param first Instructs to send the stream for first updates for the client.
/**
 * Below is a pseudo-BNF notation representing structure of server to client stream.
 *
 * all ::= greeting frame*
 *
 * greeting ::= "GLTESTPLUS *.*\r\n"
 *
 * frame ::= frame_size ( maxclient_frame | message_frame | update_frame )
 *
 * frame_size ::= integer
 *
 * maxclient_frame ::= 'L' maxclient
 *
 * maxclient ::= integer
 *
 * message_frame ::= 'M' string
 *
 * update_frame ::= 'U' sendcount server_lastdt client_list update_buffer
 *
 * sendcount ::= integer
 *
 * server_lastdt ::= double
 *
 * client_list ::= client_count client_entry*
 *
 * client_count ::= integer
 *
 * client_entry ::= ip_address tcp_port you_indicator name
 *
 * ip_address ::= integer
 *
 * tcp_port ::= short
 *
 * you_indicator ::= 'U' | 'O'
 *
 * name ::= string
 *
 * string ::= char* '\0'
 *
 */
void Server::SendUpdateStream(ServerClient *cl, bool first){
	static int sendcount = 0;
	BinSerializeStream ss;
	SOCKET s = cl->s;

	ss << 'U';
	ss << sendcount++ << server_lastdt;
	{
		int c;
		ServerClientList::iterator psc = this->cl.begin();
		for(c = 0; psc != this->cl.end(); psc++) if(this->scs == &*psc || psc->s != INVALID_SOCKET)
			c++;
		ss << c;
	}
	{
		ServerClientList::iterator psc;
		for(psc = this->cl.begin(); psc != this->cl.end(); psc++) if(this->scs == &*psc || psc->s != INVALID_SOCKET){
			ss << psc->tcp.sin_addr.s_addr;
			ss << psc->tcp.sin_port;
			ss << (&*psc == cl ? 'U' : 'O');
			ss << psc->name;
		}
	}

	{
//		timemeas_t tm;
//		TimeMeasStart(&tm);
		lock_mutex(&mg);
		unsigned char *sendbuf;
		size_t sendbufsiz;
		if(first){
			sendbuf = NULL;
			sendbufsiz = 0;
			unsigned zero = 0;
			SyncBuf syncbuf;
			CreateDiffStream(sendbuf, sendbufsiz, syncbuf, &zero, sizeof zero);
		}
		else{
			sendbuf = this->sendbuf;
			sendbufsiz = this->sendbufsiz;
		}

		unsigned umsgsiz = unsigned(ss.getsize() + sizeof(unsigned) + sendbufsiz);
		send(s, (const char*)&umsgsiz, sizeof umsgsiz, 0);
		send(s, (const char*)ss.getbuf(), ss.getsize(), 0);
		unsigned usendbufsiz = unsigned(sendbufsiz);
		send(s, (const char*)&usendbufsiz, sizeof usendbufsiz, 0);
		send(s, (const char*)sendbuf, sendbufsiz, 0);
		unlock_mutex(&mg);

		fwrite(ss.getbuf(), ss.getsize(), 1, logfp);
		fwrite((const char*)&usendbufsiz, sizeof usendbufsiz, 1, logfp);
		fwrite(sendbuf, usendbufsiz, 1, logfp);
//		printf("SendWait %p %g\n", cl, TimeMeasLap(&tm));
	}

	/* does this work? */
	setflush(s);
	send(s, "", 0, 0);
	unsetflush(s);
}

void Server::WaitModified(){
//	if(thread_isvalid(&animThread))
//		return;
	if(lock_mutex(&mcl)){
		ServerClientList::iterator pc = cl.begin();
		while(pc != cl.end()){
			if(pc->s != INVALID_SOCKET && pc->sentFirst && lock_mutex(&pc->m)){
				SendUpdateStream(&*pc, false);
				unlock_mutex(&pc->m);
			}
			pc++;
		}
		unlock_mutex(&mcl);
	}
//	if(sv->std.modified)
//		sv->std.modified(sv->std.modified_data);
}

static void WaitBroadcast(Server *sv, const char *msg){
	if(lock_mutex(&sv->mcl)){
		dstr_t ds = dstr0;
		dstrcat(&ds, "MSG ");
		dstrcat(&ds, msg);
		BinSerializeStream ss;
		ss << 'M';
		ss << dstr(&ds);
		unsigned sssize = unsigned(ss.getsize());
		Server::ServerClientList::iterator pc = sv->cl.begin();
		while(pc != sv->cl.end()){
			if(pc->s != INVALID_SOCKET && lock_mutex(&pc->m)){
				send(pc->s, (const char*)&sssize, sizeof sssize, 0);
				send(pc->s, (const char*)ss.getbuf(), ss.getsize(), 0);
				unlock_mutex(&pc->m);
			}
			pc++;
		}
		unlock_mutex(&sv->mcl);
		fwrite(ss.getbuf(), ss.getsize(), 1, sv->logfp);
		if(sv->std.app)
			sv->std.app->signalMessage(msg);
		dstrfree(&ds);
	}
}

static void RawBroadcast(Server *sv, const char *msg, int len){
	if(lock_mutex(&sv->mcl)){
		Server::ServerClientList::iterator pc = sv->cl.begin();
		while(pc != sv->cl.end()){
			if(pc->s != INVALID_SOCKET && lock_mutex(&pc->m)){
				send(pc->s, msg, len, 0);
				unlock_mutex(&pc->m);
			}
			pc++;
		}
		unlock_mutex(&sv->mcl);
	}
}

static void RawBroadcastTeam(Server *sv, const char *msg, int len, int team){
	if(lock_mutex(&sv->mcl)){
		Server::ServerClientList::iterator pc = sv->cl.begin();
		while(pc != sv->cl.end()){
			if(pc->team == team && pc->s != INVALID_SOCKET && lock_mutex(&pc->m)){
				send(pc->s, msg, len, 0);
				unlock_mutex(&pc->m);
			}
			pc++;
		}
		unlock_mutex(&sv->mcl);
	}
}

static void ShutdownClient(ServerClient *p){
	if(p->s)
		shutdown(p->s, SD_BOTH);
}

static void dstrip(dstr_t *pds, const sockaddr_in *s){
	unsigned long ipa;
	ipa = ntohl(s->sin_addr.s_addr);
	dstrlong(pds, (ipa>>24) & 0xff);
	dstrcat(pds, ".");
	dstrlong(pds, (ipa>>16) & 0xff);
	dstrcat(pds, ".");
	dstrlong(pds, (ipa>>8) & 0xff);
	dstrcat(pds, ".");
	dstrlong(pds, ipa % 0x100);
	dstrcat(pds, ":");
	dstrlong(pds, ntohs(s->sin_port));
}


/** \brief The server's accept thread.
 *
 * Server mechanism (no matter embedded or dedicated) contains a set of threads:
 *   ServerThread - accepts incoming connections
 *   AnimThread - animate the world in constant pace
 *   ClientThread - communicate with the associated client via socket
 */

threadret_t WINAPI Server::ServerThread(ServerThreadDataInt *pstdi){
	Server &sv = *pstdi->sv;
	ServerClient *&scs = sv.scs;

#ifdef DEDICATED
	thread_create(&sv.animThread, &Server::AnimThread, &sv);
	timer_set(&sv.timer, FRAMETIME, &sv.hAnimEvent);
#endif

	volatile u_short port;
	char *hostname = sv.hostname;
	port = pstdi->port;
	{
		SOCKET ListenSocket = sv.listener;

		//----------------------
		// Create a SOCKET for accepting incoming requests.
		{
			SOCKET AcceptSocket;
			//----------------------
			// Accept the connection.
			while(1) {
				sockaddr_in client;
				socklen_t size = sizeof client;
		#ifndef _WIN32
				pthread_t pt;
		#endif

				printf("Waiting for a client to connect...\n");
				AcceptSocket = accept( ListenSocket, (struct sockaddr*) &client, &size );

				if(AcceptSocket == SOCKET_ERROR)
					break;

				lock_mutex(&sv.mcl);

				/* Because thread implementation is platform dependant, it's unsure
					whether a thread can delete itself. So we check closed connections
					every time a new connection is made. This way we'll never increase
					but keep garbage memory. Yet, mutex is necessary to serialize
					multiple read access. */
				{
					int dels;
					printf("%d ClientThreads deleted\n", dels = cl_clean(sv.cl, &sv.mcl));
				}

				printf("Client connected. family=%d, port=%d, addr=%s\n", client.sin_family, ntohs(client.sin_port), inet_ntoa(client.sin_addr));
				{
					int sb;
					socklen_t size = sizeof sb;
					getsockopt(AcceptSocket, SOL_SOCKET, SO_SNDBUF, (sockopt_t)&sb, &size);
					printf("send buffer size = %d\n", sb);
				}
				{
					int nd;
					socklen_t size = sizeof nd;
					getsockopt(AcceptSocket, IPPROTO_TCP, TCP_NODELAY, (sockopt_t)&nd, &size);
					printf("nodelay = %d\n", nd);
				}

				if(sv.started || sv.maxncl <= sv.cl.size()){
//					int flag = 1;
//					setsockopt(AcceptSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof flag);
					send(AcceptSocket, "FULL\r\n", sizeof"FULL\r\n"-1, 0);
//					fsync(AcceptSocket);
					shutdown(AcceptSocket, SD_BOTH);
					closesocket(AcceptSocket);
					unlock_mutex(&sv.mcl);
					printf("Connection refused because of no available slot.\n");
					continue;
				}

/*					if(sv.message){
					dstr_t ds = dstr0;
					dstrcat(&ds, "Client ");
					dstrip(&ds, &client);
					dstrcat(&ds, " logged in");
					sv.message(dstr(&ds), sv.message_data);
					WaitBroadcast(&sv, dstr(&ds));
					dstrfree(&ds);
				}*/

				ServerClient *pcl;
				try{
					pcl = &sv.addServerClient();
					pcl->s = AcceptSocket;
					pcl->sentFirst = false;
			//		pcl->rate = .5;
					pcl->tcp = client;
					unlock_mutex(&sv.mcl);
				}
				catch(std::bad_alloc){
					const char say[] = "MSG Server memory couldn't aquired.\r\n";
					send(AcceptSocket, say, sizeof say-1, 0);
		#if OUTFILE
					fwrite(say, 1, sizeof say-1, pcl->ofp);
		#endif
					printf("Client data allocation failed.\n");
					closesocket(AcceptSocket);
					unlock_mutex(&sv.mcl);
					continue;
				}

				/* send protocol signature, versions and basic info about the server. */
				{
					char buf[128];
					sprintf(buf, PROTOCOL_SIGNATURE"%d.%d\r\n",
						PROTOCOL_MAJOR, PROTOCOL_MINOR);
					int len = strlen(buf);
					unsigned size = sizeof'L' + sizeof sv.maxncl;
					char *p = &buf[len];
					memcpy(p, &size, sizeof size);
					p += sizeof size;
					*p++ = 'L';
					memcpy(p, &sv.maxncl, sizeof sv.maxncl);
					p += sizeof sv.maxncl;
					send(AcceptSocket, buf, p - buf, 0);
					sv.SendUpdateStream(pcl, true);
					pcl->sentFirst = true;
				}

				/* Hand process to the new thread */
				thread_create(&pcl->t, ClientThread, pcl);
//				WaitModified(&sv);
			}
			lock_mutex(&sv.mcl);
			sv.terminating = 1;
			cl_destroy(sv.cl);
			unlock_mutex(&sv.mcl);
/*				InterlockedIncrement(&sv.terminating);
			thread_join(animThread);*/
			delete_mutex(&sv.mcl);
			lock_mutex(&sv.mg);
			if(sv.timer)
				timer_kill(&sv.timer);
			if(thread_isvalid(&sv.animThread)){
				event_set(&sv.hAnimEvent);
				thread_join(&sv.animThread);
			}
			if(sv.pg)
				delete sv.pg;
			if(!event_isnull(&sv.hAnimEvent))
				event_delete(&sv.hAnimEvent);
			unlock_mutex(&sv.mg);
			delete_mutex(&sv.mg);
			FreeConsole();
		}
	}
	return 0;
}

class OutStream{
public:
	virtual int write(const char *data, size_t size) = 0;
};

class InStream{
public:
	virtual const char *read() = 0;
	virtual const char *readline(int *len = NULL) = 0;
};

class SocketOutStream : public BinSerializeStream{
	virtual int write(const char *data, size_t size);
	SOCKET sock;
public:
	SocketOutStream(SOCKET s) : sock(s){}
};

int SocketOutStream::write(const char *data, size_t size){
	return send(sock, data, (int)size, 0);
}

/// \brief Dedicated server's routine for simulation computation.
threadret_t Server::AnimThread(Server *ps){
#ifdef _WIN32
	while(event_wait(&ps->hAnimEvent)){
#else
	// Wait SIGUSR2
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGUSR2);
	int sig;

	while(sigwait(&set, &sig) == 0){
#endif
//		printf("AnimThread\n");
		if(ps->terminating)
			break;
		ps->FrameProc(FRAMETIME / 1000.);
	}
	return 0;
}

/// \brief Called once a frame (simulation step) proceeds.
/// \param dt Delta-time of the frame.
void Server::FrameProc(double dt){
	if(pg && trylock_mutex(&mg)){
		pg->anim(dt);

		timemeas_t tm;
		TimeMeasStart(&tm);

		CreateDiffStream(sendbuf, sendbufsiz, syncbuf, NULL, 0);

		unlock_mutex(&mg);
		WaitModified();
		extern double server_lastdt;
		server_lastdt = TimeMeasLap(&tm);
	}
}

/// \brief Creates differential stream for client updates.
/// \param sendbuf Returns the new buffer for sending.
/// \param sendbufsiz Returns size of sendbuf.
/// \param syncbuf A SyncBuf object that is representation of the client's assumed serialized buffers.
/// \param header A buffer that will be inserted before the returned stream.
/// \param headersize The size of header.
void Server::CreateDiffStream(unsigned char *&sendbuf, size_t &sendbufsiz,
		SyncBuf &syncbuf, const void *header, size_t headersize)
{
	SectBinSerializeStream bss(NULL, syncbuf);
	pg->serialize(bss);

	// If the given header is unavailable, make it with our own.
	if(!header){
		header = bss.getbuf();
		headersize = bss.getsize();
	}

	delete[] sendbuf;
	sendbufsiz = headersize;
	sendbuf = new unsigned char[headersize];
	memcpy(sendbuf, header, headersize);

	const std::map<SerializableId, std::list<Patch> > &rc = bss.getPatches();
	std::map<SerializableId, std::list<Patch> >::const_iterator it = rc.begin();
	std::vector<unsigned char> compstream;
	for(; it != rc.end(); it++){
		std::vector<unsigned char> buf;
		applyPatches(syncbuf[it->first], it->second);

		if(it->second.empty())
			continue;
		patchsize_t org = compstream.size();
		compstream.resize(compstream.size() + sizeof(SerializableId) + sizeof(unsigned));
		*(SerializableId*)&compstream[org] = it->first;
		*(unsigned*)&compstream[org+sizeof(SerializableId)] = it->second.size();

		std::list<Patch>::const_iterator it3 = it->second.begin();
		for(; it3 != it->second.end(); it3++){
			patchsize_t org = compstream.size();
			compstream.resize(compstream.size() + 3 * sizeof(patchsize_t) + it3->buf.size());
			*(patchsize_t*)&compstream[org] = it3->start;
			*(patchsize_t*)&compstream[org+sizeof(patchsize_t)] = it3->size;
			*(patchsize_t*)&compstream[org+2*sizeof(patchsize_t)] = it3->buf.size();
			memcpy(&compstream[org+3*sizeof(patchsize_t)], &it3->buf.front(), it3->buf.size());
		}
	}
	if(compstream.size()){
			unsigned newsize = sendbufsiz + compstream.size();
			unsigned char *newbuf = new unsigned char [newsize];
			memcpy(newbuf, sendbuf, sendbufsiz);
			memcpy(&newbuf[sendbufsiz], &compstream.front(), compstream.size());
			delete[] sendbuf;
			sendbuf = newbuf;
			sendbufsiz = newsize;
	}

}


int StartServer(struct ServerParams *pstd, struct ServerThreadHandle *ph){
	DWORD dw;
	static struct Server::ServerThreadDataInt stdi;
#ifdef _WIN32
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR)
		return 0;
#else
	// Block SIGUSR2 from invoking signal handler.
	sigset_t newmask;
	sigemptyset(&newmask);
	sigaddset(&newmask, SIGUSR2);
	pthread_sigmask(SIG_BLOCK, &newmask, NULL);

#endif

/*	static struct server svsv;
	ph->sv = stdi.sv = &svsv;*/
	ph->sv = stdi.sv = new Server();

	//----------------------
	// Create a SOCKET for listening for
	// incoming connection requests.
	if(INVALID_SOCKET == (stdi.sv->listener = socketty())){
		WSACleanup();
		return 0;
	}
	// Initialize console
#ifndef DEDICATED
//	if(!AllocConsole()) return FALSE;
	{DWORD rsz;
//	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "Console Allocated\n", sizeof"Console Allocated\n"-1, &rsz, NULL);
	}
#endif
	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port for the socket that is being bound.
	{
		sockaddr_in service;
		unsigned long ipa;
		memset((char *)&service, 0, sizeof(service));
		service.sin_family = AF_INET;
		ipa = ntohl(service.sin_addr.s_addr = (INADDR_ANY));
		service.sin_port = htons(pstd->port);
		printf("server address: %d.%d.%d.%d:%hu\n",
			(ipa>>24) & 0xff,
			(ipa>>16) & 0xff,
			(ipa>>8) & 0xff,
			ipa % 0x100,
			ntohs(service.sin_port));
		printf("Creating socket for waiting connections...\n");
		if (bind( stdi.sv->listener, (SOCKADDR*) &service, sizeof(service)) == SOCKET_ERROR) {
			closesocket(stdi.sv->listener);
			WSACleanup();
			FreeConsole();
			return 0;
		}
		stdi.svtcp = service;
	}
	//----------------------
	// Listen for incoming connection requests 
	// on the created socket
	if (listen( stdi.sv->listener, 5 ) == SOCKET_ERROR){
		WSACleanup();
		FreeConsole();
		return 0;
	}

	{
		Server &sv = *stdi.sv;
		sv.pg = NULL;
		sv.hostname = (char*)malloc(strlen(pstd->hostname) + 1);
		strcpy(sv.hostname, pstd->hostname);
		thread_null(&sv.animThread);
		event_create(&sv.hAnimEvent);
		timer_init(&sv.timer);
		sv.app = pstd->app;
		sv.maxncl = pstd->maxclients;
		sv.command_proc = WaitCommand;
		sv.terminating = 0;
		sv.started = 0;
		create_mutex(&sv.mcl);
		create_mutex(&sv.mg);
#if 0 && defined DEDICATED
		sv.scs = NULL;
#else
		ServerClient &scs = sv.addServerClient();
		scs.id = 0; // The client id 0 means special, that is the server client.
		scs.name = sv.hostname;
		scs.tcp = stdi.svtcp;
		sv.scs = &scs;
#endif
		sv.std = *pstd;
	}

	thread_create(&ph->thread, &Server::ServerThread, &stdi);
/*	event_wait(&stdi.initiated);
	event_delete(&stdi.initiated);*/
	ph->listener = stdi.sv->listener;
	return 1;
}

void StopServer(struct ServerThreadHandle *p){
	shutdown(p->listener, SD_BOTH);
	closesocket(p->listener);
	if(thread_join(&p->thread)){
		thread_null(&p->thread);
		free(p->sv->hostname);
		free(p->sv);
		p->sv = NULL;
		WSACleanup();
	}
}

void SendChatServer(ServerThreadHandle *p, const char *buf){
	assert(p && p->sv);
	if(lock_mutex(&p->sv->mcl)){
		/* concat the message header and the message content to make it atomic */
		dstr_t ds = dstr0;
		dstrcat(&ds, p->sv->scs ? p->sv->scs->name : "server");
		dstrcat(&ds, "> ");
		dstrcat(&ds, buf);
		BinSerializeStream ss;
		ss << 'M';
		ss << dstr(&ds);
		Server::ServerClientList::iterator psc = p->sv->cl.begin();
		while(psc != p->sv->cl.end()){
			if(psc->s != INVALID_SOCKET){
				send(psc->s, (const char*)ss.getbuf(), ss.getsize(), 0);
			}
			psc++;
		}
		unlock_mutex(&p->sv->mcl);
		if(p->sv->std.app){
			char *s;
			s = strpbrk(dstr(&ds), "\r\n");
			if(s)
				*s = '\0';
			p->sv->std.app->signalMessage(dstr(&ds));
		}
		dstrfree(&ds);
	}
}

void KickClientServer(ServerThreadHandle *ph, int clid){
	assert(ph && ph->sv);
	lock_mutex(&ph->sv->mcl);
	Server::ServerClientList::iterator pp = ph->sv->cl.begin();
	while(pp != ph->sv->cl.end()){
		ServerClient *p = &*pp;
		if(!clid--){
			if(p->s != INVALID_SOCKET){
				printf("Kicked %s\n", static_cast<const char*>(p->name));
				send(p->s, "KICK\r\n", sizeof"KICK\r\n", 0);
				ShutdownClient(p);
				p->kicked = true;
			}
/*			cl_freenode(p);
			*pp = pn;*/
			unlock_mutex(&ph->sv->mcl);
			return;
		}
		pp++;
	}
	unlock_mutex(&ph->sv->mcl);
	return;
}

#if 0
void SelectPositionServer(Server*, struct serverclient *cl, short team, short unit){
	if(0 <= team && team < 2 && 0 <= unit && unit < (cl->sv->maxncl + 1) / 2){
		if(lock_mutex(&cl->sv->mcl)){
			int selected = 0;

			/* if we declared that we are ready, you cannot change your mind about settings. */
			if(cl->status & SC_READY)
				;
			else if(!cl->sv->position[unit][team]){
				/* clear old info */
				if(0 <= cl->team && 0 <= cl->unit)
					cl->sv->position[cl->unit][cl->team] = NULL;
				cl->sv->position[unit][team] = cl;
				cl->team = team;
				cl->unit = unit;
				selected = 1;
			}
			else if(cl->sv->position[unit][team] == cl){
				cl->sv->position[unit][team] = NULL;
				cl->team = -1;
				cl->unit = -1;
				selected = 1;
			}
			unlock_mutex(&cl->sv->mcl);
			if(selected)
				WaitModified(cl->sv);
		}
	}
}

int ReadyClientServer(struct server *, struct serverclient *cl){
	if(lock_mutex(&cl->sv->mcl)){
		struct serverclient *psc = cl->sv->cl;
		int changed = !(cl->status & SC_READY);
		cl->status |= SC_READY;
		while(psc){
			if(!(psc->status & SC_READY))
				break;
			psc = psc->next;
		}
		if(!psc)
			cl->sv->started = 1;
		unlock_mutex(&cl->sv->mcl);
		if(changed)
			WaitModified(cl->sv);
		if(!psc){
			WaitBroadcast(cl->sv, "All Players Ready! Game start");
			if(lock_mutex(&cl->sv->mcl)){
				struct serverclient *psc = cl->sv->cl;
				while(psc){
					if(psc->s != INVALID_SOCKET){
						send(psc->s, "START\r\n", sizeof"START\r\n"-1, 0);
					}
					psc = psc->next;
				}
				cl->sv->command_proc = GameCommand;
				unlock_mutex(&cl->sv->mcl);
			}
			cl->sv->pg = new Game();
			thread_create(&cl->sv->animThread, AnimThread, cl->sv);
			timer_set(&cl->sv->timer, FRAMETIME, &cl->sv->hAnimEvent);
			return 2;
		}
		return changed;
	}
	return 0;
}

void MovetoBroadcast(Server *sv, int cx, int cy){
	dstr_t ds = dstr0;
	if(!sv->scs)
		return;
	dstrcat(&ds, "MTO ");
	dstrlong(&ds, sv->scs->id);
	dstrcat(&ds, " ");
	dstrlong(&ds, cx);
	dstrcat(&ds, " ");
	dstrlong(&ds, cy);
	dstrcat(&ds, "\n");
	RawBroadcastTeam(sv, dstr(&ds), dstrlen(&ds), sv->scs->team);
	dstrfree(&ds);
}

void LinetoBroadcast(Server *sv, int cx, int cy){
	dstr_t ds = dstr0;
	if(!sv->scs)
		return;
	dstrcat(&ds, "LTO ");
	dstrlong(&ds, sv->scs->id);
	dstrcat(&ds, " ");
	dstrlong(&ds, cx);
	dstrcat(&ds, " ");
	dstrlong(&ds, cy);
	dstrcat(&ds, "\n");
	RawBroadcastTeam(sv, dstr(&ds), dstrlen(&ds), sv->scs->team);
	dstrfree(&ds);
}
#endif

#if 0
static void GameCommand(cl_t *cl, char *lbuf){
	if(!strncmp(lbuf, "MOV ", 4)){
		if(thread_isvalid(&cl->sv->animThread) && lock_mutex(&cl->sv->mg)){
			Squad *me;
			int tx, ty;
			if(2 <= sscanf(&lbuf[4], "%d %d", &tx, &ty) && (me = cl->sv->pg->findById(cl->meid)))
				me->order(*cl->sv->pg, tx, ty);
			unlock_mutex(&cl->sv->mg);
		}
	}
	else if(!strncmp(lbuf, "ROT ", 4)){
		if(thread_isvalid(&cl->sv->animThread) && lock_mutex(&cl->sv->mg)){
			Squad *me;
			int tx, ty;
			if(2 <= sscanf(&lbuf[4], "%d %d", &tx, &ty) && (me = cl->sv->pg->findById(cl->meid)))
				me->headto(tx, ty);
			unlock_mutex(&cl->sv->mg);
		}
	}
	else if(!strncmp(lbuf, "MTO ", 4)){
		dstr_t ds = dstr0;
		dstrcat(&ds, "MTO ");
		dstrlong(&ds, cl->id);
		dstrcat(&ds, " ");
		dstrcat(&ds, &lbuf[4]);
		dstrcat(&ds, "\n");
		RawBroadcastTeam(cl->sv, dstr(&ds), dstrlen(&ds), cl->team);
		dstrfree(&ds);
		if(cl->sv->std.moveto && cl->sv->scs && cl->sv->scs->team == cl->team)
			cl->sv->std.moveto(cl->id, cl->team, &lbuf[4], cl->sv->std.moveto_data);
	}
	else if(!strncmp(lbuf, "LTO ", 4)){
		dstr_t ds = dstr0;
		dstrcat(&ds, "LTO ");
		dstrlong(&ds, cl->id);
		dstrcat(&ds, " ");
		dstrcat(&ds, &lbuf[4]);
		dstrcat(&ds, "\n");
		RawBroadcastTeam(cl->sv, dstr(&ds), dstrlen(&ds), cl->team);
		dstrfree(&ds);
		if(cl->sv->std.lineto && cl->sv->scs && cl->sv->scs->team == cl->team)
			cl->sv->std.lineto(cl->id, cl->team, &lbuf[4], cl->sv->std.lineto_data);
	}
	else if(!strncmp(lbuf, "LSC ", 4)){ /* Launch SCout */
		if(thread_isvalid(&cl->sv->animThread) && lock_mutex(&cl->sv->mg)){
			Squad *me;
			int tx, ty;
			if(2 <= sscanf(&lbuf[4], "%d %d", &tx, &ty) && (me = cl->sv->pg->findById(cl->meid)))
				me->launchScout(*cl->sv->pg, tx, ty);
			unlock_mutex(&cl->sv->mg);
		}
	}
	else if(!strncmp(lbuf, "LMI ", 4)){ /* Launch MIssile */
		if(thread_isvalid(&cl->sv->animThread) && lock_mutex(&cl->sv->mg)){
			Squad *me;
			unsigned long id;
			if(1 <= sscanf(&lbuf[4], "%lu", &id) && (me = cl->sv->pg->findById(cl->meid))){
				Squad *ps = cl->sv->pg->findById(id);
				if(ps)
					me->launchMissile(*cl->sv->pg, ps);
			}
			unlock_mutex(&cl->sv->mg);
		}
	}
	else if(!strncmp(lbuf, "SOD ", 4)){ /* Select by ORder */
		unsigned long ord;
		if(1 <= sscanf(&lbuf[4], "%lu", &ord)){
			if(lock_mutex(&cl->sv->mg)){
				cl->meid = cl->sv->pg->selectio(ord, cl->meid);
				unlock_mutex(&cl->sv->mg);
			}
		}
	}
	else if(!strncmp(lbuf, "SPO ", 4)){ /* Select by POsition */
		int cx, cy;
		if(2 <= sscanf(&lbuf[4], "%d %d", &cx, &cy)){
			if(lock_mutex(&cl->sv->mg)){
				cl->meid = cl->sv->pg->selecti(cx, cy, cl->meid);
				unlock_mutex(&cl->sv->mg);
			}
		}
	}
	else if(!strncmp(lbuf, "SPL ", 4)){
		int splits;
		if(1 <= sscanf(&lbuf[4], "%d", &splits)){
			if(lock_mutex(&cl->sv->mg)){
				cl->sv->pg->split(cl->meid, splits);
				unlock_mutex(&cl->sv->mg);
			}
		}
	}
}
#endif

static void WaitCommand(ServerClient *cl, char *lbuf){
	// "C" is the server game command request.
	if(!strncmp(lbuf, "C ", 2)){
		char *p = strchr(&lbuf[2], ' ');
		if(p){
			dstring ds;
			ds.strncpy(&lbuf[2], p - &lbuf[2]);
			ClientMessage::CtorMap::iterator it = ClientMessage::ctormap().find(ds);
			if(it != ClientMessage::ctormap().end()){
				std::istringstream iss(std::string(p+1, strlen(p+1)));
				StdUnserializeStream uss(iss);
				it->second->interpret(*cl, uss);
			}
		}
	}
	else if(!strncmp(lbuf, "ATTR ", 5)){
		char attr[3];
		int i, sum = 0;
		for(i = 0; i < 3; i++){
			sum += attr[i] = lbuf[5+i] - '0';
			if(attr[i] < 0 || 100 < attr[i])
				continue;
		}

		/* although along with nothing good, sum being less than 100 is permitted. */
		if(0 <= sum && sum <= 100){
			cl->sv->WaitModified();
		}
	}
	else if(!strcmp(lbuf, "READY")){
//		if(2 == ReadyClientServer(cl->sv, cl) && cl->sv->std.already)
//			cl->sv->std.already(cl->sv->std.already_data);
	}
	else if(!strncmp(lbuf, "LOGIN ", sizeof "LOGIN "-1)){
		gltestp::dstring str = &lbuf[sizeof "LOGIN "-1];
		if(lock_mutex(&cl->sv->mcl)){
			int ai = 0;
			gltestp::dstring str2 = str;
			// Loop until there is no duplicate name
			do{
				Server::ServerClientList::iterator psc = cl->sv->cl.begin();
				while(psc != cl->sv->cl.end()){
					ServerClient *p = &*psc;
					if(p != cl && p->name == str2)
						break;
					psc++;
				}
				if(psc != cl->sv->cl.end()){
					str2 = str << " (" << ++ai << ")";
				}
				else break;
			} while(1);
			cl->name = str2;

			// If the client logs in, assign a Player object for the client.
			Player *clientPlayer = new Player(cl->sv->pg);

			// Temporary treatment to make newly added Player's coordsys to be the same as the first Player.
			std::vector<Player*>::iterator it = cl->sv->pg->players.begin();
			if(it != cl->sv->pg->players.end()){
				clientPlayer->cs = (*it)->cs;
				clientPlayer->setpos((*it)->getpos());
				clientPlayer->setvelo((*it)->getvelo());
				clientPlayer->setrot((*it)->getrot());
			}

			// Assign an unique id to the newly added Player.
			clientPlayer->playerId = cl->sv->pg->players.size();

			// Actually add the Player to the Game's player list.
			cl->sv->pg->players.push_back(clientPlayer);

			unlock_mutex(&cl->sv->mcl);
		}
//		WaitModified(cl->sv);
		{
			dstr_t ds = dstr0;
			dstrcat(&ds, cl->name);
			dstrcat(&ds, " logged in");
			WaitBroadcast(cl->sv, dstr(&ds));
			dstrfree(&ds);
		}
	}
}

/// Receive thread for the connection to the client
static DWORD WINAPI ClientThread(LPVOID lpv){
	ServerClient *cl = (ServerClient*)lpv; /* Copy the value as soon as possible */
	SOCKET s = cl->s;
	int tid = cl->id;
	int size;
	char buf[32], *lbuf = NULL;
	size_t lbs = 0, lbp = 0;
	FILE *fp = fopen("serverrecv.log", "wb");
	printf("ClientThread[%d] started.\n", tid);
	while(SOCKET_ERROR != (size = recv(s, buf, sizeof buf, MSG_NOSIGNAL)) && size){
		char *p;
		if(lbs < lbp + size + 1) lbuf = (char*)realloc(lbuf, lbs = lbp + size + 1);
		memcpy(&lbuf[lbp], buf, size);
		lbp += size;
		lbuf[lbp] = '\0'; /* null terminate for strchr */
		while(p = strpbrk(lbuf, "\r\n")){/* some terminals doesn't end line with \n? */
			*p = '\0';

			fwrite(lbuf, strlen(lbuf), 1, fp);
			fwrite("\r\n", 2, 1, fp);
			fflush(fp);

			/* SAY command is common among all modes */
			if(!strncmp(lbuf, "SAY ", 4)){
				dstr_t ds = dstr0;
				if(cl->name)
					dstrcat(&ds, cl->name);
				else
					dstrip(&ds, &cl->tcp);
				dstrcat(&ds, "> ");
				dstrcat(&ds, &lbuf[4]);
				WaitBroadcast(cl->sv, dstr(&ds));
				dstrfree(&ds);
			}
			else
				cl->sv->command_proc(cl, lbuf);

			memmove(lbuf, p+1, lbp -= (p+1 - lbuf)); /* reorient */
		}
	}
	realloc(lbuf, 0); /* free line buffer */

	if(cl->sv->terminating){
		closesocket(s);
		printf("[%d] Socket Closed. %s:%d\n", tid, inet_ntoa(cl->tcp.sin_addr), cl->tcp.sin_port);
	}
	else{
		/*if(size == 0)*/ /* graceful shutdown. */
			closesocket(s);
		cl->s = INVALID_SOCKET;

		if(cl->kicked){
			printf("[%d] Socket Closed by kicking. %s:%d\n", tid, inet_ntoa(cl->tcp.sin_addr), ntohs(cl->tcp.sin_port));
		}
		else{
			printf("[%d] Socket Closed by foreign host. %s:%d\n", tid, inet_ntoa(cl->tcp.sin_addr), ntohs(cl->tcp.sin_port));
		}
		if(cl->name){
			dstr_t ds = dstr0;
			dstrcat(&ds, cl->name);
			dstrcat(&ds, cl->kicked ? " is kicked from server" : " left the server");
			WaitBroadcast(cl->sv, dstr(&ds));
			dstrfree(&ds);
		}
	}

	/* after all cleanup process is done, invalidate the socket to indicate
	  this thread is ready to get killed. ServerThread will delete it in
	  some opportunities. Thread's data resides in memory even after the thread
	  terminates because it has the return value. We must erase the data
	  or memory leak will take place. */
	cl->s = INVALID_SOCKET;

	if(!cl->sv->terminating)
		cl->sv->WaitModified();

	fclose(fp);

	return 0;
}

/** insert so that the newest comes first */
ServerClient &Server::addServerClient(){
	int id = cl.size(); /* numbered from zero */
	cl.push_back(ServerClient());
	ServerClient &ret = cl.back();
//	if(NULL == (ret = (ServerClient*)malloc(sizeof*ret))) return NULL;
	ret.sv = this;
	ret.kicked = false;
	ret.team = -1;
	ret.meid = ULONG_MAX;
	ret.s = INVALID_SOCKET;

	ret.id = id;
	ret.name = "";
	create_mutex(&ret.m);
#if OUTFILE
	ret->obytes = 0;
	{
		char cn[128];
		sprintf(cn, "out%d.txt", ret->id);
		ret->ofp = fopen(cn, "wb");
		OutputDebugString("output dump file: ");
		OutputDebugString(cn);
		OutputDebugString("\n");
	}
#endif
//	if(m)
//		lock_mutex(m);
//	ret->next = *root;
//	*root = ret;
//	if(m)
//		unlock_mutex(m);
//	return ret;
	return ret;
}

/** freeing memory occupied by a client is not that easy in that you must first
  ensure termination of the ClientThread */
static void cl_freenode(Server::ServerClientList::iterator p){
	/* by closing the client's socket, signal the thread that it is shutting down. */
	if(p->s != INVALID_SOCKET)
		shutdown(p->s, SD_BOTH);

	/* wait for the thread to end */
	if(p->id)
		thread_join(&p->t);

	delete_mutex(&p->m); /* the mutex object is always created so always deleted */
#if OUTFILE
	if(p->ofp) fclose(p->ofp);
#endif
}

/** delete terminated thread, returns nonzero on failure */
static int cl_clean(Server::ServerClientList &root, mutex_t *m){
	int ret = 0;
	Server::ServerClientList::iterator p = root.begin();
/*	lock_mutex(m);*/
	while(p != root.end()){
		ServerClient *psc = &*p;
		if(psc->id && psc->s == INVALID_SOCKET){
			cl_freenode(p);
			p = root.erase(p);
			ret++;
			continue; // Do not advance the iterator if erased one
		}
		else
			p++;
	}
/*	unlock_mutex(m);*/
	return ret;
}

/** delete all under the root node */
static int cl_destroy(Server::ServerClientList &root){
	int ret;
	Server::ServerClientList::iterator p = root.begin();
	for(ret = 0; p != root.end(); ret++){
		cl_freenode(p);
		p++;
	}
	root.clear();
	return ret;
}





//-----------------------------------------------------------------------------
//     DiffSectionStream Implementation
//-----------------------------------------------------------------------------


template<typename T>
inline SerializeStream &DiffSectionStream::write(T a){
	bd.put(reinterpret_cast<const unsigned char*>(&a), sizeof a);
	return *this;
}

SerializeStream &DiffSectionStream::operator <<(char a){return write(a);}
SerializeStream &DiffSectionStream::operator <<(unsigned char a){return write(a);}
SerializeStream &DiffSectionStream::operator <<(short a){return write((int16_t)a);}
SerializeStream &DiffSectionStream::operator <<(unsigned short a){return write((uint16_t)a);}
SerializeStream &DiffSectionStream::operator <<(int a){return write((int32_t)a);}
SerializeStream &DiffSectionStream::operator <<(unsigned a){return write((uint32_t)a);}
SerializeStream &DiffSectionStream::operator <<(long a){return write((int32_t)a);}
SerializeStream &DiffSectionStream::operator <<(unsigned long a){return write((uint32_t)a);}
SerializeStream &DiffSectionStream::operator <<(bool a){return write(a);}
SerializeStream &DiffSectionStream::operator <<(float a){return write(a);}
SerializeStream &DiffSectionStream::operator <<(double a){return write(a);}

SerializeStream &DiffSectionStream::operator <<(const char *a){
	bd.put(reinterpret_cast<const unsigned char*>(a), patchsize_t(strlen(a)+1));
	return *this;
}

SerializeStream &DiffSectionStream::operator<<(const std::string &a){
	bd.put(reinterpret_cast<const unsigned char*>(a.c_str()), patchsize_t(a.length()));
	return *this;
}

SerializeStream &DiffSectionStream::operator <<(const Serializable *p){
	if(p)
		return write(p->getid()/*sc->map[p]*/);
	else
		return write(Serializable::Id(0));
}

SerializeStream &DiffSectionStream::operator<<(const Vec3d &v){
	return *this << v[0] << v[1] << v[2];
}

SerializeStream &DiffSectionStream::operator<<(const Quatd &v){
	return *this << v.i() << v.j() << v.k() << v.re();
}

SerializeStream &DiffSectionStream::operator<<(const struct ::random_sequence &rs){
	return *this << rs.w << rs.z;
}


//-----------------------------------------------------------------------------
//     SectBinSerializeStream Implementation
//-----------------------------------------------------------------------------

SerializeStream *SectBinSerializeStream::substream(Serializable::Id id){
	SyncBuf::iterator it = records.find(id);
	if(it == records.end())
		return new DiffSectionStream(sc, id, NULL, 0);
	else
		return new DiffSectionStream(sc, id, &it->second.front(), it->second.size());
}

void SectBinSerializeStream::join(SerializeStream *o){
	DiffSectionStream *sss = (DiffSectionStream*)o;
//	SyncRecord r = {sss->id, std::vector<byte>(sss->getsize())};
//	r.size = ((BinSerializeStream*)o)->getsize();
//	r.buf = new unsigned char[r.size];
//	memcpy(&r.buf.front(), sss->getbuf(), r.buf.size());
//	records.push_back(r);
	patches[sss->id] = sss->bd.getPatches();
}
