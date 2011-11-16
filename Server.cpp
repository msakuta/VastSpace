#include "server.h"
#include "Game.h"
#include "serial_util.h"
//#include "game/scout.h"
extern "C"{
#include <clib/dstr.h>
}
//#include <assert.h>
//#include <limits.h>
//#include <string.h>
//#include <stdlib.h>

typedef struct serverclient cl_t;

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


static cl_t *cl_add(cl_t **root, mutex_t *m);
static void cl_freenode(cl_t *p);
static int cl_delp(cl_t **root, cl_t *dst, mutex_t *m);
static int cl_clean(cl_t **root, mutex_t *m);
static int cl_destroy(cl_t **root);

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
#define event_wait(pe) (lock_mutex(&(pe)->m), pthread_cond_wait(&(pe)->c, &(pe)->m.m), unlock_mutex(&(pe)->m))
#define event_delete(pe) (pthread_cond_destroy(&(pe)->c), delete_mutex(&(pe)->m))

static event_t *timerevent = NULL;
static void sigalrm(int i){
	if(timerevent)
		event_set(timerevent);
}
#define timer_init(timer) (*(timer)=0)
static void timer_set(timer_t *timer, unsigned long period, event_t *set){
	struct itimerval it;
	it.it_interval.tv_sec = (period) / 1000;
	it.it_interval.tv_usec = ((period) * 1000) % 1000000;
	it.it_value = it.it_interval;
	setitimer(ITIMER_REAL, &it, NULL);
	signal(SIGALRM, sigalrm);
	timerevent = (set);
	*(timer) = 1;
}
#define timer_kill(timer) {\
	struct itimerval it;\
	it.it_interval.tv_sec = it.it_interval.tv_usec = 0;\
	it.it_value = it.it_interval;\
	setitimer(ITIMER_REAL, &it, NULL);\
	timerevent = NULL;\
	*(timer) = 1;\
}

#endif

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

static void WaitCommand(cl_t *, char *);
static void GameCommand(cl_t *, char *);
static DWORD WINAPI ClientThread(LPVOID lpv);


void SendWait(struct serverclient *cl, Server *sv){
	char buf[64];
	SOCKET s = cl->s;
	send(s, "Modified\r\n", sizeof"Modified\r\n"-1, 0);
	{
		int c;
		struct serverclient *psc;
		for(c = 0, psc = sv->cl; psc; psc = psc->next) if(sv->scs == psc || psc->s != INVALID_SOCKET)
			c++;
		sprintf(buf, "%d Clients\r\n", c);
	}
	send(s, buf, strlen(buf), 0);
	{
		struct serverclient *psc;
		for(psc = sv->cl; psc; psc = psc->next) if(sv->scs == psc || psc->s != INVALID_SOCKET){
			if(psc->name){
				dstr_t ds = dstr0;
				char str[7];
				str[0] = 0 <= psc->team ? psc->team + '0' : '-';
				str[1] = 0 <= psc->unit ? psc->unit + '0' : '-';
				str[2] = psc->attr[0] + '0';
				str[3] = psc->attr[1] + '0';
				str[4] = psc->attr[2] + '0';
				str[5] = psc->status & SC_READY ? 'R' : '-';
				str[6] = '\0';
				dstrcat(&ds, psc == cl ? "U" : "O");
				dstrcat(&ds, str);
				dstrcat(&ds, psc->name);
				dstrcat(&ds, "\r\n");
				send(s, dstr(&ds), dstrlen(&ds), 0);
				dstrfree(&ds);
			}
			else{
				sprintf(buf, "%08x:%04x\r\n", ntohl(psc->tcp.sin_addr.s_addr), ntohs(psc->tcp.sin_port));
				send(s, buf, strlen(buf), 0);
			}
		}
	}

	/* does this work? */
	setflush(s);
	send(s, "", 0, 0);
	unsetflush(s);
}

void WaitModified(Server *sv){
	if(thread_isvalid(&sv->animThread))
		return;
	if(lock_mutex(&sv->mcl)){
		struct serverclient *pc = sv->cl;
		while(pc){
			if(pc->s != INVALID_SOCKET && lock_mutex(&pc->m)){
				SendWait(pc, sv);
				unlock_mutex(&pc->m);
			}
			pc = pc->next;
		}
		unlock_mutex(&sv->mcl);
	}
//	if(sv->std.modified)
//		sv->std.modified(sv->std.modified_data);
}

