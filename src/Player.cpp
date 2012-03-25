/** \file
 * \brief Implementation of Player class and its companions.
 */
#include "Player.h"
#include "Application.h"
#include "Universe.h"
#include "Entity.h"
#include "cmd.h"
#include "CoordSys.h"
#include "astro.h"
#include "serial_util.h"
#include "stellar_file.h"
#include "sqadapt.h"
#include "btadapt.h"
#include "glw/glwindow.h"
#include "motion.h"
#include "ClientMessage.h"
extern "C"{
#include <clib/mathdef.h>
#include <clib/cfloat.h>
}
#include <sstream>


/// \brief The client message that tells the server that this Player wants to change rotation.
struct CMMover : ClientMessage{
	typedef ClientMessage st;
	static CMMover s;
	void interpret(ServerClient &sc, UnserializeStream &uss);
	enum Type{Freelook, Chase, Tactical};
	static void send(Type);
private:
	CMMover() : st("Mover"){}
};

/// \brief The client message that tells the server that this Player wants to change rotation.
struct CMRot : ClientMessage{
	typedef ClientMessage st;
	static CMRot s;
	void interpret(ServerClient &sc, UnserializeStream &uss);
	static void send(const Quatd &);
private:
	CMRot() : st("Rot"){}
};

/// \brief The client message to request chase target change.
struct CMChase : ClientMessage{
	typedef ClientMessage st;
	static CMChase s;
	void interpret(ServerClient &sc, UnserializeStream &uss);
	static void send(Entity *);
private:
	CMChase() : st("Chase"){}
};

/// \brief The client message for changing chase camera number.
struct CMChaseCamera : ClientMessage{
	typedef ClientMessage st;
	static CMChaseCamera s;
	void interpret(ServerClient &sc, UnserializeStream &uss);
	static void send(int camera);
public:
	CMChaseCamera() : st("ChaseCamera"){}
};

/// \brief The client message that tells the server that this Player wants to change rotation.
struct CMViewDist : ClientMessage{
	typedef ClientMessage st;
	static CMViewDist s;
	void interpret(ServerClient &sc, UnserializeStream &uss);
	static void send(double viewdist);
private:
	CMViewDist() : st("ViewDist"){}
};



float Player::camera_mode_switch_time = 1.f;
int Player::g_overlay = 1;

//static teleport *tplist;
//static int ntplist;

static long framecount = 0;

void Player::CameraController::operator ()(const input_t &inputs, double dt){}

Vec3d Player::CameraController::getpos()const{
	return pl.pos;
}

Quatd Player::CameraController::getrot()const{
	return pl.rot;
}

// Response to rotational movement depends on mover.
void Player::CameraController::rotateLook(double dx, double dy){
	Quatd &rot = pl.rot;
	const double speed = .001 / 2. * pl.fov;
	const double one_minus_epsilon = .999;

	rot = rot.quatrotquat(Vec3d(0, dx * speed, 0));
	rot = rot.quatrotquat(Vec3d(dy * speed, 0, 0));

	// Notify the server that this client wants the angle changed.
	CMRot::send(rot);
}


class CockpitviewMover : public Player::CameraController{
public:
	typedef Player::CameraController st;
	double py[2]; /// Pitch and yaw
	CockpitviewMover(Player &a) : st(a){py[0] = py[1] = 0.;}
	virtual Quatd getrot()const{
		Quatd pyrot = Quatd::rotation(py[1], 1, 0, 0).rotate(py[0], 0, 1, 0);
		if(!pl.chase)
			return pyrot;
		Vec3d dummy;
		Quatd crot;
		pl.chase->cockpitView(dummy, crot, pl.getChaseCamera());
		return /*pyrot */ crot.cnj();
	}
	virtual Vec3d getpos()const{
		if(pl.chase){
			Vec3d pos;
			Quatd dummy;
			pl.chase->cockpitView(pos, dummy, pl.getChaseCamera());
			return pos;
		}
		return Vec3d(0,0,0);
	}
	virtual void rotateLook(double dx, double dy){
		const double speed = .001 / 2. * pl.fov;
		py[0] += dy * speed;
		py[1] += dx * speed;
	}
/*	virtual void operator ()(const input_t &input, double dt){
		const_cast<double&>(input.analog[0]) = py[0];
		const_cast<double&>(input.analog[1]) = py[1];
	}*/
};

FreelookMover::FreelookMover(Player &a) : st(a), flypower(1.), pos(a.pos){}

Quatd FreelookMover::getrot()const{
	if(!pl.chase)
		return pl.rot;
	Vec3d dummy;
	Quatd crot;
	pl.chase->cockpitView(dummy, crot, pl.chasecamera);
	return pl.rot * crot.cnj();
}

Vec3d FreelookMover::getpos()const{
	return pos;
}

void FreelookMover::setpos(const Vec3d &apos){
	pos = apos;
}


