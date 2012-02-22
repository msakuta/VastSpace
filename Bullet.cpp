#include "Bullet.h"
#include "CoordSys.h"
#include "astro.h"
#include "Player.h"
#include "judge.h"
#include "serial_util.h"
#include "draw/effects.h"
#include "draw/WarDraw.h"
//#include "warutil.h"
//#include "draw/material.h"
#include "btadapt.h"
extern "C"{
#ifndef DEDICATED
#include "bitmap.h"
#endif
#include <clib/amat4.h>
#include <clib/c.h>
#include <clib/mathdef.h>
#include <clib/colseq/cs.h>
#include <clib/sound/wavemixer.h>
#include <clib/suf/sufbin.h>
#include <clib/gl/gldraw.h>
}
#include <GL/glext.h>
#include <limits.h>
#include <stddef.h>


Bullet::Bullet(Entity *aowner, float alife, double adamage) : st(aowner->w), owner(aowner), damage(adamage), grav(true), life(alife), runlength(0){
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
}

#if 0

static void bullet_draw(struct bullet *, wardraw_t*);
static void bullet_drawmodel(struct bullet *pb, wardraw_t *wd){pb,wd;}
static int bullet_is_bullet(bullet_t *p){
	return 1;
}

static struct bullet_static bullet_s = {
	{NULL},
	bullet_anim,
	bullet_draw,
	bullet_drawmodel,
	bullet_postframe,
	bullet_get_homing_target,
	bullet_is_bullet,
};


static void nbullet_draw(struct bullet *pb, wardraw_t *wd);
static void nbullet_drawmodel(struct bullet *pb, wardraw_t *wd);
static void sbullet_drawmodel(struct bullet *pb, wardraw_t *wd);
static void mortarhead_drawmodel(struct bullet *pt, wardraw_t *wd);
static int shrapnelshell_anim(struct bullet *, warf_t *, struct tent3d_line_list *, double dt);
static void APFSDS_draw(struct bullet *pb, wardraw_t *wd);
static void APFSDS_drawmodel(struct bullet *pb, wardraw_t *wd);
static int bulletswarm_anim(struct bullet *, warf_t *, struct tent3d_line_list *, double dt);
static void bulletswarm_draw(struct bullet *, wardraw_t*);
static void bulletswarm_drawmodel(struct bullet *pb, wardraw_t *wd);

static struct bullet_static NormalBullet_s = {
	{NULL},
	bullet_anim,
	nbullet_draw,
	nbullet_drawmodel,
	bullet_postframe,
	bullet_get_homing_target,
	bullet_is_bullet,
}, ShotgunBullet_s = {
	{NULL},
	bullet_anim,
	nbullet_draw,
	sbullet_drawmodel,
	bullet_postframe,
	bullet_get_homing_target,
	bullet_is_bullet,
}, MortarHead_s = {
	{NULL},
	bullet_anim,
	nbullet_draw,
	mortarhead_drawmodel,
	bullet_postframe,
	bullet_get_homing_target,
	bullet_is_bullet,
}, TracerBullet_s = {
	{NULL},
	bullet_anim,
	nbullet_draw,
	nbullet_drawmodel,
	bullet_postframe,
	bullet_get_homing_target,
	bullet_is_bullet,
}, ExplosiveBullet_s = {
	{NULL},
	bullet_anim,
	nbullet_draw,
	nbullet_drawmodel,
	bullet_postframe,
	bullet_get_homing_target,
	bullet_is_bullet,
}, ShrapnelShell_s = {
	{NULL},
	shrapnelshell_anim,
	bullet_draw,
	bullet_drawmodel,
	bullet_postframe,
	bullet_get_homing_target,
	bullet_is_bullet,
}, APFSDS_s = {
	{NULL},
	bullet_anim,
	nbullet_draw/*APFSDS_draw*/,
	APFSDS_drawmodel,
	bullet_postframe,
	bullet_get_homing_target,
	bullet_is_bullet,
}, BulletSwarm_s = {
	{NULL},
	bulletswarm_anim,
	bulletswarm_draw,
	bulletswarm_drawmodel,
	bullet_postframe,
	bullet_get_homing_target,
	bullet_is_bullet,
};

typedef struct NormalBullet{
	bullet_t st;
	double rad;
} NormalBullet;

