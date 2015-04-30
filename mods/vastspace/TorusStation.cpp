/** \file
 * \brief Implementation of TorusStation class.
 */
#include "TorusStation.h"
#include "EntityRegister.h"
#include "vastspace.h"
#include "ContainerHead.h"
#include "SpacePlane.h"
#include "CoordSys.h"
#include "Universe.h"
#include "Player.h"
#include "war.h"
#include "cmd.h"
#include "btadapt.h"
#include "judge.h"
#include "Game.h"
#include "serial_util.h"
#include "msg/GetCoverPointsMessage.h"
#include "ModelEntity.h"
#include "Soldier.h"
#include "tent3d.h"
extern "C"{
#include <clib/c.h>
#include <clib/mathdef.h>
#include <clib/amat3.h>
#include <clib/lzw/lzw.h>
#include <clib/timemeas.h>
}
#include <assert.h>
#include "BulletCollision/NarrowPhaseCollision/btGjkConvexCast.h"

using namespace gltestp;

double TorusStation::RAD = 130.; ///< outer radius
const double TorusStation::THICK = 100.; ///< Thickness of the mirrors
const double TorusStation::stackInterval = 50.; ///< The distance between torus stacks.
const int TorusStation::segmentCount = 8; ///< The count of station segments. This could be a non-static member to have stations with various sizes.
double TorusStation::defaultMass = 1e8;


static int spacecolony_rotation(const struct coordsys *, aquat_t retq, const avec3_t pos, const avec3_t pyr, const aquat_t srcq);


class TorusStationWarSpace : public WarSpace{
public:
	typedef WarSpace st;
	TorusStationWarSpace(Game *game) : st(game), bbody(NULL){}
	TorusStationWarSpace(TorusStation *cs);
	virtual Vec3d accel(const Vec3d &srcpos, const Vec3d &srcvelo)const;
	virtual Quatd orientation(const Vec3d &pos)const;
	virtual btRigidBody *worldBody();
	virtual bool sendMessage(Message &);

protected:
	btRigidBody *bbody;
};



//const unsigned TorusStation::classid = registerClass("TorusStation", Serializable::Conster<TorusStation>);
ClassRegister<TorusStation> TorusStation::classRegister("TorusStation");

TorusStation::TorusStation(Game *game) : st(game){
	init();
}

TorusStation::TorusStation(const char *path, CoordSys *root) : st(path, root){
	st::init(path, root);
	init();
	race = 0;
}

TorusStation::~TorusStation(){
	if(ent)
		delete ent;
}

void TorusStation::init(){
	static bool initialized = false;
	if(!initialized){
		SqInit(game->sqvm, modPath() << _SC("models/TorusStation.nut"),
			// RAD is not necessarily equivalent to hit radius, but it's easier to think that way.
			SingleDoubleProcess(RAD, "hitRadius", false) <<=
			SingleDoubleProcess(defaultMass, "mass")
			);
		initialized = true;
	}
	rotation = 0.;
	sun_phase = 0.;
	ent = NULL;
	headToSun = false;
	absmag = 30.;
	rad = RAD;
	orbit_home = NULL;
	mass = defaultMass;
	basecolor = Vec4f(1., .5, .5, 1.);
	omg.clear();
	
	gases = 100;
	solids = 100;
	people = 100000;
}

void TorusStation::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << ent;
	sc.o << rotation;
	sc.o << gases;
	sc.o << solids;
	sc.o << people;
}

void TorusStation::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> ent;
	sc.i >> rotation;
	sc.i >> gases;
	sc.i >> solids;
	sc.i >> people;
}

void TorusStation::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	st::dive(sc, method);
//	ent->dive(sc, method);
}


bool TorusStation::belongs(const Vec3d &lpos)const{
	double sdxy;
	sdxy = lpos[0] * lpos[0] + lpos[2] * lpos[2];
	return sdxy < RAD * RAD && -THICK < lpos[1] && lpos[1] < THICK;
}

