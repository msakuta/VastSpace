/** \file
 * \brief Implementation of Frigate class (non-drawing methods).
 */
#define NOMINMAX // Prevent VC headers from defining min and max macros
#include "Frigate.h"
#include "Player.h"
#include "Bullet.h"
#include "CoordSys.h"
#include "Viewer.h"
#include "cmd.h"
#include "judge.h"
#include "serial_util.h"
#include "Sceptor.h"
#include "EntityCommand.h"
//#include "sensor.h"
#include "glw/popup.h"
#include "shield.h"
#ifndef DEDICATED
#include "draw/effects.h"
#endif
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
}
#include <assert.h>
#include <string.h>
#include <float.h>
#include <algorithm>



/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289

const double Frigate::modelScale = .0002;
double Frigate::shieldRadius()const{return .09;}

const struct Warpable::maneuve Frigate::frigate_mn = {
	.025, /* double accel; */
	.1, /* double maxspeed; */
	100., /* double angleaccel; */
	.4, /* double maxanglespeed; */
	50000., /* double capacity; [MJ] */
	100., /* double capacitor_gen; [MW] */
};


Frigate::Frigate(WarField *aw) : st(aw), shieldGenSpeed(0), paradec(-1){
	health = maxhealth();
	shieldAmount = maxshield();
}


void Frigate::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << shieldAmount;
	sc.o << shieldGenSpeed;
	sc.o << mother;
	sc.o << paradec;
}

void Frigate::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> shieldAmount;
	sc.i >> shieldGenSpeed;
	sc.i >> mother;
	sc.i >> paradec;
}


void Frigate::cockpitView(Vec3d &pos, Quatd &rot, int)const{
	pos = this->pos + this->rot.trans(Vec3d(.0, .05, .15));
	rot = this->rot;
}
void Frigate::anim(double dt){
	st::anim(dt);

	// If docked
	if(Docker *docker = *w){
		health = std::min(health + dt * 300., maxhealth()); // it takes several seconds to be fully repaired
		if(health == maxhealth() && !docker->remainDocked)
			docker->postUndock(this);
		return;
	}

	/* shield regeneration */
	if(0 < health){
		const double shieldPerEnergy = .5;
		shieldGenSpeed = MIN(shieldGenSpeed + 5. * dt, 300.);
		double gen = shieldGenSpeed * dt;
		if(capacitor < gen / shieldPerEnergy)
			gen = capacitor * shieldPerEnergy;
		if(maxshield() < shieldAmount + gen){
			shieldAmount = maxshield();
			capacitor -= (maxshield() - shieldAmount) / shieldPerEnergy;
		}
		else{
			shieldAmount += gen * shieldPerEnergy;
			capacitor -= gen / shieldPerEnergy;
		}
	}
	else
		w = NULL;

	se.anim(dt);

}
double Frigate::hitradius()const{return .1;}
Entity::Dockable *Frigate::toDockable(){return this;}
const Warpable::maneuve &Frigate::getManeuve()const{return frigate_mn;}


struct hitbox Frigate::hitboxes[] = {
	hitbox(Vec3d(0., 0., -.005), Quatd(0,0,0,1), Vec3d(.015, .015, .060)),
	hitbox(Vec3d(.025, -.015, .02), Quatd(0,0, -SIN15, COS15), Vec3d(.0075, .002, .02)),
	hitbox(Vec3d(-.025, -.015, .02), Quatd(0,0, SIN15, COS15), Vec3d(.0075, .002, .02)),
	hitbox(Vec3d(.0, .03, .0325), Quatd(0,0,0,1), Vec3d(.002, .008, .010)),
};
const int Frigate::nhitboxes = numof(Frigate::hitboxes);




