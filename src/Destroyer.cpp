/** \file
 * \brief Implementation of Destroyer and WireDestroyer classes.
 */
#include "Destroyer.h"
#include "EntityRegister.h"
#include "judge.h"
#include "serial_util.h"
#include "EntityCommand.h"
#include "btadapt.h"
#include "SqInitProcess-ex.h"
#ifndef DEDICATED
#include "draw/effects.h"
#endif
#include "motion.h"
#include "LTurret.h"
#include "sqadapt.h"
#include "Shipyard.h"
#include "draw/mqoadapt.h"
extern "C"{
#include <clib/mathdef.h>
}

const unsigned Destroyer::classid = registerClass("Destroyer", Conster<Destroyer>);
Entity::EntityRegister<Destroyer> Destroyer::entityRegister("Destroyer");
const char *Destroyer::classname()const{return "Destroyer";}
const char *Destroyer::dispname()const{return "Destroyer";}


double Destroyer::modelScale = .001;
double Destroyer::defaultMass = 1e8;
double Destroyer::maxHealthValue = 100000./2;
Warpable::ManeuverParams Destroyer::maneuverParams = {
	.05, /* double accel; */
	.1, /* double maxspeed; */
	10000. * .1, /* double angleaccel; */
	.2, /* double maxanglespeed; */
	150000., /* double capacity; [MJ] */
	300., /* double capacitor_gen; [MW] */
};
std::vector<hardpoint_static*> Destroyer::hardpoints;
ArmCtors Destroyer::armCtors;
std::list<Entity::VariantRegister<Destroyer>*> Destroyer::variantRegisters;
HitBoxList Destroyer::hitboxes;
GLuint Destroyer::disp = 0;
std::vector<Warpable::Navlight> Destroyer::navlights;


Destroyer::Destroyer(WarField *aw, const SQChar *variant) : st(aw), engineHeat(0.), clientDead(false){
	init();
	ArmCtors::iterator it = armCtors.find(variant);
	ArmCtors::value_type::second_type *ctors = it != armCtors.end() ? &it->second : NULL;
	for(int i = 0; i < hardpoints.size(); i++){
		ArmBase *turret;
		if(ctors && i < ctors->size()){
			const gltestp::dstring &cname = (*ctors)[i];
			turret = cname == "LTurret" ? (ArmBase*)new LTurret(this, hardpoints[i]) : (ArmBase*)new LMissileTurret(this, hardpoints[i]);
		}
		else
			turret = 1||i % 3 != 0 ? (LTurretBase*)new LTurret(this, hardpoints[i]) : (LTurretBase*)new LMissileTurret(this, hardpoints[i]);
		turrets.push_back(turret);
		if(aw)
			aw->addent(turret);
	}
	buildBody();
}

Destroyer::~Destroyer(){
	if(game->isServer()){
		// This Entity "owns" the equipments, so delete them along with.
		// When we delete the ArmBases, ObserverList's unlink() method is invoked,
		// which in turn invalidates the internal list (acutually, vector) object.
		// So we must retrieve the iterator repeatedly each time we delete an element.
		// Probably rbegin() is slightly more efficient than begin(), because we do
		// not need to memmove() if we truncate from tail.
		TurretList::reverse_iterator it;
		while(turrets.rend() != (it = turrets.rbegin()))
			delete *it;

		// Below is an alternate implementation that cleans the list without memory
		// leaks nor runtime errors, but with additional processing cost.
/*		std::vector<ArmBase*> localTurrets;
		localTurrets.assign(turrets.begin(), turrets.end());
		for(std::vector<ArmBase*>::iterator it = localTurrets.begin(); it != localTurrets.end(); ++it){
			delete *it;
		}*/
	}
	// Show death effects unless it's under construction.
	// It seems safe to use equality operator since we assign the constant 1.0 when the construction is
	// complete (rather than adding values), but being careful won't cause a trouble.
	else if(1.0 - FLT_EPSILON <= buildPhase)
		deathEffects();
}

bool Destroyer::buildBody(){
	static btCompoundShape *shape = NULL;
	return buildBodyByHitboxes(hitboxes, shape);
}

bool Destroyer::static_init(HSQUIRRELVM v){
	static bool initialized = false;
	if(!initialized){
		SqInit(v, _SC("models/Destroyer.nut"),
			ModelScaleProcess(modelScale) <<=
			MassProcess(defaultMass) <<=
			SingleDoubleProcess(maxHealthValue, "maxhealth", false) <<=
			ManeuverParamsProcess(maneuverParams) <<=
			HitboxProcess(hitboxes) <<=
			DrawOverlayProcess(disp) <<=
			NavlightsProcess(navlights) <<=
			HardPointProcess(hardpoints) <<=
			VariantProcess(armCtors, "armCtors")
			);
		for(ArmCtors::iterator it = armCtors.begin(); it != armCtors.end(); ++it)
			variantRegisters.push_back(new VariantRegister<Destroyer>(gltestp::dstring("Destroyer") + it->first, it->first));
		initialized = true;
	}
	return initialized;
}