typedef struct TracerBullet{
	bullet_t st;
	double rad;
} TracerBullet;

typedef struct BulletSwarm{
	bullet_t st;
	double rad; /* Radius of bulletswarm */
	double vrad; /* Velocity of radius expansion */
	int count;
	unsigned baf[1]; /* Bullet Availability Flags; extensible array */
} BulletSwarm;

static void BulletInit(struct bullet *pb, warf_t *w, struct bullet_static *vft){
	pb->active = 1;
	pb->grav = 1;
	pb->vft = vft;
	if(w){
		pb->next = w->bl;
		w->bl = pb;
	}
	else
		pb->next = NULL;
	VECNULL(pb->pos);
	VECNULL(pb->velo);
	VECNULL(pb->pyr);
	VECNULL(pb->omg);
	QUATIDENTITY(pb->rot);
	pb->mass = 1.;
	pb->moi = 1.;
	pb->health = 1.;
	pb->w = w;
	pb->enemy = NULL;
	pb->race = pb->shoots = pb->shoots2 = pb->kills = pb->deaths = 0;
	pb->inputs.change = pb->inputs.press = 0;
	pb->inputs.start = 0;
	pb->weapon = 0;
}

struct bullet *add_bullet(warf_t *w){
	struct bullet *pb;
	pb = malloc(sizeof*pb);
	BulletInit(pb, w, &bullet_s);
	return pb;
}

struct bullet *BulletNew(warf_t *w, entity_t *owner, double damage){
	struct bullet *pb;
	pb = add_bullet(w);
	pb->owner = owner;
	pb->damage = damage;
	pb->mass = damage;
	pb->life = 0.;
	return pb;
}
#endif


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
	if(!jHitSphere(pt->pos, pt->hitradius() + pb->hitradius(), pb->pos, pb->velo, dt))
		return 0;
	{
		ret = pt->tracehit(pb->pos, pb->velo, pb->hitradius(), dt, NULL, &pos, &nh);
		if(!ret)
			return 0;
		else if(rethitpart)
			*rethitpart = ret;
	}
/*	else{
		Vec3d sc;
		double t;
		sc[0] = sc[1] = sc[2] = pt->hitradius() / 2.;
		if(!jHitBox(pt->pos, sc, pt->rot, pb->pos, pb->velo, 0., dt, &t, &pos, &nh)){
			return 0;
		}
	}*/

	{
		pb->pos += pb->velo * dt;
		if(retpos)
			*retpos = pos;
		if(retnorm)
			*retnorm = nh;
	}
	return ret;
}

bool Bullet::bullethit(Entity *pt, WarSpace *ws, otjEnumHitSphereParam &param){
	Bullet *pb = this;
	void **hint = (void**)param.hint;
	int &hitpart = *(int*)hint[2];

			sufindex pi;
			double damage; /* calculated damaga*/

			if(!ws->ot && !bullet_hit_callback(&param, pt))
				return true;

			pb->pos = pos;
			pt->bullethit(pb);
#if 1
#ifndef DEDICATED
			{ /* ricochet */
				if(ws->tell && rseq(&w->rs) % (ws->effects + 1) == 0){
/*					int j, n;
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
						AddTelineCallback3D(ws->tell, pos, pt->velo, .0010 + n * .0005, Quatd(0, 0, sin(angle), cos(angle)),
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
				}
#if 0
					{
						frexp(pb->damage, &n);
						VECNORMIN(bvelo);
						VECSCALEIN(bvelo, -(.05 + .001 * pb->damage));
						for(j = 0; j < n; j++){
							double velo[3], len[3];
							GLubyte r, g, b;
							r = 191 + rseq(&w->rs) % 64;
							g = 128 + rseq(&w->rs) % 128;
							b = 128 + rseq(&w->rs) % 64;
							velo[0] = bvelo[0] * drseq(&gsrs);
							velo[1] = bvelo[1] * drseq(&gsrs);
							velo[2] = bvelo[2] * drseq(&gsrs);
							VECNORM(len, velo);
	/*						pyr[0] = atan2(velo[0], -velo[2]) + M_PI;
							pyr[1] = atan2(velo[1], sqrt(velo[0] * velo[0] + velo[2] * velo[2]));
							pyr[2] = 0.;*/
							VECADDIN(velo, pt->velo);
							AddTeline3D(tell, pos, velo, pb->damage * .0001 + .001, len, NULL, gravity,
								COLOR32RGBA(r,g,b,255),
								TEL3_THICK | TEL3_FADEEND, .15 + .15 * drseq(&gsrs));
						}
					}
#endif
			}
