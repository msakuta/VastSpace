/** \file
 * \brief Implementation of WarField and WarSpace's motions.
 */
#include "Warpable.h"
#include "Player.h"
#include "Bullet.h"
#include "CoordSys.h"
#include "Viewer.h"
#include "cmd.h"
#include "judge.h"
#include "astrodef.h"
#include "stellar_file.h"
#include "astro_star.h"
#include "serial_util.h"
#include "glstack.h"
#include "EntityCommand.h"
#include "btadapt.h"
#include "sqadapt.h"
#include "arms.h"
#include "Docker.h"
//#include "sensor.h"
#include "motion.h"
#include "draw/WarDraw.h"
#include "glsl.h"
#include "dstring.h"
#include "Game.h"
#include "StaticInitializer.h"
extern "C"{
#ifdef _WIN32
#include "bitmap.h"
#endif
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/suf/sufbin.h>
#include <clib/timemeas.h>
#ifdef _WIN32
#include <clib/suf/sufdraw.h>
#include <clib/GL/gldraw.h>
#include <clib/GL/multitex.h>
#include <clib/wavsound.h>
#endif
#include <clib/zip/UnZip.h>
}
#include <cpplib/vec4.h>
#include <assert.h>
#include <string.h>
#include <btBulletDynamicsCommon.h>
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"

/*
Raytracer uses the Convex rayCast to visualize the Collision Shapes/Minkowski Sum.
Very basic raytracer, rendering into a texture.
*/

///Low level demo, doesn't include btBulletCollisionCommon.h

#include "LinearMath/btQuaternion.h"
#include "LinearMath/btTransform.h"

//#include "BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.h"
//#include "BulletCollision/NarrowPhaseCollision/btSubSimplexConvexCast.h"
#include "BulletCollision/NarrowPhaseCollision/btGjkConvexCast.h"
//#include "BulletCollision/NarrowPhaseCollision/btContinuousConvexCollision.h"

//#include "BulletCollision/CollisionShapes/btSphereShape.h"
//#include "BulletCollision/CollisionShapes/btMultiSphereShape.h"

//#include "BulletCollision/CollisionShapes/btConvexHullShape.h"
//#include "LinearMath/btAabbUtil2.h"
//#include "BulletCollision/CollisionShapes/btBoxShape.h"
//#include "BulletCollision/CollisionShapes/btCompoundShape.h"




/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289



double g_capacitor_gen_factor = 1.;

/* color sequences */
#if 0
#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_orangeburn[] = {
	{0.1, COLOR32RGBA(255,255,191,255)},
	{0.15, COLOR32RGBA(255,255,31,191)},
	{0.45, COLOR32RGBA(255,127,31,95)},
	{2.3, COLOR32RGBA(255,31,0,63)},
};
static const struct color_sequence cs_orangeburn = DEFINE_COLSEQ(cnl_orangeburn, (COLOR32)-1, 3.);
static const struct color_node cnl_shortburn[] = {
	{0.1, COLOR32RGBA(255,255,191,255)},
	{0.15, COLOR32RGBA(255,255,31,191)},
	{0.25, COLOR32RGBA(255,127,31,0)},
};
static const struct color_sequence cs_shortburn = DEFINE_COLSEQ(cnl_shortburn, (COLOR32)-1, 0.5);
#endif


#if 0


static void warp_anim(struct war_field *, double dt);
static void warp_draw(struct war_field *, struct war_draw_data *);
static int warp_pointhit(warf_t *w, const avec3_t *pos, const avec3_t *velo, double dt, const struct contact_info *);
static void warp_accel(warf_t *w, avec3_t *dst, const avec3_t *srcpos, const avec3_t *srcvelo);
static double warp_atmospheric_pressure(warf_t *w, const avec3_t *pos);
static int warp_spherehit(warf_t *w, const avec3_t *pos, double rad, struct contact_info *ci);
static int warp_orientation(warf_t *w, amat3_t *dst, const avec3_t *pos);

static struct war_field_static warp_static = {
	warp_anim,
	WarPostFrame,
	WarEndFrame,
	warp_draw,
	warp_pointhit,
	warp_accel,
	warp_atmospheric_pressure,
	warp_atmospheric_pressure,
	warp_spherehit,
	warp_orientation,
};


#endif

#if 0
static void Warp::anim(struct war_field *w, double dt){
	struct bullet **ppb = &w->bl;
	static int wingfile = 0;
	static int walks = 0, apaches = 2;

	w->effects = 0;

	w->war_time += dt;

	while(*ppb){
		struct bullet *pb = *ppb;
		if(!pb->vft->anim(pb, w, w->tell, dt)){
			pb->active = 0;
		}
		ppb = &pb->next;
	}

	/* tank think */
	{
		entity_t **ppt = &w->tl;
		for(; *ppt; ppt = &(*ppt)->next){
			((struct entity_private_static*)(*ppt)->vft)->anim(*ppt, w, dt);
		}
	}
#if 0

	/* postframe process. do not change activity state while in postframe functions. */
	{
		struct bullet *pb = w->bl;
		for(; pb; pb = pb->next)
			pb->vft->postframe(pb);
	}
	{
		entity_t *pt = w->tl;
		for(; pt; pt = pt->next) if(((struct entity_private_static*)pt->vft)->postframe)
			((struct entity_private_static*)pt->vft)->postframe(pt);
	}

	/* purge inactive entities */
/*	ppb = &w->bl;
	while(*ppb){
		struct bullet *pb = *ppb;
		if(!pb->active){
			if(pb == (struct bullet*)w->pl->chase)
				w->pl->chase = NULL;
			{
				struct bullet *pb = (*ppb)->next;
				free(*ppb);
				*ppb = pb;
			}
			continue;
		}
		ppb = &pb->next;
	}*/
	{
		entity_t **ppt;
		warf_t *dw;
		for(ppt = &w->tl; *ppt; ) if(!(*ppt)->active && !(1 & ((struct entity_private_static*)(*ppt)->vft)->reuse) /*&& ((*ppt)->vft == &flare_s || (*ppt)->vft != &fly_s && (*ppt)->vft != &tank_s)*/){

			/* there are two possibilities to disappear from a warfield, that is
			  to be destroyed or to be transported to another field. we determine
			  this by calling the virtual function designed for this. 
			   note that activity state must be disabled prior to transportation,
			  to force referrers to clear their inter-field references that tend to
			  cause problems due to assumption referrer and referree belongs to the
			  same field. */
			if((*ppt)->vft->warp_dest && (dw = (*ppt)->vft->warp_dest(*ppt, w)) && dw != w){
				entity_t *pt = *ppt;
				*ppt = pt->next;
				pt->next = dw->tl;
				dw->tl = pt;
				pt->active = 1;
			}
			else{
				entity_t *next = (*ppt)->next;
				if(!(2 & ((struct entity_private_static*)(*ppt)->vft)->reuse)){
					if(*ppt == w->pl->chase)
						w->pl->chase = NULL;
					if(*ppt == w->pl->control)
						w->pl->control = NULL;
					if(*ppt == w->pl->selected)
						w->pl->selected = NULL;
					if((*ppt)->vft->destruct)
						(*ppt)->vft->destruct(*ppt);
				}
				if(!(2 & ((struct entity_private_static*)(*ppt)->vft)->reuse))
					free(*ppt);
				*ppt = next;
			}
		}
		else
			ppt = &(*ppt)->next;
	}
#endif

}

