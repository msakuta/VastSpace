/** \file
 * \brief Implementation of Shipyard class non-drawing member functions.
 */
#define NOMINMAX
#include "Shipyard.h"
#include "EntityRegister.h"
#include "judge.h"
#include "serial_util.h"
#include "Player.h"
#include "antiglut.h"
#include "sqadapt.h"
#include "btadapt.h"
#include "Destroyer.h"
#include "draw/mqoadapt.h"
extern "C"{
#include <clib/mathdef.h>
#include <clib/cfloat.h>
}
#include <sqstdio.h>
#ifndef DEDICATED
#include <gl/GL.h>
#endif

/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289
#define SQRT2P2 (M_SQRT2/2.)




#if 1
const unsigned ShipyardDocker::classid = registerClass("ShipyardDocker", Conster<ShipyardDocker>);

const char *ShipyardDocker::classname()const{return "ShipyardDocker";}

void ShipyardDocker::dock(Dockable *e){
	st::dock(e);
	if(getent()->dockingFrigate == e)
		getent()->dockingFrigate = NULL;
}

void ShipyardDocker::dockque(Dockable *e){
	QueryClassCommand com;
	com.ret = Docker::Fighter;
	e->command(&com);
	if(com.ret == Docker::Frigate){
		static_cast<Shipyard*>(this->e)->dockingFrigate = e;
	}
}


double Shipyard::maxHealthValue = 200000.;
#define SCARRY_MAX_SPEED .03
#define SCARRY_ACCELERATE .01
#define SCARRY_MAX_ANGLESPEED (.005 * M_PI)
#define SCARRY_ANGLEACCEL (.002 * M_PI)

double Shipyard::modelScale = 0.0010;
double Shipyard::hitRadius = 1.0;
double Shipyard::defaultMass = 5e9;

hardpoint_static *Shipyard::hardpoints = NULL/*[10] = {
	hardpoint_static(Vec3d(.100, .060, -.760), Quatd(0., 0., 0., 1.), "Turret 1", 0),
	hardpoint_static(Vec3d(.100, -.060, -.760), Quatd(0., 0., 1., 0.), "Turret 2", 0),
	hardpoint_static(Vec3d(-.180,  .180, .420), Quatd(0., 0., 0., 1.), "Turret 3", 0),
	hardpoint_static(Vec3d(-.180, -.180, .420), Quatd(0., 0., 1., 0.), "Turret 4", 0),
	hardpoint_static(Vec3d( .180,  .180, .420), Quatd(0., 0., 0., 1.), "Turret 5", 0),
	hardpoint_static(Vec3d( .180, -.180, .420), Quatd(0., 0., 1., 0.), "Turret 6", 0),
	hardpoint_static(Vec3d(-.180,  .180, -.290), Quatd(0., 0., 0., 1.), "Turret 7", 0),
	hardpoint_static(Vec3d(-.180, -.180, -.290), Quatd(0., 0., 1., 0.), "Turret 8", 0),
	hardpoint_static(Vec3d( .180,  .180, -.380), Quatd(0., 0., 0., 1.), "Turret 9", 0),
	hardpoint_static(Vec3d( .180, -.180, -.380), Quatd(0., 0., 1., 0.), "Turret 10", 0),
}*/;
int Shipyard::nhardpoints = 0;

Shipyard::Shipyard(WarField *w) : st(w), docker(new ShipyardDocker(this)), Builder(), clientDead(false){
	st::init();
	init();
//	for(int i = 0; i < nhardpoints; i++)
//		turrets[i] = NULL;
//		w->addent(turrets[i] = new MTurret(this, &hardpoints[i]));
}

void Shipyard::init(){
	static bool initialized = false;
	if(!initialized){
		sq_init(_SC("models/Shipyard.nut"),
				ModelScaleProcess(modelScale) <<=
				SingleDoubleProcess(hitRadius, "hitRadius", false) <<=
				MassProcess(defaultMass) <<=
				SingleDoubleProcess(maxHealthValue, "maxhealth", false) <<=
				HitboxProcess(hitboxes) <<=
				DrawOverlayProcess(disp) <<=
				NavlightsProcess(navlights));
		initialized = true;
	}
	ru = 250.;
	mass = defaultMass;
	health = getMaxHealth();
	doorphase[0] = 0.;
	doorphase[1] = 0.;
	respondingCommand = NULL;
}