class TacticalMover : public Player::CameraController{
public:
	typedef Player::CameraController st;
	Vec3d pos;
	Quatd rot;
	TacticalMover(Player &a) : st(a), pos(0,0,0), rot(0,0,0,1){}
	void operator ()(const input_t &inputs, double dt);
	Quatd getrot()const{return rot;}
	Vec3d getpos()const{return pos;}
	void setrot(const Quatd &arot){rot = arot;}
	void rotateLook(double dx, double dy);
};

// Response to rotational movement depends on mover.
void TacticalMover::rotateLook(double dx, double dy){
	const double speed = .001 / 2. * pl.fov;
	const double one_minus_epsilon = .999;

	// Some WarSpaces have special rules for which direction is up.
	WarSpace *ws;
	Quatd ort = quat_u;
	if(pl.cs->w && (ws = *pl.cs->w)){
		ort = ws->orientation(pl.cpos);
	}
	Quatd rot = this->rot * ort;

	// Fully calculate pitch, yaw and roll from quaternion.
	// This calculation is costly, but worth doing everytime mouse moves, for majority of screen is affected.
	Vec3d view = rot.itrans(vec3_001);
	double phi = -atan2(view[0], view[2]);
	double theta = atan2(view[1], sqrt(view[2] * view[2] + view[0] * view[0]));
	Quatd rot1 = Quatd(sin(theta/2), 0, 0, cos(theta/2)) * Quatd(0, sin(phi/2), 0, cos(phi/2));
	Quatd rot2 = rot * rot1.cnj();
	Vec3d right = rot2.itrans(vec3_100);

	// Add in instructed movement here.
	phi += dx * speed;
	theta = rangein(theta + dy * speed, -M_PI / 2. * one_minus_epsilon, M_PI / 2. * one_minus_epsilon);
	double roll = -atan2(right[1], right[0]);
	this->rot = Quatd(0, 0, sin(roll/2), cos(roll/2)) * Quatd(sin(theta/2), 0, 0, cos(theta/2)) * Quatd(0, sin(phi/2), 0, cos(phi/2)) * ort.cnj();
}

Player::Player(Game *game) : st(game), pos(Vec3d(0,0,0)), velo(Vec3d(0,0,0)), accel(0,0,0), rot(quat_u), rad(0.001), lastchase(NULL),
	chasecamera(0), detail(0), mousex(0), mousey(0), race(0), fov(1.), viewdist(1.),
	gametime(0), velolen(0),
	nextmover(NULL), blendmover(0), attackorder(0), forceattackorder(0),
	cpos(0,0,0),
	r_move_path(false), r_attack_path(false), r_overlay(false),
	chase(NULL), controlled(NULL), moveorder(false), move_lockz(false), move_z(0.), move_org(Vec3d(0,0,0)), move_hitpos(Vec3d(0,0,0)),
	freelook(NULL), cockpitview(NULL), tactical(NULL)
{
	// Special case that needs const member variables to be initialized outside initialization list.
	// We do not want to alter these pointers outside the constructor, but Player class is not yet
	// initialized at the time initialization list is executed.
	// Class object members need not to warry about read-only memory storage class.
	const_cast<FreelookMover*&>(freelook) = new FreelookMover(*this);
	const_cast<CameraController*&>(cockpitview) = new CockpitviewMover(*this);
	const_cast<CameraController*&>(tactical) = new TacticalMover(*this);
	mover = freelook;
}

Player::~Player(){
	free();
}

const unsigned Player::classId = registerClass("Player", Conster<Player>);

void Player::free(){
}

/// \brief Setter accessor for chasecamera member. Sends client message to the server.
void Player::setChaseCamera(int i){
	chasecamera = i;
	if(!game->isServer())
		CMChaseCamera::s.send(i);
}

CMChaseCamera CMChaseCamera::s;

void CMChaseCamera::send(int camera){
	std::stringstream ss;
	StdSerializeStream sss(ss);
	sss << camera;
	std::string str = ss.str();
	s.st::send(application, str.c_str(), str.size());
}

/// \brief A server command that accepts messages from the client to change the direction of sight.
void CMChaseCamera::interpret(ServerClient &sc, UnserializeStream &uss){
	int n;
	uss >> n;
	Player *player;
#ifndef _WIN32
	player = sc.sv->pg->players[sc.id];
#else
	player = application.serverGame->players[sc.id];
#endif
	if(player)
		player->setChaseCamera(n);
}


inline Quatd Player::getrawrot(CameraController *mover)const{
	return mover->getrot();
}

inline Vec3d Player::getrawpos(CameraController *mover)const{
	return mover->getpos();
}

Quatd Player::getrot()const{
	if(nextmover && mover != nextmover){
		return Quatd::slerp(getrawrot(mover), getrawrot(nextmover), blendmover).normin();
	}
	else
		return getrawrot(mover);
}

Vec3d Player::getpos()const{
	if(nextmover && mover != nextmover){
		return getrawpos(mover) * (1. - blendmover) + getrawpos(nextmover) * blendmover;
	}
	else
		return getrawpos(mover);
}

const char *Player::classname()const{
	return "Player";
}