static void warp_draw(struct war_field *w, struct war_draw_data *wd){
	amat4_t lightmats[3];
	struct bullet **ppb;

	wd->lightdraws = 0;
	wd->light_on(wd);

	g_gldcache.valid = 0;
	{
		entity_t *pt;
		for(pt = w->tl; pt; pt = pt->next) if(((const struct entity_private_static*)pt->vft)->draw){
			((const struct entity_private_static*)pt->vft)->draw(pt, wd);
		}
	}
	{
		GLfloat whity[4] = {1., 1., 1., 1.};
		gldMaterialfv(GL_FRONT, GL_DIFFUSE, &whity, &g_gldcache);
		gldMaterialfv(GL_FRONT, GL_AMBIENT, &whity, &g_gldcache);
	}
	for(ppb = &w->bl; *ppb; ppb = &(*ppb)->next){
		struct bullet *pb = *ppb;
		pb->vft->drawmodel(pb, wd);
	}
	if(w->gibs){
		struct tent3d_line_drawdata dd;
/*		glPushAttrib(GL_ENABLE_BIT);*/
		glDisable(GL_CULL_FACE);
		VECCPY(dd.viewpoint, wd->view);
		VECCPY(dd.viewdir, wd->viewdir);
		VECNULL(dd.pyr);
		memcpy(dd.invrot, wd->irot, sizeof dd.invrot);
		dd.fov = wd->fov;
		dd.pgc = wd->pgc;
		DrawTeline3D(w->gibs, &dd);
/*		glPopAttrib();*/
	}
/*	glCallList(wd->listLight + 1);*/
	wd->light_off(wd);


	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(0);
	{
		entity_t *pt;
		for(pt = w->tl; pt; pt = pt->next){
			if(((const struct entity_private_static*)pt->vft)->drawtra)
				((const struct entity_private_static*)pt->vft)->drawtra(pt, wd);
		}
	}
	for(ppb = &w->bl; *ppb; ppb = &(*ppb)->next){
		struct bullet *pb = *ppb;
		pb->vft->draw(pb, wd);
	}
	if(w->tepl)
		DrawTefpol3D(w->tepl, wd->view, wd->pgc);

	glPopAttrib();
}

static int warp_pointhit(warf_t *w, const avec3_t *pos, const avec3_t *velo, double dt, struct contact_info *ci){
	return 0;
}

static void warp_accel(warf_t *w, avec3_t *dst, const avec3_t *srcpos, const avec3_t *srcvelo){
	VECNULL(*dst);
}

static double warp_atmospheric_pressure(warf_t *w, const avec3_t *pos){
	return 0.;
}

static int warp_spherehit(warf_t *w, const avec3_t *pos, double plrad, struct contact_info *ci){
	return 0;
}

static int warp_orientation(warf_t *w, amat3_t *dst, const avec3_t *pos){
	MATIDENTITY(*dst);
	return 1;
}



#endif



// A wrapper class to designate it's a warping envelope.
class WarpBubble : public CoordSys{
public:
	typedef CoordSys st;
	static const ClassRegister<WarpBubble> classRegister;
	WarpBubble(Game *game) : st(game){}
	WarpBubble(const char *path, CoordSys *root) : st(path, root){}
	virtual const Static &getStatic()const{return classRegister;}
	static int serialNo;
};
const ClassRegister<WarpBubble> WarpBubble::classRegister("WarpBubble");
int WarpBubble::serialNo = 0;









#ifndef _WIN32
void Warpable::drawtra(wardraw_t *wd){}
int Warpable::popupMenu(PopupMenu &list){}
void Warpable::drawHUD(wardraw_t *wd){}
#endif

short Warpable::getDefaultCollisionMask()const{
	return -1;
}

/** \brief Definition of how to maneuver spaceships, shared among ship classes deriving Warpable.
 *
 * Assumption is that accelerations towards all directions except forward movement
 * is a half the maximum accel. */