static sqa::Initializer sqinit("DestroyerVariantRegister", Destroyer::static_init);

void Destroyer::init(){
	st::init();
	mass = defaultMass;
	engineHeat = 0.;
	buildPhase = 1.;
}

void Destroyer::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << unsigned(hardpoints.size());
	for(int i = 0; i < hardpoints.size(); i++)
		sc.o << turrets[i];
}

void Destroyer::unserialize(UnserializeContext &sc){
	st::unserialize(sc);

	// Update the dynamics body's parameters too.
	if(bbody){
		bbody->setCenterOfMassTransform(btTransform(btqc(rot), btvc(pos)));
		bbody->setAngularVelocity(btvc(omg));
		bbody->setLinearVelocity(btvc(velo));
	}

	unsigned size;
	sc.i >> size;
	turrets.clear();
	for(int i = 0; i < size; i++){
		ArmBase *a;
		sc.i >> a;
		turrets.push_back(a);
	}
}

double Destroyer::getHitRadius()const{return .27;}

void Destroyer::anim(double dt){
	if(!w)
		return;
	// Do not move until construction is complete.
	if(buildPhase != 1.){
		// Turrets should follow the position of the base even if it's inactive.
		for(TurretList::iterator it = turrets.begin(); it != turrets.end(); ++it) if(*it)
			(*it)->align();
		return;
	}
/*	if(bbody){
		const btTransform &tra = bbody->getCenterOfMassTransform();
		pos = btvc(tra.getOrigin());
		rot = btqc(tra.getRotation());
		velo = btvc(bbody->getLinearVelocity());
		omg = btvc(bbody->getAngularVelocity());
	}*/

	st::anim(dt);
	for(TurretList::iterator it = turrets.begin(); it != turrets.end(); ++it) if(*it)
		(*it)->align();

	// Exponential approach is more realistic (but costs more CPU cycles)
	engineHeat = direction & PL_W ? engineHeat + (1. - engineHeat) * (1. - exp(-dt)) : engineHeat * exp(-dt);
}

void Destroyer::clientUpdate(double dt){
	anim(dt);
}

void Destroyer::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	rot = this->rot;
	pos = rot.trans(Vec3d(0, .08, .0)) + this->pos;
}

int Destroyer::takedamage(double damage, int hitpart){
	int ret = 1;

//	playWave3D("hit.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
	if(0 < health && health - damage <= 0){
		ret = 0;
		deathEffects();
		// Actually delete only in the server.
		if(game->isServer())
			delete this;
		return ret;
	}
	health -= damage;
	return ret;
}

int Destroyer::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	return Shipyard::modelTraceHit(this, src, dir, rad, dt, ret, retp, retn, getModel());
}

