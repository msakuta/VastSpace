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
	char addr[256], name[MAX_HOSTNAME];
	u_short port;
	FILE **plogfile;
};


ClientApplication::ClientApplication() : recvbuf(NULL), recvbufsiz(0){
	hGameMutex = CreateMutex(NULL, FALSE, NULL);
}

ClientApplication::~ClientApplication(){
	CloseHandle(hGameMutex);
	delete[] recvbuf;
}


static void quitgame(ClientApplication *, bool ret){
	exit(ret);
}

static void SignalMessage(const char *text, void *){
	CmdPrint(dstring() << "MSG> " << text);
	for(std::set<GLWchat*>::iterator it = GLWchat::wndlist.begin(); it != GLWchat::wndlist.end(); it++)
		(*it)->append(text);
}



static DWORD WINAPI RecvThread(ClientApplication *pc){
	int size;
	char buf[1024], *lbuf = NULL, *src = NULL;
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

		while(p = strpbrk(lbuf, "\r\n")){/* some terminals doesn't end line with \n? */
			char *np = p;
			while(*np == '\r' || *np == '\n'){
				np++;
				assert(np - lbuf <= lbp);
			}
			*p = '\0';
			switch(mode){
			case -1:
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
				break;
			default:
			case 0:
				if(!strncmp(lbuf, "Modified", sizeof"Modified"-1)){
					mode = 1;
				}
				else if(!strcmp(lbuf, "SERIALIZE START")){
					mode = 4;
				}
				else if(!strncmp(lbuf, "MSG ", 4)){
					SignalMessage(&lbuf[4], (void*)pc);
				}
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
//					static_cast<ClientWaiter*>(pc->waiter)->m = n;
//					if(pc->pvlist)
//						free(pc->pvlist);
//					pc->pvlist = new PictVertexList[pc->ncl = n];
				}
				else if(!strcmp(lbuf, "START")){
					assert(!pc->pg);
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
						ClientApplication::ClientClient &cc = pc->ad[modi];

						strncpy(ipbuf, lbuf, 8+1+4);
						ipbuf[8+1+4] = '\0';
						namebuf = &lbuf[8+1+4];

						// Parse ip and port
						if(sscanf(ipbuf, "%08x:%4x", &ip, &port)){
							cc.ip = ip;
							cc.port = port;
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
				if(WAIT_OBJECT_0 == WaitForSingleObject(pc->hGameMutex, 1000)){
					char sizebuf[16];
					char *endptr;
					strncpy(sizebuf, lbuf, 8);
					sizebuf[8] = '\0';
					pc->recvbufsiz = strtoul(sizebuf, &endptr, 16);
					delete[] pc->recvbuf;
					pc->recvbuf = new unsigned char[pc->recvbufsiz];
					int i = 0;
					for(const char *p = &lbuf[8]; p && i < pc->recvbufsiz; i++){
						sizebuf[0] = *p++;
						sizebuf[1] = *p++;
						sizebuf[2] = '\0';
						pc->recvbuf[i] = (unsigned char)strtoul(sizebuf, &endptr, 16);
					}

					ReleaseMutex(pc->hGameMutex);
				}
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


int ClientApplication::joingame(){
	JoinGameData data;
//	quitgame(pc);
//	data.plogfile = &pc->logfile;
	data.port = PROTOCOL_PORT;
	strncpy(data.addr,  "127.0.0.1", sizeof data.addr);
	strncpy(data.name, "The Client", sizeof data.name);
	if(!ServerThreadHandleValid(server)/* && IDCANCEL != DialogBoxParam(hInst, (LPCTSTR)IDD_JOINGAME, pc->w, JoinGameDlg, (LPARAM)&data)*/){
		static ServerThreadData std;
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
/*		pc->waiter = new ClientWaiter();
		pc->attr[0] = 33;
		pc->attr[1] = 33;
		pc->attr[2] = 34;*/
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

void ClientApplication::hostgame(Game *game){
#if 0
	static bool init = false;
	static DLGTEMPLATE *dt;
	if(!init){
		DLGITEMTEMPLATE *it;
		init = true;
		dt = (DLGTEMPLATE*)malloc(sizeof*dt + sizeof*it);
		dt->style = WS_CAPTION | WS_VISIBLE;
		dt->dwExtendedStyle = 0;
		dt->cdit = 1;
		dt->x = 200;
		dt->y = 200;
		dt->cx = 200;
		dt->cy = 200;
		it = (DLGITEMTEMPLATE*)&dt[1];
		it->style = 0;
		it->dwExtendedStyle = 0;
		it->x = 20;
		it->y = 20;
		it->cx = 30;
		it->cy = 30;
		it->id = IDOK;
	}
	DialogBoxIndirect(hInst, dt, pc->w, (DLGPROC)About);
#else
//	quitgame(pc);
	ServerThreadData gstd;
//	HostGameDlgData data;
//	data.pstd = &gstd;
//	data.plogfile = &pc->logfile;
	gstd.maxclients = 2;
	gstd.client = (void*)this;
	gstd.port = PROTOCOL_PORT;
	if(!ServerThreadHandleValid(server) /*&& IDCANCEL != DialogBoxParam(hInst, (LPCTSTR)IDD_HOSTGAME, pc->w, HostGameDlg, (LPARAM)&data)*/){
		strncpy(gstd.hostname, "localhost", sizeof gstd.hostname);
//		gstd.modified = (void(*)(void*))SignalEvent;
//		gstd.modified_data = (void*)pc->hDrawEvent;
		gstd.message = SignalMessage;
		gstd.message_data = this;
//		gstd.already = SignalAlready;
//		gstd.already_data = pc;
//		gstd.moveto = SignalMoveto;
//		gstd.moveto_data = pc;
//		gstd.lineto = SignalLineto;
//		gstd.lineto_data = pc;
/*		hServerThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ServerThread, &std, 0, &dw);*/
		if(!StartServer(&gstd, &server)){
			MessageBox(w, "Server could not start.", "", 0);
			return;
		}
		server.sv->pg = game;
//		pc->waiter = new ServerWaiter(&pc->server);
//		pc->attr[0] = 33;
//		pc->attr[1] = 33;
//		pc->attr[2] = 34;
//		pc->pvlist = new PictVertexList[pc->ncl = gstd.maxclients];
		mode = ServerWaitGame;
//		RedrawMenu(pc);
//		DrawWaiting(pc);
	}
#endif
}

void ClientApplication::mouse_func(int button, int state, int x, int y){
	clientGame->mouse_func(button, state, x, y);
}


