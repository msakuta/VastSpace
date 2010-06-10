#include "player.h"
#include "Universe.h"
#include "entity.h"
#include "cmd.h"
#include "coordsys.h"
#include "astro.h"
#include "serial_util.h"
#include "stellar_file.h"
#include "sqadapt.h"
#include "btadapt.h"
extern "C"{
#include <clib/mathdef.h>
#include <clib/cfloat.h>
}

float Player::camera_mode_switch_time = 1.f;
int Player::g_overlay = 1;

static teleport *tplist;
static int ntplist;

static long framecount = 0;

void Player::mover_t::operator ()(const input_t &inputs, double dt){}

Vec3d Player::mover_t::getpos()const{
	return pl.pos;
}

Quatd Player::mover_t::getrot()const{
	return pl.rot;
}

// Response to rotational movement depends on mover.
void Player::mover_t::rotateLook(double dx, double dy){
	Quatd &rot = pl.rot;
	const double speed = .001 / 2. * pl.fov;
	const double one_minus_epsilon = .999;

	rot = rot.quatrotquat(Vec3d(0, dx * speed, 0));
	rot = rot.quatrotquat(Vec3d(dy * speed, 0, 0));
}


class CockpitviewMover : public Player::mover_t{
public:
	typedef Player::mover_t st;
	CockpitviewMover(Player &a) : st(a){}
/*	virtual Quatd getrot()const{
		if(!pl.chase)
			return pl.rot;
		Vec3d dummy;
		Quatd crot;
		pl.chase->cockpitView(dummy, crot, pl.chasecamera);
		return pl.rot * crot.cnj();
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


class TacticalMover : public Player::mover_t{
public:
	typedef Player::mover_t st;
	Vec3d pos;
	Quatd rot;
	TacticalMover(Player &a) : st(a), rot(0,0,0,1){}
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
	rot = Quatd(0, 0, sin(roll/2), cos(roll/2)) * Quatd(sin(theta/2), 0, 0, cos(theta/2)) * Quatd(0, sin(phi/2), 0, cos(phi/2));
}

Player::Player() : pos(Vec3d(0,0,0)), velo(Vec3d(0,0,0)), rot(quat_u), fov(1.), chasecamera(0), viewdist(1.),
	nextmover(NULL), blendmover(0), moveorder(false), move_lockz(false), move_z(0.), move_org(Vec3d(0,0,0)), move_hitpos(Vec3d(0,0,0)),
	freelook(new FreelookMover(*this)),
	cockpitview(new CockpitviewMover(*this)), tactical(new TacticalMover(*this))
{
	mover = freelook;
}

Player::~Player(){
	free();
}

void Player::free(){
	if(tplist){
		for(int i = 0; i < ntplist; i++)
			tplist[i].teleport::~teleport();
		::free(tplist);
	}
	tplist = NULL;
	ntplist = 0;
}


inline Quatd Player::getrawrot(mover_t *mover)const{
	return mover->getrot();
}

inline Vec3d Player::getrawpos(mover_t *mover)const{
	return mover->getpos();
}

Quatd Player::getrot()const{
	if(nextmover && mover != nextmover){
/*		btQuaternion q1 = btqc(getrawrot(mover)), q2 = btqc(getrawrot(nextmover));
		Quatd ret = btqc(q1.slerp(q2, blendmover).normalize());
		if(framecount % 2)
			return ret;
		else*/
			return Quatd::slerp(getrawrot(mover), getrawrot(nextmover), blendmover /* (sqrt(fabs(blendmover - .5) / .5) * signof(blendmover - .5) + 1.) / 2.*/).normin();
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
	sc.o << chase << selected << pos << velo << rot << cs;
	sc.o << ntplist;
	for(int i = 0; i < ntplist; i++)
		tplist[i].serialize(sc);
}

void Player::unserialize(UnserializeContext &sc){
	sc.i >> chase >> selected >> pos >> velo >> rot >> cs;
	sc.i >> ntplist;
	tplist = (teleport*)::malloc(ntplist * sizeof *tplist);
	for(int i = 0; i < ntplist; i++)
		tplist[i].unserialize(sc);
}

void Player::anim(double dt){
	framecount++;
	aviewdist += (viewdist - aviewdist) * (1. - exp(-10. * dt));
	const Universe *u = cs->findcspath("/")->toUniverse();
	if(u && !u->paused && nextmover && mover != nextmover){
		// We do not want the camera to teleport when setting camera_mode_switch_time while transiting.
		blendmover += dt / camera_mode_switch_time;
		if(1. < blendmover)
			mover = nextmover;
	}
}

void Player::transit_cs(CoordSys *cs){
	// If chasing in free-look mode, rotation is converted by the Entity being chased,
	// so we do not need to convert here.
	if(!chase || mover == tactical){
		Quatd rot = cs->tocsq(this->cs);
		this->rot *= rot;
		this->velo = rot.cnj().trans(this->velo);
	}
	if(!chase)
		this->pos = cs->tocs(this->pos, this->cs);
	this->cs = cs;
}

void Player::unlink(const Entity *pe){
	chases.erase(pe);
	if(chase == pe)
		chase = chases.empty() ? NULL : const_cast<Entity*>(*chases.begin());
	if(control == pe)
		control = NULL;
	if(lastchase == pe)
		lastchase = NULL;
	Entity **ppe;
	for(ppe = &selected; *ppe;) if(*ppe == pe){
		*ppe = pe->selectnext;
		break;
	}
	else
		ppe = &(*ppe)->selectnext;
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

	Entity *&chase = pl.chase;
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
		const CoordSys *cs = pl.cs->belongcs(pos, pl.pos);
		if(cs != pl.cs){
			Quatd rot = cs->tocsq(pl.cs);
			pl.rot *= rot;
			pl.velo = rot.cnj().trans(pl.velo);
			pl.cs = cs;
			pl.pos = pos;
		}
	}
}

