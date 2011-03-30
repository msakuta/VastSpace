#include "Warpable.h"
#include "Player.h"
#include "Bullet.h"
#include "CoordSys.h"
#include "Viewer.h"
#include "cmd.h"
#include "glw/glwindow.h"
#include "glw/GLWmenu.h"
#include "judge.h"
#include "astrodef.h"
#include "stellar_file.h"
#include "astro_star.h"
#include "serial_util.h"
#include "glstack.h"
#include "EntityCommand.h"
#include "btadapt.h"
#include "arms.h"
#include "Docker.h"
//#include "sensor.h"
#include "motion.h"
#include "draw/WarDraw.h"
#include "glsl.h"
extern "C"{
#include "bitmap.h"
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/suf/sufbin.h>
#include <clib/suf/sufdraw.h>
#include <clib/GL/gldraw.h>
#include <clib/wavsound.h>
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



int g_healthbar = 1;
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
	WarpBubble(){}
	WarpBubble(const char *path, CoordSys *root) : st(path, root){}
	const char *classname()const{return "WarpBubble";}
	static int serialNo;
};
const ClassRegister<WarpBubble> WarpBubble::classRegister("WarpBubble");
int WarpBubble::serialNo = 0;









#ifdef NDEBUG
#else
void hitbox_draw(const Entity *pt, const double sc[3], int hitflags){
	glPushMatrix();
	glScaled(sc[0], sc[1], sc[2]);
	glPushAttrib(GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_POLYGON_SMOOTH);
	glColor4ub(255,0,0,255);
	glBegin(GL_LINES);
	{
		int i, j, k;
		for(i = 0; i < 3; i++) for(j = -1; j <= 1; j += 2) for(k = -1; k <= 1; k += 2){
			double v[3];
			v[i] = j;
			v[(i+1)%3] = k;
			v[(i+2)%3] = -1.;
			glVertex3dv(v);
			v[(i+2)%3] = 1.;
			glVertex3dv(v);
		}
	}
/*	for(int ix = 0; ix < 2; ix++) for(int iy = 0; iy < 2; iy++) for(int iz = 0; iz < 2; iz++){
		glColor4fv(hitflags & (1 << (ix * 4 + iy * 2 + iz)) ? Vec4<float>(1,0,0,1) : Vec4<float>(0,1,1,1));
		glVertex3dv(vec3_000);
		glVertex3dv(1.2 * Vec3d(ix * 2 - 1, iy * 2 - 1, iz * 2 - 1));
	}*/
	glEnd();
	glPopAttrib();
	glPopMatrix();
}
#endif