void Warpable::maneuver(const Mat4d &mat, double dt, const struct maneuve *mn){
	Entity *pt = this;
	double const maxspeed2 = mn->maxspeed * mn->maxspeed;
	if(!warping){
		// I don't really get condition of deactivation, but it seems sure that moving objects can get deactivated,
		// which is not appreciated in the simulation in space.
		if(bbody && (!bbody->getLinearVelocity().isZero() || !bbody->getAngularVelocity().isZero()))
			bbody->activate();

		Vec3d torque = vec3_000;
		if(pt->inputs.press & PL_2){
			torque += mat.vec3(0) * (mn->angleaccel);
		}
		if(pt->inputs.press & PL_8){
			torque += mat.vec3(0) * (-mn->angleaccel);
		}
		if(pt->inputs.press & PL_4){
			torque += mat.vec3(1) * (mn->angleaccel);
		}
		if(pt->inputs.press & PL_6){
			torque += mat.vec3(1) * (-mn->angleaccel);
		}
		if(pt->inputs.press & PL_7){
			torque += mat.vec3(2) * (mn->angleaccel);
		}
		if(pt->inputs.press & PL_9){
			torque += mat.vec3(2) * (-mn->angleaccel);
		}
		if(bbody && mn->maxanglespeed * mn->maxanglespeed < pt->omg.slen()){
			pt->omg.normin();
			pt->omg *= mn->maxanglespeed;
		}

		if((pt->inputs.press & (PL_8 | PL_2 | PL_4 | PL_6 | PL_7 | PL_9))){
			if(bbody){
				bbody->activate();
				if(bbody->getAngularVelocity().length2() < mn->maxanglespeed * mn->maxanglespeed)
					bbody->applyTorque(btvc(torque));
			}
			else{
				double f;
				pt->omg += torque;
				f = pt->omg.len();
				if(f){
					f = MAX(0, f - dt * mn->angleaccel) / f;
					pt->omg *= f;
				}
			}
		}
		else if(bbody){
			btVector3 btomg = bbody->getAngularVelocity();

			// Control rotation to approach stationary. Avoid expensive tensor products for zero vectors.
			if(!btomg.isZero()){
				btVector3 torqueImpulseToStop = bbody->getInvInertiaTensorWorld().inverse() * -btomg;
				if(torqueImpulseToStop.length2() < dt * mn->angleaccel * dt * mn->angleaccel)
					bbody->applyTorqueImpulse(torqueImpulseToStop);
				else
					bbody->applyTorqueImpulse(torqueImpulseToStop.normalize() * dt * mn->angleaccel);
			}
		}

		Vec3d forceAccum(0,0,0);
		if(pt->inputs.press & PL_W){
			forceAccum += mat.vec3(2) * -mn->accel;
			pt->velo += mat.vec3(2) * (-dt * mn->accel);
		}
		if(pt->inputs.press & PL_S){
			forceAccum += mat.vec3(2) * mn->accel * .5;
			pt->velo += mat.vec3(2) * dt * mn->accel * .5;
		}
		if(pt->inputs.press & PL_A){
			forceAccum += mat.vec3(0) * -mn->accel * .5;
			pt->velo += mat.vec3(0), -dt * mn->accel * .5;
		}
		if(pt->inputs.press & PL_D){
			forceAccum += mat.vec3(0) * mn->accel * .5;
			pt->velo += mat.vec3(0),  dt * mn->accel * .5;
		}
		if(pt->inputs.press & PL_Q){
			forceAccum += mat.vec3(1) * mn->accel * .5;
			pt->velo += mat.vec3(1),  dt * mn->accel * .5;
		}
		if(pt->inputs.press & PL_Z){
			forceAccum += mat.vec3(1) * -mn->accel * .5;
			pt->velo += mat.vec3(1), -dt * mn->accel * .5;
		}
		if(pt->inputs.press & (PL_W | PL_S | PL_A | PL_D | PL_Q | PL_Z)){
			if(bbody){
				btVector3 btforceAccum = btvc(forceAccum);
				assert(!btforceAccum.isZero());
				btVector3 btdirection = btforceAccum.normalized();
				btVector3 btvelo = bbody->getLinearVelocity();
				btVector3 btmainThrust(0,0,0);

				// If desired steering direction is differing from real velocity,
				// compensate sliding velocity with side thrusts.
				if(!btvelo.isZero()){
					if(btvelo.dot(btdirection) < -DBL_EPSILON)
						btmainThrust = -btvelo.normalized() * mn->accel * .5;
					else{
						btVector3 v = btvelo - btvelo.dot(btdirection) * btdirection;
						if(!v.fuzzyZero())
							btmainThrust = -v.normalize() * mn->accel * .5;
					}

				}

				if(btvelo.length2() < mn->maxspeed * mn->maxspeed){
					btmainThrust += btforceAccum;
				}

				bbody->applyCentralForce(btmainThrust / bbody->getInvMass());
			}
			else if(pt->velo.slen() < maxspeed2);
			else{
				pt->velo.normin();
				pt->velo *= mn->maxspeed;
			}
		}
		else if(bbody){
			btVector3 btvelo = bbody->getLinearVelocity();

			// Try to stop motion if not instructed to move.
			if(!btvelo.isZero()){
				btVector3 impulseToStop = -btvelo / bbody->getInvMass();
				double thrust = dt * mn->accel * .5 / bbody->getInvMass();
				if(impulseToStop.length2() < thrust * thrust)
					bbody->applyCentralImpulse(impulseToStop);
				else
					bbody->applyCentralImpulse(impulseToStop.normalize() * thrust);
			}
		}
		if(!bbody){
			double f, dropoff = !(pt->inputs.press & (PL_W | PL_S | PL_A | PL_D | PL_Q | PL_Z)) ? mn->accel : mn->accel * .2;
			f = pt->velo.len();
			if(f){
				f = MAX(0, f - dt * dropoff) / f;
				pt->velo *= f;
			}
		}
	}
	direction = inputs.press;
}

/** \brief A steering behavior of arrival, gradually decreases relative velocity to the destination in the way of approaching.
 *
 * It's suitable for docking and such, but not very quick way to reach the destination.
 */
void Warpable::steerArrival(double dt, const Vec3d &atarget, const Vec3d &targetvelo, double speedfactor, double minspeed){
	Vec3d target(atarget);
	Vec3d rdr = target - this->pos; // real dr
	Vec3d rdrn = rdr.norm();
	Vec3d dv = targetvelo - this->velo;
	Vec3d dvLinear = rdrn.sp(dv) * rdrn;
	Vec3d dvPlanar = dv - dvLinear;
	double dist = rdr.len();
	double dvLinearLen = dvLinear.len();
	if(rdrn.sp(dv) < 0 && 0 < dvLinearLen) // estimate only when closing
		target += dvPlanar * dist / dvLinearLen * .1;
	Vec3d dr = this->pos - target;
	if(rot.trans(-vec3_001).sp(dr) < 0) // burst only when heading closer
		this->inputs.press |= PL_W;
//	this->throttle = dr.len() * speedfactor + minspeed;
	this->omg = 3 * this->rot.trans(vec3_001).vp(dr.norm());
	const maneuve &mn = getManeuve();
	if(mn.maxanglespeed * mn.maxanglespeed < this->omg.slen())
		this->omg.normin().scalein(mn.maxanglespeed);
//	this->rot = this->rot.quatrotquat(this->omg * dt);
	if(bbody)
		bbody->setAngularVelocity(btvc(this->omg));
//	dr.normin();
//	Vec3d sidevelo = velo - dr * dr.sp(velo);
//	bbody->applyCentralForce(btvc(-sidevelo * mass));
}




Warpable::Warpable(WarField *aw) : st(aw), task(sship_idle){
	w = aw;
	Warpable::init();
}

void Warpable::init(){
	st::init();
	warpSpeed = /*1e6 * LIGHTYEAR_PER_KILOMETER */15000. * AU_PER_KILOMETER;
	warping = 0;
//	warp_next_warf = NULL;
	capacitor = 0.;
	inputs.change = 0;
}

// transit to a CoordSys from another, keeping absolute position and velocity.
int cmd_transit(int argc, char *argv[], void *pv){
	Player *ppl = (Player*)pv;
	Entity *pt;
	if(argc < 2){
		CmdPrintf("Usage: transit dest");
		return 1;
	}
	for(Player::SelectSet::iterator pt = ppl->selected.begin(); pt != ppl->selected.end(); pt++){
		Vec3d pos;
		CoordSys *pcs;
		if(pcs = const_cast<CoordSys*>(ppl->cs)->findcspath(argv[1])){
			(*pt)->transit_cs(pcs);
		}
	}
	return 0;
}