/// \brief Death effects in the client.
///
/// It should be called exactly once per client object.
/// Due to lags between the server and the client, death effect can be invoked first in
/// either sides. In any case, we make sure to generate death effect once and not more.
/// The clientDead flag helps achieving this demand.
void Destroyer::deathEffects(){
#ifndef DEDICATED
	Destroyer *p = this;
	WarSpace *ws = *w;

	// Prevent double effects in the client, for Destroyers have costly death effects.
	if(!game->isServer() && ws && !clientDead){
		// Only clear the flag in the client.
		clientDead = true;
		Teline3List *tell = w->getTeline3d();

		if(ws->gibs) for(int i = 0; i < 128; i++){
			double pos[3], velo[3] = {0}, omg[3];
			/* gaussian spread is desired */
			for(int j = 0; j < 6; j++)
				velo[j / 2] += .025 * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
			omg[0] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
			omg[1] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
			omg[2] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
			VECCPY(pos, this->pos);
			for(int j = 0; j < 3; j++)
				pos[j] += getHitRadius() * (drseq(&w->rs) - .5);
			AddTelineCallback3D(ws->gibs, pos, velo, .010, quat_u, omg, vec3_000, debrigib, NULL, TEL3_QUAT | TEL3_NOLINE, 15. + drseq(&w->rs) * 5.);
		}

		/* smokes */
		for(int i = 0; i < 64; i++){
			double pos[3];
			COLOR32 col = 0;
			VECCPY(pos, p->pos);
			pos[0] += .3 * (drseq(&w->rs) - .5);
			pos[1] += .3 * (drseq(&w->rs) - .5);
			pos[2] += .3 * (drseq(&w->rs) - .5);
			col |= COLOR32RGBA(rseq(&w->rs) % 32 + 127,0,0,0);
			col |= COLOR32RGBA(0,rseq(&w->rs) % 32 + 127,0,0);
			col |= COLOR32RGBA(0,0,rseq(&w->rs) % 32 + 127,0);
			col |= COLOR32RGBA(0,0,0,191);
//			AddTeline3D(w->tell, pos, NULL, .035, NULL, NULL, NULL, col, TEL3_NOLINE | TEL3_GLOW | TEL3_INVROTATE, 60.);
			AddTelineCallback3D(ws->tell, pos, vec3_000, .07, quat_u, vec3_000,
				vec3_000, smokedraw, (void*)col, TEL3_INVROTATE | TEL3_NOLINE, 20.);
		}

		{/* explode shockwave thingie */
#if 0
			static const double pyr[3] = {M_PI / 2., 0., 0.};
			amat3_t ort;
			Vec3d dr, v;
			Quatd q;
			amat4_t mat;
			double p;
/*			w->vft->orientation(w, &ort, &pt->pos);
			VECCPY(dr, &ort[3]);*/
			dr = vec3_001;

			/* half-angle formula of trigonometry replaces expensive tri-functions to square root */
			q[3] = sqrt((dr[2] + 1.) / 2.) /*cos(acos(dr[2]) / 2.)*/;

			v = vec3_001.vp(dr);
			p = sqrt(1. - q[3] * q[3]) / VECLEN(v);
			q = v * p;

			AddTeline3D(tell, this->pos, vec3_000, 5., q, vec3_000, vec3_000, COLOR32RGBA(255,191,63,255), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_QUAT, 2.);
#endif
			AddTeline3D(tell, this->pos, vec3_000, 3., quat_u, vec3_000, vec3_000, COLOR32RGBA(255,255,255,127), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_INVROTATE, 2.);
		}
	}
//		playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
#endif
}

double Destroyer::getHealth()const{
	return health - (1. - buildPhase) * getMaxHealth();
}

double Destroyer::getMaxHealth()const{return maxHealthValue;}

int Destroyer::armsCount()const{
	return hardpoints.size();
}

ArmBase *Destroyer::armsGet(int i){
	if(i < 0 || armsCount() <= i)
		return NULL;
	return turrets[i];
}

int Destroyer::engineAtrIndex = -1, Destroyer::gradientsAtrIndex = -1;

Model *Destroyer::getModel(){
	static bool init = false;
	static Model *model = NULL;
	if(!init){
		model = LoadMQOModel("models/destroyer.mqo");
		if(model && model->sufs[0]){
			for(int i = 0; i < model->tex[0]->n; i++){
				const char *colormap = model->sufs[0]->a[i].colormap;
				if(colormap && !strcmp(colormap, "engine2.bmp")){
					engineAtrIndex = i;
				}
				if(colormap && !strcmp(colormap, "gradients.png")){
					gradientsAtrIndex = i;
				}
			}
		}
		init = true;
	}

	return model;
}

bool Destroyer::command(EntityCommand *com){
	if(SetBuildPhaseCommand *sbpc = InterpretCommand<SetBuildPhaseCommand>(com)){
		buildPhase = sbpc->phase;
		if(buildPhase != 1){
			for(TurretList::iterator it = turrets.begin(); it != turrets.end(); ++it)
				(*it)->online = false;
		}
		else
			for(TurretList::iterator it = turrets.begin(); it != turrets.end(); ++it)
				(*it)->online = true;
	}
	else if(GetFaceInfoCommand *gfic = InterpretCommand<GetFaceInfoCommand>(com)){
		return Shipyard::modelHitPart(this, getModel(), *gfic);
	}
	else
		return st::command(com);
}

double Destroyer::maxenergy()const{return getManeuve().capacity;}

const Warpable::ManeuverParams &Destroyer::getManeuve()const{
	return maneuverParams;
}

double Destroyer::warpCostFactor()const{
	return 1e2;
}

HitBoxList *Destroyer::getTraceHitBoxes()const{
	return &hitboxes;
}








HitBox WireDestroyer::hitboxes[] = {
	HitBox(Vec3d(0., 0., -.058), Quatd(0,0,0,1), Vec3d(.051, .032, .190)),
	HitBox(Vec3d(0., 0., .193), Quatd(0,0,0,1), Vec3d(.051, .045, .063)),
	HitBox(Vec3d(.0, -.06, .060), Quatd(0,0,0,1), Vec3d(.015, .030, .018)),
	HitBox(Vec3d(.0, .06, .060), Quatd(0,0,0,1), Vec3d(.015, .030, .018)),
	HitBox(Vec3d(.0, .0, .0), Quatd(0,0,0,1), Vec3d(.07, .070, .02)),
};
const int WireDestroyer::nhitboxes = numof(hitboxes);

