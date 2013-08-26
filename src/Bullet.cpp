/** \file
 * \brief Implementation of Bullet and Explosive Bullet classes.
 */
#include "Bullet.h"
#include "ExplosiveBullet.h"
#include "Player.h"
#include "judge.h"
#ifndef DEDICATED
#include "draw/effects.h"
#include "draw/WarDraw.h"
#endif
#include "Game.h"
extern "C"{
#include <clib/mathdef.h>
}


Bullet::Bullet(WarField *w) : st(w), owner(NULL), damage(0), grav(true), life(1.), runlength(0), active(true){
}

Bullet::Bullet(Entity *aowner, float alife, double adamage) : st(aowner->w), owner(aowner), damage(adamage), grav(true), life(alife), runlength(0), active(true){
	if(owner)
		race = owner->race;
#ifndef DEDICATED
	extern int bullet_shoots;
	bullet_shoots++;
#endif
}

const char *Bullet::idname()const{
	return "bullet";
}

const char *Bullet::classname()const{
	return "Bullet";
}

const unsigned Bullet::classid = registerClass("Bullet", Conster<Bullet>);

void Bullet::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << damage;
	sc.o << life;
	sc.o << runlength;
	sc.o << owner;
	sc.o << grav;
}

void Bullet::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> damage;
	sc.i >> life;
	sc.i >> runlength;
	sc.i >> owner;
	sc.i >> grav;

	// If we were not deleted, we were alive in the server.
	// Make the client follow the server.
	active = true;
}

static int bullet_hit_callback(const struct otjEnumHitSphereParam *param, Entity *pt){
	const Vec3d *src = param->src;
	const Vec3d *dir = param->dir;
	double dt = param->dt;
	double rad = param->rad;
	Vec3d *retpos = param->pos;
	Vec3d *retnorm = param->norm;
	void *hint = param->hint;
//	struct entity_private_static *vft;
	Bullet *pb = ((Bullet**)hint)[1];
	WarField *w = pb->w;
	int *rethitpart = ((int**)hint)[2];
	Vec3d pos, nh;
	sufindex pi;
	double damage; /* calculated damage */
	int ret = 1;

	// if the ultimate owner of the objects is common, do not hurt yourself.
	Entity *bulletAncestor = pb->getUltimateOwner();
	Entity *hitobjAncestor = pt->getUltimateOwner();
	if(bulletAncestor == hitobjAncestor)
		return 0;

//	vft = (struct entity_private_static*)pt->vft;
	if(!jHitSphere(pt->pos, pt->getHitRadius() + pb->getHitRadius(), pb->pos, pb->velo, dt))
		return 0;
	{
		ret = pt->tracehit(pb->pos, pb->velo, pb->getHitRadius(), dt, NULL, &pos, &nh);
		if(!ret)
			return 0;
		else if(rethitpart)
			*rethitpart = ret;
	}
/*	else{
		Vec3d sc;
		double t;
		sc[0] = sc[1] = sc[2] = pt->getHitRadius() / 2.;
		if(!jHitBox(pt->pos, sc, pt->rot, pb->pos, pb->velo, 0., dt, &t, &pos, &nh)){
			return 0;
		}
	}*/

	{
		// Make the bulle hit effect occur at the position of trace hit point,
		// rather than the point on bullet's trajectory.
		// The tracehit() method should return more precise position.
		pb->pos = pos;
//		pb->pos += pb->velo * dt;

		if(retpos)
			*retpos = pos;
		if(retnorm)
			*retnorm = nh;
	}
	return ret;
}