static const Vec3d joint(3.25 * cos(M_PI * 3. / 6. + M_PI / 6.), 16., 3.25 * sin(M_PI * 3. / 6. + M_PI / 6.));


Mat4d TorusStation::transform(const Viewer *vw)const{
	Mat4d ret;
	if(vw->zslice != 0){
		Quatd rot = vw->cs->tocsq(parent).cnj();
		ret = rot.tomat4();
		Vec3d delta = vw->cs->tocs(pos, parent) - vw->pos;
		ret = ret * this->rot.tomat4();
		ret.vec3(3) = delta;
	}
	else{
		ret = mat4_u;
		if(vw->cs == this || !ent || !ent->bbody){
			Mat4d rot = vw->cs->tocsim(this);
			ret.translatein(vw->cs->tocs(vec3_000, this));
			ret = ret * rot;
		}
		else{
			Mat4d rot = vw->cs->tocsim(parent);
			Mat4<btScalar> btrot;
			ent->bbody->getWorldTransform().getOpenGLMatrix(btrot);
			ret.translatein(vw->cs->tocs(vec3_000, parent));
			ret = ret * rot * btrot.cast<double>();
		}
	}
	return ret;
}

void TorusStation::anim(double dt){
	Astrobj *sun = findBrightest();

	// Heading toward sun is unnecessary, it's not Island3.
/*	if(sun)*/{
/*		if(!headToSun){
			headToSun = true;
			rot = Quatd::direction(parent->tocs(pos, sun)).rotate(-M_PI / 2., avec3_100);
		}
		Vec3d sunpos = parent->tocs(pos, sun);
		CoordSys *top = findcspath("/");*/
//		double phase = omg[1] * (!top || !top->toUniverse() ? 0. : top->toUniverse()->global_time);
//		Quatd qrot = Quatd::direction(sunpos);
//		Quatd qrot1 = qrot.rotate(-M_PI / 2., avec3_100);
		double omg = sqrt(9.8 / RAD);
		Vec3d vomg = Vec3d(0, 0, omg);
		this->rotation += omg * dt;
		this->omg = this->rot.trans(vomg)/* + sunpos.norm().vp(rot.trans(Vec3d(0,0,1))) * .1*/;
//		this->rot = Quatd::rotation(this->omg.len() * dt, this->omg.norm()) * this->rot;
//		this->rot = this->rot.quatrotquat(this->omg * dt);
//		this->rot = qrot1.rotate(phase, avec3_010);
//		this->omg.clear();
	}

	// Calculate phase of the simulated sun.
	sun_phase += 2 * M_PI / 24. / 60. / 60. * dt;

	if(!parent->w){
		if(game->isServer())
			parent->w = new WarSpace(parent);
		else // Client won't create any Entity.
			return;
	}
	WarSpace *ws = *parent->w;
	if(ws && !ent && game->isServer()){
		ent = new TorusStationEntity(ws, *this);
		ws->addent(ent);
	}
	if(ws && ent){
		ent->pos = this->pos;
		ent->rot = this->rot;
		ent->velo = this->velo;
		ent->omg = this->omg;
		ent->mass = this->mass;
		if(ws->bdw){
			if(ent->bbody){
				// This is now a task of EntityMotionState.
/*				ent->bbody->setWorldTransform(btTransform(btqc(rot), btvc(pos)));
				ent->bbody->setLinearVelocity(btvc(ent->velo));
				ent->bbody->setAngularVelocity(btvc(ent->omg));*/
			}
			else
				ent->buildShape();
		}
	}
	// Super class's methods do not assume a client can run.
	if(game->isServer())
		st::anim(dt);
}

void TorusStation::clientUpdate(double dt){
	anim(dt);
}









Entity::EntityRegisterNC<TorusStationEntity> TorusStationEntity::entityRegister("TorusStationEntity");

const double TorusStationEntity::hubRadius = 30.; // Really should be derived from hub model
const double TorusStationEntity::segmentOffset = 20.; // Offset of model from TorusStation::RAD
const double TorusStationEntity::segmentBaseHeight = 10.; // Really should be derived from segment model
double TorusStationEntity::modelScale = 0.1;
double TorusStationEntity::hitRadius = 130.;
double TorusStationEntity::maxHealthValue = 1e6;
GLuint TorusStationEntity::overlayDisp = 0;

