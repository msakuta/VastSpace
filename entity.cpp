#include "entity.h"
extern "C"{
#include <clib/aquat.h>
}
#include "beamer.h"
#include "judge.h"
#include "player.h"

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
	Entity **ppt;
	Vec3d pos;
	Mat4d mat;
	if(w == cs->w)
		return;
/*	for(ppt = &w->tl; *ppt; ppt = &(*ppt)->next) if(*ppt == pt){
		break;
	}
	assert(*ppt);
	*ppt = pt->next;
	pt->next = cs->w->tl;*/
	this->pos = cs->tocs(this->pos, w->cs);
	velo = cs->tocsv(velo, pos, w->cs);
	if(!cs->w){
		cs->w = new WarField(cs);
	}
/*	cs->w->tl = pt;*/
	w = cs->w;
	{
		Quatd q, q1;
		q1 = w->cs->tocsq(cs);
		q = q1 * this->rot;
		this->rot = q;
	}
/*	{
		aquat_t q;
		avec3_t pyr;
		amat4_t rot, rot2, rot3;
		tocsm(rot, cs, w->cs);
		QUATSCALE(q, pt->rot, -1);
		quat2imat(rot2, q);
		mat4mp(rot3, rot2, rot);
		imat2pyr(rot3, pyr);
		VECSCALEIN(pyr, -1);
		pyr2quat(pt->rot, pyr);
	}*/
	/*
	tocsim(mat, cs, w->cs);
	{
		aquat_t q, qr;
		imat2quat(mat, q);
		QUATMUL(qr, pt->rot, q);
		QUATCPY(pt->rot, qr);
	}*/
/*	VECNULL(pt->pos);*/
	if(w->pl->chase == this){
		w->pl->pos = cs->tocs(w->pl->pos, w->pl->cs);
		w->pl->cs = cs;
	}
}