void Player::serialize(SerializeContext &sc){
	Serializable::serialize(sc);
	sc.o << playerId;
	sc.o << chase;
	sc.o << (unsigned)selected.size();
	for(SelectSet::iterator it = selected.begin(); it != selected.end(); it++)
		sc.o << *it;
	sc.o << chasecamera;
	sc.o << pos << velo << accel;
	sc.o << rot;
	sc.o << fov;
	sc.o << viewdist;
	sc.o << cs;
	sc.o << (unsigned)tplist.size();
	for(int i = 0; i < tplist.size(); i++)
		tplist[i].serialize(sc);
}

void Player::unserialize(UnserializeContext &sc){
	unsigned selectedSize;
//	selected.clear();
	unsigned ntplist;

	sc.i >> playerId;
	sc.i >> chase;
	sc.i >> selectedSize;
	for(int i = 0; i++; i < selectedSize){
		Entity *e;
		sc.i >> e;
		// Read and discard stream
//		selected.insert(e);
	}
	sc.i >> chasecamera;
	sc.i >> pos >> velo >> accel;

	// Ignore rotation updates if this Player is me, in order to prevent lagged server
	// from slowing the client's camera.
	if(application.clientGame && application.clientGame->player == this){
		Quatd dummyrot;
		sc.i >> dummyrot;
	}
	else
		sc.i >> rot;

	sc.i >> fov;
	sc.i >> viewdist;
	sc.i >> cs;
	sc.i >> ntplist;
	tplist.resize(ntplist);
	for(int i = 0; i < ntplist; i++)
		tplist[i].unserialize(sc);
}

void Player::anim(double dt){
	framecount++;
	aviewdist += (viewdist - aviewdist) * (1. - exp(-10. * dt));
	const Universe *u = cs->findcspath("/")->toUniverse();
	if(u && !u->paused && nextmover && mover != nextmover){
		// We do not want the camera to teleport when setting camera_mode_switch_time while transiting.
		blendmover += float(dt / camera_mode_switch_time);
		if(1. < blendmover)
			mover = nextmover;
	}
}

void Player::transit_cs(CoordSys *cs){
	// If chasing in free-look mode, rotation is converted by the Entity being chased,
	// so we do not need to convert here.
	if(!chase || mover == tactical){
		Quatd rot = cs->tocsq(this->cs);
		this->setrot(getrot() * rot);
		this->setvelo(rot.cnj().trans(this->getvelo()));
	}
	if(!chase)
		this->pos = cs->tocs(this->pos, this->cs);
	this->cs = cs;
}

bool Player::unlink(Observable *pe){
//	chases.erase(reinterpret_cast<Entity*>(pe));
	// chases.erase() cannot be used because the raw pointer value changes when upcasting from multiple-inherited object to
	// the super class, and we do not use dynamic cast. We must iterate and find the pointer in the set to erase one.
	for(ChaseSet::const_iterator it = chases.begin(); it != chases.end();){
		if(*it == pe){
			ChaseSet::const_iterator next = it;
			++next;
			chases.erase(it);
			it = next;
		}
		else
			++it;
	}
	if(controlled == pe)
		controlled = NULL;
	if(lastchase == pe)
		lastchase = NULL;
	SelectSet::iterator ppe;
	for(ppe = selected.begin(); ppe != selected.end(); ppe++) if(*ppe == pe){
		selected.erase(ppe);
		break;
	}
	return true;
}

void Player::rotateLook(double dx, double dy){
	mover->rotateLook(dx, dy);
}

void FreelookMover::setGear(int g){
	gear = g;
	flypower = .05 * pow(16, double(g));
}

void FreelookMover::operator ()(const input_t &inputs, double dt){
	double accel = flypower * (1. + (8 <= gear ? pl.velolen / flypower : 0));
	int inputstate = inputs.press;
//	Vec3d &pos = pl.pos;
	Vec3d &velo = pl.velo;
	Quatd &rot = pl.rot;
	if(inputstate & PL_W)
		velo -= rot.itrans(vec3_001) * dt * accel;
	if(inputstate & PL_S)
		velo += rot.itrans(vec3_001) * dt * accel;
	if(inputstate & PL_A)
		velo -= rot.itrans(vec3_100) * dt * accel;
	if(inputstate & PL_D)
		velo += rot.itrans(vec3_100) * dt * accel;
	if(inputstate & PL_Z)
		velo -= rot.itrans(vec3_010) * dt * accel;
	if(inputstate & PL_Q)
		velo += rot.itrans(vec3_010) * dt * accel;
	if(inputstate & PL_E)
		velo = Vec3d(0,0,0);

	Entity *chase = pl.chase;
	if(chase){
		WarSpace *ws = (WarSpace*)chase->w;
		if(ws)
			pl.cs = ws->cs;
		Quatd dummy;
		chase->cockpitView(pos, dummy, pl.chasecamera);
		velo = chase->velo;
	}
	else{
		pl.velolen = velo.len();
		pos += velo /** (1. + velolen)*/ * dt;
		Vec3d pos;
		const CoordSys *cs = pl.cs->belongcs(pos, pl.getpos());
		if(cs != pl.cs){
			Quatd rot = cs->tocsq(pl.cs);
			pl.rot *= rot;
			pl.setvelo(rot.cnj().trans(pl.velo));
			pl.cs = cs;
			pl.setpos(pos);
		}
	}
}