// This block has no effect in dedicated server.
#ifndef DEDICATED
static int cmd_warp(int argc, char *argv[], void *pv){
	ClientGame *game = (ClientGame*)pv;
	Player *ppl = game->player;
	Entity *pt;
	if(argc < 2){
		CmdPrintf("Usage: warp dest [x] [y] [z]");
		return 1;
	}
	WarpCommand com;
	com.destpos = Vec3d(2 < argc ? atof(argv[2]) : 0., 3 < argc ? atof(argv[3]) : 0., 4 < argc ? atof(argv[2]) : 0.);

	// Search path is based on player's CoordSys, though this function would be soon unnecessary anyway.
	teleport *tp = ppl->findTeleport(argv[1], TELEPORT_WARP);
	if(tp){
		com.destcs = tp->cs;
		com.destpos = tp->pos;
	}
	else if(com.destcs = const_cast<CoordSys*>(ppl->cs->findcs(argv[1]))){
		com.destpos = vec3_000;
	} 
	else
		return 1;

	for(Player::SelectSet::iterator pt = ppl->selected.begin(); pt != ppl->selected.end(); pt++){
		(*pt)->command(&com);
		CMEntityCommand::s.send(*pt, com);
#if 0
		Warpable *p = (*pt)->toWarpable();
		if(!p)
			continue;
		WarField *w = p->w;
		if(!p->warping){
			Vec3d delta, pos;
			const CoordSys *pa = NULL;
			CoordSys *pcs;
			double landrad;
			double dist, cost;
			extern coordsys *g_galaxysystem;
			Vec3d dstpos = vec3_000;
			teleport *tp = Player::findTeleport(argv[1], TELEPORT_WARP);
			if(tp){
				pcs = tp->cs;
				pos = tp->pos;
				delta = w->cs->tocs(pos, pcs);
				VECSUBIN(delta, pt->pos);
				VECCPY(dstpos, pos);
				pa = pcs;
			}
			else if(pa = ppl->cs->findcs(argv[1])){
				dstpos[0] = argc < 3 ? 0. : atof(argv[2]);
				dstpos[1] = argc < 4 ? 0. : atof(argv[3]);
				dstpos[2] = argc < 5 ? 0. : atof(argv[4]);
				delta = w->cs->tocs(dstpos, pa);
				VECSUBIN(delta, pt->pos);
			} 
			else
				continue;
			dist = VECLEN(delta);
			cost = g_warp_cost_factor * pt->mass / 1e3 * (log10(dist + 1.) + 1.);
			if(cost < p->capacitor){
				double f;
				int i;
				p->warping = 1;
				p->task = sship_warp;
				p->capacitor -= cost;
	/*			f = VECLEN(delta);
				f = (f - pa->rad * 1.1) / f;
				VECNULL(p->warpdst);
				VECSCALE(p->warpdst, delta, f);
				VECADDIN(p->warpdst, pt->pos);*/
				VECCPY(p->warpdst, dstpos);
				for(i = 0; i < 3; i++)
					p->warpdst[i] += 2. * (drseq(&p->w->rs) - .5);
				p->totalWarpDist = dist;
				p->currentWarpDist = 0.;
				p->warpcs = NULL;
				p->warpdstcs = const_cast<CoordSys*>(pa);
//				p->warp_next_warf = NULL;
			}
		}
#endif
	}
	return 0;
}

static void register_Warpable_cmd(ClientGame &game){
	CmdAddParam("warp", cmd_warp, &game);
}

static void register_Warpable(){
	Game::addClientInits(register_Warpable_cmd);
}

static StaticInitializer sss(register_Warpable);
#endif


static int enum_cs_flags(const CoordSys *root, int mask, int flags, const CoordSys ***ret, int left){
	const CoordSys *cs;
	if((root->flags & mask) == (flags & mask)){
		*(*ret)++ = root;
		if(!--left)
			return 0;
	}
	for(cs = root->children; cs; cs = cs->next) if(!(left = enum_cs_flags(cs, mask, flags, ret, left)))
		return left;
	return left;
}




Warpable *Warpable::toWarpable(){
	return this;
}

Entity::Props Warpable::props()const{
	Props ret = st::props();
	ret.push_back(gltestp::dstring("Capacitor: ") << capacitor << '/' << maxenergy());
	ret.push_back(gltestp::dstring("task: ") << task);
	return ret;
}

void Warpable::control(input_t *inputs, double dt){
	Warpable *p = this;

	/* camera distance is always configurable. */
/*	if(inputs->press & PL_MWU){
		g_viewdist *= VIEWDIST_FACTOR;
	}
	if(inputs->press & PL_MWD){
		g_viewdist /= VIEWDIST_FACTOR;
	}*/

	if(!w || health <= 0.)
		return;
	this->inputs = *inputs;

/*	if(inputs->change & inputs->press & PL_G)
		p->menu = !p->menu;*/

#if 0
	if(p->menu && !p->warping){
		extern coordsys *g_sun_low_orbit, *g_iserlohnsystem, *g_rwport, *g_earthorbit, *g_lagrange1, *g_moon_low_orbit, *g_jupiter_low_orbit, *g_saturn_low_orbit;
		double landrad;
		avec3_t dstpos = {.0};
		coordsys *pa = NULL;
		if(0);
/*		else if(inputs->change & inputs->press & PL_D){
			pa = g_iserlohnsystem;
			dstpos[0] = +75.;
		}*/
		else if(inputs->change & inputs->press & PL_B){
			pa = g_rwport;
		}
		else if(inputs->change & inputs->press & PL_A){
			pa = g_earthorbit;
			dstpos[1] = 1.;
		}
		else if(inputs->change & inputs->press & PL_S){
			pa = g_lagrange1;
			dstpos[0] = +15.;
			dstpos[1] = -22.;
		}
		else if(inputs->change & inputs->press & PL_D){
			pa = g_moon_low_orbit;
		}
		else if(inputs->change & inputs->press & PL_W){
			pa = g_jupiter_low_orbit;
		}
		else if(inputs->change & inputs->press & PL_Z){
			pa = g_saturn_low_orbit;
		}
		else if(inputs->change & inputs->press & PL_Q){
			pa = g_sun_low_orbit;
		}
		if(pa){
			avec3_t delta;
			double f;
			int i;
			p->warping = 1;
			tocs(delta, w->cs, pa->pos, pa);
			VECSUBIN(delta, pt->pos);
			f = VECLEN(delta);
			f = (f - pa->rad * 1.1) / f;
/*			VECNULL(p->warpdst);
			VECSCALE(p->warpdst, delta, f);
			VECADDIN(p->warpdst, pt->pos);*/
			VECCPY(p->warpdst, dstpos);
			for(i = 0; i < 3; i++)
				p->warpdst[i] += 2. * (drseq(&w->rs) - .5);
			p->menu = 0;
			p->warpcs = NULL;
			p->warpdstcs = pa;
			p->warp_next_warf = NULL;
		}
	}
#endif
}