static void WaitBroadcast(Server *sv, const char *msg){
	if(lock_mutex(&sv->mcl)){
		dstr_t ds = dstr0;
		dstrcat(&ds, "MSG ");
		dstrcat(&ds, msg);
		dstrcat(&ds, "\r\n");
		struct serverclient *pc = sv->cl;
		while(pc){
			if(pc->s != INVALID_SOCKET && lock_mutex(&pc->m)){
				send(pc->s, dstr(&ds), dstrlen(&ds), 0);
				unlock_mutex(&pc->m);
			}
			pc = pc->next;
		}
		unlock_mutex(&sv->mcl);
		if(sv->std.message)
			sv->std.message(msg, sv->std.message_data);
		dstrfree(&ds);
	}
}

static void RawBroadcast(Server *sv, const char *msg, int len){
	if(lock_mutex(&sv->mcl)){
		struct serverclient *pc = sv->cl;
		while(pc){
			if(pc->s != INVALID_SOCKET && lock_mutex(&pc->m)){
				send(pc->s, msg, len, 0);
				unlock_mutex(&pc->m);
			}
			pc = pc->next;
		}
		unlock_mutex(&sv->mcl);
	}
}

static void RawBroadcastTeam(Server *sv, const char *msg, int len, int team){
	if(lock_mutex(&sv->mcl)){
		struct serverclient *pc = sv->cl;
		while(pc){
			if(pc->team == team && pc->s != INVALID_SOCKET && lock_mutex(&pc->m)){
				send(pc->s, msg, len, 0);
				unlock_mutex(&pc->m);
			}
			pc = pc->next;
		}
		unlock_mutex(&sv->mcl);
	}
}

static void ShutdownClient(struct serverclient *p){
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

struct ServerThreadDataInt{
	sockaddr_in svtcp;
	Server *sv;
	int port;
};

/*
 * Server mechanism (no matter embedded or dedicated) contains a set of threads:
 *   ServerThread - accepts incoming connections
 *   AnimThread - animate the world in constant pace
 *   ClientThread - communicate with the associated client via socket
 */

DWORD WINAPI ServerThread(struct ServerThreadDataInt *pstdi){
	Server &sv = *pstdi->sv;
	struct serverclient *&scs = sv.scs;
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
				cl_t *pcl;

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
					printf("%d ClientThreads deleted\n", dels = cl_clean(&sv.cl, &sv.mcl));
					sv.ncl -= dels;
				}

#ifdef _WIN32
				printf("Client connected. family=%d, port=%d, addr=%d.%d.%d.%d\n", client.sin_family, client.sin_port
					, client.sin_addr.S_un.S_un_b.s_b1
					, client.sin_addr.S_un.S_un_b.s_b2
					, client.sin_addr.S_un.S_un_b.s_b3
					, client.sin_addr.S_un.S_un_b.s_b4
					);
#else
				printf("Client connected. family=%d, port=%d, addr=%s\n", client.sin_family, client.sin_port, inet_ntoa(client.sin_addr));
#endif
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

				if(sv.started || sv.maxncl <= sv.ncl){
					send(AcceptSocket, "FULL\r\n", sizeof"FULL\r\n"-1, 0);
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
				pcl = cl_add(&sv.cl, NULL);
				if(!pcl){
					const char say[] = "MSG Server memory couldn't aquired.\r\n";
					send(AcceptSocket, say, sizeof say-1, 0);
		#if OUTFILE
					fwrite(say, 1, sizeof say-1, pcl->ofp);
		#endif
					printf("Client data allocation failed.\n");
					closesocket(AcceptSocket);
					continue;
				}
				pcl->s = AcceptSocket;
				pcl->sv = &sv;
		//		pcl->id = tcount++;
		//		pcl->rate = .5;
				pcl->name = NULL;
				pcl->tcp = client;
				sv.ncl++;
				unlock_mutex(&sv.mcl);

				/* send protocol signature, versions and basic info about the server. */
				{
					char buf[128];
					sprintf(buf, PROTOCOL_SIGNATURE"%d.%d\r\nMAXCLT %d\r\n",
						PROTOCOL_MAJOR, PROTOCOL_MINOR,
						sv.maxncl);
					send(AcceptSocket, buf, strlen(buf), 0);
				}

				/* Hand process to the new thread */
				thread_create(&pcl->t, ClientThread, pcl);
				WaitModified(&sv);
			}
			lock_mutex(&sv.mcl);
			sv.terminating = 1;
			cl_destroy(&sv.cl);
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
			free(sv.position);
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

static void *AnimThread(Server *ps){
	while(event_wait(&ps->hAnimEvent)){
		if(ps->terminating)
			break;
		if(ps->pg && lock_mutex(&ps->mg)){
			ps->pg->anim(NULL);
			struct serverclient *psc = ps->cl;
#ifndef DEDICATED
			SocketOutStream vos(INVALID_SOCKET);
			ps->pg->serialize(vos);
#endif
			unlock_mutex(&ps->mg);
		}
	}
	return 0;
}



int StartServer(struct ServerThreadData *pstd, struct ServerThreadHandle *ph){
	DWORD dw;
	static struct ServerThreadDataInt stdi;
#ifdef _WIN32
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR)
		return 0;
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
		sv.client = pstd->client;
		sv.cl = NULL;
		sv.ncl = 1;
		sv.maxncl = pstd->maxclients;
//		sv.position = (serverclient *(*)[2])calloc(sizeof*sv.position, (sv.maxncl + 1) / 2);
		sv.command_proc = WaitCommand;
		sv.terminating = 0;
		sv.started = 0;
		create_mutex(&sv.mcl);
		create_mutex(&sv.mg);
#ifdef DEDICATED
		sv.scs = NULL;
#else
		sv.scs = cl_add(&sv.cl, NULL);
		sv.scs->s = INVALID_SOCKET;
		sv.scs->sv = &sv;
		sv.scs->name = sv.hostname;
		sv.scs->tcp = stdi.svtcp;
#endif
		sv.std = *pstd;
	}

	thread_create(&ph->thread, ServerThread, &stdi);
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
		dstrcat(&ds, "MSG ");
		dstrcat(&ds, p->sv->scs ? p->sv->scs->name : "server");
		dstrcat(&ds, "> ");
		dstrcat(&ds, buf);
		dstrcat(&ds, "\r\n");
		struct serverclient *psc = p->sv->cl;
		while(psc){
			if(psc->s != INVALID_SOCKET){
				send(psc->s, dstr(&ds), strlen(dstr(&ds)), 0);
			}
			psc = psc->next;
		}
		unlock_mutex(&p->sv->mcl);
		if(p->sv->std.message){
			char *s;
			s = strpbrk(dstr(&ds), "\r\n");
			if(s)
				*s = '\0';
			p->sv->std.message(&dstr(&ds)[4], p->sv->std.message_data);
		}
		dstrfree(&ds);
	}
}

