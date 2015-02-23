/** \file
 * \brief Implementation of Autonomous class.
 */
#include "Autonomous.h"
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
#include "EntityCommand.h"
#include "btadapt.h"
#include "sqadapt.h"
#include "arms.h"
#include "Docker.h"
//#include "sensor.h"
#include "motion.h"
#ifndef DEDICATED
#include "draw/WarDraw.h"
#include "glstack.h"
#include "glsl.h"
#endif
#include "dstring.h"
#include "Game.h"
#include "StaticInitializer.h"
extern "C"{
#ifndef DEDICATED
#include "bitmap.h"
#endif
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/suf/sufbin.h>
#include <clib/timemeas.h>
#ifndef DEDICATED
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
#include <fstream>
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


#define DEBUG_ENTERFIELD 1


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
		Teline3DrawData dd;
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










#ifndef _WIN32
void Autonomous::drawtra(wardraw_t *wd){}
void Autonomous::drawHUD(wardraw_t *wd){}
#endif

short Autonomous::getDefaultCollisionMask()const{
	return -1;
}

Vec3d Autonomous::absvelo()const{
	return velo;
}

/** \brief Definition of how to maneuver spaceships, shared among ship classes deriving Autonomous.
 *
 *  */
void Autonomous::maneuver(const Mat4d &mat, double dt, const ManeuverParams *mn){
	Entity *pt = this;
	double const maxspeed2 = mn->maxspeed * mn->maxspeed;
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
			if(omg.slen() < mn->maxanglespeed * mn->maxanglespeed){
				pt->omg += torque * dt;
				if(mn->maxanglespeed * mn->maxanglespeed < omg.slen())
					omg *= mn->maxanglespeed / omg.len();
			}
		}
	}
	else if(bbody){
		// Some entities (namely, GimbalTurret) derive Autonomous just for some reason,
		// but cannot rotate on their own at all.
		if(0 < mn->angleaccel){
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
	}
	else{
		double len = omg.len();
		if(len < dt * mn->angleaccel)
			omg.clear();
		else if(0. < len)
			omg *= (len - dt * mn->angleaccel) / len;
	}

	Vec3d forceAccum(0,0,0);
	if(pt->inputs.press & PL_W){
		forceAccum += mat.vec3(2) * -mn->getAccel(ManeuverParams::NZ);
	}
	if(pt->inputs.press & PL_S){
		forceAccum += mat.vec3(2) * mn->getAccel(ManeuverParams::PZ);
	}
	if(pt->inputs.press & PL_A){
		forceAccum += mat.vec3(0) * -mn->getAccel(ManeuverParams::NX);
	}
	if(pt->inputs.press & PL_D){
		forceAccum += mat.vec3(0) * mn->getAccel(ManeuverParams::PX);
	}
	if(pt->inputs.press & PL_Q){
		forceAccum += mat.vec3(1) * mn->getAccel(ManeuverParams::PY);
	}
	if(pt->inputs.press & PL_Z){
		forceAccum += mat.vec3(1) * -mn->getAccel(ManeuverParams::NY);
	}
	if(pt->inputs.press & (PL_W | PL_S | PL_A | PL_D | PL_Q | PL_Z)){
		if(bbody){
			btVector3 btforceAccum = btvc(forceAccum);
			// It could be zero if arrow keys in opposite directions are pressed
//			assert(!btforceAccum.isZero());
			if(!btforceAccum.isZero()){
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

				// Need activation to those Entites who are in completely stationary before applied force takes effect.
				bbody->activate();
			}
		}
		else if(0 < forceAccum.slen() && pt->velo.sp(forceAccum) / forceAccum.len() < mn->maxspeed){
			pt->velo += forceAccum * dt;
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
	direction = inputs.press;
}

/** \brief A steering behavior of arrival, gradually decreases relative velocity to the destination in the way of approaching.
 *
 * It's suitable for docking and such, but not very quick way to reach the destination.
 */
void Autonomous::steerArrival(double dt, const Vec3d &atarget, const Vec3d &targetvelo, double speedfactor, double minspeed){
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
	const ManeuverParams &mn = getManeuve();
	if(mn.maxanglespeed * mn.maxanglespeed < this->omg.slen())
		this->omg.normin().scalein(mn.maxanglespeed);
//	this->rot = this->rot.quatrotquat(this->omg * dt);
	if(bbody)
		bbody->setAngularVelocity(btvc(this->omg));
//	dr.normin();
//	Vec3d sidevelo = velo - dr * dr.sp(velo);
//	bbody->applyCentralForce(btvc(-sidevelo * mass));
}




Autonomous::Autonomous(WarField *aw) : st(aw), task(sship_idle){
	w = aw;
	Autonomous::init();
}

void Autonomous::init(){
	st::init();
	inputs.change = 0;
}

// transit to a CoordSys from another, keeping absolute position and velocity.
int cmd_transit(int argc, char *argv[], void *pv){
	Game *game = (Game*)pv;
	Player *ppl = game->player;
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

#ifndef DEDICATED
static void register_Autonomous_cmd(ClientGame &game){
	// It's safer to static_cast to Game pointer, in case Game class is not the first superclass
	// in ClientGame's derive list.
	// Which way is better to write this cast, &static_cast<Game&>(game) or static_cast<Game*>(&game) ?
	// They seems equivalent to me and count of the keystrokes are the same.
	CmdAddParam("transit", cmd_transit, static_cast<Game*>(&game));
}

static void register_Autonomous(){
	Game::addClientInits(register_Autonomous_cmd);
}

static StaticInitializer sss(register_Autonomous);
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




Entity::Props Autonomous::props()const{
	Props ret = st::props();
	ret.push_back(gltestp::dstring("task: ") << task);
	return ret;
}

void Autonomous::control(const input_t *inputs, double dt){
	Autonomous *p = this;

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

unsigned Autonomous::analog_mask(){
	return 0;
}


/// Assumption is that accelerations towards all directions except forward movement
/// is a half of the maximum accel.
 double Autonomous::ManeuverParams::getAccel(Direction dir)const{
	if(dir_accel[dir] != 0.)
		return dir_accel[dir];
	else if(dir == NZ)
		return accel;
	else
		return accel * 0.5;
}

const Autonomous::ManeuverParams Autonomous::mymn = {
	0, // double accel;
	0, // double maxspeed;
	0, // double angleaccel;
	0, // double maxanglespeed;
	0, // double capacity; /* capacity of capacitor [MJ] */
	0, // double capacitor_gen; /* generated energy [MW] */
};
const Autonomous::ManeuverParams &Autonomous::getManeuve()const{
	return mymn;
}
bool Autonomous::isTargettable()const{
	return true;
}
bool Autonomous::isSelectable()const{return true;}

void Autonomous::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << dest;
	sc.o << task;
}

void Autonomous::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> dest;
	sc.i >> (int&)task;
}

void Autonomous::addRigidBody(WarSpace *ws){
	if(ws && ws->bdw){
		buildBody();
		//add the body to the dynamics world
		if(bbody)
			ws->bdw->addRigidBody(bbody, bbodyGroup(), bbodyMask());
	}
}

void Autonomous::anim(double dt){
	if(!w)
		return;
	WarSpace *ws = *w;
	if(!ws){
		st::anim(dt);
		return;
	}
	Mat4d mat;
	transform(mat);
	Autonomous *const p = this;
	Entity *pt = this;
	const ManeuverParams *mn = &getManeuve();

	if(bbody){
		pos = btvc(bbody->getCenterOfMassPosition());
		velo = btvc(bbody->getLinearVelocity());
		rot = btqc(bbody->getOrientation());
		omg = btvc(bbody->getAngularVelocity());
	}

	if(controller)
		;
	else if(task == sship_idle)
		inputs.press = 0;
	else if(task == sship_moveto){
		steerArrival(dt, dest, vec3_000, .1, .01);
		if((pos - dest).slen() < getHitRadius() * getHitRadius()){
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

	st::anim(dt);
}

bool Autonomous::command(EntityCommand *com){
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
	else if(RemainDockedCommand *rdc = InterpretCommand<RemainDockedCommand>(com)){
		Docker *docker = getDocker();
		if(docker)
			docker->remainDocked = rdc->enable;
	}
	else if(UndockQueueCommand *uqc = InterpretCommand<UndockQueueCommand>(com)){
		Docker *docker = getDocker();
		if(docker)
			docker->postUndock(uqc->e);
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


int Autonomous::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	// If this ship has a hitbox list, use it to check intersection with the ray.
	if(std::vector<hitbox> *hitboxes = getTraceHitBoxes()){
		double best = dt;
		int reti = 0;
		double retf;
		for(int n = 0; n < hitboxes->size(); n++){
			hitbox &hb = (*hitboxes)[n];
			Vec3d org;
			Quatd rot;
			org = this->rot.trans(hb.org) + this->pos;
			rot = this->rot * hb.rot;
			int i;
			double sc[3];
			for(i = 0; i < 3; i++)
				sc[i] = hb.sc[i] + rad;
			if((jHitBox(org, sc, rot, src, dir, 0., best, &retf, retp, retn)) && (retf < best)){
				best = retf;
				if(ret) *ret = retf;
				reti = i + 1;
			}
		}
		return reti;
	}

	// Otherwise, use Bullet dynamics engine's rayTest.
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

/// \brief Overrides group and mask for broadphase culling.
void ModelEntity::addRigidBody(WarSpace *ws){
	if(ws && ws->bdw){
		buildBody();
		//add the body to the dynamics world
		if(bbody)
			ws->bdw->addRigidBody(bbody, bbodyGroup(), bbodyMask());
	}
}

/// \brief Virtual method to construct a bullet dynamics body for this Warpable.
///
/// By default, it does not build a thing.
bool ModelEntity::buildBody(){
	return false;
}

/// \return Defaults 1
short ModelEntity::bbodyGroup()const{
	return 1;
}

/// \return Defaults all bits raised
short ModelEntity::bbodyMask()const{
	return ~0;
}

/// This function is shared by many Entities with models which consist of simple hitboxes.
bool ModelEntity::buildBodyByHitboxes(const HitBoxList &hitboxes, btCompoundShape *&shape){
	if(!w || bbody)
		return false;
	WarSpace *ws = *w;
	if(ws && ws->bdw){
		if(!shape){
			shape = new btCompoundShape();
			for(int i = 0; i < hitboxes.size(); i++){
				const Vec3d &sc = hitboxes[i].sc;
				const Quatd &rot = hitboxes[i].rot;
				const Vec3d &pos = hitboxes[i].org;
				btBoxShape *box = new btBoxShape(btvc(sc));
				btTransform trans = btTransform(btqc(rot), btvc(pos));
				shape->addChildShape(trans, box);
			}
		}

		btTransform startTransform;
		startTransform.setIdentity();
		startTransform.setOrigin(btvc(pos));

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			shape->calculateLocalInertia(mass,localInertia);

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,shape,localInertia);

		// The space does not have friction whatsoever.
//		rbInfo.m_linearDamping = .5;
//		rbInfo.m_angularDamping = .25;
		bbody = new btRigidBody(rbInfo);

//		bbody->setSleepingThresholds(.0001, .0001);

		//add the body to the dynamics world
//		ws->bdw->addRigidBody(bbody);
		return true;
	}
	return false;
}

/// \brief Gets the hitbox list for trace hit test, instead of Bullet dynamics engine's ray trace with the shape.
/// \return Defaults NULL, which means no hitbox is used and Bullet dynamics engine does the job.
HitBoxList *Autonomous::getTraceHitBoxes()const{
	return NULL;
}

void Autonomous::ManeuverParamsProcess::process(HSQUIRRELVM v)const{
	static const SQChar *paramNames[] = {
		_SC("accel"),
		_SC("maxspeed"),
		_SC("angleaccel"),
		_SC("maxanglespeed"),
		_SC("capacity"),
		_SC("capacitorGen"),
	};
	static double ManeuverParams::*const paramOfs[] = {
		&ManeuverParams::accel,
		&ManeuverParams::maxspeed,
		&ManeuverParams::angleaccel,
		&ManeuverParams::maxanglespeed,
		&ManeuverParams::capacity,
		&ManeuverParams::capacitor_gen,
	};

	for(int i = 0; i < numof(paramNames); i++){
		sq_pushstring(v, paramNames[i], -1); // root string
		if(SQ_FAILED(sq_get(v, -2)))
			continue; // Ignore not found names
		SQFloat f;
		// You can report the error in case the variable cannot be converted.
		// The user is likely to mistaken rather than omitted definition of the variable.
		if(SQ_FAILED(sq_getfloat(v, -1, &f)))
			throw SQFError(gltestp::dstring(paramNames[i]) << _SC(" couldn't be converted to float"));
		mn.*paramOfs[i] = f;
		sq_poptop(v);
	}

	// See if directional acceleration table is defined
	sq_pushstring(v, _SC("dir_accel"), -1);
	if(SQ_FAILED(sq_get(v, -2))){
		for(int i = 0; i < numof(mn.dir_accel); i++)
			mn.dir_accel[i] = 0.; // Make sure to reset the parameters to zeros.
		return; // Not defining a directional acceleration is not an error.
	}
	for(int i = 0; i < numof(mn.dir_accel); i++){
		SQFloat f;
		// Defining a directional accel table and not providing sufficient parameters is an error.
		if(SQ_FAILED(sq_getfloat(v, -1, &f)))
			throw SQFError(gltestp::dstring("dir_accel[") << i << _SC("] couldn't be converted to float"));
		mn.dir_accel[i] = f;
		sq_poptop(v);
	}
}

void Autonomous::EnginePosListProcess::process(HSQUIRRELVM v)const{
	sq_pushstring(v, name, -1); // root string

	if(SQ_FAILED(sq_get(v, -2))) // root obj
		throw SQFError(gltestp::dstring(name) << _SC(" not found"));
	SQInteger len = sq_getsize(v, -1);
	if(-1 == len)
		throw SQFError(gltestp::dstring(name) << _SC(" size could not be aquired"));
	for(int i = 0; i < len; i++){
		sq_pushinteger(v, i); // root obj i
		if(SQ_FAILED(sq_get(v, -2))) // root obj obj[i]
			throw SQFError(gltestp::dstring(name) << "[" << i << "] get failed");
		EnginePos a;

		{
			sq_pushstring(v, _SC("pos"), -1); // root obj obj[i] "pos"
			if(SQ_FAILED(sq_get(v, -2))) // root obj obj[i] obj[i].pos
				throw SQFError(gltestp::dstring(name) << "[" << i << "].pos get failed");
			SQVec3d r;
			r.getValue(v, -1);
			a.pos = r.value;
			sq_poptop(v); // root obj obj[i]
		}

		{
			sq_pushstring(v, _SC("rot"), -1); // root obj obj[i] "rot"
			if(SQ_FAILED(sq_get(v, -2))) // root obj obj[i].rot
				throw SQFError(gltestp::dstring(name) << "[" << i << "].rot get failed");
			SQQuatd r;
			r.getValue(v, -1);
			a.rot = r.value;
			sq_poptop(v); // root obj obj[i]
		}
		vec.push_back(a);
		sq_poptop(v); // root obj
	}
	sq_poptop(v); // root
}

void Autonomous::HitboxProcess::process(HSQUIRRELVM v)const{
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

void ModelEntity::DrawOverlayProcess::process(HSQUIRRELVM v)const{
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

void ModelEntity::NavlightsProcess::process(HSQUIRRELVM v)const{
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
double ModelEntity::Navlight::patternIntensity(double)const{return 0.;}
void ModelEntity::drawNavlights(WarDraw *wd, const NavlightList &navlights, const Mat4d *transmat){}
#endif