int Frigate::takedamage(double damage, int hitpart){
	if(!w)
		return 1;
	Frigate *p = this;
	struct tent3d_line_list *tell = w->getTeline3d();
	int ret = 1;

	if(hitpart == 1000){
		se.takedamage(damage / maxshield());
		if(damage < p->shieldAmount)
			p->shieldAmount -= damage;
		else
			p->shieldAmount = -50.;
		shieldGenSpeed = 0.;
		return 1;
	}
	else if(p->shieldAmount < 0.)
		p->shieldAmount = -50.;

/*	if(tell){
		int j, n;
		frexp(damage, &n);
		for(j = 0; j < n; j++){
			double velo[3];
			velo[0] = .15 * (drseq(&w->rs) - .5);
			velo[1] = .15 * (drseq(&w->rs) - .5);
			velo[2] = .15 * (drseq(&w->rs) - .5);
			AddTeline3D(tell, pt->pos, velo, .001, NULL, NULL, w->gravity,
				j % 2 ? COLOR32RGBA(255,255,255,255) : COLOR32RGBA(255,191,63,255),
				TEL3_HEADFORWARD | TEL3_REFLECT | TEL3_FADEEND, .5 + drseq(&w->rs) * .5);
		}
	}*/
//	playWave3D("hit.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
	if(0 < p->health && p->health - damage <= 0){
		int i;
		ret = 0;
/*		effectDeath(w, pt);*/
		WarSpace *ws = *w;
		if(ws){
#if 0
			for(i = 0; i < 32; i++){
				Vec3d pos, velo;
				velo[0] = drseq(&w->rs) - .5;
				velo[1] = drseq(&w->rs) - .5;
				velo[2] = drseq(&w->rs) - .5;
				velo.normin();
				pos = p->pos;
				velo *= .1;
				pos += velo * .1;
				AddTeline3D(ws->tell, pos, velo, .005, NULL, NULL, NULL, COLOR32RGBA(255, 31, 0, 255), TEL3_HEADFORWARD | TEL3_THICK | TEL3_FADEEND, 15. + drseq(&w->rs) * 5.);
			}
#endif

#ifndef DEDICATED
			if(ws->gibs) for(i = 0; i < 32; i++){
				double pos[3], velo[3] = {0}, omg[3];
				/* gaussian spread is desired */
				for(int j = 0; j < 6; j++)
					velo[j / 2] += .0125 * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				omg[0] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				omg[1] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				omg[2] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				VECCPY(pos, this->pos);
				for(int j = 0; j < 3; j++)
					pos[j] += hitradius() * (drseq(&w->rs) - .5);
				AddTelineCallback3D(ws->gibs, pos, velo, .010, quat_u, omg, vec3_000, debrigib, NULL, TEL3_QUAT | TEL3_NOLINE, 15. + drseq(&w->rs) * 5.);
			}

			/* smokes */
			for(i = 0; i < 32; i++){
				double pos[3], velo[3];
				COLOR32 col = 0;
				VECCPY(pos, p->pos);
				pos[0] += .1 * (drseq(&w->rs) - .5);
				pos[1] += .1 * (drseq(&w->rs) - .5);
				pos[2] += .1 * (drseq(&w->rs) - .5);
				col |= COLOR32RGBA(rseq(&w->rs) % 32 + 127,0,0,0);
				col |= COLOR32RGBA(0,rseq(&w->rs) % 32 + 127,0,0);
				col |= COLOR32RGBA(0,0,rseq(&w->rs) % 32 + 127,0);
				col |= COLOR32RGBA(0,0,0,191);
	//			AddTeline3D(w->tell, pos, NULL, .035, NULL, NULL, NULL, col, TEL3_NOLINE | TEL3_GLOW | TEL3_INVROTATE, 60.);
				AddTelineCallback3D(ws->tell, pos, vec3_000, .03, quat_u, vec3_000, vec3_000, smokedraw, (void*)col, TEL3_INVROTATE | TEL3_NOLINE, 60.);
			}

			/* explode shockwave thingie */
			AddTeline3D(tell, this->pos, vec3_000, 3., quat_u, vec3_000, vec3_000, COLOR32RGBA(255,255,255,127), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_INVROTATE, 2.);
#endif
		}
//		playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
//		p->w = NULL;
	}
	p->health -= damage;
	return ret;
}

void Frigate::bullethit(const Bullet *pb){
	Frigate *p = this;
	if(pb->damage < p->shieldAmount){
		se.bullethit(this, pb, maxshield());
	}
}

int Frigate::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	Frigate *p = this;
	double sc[3];
	double best = dt, retf;
	int reti = 0, i, n;
	if(0 < p->shieldAmount){
		Vec3d hitpos;
		if(jHitSpherePos(pos, hitradius() + rad, src, dir, dt, ret, &hitpos)){
			if(retp) *retp = hitpos;
			if(retn) *retn = (hitpos - pos).norm();
			return 1000; /* something quite unlikely to reach */
		}
	}
	for(n = 0; n < nhitboxes; n++){
		Vec3d org;
		Quatd rot;
		org = this->rot.itrans(hitboxes[n].org) + this->pos;
		rot = this->rot * hitboxes[n].rot;
		for(i = 0; i < 3; i++)
			sc[i] = hitboxes[n].sc[i] + rad;
		if((jHitBox(org, sc, rot, src, dir, 0., best, &retf, retp, retn)) && (retf < best)){
			best = retf;
			if(ret) *ret = retf;
			reti = i + 1;
		}
	}
	return reti;
}

Entity::Props Frigate::props()const{
	Props ret = st::props();
//	ret.push_back(cpplib::dstring("Shield: ") << shieldAmount << '/' << maxshield());
	return ret;
}

bool Frigate::solid(const Entity *o)const{
	return !(task == sship_undock || task == sship_warp);
}

bool Frigate::command(EntityCommand *com){
	if(InterpretCommand<ParadeCommand>(com)){
		findMother();
		task = sship_parade;
		enemy = NULL; // Temporarily forget about enemy
		return true;
	}
	else if(AttackCommand *ac = InterpretDerivedCommand<AttackCommand>(com)){
		if(!ac->ents.empty()){
			enemy = *ac->ents.begin();
			task = sship_attack;
		}
		return true;
	}
	else return st::command(com);
}

Entity *Frigate::findMother(){
	Entity *pm = NULL;
	double best = 1e10 * 1e10, sl;
	for(WarField::EntityList::iterator it = w->entlist().begin(); it != w->entlist().end(); it++) if(*it){
		Entity *e = *it;
		if(e->getDocker() && (sl = (e->pos - this->pos).slen()) < best){
			mother = e->getDocker();
			pm = mother->e;
			best = sl;
		}
	}
	return pm;
}


double Frigate::maxenergy()const{return frigate_mn.capacity;}
double Frigate::maxshield()const{return 5000.;}

#ifdef DEDICATED
int Frigate::popupMenu(PopupMenu &){}
void Frigate::drawShield(wardraw_t *wd){}
#endif