void TacticalMover::operator()(const input_t &inputs, double dt){
//	Vec3d &pos = pl.pos;
	Vec3d &velo = refvelo();
//	Quatd &rot = pl.rot;
	Vec3d &cpos = pl.cpos;
	std::set<const Entity*> &chases = pl.chases;
	Entity *&chase = pl.chase;
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
		rot = Quatd(0, 0, sin(roll/2), cos(roll/2)) * rot1;
	}
	if(!chases.empty()){
		int n = 0;
		for(std::set<const Entity*>::iterator it = chases.begin(); it != chases.end(); it++){
			cpos = (cpos * n + (*it)->pos) / (n + 1);
			n++;
		}
	}
	else if(chase && chase->w->cs == pl.cs){
		cpos = chase->pos;
	}
	pos = cpos + view * pl.aviewdist;
}

void Player::cmdInit(Player &pl){
	CmdAddParam("mover", cmd_mover, &pl);
	CmdAddParam("teleport", cmd_teleport, &pl);
	CmdAddParam("moveorder", cmd_moveorder, &pl);
	CvarAdd("camera_mode_switch_time", &camera_mode_switch_time, cvar_float);
	CvarAdd("g_overlay", &g_overlay, cvar_int);
}

int Player::cmd_mover(int argc, char *argv[], void *pv){
	Player *p = (Player*)pv;
	if(argc < 2){
		CmdPrint("usage: mover func");
		CmdPrint("  func - freelook or tactical");
		return 0;
	}
	p->blendmover = 0.;
	if(!strcmp(argv[1], "freelook"))
		p->nextmover = p->freelook;
	else if(!strcmp(argv[1], "tactical"))
		p->nextmover = p->tactical;
	else if(!strcmp(argv[1], "cycle"))
		p->nextmover = p->mover == p->freelook ? p->tactical : p->freelook;
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
		for(i = 0; i < ntplist; i++) if(!strcmp(argv[1], tplist[i].name)){
			pl.cs = tplist[i].cs;
			pl.pos = tplist[i].pos;
			pl.velo.clear();
			break;
		}
		if(i == ntplist)
			CmdPrintf("Could not find location \"%s\".", arg);
	}
	return 0;
}

int Player::cmd_moveorder(int argc, char *argv[], void *pv){
	Player &pl = *(Player*)pv;
	pl.moveorder = !pl.moveorder;
	pl.move_z = 0.;
	return 0;
}

teleport *Player::findTeleport(const char *name, int flags){
	for(int i = 0; i < ntplist; i++) if(!strcmp(tplist[i].name, name) && flags & tplist[i].flags)
		return &tplist[i];
	return NULL;
}

teleport *Player::addTeleport(){
	tplist = (teleport*)realloc(tplist, ++ntplist * sizeof *tplist);
	return &tplist[ntplist-1];
}

Player::teleport_iterator Player::beginTeleport(){
	return 0;
}

teleport *Player::getTeleport(teleport_iterator i){
	return &tplist[i];
}

Player::teleport_iterator Player::endTeleport(){
	return ntplist;
}

SQInteger Player::sqf_get(HSQUIRRELVM v){
	Player *p;
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	SQRESULT sr;
	if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
		return sr;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!strcmp(wcs, _SC("cs"))){
		sq_pushroottable(v);
		sq_pushstring(v, _SC("CoordSys"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		if(!sqa_newobj(v, const_cast<CoordSys*>(p->cs)))
			return sq_throwerror(v, _SC("no cs"));
		return 1;
	}
	else if(!strcmp(wcs, _SC("selected"))){
		if(!p->selected){
			sq_pushnull(v);
			return 1;
		}
		sq_pushroottable(v);
		sq_pushstring(v, _SC("Entity"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		if(!sqa_newobj(v, p->selected))
			return sq_throwerror(v, _SC("no selected"));
		return 1;
	}
	else if(!strcmp(wcs, _SC("chase"))){
		if(!p->chase){
			sq_pushnull(v);
			return 1;
		}
		sq_pushroottable(v);
		sq_pushstring(v, _SC("Entity"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		if(!sqa_newobj(v, p->chase)){
			return sq_throwerror(v, _SC("no ent"));
		}
//		sq_setinstanceup(v, -1, p->chase);
		return 1;
	}
	else
		return sqa::sqf_get<Player>(v);
}

SQInteger Player::sqf_set(HSQUIRRELVM v){
	Player *p;
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	SQRESULT sr;
	if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
		return sr;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!strcmp(wcs, _SC("cs"))){
		if(OT_INSTANCE != sq_gettype(v, 3))
			return SQ_ERROR;
		if(!sqa_refobj(v, (SQUserPointer*)&p->cs, &sr, 3))
			return sr;
		return 1;
	}
	else if(!strcmp(wcs, _SC("chase"))){
		if(OT_INSTANCE != sq_gettype(v, 3))
			return SQ_ERROR;
		if(!sqa_refobj(v, (SQUserPointer*)&p->chase, &sr, 3))
			return sr;
//		sq_getinstanceup(v, 3, (SQUserPointer*)&p->chase, NULL);
		return 1;
	}
	else
		return sqa::sqf_set<Player>(v);
}

SQInteger Player::sqf_getpos(HSQUIRRELVM v){
	const SQChar *wcs;
	SQRESULT sr;
	Player *p;
	if(SQ_FAILED(sqa_refobj(v, (SQUserPointer*)p, &sr)))
		return sr;
	SQVec3d sqpos;
	sqpos.value = p->getpos();
	sqpos.push(v);
	return 1;
}
