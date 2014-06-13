/** \file
 * \brief Implementation of Warpable class.
 */
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#include "Warpable.h"
#include "cmd.h"
#include "astrodef.h"
#include "astro_star.h"
#include "btadapt.h"
#include "Builder.h"
#ifndef DEDICATED
#include "astrodraw.h"
#endif
#include "StarEnum.h"
#include "StaticInitializer.h"
#include "sqadapt.h"
#include "SqInitProcess-ex.h"
#include "galaxy_field.h"
#include "bitmap.h"
#include "Viewer.h"
extern "C"{
#include <clib/mathdef.h>
#include <clib/timemeas.h>
}
#include <cpplib/CRC32.h>
#include <fstream>
#include <tuple>
#include <functional>
#include <deque>
#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <algorithm>



/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289


#define DEBUG_ENTERFIELD 0


double g_capacitor_gen_factor = 1.;




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
	static bool initialized = false;
	if(!initialized){
		sq_init(_SC("models/Warpable.nut"),
			SqCallbackProcess(sqPopupMenu, _SC("popupMenu")));
		initialized = true;
	}
	warpSpeed = /*1e6 * LIGHTYEAR */15000. * AU_PER_METER;
	warping = 0;
//	warp_next_warf = NULL;
	capacitor = 0.;
}