Entity::EntityStatic &TorusStationEntity::getStatic()const{return entityRegister;}

void TorusStationEntity::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << astro;
	sc.o << docker;
}

void TorusStationEntity::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> astro;
	sc.i >> docker;
}

void TorusStationEntity::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	st::dive(sc, method);
	docker->dive(sc, method);
}

void TorusStationEntity::enterField(WarField *w){
	WarSpace *ws = *w;
	if(ws && ws->bdw){
		buildShape();
		// Adding the body to the dynamics world will be done in st::enterField().
//		if(bbody)
//			ws->bdw->addRigidBody(bbody);
	}
	st::enterField(w);
}


void TorusStationEntity::buildShape(){
	WarSpace *ws = *w;
	if(ws){
		if(!btshape){
			/// A temporary closure that is made because it's called 3 times in this function
			/// in different ways.
			struct{
				TorusStationEntity *e;
				void process(const Vec3d &sc, const Quatd &rot, const Vec3d &pos){
					const Vec3d tpos = rot.trans(pos);
					btBoxShape *box = new btBoxShape(btvc(sc));
					btTransform trans = btTransform(btqc(rot), btvc(tpos));
					e->btshape->addChildShape(trans, box);
					box->setMargin(0.04); // default margin

					// Add a hitbox just like btBoxShape for bullet hit testing.
					e->hitboxes.push_back(hitbox(tpos, rot, sc));
				}
			} shapeProc;
			shapeProc.e = this;

			btshape = new btCompoundShape();

			// The hub's Bullet shape. Note that it's divided into 3 in form of hitcylinders.
			const double radius = 30.;
			btvc hubsc = Vec3d(radius, radius, 36. + TorusStation::stackInterval * TorusStation::stackCount / 2.);
//			btBoxShape *cyl = new btBoxShape(hubsc); // Test box shape for visualization
			btCylinderShape *cyl = new btCylinderShapeZ(hubsc);
			cyl->setMargin(0.04 * 0.01); // 1/100 of default margin
			btshape->addChildShape(btTransform(btqc(Quatd(0,0,0,1)), btvc(Vec3d(0,0,0))), cyl);

			// The hub's main shaft rotates along with the wheels, so we must separate it from the ports.
			hitcylinders.push_back(HitCylinder(Vec3d(0,0,0), Vec3d(0,0, TorusStation::stackInterval * TorusStation::stackCount / 2.), radius));

			// The ports at both ends of the hub. Add to hitcylinders after the hub to make their hit part greater than portHitPartOffset.
			hitcylinders.push_back(HitCylinder(Vec3d(0,0, 18. + TorusStation::stackInterval * TorusStation::stackCount / 2.), Vec3d(0,0, 18.), radius));
			hitcylinders.push_back(HitCylinder(Vec3d(0,0, -(18. + TorusStation::stackInterval * TorusStation::stackCount / 2.)), Vec3d(0,0, 18.), radius));

			const int stackCount = TorusStation::stackCount;
			for(int n = 0; n < stackCount; n++){
				double zpos = TorusStation::getZOffsetStack(n);
				const int segmentCount = TorusStation::segmentCount;
				for(int i = 0; i < segmentCount; i++){
					// The residence segments
					shapeProc.process(Vec3d(40., 10., 10.), Quatd::rotation(i * 2 * M_PI / segmentCount, 0, 0, 1),
						Vec3d(0, -TorusStation::RAD + segmentOffset, zpos));

					// The joints connecting residence segments
					shapeProc.process(Vec3d(15., 8., 8.),
						Quatd::rotation((i * 2 + 1) * M_PI / segmentCount, 0, 0, 1),
						Vec3d(0, (-TorusStation::RAD + segmentOffset + 5.) / cos(M_PI / segmentCount), zpos));

					// The spokes
					shapeProc.process(Vec3d(5., (TorusStation::RAD - hubRadius - segmentBaseHeight - segmentOffset) / 2., 5.),
						Quatd::rotation(i * 2 * M_PI / segmentCount, 0, 0, 1),
						Vec3d(0, (-TorusStation::RAD + hubRadius + segmentBaseHeight + segmentOffset) / 2. - hubRadius, zpos));
				}
			}
		}
//		btTransform startTransform;
//		startTransform.setIdentity();
//		startTransform.setOrigin(btvc(pos));

		// The station has a mass as an astronomical object of course, but it doesn't
		// in Bullet dynamics world.
		double mass = 0;

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			btshape->calculateLocalInertia(mass,localInertia);

		/// Motion state that transports Bullet's world transform to Entity and
		/// vice versa.
		class EntityMotionState : public btMotionState{
		public:
			/// The Entity should never be deleted before bbody, so this may not be
			/// necessary to be a WeakPtr.
			WeakPtr<Entity> e;
			EntityMotionState(Entity *e) : e(e){}
			void getWorldTransform(btTransform &tr)const{
				if(e){
					tr.setOrigin(btvc(e->pos));
					tr.setRotation(btqc(e->rot));
				}
			}
			void setWorldTransform(const btTransform &tr){
				if(e){
					e->pos = btvc(tr.getOrigin());
					e->rot = btqc(tr.getRotation());
				}
			}
		};

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		// ...
		// Honestly, although the example codes recommend using motion states, I don't feel like using them,
		// because we can update the bbody in every frame in anim() and currently Bullet's simulation step is synchronized
		// with the Game engine (thus no interpolation is needed). It's just waste of CPU time and memory to manage
		// motion states. It also costs effort to maintain excess codes (I haven't understood what
		// motion states are provided for for years).
		// But they're required for using the function that the Bullet SDK manual calls kinematic object, so I couldn't
		// help it.
		EntityMotionState* myMotionState = new EntityMotionState(this);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,btshape,localInertia);
		bbody = new btRigidBody(rbInfo);

		// The station structure is kinematic object, which means it can move but never affected by colliding objects.
		bbody->setCollisionFlags(bbody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		bbody->setActivationState(DISABLE_DEACTIVATION);
	}
}