Shipyard::~Shipyard(){
	if(game->isServer())
		delete docker;
	else if(!clientDead)
		deathEffects();
}

const char *Shipyard::idname()const{return "Shipyard";}
const char *Shipyard::classname()const{return "Shipyard";}

const unsigned Shipyard::classid = registerClass("Shipyard", Conster<Shipyard>);
Entity::EntityRegister<Shipyard> Shipyard::entityRegister("Shipyard");

void Shipyard::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << docker;
	sc.o << undockingFrigate;
	sc.o << dockingFrigate;
	sc.o << buildingCapital;
	Builder::serialize(sc);
//	for(int i = 0; i < nhardpoints; i++)
//		sc.o << turrets[i];
}

void Shipyard::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> docker;
	sc.i >> undockingFrigate;
	sc.i >> dockingFrigate;
	sc.i >> buildingCapital;
	Builder::unserialize(sc);

	// Update the dynamics body's parameters too.
	if(bbody){
		bbody->setCenterOfMassTransform(btTransform(btqc(rot), btvc(pos)));
		bbody->setAngularVelocity(btvc(omg));
		bbody->setLinearVelocity(btvc(velo));
	}

//	for(int i = 0; i < nhardpoints; i++)
//		sc.i >> turrets[i];
}

void Shipyard::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	st::dive(sc, method);
	docker->dive(sc, method);
}

const char *Shipyard::dispname()const{
	return "Shipyard";
};

double Shipyard::getHealth()const{
	return std::max(0., health);
}

double Shipyard::getMaxHealth()const{
	return maxHealthValue;
}

double Shipyard::getHitRadius()const{
	return hitRadius;
}

int Shipyard::takedamage(double damage, int hitpart){
	if(this->health < 0.)
		return 1;
	if(0 < health && health - damage <= 0){
		health = -5.0; // Death effect time
	}
	else
		health -= damage;
	return 1;
}

double Shipyard::maxenergy()const{
	return 5e8;
}

void Shipyard::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	static const Vec3d pos0[] = {
		Vec3d(.100, .22, -.610),
		Vec3d(.100, .10, -1.000),
		Vec3d(-.100, .10, 1.000),
	};
	if(seatid < numof(pos0)){
		pos = this->pos + this->rot.trans(pos0[seatid % numof(pos0)]);
		rot = this->rot;
	}
	else{
		Quatd trot = this->rot.rotate(w->war_time() * M_PI / 6., 0, 1, 0).rotate(sin(w->war_time() * M_PI / 23.) * M_PI / 3., 1, 0, 0);
		pos = this->pos + trot.trans(Vec3d(0, 0, 1.));
		rot = trot;
	}
}

