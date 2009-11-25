#include "entity.h"
extern "C"{
#include <clib/aquat.h>
}
#include "beamer.h"
#include "judge.h"
#include "player.h"

Quatd Player::getrot()const{
	if(!chase)
		return rot;
	return rot * chase->rot.cnj();
}

Vec3d Player::getpos()const{
	if(!chase)
		return pos;
	return pos + chase->rot.trans(Vec3d(.0, .05, .15));
}

template<class T> Entity *Constructor(){
	return new T();
};

static const char *ent_name[] = {
	"beamer",
};
static Entity *(*const ent_creator[])() = {
	Constructor<Beamer>,
};

Entity *Entity::create(const char *cname){
	int i;
	for(i = 0; i < numof(ent_name); i++) if(!strcmp(ent_name[i], cname)){
		Entity *pt;
		pt = ent_creator[i]();
		return pt;
	}
	return NULL;
}

const char *Entity::idname()const{
	return "entity";
}

const char *Entity::classname()const{
	return "Entity";
}

void Entity::anim(double){}
void Entity::postframe(){}

bool Entity::tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *fret, Vec3d *retp, Vec3d *retnormal){
	Vec3d retpos;
	bool bret = !!jHitSpherePos(pos, this->hitradius() + rad, start, dir, 1., fret, &retpos);
	if(bret && retp)
		*retp = retpos;
	if(bret && retnormal)
		*retnormal = (retpos - pos).normin();
	return bret;
//	(hitpart = 0, 0. < (sdist = -VECSP(delta, &mat[8]) - rad))) && sdist < best
}

int Entity::takedamage(double damage, int hitpart){
	if(health <= damage){
		health = 0;
		w = NULL;
		return 0;
	}
	health -= damage;
	return 1;
}

int Entity::popupMenu(char ***const titles, int **keys, char ***cmds, int *num){
	return 0;
}

Warpable *Entity::toWarpable(){
	return NULL;
}

void Entity::transit_cs(CoordSys *cs){
	Vec3d pos;
	Mat4d mat;
	if(w == cs->w)
		return;
	this->pos = cs->tocs(this->pos, w->cs);
	velo = cs->tocsv(velo, pos, w->cs);
	if(!cs->w){
		cs->w = new WarField(cs);
	}
	w = cs->w;
	{
		Quatd q, q1;
		q1 = w->cs->tocsq(cs);
		q = q1 * this->rot;
		this->rot = q;
	}
	if(w->pl->chase == this){
		w->pl->pos = cs->tocs(w->pl->pos, w->pl->cs);
		w->pl->cs = cs;
	}
}