/// \brief An utility method that returns if the Entity can be culled in rendering of viewing volume.
/// \returns true if the object is culled
bool TorusStation::cull(const Viewer &vw)const{
#ifndef DEDICATED
	GLcull *gc = vw.gc, *gc2 = vw.gclist[0];

	Vec3d pos = vw.cs->tocs(vec3_000, this);
	if(gc->cullFrustum(pos, RAD * 2.))
		return true;
	double pixels = fabs(gc->scale(pos)) * RAD;
	if(pixels < 1)
		return true;
	return false;
#endif
}





void TorusStation::onChangeParent(CoordSys *oldParent){
	if(ent){
		if(!parent->w)
			parent->w = new WarSpace(parent);
		parent->w->addent(ent);
	}
}

bool TorusStation::readFile(StellarContext &sc, int argc, const char *argv[]){
	if(!strcmp(argv[0], "race")){
		if(1 < argc)
			race = atoi(argv[1]);
		return true;
	}
	else
		return st::readFile(sc, argc, argv);
}




TorusStationEntity::TorusStationEntity(Game *game) : st(game), btshape(NULL){
	init();
}

TorusStationEntity::TorusStationEntity(WarField *w, TorusStation &astro) : st(w), astro(&astro), btshape(NULL){
	init();
	health = getMaxHealth();
	race = astro.race;
	docker = new TorusStationDocker(this);
}

TorusStationEntity::~TorusStationEntity(){
	astro->ent = NULL;
	delete btshape;
}