/// \brief Callback logic for bullet hit event.
/// \param pt Pointer to the Entity being hit. Note that it can be NULL.
/// \param ws The surrounding WarSpace.
/// \param param Parameters for hit position and involving objects.
/// \returns false if the Bullet object is destroyed.
bool Bullet::bulletHit(Entity *pt, WarSpace *ws, otjEnumHitSphereParam &param){
	Bullet *pb = this;
	void **hint = (void**)param.hint;
	int &hitpart = *(int*)hint[2];

	sufindex pi;
	double damage; /* calculated damaga*/

	if(!ws->ot && pt && !bullet_hit_callback(&param, pt))
		return true;

	pb->pos = pos;
	if(pt)
		pt->onBulletHit(pb, hitpart);
#ifndef DEDICATED
	{ /* ricochet */
		if(ws->tell && rseq(&w->rs) % (ws->effects + 1) == 0){
/*			int j, n;
			Vec3d pyr, bvelo;
			bvelo = pb->velo - pt->velo;
			Vec3d delta = pos - pt->pos;
			pt->bullethole(pi, pb->damage * .00001, delta, Quatd::direction(delta));
			AddTeline3D(w->tell, pos, pt->velo, pb->damage * .0001 + .001, pyr, NULL, NULL, COLOR32RGBA(255,215,127,255), TEL3_NOLINE | TEL3_CYLINDER, .5 + .001 * pb->damage);*/
		}
		ws->effects++;
		if(hitpart != 1000 && ws->tell){
			Vec3d accel = w->accel(pb->pos, pb->velo);
			int j, n;
			frexp(pb->damage, &n);
			n = n / 2 + drseq(&w->rs);

			// Add spark sprite
			{
				double angle = w->rs.nextd() * 2. * M_PI / 2.;
				AddTelineCallback3D(ws->tell, pos, pt ? pt->velo : vec3_000, .0003 + n * .0005, Quatd(0, 0, sin(angle), cos(angle)),
					vec3_000, accel, sparkspritedraw, NULL, 0, .20 + drseq(&w->rs) * .20);
			}

			// Add spark traces
			for(j = 0; j < n; j++){
				Vec3d velo = -pb->velo.norm() * .2;
				for(int k = 0; k < 3; k++)
					velo[k] += .15 * (drseq(&w->rs) - .5);
/*				AddTeline3D(ws->tell, pos, velo, .001, quat_u, vec3_000, accel,
					j % 2 ? COLOR32RGBA(255,255,255,255) : COLOR32RGBA(255,191,63,255),
					TEL3_HEADFORWARD | TEL3_FADEEND, .5 + drseq(&w->rs) * .5);*/
				AddTelineCallback3D(ws->tell, pos, velo, .00025 + n * .0001, quat_u, vec3_000, accel, sparkdraw, NULL, TEL3_HEADFORWARD | TEL3_REFLECT, .20 + drseq(&w->rs) * .20);
			}
		}
	}
#endif
	damage = pb->damage;

	if(pt && pt->w == w) if(!pt->takedamage(this->damage, hitpart)){
#ifndef DEDICATED
		extern int bullet_kills, missile_kills;
		if(!strcmp("Bullet", classname()))
			bullet_kills++;
		else if(!strcmp("Missile", classname()))
			missile_kills++;
#endif
	}
//	makedamage(pb, pt, w, pb->damage, hitpart);

	bulletDeathEffect(-1, NULL);
	if(game->isServer())
		delete this;
	else
		active = false;

#ifndef DEDICATED
	extern int bullet_hits;
	bullet_hits++;
#endif

	return false;
}

