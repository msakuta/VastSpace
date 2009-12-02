#include "player.h"
#include "entity.h"
#include "cmd.h"
#include "coordsys.h"


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