unsigned Warpable::analog_mask(){
	return 0;
}


void Warpable::warp_collapse(){
	int i;
	Warpable *p = this;
	Entity *pt2;
	if(!warpcs)
		return;
	WarField *w = this->w; // Entity::w could be altered in transit_cs, so we reserve it here.
	for(WarField::EntityList::iterator it = w->el.begin(); it != w->el.end();){
		WarField::EntityList::iterator next = it;
		++next;
		if(*it){
			(*it)->transit_cs(p->warpdstcs);
		}
		it = next;
	}
	if(p->warpcs){
/*		for(i = 0; i < npf; i++){
			if(pf[i])
				ImmobilizeTefpol3D(pf[i]);
			pf[i] = AddTefpolMovable3D(p->warpdstcs->w->tepl, p->st.pos, pt->velo, avec3_000, &cs_orangeburn, TEP3_THICKEST | TEP3_ROUGH, cs_orangeburn.t);
		}*/
		p->warpcs->flags |= CS_DELETE;
		p->warpcs = NULL;
	}
}

const Warpable::maneuve Warpable::mymn = {
	0, // double accel;
	0, // double maxspeed;
	0, // double angleaccel;
	0, // double maxanglespeed;
	0, // double capacity; /* capacity of capacitor [MJ] */
	0, // double capacitor_gen; /* generated energy [MW] */
};
const Warpable::maneuve &Warpable::getManeuve()const{
	return mymn;
}
bool Warpable::isTargettable()const{
	return true;
}
bool Warpable::isSelectable()const{return true;}

void Warpable::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << capacitor; /* Temporarily stored energy, MegaJoules */
	sc.o << dest;
	sc.o << warping;
	if(warping){
		sc.o << warpdst;
		sc.o << warpSpeed;
		sc.o << totalWarpDist << currentWarpDist;
		sc.o << warpcs << warpdstcs;
	}
	sc.o << task;
}

void Warpable::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> capacitor; /* Temporarily stored energy, MegaJoules */
	sc.i >> dest;
	sc.i >> warping;
	if(warping){
		sc.i >> warpdst;
		sc.i >> warpSpeed;
		sc.i >> totalWarpDist >> currentWarpDist;
		sc.i >> warpcs >> warpdstcs;
	}
	sc.i >> (int&)task;
}

void Warpable::anim(double dt){
	WarSpace *ws = *w;
	if(!ws){
		st::anim(dt);
		return;
	}
	Mat4d mat;
	transform(mat);
	Warpable *const p = this;
	Entity *pt = this;
	const maneuve *mn = &getManeuve();

	if(p->warping){

		// Do not interact with normal space objects, for it can push things really fast away.
		if(bbody){
			btBroadphaseProxy *proxy = bbody->getBroadphaseProxy();
			if(proxy)
				proxy->m_collisionFilterMask = 0;
		}

		double desiredvelo, velo;
		Vec3d *pvelo = p->warpcs ? &p->warpcs->velo : &pt->velo;
		Vec3d dstcspos, warpdst; /* current position measured in destination coordinate system */
		double sp, scale;
		dstcspos = p->warpdstcs->tocs(pt->pos, w->cs);
		warpdst = w->cs->tocs(p->warpdst, p->warpdstcs);
		{
			Quatd qc;
			Vec3d omega, dv, forward;
			dv = warpdst - pt->pos;
			dv.normin();
			forward = pt->rot.trans(avec3_001);
			forward *= -1.;
			sp = dv.sp(forward);
			omega = dv.vp(forward);
			if(sp < 0.){
				omega += mat.vec3(0) * mn->angleaccel;
			}
			scale = -.5 * dt;
			if(scale < -1.)
				scale = -1.;
			omega *= scale;
			pt->rot = pt->rot.quatrotquat(omega);
		}
		velo = (*pvelo).len();
		desiredvelo = .5 * VECDIST(warpdst, dstcspos);
		desiredvelo = MIN(desiredvelo, p->warpSpeed);
/*		desiredvelo = desiredvelo < 5. ? desiredvelo * desiredvelo / 5. : desiredvelo;*/
/*		desiredvelo = MIN(desiredvelo, 1.47099e8);*/
		if((warpdst - dstcspos).slen() < 1. * 1.){
			warp_collapse();

			// Restore normal collision filter mask.
			if(bbody)
				bbody->getBroadphaseProxy()->m_collisionFilterMask = getDefaultCollisionMask();

			p->warping = 0;
			task = sship_idle;
			pt->velo.clear();
			if(game->player->chase == pt){
				game->player->setvelo(game->player->cs->tocsv(pt->velo, pt->pos, w->cs));
			}
			post_warp();
		}
		else if(desiredvelo < velo){
			Vec3d delta, dst, dstvelo;
			double dstspd, u, len;
			delta = p->warpdst - dstcspos;
/*			VECSUB(delta, warpdst, pt->pos);*/
			len = delta.len();
#if 1
			u = desiredvelo / len;
			dstvelo = delta * u;
			*pvelo = dstvelo;
#else
			u = -exp(-desiredvelo * dt);
			VECSCALE(dst, delta, u);
			VECADDIN(dst, warpdst);
			VECSUB(*pvelo, dst, pt->pos);
			VECSCALEIN(*pvelo, 1. / dt);
#endif
			if(p->warpcs){
				p->warpcs->adopt_child(p->warpdstcs);
			}
		}
		else if(.9 < sp){
			double dstspd, u, len;
			const double L = LIGHT_SPEED;
			Vec3d delta = warpdst - pt->pos;
			len = delta.len();
			u = (velo + .5) * 1e1 /** 5e-1 * (len - p->sight->rad)*/;
	/*		u = L - L * L / (u + L);*/
	/*		dstspd = (u + v) / (1 + u * v / LIGHT_SPEED / LIGHT_SPEED);*/
			delta.normin();
			Vec3d dstvelo = delta * u;
			dstvelo -= *pvelo;
			*pvelo += dstvelo * (.2 * dt);
	/*		VECSUB(delta, dstvelo, p->velo);
			VECCPY(p->velo, dstvelo);*/
	/*		VECSADD(p->velo, delta, dt * 1e-8 * (1e0 + VECLEN(p->velo)));*/
		}

		// A new warp bubble can only be created in the server. If the client could, we couldn't match two simultaneously
		// created warp bubbles in the server and the client.
		if(game->isServer() && !p->warpcs && w->cs != p->warpdstcs && w->cs->csrad * w->cs->csrad < pt->pos.slen()){
			p->warpcs = new WarpBubble(cpplib::dstring("warpbubble") << WarpBubble::serialNo++, w->cs);
			p->warpcs->pos = pt->pos;
			p->warpcs->velo = pt->velo;
			p->warpcs->csrad = pt->hitradius() * 10.;
			p->warpcs->flags = 0;
			p->warpcs->w = new WarSpace(p->warpcs);
			p->warpcs->w->pl = w->pl;
/*			for(i = 0; i < npf; i++) if(pf[i]){
				ImmobilizeTefpol3D(pf[i]);
				pf[i] = NULL;
			}*/
			transit_cs(p->warpcs);
#if 0
			/* TODO: bring docked objects along */
			if(pt->vft == &scarry_s){
				entity_t **ppt2;
				for(ppt2 = &w->tl; *ppt2; ppt2 = &(*ppt2)->next){
					entity_t *pt2 = *ppt2;
					if(pt2->vft == &scepter_s && ((scepter_t*)pt2)->mother == p){
						if(((scepter_t*)*ppt2)->docked)
							transit_cs(pt2, w, p->warpcs);
						else
							((scepter_t*)*ppt2)->mother = NULL;
/*						if(((scepter_t*)*ppt2)->docked){
							transit_cs
							*ppt2 = pt2->next;
							pt2->next = p->warpcs->w->tl;
							p->warpcs->w->tl = pt2;
						}*/
					}
				}
			}
#endif
		}
		else{
			if(w->cs == p->warpcs && dstcspos.slen() < p->warpdstcs->csrad * p->warpdstcs->csrad)
				warp_collapse();
		}

		if(p->warpcs){
			p->currentWarpDist += VECLEN(p->warpcs->velo) * dt;
			p->warpcs->pos += p->warpcs->velo * dt;
		}
		pos += this->velo * dt;
		Entity::setPosition(&pos, &rot, &this->velo, &omg);
	}
	else{ /* Regenerate energy in capacitor only unless warping */

		if(bbody){
			pos = btvc(bbody->getCenterOfMassPosition());
			velo = btvc(bbody->getLinearVelocity());
			rot = btqc(bbody->getOrientation());
			omg = btvc(bbody->getAngularVelocity());
		}

		double gen = dt * g_capacitor_gen_factor * mn->capacitor_gen;
		if(p->capacitor + gen < mn->capacity)
			p->capacitor += gen;
		else
			p->capacitor = mn->capacity;

		if(controller)
			;
		else if(task == sship_idle)
			inputs.press = 0;
		else if(task == sship_moveto){
			steerArrival(dt, dest, vec3_000, .1, .01);
			if((pos - dest).slen() < hitradius() * hitradius()){
				task = sship_idle;
				inputs.press = 0;
			}
		}

		if(0. < dt)
			maneuver(mat, dt, mn);

		if(!bbody){
			pos += velo * dt;
			rot = rot.quatrotquat(omg * dt);
		}
	/*	VECSCALEIN(pt->omg, 1. / (dt * .4 + 1.));
		VECSCALEIN(pt->velo, 1. / (dt * .01 + 1.));*/
	}
	st::anim(dt);
}