void TorusStationEntity::init(){
	static bool initialized = false;
	if(!initialized){
		// Initialization with the script is totally redundant in that it's interpreted in
		// TorusStation::init() too. But we couldn't help having two-pass initialization
		// because TorusStation (CoordSys) and TorusStationEntity (Entity) have different
		// timing for construction in the startup process. 
		// We must defer invocation of DrawOverlayProcess until OpenGL context is initialized.
		// Also, if we happen to have no corresponding Entity for this TorusStation, we don't
		// need to load overlayDisp.
		SqInit(game->sqvm, modPath() << _SC("models/TorusStation.nut"),
			SingleDoubleProcess(modelScale, "modelScale") <<=
			SingleDoubleProcess(hitRadius, "hitRadius", false) <<=
			SingleDoubleProcess(maxHealthValue, "maxhealth", false) <<=
			ModelEntity::DrawOverlayProcess(overlayDisp)
			);
		initialized = true;
	}
}

int TorusStationEntity::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	double best = dt;
	int reti = 0;
	double retf;
	// Test hits with hitboxes.
	for(int n = 0; n < hitboxes.size(); n++){
		hitbox &hb = hitboxes[n];
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
			reti = n + 1;
		}
	}
	for(int n = 0; n < hitcylinders.size(); n++){
		HitCylinder &hb = hitcylinders[n];
		Vec3d org = this->rot.trans(hb.org) + this->pos;
		Vec3d axis = this->rot.trans(hb.axis);
		if((jHitCylinderPos(org, axis, hb.radius, src, dir, best, &retf, retp, retn, 0)) && (retf < best)){
			best = retf;
			if(ret) *ret = retf;
			reti = n + portHitPartOffset;
		}
	}
	return reti;
}

int TorusStationEntity::takedamage(double damage, int hitpart){
	int ret = 1;
	if(0 < health && health - damage <= 0){
		ret = 0;
#ifndef DEDICATED
		WarSpace *ws = *w;
		if(ws){
			AddTeline3D(ws->tell, this->pos, vec3_000, 30000., quat_u, vec3_000, vec3_000, COLOR32RGBA(255,255,255,127), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_INVROTATE, 2.);
		}
#endif
		astro->flags |= CS_DELETE;
		w = NULL;
	}
	health -= damage;
	return ret;
}


Docker *TorusStationEntity::getDockerInt(){
	return docker;
}

bool TorusStationEntity::command(EntityCommand *com){
	if(HookPosLocalToWorldCommand *lwcom = InterpretCommand<HookPosLocalToWorldCommand>(com)){
		// The +1 is for hub shaft, which rotates along with the wheels.
		if(portHitPartOffset + 1 <= lwcom->hitpart){
			Quatd lrot = rot.rotate(-astro->rotation, 0, 0, 1);
			lwcom->pos = pos + lrot.trans(*lwcom->srcpos);
			return true;
		}
		else
			return st::command(com);
	}
	else if(HookPosWorldToLocalCommand *lwcom = InterpretCommand<HookPosWorldToLocalCommand>(com)){
		// The +1 is for hub shaft, which rotates along with the wheels.
		if(portHitPartOffset + 1 <= lwcom->hitpart){
			Quatd lrot = rot.rotate(-astro->rotation, 0, 0, 1);
			lwcom->pos = lrot.cnj().trans(*lwcom->srcpos - pos);
			return true;
		}
		else
			return st::command(com);
	}
	else
		return st::command(com);
}



static const double cutheight = 200.;
static const double floorHeight = 3.75;
static const double floorStep = .5;



const unsigned TorusStationDocker::classid = registerClass("TorusStationDocker", Conster<TorusStationDocker>);

TorusStationDocker::TorusStationDocker(TorusStationEntity *ent) : st(ent){
}

const char *TorusStationDocker::classname()const{return "TorusStationDocker";}


void TorusStationDocker::dock(Dockable *d){
	d->dock(this);
	if(!strcmp(d->idname(), "ContainerHead") || !strcmp(d->idname(), "SpacePlane"))
		d->w = NULL;
	else{
		if(e->w)
			e->w = this;
		else{
			e->w = this;
			addent(e);
		}
	}
}

Vec3d TorusStationDocker::getPortPos(Dockable *)const{
	Vec3d yhat = e->rot.trans(Vec3d(0,1,0));
	return yhat * (-16. - 3.25) + e->pos;
}

