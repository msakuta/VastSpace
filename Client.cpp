/** \file
 * \brief Implementation of Client class and related callbacks.
 */
#include "Application.h"
#include "cmd.h"
//#include "remotegame.h"
#include "serial_util.h"
#include "glw/GLWchat.h"
#include <sstream>

struct JoinGameData{
	char addr[256];
	gltestp::dstring name;
	u_short port;
	FILE **plogfile;
};


ClientApplication::ClientApplication(){
	hGameMutex = CreateMutex(NULL, FALSE, NULL);
}

ClientApplication::~ClientApplication(){
	CloseHandle(hGameMutex);
}


static void quitgame(ClientApplication *, bool ret){
//	exit(ret);
}

void ClientApplication::signalMessage(const char *text){
	Application::signalMessage(text);
	for(std::set<GLWchat*>::iterator it = GLWchat::wndlist.begin(); it != GLWchat::wndlist.end(); it++)
		(*it)->append(text);
}



static DWORD WINAPI RecvThread(ClientApplication *pc){
	int size;
	char buf[4096], *lbuf = NULL, *src = NULL;
	size_t lbs = 0, lbp = 0;
//	ClientWaiter::addr *ad = NULL;
//	BufOutStream os;
	int mode = -1, modn, modi;
	FILE *fp;
	fp = fopen("client.log", "wb");

	while(SOCKET_ERROR != (size = recv(pc->con, buf, sizeof buf, 0)) && size){
		fwrite(buf, size, 1, fp);
		fflush(fp);
		char *p;
		if(lbs < lbp + size + 1) lbuf = (char*)realloc(lbuf, lbs = lbp + size + 1);
		memcpy(&lbuf[lbp], buf, size);
		lbp += size;
		lbuf[lbp] = '\0'; /* null terminate for strchr */

		if(mode == -1)
		while(p = strpbrk(lbuf, "\r\n")){/* some terminals doesn't end line with \n? */
			char *np = p;
			while(*np == '\r' || *np == '\n'){
				np++;
				assert(np - lbuf <= lbp);
			}
			*p = '\0';
				{
					int major, minor;
					if(!strncmp(lbuf, PROTOCOL_SIGNATURE, sizeof(PROTOCOL_SIGNATURE)-1) &&
						2 <= sscanf(&lbuf[sizeof(PROTOCOL_SIGNATURE)-1], "%d.%d", &major, &minor))
					{
						if(major < PROTOCOL_MAJOR){
							quitgame(pc, true);
							PostMessage(pc->w, WM_USER, (WPARAM)"The server's protocol version is too low. "
								"Ask the server administrator to have it up to date.", 0);
							goto cleanup;
						}
						else if(PROTOCOL_MAJOR < major){
							quitgame(pc, true);
							PostMessage(pc->w, WM_USER, (WPARAM)"The server's protocol version is too high. "
								"Get the latest version of client.", 0);
							goto cleanup;
						}
						mode = 0;
					}
					else if(!strcmp(lbuf, "FULL")){
						quitgame(pc, true);
						PostMessage(pc->w, WM_USER, (WPARAM)"The server is full.", 0);
						goto cleanup;
					}
					else{
						quitgame(pc, true);
						PostMessage(pc->w, WM_USER, (WPARAM)"Protocol signature not found. Possibly trying to connect to incompatible server application.", 0);
						goto cleanup;
					}
				}
			if(lbp < np - lbuf)
				assert(0);
			memmove(lbuf, np, lbp -= (np - lbuf)); /* reorient */
			lbuf[lbp] = '\0';
		}
		else
			// Consume all message frames. The given size does not include the size integer itself.
			while(*(unsigned*)lbuf + sizeof(unsigned) <= lbp)
			switch(lbuf[sizeof(unsigned)]){
			case 'L':
				{
					char *p = &lbuf[sizeof(unsigned) + sizeof 'L'];
					if(pc)
						pc->serverParams.maxclients = *(unsigned*)p;
					((unsigned*&)p)++;
					memmove(lbuf, p, lbp -= (p - lbuf)); /* reorient */
				}
				break;
			case 'U':
				try{
					struct Mutex{
						HANDLE h;
						Mutex(HANDLE h) : h(h){}
						~Mutex(){ReleaseMutex(h);}
					};
					if(WAIT_OBJECT_0 == WaitForSingleObject(pc->hGameMutex, 10000)){
						Mutex hm(pc->hGameMutex);
						BinUnserializeStream us((unsigned char*)&lbuf[5], lbp - 5);
						int sendcount;
						us >> sendcount;
						double server_lastdt;
						us >> server_lastdt;
						int client_count;
						us >> client_count;

						pc->ad.resize(client_count);
						int modi = 0;
						for(; modi < client_count; modi++){
							ClientApplication::ClientClient &cc = pc->ad[modi];
							us >> cc.tcp.sin_addr.s_addr;
							us >> cc.tcp.sin_port;
							char you_indicator;
							us >> you_indicator;
							if(you_indicator == 'U')
								pc->thisad = modi;
							us >> cc.name;
						}

						unsigned recvbufsiz;
						us >> recvbufsiz;

						if(recvbufsiz){
							// Allocate a new buffer in the queue and get a reference to it.
							pc->recvbuf.push_back(std::vector<unsigned char>());
							std::vector<unsigned char> &buf = pc->recvbuf.back();

							// Fill the buffer by decoding hex-encoded input stream.
							buf.resize(recvbufsiz);
							us.read((char*)&buf.front(), recvbufsiz);
						}

						if(lbp < us.getCur() - (unsigned char*)lbuf)
							assert(0);
						memmove(lbuf, us.getCur(), lbp -= (us.getCur() - (unsigned char*)lbuf)); /* reorient */
					}
					else
						assert(0);
				}
				catch(InputShortageException e){
					// If input buffer is not complete yet, wait for another input to retry interpretation.
					printf("Shorty!\n");
					assert(0);
				}
				break;
			case 'M':
				try{
					BinUnserializeStream us((unsigned char*)&lbuf[5], lbp - 5);
					dstring ds;
					us >> ds;
					pc->signalMessage(ds);

					if(lbp < us.getCur() - (unsigned char*)&lbuf[5])
						assert(0);
					memmove(lbuf, us.getCur(), lbp -= (us.getCur() - (unsigned char*)lbuf)); /* reorient */
				}
				catch(InputShortageException e){
					// If input buffer is not complete yet, wait for another input to retry interpretation.
					printf("Shorty!\n");
					assert(0);
				}
				break;
		}
#if 0
/*				if(!strncmp(lbuf, "Modified", sizeof"Modified"-1)){
					mode = 1;
				}
				else if(!strcmp(lbuf, "SERIALIZE START")){
					mode = 4;
				}
				else if(!strncmp(lbuf, "MSG ", 4)){
					pc->signalMessage(&lbuf[4]);
				}*/
/*				else if(!strncmp(lbuf, "MTO ", 4)){
					int cid, cx, cy;
					if(3 == sscanf(&lbuf[4], "%d %d %d", &cid, &cx, &cy) && WAIT_OBJECT_0 == WaitForSingleObject(pc->hGameMutex, 10)){
						if(pc->pvlist && cid < pc->ncl)
							PVLAdd(&pc->pvlist[cid], cx, cy, 0);
						ReleaseMutex(pc->hGameMutex);
					}
				}
				else if(!strncmp(lbuf, "LTO ", 4)){
					int cid, cx, cy;
					if(3 == sscanf(&lbuf[4], "%d %d %d", &cid, &cx, &cy) && WAIT_OBJECT_0 == WaitForSingleObject(pc->hGameMutex, 10)){
						if(pc->pvlist && cid < pc->ncl)
							PVLAdd(&pc->pvlist[cid], cx, cy, 1);
						ReleaseMutex(pc->hGameMutex);
					}
				}
				else if(!strncmp(lbuf, "SEL ", 4)){
					if(WAIT_OBJECT_0 == WaitForSingleObject(pc->hGameMutex, 50)){
						pc->meid = atol(&lbuf[4]);
						ReleaseMutex(pc->hGameMutex);
					}
				}*/
				else if(!strncmp(lbuf, "MAXCLT ", sizeof"MAXCLT "-1)){
					int n;
					n = atoi(&lbuf[sizeof"MAXCLT "-1]);

					// To be perfect, it needs synchronization
					if(pc)
						pc->serverParams.maxclients = n;
				}
				else if(!strcmp(lbuf, "START")){
					assert(!pc->serverGame);
//					pc->pg = new RemoteGame(pc->con);
//					pc->mode = ClientGame;
					mode = 4;
				}
				else if(!strcmp(lbuf, "KICK")){
					quitgame(pc, true);
					PostMessage(pc->w, WM_USER, (WPARAM)"You are kicked.", 0);
					goto cleanup;
				}
				break;
			case 1:
				{
					int i;
					if(sscanf(lbuf, "%d Clients", &i)){
						modn = modi = i;
						pc->ad.resize(modn);
						mode = 2;
					}
					else
						mode = 0;
				}
				break;
			case 2:
				{
					if(modi--){
						unsigned int ip, port;
						char ipbuf[12];
						char *namebuf;
						ClientApplication::ClientClient &cc = pc->ad[pc->ad.size() - modi - 1];

						strncpy(ipbuf, lbuf, 8+1+4);
						ipbuf[8+1+4] = '\0';
						namebuf = &lbuf[8+1+4];

						// Parse ip and port
						if(sscanf(ipbuf, "%08x:%4x", &ip, &port)){
							cc.tcp.sin_addr.s_addr = htonl(ip);
							cc.tcp.sin_port = htons(port);
						}

						// Parse the flag whether this user entry is me
						if(namebuf[0] == 'U')
							pc->thisad = modi;

						// If the name is given, save it to local user dictionary.
						if(1 < strlen(namebuf))
							cc.name = &namebuf[1];

						if(!modi){
//							if(!pc->waiter)
//								pc->waiter = new ClientWaiter();
//							ClientWaiter *cw = static_cast<ClientWaiter*>(pc->waiter);
//							cw->freesrc();
//							cw->src = ad;
//							cw->n = modn;
//							SetEvent(pc->hDrawEvent);
							size_t sz;
/*							recv(pc->con, (char*)&sz, sizeof sz, 0);
							fwrite(&sz, sizeof sz, 1, fp);
							fflush(fp);
							char *buf = new char[sz];
							recv(pc->con, buf, sz, 0);
							fwrite(buf, sz, 1, fp);
							fflush(fp);*/
							mode = 3;
//							delete[] buf;
						}
					}
				}
				break;
			case 3:
				if(lbp == 0)
					break;
				if(WAIT_OBJECT_0 == WaitForSingleObject(pc->hGameMutex, 10000)){
					char sizebuf[16];
					char *endptr;
					strncpy(sizebuf, lbuf, 8);
					sizebuf[8] = '\0';
					size_t recvbufsiz = strtoul(sizebuf, &endptr, 16);

					// Allocate a new buffer in the queue and get a reference to it.
					pc->recvbuf.push_back(std::vector<unsigned char>());
					std::vector<unsigned char> &buf = pc->recvbuf.back();

					// Fill the buffer by decoding hex-encoded input stream.
					buf.resize(recvbufsiz);
					int i = 0;
					for(const char *p = &lbuf[8]; p && i < recvbufsiz; i++){
						sizebuf[0] = *p++;
						sizebuf[1] = *p++;
						sizebuf[2] = '\0';
						buf[i] = (unsigned char)strtoul(sizebuf, &endptr, 16);
					}

					ReleaseMutex(pc->hGameMutex);
				}
				else
					assert(0);
				mode = 0;
				break;
			case 4:
				if(!strcmp(lbuf, "SERIALIZE END") && WAIT_OBJECT_0 == WaitForSingleObject(pc->hGameMutex, 50)){
/*					ScoutData *psd = NULL;
					{
						unsigned long enid = pc->dd.enemy ? pc->dd.enemy->id : ULONG_MAX;
						if(pc->pg)
							delete pc->pg;
						pc->pg = new RemoteGame(BufOutStream::BufInStream(os), *pc->waiter, &psd, pc->con);
						pc->dd.enemy = enid == ULONG_MAX ? NULL : pc->pg->findSDById(enid);
					}
					pc->meid = psd ? psd->id : ULONG_MAX;
					ReleaseMutex(pc->hGameMutex);
					os.~BufOutStream();
					SetEvent(pc->hDrawEvent);*/
					mode = 0;
				}
				else{
//					os.write(lbuf, strlen(lbuf));
//					os.write("\r\n", 2);
				}
			}
			if(lbp < np - lbuf)
				assert(0);
			memmove(lbuf, np, lbp -= (np - lbuf)); /* reorient */
			lbuf[lbp] = '\0';
		}