bool Warpable::command(EntityCommand *com){
	if(InterpretCommand<HaltCommand>(com)){
		task = sship_idle;
		inputs.press = 0;
		int narms = armsCount();
		for(int i = 0; i < narms; i++){
			ArmBase *arm = armsGet(i);
			if(arm)
				arm->command(com);
		}
		return true;
	}
	else if(MoveCommand *mc = InterpretCommand<MoveCommand>(com)){
		task = sship_moveto;
		dest = mc->destpos;
		return true;
	}
	else if(AttackCommand *ac = InterpretDerivedCommand<AttackCommand>(com)){
		if(!ac->ents.empty())
			enemy = *ac->ents.begin();
		int narms = armsCount();
		for(int i = 0; i < narms; i++){
			ArmBase *arm = armsGet(i);
			if(arm)
				arm->command(com);
		}
		return true;
	}
	else if(WarpCommand *wc = InterpretCommand<WarpCommand>(com)){
		const double g_warp_cost_factor = .001;
		if(!warping){
			Vec3d delta = w->cs->tocs(wc->destpos, wc->destcs) - this->pos;
			double dist = delta.len();
			double cost = g_warp_cost_factor * this->mass / 1e3 * (log10(dist + 1.) + 1.);
			Warpable *p = this;
			if(cost < p->capacitor){
				double f;
				int i;
				p->warping = 1;
				p->task = sship_warp;
				p->capacitor -= cost;
				p->warpdst = wc->destpos;
				for(i = 0; i < 3; i++)
					p->warpdst[i] += 2. * p->w->rs.nextd();
				p->totalWarpDist = dist;
				p->currentWarpDist = 0.;
				p->warpcs = NULL;
				p->warpdstcs = wc->destcs;
			}
			return true;
		}

		// Cannot respond when warping
		return false;
	}
	else if(RemainDockedCommand *rdc = InterpretCommand<RemainDockedCommand>(com)){
		Docker *docker = getDocker();
		if(docker)
			docker->remainDocked = rdc->enable;
	}
	return false;
}

bool singleObjectRaytest(btRigidBody *bbody, const btVector3& rayFrom,const btVector3& rayTo,btScalar &fraction,btVector3& worldNormal,btVector3& worldHitPoint)
{
	btScalar closestHitResults = 1.f;

	btCollisionWorld::ClosestRayResultCallback resultCallback(rayFrom,rayTo);

	bool hasHit = false;
	btConvexCast::CastResult rayResult;
//	btSphereShape pointShape(0.0f);
	btTransform rayFromTrans;
	btTransform rayToTrans;

	rayFromTrans.setIdentity();
	rayFromTrans.setOrigin(rayFrom);
	rayToTrans.setIdentity();
	rayToTrans.setOrigin(rayTo);

	//do some culling, ray versus aabb
	btVector3 aabbMin,aabbMax;
	bbody->getCollisionShape()->getAabb(bbody->getWorldTransform(),aabbMin,aabbMax);
	btScalar hitLambda = 1.f;
	btVector3 hitNormal;
	btCollisionObject	tmpObj;
	tmpObj.setWorldTransform(bbody->getWorldTransform());


//	if (btRayAabb(rayFrom,rayTo,aabbMin,aabbMax,hitLambda,hitNormal))
	{
		//reset previous result

		btCollisionWorld::rayTestSingle(rayFromTrans,rayToTrans, &tmpObj, bbody->getCollisionShape(), bbody->getWorldTransform(), resultCallback);
		if (resultCallback.hasHit())
		{
			//float fog = 1.f - 0.1f * rayResult.m_fraction;
			resultCallback.m_hitNormalWorld.normalize();//.m_normal.normalize();
			worldNormal = resultCallback.m_hitNormalWorld;
			//worldNormal = transforms[s].getBasis() *rayResult.m_normal;
			worldNormal.normalize();
			worldHitPoint = resultCallback.m_hitPointWorld;
			hasHit = true;
			fraction = resultCallback.m_closestHitFraction;
		}
	}

	return hasHit;
}


