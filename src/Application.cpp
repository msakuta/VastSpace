/** \file 
 * \brief Implementation of Application class and its descendants.
 *
 * This file's purpose is to provide shared codes among client and dedicated server.
 */

#include "Application.h"
#include "ClientMessage.h"
#include "Game.h"
#include "Player.h"
#include "Entity.h"
#include "CoordSys.h"
#include "stellar_file.h"
#include "cmd.h"
#include "Universe.h"
#include "sqadapt.h"
#include "EntityCommand.h"
#include "serial_util.h"

extern "C"{
#include <clib/timemeas.h>
#include <clib/c.h>
#include <clib/cfloat.h>
}
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include <cpplib/gl/cullplus.h>


#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <sstream>


#if 0
void Game::display_func(double dt){
#ifdef _WIN32
		sqa_anim(sqvm, dt);

		MotionFrame(dt);

		MotionAnim(*player, dt, flypower());

		player->anim(dt);

#if 0
#define TRYBLOCK(a) {try{a;}catch(std::exception e){fprintf(stderr, __FILE__"(%d) Exception %s\n", __LINE__, e.what());}catch(...){fprintf(stderr, __FILE__"(%d) Exception ?\n", __LINE__);}}
#else
#define TRYBLOCK(a) (a);
#endif
		if(!universe->paused) try{
			timemeas_t tm;
			TimeMeasStart(&tm);
			TRYBLOCK(anim(dt));
			TRYBLOCK(GLwindow::glwpostframe());
			TRYBLOCK(universe->endframe());
			TRYBLOCK(application.clientGame->universe->clientUpdate(dt));
			watime = TimeMeasLap(&tm);
		}
		catch(std::exception e){
			fprintf(stderr, __FILE__"(%d) Exception %s\n", __LINE__, e.what());
		}
		catch(...){
			fprintf(stderr, __FILE__"(%d) Exception ?\n", __LINE__);
		}

		if(glwfocus && cmdwnd)
			glwfocus = NULL;

		input_t inputs;
		inputs.press = MotionGet();
		inputs.change = MotionGetChange();
		if(joyStick.InitJoystick()){
			joyStick.CheckJoystick(inputs);
		}
		(*player->mover)(inputs, dt);
		if(player->nextmover && player->nextmover != player->mover){
/*			Vec3d pos = pl.pos;
			Quatd rot = pl.rot;*/
			(*player->nextmover)(inputs, dt);
/*			pl.pos = pos * (1. - pl.blendmover) + pl.pos * pl.blendmover;
			pl.rot = rot.slerp(rot, pl.rot, pl.blendmover);*/
		}

/*		if(player->chase && player->controlled == player->chase){
			inputs.analog[1] += mousedelta[0];
			inputs.analog[0] += mousedelta[1];
			player->chase->control(&inputs, dt);
		}*/

		if(player->controlled)
			glwfocus = NULL;

		// Really should be in draw method, since windows are property of the client.
		glwlist->glwAnim(dt);
#endif
}
#endif

	


struct CMHalt : public ClientMessage{
	typedef ClientMessage st;
	static CMHalt s;
	void interpret(ServerClient &sc, UnserializeStream &uss);
	static void send(Entity &);
private:
	static bool sqf_define(HSQUIRRELVM v);
	static SQInteger sqf_call(HSQUIRRELVM v);
	CMHalt();
};

CMHalt CMHalt::s;

CMHalt::CMHalt() : st("Halt"){
	defineMap()[id] = sqf_define;
}

void CMHalt::send(Entity &pt){
	if(pt.getGame()->isServer()){
		HaltCommand com;
		Game::IdMap::const_iterator it = application.serverGame->idmap().find(pt.getid());
		if(it != application.serverGame->idmap().end()){
			Entity *e = (Entity*)it->second;
//			if(e->race == application.serverGame->player->race)
				e->command(&com);
		}
	}
	else{
		std::stringstream ss;
		StdSerializeStream sss(ss);
		sss << pt.getid();
		std::string str = ss.str();
		s.st::send(application, str.c_str(), str.size());
	}
}

void CMHalt::interpret(ServerClient &sc, UnserializeStream &uss){
	HaltCommand com;
	Serializable::Id id;
	uss >> id;
	Game::IdMap::const_iterator it = sc.sv->pg->idmap().find(id);
	if(it != sc.sv->pg->idmap().end()){
		Entity *e = (Entity*)it->second;
		if(e->race == sc.sv->pg->players[sc.id]->race)
			e->command(&com);
	}
}