void draw_healthbar(Entity *pt, wardraw_t *wd, double v, double scale, double s, double g){
	double x = v * 2. - 1., h = MIN(.1, .1 / (1. + scale)), hs = h / 2.;
	if(!g_healthbar || !Player::g_overlay || wd->shadowmapping)
		return;
	if(g_healthbar == 1 && wd->w && wd->w->pl){
		Entity *pt2;
		for(pt2 = wd->w->pl->selected; pt2; pt2 = pt2->selectnext) if(pt2 == pt){
			break;
		}
		if(!pt2)
			return;
	}
	if(wd->shader)
		glUseProgram(0);
	glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glPushMatrix();
	gldTranslate3dv(pt->pos);
	glMultMatrixd(wd->vw->irot);
	gldScaled(scale);
#if 0 /* signal spectrum drawing */
	if(vft->signalSpectrum){
		double x;
		glColor4ub(0,255,127,255);
		glBegin(GL_LINES);
		for(x = -10; x < 10; x++){
			glVertex2d(x / 10., -1);
			glVertex2d(x / 10., -.95);
		}
		glEnd();
		glBegin(GL_LINE_STRIP);
		for(x = -100; x < 100; x++){
			glVertex2d(x / 100., -1 + vft->signalSpectrum(pt, x / 10.));
		}
		glEnd();
	}
#endif
	glBegin(GL_QUADS);
	glColor4ub(0,255,0,255);
	glVertex3d(-1., 1., 0.);
	glVertex3d( x, 1., 0.);
	glVertex3d( x, 1. - h, 0.);
	glVertex3d(-1., 1. - h, 0.);
	glColor4ub(255,0,0,255);
	glVertex3d( x, 1., 0.);
	glVertex3d( 1., 1., 0.);
	glVertex3d( 1., 1. - h, 0.);
	glVertex3d( x, 1. - h, 0.);
	if(0 <= s){
		x = s * 2. - 1.;
		glColor4ub(0,255,255,255);
		glVertex3d(-1., 1. + hs * 2, 0.);
		glVertex3d( x, 1. + hs * 2, 0.);
		glVertex3d( x, 1. + hs, 0.);
		glVertex3d(-1., 1. + hs, 0.);
		glColor4ub(255,0,127,255);
		glVertex3d( x, 1. + hs * 2, 0.);
		glVertex3d( 1., 1. + hs * 2, 0.);
		glVertex3d( 1., 1. + hs, 0.);
		glVertex3d( x, 1. + hs, 0.);
	}
	glEnd();
	if(0 <= g){
		x = g * 2. - 1.;
		glTranslated(0, -2. * h, 0.);
		glBegin(GL_QUADS);
		glColor4ub(255,255,0,255);
		glVertex3d(-1., 1. - hs, 0.);
		glVertex3d( x, 1. - hs, 0.);
		glVertex3d( x, 1., 0.);
		glVertex3d(-1., 1., 0.);
		glColor4ub(255,0,127,255);
		glVertex3d( x, 1. - hs, 0.);
		glVertex3d( 1., 1. - hs, 0.);
		glVertex3d( 1., 1., 0.);
		glVertex3d( x, 1., 0.);
		glEnd();
	}
/*	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);*/
/*	glBegin(GL_LINE_LOOP);
	glColor4ub(0,255,255,255);
	glVertex3d(-1., 1., 0.);
	glVertex3d( 1, 1., 0.);
	glVertex3d( 1, 1. - h, 0.);
	glVertex3d(-1., 1. - h, 0.);
	glEnd();*/
	glPopMatrix();
	glPopAttrib();
	if(wd->shader)
		glUseProgram(wd->shader);
}


void Warpable::drawtra(wardraw_t *wd){
	if(Player::g_overlay && task == sship_moveto){
		glBegin(GL_LINES);
		glColor4ub(0,0,255,255);
		glVertex3dv(pos);
		glColor4ub(0,255,255,255);
		glVertex3dv(dest);
		glEnd();
	}
}

short Warpable::getDefaultCollisionMask()const{
	return -1;
}

/* maneuvering spaceships, integrated common responses.
 assumption is that accelerations towards all directions except forward movement
 is a half the maximum accel. */
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
					if(btvelo.dot(btdirection) < .0)
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
}

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
	warpSpeed = /*1e6 * LIGHTYEAR_PER_KILOMETER */5. * AU_PER_KILOMETER;
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
	for(pt = ppl->selected; pt; pt = pt->selectnext){
		Vec3d pos;
		CoordSys *pcs;
		if(pcs = const_cast<CoordSys*>(ppl->cs)->findcspath(argv[1])){
			pt->transit_cs(pcs);
		}
	}
	return 0;
}