void KickClientServer(ServerThreadHandle *ph, int clid){
	assert(ph && ph->sv);
	lock_mutex(&ph->sv->mcl);
	struct serverclient **pp = &ph->sv->cl;
	while(*pp){
		struct serverclient *p = *pp;
		if(!clid--){
			struct serverclient *pn = p->next;
			if(p->s != INVALID_SOCKET){
				printf("Kicked %s\n", pn->name);
				send(p->s, "KICK\r\n", sizeof"KICK\r\n", 0);
				ShutdownClient(p);
				p->status = KICKED;
			}
/*			cl_freenode(p);
			*pp = pn;*/
			unlock_mutex(&ph->sv->mcl);
			return;
		}
		pp = &p->next;
	}
	unlock_mutex(&ph->sv->mcl);
	return;
}

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

static void GameCommand(cl_t *cl, char *lbuf){
#if 0
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
#endif
}

static void WaitCommand(cl_t *cl, char *lbuf){
	if(!strncmp(lbuf, "SEL ", 4)){
		short team, unit;
		team = lbuf[4] - '0';
		unit = lbuf[5] - '0';
		SelectPositionServer(cl->sv, cl, team, unit);
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
			memcpy(cl->attr, attr, sizeof cl->attr);
			WaitModified(cl->sv);
		}
	}
	else if(!strcmp(lbuf, "READY")){
//		if(2 == ReadyClientServer(cl->sv, cl) && cl->sv->std.already)
//			cl->sv->std.already(cl->sv->std.already_data);
	}
	else if(!strncmp(lbuf, "LOGIN ", sizeof "LOGIN "-1)){
		const char *const str = &lbuf[sizeof "LOGIN "-1];
		if(lock_mutex(&cl->sv->mcl)){
			if(cl->name)
				free(cl->name);
			int ai = 0;
			char *str2;
			str2 = (char*)malloc(strlen(str)+1);
			strcpy(str2, str);
			do{
				struct serverclient *psc = cl->sv->cl;
				while(psc){
					if(psc != cl && psc->name && !strcmp(psc->name, str2))
						break;
					psc = psc->next;
				}
				if(psc){
					char *oldstr2 = str2;
					str2 = (char*)malloc(strlen(str)+12+1);
					sprintf(str2, "%s (%d)", str, ++ai);
					free(oldstr2);
				}
				else break;
			} while(1);
			cl->name = str2;
			unlock_mutex(&cl->sv->mcl);
		}
		WaitModified(cl->sv);
		{
			dstr_t ds = dstr0;
			dstrcat(&ds, cl->name);
			dstrcat(&ds, " logged in");
			WaitBroadcast(cl->sv, dstr(&ds));
			dstrfree(&ds);
		}
	}
}