bool CMHalt::sqf_define(HSQUIRRELVM v){
	register_closure(v, _SC("CMHalt"), &sqf_call);
	return true;
}

SQInteger CMHalt::sqf_call(HSQUIRRELVM v){
	try{
		SQRESULT sr;
		Entity *p = Entity::sq_refobj(v, -1);
		send(*p);
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

/// \brief Unused
static int cmd_halt(int, char *[], void *pv){
	Player *pl = (Player*)pv;
	for(Player::SelectSet::iterator it = pl->selected.begin(); it != pl->selected.end(); it++){
		HaltCommand com;
		(*it)->command(&com);
		CMHalt::send(**it);
	}
	return 0;
}

struct CMMove : public ClientMessage{
	typedef ClientMessage st;
	static CMMove s;
	void interpret(ServerClient &sc, UnserializeStream &uss);
	static void send(const Vec3d &, Entity &);
private:
	CMMove() : st("Move"){}
};

int cmd_move(int argc, char *argv[], void *pv){
	Application *cl = (Application*)pv;
	if(!cl || !cl->clientGame)
		return 0;
	Player *pl = cl->clientGame->player;
	if(argc < 4 || !pl->cs)
		return 0;
	MoveCommand com;
	Vec3d comdst;
	comdst[0] = atof(argv[1]);
	comdst[1] = atof(argv[2]);
	comdst[2] = atof(argv[3]);
	com.destpos = comdst;
	Quatd headrot;
	int n = 0;
	for(Player::SelectSet::iterator it = pl->selected.begin(); it != pl->selected.end(); it++){
		Entity *pt = *it;
		if(pt->w == pl->cs->w){
			if(n == 0 && DBL_EPSILON < (comdst - pt->pos).slen())
				headrot = Quatd::direction(-(comdst - pt->pos));
			n++;
			Vec3d dp((n % 2 * 2 - 1) * (n / 2 * .05), 0., n / 2 * .05);
			com.destpos = comdst + headrot.trans(dp);
			pt->command(&com);

			CMMove::send(com.destpos, *pt);
		}
	}

	return 0;
}


CMMove CMMove::s;

void CMMove::send(const Vec3d &destpos, Entity &pt){
	if(application.mode & application.ServerBit){
		MoveCommand com;
		com.destpos = destpos;
		Game::IdMap::const_iterator it = application.serverGame->idmap().find(pt.getid());
		if(it != application.serverGame->idmap().end())
			((Entity*)it->second)->command(&com);
	}
	else{
		std::stringstream ss;
		StdSerializeStream sss(ss);
		sss << pt.getid() << destpos;
		std::string str = ss.str();
		s.st::send(application, str.c_str(), str.size());
	}
}

void CMMove::interpret(ServerClient &sc, UnserializeStream &uss){
	MoveCommand com;
	Serializable::Id id;
	uss >> id;
	uss >> com.destpos;
	Game::IdMap::const_iterator it = sc.sv->pg->idmap().find(id);
	if(it != sc.sv->pg->idmap().end())
		((Entity*)it->second)->command(&com);
}




/// \brief Returns maximum clients allowed in the hosted or joined game.
///
/// You cannot set the value with this console command, yet.
static int cmd_maxclients(int argc, char *argv[], void *pv){
	Application *app = (Application*)pv;
	if(app->server.sv){
		CmdPrint(gltestp::dstring() << "maxclient is " << app->server.sv->maxncl);
	}
	else{
		CmdPrint(gltestp::dstring() << "maxclient is " << app->serverParams.maxclients);
	}
	return 0;
}

/// \brief Sets or gets server update rate.
///
/// You cannot query this value from the client (at this time).
static int cmd_uprate(int argc, char *argv[], void *pv){
	Application *app = (Application*)pv;
	if(app->server.sv){
		if(argc < 2)
			CmdPrint(gltestp::dstring() << "UpdateInterval is " << app->server.sv->getUpdateInterval());
		else
			CmdPrint(gltestp::dstring() << "UpdateInterval is set to " << app->server.sv->setUpdateInterval(atof(argv[1])));
	}
	else
		CmdPrint("You are a client; UpdateInterval is unknown");
	return 0;
}

static gltestp::dstring dstrip(const sockaddr_in &s){
	unsigned long ipa;
	ipa = ntohl(s.sin_addr.s_addr);
	gltestp::dstring ret = (ipa>>24) & 0xff;
	ret << ".";
	ret << ((ipa>>16) & 0xff);
	ret << ".";
	ret << ((ipa>>8) & 0xff);
	ret << ".";
	ret << (ipa % 0x100);
	ret << ":";
	ret << ntohs(s.sin_port);
	return ret;
}

/// \brief Console command to return connected client list.
///
/// You cannot edit or something about the printed list.
///
/// Actually, the server's dummy client entry is printed too.
static int cmd_clients(int argc, char *argv[], void *pv){
	Application *app = (Application*)pv;
	if(app->server.sv){
		Server &server = *app->server.sv;
		if(lock_mutex(&server.mcl)){
			Server::ServerClientList::iterator it = server.cl.begin();
			for(; it != server.cl.end(); it++)
				CmdPrint(gltestp::dstring() << it->id << ": \"" << it->name << "\", " << dstrip(it->tcp));
			CmdPrint(gltestp::dstring() << server.cl.size() << " clients listed");
			unlock_mutex(&server.mcl);
		}
	}
#ifndef DEDICATED
	else{
		// TODO: mutex synchronization is necessary to avoid dirty writes from the ClientThread.
		ClientApplication *capp = (ClientApplication*)app;
		ClientApplication::ClientList::iterator it = capp->ad.begin();
		int i = 0;
		for(; it != capp->ad.end(); it++)
			CmdPrint(gltestp::dstring() << i++ << ": \"" << it->name << "\", " << dstrip(it->tcp));
		CmdPrint(gltestp::dstring() << capp->ad.size() << " clients listed");
	}
#endif
	return 0;
}





static int cmd_exit(int argc, char *argv[]){
#ifndef DEDICATED
	// Safely close the window
	PostMessage(application.w, WM_SYSCOMMAND, SC_CLOSE, 0);
#else
	exit(0);
#endif
	return 0;
}


static int cmd_sq(int argc, char *argv[], void *pv){
	Game *game = (Game*)pv;
	if(argc < 2){
		CmdPrint(gltestp::dstring() << "usage: " << argv[0] << " \"Squirrel One Linear Source\"");
		return 0;
	}
	HSQUIRRELVM &v = game->sqvm;
	if(SQ_FAILED(sq_compilebuffer(v, argv[1], strlen(argv[1]), _SC("sq"), SQTrue))){
		CmdPrint("Compile error");
	}
	else{
		sq_pushroottable(v);
		sq_call(v, 1, SQFalse, SQTrue);
		sq_pop(v, 1);
	}
	return 0;
}







static int cmd_say(int argc, char *argv[]){
	dstring text;
	for(int i = 1; i < argc; i++){
		if(i != 1)
			text << " ";
		text << argv[i];
	}
	application.sendChat(text);
	return 0;
}

void Application::sendChat(const char *buf){
	if(mode & ServerBit){
		if(server.sv)
			server.sv->SendChatMessage(buf);
	}
	else if(mode & ClientBit){
		dstring ds = dstring() << "SAY " << buf << "\r\n";
		send(con, ds, ds.len(), 0);
	}
}


Application::Application() :
	mode(InactiveGame),
	serverGame(NULL),
	clientGame(NULL),
	con(INVALID_SOCKET),
	logfile(NULL),
	loginName("The Client")
{
	serverParams.app = this;
	serverParams.hostname = "Server";
	serverParams.port = PROTOCOL_PORT;
	serverParams.maxclients = 2;
}

Application::~Application(){
	if(logfile)
		fclose(logfile);
}

void Application::init()
{
	viewport vp;
	CmdInit(&vp);
	CmdAdd("exit", cmd_exit);
	CmdAddParam("sq", cmd_sq, static_cast<Game*>(clientGame ? clientGame : serverGame)); // clientGame is preceded
	CmdAdd("say", cmd_say);
	CmdAddParam("move", cmd_move, this);
	CmdAddParam("maxclients", cmd_maxclients, this);
	CmdAddParam("clients", cmd_clients, this);
	CmdAddParam("uprate", cmd_uprate, this);
	CoordSys::registerCommands(&application);
	if(serverGame){
		CvarAdd("g_timescale", &serverGame->universe->timescale, cvar_double);
		CvarAdd("g_astro_timescale", &serverGame->universe->astro_timescale, cvar_double);
	}
//	Player::cmdInit(application);

	if(serverGame){
		// Explicitly add sq command for the server game.
		// The static_cast is necessary if ServerGame virtually inherits Game.
		CmdAddParam("ssq", cmd_sq, static_cast<Game*>(serverGame));
		sqa_init(serverGame);
	}
	if(clientGame){
		// Explicitly add sq command for the client game
		// The static_cast is necessary if ClientGame virtually inherits Game.
		CmdAddParam("csq", cmd_sq, static_cast<Game*>(clientGame));
		if(serverGame != clientGame) // Prevent double initialization
			sqa_init(clientGame);
	}
	if(mode & ServerBit){
		HSQUIRRELVM v = serverGame->sqvm;
		const SQChar *s = _SC("space.dat");
		StackReserver sr(v);
		sq_pushroottable(v); // root
		sq_pushstring(v, _SC("stellar_file"), -1); // root "init_Universe"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root closure
			sq_getstring(v, -1, &s);
		}
		serverGame->StellarFileLoad(s);
	}

#if 0
	if(isClient)
		application.joingame();
	else
		application.hostgame(server);
#endif
}

bool Application::parseArgs(int argc, char *argv[]){
	for(int i = 1; i < argc; i++){
		if(!strcmp(argv[i], "-h")){ // host
			if(argc <= i+1){
				fprintf(stderr, "Argument for -h parameter is missing.\n");
				return false;
			}
			serverParams.hostname = argv[i+1];
			i++;
		}
		else if(!strcmp(argv[i], "-p")){ // port
			if(argc <= i+1){
				fprintf(stderr, "Argument for -p parameter is missing.\n");
				return false;
			}
			serverParams.port = atoi(argv[i+1]);
			i++;
		}
		else if(!strcmp(argv[i], "-m")){ // maxclients
			if(argc <= i+1){
				fprintf(stderr, "Argument for -m parameter is missing.\n");
				return false;
			}
			serverParams.maxclients = atoi(argv[i+1]);
			i++;
		}
		else if(!strcmp(argv[i], "-n")){ // login name
			if(argc <= i+1){
				fprintf(stderr, "Argument for -n parameter is missing.\n");
				return false;
			}
			loginName = argv[i+1];
			i++;
		}
		else if(!strcmp(argv[i], "-c")){ // client
			mode = GameMode(mode | ClientBit);
		}
	}
	return true;
}

bool Application::hostgame(Game *game, int port){
	if(!ServerThreadHandleValid(server)){
		if(!StartServer(&serverParams, &server)){
			errorMessage("Server could not start.");
			return false;
		}
		server.sv->pg = game;
		mode = ServerGame;
		return true;
	}
	return false;
}

bool Application::standalone(Game *game){
	mode = StandaloneGame;
	serverGame = clientGame = game;
	return true;
}

void Application::signalMessage(const char *text){
	CmdPrint(dstring() << "MSG> " << text);
	if(logfile)
		fprintf(logfile, "%s\n", text);
}

void Application::errorMessage(const char *str){
	fprintf(stderr, "%s\n", str);
}



//-----------------------------------------------------------------------------
//  ClientMessage implementation
//-----------------------------------------------------------------------------

ClientMessage::CtorMap &ClientMessage::ctormap(){
	static CtorMap s;
	return s;
}

ClientMessage::ClientMessage(gltestp::dstring id){
	this->id = id;
	ctormap()[id] = this;
}

ClientMessage::~ClientMessage(){
	ctormap().erase(id);
}

bool ClientMessage::sq_send(Application &app, HSQUIRRELVM v)const{
	return false;
}

void ClientMessage::send(Application &cl, const void *p, size_t size){
#ifdef _WIN32
	if(cl.mode & cl.ServerBit){
		ClientMessage::CtorMap::iterator it = ClientMessage::ctormap().find(id);
		if(it != ClientMessage::ctormap().end()){
			std::istringstream iss(std::string((const char*)p, strlen((const char*)p)));
			StdUnserializeStream uss(iss);
			UnserializeContext usc(uss, Serializable::ctormap(), (UnserializeMap&)cl.serverGame->idmap());
			uss.usc = &usc;
			ServerClient dummyServerClient;
			dummyServerClient.id = 0;
			it->second->interpret(cl.server.sv ? *cl.server.sv->scs : dummyServerClient, uss);
		}
	}
	else{
		dstring vb = "C ";
		vb << id << ' ';
		vb.strncat((const char*)p, size);
		vb << "\r\n";
		::send(cl.con, vb, vb.len(), 0);
	}
#endif
}