int cmd_warp(int argc, char *argv[], void *pv){
	Player *ppl = (Player*)pv;
	Entity *pt;
	if(argc < 2){
		CmdPrintf("Usage: warp dest [x] [y] [z]");
		return 1;
	}
	WarpCommand com;
	com.destpos = Vec3d(2 < argc ? atof(argv[2]) : 0., 3 < argc ? atof(argv[3]) : 0., 4 < argc ? atof(argv[2]) : 0.);

	// Search path is based on player's CoordSys, though this function would be soon unnecessary anyway.
	teleport *tp = Player::findTeleport(argv[1], TELEPORT_WARP);
	if(tp){
		com.destcs = tp->cs;
		com.destpos = tp->pos;
	}
	else if(com.destcs = const_cast<CoordSys*>(ppl->cs->findcs(argv[1]))){
		com.destpos = vec3_000;
	} 
	else
		return 1;

	for(pt = ppl->selected; pt; pt = pt->selectnext){
		pt->command(&com);
#if 0
		Warpable *p = pt->toWarpable();
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



int cmd_togglewarpmenu(int argc, char *argv[], void *){
	extern coordsys *g_galaxysystem;
	char *cmds[64]; /* not much of menu items as 64 are able to displayed after all */
	const char *subtitles[64];
	coordsys *reta[64], **retp = reta;
	static const char *windowtitle = "Warp Destination";
	GLwindow *wnd, **ppwnd;
	int left, i;
	ppwnd = GLwindow::findpp(&glwlist, &GLwindow::TitleCmp(windowtitle));
	if(ppwnd){
		glwActivate(ppwnd);
		return 0;
	}
/*	for(ppwnd = &glwlist; *ppwnd; ppwnd = &(*ppwnd)->next) if((*ppwnd)->title == windowtitle){
		glwActivate(ppwnd);
		return 0;
	}*/
/*	for(Player::teleport_iterator it = Player::beginTeleport(), left = 0; it != Player::endTeleport() && left < numof(cmds); it++){
		teleport *tp = Player::getTeleport(it);
		if(!(tp->flags & TELEPORT_WARP))
			continue;
		cmds[left] = (char*)malloc(sizeof "warp \"\"" + strlen(tp->name));
		strcpy(cmds[left], "warp \"");
		strcat(cmds[left], tp->name);
		strcat(cmds[left], "\"");
		subtitles[left] = tp->name;
		left++;
	}*/
	PopupMenu pm;
	for(Player::teleport_iterator it = Player::beginTeleport(); it != Player::endTeleport(); it++){
		teleport *tp = Player::getTeleport(it);
		if(!(tp->flags & TELEPORT_WARP))
			continue;
		pm.append(tp->name, 0, cpplib::dstring("warp \"") << tp->name << '"');
	}
//	wnd = glwMenu(windowtitle, left, subtitles, NULL, cmds, 0);
	wnd = glwMenu(windowtitle, pm, GLW_CLOSE | GLW_COLLAPSABLE);
	glwAppend(wnd);
/*	for(i = 0; i < left; i++){
		free(cmds[i]);
	}*/
/*	left = enum_cs_flags(g_galaxysystem, CS_WARPABLE, CS_WARPABLE, &retp, numof(reta));
	{
		int i;
		for(i = 0; i < numof(cmds) - left; i++){
			cmds[i] = malloc(sizeof "warp \"\"" + strlen(reta[i]->name));
			strcpy(cmds[i], "warp \"");
			strcat(cmds[i], reta[i]->name);
			strcat(cmds[i], "\"");
			subtitles[i] = reta[i]->fullname ? reta[i]->fullname : reta[i]->name;
		}
	}
	wnd = glwMenu(numof(reta) - left, subtitles, NULL, cmds, 0);*/
//	wnd->title = windowtitle;
	return 0;
}

int Warpable::popupMenu(PopupMenu &list){
	int ret = st::popupMenu(list);
	list.appendSeparator().append("Warp to...", 'w', "togglewarpmenu");
	return ret;
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

void Warpable::drawHUD(wardraw_t *wd){
	Warpable *p = this;
/*	base_drawHUD_target(pt, wf, wd, gdraw);*/
	st::drawHUD(wd);
	glPushMatrix();
	glPushAttrib(GL_CURRENT_BIT);
	if(warping){
		Vec3d warpdstpos = w->cs->tocs(warpdst, warpdstcs);
		Vec3d eyedelta = warpdstpos - wd->vw->pos;
		eyedelta.normin();
		glLoadMatrixd(wd->vw->rot);
		glRasterPos3dv(eyedelta);
		gldprintf("warpdst");
	}
	glLoadIdentity();

	{
		GLint vp[4];
		double left, bottom, velo;
		GLmatrix glm;

		{
			int w, h, m, mi;
			glGetIntegerv(GL_VIEWPORT, vp);
			w = vp[2], h = vp[3];
			m = w < h ? h : w;
			mi = w < h ? w : h;
			left = -(double)w / m;
			bottom = -(double)h / m;

			velo = warping && warpcs ? warpcs->velo.len() : this->velo.len();
	//		w->orientation(wf, &ort, pt->pos);
			glRasterPos3d(left + 200. / m, -bottom - 100. / m, -1);
			gldprintf("%lg km/s", velo);
			glRasterPos3d(left + 200. / m, -bottom - 120. / m, -1);
			gldprintf("%lg kt", 1944. * velo);
			glScaled((double)mi / m, (double)mi / m, 1);
		}

		if(p->warping){
			double (*cuts)[2];
			char buf[128];
			Vec3d *pvelo = warpcs ? &warpcs->velo : &this->velo;
			Vec3d dstcspos = warpdstcs->tocs(this->pos, w->cs);/* current position measured in destination coordinate system */
			Vec3d warpdst = w->cs->tocs(p->warpdst, p->warpdstcs);

			double velo = (*pvelo).len();
			Vec3d delta = p->warpdst - dstcspos;
			double dist = delta.len();
			p->totalWarpDist = p->currentWarpDist + dist;
			cuts = CircleCuts(32);
			glTranslated(0, -.65, -1);
			glScaled(.25, .25, 1.);

			double f = log10(velo / p->warpSpeed);
			int iphase = -8 < f ? 8 + f : 0;

			/* speed meter */
			glColor4ub(127,127,127,255);
			glBegin(GL_QUAD_STRIP);
			for(int i = 4; i <= 4 + iphase; i++){
				glVertex2d(-cuts[i][1], cuts[i][0]);
				glVertex2d(-.5 * cuts[i][1], .5 * cuts[i][0]);
			}
			glEnd();

			glColor4ub(255,255,255,255);
			glBegin(GL_LINE_LOOP);
			for(int i = 4; i <= 12; i++){
				glVertex2d(cuts[i][1], cuts[i][0]);
			}
			for(int i = 12; 4 <= i; i--){
				glVertex2d(.5 * cuts[i][1], .5 * cuts[i][0]);
			}
			glEnd();

			glBegin(GL_LINES);
			for(int i = 4; i <= 12; i++){
				glVertex2d(cuts[i][1], cuts[i][0]);
				glVertex2d(.5 * cuts[i][1], .5 * cuts[i][0]);
			}
			glEnd();

			/* progress */
			glColor4ub(127,127,127,255);
			glBegin(GL_QUADS);
			glVertex2d(-1., -.5);
			glVertex2d(p->currentWarpDist / p->totalWarpDist * 2. - 1., -.5);
			glVertex2d(p->currentWarpDist / p->totalWarpDist * 2. - 1., -.7);
			glVertex2d(-1., -.7);
			glEnd();

			glColor4ub(255,255,255,255);
			glBegin(GL_LINE_LOOP);
			glVertex2d(-1., -.5);
			glVertex2d(1., -.5);
			glVertex2d(1., -.7);
			glVertex2d(-1., -.7);
			glEnd();

			glRasterPos2d(- 0. / wd->vw->vp.w, -.4 + 16. * 4 / wd->vw->vp.h);
			gldPutString(cpplib::dstring(p->currentWarpDist / p->totalWarpDist * 100.) << '%');
			if(velo != 0){
				double eta = (p->totalWarpDist - p->currentWarpDist) / p->warpSpeed;
				eta = fabs(eta);
				if(3600 * 24 < eta)
					sprintf(buf, "ETA: %d days %02d:%02d:%02lg", (int)(eta / (3600 * 24)), (int)(fmod(eta, 3600 * 24) / 3600), (int)(fmod(eta, 3600) / 60), fmod(eta, 60));
				else if(60 < eta)
					sprintf(buf, "ETA: %02d:%02d:%02lg", (int)(eta / 3600), (int)(fmod(eta, 3600) / 60), fmod(eta, 60));
				else
					sprintf(buf, "ETA: %lg sec", eta);
				glRasterPos2d(- 8. / wd->vw->vp.w, -.4);
				gldPutString(buf);
			}
		}
	}

	glPopAttrib();
	glPopMatrix();
}

#if 0
static warf_t *warpable_warp_dest(entity_t *pt, const warf_t *w){
	warpable_t *p = ((warpable_t*)pt);
	if(!p->warping)
		return NULL;
	return p->warp_next_warf;
}
#endif


void Warpable::warp_collapse(){
	int i;
	Warpable *p = this;
	Entity *pt2;
	if(!warpcs)
		return;
	for(pt2 = w->el; pt2; pt2 = pt2->next)
		transit_cs(p->warpdstcs);
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
		if(bbody)
			bbody->getBroadphaseProxy()->m_collisionFilterMask = 0;

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
			if(w->pl->chase == pt){
				w->pl->setvelo(w->pl->cs->tocsv(pt->velo, pt->pos, w->cs));
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
		if(!p->warpcs && w->cs != p->warpdstcs && w->cs->csrad * w->cs->csrad < VECSLEN(pt->pos)){
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

		if(w->pl->control == this)
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