#endif
			damage = pb->damage;

#if 0
			{
			extern double g_bulletimpact;
			extern struct entity_private_static container_s;
			if(/*0 && pt->vft == &tank_s ||*/ pt->vft == &container_s || ((struct entity_private_static*)pt->vft)->altaxis & 4){
				double lmomentum;
				avec3_t dr, momentum, amomentum;
				VECSUB(dr, pb->pos, pt->pos);
				VECSUBIN(dr, ((struct entity_private_static*)pt->vft)->cog);
				lmomentum = g_bulletimpact * pb->mass/* * VECLEN(pb->velo)*/;
				VECSCALE(momentum, pb->velo, lmomentum);
				RigidAddMomentum(pt, dr, momentum);
#if 0
				VECSADD(pt->velo, momentum, 1. / pt->mass);

				/* vector product of dr and momentum becomes angular momentum */
				VECVP(amomentum, dr, momentum);
				VECSADD(pt->omg, amomentum, 1. / pt->moi);
#endif
			}
			}

			if(hitobj)
				*hitobj = pt;
#endif
#endif

			if(pt->w == w) if(!pt->takedamage(this->damage, hitpart)){
#ifndef DEDICATED
				extern int bullet_kills, missile_kills;
				if(!strcmp("Bullet", classname()))
					bullet_kills++;
				else if(!strcmp("Missile", classname()))
					missile_kills++;
#endif
			}
//			makedamage(pb, pt, w, pb->damage, hitpart);

			bulletkill(-1, NULL);
			delete this;

#ifndef DEDICATED
			extern int bullet_hits;
			bullet_hits++;
#endif

/*			{
				avec3_t delta;
				VECSUB(delta, pb->pos, pt->pos);
				VECNORM(ci.normal, delta);
				ci.depth = 0.;
				VECCPY(ci.velo, pt->velo);
				bulletkill(pb, w, w->tell, avec3_000, 1, &ci);
			}*/

			return false;
}

void Bullet::anim(double dt){
	if(!w)
		return;
	WarSpace *ws = *w;
	Bullet *const pb = this;
	struct contact_info ci;

	if(0 < pb->life && (pb->life -= dt) <= 0.){
/*		if(hitobj)
			*hitobj = NULL;
		return 0;*/
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
				if(!bullethit(pt, ws, param))
					return;
		}
		else{
			for(WarField::EntityList::iterator it = ws->el.begin(); it != ws->el.end(); it++) if(*it)
				if(!bullethit(*it, ws, param))
					return;
		}
#endif

	}

#if 0 // world hit
	{
		int i;
		for(i = 0; i <= 4; i++){
			avec3_t pos;
			VECCPY(pos, pb->pos);
			VECSADD(pos, pb->velo, dt * i / 4);
			if(w->vft->pointhit(w, &pos, &pb->velo, dt, &ci) /*pb->pos[1] <= (w->map ? warmapheight(w->map, pb->pos[0], pb->pos[2], NULL) : 0.)*/ /*|| BULLETRANGE * BULLETRANGE < pb->pos[0] * pb->pos[0] + pb->pos[1] * pb->pos[1] + pb->pos[2] * pb->pos[2]*/){
				static const double pyr[3] = {M_PI / 2., 0., 0.};
				VECCPY(pb->pos, pos);
				bulletkill(pb, w, w->tell, pyr, 1, &ci);
				if(hitobj)
					*hitobj = NULL;
				return 0;
			}
		}
	}
#endif

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
	if(grav){
		Vec3d accel = w->accel(pos, velo);
		velo += accel * dt;
	}
	Vec3d move = velo * dt;
	runlength += move.len();
	pos += move;
}

double Bullet::hitradius()const{
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

#ifdef DEDICATED
void Bullet::drawtra(wardraw_t *wd){}
void Bullet::bulletkill(int hitground, const struct contact_info *ci){}
#endif

bool Bullet::isTargettable()const{
	return false;
}
