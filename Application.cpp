/** \file 
 * \brief Implementation of Application class and its descendants.
 *
 * This file's purpose is to provide shared codes among client and dedicated server.
 */

#include "Application.h"
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

	

namespace sqa{
typedef std::map<dstring, bool (*)(HSQUIRRELVM)> SQDefineMap;

extern SQDefineMap &defineMap();
}

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
	if(application.mode & application.ServerBit){
		HaltCommand com;
		Game::IdMap::const_iterator it = application.pg->idmap().find(pt.getid());
		if(it != application.pg->idmap().end())
			((Entity*)it->second)->command(&com);
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
	if(it != sc.sv->pg->idmap().end())
		((Entity*)it->second)->command(&com);
}

bool CMHalt::sqf_define(HSQUIRRELVM v){
	register_closure(v, _SC("CMHalt"), &sqf_call);
	return true;
}

SQInteger CMHalt::sqf_call(HSQUIRRELVM v){
	try{
		SQRESULT sr;
		Entity *p;
		if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
			return sr;
		send(*p);
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}

int cmd_halt(int, char *[], void *pv){
	Player *pl = (Player*)pv;
	for(std::set<Entity*>::iterator it = pl->selected.begin(); it != pl->selected.end(); it++){
		(*it)->command(&HaltCommand());
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
		Game::IdMap::const_iterator it = application.pg->idmap().find(pt.getid());
		if(it != application.pg->idmap().end())
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






static int cmd_exit(int argc, char *argv[]){
#ifdef _WIN32
	// Safely close the window
	PostMessage(application.w, WM_SYSCOMMAND, SC_CLOSE, 0);
#else
	exit(0);
#endif
	return 0;
}


static int cmd_sq(int argc, char *argv[]){
	if(argc < 2){
		CmdPrint("usage: sq \"Squirrel One Linear Source\"");
		return 0;
	}
	HSQUIRRELVM &v = g_sqvm;
	if(SQ_FAILED(sq_compilebuffer(g_sqvm, argv[1], strlen(argv[1]), _SC("sq"), SQTrue))){
		CmdPrint("Compile error");
	}
	else{
		sq_pushroottable(g_sqvm);
		sq_call(g_sqvm, 1, SQFalse, SQTrue);
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
	if(mode == ServerWaitGame || mode == ServerGame){
#ifdef _WIN32
		SendChatServer(&server, buf);
#endif
	}
	else if(mode == ClientWaitGame || mode == ClientGame){
		dstring ds = dstring() << "SAY " << buf << "\r\n";
		send(con, ds, ds.len(), 0);
	}
}


void Application::init(bool isClient)
{
	Game *&server = pg;

	viewport vp;
	CmdInit(&vp);
	CmdAdd("exit", cmd_exit);
	CmdAdd("sq", cmd_sq);
	CmdAdd("say", cmd_say);
	CmdAddParam("move", cmd_move, this);
	CoordSys::registerCommands(application.clientGame);
	if(server){
		CvarAdd("pause", &server->universe->paused, cvar_int);
		CvarAdd("g_timescale", &server->universe->timescale, cvar_double);
		CvarAdd("seat", &server->player->chasecamera, cvar_int);
	}
	CvarAdd("g_astro_timescale", &OrbitCS::astro_timescale, cvar_double);
//	Player::cmdInit(application);

	if(server)
		sqa_init(server);
	if(clientGame)
		sqa_init(clientGame);
	if(!isClient){
		HSQUIRRELVM v = pg->sqvm;
		const SQChar *s = _SC("space.dat");
		StackReserver sr(v);
		sq_pushroottable(v); // root
		sq_pushstring(v, _SC("stellar_file"), -1); // root "init_Universe"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root closure
			sq_getstring(v, -1, &s);
		}
		StellarFileLoad(s, server->universe);
	}

#if 0
	if(isClient)
		application.joingame();
	else
		application.hostgame(server);
#endif
}



//-----------------------------------------------------------------------------
//  ClientMessage implementation
//-----------------------------------------------------------------------------

ClientMessage::CtorMap &ClientMessage::ctormap(){
	static CtorMap s;
	return s;
}

ClientMessage::ClientMessage(dstring id){
	this->id = id;
	ctormap()[id] = this;
}

ClientMessage::~ClientMessage(){
	ctormap().erase(id);
}

void ClientMessage::send(Application &cl, const void *p, size_t size){
#ifdef _WIN32
	if(cl.mode & cl.ServerBit){
		ClientMessage::CtorMap::iterator it = ClientMessage::ctormap().find(id);
		if(it != ClientMessage::ctormap().end()){
			std::istringstream iss(std::string((const char*)p, strlen((const char*)p)));
			StdUnserializeStream uss(iss);
			it->second->interpret(*cl.server.sv->scs, uss);
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


