/** \file
 * \brief Implementation of Warpable class.
 */
#include "Warpable.h"
#include "cmd.h"
#include "astrodef.h"
#include "astro_star.h"
#include "btadapt.h"
#include "Builder.h"
#ifndef DEDICATED
#include "astrodraw.h"
#endif
#include "StaticInitializer.h"
extern "C"{
#include <clib/mathdef.h>
}




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
				Astrobj *cs = bestsc->system;
				if(!cs){ // Materialize a randomly generated star; create a Star object and associate it with the StarCache.
					Star *child = new Star(bestsc->name, p->warpdstcs);
					child->pos = bestPos;
					child->name = bestsc->name;
					double radscale = exp(rs->nextGauss());
					child->rad = 6.960e5 * radscale; // Exponential distribution around Rsun
					child->mass = 1.9884e30 * radscale * radscale * radscale * exp(rs->nextGauss()); // Mass has variance too, but probably mean is proportional to cubic radius.
					child->spect = Star::G;
					// Create a temporary orbit to arrive.
					OrbitCS *orbit = new OrbitCS("orbit", child);
					Vec3d orbitPos = child->tocs(this->warpdst, this->warpdstcs);
					orbit->orbits(child, orbitPos.len(), 0., Quatd::direction(orbitPos).rotate(M_PI / 2., Vec3d(1,0,0)));
					orbit->setShowOrbit(true);
					this->warpdst = orbit->tocs(this->warpdst, this->warpdstcs);
					this->warpdstcs = orbit;
					bestsc->system = child;
				}
				else{
					// Reuse the temporary orbit
					OrbitCS *orbit = new OrbitCS("orbit", cs);
					this->warpdst = orbit->tocs(this->warpdst, this->warpdstcs);
					this->warpdstcs = orbit;
				}
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
	else{
		if(Builder *builder = getBuilder()){
			if(builder->command(com))
				return true;
		}
		return st::command(com);
	}
	return false;
}


void Warpable::post_warp(){
}

