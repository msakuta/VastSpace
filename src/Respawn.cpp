#include "respawn.h"
#include <clib/aquat.h>
#include <clib/rseq.h>

const unsigned Respawn::classid = registerClass("Respawn", Conster<Respawn>);
const char *Respawn::classname()const{return "Respawn";}

double Respawn::getHitRadius()const{return 0.;}
bool Respawn::isTargettable()const{return false;}

Respawn::Respawn(WarField *aw, double ainterval, double initial_phase, int max_count, const char *childClassName) : st(aw), interval(ainterval), timer(ainterval - initial_phase), count(max_count), creator(childClassName){
	init();
/*	VECNULL(ret->pos);
	VECNULL(ret->velo);
	VECNULL(ret->pyr);
	VECNULL(ret->omg);
	QUATZERO(ret->rot);
	ret->rot[3] = 1.;*/
	mass = 100000.; /* kilograms */
	moi = 15.; /* kilograms * kilometer^2 */
	health = 0.;
}


void Respawn::anim(double dt){
	if(count <= 0){
		w = NULL;
		return;
	}

	while(timer < dt){
		struct random_sequence rs;
		int i;
		Entity *pt2;

		{
			bool found = false;
			int c = 0;
			for(WarField::EntityList::iterator it = w->el.begin(); it != w->el.end(); it++) if(*it){
				Entity *pt2 = *it;
				if(pt2->w && 0. < pt2->getHealth() && pt2->race == race && !strcmp(pt2->classname(), creator)){
					if(count <= ++c){
						timer += interval;
						found = true;
						break;
					}
					if((pt2->pos - pos).slen() < pt2->getHitRadius() * pt2->getHitRadius()){
						timer += interval;
						found = true;
						break;
					}
				}
			}
			if(found)
				continue;
		}

		double wartime = w->war_time();
		init_rseq(&rs, (*(unsigned long*)&wartime) ^ (unsigned long)this);
		pt2 = Entity::create(creator, w);
		if(pt2){
			pt2->race = race;
			pt2->pos = pos;

			/* randomize the position a bit to avoid respawn camping */
			for(i = 0; i < 3; i++)
				pt2->pos[i] += (drseq(&rs) - .5) * .05;

			pt2->velo = velo;
			pt2->omg = omg;
			pt2->rot = rot;
			pt2->race = race;
			pt2->anim(dt - timer);
		}

		timer += interval;
/*		p->count--;*/
	}

	timer -= dt;
}