static DWORD WINAPI ClientThread(LPVOID lpv){
	cl_t *cl = (cl_t*)lpv; /* Copy the value as soon as possible */
	SOCKET s = cl->s;
	int tid = cl->id;
	int size;
	char buf[32], *lbuf = NULL;
	size_t lbs = 0, lbp = 0;
	printf("ClientThread[%d] started.\n", tid);
	while(SOCKET_ERROR != (size = recv(s, buf, sizeof buf, MSG_NOSIGNAL)) && size){
		char *p;
		if(lbs < lbp + size + 1) lbuf = (char*)realloc(lbuf, lbs = lbp + size + 1);
		memcpy(&lbuf[lbp], buf, size);
		lbp += size;
		lbuf[lbp] = '\0'; /* null terminate for strchr */
		while(p = strpbrk(lbuf, "\r\n")){/* some terminals doesn't end line with \n? */
			*p = '\0';

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

		if(cl->team != -1 && cl->unit != -1 && lock_mutex(&cl->sv->mcl)){
			cl->sv->position[cl->unit][cl->team] = NULL;
			unlock_mutex(&cl->sv->mcl);
		}

		if(cl->status & KICKED){
			printf("[%d] Socket Closed by kicking. %s:%d\n", tid, inet_ntoa(cl->tcp.sin_addr), cl->tcp.sin_port);
		}
		else{
			printf("[%d] Socket Closed by foreign host. %s:%d\n", tid, inet_ntoa(cl->tcp.sin_addr), cl->tcp.sin_port);
		}
		if(cl->name){
			dstr_t ds = dstr0;
			dstrcat(&ds, cl->name);
			dstrcat(&ds, cl->status & KICKED ? " is kicked from server" : " left the server");
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
		WaitModified(cl->sv);

	return 0;
}

/* insert so that the newest comes first */
static cl_t *cl_add(cl_t **root, mutex_t *m){
	cl_t *ret;
	if(NULL == (ret = (cl_t*)malloc(sizeof*ret))) return NULL;
	ret->status = 0;
	ret->meid = ULONG_MAX;
	ret->team = ret->unit = -1;

	/* initially average the attributes */
	ret->attr[0] = 33;
	ret->attr[1] = 33;
	ret->attr[2] = 34;

	ret->id = *root == NULL ? 0 : (*root)->id + 1; /* numbered from zero */
	ret->name = NULL;
	create_mutex(&ret->m);
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
	if(m)
		lock_mutex(m);
	ret->next = *root;
	*root = ret;
	if(m)
		unlock_mutex(m);
	return ret;
}

/* freeing memory occupied by a client is not that easy in that you must first
  ensure termination of the ClientThread */
static void cl_freenode(cl_t *p){
	/* by closing the client's socket, signal the thread that it is shutting down. */
	if(p->s != INVALID_SOCKET)
		shutdown(p->s, SD_BOTH);

	/* wait for the thread to end */
	if(p->id)
		thread_join(&p->t);

	if(p->id && p->name) free(p->name); /* name is always stored in heap */
	delete_mutex(&p->m); /* the mutex object is always created so always deleted */
#if OUTFILE
	if(p->ofp) fclose(p->ofp);
#endif
	free(p);
}

/* delete, returns nonzero on failure */
static int cl_delp(cl_t **root, cl_t *dst, mutex_t *m){
	cl_t *p = *root, **pp = root;
	lock_mutex(m);
	while(p){
		if(p == dst){
			thread_t t;
			*pp = p->next;
			t = p->t;
			cl_freenode(p);
			unlock_mutex(m);
			return 0;
		}
		pp = &p->next, p = p->next;
	}
	unlock_mutex(m);
	return 1;
}

/* delete terminated thread, returns nonzero on failure */
static int cl_clean(cl_t **root, mutex_t *m){
	int ret = 0;
	cl_t **pp = root;
/*	lock_mutex(m);*/
	while(*pp){
		cl_t *p = *pp;
		if(p->id && p->s == INVALID_SOCKET){
			thread_t t;
			*pp = p->next;
			t = p->t;
			cl_freenode(p);
			ret++;
			continue;
		}
		pp = &p->next;
	}
/*	unlock_mutex(m);*/
	return ret;
}

/* delete all under the root node */
static int cl_destroy(cl_t **root){
	int ret;
	cl_t *p = *root;
	if(!p) return 0;
	for(ret = 0; p; ret++){
		cl_t *p1 = p->next;
		cl_freenode(p);
		p = p1;
	}
	*root = NULL;
	return ret;
}
