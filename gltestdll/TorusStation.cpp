/** \file
 * \brief Implementation of TorusStation class.
 */
#include "TorusStation.h"
#include "ContainerHead.h"
#include "SpacePlane.h"
#include "astrodraw.h"
#include "CoordSys.h"
#include "Universe.h"
#include "Player.h"
#include "war.h"
#include "glsl.h"
#include "glstack.h"
#include "draw/material.h"
#include "cmd.h"
#include "btadapt.h"
#include "judge.h"
#include "bitmap.h"
#include "Game.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/ShadowMap.h"
#include "draw/ShaderBind.h"
#include "glw/popup.h"
#include "serial_util.h"
#include "msg/GetCoverPointsMessage.h"
extern "C"{
#include <clib/c.h>
#include <clib/gl/multitex.h>
#include <clib/amat3.h>
#include <clib/suf/suf.h>
#include <clib/suf/sufbin.h>
#include <clib/gl/gldraw.h>
#include <clib/gl/cull.h>
#include <clib/lzw/lzw.h>
#include <clib/timemeas.h>
}
#include <gl/glu.h>
#include <gl/glext.h>
#include <assert.h>
#include "BulletCollision/NarrowPhaseCollision/btGjkConvexCast.h"

using namespace gltestp;

const double TorusStation::RAD = 0.3; ///< outer radius
const double TorusStation::THICK = 0.1; ///< Thickness of the mirrors


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
int &TorusStation::g_shader_enable = ::g_shader_enable;

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
	rotation = 0.;
	sun_phase = 0.;
	ent = NULL;
	headToSun = false;
	absmag = 30.;
	rad = 100.;
	orbit_home = NULL;
	mass = 1e10;
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
			Mat4d btrot;
			ent->bbody->getWorldTransform().getOpenGLMatrix(btrot);
			ret.translatein(vw->cs->tocs(vec3_000, parent));
			ret = ret * rot * btrot;
		}
	}
	return ret;
}

void TorusStation::anim(double dt){
	Astrobj *sun = findBrightest();

	// Head toward sun
	if(sun){
		if(!headToSun){
			headToSun = true;
			rot = Quatd::direction(parent->tocs(pos, sun)).rotate(-M_PI / 2., avec3_100);
		}
		Vec3d sunpos = parent->tocs(pos, sun);
		CoordSys *top = findcspath("/");
//		double phase = omg[1] * (!top || !top->toUniverse() ? 0. : top->toUniverse()->global_time);
//		Quatd qrot = Quatd::direction(sunpos);
//		Quatd qrot1 = qrot.rotate(-M_PI / 2., avec3_100);
		double omg = sqrt(.0098 / 3.25);
		Vec3d vomg = Vec3d(0, omg, 0);
		this->rotation += omg * dt;
		this->omg = this->rot.trans(vomg) + sunpos.norm().vp(rot.trans(Vec3d(0,1,0))) * .1;
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
	if(ws && ent && game->isServer()){
		ent->pos = this->pos;
		ent->rot = this->rot;
		ent->velo = this->velo;
		ent->omg = this->omg;
		ent->mass = this->mass;

		RandomSequence rs((unsigned long)this + (unsigned long)(ws->war_time() / .0001));

		// Randomly create container heads
		// temporarily disabled until being accepted in the server-client model.
		if(false && floor(ws->war_time()) < floor(ws->war_time() + dt) && rs.nextd() < 0.02){
			Entity *ch = rs.next() % 2 ? (Entity*)(new ContainerHead(this->ent)) : new SpacePlane(this->ent);
			ch->race = race;
			ws->addent(ch);
			Vec3d rpos = this->rot.trans(Vec3d(0, -16. - 3.25, 0.));
			ch->pos = rpos + this->pos + .1 * Vec3d(rs.nextGauss(), rs.nextGauss(), rs.nextGauss());
			ch->rot = this->rot.rotate(-M_PI / 2., Vec3d(1,0,0));
			ch->velo = this->velo + this->omg.vp(rpos);
			ch->omg = this->omg;
			ch->setPosition(&ch->pos, &ch->rot, &ch->velo, &ch->omg);
			ch->undock(this->ent->docker);
		}
	}
	if(ws && ws->bdw && ent) if(!game->isServer() && !ent->bbody){
		// Client won't create bbody, just assign identity transform to wingtrans.
		for(int n = 0; n < 3; n++){
			ent->wingtrans[n] = btTransform::getIdentity();
		}
	}
	else{
		if(!ent->bbody)
			ent->buildShape();
	}
	// Super class's methods do not assume a client can run.
	if(game->isServer())
		st::anim(dt);
}

void TorusStation::clientUpdate(double dt){
	anim(dt);
}

static const avec3_t pos0[] = {
	{0., 17., 0.},
	{3.25, 16., 0.},
	{3.25, 12., 0.},
	{3.25, 8., 0.},
	{3.25, 4., 0.},
	{3.25, .0, 0.},
	{3.25, -4., 0.},
	{3.25, -8., 0.},
	{3.25, -12., 0.},
	{3.25, -16., 0.},
	{0., -17., 0.},
};








unsigned TorusStationEntity::classid = registerClass("TorusStationEntity", Conster<TorusStationEntity>);


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
			btshape = new btCompoundShape();

			btCylinderShape *cyl = new btCylinderShape(btVector3(TorusStation::RAD + .01, TorusStation::THICK, TorusStation::RAD + .01));
			btshape->addChildShape(btTransform(btqc(Quatd(0,0,0,1)), btvc(Vec3d(0,0,0))), cyl);
		}
		btTransform startTransform;
		startTransform.setIdentity();
		startTransform.setOrigin(btvc(pos));

		mass = 1e10;

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			btshape->calculateLocalInertia(mass,localInertia);

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,btshape,localInertia);
//		rbInfo.m_linearDamping = .5;
//		rbInfo.m_angularDamping = .5;
		bbody = new btRigidBody(rbInfo);

