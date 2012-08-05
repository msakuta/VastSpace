/** \file
 * \brief Implementation of Warpable class.
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

Vec3d Warpable::absvelo()const{
	return warping && warpcs ? warpcs->velo : this->velo;
}

double Warpable::warpCostFactor()const{
	return 1.;
}

/** \brief Definition of how to maneuver spaceships, shared among ship classes deriving Warpable.
 *
 * Assumption is that accelerations towards all directions except forward movement
 * is a half the maximum accel. */
void Warpable::maneuver(const Mat4d &mat, double dt, const ManeuverParams *mn){
	if(!warping){
		st::maneuver(mat, dt, mn);
	}
}



Warpable::Warpable(WarField *aw) : st(aw){
	w = aw;
	Warpable::init();
}

void Warpable::init(){
	st::init();
	warpSpeed = /*1e6 * LIGHTYEAR_PER_KILOMETER */15000. * AU_PER_KILOMETER;
	warping = 0;
//	warp_next_warf = NULL;
	capacitor = 0.;
}

// This block has no effect in dedicated server.
#ifndef DEDICATED
static int cmd_warp(int argc, char *argv[], void *pv){
	Game *game = (Game*)pv;
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
	// It's safer to static_cast to Game pointer, in case Game class is not the first superclass
	// in ClientGame's derive list.
	// Which way is better to write this cast, &static_cast<Game&>(game) or static_cast<Game*>(&game) ?
	// They seems equivalent to me and count of the keystrokes are the same.
	CmdAddParam("warp", cmd_warp, static_cast<Game*>(&game));
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
	return ret;
}


void Warpable::warp_collapse(){
	int i;
	Warpable *p = this;
	Entity *pt2;
	if(!warpcs)
		return;
	double sdist = (pos - w->cs->tocs(Vec3d(0,0,0), this->warpcs->parent)).slen();
	double ddist = (pos - w->cs->tocs(warpdst, warpdstcs)).slen();
	CoordSys *newcs = sdist < ddist ? warpcs->parent : (CoordSys*)warpdstcs;
	WarField *w = this->w; // Entity::w could be altered in transit_cs, so we reserve it here.
	for(WarField::EntityList::iterator it = w->el.begin(); it != w->el.end();){
		WarField::EntityList::iterator next = it;
		++next;
		if(*it){
			(*it)->transit_cs(newcs);
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


void Warpable::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << capacitor; /* Temporarily stored energy, MegaJoules */
	sc.o << warping;
	if(warping){
		sc.o << warpdst;
		sc.o << warpSpeed;
		sc.o << totalWarpDist << currentWarpDist;
		sc.o << warpcs << warpdstcs;
	}
}

void Warpable::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> capacitor; /* Temporarily stored energy, MegaJoules */
	sc.i >> warping;
	if(warping){
		sc.i >> warpdst;
		sc.i >> warpSpeed;
		sc.i >> totalWarpDist >> currentWarpDist;
		sc.i >> warpcs >> warpdstcs;
	}
}

void Warpable::enterField(WarField *target){
	WarSpace *ws = *target;

	if(ws && ws->bdw){
		buildBody();
		//add the body to the dynamics world
		ws->bdw->addRigidBody(bbody, bbodyGroup(), bbodyMask());
	}
#if DEBUG_ENTERFIELD
	std::ofstream of("debug.log", std::ios_base::app);
	of << game->universe->global_time << ": enterField: " << (game->isServer()) << " {" << classname() << ":" << id << "} to " << target->cs->getpath() << std::endl;
#endif
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
	const ManeuverParams *mn = &getManeuve();

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
		double sp;
		dstcspos = p->warpdstcs->tocs(pt->pos, w->cs);
		warpdst = w->cs->tocs(p->warpdst, p->warpdstcs);
		{
			Vec3d dv = (warpdst - pt->pos).norm();
			Vec3d forward = -pt->rot.trans(vec3_001);
			sp = dv.sp(forward);
			Vec3d omega = dv.vp(forward);
			if(sp < 0.){
				// Make it continuous function
				omega += mat.vec3(1) * mn->maxanglespeed * (1. - sp);
			}
			double scale = -.5 * dt;
			if(scale < -1.)
				scale = -1.;
			omega *= scale;
#if 0 // This precise calculation seems not necessary.
			Quatd qomg = Quatd::rotation(asin(omega.len()), omega.norm());
			pt->rot = qomg * pt->rot;
#else
			pt->rot = pt->rot.quatrotquat(omega);
#endif
			pt->rot.normin();
		}

		velo = (*pvelo).len();
		desiredvelo = .5 * (warpdst - dstcspos).len();
		desiredvelo = MIN(desiredvelo, p->warpSpeed);
/*		desiredvelo = desiredvelo < 5. ? desiredvelo * desiredvelo / 5. : desiredvelo;*/
/*		desiredvelo = MIN(desiredvelo, 1.47099e8);*/
		double sdist = (warpdst - dstcspos).slen();
		if(sdist < 1. * 1. || capacitor <= 0. && velo < 1.){
			if(sdist < warpdstcs->csrad * warpdstcs->csrad)
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
		else if(capacitor <= dt * warpCostFactor()){
			// If the capacitor cannot feed enough energy to keep the warp field active, slow down.
			(*pvelo) *= exp(-dt);
			capacitor = 0.;
		}
		else if(.99 < sp){
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

			// Consume energy
			capacitor -= dt * warpCostFactor();
		}

		// If it's belonging to a warp bubble, adopt it for the warping CoordSys instead of creating a new warpbubble
		// as a child of the warpbubble. This way we can prevent it from creating multiple warpbubbles by repeatedly
		// trying warping and stopping.
		if(!warpcs && !strncmp(w->cs->name, "warpbubble", sizeof"warpbubble"-1))
			warpcs = w->cs;

		// Obtain the source CoordSys of warping.
		CoordSys *srccs = warpcs ? warpcs->parent : w->cs;

		// A new warp bubble can only be created in the server. If the client could, we couldn't match two simultaneously
		// created warp bubbles in the server and the client.
		if(game->isServer() && !p->warpcs && w->cs != p->warpdstcs && srccs->csrad * srccs->csrad < pt->pos.slen()){
			p->warpcs = new WarpBubble(cpplib::dstring("warpbubble") << WarpBubble::serialNo++, w->cs);
			p->warpcs->pos = pt->pos;
			p->warpcs->velo = pt->velo;
			p->warpcs->csrad = pt->getHitRadius() * 10.;
			p->warpcs->flags = 0;
			p->warpcs->w = new WarSpace(p->warpcs);
			p->warpcs->w->pl = w->pl;
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
	/*	VECSCALEIN(pt->omg, 1. / (dt * .4 + 1.));
		VECSCALEIN(pt->velo, 1. / (dt * .01 + 1.));*/
	}
	st::anim(dt);
}

bool Warpable::command(EntityCommand *com){
	if(WarpCommand *wc = InterpretCommand<WarpCommand>(com)){
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
	else
		return st::command(com);
	return false;
}


void Warpable::post_warp(){
}