void TacticalMover::operator()(const input_t &inputs, double dt){
//	Vec3d &pos = pl.pos;
	Vec3d &velo = refvelo();
//	Quatd &rot = pl.rot;
	Vec3d &cpos = pl.cpos;
	Player::ChaseSet &chases = pl.chases;
	Entity *chase = pl.chase;

	// Some WarSpaces have special rules for which direction is up.
	// Camera rotation by mouse rotation should follow the rules when in trackball type rotation.
	WarSpace *ws;
	Quatd ort = quat_u;
	if(pl.cs->w && (ws = *pl.cs->w)){
		ort = ws->orientation(pl.cpos);
	}
	Quatd rot = this->rot * ort;

	Vec3d view = rot.itrans(vec3_001);
	double phi = -atan2(view[0], view[2]);
	double theta = atan2(view[1], sqrt(view[2] * view[2] + view[0] * view[0]));
	double roll;
	{
		Quatd rot1 = Quatd(sin(theta/2), 0, 0, cos(theta/2)) * Quatd(0, sin(phi/2), 0, cos(phi/2));
		Quatd rot2 = rot * rot1.cnj();
		Vec3d right = rot2.itrans(vec3_100);
		roll = -atan2(right[1], right[0]);
		roll *= exp(-dt);
		this->rot = Quatd(0, 0, sin(roll/2), cos(roll/2)) * rot1 * ort.cnj();
	}
	if(!chases.empty()){
		int n = 0;
		for(Player::ChaseSet::iterator it = chases.begin(); it != chases.end(); it++){
			cpos = (cpos * n + (*it)->pos) / (n + 1);
			n++;
		}
	}
	else if(chase && chase->w->cs == pl.cs){
		cpos = chase->pos;
	}
	pos = cpos + this->rot.itrans(vec3_001) * pl.aviewdist;
}

template<typename T, T Player::*M>
int cmd_cvar(int argc, char *argv[], void *pv){
	Player *ppl = (Player*)pv;
	if(argc < 2)
		CmdPrint(cpplib::dstring(argv[0]) << " = " << ppl->*M);
	else if(toupper(argv[1][0]) == 'T')
		(ppl->*M) = !(ppl->*M);
	else
		(ppl->*M) = !!atoi(argv[1]);
	return 0;
}



CMRot CMRot::s;

/// \brief Sends the command message to the server that this client wants to change direction of sight.
static int cmd_rot(int argc, char *argv[]){
#ifdef _WIN32
	if(!(application.mode & application.ServerBit)){
		Quatd q;
		int c = min(argc - 1, 4);
		for(int i = 0; i < c; i++)
			q[i] = atof(argv[i+1]);
		if(DBL_EPSILON < q.slen()){
			q.normin();
			CMRot::send(q);
		}
	}
#endif
	return 0;
}

/// \brief Sends to the server that this player wants to change rotation to the given quaternion.
///
/// Note that the argument is not the delta, but absolute rotation in the current CoordSys which is the Player
/// lives in. This is to prevent numerical errors to sum up.
void CMRot::send(const Quatd &q){
	std::stringstream ss;
	StdSerializeStream sss(ss);
	for(int i = 0; i < 4; i++)
		sss << q[i];
	std::string str = ss.str();
	s.st::send(application, str.c_str(), str.size());
}

/// \brief A server command that accepts messages from the client to change the direction of sight.
void CMRot::interpret(ServerClient &sc, UnserializeStream &uss){
	Quatd q;//(atof(argv[1]), atof(argv[2]), atof(argv[3]), atof(argv[4]));
	for(int i = 0; i < 4; i++)
		uss >> q[i];
	Player *player;
#ifndef _WIN32
	player = sc.sv->pg->players[sc.id];
#else
	player = application.serverGame->players[sc.id];
#endif
	if(player)
		player->setrot(q);
}

/// \brief Sends the command message to the server that this client wants to change position of eyes.
static int cmd_pos(int argc, char *argv[]){
#ifdef _WIN32
	if(!(application.mode & application.ServerBit))
	{
		dstring ds = "C spos";
		for(int i = 1; i < argc; i++)
			ds << " " << argv[i];
		ds << "\r\n";
		send(application.con, ds, ds.len(), 0);
	}
#endif
	return 0;
}

/// \brief A server command that accepts messages from the client to change the position of eyes.
static int scmd_spos(int argc, char *argv[], ServerClient *sc){
	if(argc == 4){
		Vec3d v(atof(argv[1]), atof(argv[2]), atof(argv[3]));
#ifdef _WIN32
		application.serverGame->players[sc->id]->setrot(v);
#else
		sc->sv->pg->players[sc->id]->setrot(v);
#endif
	}
	return 0;
}