Quatd TorusStationDocker::getPortRot(Dockable *)const{
	return e->rot;
}


TorusStationWarSpace::TorusStationWarSpace(TorusStation *cs) : st(cs), bbody(NULL){
	if(bdw){
		static btCompoundShape *shape = NULL;
		if(!shape){
			shape = new btCompoundShape();

			static const double radius[2][2] = {
				{200. + TorusStation::RAD / 2., 200. + TorusStation::RAD  / 2.},
				{TorusStation::RAD  + .5 - 1., TorusStation::RAD  + .5 - 1.},
			};
			static const double thickness[2] = {
				TorusStation::RAD  / 2. + 9.,
				500.,
			};
			static const double widths[2] = {
				130. * 8,
				130.,
			};
			static const double lengths[2][2] = {
				{-TorusStation::THICK  - TorusStation::RAD, -TorusStation::THICK},
				{-TorusStation::THICK, TorusStation::THICK},
			};
			static const int cuts_n[2] = {12, 96};

			for(int n = 0; n < 2; n++){
				const int cuts = cuts_n[n];
				for(int i = 0; i < cuts; i++){
					const Vec3d sc(widths[n], (lengths[n][1] - lengths[n][0]) / 2., thickness[n]);
					const Quatd rot = Quatd::rotation(2 * M_PI * (i + .5) / cuts, 0, 1, 0);
					const Vec3d pos = rot.trans(Vec3d(0, (lengths[n][1] + lengths[n][0]) / 2., radius[n][(i + cuts / 12) % (cuts / 3) < cuts / 6]));
					btBoxShape *box = new btBoxShape(btvc(sc));
					btTransform trans = btTransform(btqc(rot), btvc(pos));
					shape->addChildShape(trans, box);
				}
			}

			btBoxShape *endbox = new btBoxShape(btVector3(TorusStation::RAD, TorusStation::RAD / 2., TorusStation::RAD));
			shape->addChildShape(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, TorusStation::THICK + TorusStation::RAD / 2., 0)), endbox);
		}
		btTransform startTransform;
		startTransform.setIdentity();
//		startTransform.setOrigin(btvc(pos));

		// Assume Island hull is static
		btVector3 localInertia(0,0,0);

		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(0,myMotionState,shape,localInertia);
		bbody = new btRigidBody(rbInfo);

		//add the body to the dynamics world
		bdw->addRigidBody(bbody);
	}
}

Vec3d TorusStationWarSpace::accel(const Vec3d &srcpos, const Vec3d &srcvelo)const{
	double omg2 = cs->omg.slen();

	/* centrifugal force */
	Vec3d ret(srcpos[0] * omg2, 0., srcpos[2] * omg2);

	/* coriolis force */
	Vec3d coriolis = srcvelo.vp(sqrt(omg2) * Vec3d(0,1,0));
	ret += coriolis;
	return ret;
}

Quatd TorusStationWarSpace::orientation(const Vec3d &pos)const{
	static const Quatd org(-sqrt(2.) / 2., 0, 0, sqrt(2.) / 2.);
	double phase = atan2(pos[0], pos[2]);
	return org.rotate(phase, 0, 0, 1);
}

btRigidBody *TorusStationWarSpace::worldBody(){
	return bbody;
}

bool TorusStationWarSpace::sendMessage(Message &com){
	if(GetCoverPointsMessage *p = InterpretMessage<GetCoverPointsMessage>(com)){
		CoverPointVector &ret = p->cpv;
		TorusStation *i3 = static_cast<TorusStation*>(cs);
		return true;
	}
	else
		return false;
}


#ifdef DEDICATED
void TorusStation::predraw(const Viewer *vw){}
void TorusStation::draw(const Viewer *vw){}
void TorusStation::drawtra(const Viewer *vw){}
void TorusStationEntity::draw(WarDraw *){}
void TorusStationEntity::drawOverlay(WarDraw *){}
Entity::Props TorusStationEntity::props()const{return st::props();}
int TorusStationEntity::popupMenu(PopupMenu &list){return 0;}
#endif