int Warpable::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	if(bbody){
		btScalar btfraction;
		btVector3 btnormal, btpos;
		btVector3 from = btvc(src);
		btVector3 to = btvc(src + (dir - velo) * dt);
		if(WarSpace *ws = *w){
			btCollisionWorld::ClosestRayResultCallback callback(from, to);
			ws->bdw->rayTest(from, to, callback);
			if(callback.hasHit() && callback.m_collisionObject == bbody){
				if(ret) *ret = callback.m_closestHitFraction * dt;
				if(retp) *retp = btvc(callback.m_hitPointWorld);
				if(retn) *retn = btvc(callback.m_hitNormalWorld);
				return 1;
			}
		}
		else if(singleObjectRaytest(bbody, from, to, btfraction, btnormal, btpos)){
			if(ret) *ret = btfraction * dt;
			if(retp) *retp = btvc(btpos);
			if(retn) *retn = btvc(btnormal);
			return 1;
		}
	}
	return 0/*st::tracehit(src, dir, rad, dt, ret, retp, retn)*/;
}

void Warpable::post_warp(){
}

/// \brief The function that is called to initialize static customizable variables to a specific Entity class.
///
/// Be aware that this function can be called in both the client game and the server game.
/// It depends on initialization order. Specifically, dedicated server always invoke in the server game,
/// while the Windows client invokes in the client game if it's connected to a server.
/// As for a standalone server, it's not determined.
bool Warpable::sq_init(const SQChar *scriptFile, const SqInitProcess &procs){
	try{
		HSQUIRRELVM v = game->sqvm;
		StackReserver sr(v);
		timemeas_t tm;
		TimeMeasStart(&tm);
		sq_newtable(v);
		sq_pushroottable(v); // root
		sq_setdelegate(v, -2);
		if(SQ_SUCCEEDED(sqa_dofile(game->sqvm, scriptFile, 0, 1))){
			const SqInitProcess *proc = &procs;
			for(; proc; proc = proc->next)
				proc->process(v);
		}
		double d = TimeMeasLap(&tm);
		CmdPrint(gltestp::dstring() << scriptFile << " total: " << d << " sec");
	}
	catch(SQFError &e){
		// TODO: We wanted to know which line caused the error, but it seems it's not possible because the errors
		// occur in the deferred loading callback objects.
/*		SQStackInfos si0;
		if(SQ_SUCCEEDED(sq_stackinfos(game->sqvm, 0, &si0))){
			SQStackInfos si;
			for(int i = 1; SQ_SUCCEEDED(sq_stackinfos(game->sqvm, i, &si)); i++)
				si0 = si;
			CmdPrint(gltestp::dstring() << scriptFile << "(" << int(si0.line) << "): " << si0.funcname << ": error: " << e.what());
		}
		else*/
			CmdPrint(gltestp::dstring() << scriptFile << " error: " << e.what());
	}
	return true;
}

/// ModelScale is mandatory if specified by the caller.
void Warpable::ModelScaleProcess::process(HSQUIRRELVM v)const{
	sq_pushstring(v, _SC("modelScale"), -1); // root string
	if(SQ_FAILED(sq_get(v, -2)))
		throw SQFError(_SC("modelScale not found"));
	SQFloat f;
	if(SQ_FAILED(sq_getfloat(v, -1, &f)))
		throw SQFError(_SC("modelScale couldn't be converted to float"));
	modelScale = f;
	sq_poptop(v);
}

void Warpable::HitboxProcess::process(HSQUIRRELVM v)const{
	sq_pushstring(v, _SC("hitbox"), -1); // root string

	// Not defining a hitbox is probably not intended, since any rigid body must have a spatial body.
	if(SQ_FAILED(sq_get(v, -2))) // root obj
		throw SQFError(_SC("hitbox not found"));
	SQInteger len = sq_getsize(v, -1);
	if(-1 == len)
		throw SQFError(_SC("hitbox size could not be aquired"));
	for(int i = 0; i < len; i++){
		sq_pushinteger(v, i); // root obj i
		if(SQ_FAILED(sq_get(v, -2))) // root obj obj[i]
			throw SQFError(gltestp::dstring("hitbox.len[") << i << "] get failed");
		Vec3d org;
		Quatd rot;
		Vec3d sc;

		{
			sq_pushinteger(v, 0); // root obj obj[i] 0
			if(SQ_FAILED(sq_get(v, -2))) // root obj obj[i] obj[i][0]
				throw SQFError(gltestp::dstring("hitbox.len[") << i << "][0] get failed");
			SQVec3d r;
			r.getValue(v, -1);
			org = r.value;
			sq_poptop(v); // root obj obj[i]
		}

		{
			sq_pushinteger(v, 1); // root obj obj[i] 1
			if(SQ_FAILED(sq_get(v, -2))) // root obj obj[i][1]
				throw SQFError(gltestp::dstring("hitbox.len[") << i << "][1] get failed");
			SQQuatd r;
			r.getValue(v, -1);
			rot = r.value;
			sq_poptop(v); // root obj obj[i]
		}
		{
			sq_pushinteger(v, 2); // root obj obj[i] 2
			if(SQ_FAILED(sq_get(v, -2))) // root obj obj[i] obj[i][2]
				throw SQFError(gltestp::dstring("hitbox.len[") << i << "][2] get failed");
			SQVec3d r;
			r.getValue(v, -1);
			sc = r.value;
			sq_poptop(v); // root obj obj[i]
		}
		hitboxes.push_back(hitbox(org, rot, sc));
		sq_poptop(v); // root obj
	}
	sq_poptop(v); // root
}