static int cmd_chasecamera(int argc, char *argv[]){
	Game *server = application.serverGame;
	if(application.clientGame){
		Player *player = application.clientGame->player;
		if(player && !player->selected.empty()){
			player->cs = (*player->selected.begin())->w->cs;
			player->chases.clear();
			for(Player::SelectSet::iterator it = player->selected.begin(); it != player->selected.end(); it++)
				player->chases.insert(*it);
			CMChase::send(*player->selected.begin());
		}
	}
	else if(server && !server->player->selected.empty()){
		server->player->chase = *server->player->selected.begin();
		server->player->cs = (*server->player->selected.begin())->w->cs;
		server->player->chases.clear();
		for(Player::SelectSet::iterator it = server->player->selected.begin(); it != server->player->selected.end(); it++)
			server->player->chases.insert(*it);
	}
	return 0;
}

void Player::cmdInit(ClientApplication &application){
#ifdef _WIN32
	CmdAdd("chasecamera", cmd_chasecamera);
	CmdAddParam("mover", cmd_mover, application.clientGame);
	if(!application.serverGame)
		return;
	Player &pl = *application.serverGame->player;
	CmdAdd("rot", cmd_rot);
//	ServerCmdAdd("srot", scmd_srot);
	CmdAdd("pos", cmd_pos);
	ServerCmdAdd("spos", scmd_spos);
	CmdAddParam("teleport", cmd_teleport, &pl);
	CmdAddParam("moveorder", cmd_moveorder, &application);
	CmdAddParam("control", cmd_control, &pl);
	CmdAddParam("r_move_path", cmd_cvar<bool, &Player::r_move_path>, &pl);
	CmdAddParam("r_attack_path", cmd_cvar<bool, &Player::r_attack_path>, &pl);
	CmdAddParam("r_overlay", cmd_cvar<bool, &Player::r_overlay>, &pl);
	CvarAdd("fov", &pl.fov, cvar_double);
	CvarAdd("camera_mode_switch_time", &camera_mode_switch_time, cvar_float);
	CvarAdd("g_overlay", &g_overlay, cvar_int);
	CvarAdd("attackorder", &pl.attackorder, cvar_int);
	CvarAdd("forceattackorder", &pl.forceattackorder, cvar_int);
#endif
}

int Player::cmd_mover(int argc, char *argv[], void *pv){
	Game *game = (Game*)pv;
	Player *p = game->player;
	if(argc < 2){
		CmdPrint("usage: mover func");
		CmdPrint("  func - freelook or tactical");
		return 0;
	}
	p->blendmover = 0.;
	if(!strcmp(argv[1], "freelook")){
		p->nextmover = p->freelook;
		CMMover::s.send(CMMover::Freelook);
	}
	else if(!strcmp(argv[1], "tactical")){
		p->nextmover = p->tactical;
		CMMover::s.send(CMMover::Tactical);
	}
	else if(!strcmp(argv[1], "cycle")){
		p->nextmover = p->mover == p->freelook ? p->tactical : p->freelook;
		CMMover::s.send(p->mover == p->freelook ? CMMover::Freelook : CMMover::Tactical);
	}
	else
		CmdPrint("unknown func");
	return 0;
}

int Player::cmd_teleport(int argc, char *argv[], void *pv){
	Player &pl = *(Player*)pv;
	const char *arg = argv[1];
	if(!arg){
		CmdPrint("Specify location you want to teleport to.");
		return 0;
	}
	if(pl.chase){
		return 0;
	}
	{
		int i;
		for(i = 0; i < pl.tplist.size(); i++) if(!strcmp(argv[1], pl.tplist[i].name)){
			pl.cs = pl.tplist[i].cs;
			pl.pos = pl.tplist[i].pos;
			pl.velo.clear();
			break;
		}
		if(i == pl.tplist.size())
			CmdPrint(cpplib::dstring() << "Could not find location \"" << arg << "\".");
	}
	return 0;
}

int Player::cmd_moveorder(int argc, char *argv[], void *pv){
#ifdef _WIN32
	ClientApplication &cl = *(ClientApplication*)pv;
	cl.clientGame->player->moveorder = !cl.clientGame->player->moveorder;
	cl.clientGame->player->move_z = 0.;
#endif
	return 0;
}

#ifdef _WIN32
int Player::cmd_control(int argc, char *argv[], void *pv){
	// TODO: Passing Player as static user pointer is not safe, because the player can be
	// absent.
	Player &pl = *(Player*)pv;
	if(pl.controlled)
		pl.uncontrol();
	else if(!pl.selected.empty()){
		Entity *e = *pl.selected.begin();
		pl.controlled = e;
		pl.mover = pl.nextmover = pl.cockpitview;
		pl.chase = e;
		pl.mover->setrot(quat_u);
		capture_mouse();
		e->controller = &pl;
	}
	return 0;
}
#endif

bool Player::control(Entity *e, double dt){
//	e->control(NULL, dt);
	return false;
}


teleport *Player::findTeleport(const char *name, int flags){
	for(int i = 0; i < tplist.size(); i++) if(!strcmp(tplist[i].name, name) && flags & tplist[i].flags)
		return &tplist[i];
	return NULL;
}

teleport *Player::addTeleport(){
	tplist.push_back(teleport());
	return &tplist.back();
}

Player::teleport_iterator Player::beginTeleport(){
	return 0;
}