void Bullet::anim(double dt){
	if(!w || life < 0. || !active)
		return;
	WarSpace *ws = *w;
	Bullet *const pb = this;
	struct contact_info ci;

	if(0 < pb->life && (pb->life -= dt) <= 0.){
/*		if(hitobj)
			*hitobj = NULL;
		return 0;*/
		if(game->isServer())
			delete this;
		return;
	}

	/* tank hit check */
	if(1){
		struct otjEnumHitSphereParam param;
		void *hint[3];
		Entity *pt;
		Vec3d pos, nh;
		int hitpart = 0;
		hint[0] = w;
		hint[1] = pb;
		hint[2] = &hitpart;
		param.root = ws->otroot;
		param.src = &pb->pos;
		param.dir = &pb->velo;
		param.dt = dt;
		param.rad = 0.;
		param.pos = &pos;
		param.norm = &nh;
		param.flags = OTJ_CALLBACK;
		param.callback = bullet_hit_callback;
		param.hint = hint;
#if 0
//		pt = w->ot ? otjHitSphere(&w->ot[w->oti-1], pb->pos, pb->velo, dt, 0., NULL) : NULL;
		for(pt = w->el; pt; pt = pt->next){
#else
		if(ws->ot){
			Entity *pt = otjEnumHitSphere(&param);
			if(pt)
				if(!bulletHit(pt, ws, param))
					return;
		}
		else{
			for(WarField::EntityList::iterator it = ws->el.begin(); it != ws->el.end(); it++) if(*it)
				if(!bulletHit(*it, ws, param))
					return;
		}
#endif

	}

#if 0
	if(ws) do{
		const btVector3 &btvelo = btvc(velo);
		btScalar velolen = btvelo.length();
		if(velolen == 0. || runlength == 0.)
			break;
		const btVector3 &to = btvc(pos) + dt * btvelo;
		const btVector3 from = btvc(pos) - min(runlength, velolen * dt) / velolen * btvelo; // backtrace
		btCollisionWorld::ClosestRayResultCallback rayCallback(from, to);

		ws->bdw->rayTest(from, to, rayCallback);

		if(rayCallback.hasHit() /*&& (!owner || rayCallback.m_collisionObject != owner->bbody)*/){
					Vec3d accel = w->accel(pb->pos, pb->velo);
					int j, n;
					frexp(pb->damage, &n);
					n = n / 2 + drseq(&w->rs);

//					if(rayCallback.m_collisionObject->upcast().getLinearVelocity());

					// Add spark sprite
					{
						double angle = w->rs.nextd() * 2. * M_PI / 2.;
						AddTelineCallback3D(ws->tell, pos, Vec3d(0,0,0), .0010 + n * .0005, Quatd(0, 0, sin(angle), cos(angle)),
							vec3_000, accel, sparkspritedraw, NULL, 0, .20 + drseq(&w->rs) * .20);
					}

					// Add spark traces
					for(j = 0; j < n; j++){
						Vec3d velo = -pb->velo.norm() * .2;
						for(int k = 0; k < 3; k++)
							velo[k] += .15 * (drseq(&w->rs) - .5);
/*						AddTeline3D(ws->tell, pos, velo, .001, quat_u, vec3_000, accel,
							j % 2 ? COLOR32RGBA(255,255,255,255) : COLOR32RGBA(255,191,63,255),
							TEL3_HEADFORWARD | TEL3_FADEEND, .5 + drseq(&w->rs) * .5);*/
						AddTelineCallback3D(ws->tell, pos, velo, .00025 + n * .0001, quat_u, vec3_000, accel, sparkdraw, NULL, TEL3_HEADFORWARD | TEL3_REFLECT, .20 + drseq(&w->rs) * .20);
					}
			bulletkill(-1, NULL);
			w = NULL;
			return;
		}
	}while(0);
#endif

	if(pb->grav){
		Vec3d accel = w->accel(pb->pos, pb->velo);
		pb->velo += accel * dt;
	}
	Vec3d move = pb->velo * dt;
	runlength += move.len();
	pb->pos += move;
}

/// In the client, Bullet only advances as if there're no obstacles.
void Bullet::clientUpdate(double dt){
	anim(dt);
/*	if(grav){
		Vec3d accel = w->accel(pos, velo);
		velo += accel * dt;
	}
	Vec3d move = velo * dt;
	runlength += move.len();
	pos += move;*/
}

double Bullet::getHitRadius()const{
	return 0.;
}

Entity *Bullet::getOwner(){
	return owner;
}

void Bullet::postframe(){
	st::postframe();
	if(!w)
		return;
	if(owner && owner->w != w)
		owner = NULL;
}


SQInteger Bullet::sqGet(HSQUIRRELVM v, const SQChar *name)const{
	if(!scstrcmp(name, _SC("life"))){
		if(!this){
			sq_pushnull(v);
			return 1;
		}
		sq_pushfloat(v, life);
		return 1;
	}
	else if(!scstrcmp(name, _SC("damage"))){
		if(!this){
			sq_pushnull(v);
			return 1;
		}
		sq_pushfloat(v, damage);
		return 1;
	}
	else if(!scstrcmp(name, _SC("owner"))){
		if(!this){
			sq_pushnull(v);
			return 1;
		}
		Entity::sq_pushobj(v, owner);
		return 1;
	}
	else
		return st::sqGet(v, name);
}

SQInteger Bullet::sqSet(HSQUIRRELVM v, const SQChar *name){
	if(!scstrcmp(name, _SC("life"))){
		SQFloat retf;
		if(SQ_FAILED(sq_getfloat(v, 3, &retf)))
			return SQ_ERROR;
		life = retf;
		return 0;
	}
	else if(!scstrcmp(name, _SC("damage"))){
		SQFloat retf;
		if(SQ_FAILED(sq_getfloat(v, 3, &retf)))
			return SQ_ERROR;
		damage = retf;
		return 0;
	}
	else if(!scstrcmp(name, _SC("owner"))){
		Entity *rete = Entity::sq_refobj(v, 3);
		owner = rete;
		if(owner) // Belong to owner's race
			race = owner->race;
		return 0;
	}
	else
		return st::sqSet(v, name);
}

#ifdef DEDICATED
void Bullet::drawtra(wardraw_t *wd){}
void Bullet::bulletkill(int hitground, const struct contact_info *ci){}
#endif

bool Bullet::isTargettable()const{
	return false;
}


//-----------------------------------------------------------------------------
//	ExplosiveBullet implementation
//-----------------------------------------------------------------------------

Entity::EntityRegister<ExplosiveBullet> ExplosiveBullet::entityRegister("ExplosiveBullet");

ExplosiveBullet::ExplosiveBullet(WarField *w) : st(w), splashRadius(0), friendlySafe(false){
}

const char *ExplosiveBullet::classname()const{return "ExplosiveBullet";}

SQInteger ExplosiveBullet::sqGet(HSQUIRRELVM v, const SQChar *name)const{
	if(!scstrcmp(name, _SC("splashRadius"))){
		sq_pushfloat(v, splashRadius);
		return 1;
	}
	else if(!scstrcmp(name, _SC("friendlySafe"))){
		sq_pushbool(v, friendlySafe);
		return 1;
	}
	else
		return st::sqGet(v, name);
}

SQInteger ExplosiveBullet::sqSet(HSQUIRRELVM v, const SQChar *name){
	if(!scstrcmp(name, _SC("splashRadius"))){
		SQFloat retf;
		if(SQ_FAILED(sq_getfloat(v, 3, &retf)))
			return SQ_ERROR;
		splashRadius = retf;
		return 0;
	}
	else if(!scstrcmp(name, _SC("friendlySafe"))){
		SQBool retb;
		if(SQ_FAILED(sq_getbool(v, 3, &retb)))
			return SQ_ERROR;
		friendlySafe = retb;
		return 0;
	}
	else
		return st::sqSet(v, name);
}

bool ExplosiveBullet::bulletHit(Entity *pt, WarSpace *ws, otjEnumHitSphereParam &param){
	// The hit Entity can be destroyed by splash damage effect.
	// In that case, pt can be invalid pointer after the next if block.
	// We must reserve a weak pointer before there to respond such a event to prevent
	// access violations.
	WeakPtr<Entity> wpt = pt;

	// Do splash damage
	if(0 < splashRadius){
		WarField::EntityList &el = ws->entlist();
		for(WarField::EntityList::iterator it = el.begin(); it != el.end();){
			WarField::EntityList::iterator next = it;
			++next;
			Entity *e = *it;
			double dist = (e->pos - this->pos).len();
			if((!friendlySafe || e->race != this->race) && dist < splashRadius)
				e->takedamage(damage * (splashRadius - dist) / splashRadius, -1);
			it = next;
		}
	}

	return Bullet::bulletHit(wpt, ws, param);
}

#ifdef DEDICATED
void ExplosiveBullet::drawtra(WarDraw *){}
void ExplosiveBullet::bulletDeathEffect(int hitground, const struct contact_info *ci){}
#endif
