#include "player.h"
#include "Universe.h"
#include "entity.h"
#include "cmd.h"
#include "coordsys.h"
#include "astro.h"
#include "serial_util.h"
#include "stellar_file.h"
extern "C"{
#include <clib/mathdef.h>
#include <clib/cfloat.h>
}

static teleport *tplist;
static int ntplist;

Player::Player() : pos(Vec3d(0,0,0)), velo(Vec3d(0,0,0)), rot(quat_u), fov(1.), chasecamera(0), flypower(1.), viewdist(1.),
	mover(&Player::freelook), moveorder(false), move_lockz(false), move_z(0.), move_org(Vec3d(0,0,0)), move_hitpos(Vec3d(0,0,0))
{
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


Quatd Player::getrot()const{
	if(!chase || mover == &Player::tactical)
		return rot;
	Vec3d dummy;
	Quatd crot;
	chase->cockpitView(dummy, crot, chasecamera);
	return rot * crot.cnj();
}

Vec3d Player::getpos()const{
	return pos;
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
	aviewdist += (viewdist - aviewdist) * (1. - exp(-10. * dt));
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
	const double speed = .001 / 2. * fov;
	const double one_minus_epsilon = .999;

	// Response to rotational movement depends on mover.
	if(mover == &Player::freelook){
		rot = rot.quatrotquat(Vec3d(0, dx * speed, 0));
		rot = rot.quatrotquat(Vec3d(dy * speed, 0, 0));
	}
	else{
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
}

void Player::freelook(const input_t &inputs, double dt){
	double accel = flypower;
	int inputstate = inputs.press;
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

	if(chase){
		WarSpace *ws = (WarSpace*)chase->w;
		if(ws)
			this->cs = ws->cs;
		Quatd dummy;
		chase->cockpitView(pos, dummy, chasecamera);
		velo = chase->velo;
	}
	else{
		velolen = velo.len();
		pos += velo * (1. + velolen) * dt;
		Vec3d pos;
		const CoordSys *cs = this->cs->belongcs(pos, this->pos);
		if(cs != this->cs){
			Quatd rot = cs->tocsq(this->cs);
			this->rot *= rot;
			this->velo = rot.cnj().trans(this->velo);
			this->cs = cs;
			this->pos = pos;
		}
	}
}

void Player::tactical(const input_t &inputs, double dt){
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
	else if(chase && chase->w->cs == cs){
		cpos = chase->pos;
	}
	pos = cpos + view * aviewdist;
}

int Player::cmd_mover(int argc, char *argv[], void *pv){
	Player *p = (Player*)pv;
	if(argc < 2){
		CmdPrint("usage: mover func");
		CmdPrint("  func - freelook or tactical");
		return 0;
	}
	if(!strcmp(argv[1], "freelook"))
		p->mover = &Player::freelook;
	else if(!strcmp(argv[1], "tactical"))
		p->mover = &Player::tactical;
	else if(!strcmp(argv[1], "cycle"))
		p->mover = p->mover == &Player::freelook ? &Player::tactical : &Player::freelook;
	else
		CmdPrint("unknown func");
	return 0;
}

int Player::cmd_coordsys(int argc, char *argv[], void *pv){
	Player *p = (Player*)pv;
	const Universe *univ = p->cs->findcspath("/")->toUniverse();
	char buf[128];
	if(argc <= 1){
		buf[0] = '\0';
		CmdPrintf("identity: %s", p->cs->name);
		CmdPrintf("path: %s", p->cs->getpath());
		CmdPrintf("formal name: %s", p->cs->fullname ? p->cs->fullname : p->cs->name);
	}
	else{
		const CoordSys *cs;
		if(argv[1][0] == '/')
			cs = univ->findcspath(&argv[1][1]);
		else if(!(cs = p->cs->findcspath(argv[1])))
			cs = univ->findcs(argv[1]);
		if(cs && cs != p->cs){
			Quatd rot = cs->tocsq(p->cs);
			Vec3d pos = cs->tocs(p->pos, p->cs);
			p->rot *= rot;
			p->velo = rot.cnj().trans(p->velo);
			p->pos = pos;
			p->cs = cs;
		}
		else{
			CmdPrint("Specified coordinate system is not existing or currently selected");
		}
	}
	return 0;
}

int Player::cmd_position(int argc, char *argv[], void *pv){
	Player &pl = *(Player*)pv;
	const char *arg = argv[1];
	if(!arg){
		char buf[128];
		sprintf(buf, "@echo (%lg,%lg,%lg)", pl.pos[0], pl.pos[1], pl.pos[2]);
		CmdExec(buf);
	}
	else{
		char args[128], *p;
		strncpy(args, arg, sizeof args);
		if(p = strtok(args, " \t,"))
			pl.pos[0] = atof(p);
		if(p = strtok(NULL, " \t,"))
			pl.pos[1] = atof(p);
		if(p = strtok(NULL, " \t,"))
			pl.pos[2] = atof(p);
	}
	return 0;
}

int Player::cmd_velocity(int argc, char *argv[], void *pv){
	Player &pl = *(Player*)pv;
	const char *arg = argv[1];
	if(!arg){
		char buf[128];
		sprintf(buf, "@echo (%lg,%lg,%lg)", pl.velo[0], pl.velo[1], pl.velo[2]);
		CmdExec(buf);
	}
	else{
		char args[128], *p;
		strncpy(args, arg, sizeof args);
		if(p = strtok(args, " \t,"))
			pl.velo[0] = atof(p);
		if(p = strtok(NULL, " \t,"))
			pl.velo[1] = atof(p);
		if(p = strtok(NULL, " \t,"))
			pl.velo[2] = atof(p);
	}
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