teleport *Player::getTeleport(teleport_iterator i){
	return &tplist[i];
}

Player::teleport_iterator Player::endTeleport(){
	return tplist.size();
}

/// \brief The release hook of Entity that clears the weak pointer.
///
/// \param size is always 0?
static SQInteger sqh_release(SQUserPointer p, SQInteger size){
	((WeakPtr<Player>*)p)->~WeakPtr<Player>();
	return 1;
}

void Player::sq_pushobj(HSQUIRRELVM v, Player *e){
	sq_pushroottable(v);
	sq_pushstring(v, _SC("Player"), -1);
	if(SQ_FAILED(sq_get(v, -2)))
		throw SQFError("Something's wrong with Player class definition.");
	if(SQ_FAILED(sq_createinstance(v, -1)))
		throw SQFError("Something's wrong with Player class instance.");
	SQUserPointer p;
	if(SQ_FAILED(sq_getinstanceup(v, -1, &p, NULL)))
		throw SQFError("Something's wrong with Squirrel Class Instace of Player.");
	new(p) WeakPtr<Player>(e);
	sq_setreleasehook(v, -1, sqh_release);
	sq_remove(v, -2); // Remove Class
	sq_remove(v, -2); // Remove root table
}

Player *Player::sq_refobj(HSQUIRRELVM v, SQInteger idx){
	SQUserPointer up;
	// If the instance does not have a user pointer, it's a serious exception that might need some codes fixed.
	if(SQ_FAILED(sq_getinstanceup(v, idx, &up, NULL)) || !up)
		throw SQFError("Something's wrong with Squirrel Class Instace of Player.");
	return *(WeakPtr<Player>*)up;
}

SQInteger Player::sqf_select(HSQUIRRELVM v){
	Player *p = sq_refobj(v);
	if(OT_ARRAY != sq_gettype(v, 2))
		return sq_throwerror(v, _SC("Player::setSelection: Argument must be an array."));
	int n = sq_getsize(v, 2);
	if(n < 0)
		return SQ_ERROR;
	for(int i = 0; i < n; i++){
		sq_pushinteger(v, i);
		sq_get(v, 2);
		Entity *pe = Entity::sq_refobj(v, 3);
		sq_poptop(v);
		if(!pe) // Entities can have been deleted
			continue;
		p->selected.insert(pe);
	}
	return 0;
}

SQInteger Player::sqf_players(HSQUIRRELVM v){
	Game *game = (Game*)sq_getforeignptr(v);
	sq_newarray(v, game->players.size()); // array
	for(int i = 0; i < game->players.size(); i++){
		sq_pushinteger(v, i); // array i
		sq_pushobj(v, game->players[i]); // array i instance
		sq_set(v, -3); // array
	}
	return 1;
}

SQInteger Player::sqf_tostring(HSQUIRRELVM v){
	Player *p = Player::sq_refobj(v);
	if(p){
		sq_pushstring(v, gltestp::dstring() << "{" << p->classname() << ":" << p->getid() << "}", -1);
	}
	else
		return sq_throwerror(v, _SC("Object is deleted"));
	return 1;
}


void Player::sq_define(HSQUIRRELVM v){
	sq_pushstring(v, _SC("Player"), -1);
	sq_newclass(v, SQFalse);
	sq_settypetag(v, -1, SQUserPointer(_SC("Player")));
	sq_setclassudsize(v, -1, sizeof(WeakPtr<Player>));
	register_closure(v, _SC("getpos"), sqf_getintrinsic2<Player, Vec3d, accessorgetter<Player, Vec3d, &Player::getpos>, sq_refobj >);
	register_closure(v, _SC("setpos"), sqf_setintrinsica2<Player, Vec3d, &Player::setpos, sq_refobj>);
	register_closure(v, _SC("getvelo"), sqf_getintrinsic2<Player, Vec3d, accessorgetter<Player, Vec3d, &Player::getvelo>, sq_refobj >, 1, _SC("x"));
	register_closure(v, _SC("setvelo"), sqf_setintrinsica2<Player, Vec3d, &Player::setvelo, sq_refobj>, 2, _SC("xx"));
	register_closure(v, _SC("getrot"), sqf_getintrinsic2<Player, Quatd, accessorgetter<Player, Quatd, &Player::getrot>, sq_refobj >);
	register_closure(v, _SC("setrot"), sqf_setintrinsica2<Player, Quatd, &Player::setrot, sq_refobj>);
	register_closure(v, _SC("getmover"), Player::sqf_getmover);
	register_closure(v, _SC("setmover"), Player::sqf_setmover);
	register_closure(v, _SC("select"), sqf_select);
	register_closure(v, _SC("_get"), &Player::sqf_get);
	register_closure(v, _SC("_set"), &Player::sqf_set);
	register_closure(v, _SC("_tostring"), sqf_tostring);
	sq_createslot(v, -3);

	register_global_func(v, sqf_players, _SC("players"));
}