bool Shipyard::buildBody(){
	if(!bbody){
		static btCompoundShape *shape = NULL;
		if(!shape){
			shape = new btCompoundShape();
			// Assign dummy value if the initialize file is not available.
			if(hitboxes.empty())
				hitboxes.push_back(hitbox(Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(.3, .2, .500)));
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


short Shipyard::bbodyGroup()const{
	return 2;
}

short Shipyard::bbodyMask()const{
	return ~2;
}

HitBoxList *Shipyard::getTraceHitBoxes()const{
	return &hitboxes;
}

void Shipyard::anim(double dt){
	st::anim(dt);
	docker->anim(dt);
	Builder::anim(dt);

	if(health < 0){
		this->health += dt;
		if(0. < this->health && !clientDead){
			deathEffects();

			// Delete only in the server.
			if(game->isServer()){
				delete this;
				return;
			}
			else // Mark as dead to prevent multiple death effects.
				this->clientDead = true;
		}
		else
			dyingEffects(dt);
		return;
	}

	if(buildingCapital && nbuildque){
		SetBuildPhaseCommand com(1. - build / buildque[0].st->buildtime);
		buildingCapital->command(&com);
	}
//	for(int i = 0; i < nhardpoints; i++) if(turrets[i])
//		turrets[i]->align();
//	clientUpdate(dt);
}

void Shipyard::clientUpdate(double dt){
	Shipyard::anim(dt);

	if(health < 0)
		return;

	if(undockingFrigate){
		double threshdist = .5 + undockingFrigate->getHitRadius();
		const Vec3d &udpos = undockingFrigate->pos;
		Vec3d delta = udpos - (pos + rot.trans(Vec3d(0.1, 0, -0.35)));
		if(threshdist * threshdist < delta.slen())
			undockingFrigate = NULL;
		else
			doorphase[0] = approach(doorphase[0], 1., dt * .5, 0.);
	}
	else
		doorphase[0] = approach(doorphase[0], 0., dt * 0.5, 0.);

	if(dockingFrigate){
		if(dockingFrigate->w != w)
			dockingFrigate = NULL;
		else
			doorphase[1] = approach(doorphase[1], 1., dt * .5, 0.);
	}
	else
		doorphase[1] = approach(doorphase[1], 0., dt * .5, 0.);
}

Entity::Props Shipyard::props()const{
	Props ret = st::props();
//	ret.push_back(cpplib::dstring("?: "));
	return ret;
}

bool Shipyard::command(EntityCommand *com){
	// Trick to prevent infinite recursive call. Should have more intrinsic fix.
	if(respondingCommand)
		return false;

	if(GetFaceInfoCommand *gfic = InterpretCommand<GetFaceInfoCommand>(com)){
		// Retrieving face information from hit part index require model to be loaded.
		Model *model = getModel();

		return modelHitPart(this, model, *gfic);
	}
	else{
		respondingCommand = com;
		bool ret = st::command(com);
		respondingCommand = NULL;
		return ret;
	}
}

bool Shipyard::modelHitPart(const Entity *e, Model *model, GetFaceInfoCommand &gfic){
	if(!model)
		return false;
	int hitMesh = gfic.hitpart / 0x10000;
	int hitPoly = gfic.hitpart % 0x10000 - 1;
	if(model->n <= hitMesh || model->sufs[hitMesh]->np <= hitPoly)
		return false;

	// Obtain source position in local coordinates
	Mat4d mat;
	e->transform(mat);
	Mat4d imat = mat.scalein(-1. / modelScale, 1. / modelScale, -1. / modelScale).transpose();
	Vec3d lsrc = imat.dvp3(gfic.pos - e->pos);

	// Hit part number is index of polygon buffer in the model.
	Mesh *mesh = model->sufs[hitMesh];
	Mesh::Primitive *pr = mesh->p[hitPoly];

	// Convert indices for jHitPolygon
	unsigned short indices[4];
	int n = 0;
	if(pr->t == Mesh::ET_Polygon){
		Mesh::Polygon &p = pr->p;
		assert(p.n <= 4);
		n = p.n;
		for(int j = 0; j < p.n; j++)
			indices[p.n - j - 1] = p.v[j].pos; // Reverse face direction
	}
	else if(pr->t == Mesh::ET_UVPolygon){
		Mesh::UVPolygon &p = pr->uv;
		assert(p.n <= 4);
		n = p.n;
		for(int j = 0; j < p.n; j++)
			indices[p.n - j - 1] = p.v[j].pos; // Reverse face direction
	}

	// Local normal vector
	Vec3d lnrm = mesh->v[pr->t == Mesh::ET_Polygon ? pr->p.v[0].nrm : pr->uv.v[0].nrm];

	// We could have simpler algorithm to determine if the source position projected onto the face
	// is inside the polygon shape than jHitPolygon(), but for now it's easier to reuse it.
	int iret = jHitPolygon(mesh->v, indices, n, lsrc, -lnrm, 0, 1e8, NULL, NULL, NULL);

	// Return hit state to the caller
	gfic.retPosHit = iret != 0;

	// Return normal vector in world coordinates
	lnrm[0] *= -1.;
	lnrm[2] *= -1.;
	gfic.retNormal = e->rot.trans(lnrm);

	// Return projected point in world coordinates
	Vec3d lpos = mesh->v[pr->t == Mesh::ET_Polygon ? pr->p.v[0].pos : pr->uv.v[0].pos];
	lpos *= modelScale;
	lpos[0] *= -1.;
	lpos[2] *= -1.;
	gfic.retPos = gfic.pos + gfic.retNormal.sp(e->pos + e->rot.trans(lpos) - gfic.pos) * gfic.retNormal;

	return true;
}

int Shipyard::armsCount()const{
	return nhardpoints;
}

ArmBase *Shipyard::armsGet(int i){
	return turrets[i];
}

Builder *Shipyard::getBuilderInt(){return this;}
Docker *Shipyard::getDockerInt(){return docker;}


const Shipyard::ManeuverParams Shipyard::mymn = {
	SCARRY_ACCELERATE, /* double accel; */
	SCARRY_MAX_SPEED, /* double maxspeed; */
	SCARRY_ANGLEACCEL, /* double angleaccel; */
	SCARRY_MAX_ANGLESPEED, /* double maxanglespeed; */
	5e8, /* double capacity; */
	1e6, /* double capacitor_gen; */
};

const Warpable::ManeuverParams &Shipyard::getManeuve()const{return mymn;}

HitBoxList Shipyard::hitboxes;
GLuint Shipyard::disp = 0;
std::vector<Shipyard::Navlight> Shipyard::navlights;

bool Shipyard::startBuild(){
	const BuildRecipe *br = buildque[0].st;
	const Destroyer::VariantList &vars = Destroyer::getVariantRegisters();
	bool destroyerVariant = !strcmp(br->className, "Destroyer"); // Destroyer itself is included in variants.

	// Find if this BuildRecipe is a variant of Destroyer.
	if(!destroyerVariant)
	for(Destroyer::VariantList::const_iterator it = vars.begin(); it != vars.end(); ++it){
		if((*it)->classname == br->className){
			destroyerVariant = true;
			break;
		}
	}

	if(destroyerVariant){
		Vec3d newPos = pos + rot.trans(Vec3d(-0.45, 0, 0));
		WarSpace *ws = *w;
		if(ws){
			WarField::EntityList &el = ws->entlist();
			// Iterate through all Entities to find if any one blocks the scaffold.
			// We'd really want to find it by using the object tree, but it's currently absent in the client.
			for(WarField::EntityList::iterator it = el.begin(); it != el.end(); ++it){
				// Ignore the Shipyard itself.
				if(*it == this)
					continue;
				// Radius required for building is assumed 300 meters.
				double radius = (*it)->getHitRadius() + 0.3;
				if((newPos - (*it)->pos).slen() < radius * radius)
					return false;
			}
		}

		Entity *e = Entity::create(br->className, this->w);
		assert(e);

		e->setPosition(&newPos, &rot, &velo);

		// Let's get along with our mother's faction.
		e->race = this->race;

		buildingCapital = e;
	}
	return Builder::startBuild();
}

bool Shipyard::finishBuild(){
	if(!buildingCapital)
		return Builder::finishBuild();
	else{
		doneBuild(buildingCapital);
		buildingCapital = NULL;
	}
	return true;
}

bool Shipyard::cancelBuild(int index, bool recalc_time){
	if(game->isServer() && buildingCapital)
		delete buildingCapital;
	return Builder::cancelBuild(index, recalc_time);
}

void Shipyard::doneBuild(Entity *e){
	Entity::Dockable *d = e->toDockable();
	if(d)
		docker->dock(d);
	else{
		// The builder's default is to add the built Entity to the belonging WarField, so we
		// do not need to addent() here.  If we come to need it, we should leaveField() before doing so.
//		e->w = this->Entity::w;
//		this->Entity::w->addent(e);
		Vec3d newPos = pos + rot.trans(Vec3d(-0.45, 0, 0));
		e->setPosition(&newPos, &rot, &velo);

		// Send build phase complete message
		SetBuildPhaseCommand sbpc(1.);
		e->command(&sbpc);

		// Send move order to go forward
		MoveCommand com;
		com.destpos = newPos + rot.trans(Vec3d(0, 0, -1.2));
		e->command(&com);
	}
}

bool ShipyardDocker::undock(Entity::Dockable *pe){
	if(st::undock(pe)){
		QueryClassCommand com;
		com.ret = Docker::Fighter;
		pe->command(&com);
		Vec3d dockPos;
		if(com.ret == Docker::Fighter)
			dockPos = Vec3d(-.10, 0.05, 0);
		else if(com.ret == Docker::Frigate){
			dockPos = Vec3d(.10, 0.0, -.350);
			static_cast<Shipyard*>(e)->undockingFrigate = pe;
		}
		else
			dockPos = Vec3d(1.0, 0.0, 0);
		Vec3d pos = e->pos + e->rot.trans(dockPos);
		pe->setPosition(&pos, &e->rot, &e->velo, &e->omg);
		return true;
	}
	return false;
}

Vec3d ShipyardDocker::getPortPos(Dockable *e)const{
	QueryClassCommand com;
	com.ret = Fighter;
	e->command(&com);
	if(com.ret == Fighter)
		return Vec3d(-100. * Shipyard::modelScale, -50. * Shipyard::modelScale, -0.350);
	else if(com.ret == Frigate)
		return Vec3d(100. * Shipyard::modelScale, 0, 0.350);
}

Quatd ShipyardDocker::getPortRot(Dockable *)const{
	return quat_u;
}

Model *Shipyard::getModel(){
	static Model *model = LoadMQOModel("models/shipyard.mqo");
	return model;
}

#define PROFILE_HITPOLY 0

int Shipyard::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	return modelTraceHit(this, src, dir, rad, dt, ret, retp, retn, getModel());
}

int Shipyard::modelTraceHit(const Entity *e, const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn, const Model *model){
	if(!model)
		return 0;
#if PROFILE_HITPOLY
	timemeas_t tm;
	TimeMeasStart(&tm);
#endif
	Mat4d mat;
	e->transform(mat);
	Mat4d imat = mat.scalein(-1. / modelScale, 1. / modelScale, -1. / modelScale).transpose();
	mat.scalein(-modelScale, modelScale, -modelScale);
	Vec3d lsrc = imat.dvp3(src - e->pos);
	Vec3d ldir = imat.dvp3(dir);
	int bestiret = 0;
	double bestdret = dt;
	Vec3d bestlretn;

	for(int meshi = 0; meshi < model->n; meshi++){
		Mesh *mesh = model->sufs[meshi];
		for(int i = 0; i < mesh->np; i++){
			unsigned short indices[4];
			Mesh::Primitive *pr = mesh->p[i];
			int n = 0;
			if(pr->t == Mesh::ET_Polygon){
				Mesh::Polygon &p = pr->p;
				assert(p.n <= 4);
				n = p.n;
				for(int j = 0; j < p.n; j++)
					indices[p.n - j - 1] = p.v[j].pos; // Reverse face direction
			}
			else if(pr->t == Mesh::ET_UVPolygon){
				Mesh::UVPolygon &p = pr->uv;
				assert(p.n <= 4);
				n = p.n;
				for(int j = 0; j < p.n; j++)
					indices[p.n - j - 1] = p.v[j].pos; // Reverse face direction
			}
			else assert(false);
			double dret;
			Vec3d lretn;
			int iret = jHitPolygon(mesh->v, indices, n, lsrc, ldir, 0, dt, &dret, NULL, (double (*)[3])&lretn[0]);
			if(iret && dret < bestdret){
				bestiret = meshi * 0x10000 + i + 1;
				bestdret = dret;
				bestlretn = lretn;
			}
		}
	}
	if(bestiret){
		if(ret != NULL)
			*ret = bestdret;
		if(retp != NULL)
			*retp = src + dir * bestdret;
		if(retn != NULL)
			*retn = mat.dvp3(bestlretn);
	}
#if PROFILE_HITPOLY
	fprintf(stderr, "tracehit for %d: %lg\n", model->sufs[0]->np, TimeMeasLap(&tm));
#endif
	return bestiret;
}
#endif

#ifndef _WIN32
int Shipyard::popupMenu(PopupMenu &list){return st::popupMenu(list);}
void Shipyard::draw(WarDraw*){}
void Shipyard::drawtra(WarDraw*){}
void Shipyard::drawOverlay(wardraw_t *){}
void Shipyard::dyingEffects(double){}
void Shipyard::deathEffects(){}
#endif


IMPLEMENT_COMMAND(SetBuildPhaseCommand, "SetBuildPhase");

IMPLEMENT_COMMAND(GetFaceInfoCommand, "GetFaceInfo");
