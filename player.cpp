#include "player.h"
#include "entity.h"
#include "cmd.h"
#include "coordsys.h"
#include "astro.h"
#include "serial_util.h"

Player::Player() : pos(Vec3d(0,0,0)), velo(Vec3d(0,0,0)), rot(quat_u), fov(1.), flypower(1.), viewdist(1.), mover(&Player::freelook){
}


Quatd Player::getrot()const{
	if(!chase || mover == &Player::tactical)
		return rot;
	Vec3d dummy;
	Quatd crot;
	chase->cockpitView(dummy, crot, 0);
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
	sc.o << " " << pos << " " << velo << " " << rot << " " << sc.map[cs];
}

void Player::unserialize(UnserializeContext &sc){
	unsigned cs;
	sc.i >> " " >> pos >> " " >> velo >> " " >> rot >> " " >> cs;
	this->cs = static_cast<CoordSys*>(sc.map[cs]);
}

void Player::anim(double dt){
	aviewdist += (viewdist - aviewdist) * (1. - exp(-10. * dt));
}

void Player::unlink(const Entity *pe){
	if(chase == pe)
		chase = NULL;
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
		this->cs = chase->w->cs;
		Quatd dummy;
		chase->cockpitView(pos, dummy, 0);
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
	rot = Quatd(sin(theta/2), 0, 0, cos(theta/2)) * Quatd(0, sin(phi/2), 0, cos(phi/2));
	if(chase && chase->w->cs == cs){
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