SQInteger Player::sqf_get(HSQUIRRELVM v){
	try{
		Player *p = sq_refobj(v);
		const SQChar *wcs;
		sq_getstring(v, 2, &wcs);
		if(!strcmp(wcs, _SC("alive"))){
			sq_pushbool(v, !!p);
			return 1;
		}

		// It's not uncommon that a customized Squirrel code accesses a destroyed object.
		if(!p)
			return sq_throwerror(v, _SC("The object being accessed is destructed in the game engine"));

		if(!strcmp(wcs, _SC("cs"))){
			CoordSys::sq_pushobj(v, const_cast<CoordSys*>(p->cs));
			return 1;
		}
		else if(!strcmp(wcs, _SC("selected"))){
			// We cannot foresee how many items in the selection list are really alive.
			// So we just append each item to the array, examining if each one is alive in the way.
			sq_newarray(v, 0); // array

			// Retrieve library-provided "append" method for an array.
			// We'll reuse the method for all the elements, which is not exactly the same way as
			// an ordinally Squirrel codes evaluate.
			sq_pushstring(v, _SC("append"), -1); // array "append"
			if(SQ_FAILED(sq_get(v, -2))) // array array.append
				return sq_throwerror(v, _SC("append not found"));

			// Note the "if"
			for(SelectSet::iterator it = p->selected.begin(); it != p->selected.end(); it++) if(*it){
				sq_push(v, -2); // array array.append array
				Entity::sq_pushobj(v, *it); // array array.append array Entity-instance
				sq_call(v, 2, SQFalse, SQFalse); // array array.append
			}

			// Pop the saved "append" method
			sq_poptop(v); // array

			return 1;
		}
		else if(!strcmp(wcs, _SC("chase"))){
			if(!p->chase){
				sq_pushnull(v);
				return 1;
			}
			Entity::sq_pushobj(v, p->chase);
			return 1;
		}
		else if(!strcmp(wcs, _SC("chasecamera"))){
			sq_pushinteger(v, p->chasecamera);
			return 1;
		}
		else if(!strcmp(wcs, _SC("viewdist"))){
			sq_pushfloat(v, SQFloat(p->viewdist));
			return 1;
		}
		else if(!strcmp(wcs, _SC("playerId"))){
			sq_pushinteger(v, SQInteger(p->playerId));
			return 1;
		}
		else
			return SQ_ERROR;
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.what());
	}
}

SQInteger Player::sqf_set(HSQUIRRELVM v){
	try{
		Player *p = sq_refobj(v);
		const SQChar *wcs;
		sq_getstring(v, 2, &wcs);
		SQRESULT sr;

		// It's not uncommon that a customized Squirrel code accesses a destroyed object.
		if(!p)
			return sq_throwerror(v, _SC("The object being accessed is destructed in the game engine"));

		if(!strcmp(wcs, _SC("cs"))){
			if(OT_INSTANCE != sq_gettype(v, 3))
				return SQ_ERROR;
			p->cs = CoordSys::sq_refobj(v, 3);
			return 1;
		}
		else if(!strcmp(wcs, _SC("chase"))){
			SQObjectType ot = sq_gettype(v, 3);
			if(OT_NULL == ot){
				p->chase = NULL;
				return 1;
			}
			if(OT_INSTANCE != ot)
				return SQ_ERROR;
			Entity *o = Entity::sq_refobj(v, 3);
			if(!o)
				return SQ_ERROR;
			p->chase = o;
			p->chases.insert(p->chase);
			p->chase->addObserver(p);
	//		sq_getinstanceup(v, 3, (SQUserPointer*)&p->chase, NULL);
			return 1;
		}
		else if(!strcmp(wcs, _SC("chasecamera"))){
			SQInteger i;
			if(SQ_SUCCEEDED(sq_getinteger(v, 3, &i))){
				p->setChaseCamera(i);
				return 0;
			}
			else
				return sq_throwerror(v, _SC("The value set to Player::chasecamera must be an integer"));
		}
		else if(!strcmp(wcs, _SC("viewdist"))){
			if(OT_FLOAT != sq_gettype(v, 3))
				return SQ_ERROR;
			SQFloat f;
			sq_getfloat(v, 3, &f);
			p->viewdist = f;
			CMViewDist::s.send(f);
			return 1;
		}
		else
			return SQ_ERROR;
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.what());
	}
}

SQInteger Player::sqf_getpos(HSQUIRRELVM v){
	Player *p = sq_refobj(v);
	SQVec3d sqpos;
	sqpos.value = p->getpos();
	sqpos.push(v);
	return 1;
}

/** \brief Sets mover object to a Player.
 *
 *  To avoid complication about object lifetime management, we do not allow
 * a Squirrel VM to store a "mover" object, but passing or retrieving a string
 * indicating what mover a Player currently has.
 */
SQInteger Player::sqf_setmover(HSQUIRRELVM v){
	Player *p = sq_refobj(v);
	const SQChar *movername;
	if(SQ_FAILED(sq_getstring(v, 2, &movername)))
		return SQ_ERROR;
	SQBool instant;
	if(SQ_FAILED(sq_getbool(v, 3, &instant)))
		instant = true; // defaults true
	CameraController *&mover = instant ? p->mover : p->nextmover;
	if(!instant)
		p->blendmover = 1.;

	if(!strcmp(movername, "freelook"))
		mover = p->freelook;
	else if(!strcmp(movername, "tactical"))
		mover = p->tactical;
	else
		return SQ_ERROR;

	return 0;
}