WireDestroyer::WireDestroyer(WarField *aw) : st(aw), wirephase(0), wireomega(0), wirelength(2.){
	init();
	mass = 1e6;
}


const unsigned WireDestroyer::classid = registerClass("WireDestroyer", Conster<WireDestroyer>);
Entity::EntityRegister<WireDestroyer> WireDestroyer::entityRegister("WireDestroyer");
const char *WireDestroyer::classname()const{return "WireDestroyer";}
const char *WireDestroyer::dispname()const{return "Wire Destroyer";}

void WireDestroyer::serialize(SerializeContext &sc){
	st::serialize(sc);
}

void WireDestroyer::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
}

double WireDestroyer::getHitRadius()const{return .3;}
double WireDestroyer::getMaxHealth()const{return 100000.;}
double WireDestroyer::maxenergy()const{return 10000.;}

static int wire_hit_callback(const struct otjEnumHitSphereParam *param, Entity *pt){
	const Vec3d *src = param->src;
	const Vec3d *dir = param->dir;
	double dt = param->dt;
	double rad = param->rad;
	Vec3d *retpos = param->pos;
	Vec3d *retnorm = param->norm;
	void *hint = param->hint;
	WireDestroyer *pb = ((WireDestroyer**)hint)[1];
	double hitrad = .02;
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
	if(!jHitSphere(pt->pos, pt->getHitRadius() + hitrad, *param->src, *param->dir, dt))
		return 0;
/*	{
		ret = pt->tracehit(pb->pos, pb->velo, hitrad, dt, NULL, &pos, &nh);
		if(!ret)
			return 0;
		else if(rethitpart)
			*rethitpart = ret;
	}*/

/*	{
		pb->pos += pb->velo * dt;
		if(retpos)
			*retpos = pos;
		if(retnorm)
			*retnorm = nh;
	}*/
	return ret;
}

void WireDestroyer::anim(double dt){
	st::anim(dt);
	wireomega = 6. * M_PI;

	int subwires = 8;
#if 0 // Temporarily disabled wire hit check
	if(WarSpace *ws = *w) for(int n = 0; n < subwires; n++){
		Mat4d mat;
		transform(mat);
		for(int i = 0; i < 2; i++){
			Mat4d rot = mat.rotz(wirephase + (wireomega * dt) * n / subwires);
			Vec3d src = rot.vp3(Vec3d(.07 * (i * 2 - 1), 0, 0));
			Vec3d dst = rot.vp3(Vec3d(wirelength * (i * 2 - 1), 0, 0));
			Vec3d dir = (dst - src).norm();
			struct otjEnumHitSphereParam param;
			void *hint[3];
			Entity *pt;
			Vec3d pos, nh;
			int hitpart = 0;
			hint[0] = w;
			hint[1] = this;
			hint[2] = &hitpart;
			param.root = ws->otroot;
			param.src = &src;
			param.dir = &dir;
			param.dt = wirelength;
			param.rad = 0.01;
			param.pos = &pos;
			param.norm = &nh;
			param.flags = OTJ_CALLBACK;
			param.callback = wire_hit_callback;
			param.hint = hint;
			for(pt = ws->ot ? (otjEnumHitSphere(&param)) : ws->el; pt; pt = ws->ot ? NULL : pt->next){
				sufindex pi;
				double damage = 1000.;

				if(!ws->ot && !wire_hit_callback(&param, pt))
					continue;

				if(pt->w == w) if(!pt->takedamage(damage, hitpart)){
					extern int wire_kills;
					wire_kills++;
				}
			}while(0);
		}
	}
#endif
	wirephase += wireomega * dt;
}

int WireDestroyer::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	double sc[3];
	double best = dt, retf;
	int reti = 0, i, n;
#if 0
	if(0 < p->shieldAmount){
		Vec3d hitpos;
		if(jHitSpherePos(pos, BEAMER_SHIELDRAD + rad, src, dir, dt, ret, &hitpos)){
			if(retp) *retp = hitpos;
			if(retn) *retn = (hitpos - pos).norm();
			return 1000; /* something quite unlikely to reach */
		}
	}
#endif
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

void WireDestroyer::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	rot = this->rot;
	pos = rot.trans(Vec3d(0, .08, .05)) + this->pos;
}

const Warpable::ManeuverParams &WireDestroyer::getManeuve()const{
	return Destroyer::maneuverParams;
}


#ifdef DEDICATED
void Destroyer::draw(WarDraw *){}
void Destroyer::drawtra(WarDraw *){}
void Destroyer::drawOverlay(WarDraw *){}
int Destroyer::popupMenu(PopupMenu &){}
void WireDestroyer::draw(WarDraw *){}
void WireDestroyer::drawtra(WarDraw *){}
#endif
