#include "Player.h"
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
extern "C"{
#include <clib/mathdef.h>
#include <clib/cfloat.h>
}

float Player::camera_mode_switch_time = 1.f;
int Player::g_overlay = 1;

static teleport *tplist;
static int ntplist;

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
		pl.chase->cockpitView(dummy, crot, pl.chasecamera);
		return /*pyrot */ crot.cnj();
	}
	virtual Vec3d getpos()const{
		if(pl.chase){
			Vec3d pos;
			Quatd dummy;
			pl.chase->cockpitView(pos, dummy, pl.chasecamera);
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

Player::Player() : pos(Vec3d(0,0,0)), velo(Vec3d(0,0,0)), accel(0,0,0), rot(quat_u), rad(0.001), lastchase(NULL),
	chasecamera(0), detail(0), mousex(0), mousey(0), race(0), fov(1.), viewdist(1.),
	gametime(0), velolen(0),
	nextmover(NULL), blendmover(0), attackorder(0), forceattackorder(0),
	cpos(0,0,0),
	r_move_path(false), r_attack_path(false), r_overlay(false),
	selected(NULL), controlled(NULL), moveorder(false), move_lockz(false), move_z(0.), move_org(Vec3d(0,0,0)), move_hitpos(Vec3d(0,0,0)),
	freelook(NULL), cockpitview(NULL), tactical(NULL)
{
	// Special case that needs const member variables to be initialized outside initialization list.
	// We do not want to alter these pointers outside the constructor, but Player class is not yet
	// initialized at the time initialization list is executed.
	// Class object members need not to warry about read-only memory storage class.
	const_cast<FreelookMover*>(freelook) = new FreelookMover(*this);
	const_cast<CameraController*>(cockpitview) = new CockpitviewMover(*this);
	const_cast<CameraController*>(tactical) = new TacticalMover(*this);
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
	sc.o << chase;
	sc.o << selected;
	sc.o << pos << velo << accel << rot;
	sc.o << fov;
	sc.o << cs;
	sc.o << ntplist;
	for(int i = 0; i < ntplist; i++)
		tplist[i].serialize(sc);
}

void Player::unserialize(UnserializeContext &sc){
	sc.i >> chase;
	sc.i >> selected;
	sc.i >> pos >> velo >> accel >> rot;
	sc.i >> fov;
	sc.i >> cs;
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

void Player::unlink(const Entity *pe){
	chases.erase(pe);
	if(chase == pe)
		chase = chases.empty() ? NULL : const_cast<Entity*>(*chases.begin());
	if(controlled == pe)
		controlled = NULL;
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
	std::set<const Entity*> &chases = pl.chases;
	Entity *&chase = pl.chase;

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
		for(std::set<const Entity*>::iterator it = chases.begin(); it != chases.end(); it++){
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

void Player::cmdInit(Player &pl){
	CmdAddParam("mover", cmd_mover, &pl);
	CmdAddParam("teleport", cmd_teleport, &pl);
	CmdAddParam("moveorder", cmd_moveorder, &pl);
	CmdAddParam("control", cmd_control, &pl);
	CmdAddParam("r_move_path", cmd_cvar<bool, &Player::r_move_path>, &pl);
	CmdAddParam("r_attack_path", cmd_cvar<bool, &Player::r_attack_path>, &pl);
	CmdAddParam("r_overlay", cmd_cvar<bool, &Player::r_overlay>, &pl);
	CvarAdd("fov", &pl.fov, cvar_double);
	CvarAdd("camera_mode_switch_time", &camera_mode_switch_time, cvar_float);
	CvarAdd("g_overlay", &g_overlay, cvar_int);
	CvarAdd("attackorder", &pl.attackorder, cvar_int);
	CvarAdd("forceattackorder", &pl.forceattackorder, cvar_int);
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
			CmdPrint(cpplib::dstring() << "Could not find location \"" << arg << "\".");
	}
	return 0;
}

int Player::cmd_moveorder(int argc, char *argv[], void *pv){
	Player &pl = *(Player*)pv;
	pl.moveorder = !pl.moveorder;
	pl.move_z = 0.;
	return 0;
}

int Player::cmd_control(int argc, char *argv[], void *pv){
	Player &pl = *(Player*)pv;
	if(pl.controlled)
		pl.uncontrol();
	else if(pl.selected){
		pl.controlled = pl.selected;
		pl.mover = pl.nextmover = pl.cockpitview;
		pl.chase = pl.selected;
		pl.mover->setrot(quat_u);
		capture_mouse();
		pl.selected->controller = &pl;
	}
	return 0;
}

bool Player::control(Entity *e, double dt){
//	e->control(NULL, dt);
	return false;
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

static SQInteger sqf_select(HSQUIRRELVM v){
	Player *p;
	SQRESULT sr;
	if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
		return sr;
	if(OT_ARRAY != sq_gettype(v, 2))
		return sq_throwerror(v, _SC("Player::setSelection: Argument must be an array."));
	int n = sq_getsize(v, 2);
	if(n < 0)
		return SQ_ERROR;
	Entity **pnext = &p->selected;
	for(int i = 0; i < n; i++){
		Entity *pe;
		sq_pushinteger(v, i);
		sq_get(v, 2);
		bool ret = sqa_refobj(v, (SQUserPointer*)&pe, &sr, 3);
		sq_poptop(v);
		if(!ret) // Entities can have been deleted
			continue;
		*pnext = pe;
		pnext = &pe->selectnext;
	}
	*pnext = NULL;
	return 0;
}

void Player::sq_define(HSQUIRRELVM v){
/*	SqPlus::SQClassDef<Player>(_SC("Player")).
		func(&Player::classname, _SC("classname")).
		func(&Player::getcs, _SC("cs"));*/
	sq_pushstring(v, _SC("Player"), -1);
	sq_newclass(v, SQFalse);
	sq_pushstring(v, _SC("ref"), -1);
	sq_pushnull(v);
	sq_newslot(v, -3, SQFalse);
	register_closure(v, _SC("getpos"), sqf_getintrinsic<Player, Vec3d, accessorgetter<Player, Vec3d, &Player::getpos> >);
	register_closure(v, _SC("setpos"), sqf_setintrinsica<Player, Vec3d, &Player::setpos>);
	register_closure(v, _SC("getvelo"), sqf_getintrinsic<Player, Vec3d, accessorgetter<Player, Vec3d, &Player::getvelo> >, 1, _SC("x"));
	register_closure(v, _SC("setvelo"), sqf_setintrinsica<Player, Vec3d, &Player::setvelo>, 2, _SC("xx"));
	register_closure(v, _SC("getrot"), sqf_getintrinsic<Player, Quatd, accessorgetter<Player, Quatd, &Player::getrot> >);
	register_closure(v, _SC("setrot"), sqf_setintrinsica<Player, Quatd, &Player::setrot>);
	register_closure(v, _SC("getmover"), Player::sqf_getmover);
	register_closure(v, _SC("setmover"), Player::sqf_setmover);
	register_closure(v, _SC("select"), sqf_select);
	register_closure(v, _SC("_get"), &Player::sqf_get);
	register_closure(v, _SC("_set"), &Player::sqf_set);
	sq_createslot(v, -3);
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
		sq_pushstring(v, p->cs->getStatic().s_sqclassname, -1);
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
	else if(!strcmp(wcs, _SC("viewdist"))){
		sq_pushfloat(v, SQFloat(p->viewdist));
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
		SQObjectType ot = sq_gettype(v, 3);
		if(OT_NULL == ot){
			p->chase = NULL;
			return 1;
		}
		if(OT_INSTANCE != ot)
			return SQ_ERROR;
		if(!sqa_refobj(v, (SQUserPointer*)&p->chase, &sr, 3))
			return sr;
//		sq_getinstanceup(v, 3, (SQUserPointer*)&p->chase, NULL);
		return 1;
	}
	else if(!strcmp(wcs, _SC("viewdist"))){
		if(OT_FLOAT != sq_gettype(v, 3))
			return SQ_ERROR;
		SQFloat f;
		sq_getfloat(v, 3, &f);
		p->viewdist = f;
		return 1;
	}
	else
		return sqa::sqf_set<Player>(v);
}

SQInteger Player::sqf_getpos(HSQUIRRELVM v){
	SQRESULT sr;
	Player *p;
	if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
		return sr;
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
	SQRESULT sr;
	Player *p;
	if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
		return sr;
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
	SQRESULT sr;
	Player *p;
	if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
		return sr;
	if(p->mover == p->freelook)
		sq_pushstring(v, _SC("freelook"), -1);
	else if(p->mover == p->tactical)
		sq_pushstring(v, _SC("tactical"), -1);
	else
		sq_pushstring(v, _SC(""), -1);
	return 1;
}

/// A 2-state button that represents whether the player is controlling something.
/// \sa Player::newControlButton
class GLWcontrolButton : public GLWstateButton{
	Player &pl;
public:
	typedef GLWstateButton st;
	GLWcontrolButton(Player &apl, const char *filename, const char *filename2, const char *tips = NULL) : st(filename, filename2, tips), pl(apl){}
	virtual bool state()const{return !!pl.controlled;}
	/// Issues "control" console command.
	virtual void press(){
		char *str[1] = {"control"};
		Player::cmd_control(1, str, &pl);
	}
};

/// \param pl The Player object that is bound to the newly created button.
/// \param filename File name of button image when being active
/// \param filename2 File name of button image when being inactive
/// \param tips Displayed when mouse cursor is floating over.
GLWstateButton *Player::newControlButton(Player &pl, const char *filename, const char *filename2, const char *tips){
	return new GLWcontrolButton(pl, filename, filename2, tips);
};

/// \param pl The Player object that is bound to the newly created button.
/// \param filename File name of button image when being active
/// \param filename2 File name of button image when being inactive
/// \param tips Displayed when mouse cursor is floating over.
GLWstateButton *Player::newMoveOrderButton(Player &pl, const char *filename, const char *filename2, const char *tips){
	/// Command
	class GLWmoveOrderButton : public GLWstateButton{
	public:
		Player *pl;
		GLWmoveOrderButton(const char *filename, const char *filename1, Player *apl, const char *tip = NULL) :
			GLWstateButton(filename, filename1, tip), pl(apl){}
		virtual bool state()const{
			return pl->moveorder;
		}
		virtual void press(){
			pl->cmd_moveorder(0, NULL, pl);
		}
	};
	return new GLWmoveOrderButton(filename, filename2, &pl, tips);
}