SQInteger Player::sqf_getmover(HSQUIRRELVM v){
	Player *p = sq_refobj(v);
	if(p->mover == p->freelook)
		sq_pushstring(v, _SC("freelook"), -1);
	else if(p->mover == p->tactical)
		sq_pushstring(v, _SC("tactical"), -1);
	else
		sq_pushstring(v, _SC(""), -1);
	return 1;
}

#ifdef _WIN32
/// A 2-state button that represents whether the player is controlling something.
/// \sa Player::newControlButton
class GLWcontrolButton : public GLWstateButton{
public:
	typedef GLWstateButton st;
	GLWcontrolButton(Game *game, const char *filename, const char *filename2, const char *tips = NULL) : st(game, filename, filename2, tips){}
	virtual bool state()const{
		return game && game->player ? !!game->player->controlled : false;
	}
	/// Issues "control" console command.
	virtual void press(){
		if(game && game->player){
			char *str[1] = {"control"};
			Player::cmd_control(1, str, game->player);
		}
	}
};

/// \param game The Game object that is bound to the newly created button.
/// \param filename File name of button image when being active
/// \param filename2 File name of button image when being inactive
/// \param tips Displayed when mouse cursor is floating over.
GLWstateButton *Player::newControlButton(Game *game, const char *filename, const char *filename2, const char *tips){
	return new GLWcontrolButton(game, filename, filename2, tips);
};

/// \param game The Game object that is bound to the newly created button.
/// \param filename File name of button image when being active
/// \param filename2 File name of button image when being inactive
/// \param tips Displayed when mouse cursor is floating over.
GLWstateButton *Player::newMoveOrderButton(Game *game, const char *filename, const char *filename2, const char *tips){
	/// Command
	class GLWmoveOrderButton : public GLWstateButton{
	public:
		WeakPtr<Player> player;
		GLWmoveOrderButton(Game *game, const char *filename, const char *filename1, const char *tip = NULL) :
			GLWstateButton(game, filename, filename1, tip){}
		virtual bool state()const{
			if(!game || !game->player)
				return false;
			return game->player->moveorder;
		}
		virtual void press(){
			if(!game || !game->player)
				return;
			// The client has only one global application.
			Player::cmd_moveorder(0, NULL, &application);
		}
	};
	return new GLWmoveOrderButton(game, filename, filename2, tips);
}
#endif





//-----------------------------------------------------------------------------
//  CMMover implementation
//-----------------------------------------------------------------------------

CMMover CMMover::s;

void CMMover::send(Type t){
	std::stringstream ss;
	StdSerializeStream sss(ss);
	sss << t;
	std::string str = ss.str();
	s.st::send(application, str.c_str(), str.size());
}

/// \brief A server command that accepts messages from the client to change the direction of sight.
void CMMover::interpret(ServerClient &sc, UnserializeStream &uss){
	Type t;
	uss >> (int&)t;
	Player *player;
#ifndef _WIN32
	player = sc.sv->pg->players[sc.id];
#else
	player = application.serverGame->players[sc.id];
#endif
	if(player){
		player->blendmover = 0.;
		if(t == Freelook)
			player->nextmover = player->freelook;
		else if(t == Tactical)
			player->nextmover = player->tactical;
	}
}


//-----------------------------------------------------------------------------
//  CMChase implementation
//-----------------------------------------------------------------------------

CMChase CMChase::s;

void CMChase::send(Entity *e){
	std::stringstream ss;
	StdSerializeStream sss(ss);
	sss << e;
	std::string str = ss.str();
	s.st::send(application, str.c_str(), str.size());
}

/// \brief A server command that accepts messages from the client to change the direction of sight.
void CMChase::interpret(ServerClient &sc, UnserializeStream &uss){
	Entity *e;
	uss >> e;
	Player *player;
#ifndef _WIN32
	player = sc.sv->pg->players[sc.id];
#else
	player = application.serverGame->players[sc.id];
#endif
	if(player){
		player->chases.clear();
		player->chase = e;
		player->chases.insert(e);
	}
}

//-----------------------------------------------------------------------------
//  CMViewDist implementation
//-----------------------------------------------------------------------------

CMViewDist CMViewDist::s;

void CMViewDist::send(double viewdist){
	std::stringstream ss;
	StdSerializeStream sss(ss);
	sss << viewdist;
	std::string str = ss.str();
	s.st::send(application, str.c_str(), str.size());
}

/// \brief A server command that accepts messages from the client to change the direction of sight.
void CMViewDist::interpret(ServerClient &sc, UnserializeStream &uss){
	double viewdist;
	uss >> viewdist;
	Player *player;
#ifndef _WIN32
	player = sc.sv->pg->players[sc.id];
#else
	player = application.serverGame->players[sc.id];
#endif
	if(player){
		player->viewdist = viewdist;
	}
}