#endif
	}
cleanup:
	fclose(fp);
	realloc(lbuf, 0); /* free line buffer */
	if(!pc->shutclient){
		quitgame(pc, true);
		pc->shutclient = false;
		/* don't MessageBox herein this thread or we'll deadlock. */
		PostMessage(pc->w, WM_USER, (WPARAM)"The server shut down.", 0);
	}
	return 0;
}


int ClientApplication::joingame(const char *host, int port){
	JoinGameData data;
//	quitgame(pc);
//	data.plogfile = &pc->logfile;
	data.port = port;
	if(!host) // If not specified, connect to localhost.
		strncpy(data.addr, "127.0.0.1", sizeof data.addr);
	else
		strncpy(data.addr, host, sizeof data.addr);
	data.name = loginName;
	if(!ServerThreadHandleValid(server)/* && IDCANCEL != DialogBoxParam(hInst, (LPCTSTR)IDD_JOINGAME, pc->w, JoinGameDlg, (LPARAM)&data)*/){
		static ServerParams std;
		DWORD dw;
		WSADATA wsaData;

		if(hRecvThread){
			CloseHandle(hRecvThread);
			hRecvThread = NULL;
		}

		int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
		if (iResult != NO_ERROR){
			OutputDebugString("Error at WSAStartup()\n");
			return 0;
		}

		SOCKET s;
		s = socketty();
		if(s == INVALID_SOCKET){
			WSACleanup();
			return FALSE;
		}
		con = s;

		{ /* connect to foreign host */
			sockaddr_in service;
			u_long ip;
			struct hostent *he;
			service.sin_family = AF_INET;
			ip = inet_addr(data.addr);
			if(ip == INADDR_NONE){
				struct hostent *he;
				he = gethostbyname(data.addr);
				if(!he){
					MessageBox(w, "gethostbyname() failed.\n"
					"Make sure the destination host name is valid and try again.", "Error", MB_OK);
					closesocket(s);
					WSACleanup();
					return FALSE;
				}
				service.sin_addr.s_addr = **(u_long**)he->h_addr_list;
			}
			else
				service.sin_addr.s_addr = ip;
			{
				int port;
				port = /*argc >= 2 ? atoi(argv[1]) : */data.port;
				service.sin_port = htons(port ? port : 10000);
		/*		printf("port: %d\n", port ? port : 10000);*/
			}
			if(connect(s, (SOCKADDR*) &service, sizeof(service)) == SOCKET_ERROR) {
				(0, "connect() failed.\n");
				MessageBox(w, "connect() failed.\n"
					"Make sure the destination address is correct and try again.", "Error", MB_OK);
				closesocket(s);
				WSACleanup();
				return 0;
			}
		}
		{
			DWORD dw;
			hRecvThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RecvThread, (LPVOID)this, 0, &dw);
		}
		{
			dstring ds = dstring() << "LOGIN " << data.name << "\r\n";
			send(s, ds, ds.len(), 0);
		}
		shutclient = false;
		mode = ClientWaitGame;
//		RedrawMenu(pc);
//		SetEvent(pc->hDrawEvent);
		return 1;
	}
	return 0;
}

void ClientApplication::errorMessage(const char *str){
	MessageBox(w, "Server could not start.", "", 0);
}


void ClientApplication::mouse_func(int button, int state, int x, int y){
	clientGame->mouse_func(button, state, x, y);
}