void Warpable::HardPointProcess::process(HSQUIRRELVM v)const{
	Game *game = (Game*)sq_getforeignptr(v);

	// If it's the server, loading the settings from the local file is meaningless,
	// because they will be sent from the server.
	// Loading them in client does not hurt more than some little memory leaks,
	// but we clearly state here that the local settings won't ever used.
	if(!game->isServer())
		return;

	sq_pushstring(v, _SC("hardpoints"), -1); // root string

	// Not defining hardpoints is valid. Just ignore the case.
	if(SQ_FAILED(sq_get(v, -2))) // root obj
		return;
	SQInteger len = sq_getsize(v, -1);
	if(-1 == len)
		throw SQFError(_SC("hardpoints size could not be aquired"));
	for(int i = 0; i < len; i++){
		sq_pushinteger(v, i); // root obj i
		if(SQ_FAILED(sq_get(v, -2))) // root obj obj[i]
			continue;
		hardpoint_static *np = new hardpoint_static(game);
		hardpoint_static &n(*np);

		sq_pushstring(v, _SC("pos"), -1); // root obj obj[i] "pos"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].pos
			SQVec3d r;
			r.getValue(v, -1);
			n.pos = r.value;
			sq_poptop(v); // root obj obj[i]
		}
		else // Don't take it serously when the item is undefined, just assign the default.
			n.pos = Vec3d(0,0,0);

		sq_pushstring(v, _SC("rot"), -1); // root obj obj[i] "rot"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].rot
			SQQuatd r;
			r.getValue(v);
			n.rot = r.value;
			sq_poptop(v); // root obj obj[i]
		}
		else // Don't take it serously when the item is undefined, just assign the default.
			n.rot = Quatd(0,0,0,1);

		sq_pushstring(v, _SC("name"), -1); // root obj obj[i] "name"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].name
			const SQChar *sqstr;
			if(SQ_SUCCEEDED(sq_getstring(v, -1, &sqstr))){
				char *name = new char[strlen(sqstr) + 1];
				strcpy(name, sqstr);
				n.name = name;
			}
			else // Throw an error because there's no such thing like default name.
				throw SQFError("HardPointsProcess: name is not specified");
			sq_poptop(v); // root obj obj[i]
		}
		else // Throw an error because there's no such thing like default name.
			throw SQFError("HardPointsProcess: name is not specified");

		sq_pushstring(v, _SC("flagmask"), -1); // root obj obj[i] "flagmask"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].flagmask
			SQInteger i;
			if(SQ_SUCCEEDED(sq_getinteger(v, -1, &i)))
				n.flagmask = i;
			else
				n.flagmask = 0;
			sq_poptop(v); // root obj obj[i]
		}
		else
			n.flagmask = 0;

		hardpoints.push_back(np);
		sq_poptop(v); // root obj
	}
	sq_poptop(v); // root
}

void Warpable::DrawOverlayProcess::process(HSQUIRRELVM v)const{
#ifndef DEDICATED // Do nothing in the server.
	sq_pushstring(v, _SC("drawOverlay"), -1); // root string
	if(SQ_FAILED(sq_get(v, -2))) // root obj
		throw SQFError(_SC("drawOverlay not found"));
	sq_pushroottable(v);
	
	// Compile the function into display list
	glNewList(disp = glGenLists(1), GL_COMPILE);
	SQRESULT res = sq_call(v, 1, 0, SQTrue);
	glEndList();

	if(SQ_FAILED(res))
		throw SQFError(_SC("drawOverlay call error"));
	sq_poptop(v);
#endif
}

void Warpable::NavlightsProcess::process(HSQUIRRELVM v)const{
#ifndef DEDICATED // Do nothing in the server.
	sq_pushstring(v, _SC("navlights"), -1); // root string

	// Not defining Navlights is valid. Just ignore the case.
	if(SQ_FAILED(sq_get(v, -2))) // root obj
		return;
	SQInteger len = sq_getsize(v, -1);
	if(-1 == len)
		throw SQFError(_SC("navlights size could not be aquired"));
	for(int i = 0; i < len; i++){
		sq_pushinteger(v, i); // root obj i
		if(SQ_FAILED(sq_get(v, -2))) // root obj obj[i]
			continue;
		Navlight n;

		sq_pushstring(v, _SC("pos"), -1); // root obj obj[i] "pos"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].pos
			SQVec3d r;
			r.getValue(v, -1);
			n.pos = r.value;
			sq_poptop(v); // root obj obj[i]
		}
		else // Don't take it serously when the item is undefined, just assign default.
			n.pos = Vec3d(0,0,0);

		sq_pushstring(v, _SC("color"), -1); // root obj obj[i] "color"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].color
			for(int j = 0; j < 4; j++){
				static const SQFloat defaultColor[4] = {1., 1., 1., 1.};
				sq_pushinteger(v, j); // root obj obj[i] obj[i].color j
				if(SQ_FAILED(sq_get(v, -2))){ // root obj obj[i] obj[i].color obj[i].color[j]
					n.color[j] = defaultColor[j];
					continue;
				}
				SQFloat f;
				if(SQ_FAILED(sq_getfloat(v, -1, &f)))
					n.color[j] = defaultColor[j];
				else
					n.color[j] = f;
				sq_poptop(v);
			}
			sq_poptop(v); // root obj obj[i]
		}
		else // Don't take it serously when the item is undefined, just assign default.
			n.color = Vec4f(1,0,0,1);

		sq_pushstring(v, _SC("radius"), -1); // root obj obj[i] "radius"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].radius
			SQFloat f;
			if(SQ_SUCCEEDED(sq_getfloat(v, -1, &f)))
				n.radius = f;
			sq_poptop(v); // root obj obj[i]
		}

		sq_pushstring(v, _SC("period"), -1); // root obj obj[i] "period"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].period
			SQFloat f;
			if(SQ_SUCCEEDED(sq_getfloat(v, -1, &f)))
				n.period = f;
			else
				n.period = 1.;
			sq_poptop(v); // root obj obj[i]
		}

		sq_pushstring(v, _SC("phase"), -1); // root obj obj[i] "phase"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].phase
			SQFloat f;
			if(SQ_SUCCEEDED(sq_getfloat(v, -1, &f)))
				n.phase = f;
			sq_poptop(v); // root obj obj[i]
		}

		sq_pushstring(v, _SC("pattern"), -1); // root obj obj[i] "pattern"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].pattern
			const SQChar *sqstr;
			if(SQ_SUCCEEDED(sq_getstring(v, -1, &sqstr))){
				if(!strcmp(sqstr, "Constant"))
					n.pattern = n.Constant;
				else if(!strcmp(sqstr, "Triangle"))
					n.pattern = n.Triangle;
				else if(!strcmp(sqstr, "Step"))
					n.pattern = n.Step;
				else
					CmdPrint(gltestp::dstring("NavlightProcess Warning: pattern not recognized: ") << sqstr);
			}
			sq_poptop(v); // root obj obj[i]
		}

		sq_pushstring(v, _SC("duty"), -1); // root obj obj[i] "duty"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].duty
			SQFloat f;
			if(SQ_SUCCEEDED(sq_getfloat(v, -1, &f)))
				n.duty = f;
			sq_poptop(v); // root obj obj[i]
		}

		navlights.push_back(n);
		sq_poptop(v); // root obj
	}
	sq_poptop(v); // root
#endif
}

#ifdef DEDICATED
double Warpable::Navlight::patternIntensity(double)const{return 0.;}
void Warpable::drawNavlights(WarDraw *wd, const std::vector<Navlight> &navlights, const Mat4d *transmat){}
#endif
