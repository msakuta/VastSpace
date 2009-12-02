#include "entity.h"
extern "C"{
#include <clib/aquat.h>
}
#include "beamer.h"
#include "judge.h"
#include "player.h"
#include "cmd.h"


Entity::Entity(WarField *aw) : pos(vec3_000), velo(vec3_000), omg(vec3_000), rot(quat_u), mass(1e3), moi(1e1), enemy(NULL), w(aw), inputs(), health(1){
	if(aw)
		aw->addent(this);
}

void Entity::init(){
	health = maxhealth();
}

template<class T> Entity *Constructor(WarField *w){
	return new T(w);
};

static const char *ent_name[] = {
	"beamer", "assault",
};
static Entity *(*const ent_creator[])(WarField *w) = {
	Constructor<Beamer>, Constructor<Assault>,
};

Entity *Entity::create(const char *cname, WarField *w){
	int i;
	for(i = 0; i < numof(ent_name); i++) if(!strcmp(ent_name[i], cname)){
		Entity *pt;
		pt = ent_creator[i](w);
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

double Entity::maxhealth()const{return 100.;}
void Entity::anim(double){}
void Entity::postframe(){}
void Entity::control(const input_t *i, double){inputs = *i;}
unsigned Entity::analog_mask(){return 0;}
void Entity::cockpitView(Vec3d &pos, Quatd &rot, int)const{pos = this->pos; rot = this->rot;}
int Entity::numCockpits()const{return 1;}
void Entity::draw(wardraw_t *){}
void Entity::drawtra(wardraw_t *){}
void Entity::bullethit(const Bullet *){}
Entity *Entity::getOwner(){return NULL;}
int Entity::armsCount()const{return 0;}
const ArmBase *Entity::armsGet(int)const{return NULL;}

int Entity::tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *fret, Vec3d *retp, Vec3d *retnormal){
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

/* estimate target position based on distance and approaching speed */
int estimate_pos(Vec3d &ret, const Vec3d &pos, const Vec3d &velo, const Vec3d &srcpos, const Vec3d &srcvelo, double speed, const WarField *w){
	double dist;
	Vec3d grv;
	double posy;
	double h = 0.;
	double h0 = 0.;
	if(w){
		Vec3d mid = (pos + srcpos) * .5;
		Vec3d evelo = (pos - srcpos).norm() * -speed;
		grv = w->accel(mid, evelo);
	}
	else
		VECNULL(grv);
	dist = (pos - srcpos).len();
	posy = pos[1] + (velo[1] + grv[1]) * dist / speed;
/*	(*ret)[0] = pos[0] + (velo[0] - srcvelo[0] + grv[0] * dist / speed / 2.) * dist / speed;
	(*ret)[1] = posy - srcvelo[1] * dist / speed;
	(*ret)[2] = pos[2] + (velo[2] - srcvelo[2] + grv[2] * dist / speed / 2.) * dist / speed;*/
	ret = pos + (velo - srcvelo + grv * dist / speed / 2.) * dist / speed;
	return 1;
}