// This block has no effect in dedicated server.
#ifndef DEDICATED
static int cmd_warp(int argc, char *argv[], void *pv){
	Game *game = (Game*)pv;
	Player *ppl = game->player;
	if(argc < 2){
		CmdPrintf("Usage: warp dest [x] [y] [z]");
		return 1;
	}
	WarpCommand com;
	com.destpos = Vec3d(2 < argc ? atof(argv[2]) : 0., 3 < argc ? atof(argv[3]) : 0., 4 < argc ? atof(argv[2]) : 0.);

	// Search path is based on player's CoordSys, though this function would be soon unnecessary anyway.
	if(com.destcs = const_cast<CoordSys*>(ppl->cs->findcs(argv[1]))){
		com.destpos = vec3_000;
	} 
	else
		return 1;

	for(Player::SelectSet::iterator pt = ppl->selected.begin(); pt != ppl->selected.end(); pt++){
		(*pt)->command(&com);
		CMEntityCommand::s.send(*pt, com);
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
	Warpable *p = this;
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
	of << game->universe->global_time << ": enterField: " << (game->isServer()) << " {" << classname() << ":" << id << "} to " << target->cs->getpath()
		<< " with bbody " << bbody << std::endl;
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
		desiredvelo = .5 * (p->warpdst - dstcspos).len();
		desiredvelo = MIN(desiredvelo, p->warpSpeed);
/*		desiredvelo = desiredvelo < 5. ? desiredvelo * desiredvelo / 5. : desiredvelo;*/
/*		desiredvelo = MIN(desiredvelo, 1.47099e8);*/
		double sdist = (this->warpdst - dstcspos).slen();
		if(p->warpdstcs == game->universe && sdist < 1e10 * 1e10){
			StarEnum se(dstcspos, 0, true);
			Vec3d pos, bestPos;
			double bestDist = 1e100;
			RandomSequence *rs = NULL;
			StarCache *bestsc = NULL, *sc;
			while(se.next(pos, &sc)){
				double dist = (pos - p->warpdst).len();
				if(dist < bestDist){
					bestDist = dist;
					bestPos = pos;
					bestsc = sc;
					rs = se.getRseq();
				}
			}
			if(bestsc){
				CoordSys *cs = bestsc->system;
				if(!cs){
					// Materialize a randomly generated star; create a Star object and associate it with the StarCache.
					// This logic is written in Squirrel scripts.
					try{
						HSQUIRRELVM v = game->sqvm;
						StackReserver sr(v);
						sq_pushroottable(v);
						sq_pushstring(v, _SC("materializeStar"), -1);
						if(SQ_FAILED(sq_get(v, -2)))
							throw SQFError(_SC("Couldn't get materializeStar"));
						sq_pushroottable(v); // .. func root
						sq_pushstring(v, bestsc->name, -1); // .. func root name
						SQVec3d q = bestPos;
						q.push(v); // .. func root name pos
						sq_pushobj(v, this); // .. func root name pos e
						sq_pushstring(v, _SC("RandomSequencePtr"), -1); // .. func root name pos e "RandomSeqeuncePtr"
						if(SQ_FAILED(sq_get(v, -5))) // .. func root name pos e RandomSequencePtr
							throw SQFError(_SC("RandomSequencePtr not defined in root table"));
						sq_createinstance(v, -1); // .. func root name pos e RandomSequencePtr instance
						sq_remove(v, -2); // .. func root name pos e instance

						// TODO: It's dangerous to pass a pointer to automatic variable to Squirrel VM, since it can
						// save the reference to somewhere outside the closure which could be referred to later.
						// Probably we should pass by value, in which case we need another class defined for VM.
						sq_setinstanceup(v, -1, rs);

						if(SQ_FAILED(sq_call(v, 5, SQTrue, SQTrue)))
							throw SQFError(_SC("Couldn't get materializeStar"));
						CoordSys *cs = CoordSys::sq_refobj(v, -1);
						if(cs)
							bestsc->system = cs;
					}
					catch(SQFError &e){
						CmdPrint(e.what());
					}
				}
				else{
					// Reuse the temporary orbit
					OrbitCS *orbit = new OrbitCS("orbit", cs);
					this->warpdst = orbit->tocs(this->warpdst, this->warpdstcs);
					this->warpdstcs = orbit;
				}
				// Recalculate our position in destination CoordSys, because the star materialization can modify
				// our warpdstcs.
				dstcspos = p->warpdstcs->tocs(pt->pos, w->cs);
			}
		}
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
			Vec3d delta = p->warpdst - dstcspos;
			double len = delta.len();
#if 1
			double u = desiredvelo / len;
			Vec3d dstvelo = delta * u;
			*pvelo = dstvelo;
#else
			Vec3d dst;
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
			const double L = LIGHT_SPEED;
			Vec3d delta = warpdst - pt->pos;
			double len = delta.len();
			double u = (velo + .5) * 1e1 /** 5e-1 * (len - p->sight->rad)*/;
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
	else{
		if(Builder *builder = getBuilder()){
			if(builder->command(com))
				return true;
		}
		return st::command(com);
	}
	return false;
}

HSQOBJECT Warpable::sqPopupMenu = sq_nullobj();

HSQOBJECT Warpable::getSqPopupMenu(){
	return sqPopupMenu;
}

SQInteger Warpable::sqGet(HSQUIRRELVM v, const SQChar *name)const{
	if(!scstrcmp(name, _SC("warping"))){
		sq_pushbool(v, warping ? SQTrue : SQFalse);
		return 1;
	}
	else if(!scstrcmp(name, _SC("warpdst"))){
		SQVec3d q(warpdst);
		q.push(v);
		return 1;
	}
	else if(!scstrcmp(name, _SC("warpdstcs"))){
		CoordSys::sq_pushobj(v, warpdstcs);
		return 1;
	}
	else if(!scstrcmp(name, _SC("capacitor"))){
		sq_pushfloat(v, SQFloat(capacitor));
		return 1;
	}
	else if(!scstrcmp(name, _SC("capacity"))){
		sq_pushfloat(v, SQFloat(getManeuve().capacity));
		return 1;
	}
	else if(!scstrcmp(name, _SC("warpSpeed"))){
		sq_pushfloat(v, SQFloat(warpSpeed));
		return 1;
	}
	else
		return st::sqGet(v, name);
}

SQInteger Warpable::sqSet(HSQUIRRELVM v, const SQChar *name){
	if(!scstrcmp(name, _SC("warping"))){
		// Warping flag should not regularly be changed by scripts!
		SQBool b;
		if(SQ_SUCCEEDED(sq_getbool(v, 3, &b)))
			this->warping = b != SQFalse;
		return 0;
	}
	else if(!scstrcmp(name, _SC("warpdst"))){
		SQVec3d q;
		q.getValue(v, 3);
		this->warpdst = q.value;
		return 0;
	}
	else if(!scstrcmp(name, _SC("warpdstcs"))){
		this->warpdstcs = CoordSys::sq_refobj(v, 3);
		return 0;
	}
	else if(!scstrcmp(name, _SC("capacitor"))){
		SQFloat f;
		if(SQ_FAILED(sq_getfloat(v, 3, &f)))
			return sq_throwerror(v, _SC("Warpable.capacitor could not convert to float"));
		this->capacitor = f;
		return 0;
	}
	else if(!scstrcmp(name, _SC("warpSpeed"))){
		SQFloat f;
		if(SQ_FAILED(sq_getfloat(v, 3, &f)))
			return sq_throwerror(v, _SC("Warpable.warpSpeed could not convert to float"));
		this->warpSpeed = f;
		return 0;
	}
	else
		return st::sqSet(v, name);
}


void Warpable::post_warp(){
}



//=============================================================================
// StarEnum implementation
//=============================================================================

#ifdef NDEBUG
double g_star_num = 1.;
#else
double g_star_num = 1.;
#endif

/// This sector size value is controversial.  It cannot be easily changed, but
/// either too low value or too high value can cause problems.
/// If you set it too low, stars won't generate at all (unless we properly
/// implement Poisson distributed pseudo random number generator).
/// If you set it too high, you will get too many stars enumerated for display,
/// causing frame rate drop.
/// 15 ly is trade-off value and generate 2-3 stars per sector around Sol.
const double StarEnum::sectorSize = 15 * LIGHTYEAR;

StarEnum::StarEnum(const Vec3d &plpos, int numSectors, bool genCache) :
	plpos(plpos),
	cen(
		(int)floor(plpos[0] / sectorSize + .5),
		(int)floor(plpos[1] / sectorSize + .5),
		(int)floor(plpos[2] / sectorSize + .5)
	),
	numSectors(numSectors),
	genCache(genCache)
{
	gx = cen[0] - numSectors;
	gy = cen[1] - numSectors;
	gz = cen[2] - numSectors - 1; // Start from the first sector subtracted by 1.
	newCell();
}

StarCacheDB StarEnum::starCacheDB;

struct SylKey{
	char key[2];
	SylKey(const char *key = NULL){
		if(key != NULL){
			this->key[0] = key[0];
			this->key[1] = key[1];
		}
	}
	bool operator<(const SylKey &o)const{
		return this->key[0] != o.key[0] ? this->key[0] < o.key[0] : this->key[1] < o.key[1];
	}
	char operator[](int index)const{
		return key[index];
	}
};

bool StarEnum::newCell(){
	do{
		// Advance indices to the next sector.
		if(gz <= cen[2] + numSectors)
			gz++;
		else if(gy <= cen[1] + numSectors)
			gy++, gz = cen[2] - numSectors;
		else if(gx <= cen[0] + numSectors)
			gx++, gy = cen[1] - numSectors, gz = cen[2] - numSectors;
		else
			return false;

		// Z axis has a length of GALAXY_EXTENT times FIELDZ / FIELD.
		const double volume = GALAXY_EXTENT * GALAXY_EXTENT * (GALAXY_EXTENT * FIELDZ / FIELD);

		// It is speculated that the Galaxy has 100-400 billion stars, but we will ignore red dwarfs, so
		// 100 billion is a reasonable estimate.
		const double totalStars = 100.e9;

		// Stars per cubic kilometers (!).  It should be incredibly small number.
		const double averageStarDensity = totalStars / volume;

		const double starDensityPerSector = averageStarDensity * sectorSize * sectorSize * sectorSize;

		// Start a new random number sequence for this sector.
		int sectors[3] = {gx, gy, gz};
		rs.init(crc32(sectors, sizeof sectors), 0);

		// This value should be Poisson-distributed random value.
		numstars = int(g_star_num * starDensityPerSector * galaxy_get_star_density_pos(Vec3d(gx, gy, gz) * sectorSize) + rs.nextd());
	}while(0 == numstars); // Skip sectors containing no stars

	if(genCache){
		static bool sylsInit = false;
		struct Syllable{
			int beginc;
			int endc;
			char str[3];

			/// Predicate for bseach
			static int cmp(const void *a, const void *b){
				int r = *(int*)a;
				const Syllable *f = (const Syllable*)b;
				if(f->beginc <= r && r < f->endc)
					return 0;
				else
					return r - f->beginc;
			}
		};

		typedef std::map<SylKey, std::vector<Syllable> > SyllableSet;
		static SyllableSet sylsDB;

		static std::vector<Syllable> firstDB;
		static int firstCount = 0;
		if(!sylsInit){
			std::ifstream syls("data/syl.txt");
			int sylcount;
			syls >> sylcount;
			while(!syls.eof()){
				std::string sylstr;
				Syllable syl;
				int count;
				int first;
				syls >> sylstr >> count >> first;
				memcpy(syl.str, sylstr.c_str(), sizeof syl.str);
				SylKey sylkey(sylstr.c_str());
				auto it = sylsDB.find(sylkey);
				if(it == sylsDB.end()){
					syl.beginc = 0;
					syl.endc = count;
				}
				else{
					syl.beginc = sylsDB[sylkey].back().endc;
					syl.endc = syl.beginc + count;
				}
				sylsDB[sylkey].push_back(syl);

				if(first){
					Syllable f;
					f.beginc = firstCount;
					firstCount += first;
					f.endc = firstCount;
					std::copy(sylstr.begin(), sylstr.begin() + 3, f.str);
					firstDB.push_back(f);
				}
			}
			sylsInit = true;
		}

		// This value could be evaluated for more optimized value.
		static const int maxStarCaches = 1000;

		StarCacheKey gkey(gx,gy,gz);
		StarCacheDB::iterator names = starCacheDB.find(gkey);
		// History of cache keys. If it reaches maxStarCaches, the oldest ones gets deleted.
		static std::deque<StarCacheKey> starCacheHistory;
		if(names == starCacheDB.end()){

			// Delete stale StarCaches when we try to add one.
			while(maxStarCaches <= starCacheHistory.size()){
				StarCacheKey staleKey = starCacheHistory.front();

				// Check if this stale sector contains materialized systems
				bool keepCache = false;
				for(auto it : starCacheDB[staleKey]){
					// Skip sectors containing materialized systems
					if(it.system){
						keepCache = true;
						break;
					}
				}
				if(!keepCache)
					starCacheDB.erase(staleKey);

				// Erase history for sectors containing materialized systems to prevent further inspection
				starCacheHistory.pop_front();
			}

			RandomSequence rs = this->rs;
			StarCacheList &newnames = starCacheDB[gkey];
			for(int c = 0; c < numstars; c++){
				gltestp::dstring name;
				do{
					std::string word;
					SylKey next;
					int length = rs.next() % 10 + 1;

					// Penalize 3 character names by rolling the dice again, because 3 character names
					// often collide.
					if(length == 1)
						length = rs.next() % 10 + 1;

					for(int n = 0; n < length; n++){
						// If we could not find the next syllable beginning with given 2 character sequence, stop there
						if(n != 0 && sylsDB.find(next) == sylsDB.end())
							break;
						int sylcount = n == 0 ? firstCount : sylsDB[next].back().endc;

						if(sylcount == 0)
							break;

						int r = rs.next() % sylcount;
						int key = 0;

						auto &syls = n == 0 ? firstDB : sylsDB[next];
						const Syllable *v = (const Syllable*)std::bsearch(&r, &syls.front(), syls.size(), sizeof(syls.front()), Syllable::cmp);
						assert(v);
						next = &v->str[1];
						word += v->str[0];
					}
					name << char(toupper(word[0])) << word.substr(1).c_str() << next[0] << next[1];

					// Regenerate name if there is a collision of name in the sector.
					// Note that we cannot avoid collisions among sectors because their order of
					// creation is unpredictable.
					bool duplicate = false;
					for(auto ename : newnames){
						if(ename.name == name){
							duplicate = true;
							break;
						}
					}
					if(duplicate)
						continue;
				}while(false);

				newnames.push_back(StarCache(name));
			}
			names = starCacheDB.find(gkey);

			// Record added StarCache to the history.
			starCacheHistory.push_back(gkey);
		}

		this->cacheList = &names->second;

		// Begin enumeration for next()
		this->it = names->second.begin();
		this->itend = names->second.end();
	}
	else{
		// Though if genCache is false, we could see if cache was already created.
		std::tuple<int,int,int> gkey(gx,gy,gz);
		StarCacheDB::iterator names = starCacheDB.find(gkey);
		if(names != starCacheDB.end()){
			this->cacheList = &names->second;
			this->it = names->second.begin();
			this->itend = names->second.end();
		}
		else
			this->cacheList = NULL;
	}
	return true;
}

bool StarEnum::next(Vec3d &pos, StarCache **sc){
	if(numstars <= 0){
		if(!newCell())
			return false; // We have gone through all the cells.
	}
	numstars--;

	pos[0] = (drseq(&rs) + gx - 0.5) * sectorSize;
	pos[1] = (drseq(&rs) + gy - 0.5) * sectorSize;
	pos[2] = (drseq(&rs) + gz - 0.5) * sectorSize;

	if(sc != NULL){
		if(cacheList && it != itend){
			*sc = &*it;
			++it; // Advance pointer
		}
		else
			*sc = NULL;
	}
	return true;
}

RandomSequence *StarEnum::getRseq(){
	// Return newly created random sequence instead of the sequence to generate stars in the main loop.
	int buf[4] = {gx, gy, gz, numstars};
	rsStar.init(crc32(buf, sizeof buf), 0);
	return &rsStar;
}

static int cmd_find(int argc, char *argv[]){
	if(argc <= 1){
		for(auto it : StarEnum::starCacheDB){
			for(auto it2 : it.second){
				CmdPrintf("(%d,%d,%d) %s", std::get<0>(it.first), std::get<1>(it.first), std::get<2>(it.first), it2.name.c_str());
			}
		}
		CmdPrint("usage: find star_name");
		return 0;
	}
	int count = 0;
	for(auto it : StarEnum::starCacheDB){
		for(auto it2 : it.second){
			if(it2.name == argv[1]){
				CmdPrintf("(%d,%d,%d) %s", std::get<0>(it.first), std::get<1>(it.first), std::get<2>(it.first), argv[1]);
				count++;
			}
		}
	}
	if(!count)
		CmdPrintf("No such a star found: %s", argv[1]);
	return 0;
}

StaticInitializer stin([](){CmdAdd("find", cmd_find);});
static bool sqStarEnumDef(HSQUIRRELVM){
	return true;
}

sqa::Initializer sqin("StarEnum", [](HSQUIRRELVM v){
	static SQUserPointer tt_StarEnum = SQUserPointer("StarEnum");
	sq_pushroottable(v);
	sq_pushstring(v, _SC("StarEnum"), -1);
	sq_newclass(v, SQFalse);
	sq_settypetag(v, -1, tt_StarEnum);
	sq_setclassudsize(v, -1, sizeof(StarEnum));
	register_closure(v, _SC("constructor"), [](HSQUIRRELVM v){
		try{
			void *p;
			if(SQ_FAILED(sq_getinstanceup(v, 1, &p, tt_StarEnum)))
				return sq_throwerror(v, _SC("Something is wrong in StarEnum.constructor"));
			SQVec3d plpos;
			plpos.getValue(v, 2);
			SQInteger numSectors;
			if(SQ_FAILED(sq_getinteger(v, 3, &numSectors)))
				return sq_throwerror(v, _SC("StarEnum() second argument not convertible to int"));
			SQBool genCache;
			if(SQ_FAILED(sq_getbool(v, 4, &genCache)))
				genCache = SQFalse;
			new(p) StarEnum(plpos.value, int(numSectors), genCache == SQTrue);

			sq_setreleasehook(v, 1, [](SQUserPointer p, SQInteger){
				((StarEnum*)p)->~StarEnum();
				return SQInteger(1);
			});
		}
		catch(SQFError &e){
			return sq_throwerror(v, e.what());
		}
		return SQInteger(0);
	});
	register_closure(v, _SC("next"), [](HSQUIRRELVM v){
		void *p;
		if(SQ_FAILED(sq_getinstanceup(v, 1, &p, tt_StarEnum)))
			return sq_throwerror(v, _SC("Something is wrong in StarEnum.next"));
		StarEnum *se = (StarEnum*)p;
		SQVec3d pos;
		StarCache *sc;
		bool ret = se->next(pos.value, &sc);
		auto def = [v](const SQChar *name, const SQChar *value){
			sq_pushstring(v, name, -1);
			sq_pushstring(v, value, -1);
			sq_newslot(v, 2, SQFalse);
		};
		if(ret && sc){
			def(_SC("name"), sc->name);
			sq_pushstring(v, _SC("pos"), -1);
			pos.push(v);
			sq_newslot(v, 2, SQFalse);
			sq_pushstring(v, _SC("system"), -1);
			CoordSys::sq_pushobj(v, sc->system);
			sq_newslot(v, 2, SQFalse);
		}
		sq_pushbool(v, ret ? SQTrue : SQFalse);
		return SQInteger(1);
	});
	sq_newslot(v, -3, SQFalse);
	return true;
});


//=============================================================================
// GalaxyField implementation
//=============================================================================

static int g_galaxy_field_cache = 1;

static void perlin_noise(GalaxyField &field, GalaxyField &work, long seed){
	int octave;
	struct random_sequence rs;
	init_rseq(&rs, seed);
	for(octave = 0; (1 << octave) < FIELDZ; octave += 5){
		int cell = 1 << octave;
		int xi, yi, zi;
		int k;
		for(zi = 0; zi < FIELDZ / cell; zi++) for(xi = 0; xi < FIELD / cell; xi++) for(yi = 0; yi < FIELD / cell; yi++){
			int base;
			base = rseq(&rs) % 64 + 191;
			for(k = 0; k < 4; k++)
				work[xi][yi][zi][k] = /*rseq(&rs) % 32 +*/ base;
		}
		if(octave == 0)
			memcpy(field, work, FIELD * FIELD * FIELDZ * 4);
		else for(zi = 0; zi < FIELDZ; zi++) for(xi = 0; xi < FIELD; xi++) for(yi = 0; yi < FIELD; yi++){
			int xj, yj, zj;
			int sum[4] = {0};
			for(k = 0; k < 4; k++){
				for(xj = 0; xj <= 1; xj++) for(yj = 0; yj <= 1; yj++) for(zj = 0; zj <= 1; zj++){
					sum[k] += int((double)work[xi / cell + xj][yi / cell + yj][zi / cell + zj][k]
					* (xj ? xi % cell : (cell - xi % cell - 1)) / (double)cell
					* (yj ? yi % cell : (cell - yi % cell - 1)) / (double)cell
					* (zj ? zi % cell : (cell - zi % cell - 1)) / (double)cell);
				}
				field[xi][yi][zi][k] = MIN(255, field[xi][yi][zi][k] + sum[k] / 2);
			}
		}
	}
}

static unsigned char galaxy_set_star_density(GalaxyField &field, unsigned char c){
	Vec3d v0(FIELD / 2., FIELD / 2., FIELDZ / 2.);
	int v1[3];
	for(int i = 0; i < 3; i++)
		v1[i] = int(v0[i]);
/*	printf("stardensity(%lg,%lg,%lg)[%d,%d,%d]: %lg\n", v0[0], v0[1], v0[2], v1[0], v1[1], v1[2], );*/
	if(0 <= v1[0] && v1[0] < FIELD-1 && 0 <= v1[1] && v1[1] < FIELD-1 && 0 <= v1[2] && v1[2] < FIELDZ-1){
		int xj, yj, zj;
		for(xj = 0; xj <= 1; xj++) for(yj = 0; yj <= 1; yj++) for(zj = 0; zj <= 1; zj++){
			field[v1[0] + xj][v1[1] + yj][v1[2] + zj][3] = (GalaxyFieldCell)(c
				* (xj ? v0[0] - v1[0] : 1. - (v0[0] - v1[0]))
				* (yj ? v0[1] - v1[1] : 1. - (v0[1] - v1[1]))
				* (zj ? v0[2] - v1[2] : 1. - (v0[2] - v1[2])));
		}
		return c;
	}
	return (0 <= v1[0] && v1[0] < FIELD && 0 <= v1[1] && v1[1] < FIELD && 0 <= v1[2] && v1[2] < FIELDZ ? field[v1[0]][v1[1]][v1[2]][3] = c : 0);
}

#ifndef _WIN32
int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y){
	if(x->tv_usec < y->tv_usec){
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 100000 * nsec;
		y->tv_sec += nsec;
	}
	if(x->tv_usec - y->tv_usec > 1000000){
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	return x->tv_sec < y->tv_sec;
}
int timespec_subtract(struct timespec *result, struct timespec *x, struct timespec *y){

	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_nsec = x->tv_nsec - y->tv_nsec;

	return x->tv_sec < y->tv_sec;
}
#endif

static int ftimecmp(const char *file1, const char *file2){
#ifdef _WIN32
	WIN32_FILE_ATTRIBUTE_DATA fd, fd2;
	BOOL b1, b2;

	b1 = GetFileAttributesEx(file1, GetFileExInfoStandard, &fd);
	b2 = GetFileAttributesEx(file2, GetFileExInfoStandard, &fd2);

	/* absent file is valued oldest */
	if(!b1 && !b2)
		return 0;
	if(!b1)
		return -1;
	if(!b2)
		return 1;
	return (int)CompareFileTime(&fd.ftLastWriteTime, &fd2.ftLastWriteTime);
#else
	struct stat s1, s2;
	int e1, e2;
	int r1 = stat(file1, &s1);
	if(r1)
		e1 = errno;
	int r2 = stat(file2, &s2);
	if(r2)
		e2 = errno;
	if(r1 && r2)
		return 0;
	if(r1)
		return -1;
	if(r2)
		return 1;
	struct timespec temptim;
	return timespec_subtract(&temptim, &s1.st_mtim, &s2.st_mtim);
#endif
}

#define DARKNEBULA 8

static gltestp::dstring g_galaxy_file = "data/galaxy.png";

Initializer galaxy_init("galaxy_file", [](HSQUIRRELVM v){
	StackReserver sr(v);
	register_closure(v, _SC("setGalaxyFile"), [](HSQUIRRELVM v){
		const SQChar *str;
		if(SQ_FAILED(sq_getstring(v, 2, &str)))
			return sq_throwerror(v, _SC("setGalaxyFile argument not convertible to string"));
		g_galaxy_file = str;
		return SQInteger(0);
	});
	return true;
});

const GalaxyField *initGalaxyField(){
	static GalaxyField field;
	static bool field_init = false;

	if(!field_init){
		timemeas_t tm;
		TimeMeasStart(&tm);
		field_init = true;
		if(ftimecmp(g_galaxy_file, "cache/ourgalaxyvol.raw") < 0){
			FILE *fp = fopen("cache/ourgalaxyvol.raw", "rb");
			fread(field, sizeof field, 1, fp);
			fclose(fp);
		}
		else{
		struct random_sequence rs;
		int xi, yi, zi;
		int k;
		int srcx, srcy;
		double darknebula[DARKNEBULA][DARKNEBULA];
		GalaxyField *pfield2 = (GalaxyField*)malloc(sizeof field);
		if(!pfield2)
			return NULL;
		GalaxyField &field2 = *pfield2;
#if 1
		void (*freeproc)(BITMAPINFO*);
		BITMAPINFO *bmi = ReadPNG(g_galaxy_file, &freeproc);
		// We assume 8 bit grayscale image
		if(bmi->bmiHeader.biBitCount != 8)
			return NULL;
		const unsigned char *src = (unsigned char*)&bmi->bmiColors[bmi->bmiHeader.biClrUsed];
		srcx = bmi->bmiHeader.biWidth;
		srcy = bmi->bmiHeader.biHeight;
#elif 1
		FILE *fp;
		fp = fopen("ourgalaxy3.raw", "rb");
		if(!fp)
			return;
		srcx = srcy = 512;
		GLfloat (*src)[4] = (GLfloat(*)[4])malloc(srcx * srcy * sizeof *src);
		for(xi = 0; xi < srcx; xi++) for(yi = 0; yi < srcy; yi++){
			unsigned char c;
			fread(&c, 1, sizeof c, fp);
			src[xi * srcy + yi][2] = c / 256.f /*pow(c / 256.f, 2.)*/;
			fread(&c, 1, sizeof c, fp);
			src[xi * srcy + yi][1] = c / 256.f /*pow(c / 256.f, 2.)*/;
			fread(&c, 1, sizeof c, fp);
			src[xi * srcy + yi][0] = c / 256.f /*pow(c / 256.f, 2.)*/;
			src[xi * srcy + yi][3] = (src[xi * srcy + yi][0] + src[xi * srcy + yi][1] + src[xi * srcy + yi][2]) / 1.;
/*			c = src[xi * srcy + yi] * 255;
			fwrite(&c, 1, sizeof c, ofp);*/
		}
		fclose(fp);
#else
		fp = fopen("ourgalaxy_model.dat", "rb");
		if(!fp)
			return;
		fread(&srcx, 1, sizeof srcx, fp);
		fread(&srcy, 1, sizeof srcy, fp);
		src = (GLfloat*)malloc(srcx * srcy * sizeof *src);
		for(xi = 0; xi < srcx; xi++) for(yi = 0; yi < srcy; yi++){
			unsigned char c;
			fread(&src[xi * srcy + yi], 1, sizeof *src, fp);
/*			c = src[xi * srcy + yi] * 255;
			fwrite(&c, 1, sizeof c, ofp);*/
		}
		fclose(fp);
#endif
		CmdPrintf("draw_gs.load: %lg sec", TimeMeasLap(&tm));
		perlin_noise(field2, field, 3522526);
		init_rseq(&rs, 35229);
/*		for(zzi = 0; zzi < 16; zzi++){
		int zzz = 1;
		char sbuf[64];
		sprintf(sbuf, "gal%d.raw", zi);
		ofp = fopen(sbuf, "wb");
		for(zi = 16 - zzi; zzi && zi <= 16 + zzi; zi += zzi * 2, zzz -= 2)*/
		CmdPrintf("draw_gs.noise: %lg sec", TimeMeasLap(&tm));
		for(xi = 0; xi < DARKNEBULA; xi++) for(yi = 0; yi < DARKNEBULA; yi++){
			darknebula[xi][yi] = ((drseq(&rs) - .5) + (drseq(&rs) - .5));
		}
		CmdPrintf("draw_gs.nebula: %lg sec", TimeMeasLap(&tm));
		for(zi = 0; zi < FIELDZ; zi++){
		for(xi = 0; xi < FIELD; xi++) for(yi = 0; yi < FIELD; yi++){
			int xj, yj;
			double z0 = 0.;
			if(xi / (FIELD / DARKNEBULA) < DARKNEBULA-1 && yi / (FIELD / DARKNEBULA) < DARKNEBULA-1) for(xj = 0; xj <= 1; xj++) for(yj = 0; yj <= 1; yj++){
				int cell = FIELD / DARKNEBULA;
				z0 += (darknebula[xi / cell + xj][yi / cell + yj]
					* (xj ? xi % cell : (cell - xi % cell - 1)) / (double)cell
					* (yj ? yi % cell : (cell - yi % cell - 1)) / (double)cell
				/*+ (drseq(&rs) - .5)*/) * FIELDZ * .10;
			}
			double sdz = (zi + z0 - HFIELDZ) * (zi + z0 - HFIELDZ) / (double)(HFIELDZ * HFIELDZ);
			double sd = ((double)(xi - HFIELD) * (xi - HFIELD) / (HFIELD * HFIELD) + (double)(yi - HFIELD) * (yi - HFIELD) / (HFIELD * HFIELD) + sdz);

			// Distance from the center of the galaxy along galactic plane
			double dxy = sqrt((double)(xi - HFIELD) * (xi - HFIELD) / (HFIELD * HFIELD) + (double)(yi - HFIELD) * (yi - HFIELD) / (HFIELD * HFIELD));
			double dz = sqrt(sdz); // Distance from galactic plane
			// Elliptical distance
			double dellipse = sqrt((double)(xi - HFIELD) * (xi - HFIELD) / (HFIELD * HFIELD / 3 / 3) + (double)(yi - HFIELD) * (yi - HFIELD) / (HFIELD * HFIELD / 3 / 3) + sdz);

			// Obtain source pixel in the raw image.
			auto srcpixel = src[xi * srcx / FIELD + yi * srcy / FIELD * srcx];
			double srcw = srcpixel / 256.; // Normalized source pixel

			// Computed thickness of the galaxy at the pixel.  Thickness drops near galaxy edge.
			// If the pixel has low intensity, thickness is reduced to simulate spiral arms.
			double thickness = sqrt(std::max(0., 1. - dxy)) * srcw;

			if(srcpixel == 0 || 1. < dxy || thickness <= dz){
				memset(field2[xi][yi][zi], 0, sizeof (GalaxyFieldCell[4]));
			}
			else{
				// There are dark nebulae along the central plane of the galaxy.  We simulate this by darkening
				// the voxels around dz near 0.  We are dividing dz by thickness here to avoid everything become dark
				// near the edge of the galaxy.
				double dzq = dz / thickness;

				for(k = 0; k < 4; k++)
					field2[xi][yi][zi][k] *= (GalaxyFieldCell)(k == 3 ? std::min(1., (1. - dz / thickness) * 2.) : dzq);
			}

			// Enhance color near center to render bulge
			if(dellipse < 1.){
				field2[xi][yi][zi][0] = MIN(255, (GalaxyFieldCell)(field2[xi][yi][zi][0] + 256 * (1. - dellipse)));
				field2[xi][yi][zi][1] = MIN(255, (GalaxyFieldCell)(field2[xi][yi][zi][1] + 256 * (1. - dellipse)));
				field2[xi][yi][zi][2] = MIN(255, (GalaxyFieldCell)(field2[xi][yi][zi][2] + 128 * (1. - dellipse)));
				field2[xi][yi][zi][3] = MIN(255, (GalaxyFieldCell)(field2[xi][yi][zi][3] + 128 * (1. - dellipse)));
			}
/*			fwrite(&field2[xi][yi][zi], 1, 3, ofp);*/
		}
/*		fclose(ofp);*/
		}
#if 0
		for(zi = 1; zi < FIELDZ-1; zi++){
		char sbuf[64];
		for(n = 0; n < 1; n++){
			GLubyte (*srcf)[FIELD][FIELDZ][4] = n % 2 == 0 ? field2 : field;
			GLubyte (*dstf)[FIELD][FIELDZ][4] = n % 2 == 0 ? field : field2;
			for(xi = 1; xi < FIELD-1; xi++) for(yi = 1; yi < FIELD-1; yi++){
				int sum[4] = {0}, add[4] = {0};
				for(xj = -1; xj <= 1; xj++) for(yj = -1; yj <= 1; yj++) for(zj = -1; zj <= 1; zj++) for(k = 0; k < 4; k++){
					sum[k] += srcf[xi+xj][yi+yj][zi+zj][k];
					add[k]++;
				}
				for(k = 0; k < 4; k++)
					dstf[xi][yi][zi][k] = sum[k] / add[k]/*(3 * 3 * 3)*/;
			}
		}
/*		sprintf(sbuf, "galf%d.raw", zi);
		ofp = fopen(sbuf, "wb");
		for(xi = 0; xi < FIELD; xi++) for(yi = 0; yi < FIELD; yi++){
			fwrite(&(n % 2 == 0 ? field : field2)[xi][yi][zi], 1, 3, ofp);
		}
		fclose(ofp);*/
		}
#else
		memcpy(field, field2, sizeof field);
#endif
		galaxy_set_star_density(field, 64);
		if(g_galaxy_field_cache){
#ifdef _WIN32
			if(GetFileAttributes("cache") == -1)
				CreateDirectory("cache", NULL);
#else
			mkdir("cache", 0755);
#endif
			FILE *fp = fopen("cache/ourgalaxyvol.raw", "wb");
			fwrite(field, sizeof field, 1, fp);
			fclose(fp);
		}
		free(field2);
//		free(src);
		if(bmi){
			freeproc(bmi);
		}
		}
		CmdPrintf("draw_gs: %lg sec", TimeMeasLap(&tm));
	}

	return &field;
}

double galaxy_get_star_density_pos(const Vec3d &pos, const CoordSys *cs){
	const GalaxyField *field = initGalaxyField();
	if(!field)
		return 0.;
	Vec3d srcpos = cs ? cs->getGame()->universe->tocs(pos, cs) : pos;
	const Vec3d v0 = (srcpos - getGalaxyOffset()) * FIELD / GALAXY_EXTENT + Vec3d(FIELD, FIELD, FIELDZ) / 2.;
	int v1[3];
	int i;
	for(i = 0; i < 3; i++)
		v1[i] = int(v0[i]);
	/* cubic linear interpolation is fairly slower, but this function is rarely called. */
	if(0 <= v1[0] && v1[0] < FIELD-1 && 0 <= v1[1] && v1[1] < FIELD-1 && 0 <= v1[2] && v1[2] < FIELDZ-1){
		int xj, yj, zj;
		double sum = 0;
		for(xj = 0; xj <= 1; xj++) for(yj = 0; yj <= 1; yj++) for(zj = 0; zj <= 1; zj++){
			sum += (*field)[v1[0] + xj][v1[1] + yj][v1[2] + zj][3]
				* (xj ? v0[0] - v1[0] : 1. - (v0[0] - v1[0]))
				* (yj ? v0[1] - v1[1] : 1. - (v0[1] - v1[1]))
				* (zj ? v0[2] - v1[2] : 1. - (v0[2] - v1[2]));
		}
		return sum / 256.;
	}
	return (0 <= v1[0] && v1[0] < FIELD && 0 <= v1[1] && v1[1] < FIELD && 0 <= v1[2] && v1[2] < FIELDZ ? (*field)[v1[0]][v1[1]][v1[2]][3] / 256. : 0.);
}

double galaxy_get_star_density(Viewer *vw){
	CoordSys *csys = vw->cs->getGame()->universe;
	return galaxy_get_star_density_pos(vw->pos, vw->cs);
}

Vec3d getGalaxyOffset(){
	/// The Sol is located in 25000 ly offset from the center of Milky Way Galaxy
	static const Vec3d solarsystempos(-0, -25000.0 * LIGHTYEAR, 0);
	return solarsystempos;
}