//		bbody->setSleepingThresholds(.0001, .0001);

		// Adding the body to the dynamics world will be done in st::enterField().
//			ws->bdw->addRigidBody(bbody, 1, ~2);
//		ws->bdw->addRigidBody(bbody);
	}
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
}

TorusStationEntity::TorusStationEntity(WarField *w, TorusStation &astro) : st(w), astro(&astro), btshape(NULL){
	health = maxhealth();
	race = astro.race;
	docker = new TorusStationDocker(this);
}

TorusStationEntity::~TorusStationEntity(){
	astro->ent = NULL;
	delete btshape;
}

int TorusStationEntity::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
#if 1
	double best = dt, retf;
	int reti = 0, n;
#endif
	if(w && bbody){
		btScalar btfraction;
		btVector3 btnormal, btpos;
		btVector3 from = btvc(src);
		btVector3 to = btvc(src + (dir - velo) * dt);
		if((from - to).length() < 1e-10);
		else if(WarSpace *ws = *w){
			btCollisionWorld::ClosestRayResultCallback callback(from, to);
			ws->bdw->rayTest(from, to, callback);
			if(callback.hasHit() && callback.m_collisionObject == bbody){
				if(ret) *ret = callback.m_closestHitFraction * dt;
				if(retp) *retp = btvc(callback.m_hitPointWorld);
				if(retn) *retn = btvc(callback.m_hitNormalWorld);
				return 1;
			}
		}
/*		else if(singleObjectRaytest(bbody, from, to, btfraction, btnormal, btpos)){
			if(ret) *ret = btfraction * dt;
			if(retp) *retp = btvc(btpos);
			if(retn) *retn = btvc(btnormal);
			return 1;
		}*/
	}
	return 0;
}

int TorusStationEntity::takedamage(double damage, int hitpart){
	int ret = 1;
	if(0 < health && health - damage <= 0){
		ret = 0;
		WarSpace *ws = *w;
		if(ws){
			AddTeline3D(ws->tell, this->pos, vec3_000, 30., quat_u, vec3_000, vec3_000, COLOR32RGBA(255,255,255,127), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_INVROTATE, 2.);
		}
		astro->flags |= CS_DELETE;
		w = NULL;
	}
	health -= damage;
	return ret;
}


Docker *TorusStationEntity::getDockerInt(){
	return docker;
}


static const double cutheight = .2;
static const double floorHeight = .00375;
static const double floorStep = .0005;

static const GLdouble vertex[][3] = {
  { -1.0, 0.0, -1.0 },
  { 1.0, 0.0, -1.0 },
  { 1.0, 1.0, -1.0 },
  { -1.0, 1.0, -1.0 },
  { -1.0, 0.0, 1.0 },
  { 1.0, 0.0, 1.0 },
  { 1.0, 1.0, 1.0 },
  { -1.0, 1.0, 1.0 }
};

static const int edges[][2] = {
  { 0, 1 },
  { 1, 2 },
  { 2, 3 },
  { 3, 0 },
  { 4, 5 },
  { 5, 6 },
  { 6, 7 },
  { 7, 4 },
  { 0, 4 },
  { 1, 5 },
  { 2, 6 },
  { 3, 7 }
};

static const int face[][4] = {
	{2,3,7,6},
/*	{0,1,5,4},*/

	{0,3,2,1},
	{5,6,7,4},
	{4,7,3,0},
	{1,2,6,5},

	{1,2,3,0},
	{4,7,6,5},
	{0,3,7,4},
	{5,6,2,1},
};
static const double normal[][3] = {
	{ 0.,  1.,  0.},
/*	{ 0., -1.,  0.},*/

	{ 0.,  0., -1.},
	{ 0.,  0.,  1.},
	{-1.,  0.,  0.},
	{ 1.,  0.,  0.},

	{ 0.,  0.,  1.},
	{ 0.,  0., -1.},
	{ 1.,  0.,  0.},
	{-1.,  0.,  0.},

	{ 0., -1.,  0.},
};
static const double texcoord[][3] = {
	{ 0.,  0., 0.},
	{ 0.,  1., 0.},
	{ 1.,  1., 0.},
	{ 1.,  0., 0.},
};


bool TorusStationEntity::command(EntityCommand *com){
	if(TransportResourceCommand *trc = InterpretCommand<TransportResourceCommand>(com)){
		astro->gases += trc->gases;
		astro->solids += trc->solids;
		return true;
	}
	if(TransportPeopleCommand *tpc = InterpretCommand<TransportPeopleCommand>(com)){
		astro->people += tpc->people;
		return true;
	}
	else		
		return st::command(com);
}



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
				{.2 + TorusStation::RAD / 2., .2 + TorusStation::RAD  / 2.},
				{TorusStation::RAD  + .5 - .001, TorusStation::RAD  + .5 - .001},
			};
			static const double thickness[2] = {
				TorusStation::RAD  / 2. + .009,
				.5,
			};
			static const double widths[2] = {
				.13 * 8,
				.13,
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


