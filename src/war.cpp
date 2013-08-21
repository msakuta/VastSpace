/** \file
 * \brief Implementation of WarField and WarSpace.
 */
#include "war.h"
#include "Game.h"
#include "Universe.h"
#include "Entity.h"
#include "Player.h"
#include "CoordSys.h"
#include "astro.h"
#include "judge.h"
#include "serial_util.h"
#include "cmd.h"
#include "sqadapt.h"
#include "btadapt.h"
#include "msg/Message.h"
#include "msg/GetCoverPointsMessage.h"
#include "tefpol3d.h"
#ifndef DEDICATED
#include "glw/GLWchart.h"
#endif
extern "C"{
#include <clib/mathdef.h>
#include <clib/timemeas.h>
}
//#include <GL/glext.h>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.h>
#include <BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.h>

#include <fstream>

// Recent Windows have POSIX compatibility, so we may not need this #ifndef.
#ifndef _WIN32
#include <sys/stat.h>
#endif




WarField::WarField(Game *g) : Serializable(g), cs(NULL), realtime(0){}

WarField::WarField(CoordSys *acs) : Serializable(acs ? acs->getGame() : NULL), cs(acs), realtime(0){
	init_rseq(&rs, 2426);
}

WarField::~WarField(){
	// Client could never delete a WarField.
	if(game->isServer()){
		// Even if it's the server, well-designed sequence would never cause a WarField containing
		// Entities to be deleted, which means the code below should not be necessary.
		// But I'm not sure it applies forever, so placed it here. At least we can put a breakpoint
		// for debugging.
		for(EntityList::iterator it = el.begin(); it != el.end();){
			EntityList::iterator next = it;
			++next;
			(*it)->transit_cs(cs->parent);
			it = next;
		}
	}
	else{ // If client's WarField is destructed, it's only by the server's instruction.
		for(EntityList::iterator it = el.begin(); it != el.end(); ++it){
			(*it)->w = NULL;
		}
		for(EntityList::iterator it = bl.begin(); it != bl.end(); ++it){
			(*it)->w = NULL;
		}
	}
}

const char *WarField::classname()const{
	return "WarField";
}

const unsigned WarField::classid = registerClass("WarField", Conster<WarField>);

#if 0 // reference
	Player *pl;
	Entity *el; /* entity list */
	Entity *bl; /* bullet list */
	Teline3List *tell, *gibs;
	struct TefpolList *tepl;
	struct random_sequence rs;
	int effects; /* trivial; telines or tefpols generated by this frame: to control performance */
	double realtime;
	double soundtime;
	otnt *ot, *otroot, *ottemp;
	int ots, oti;
	CoordSys *cs; /* redundant pointer to indicate belonging coordinate system */
#endif

void WarField::serialize(SerializeContext &sc){
	Serializable::serialize(sc);
	sc.o << el;
	sc.o << bl;
	sc.o << rs;
	sc.o << realtime;
	sc.o << cs;
}

void WarField::unserialize(UnserializeContext &sc){
	Serializable::unserialize(sc);
	sc.i >> el;
	sc.i >> bl;
	sc.i >> rs;
	sc.i >> realtime;
	sc.i >> cs;
}

void WarField::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext&)){
	(this->*method)(sc);
	for(EntityList::iterator it = el.begin(); it != el.end(); it++)
		(*it)->dive(sc, method);
	for(EntityList::iterator it = bl.begin(); it != bl.end(); it++)
		(*it)->dive(sc, method);
}

//static ObservePtr<WarField,0,Entity *WarField::*const list[2] = {&WarField::el, &WarField::bl};


void aaanim(double dt, WarField *w, WarField::EntityList WarField::*li, void (Entity::*method)(double)){
//	Player *pl = w->getPlayer();
	for(WarField::EntityList::iterator it = (w->*li).begin(); it != (w->*li).end();){
		WarField::EntityList::iterator next = it;
		++next;
		Entity *pe = *it;
		if(pe) try{
			// The object can be deleted in the method.
			(pe->*method)(dt);
		}
		catch(std::exception e){
			fprintf(stderr, __FILE__"(%d) Exception in %p->%s::anim(): %s\n", __LINE__, pe, pe->idname(), e.what());
		}
		catch(...){
			fprintf(stderr, __FILE__"(%d) Exception in %p->%s::anim(): ?\n", __LINE__, pe, pe->idname());
		}
//		if(pl->cs == w->cs && !pl->chase && (pe->pos - pl->getpos()).slen() < .002 * .002)
//			pl->chase = pe;
		it = next;
	}
}

#if 0
#define TRYBLOCK(a) {try{a;}catch(std::exception e){fprintf(stderr, __FILE__"(%d) Exception %s\n", __LINE__, e.what());}catch(...){fprintf(stderr, __FILE__"(%d) Exception ?\n", __LINE__);}}
#else
#define TRYBLOCK(a) (a);
#endif

void WarField::anim(double dt){
	aaanim(dt, this, &WarField::el, &Entity::callServerUpdate);
	aaanim(dt, this, &WarField::bl, &Entity::callServerUpdate);
}

void WarField::clientUpdate(double dt){
	aaanim(dt, this, &WarField::el, &Entity::callClientUpdate);
	aaanim(dt, this, &WarField::bl, &Entity::callClientUpdate);
}

void WarField::postframe(){
//	for(int i = 0; i < 2; i++)
//	for(Entity *e = this->*list[i]; e; e = e->next)
//		e->postframe();
}

bool WarField::EntityPtr::handleEvent(const Observable *o, ObserveEvent &e){
	if(TransitEvent *pe = InterpretEvent<TransitEvent>(e)){
		if(ptr){
			ptr->removeWeakPtr(this);
			ptr = NULL;
		}
		return true;
	}
	return false;
}

// Collision detection and resolution is handled by Bullet dynamics engine.
/*bool WarField::pointhit(const Vec3d &pos, const Vec3d &velo, double dt, struct contact_info*)const{
	return false;
}*/

double WarField::atmosphericPressure(const Vec3d &pos)const{
	return 0.; // Defaults in space
}

Vec3d WarField::accel(const Vec3d &srcpos, const Vec3d &srcvelo)const{
	return Vec3d(0,0,0);
}

double WarField::war_time()const{
	CoordSys *top = cs;
	for(; top->parent; top = top->parent);
	if(!top || !top->toUniverse())
		return 0.;
	return top->toUniverse()->global_time;
}

Teline3List *WarField::getTeline3d(){return NULL;}
TefpolList *WarField::getTefpol3d(){return NULL;}
WarField::operator WarSpace*(){return NULL;}
WarField::operator Docker*(){return NULL;}
bool WarField::sendMessage(Message &){return false;}

Entity *WarField::addent(Entity *e){
	if(e->isTargettable()){
//		WeakPtr<Entity> *plist = &el;
		e->w = this;
		e->addObserver(this);
		el.push_back(e);
		e->enterField(this); // This method is called after the object is brought into entity list.
	}
	else{
//		WeakPtr<Entity> *plist = &bl;
		e->w = this;
//		e->next = *plist;
		e->addObserver(this);
		bl.push_back(e);
		e->enterField(this); // This method is called after the object is brought into entity list.
	}
	return e;
}

Player *WarField::getPlayer(){
	return game->player;
}

bool WarField::unlink(const Observable *o){
	unlinkList(el, o);
	unlinkList(bl, o);
	return true;
}

bool WarField::handleEvent(const Observable *o, ObserveEvent &e){
	if(InterpretEvent<TransitEvent>(e)){
		// Just unlink transiting Entities
		unlink(o);
		return true;
	}
	else
		return Observer::handleEvent(o, e);
}

void WarField::unlinkList(EntityList &el, const Observable *o){
	EntityList::iterator it = el.begin();
	while(it != el.end()){
		EntityList::iterator next = it;
		++next;
		if(*it == o){
			// Entity CoordSys transition does not work if the line below is commented
			// by unknown reason.
			if(game->isServer())
				o->removeObserver(this);
			el.erase(it);
		}
		it = next;
	}
}





























class WarSpaceFilterCallback : public btOverlapFilterCallback{
	virtual bool needBroadphaseCollision(btBroadphaseProxy *proxy0, btBroadphaseProxy *proxy1)const{
		bool ret = btOverlapFilterCallback::needBroadphaseCollision(proxy0, proxy1);
		return ret;
	}
};


const char *WarSpace::classname()const{
	return "WarSpace";
}


const unsigned WarSpace::classid = registerClass("WarSpace", Conster<WarSpace>);

#if 0 // reference
	Player *pl;
	Entity *el; /* entity list */
	Entity *bl; /* bullet list */
	Teline3List *tell, *gibs;
	struct TefpolList *tepl;
	struct random_sequence rs;
	int effects; /* trivial; telines or tefpols generated by this frame: to control performance */
	double realtime;
	double soundtime;
	otnt *ot, *otroot, *ottemp;
	int ots, oti;
	CoordSys *cs; /* redundant pointer to indicate belonging coordinate system */
#endif

void WarSpace::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << effects << soundtime << cs;
}

static btDiscreteDynamicsWorld *bulletInit(){
	btDiscreteDynamicsWorld *btc = NULL;

	///collision configuration contains default setup for memory, collision setup
	btDefaultCollisionConfiguration *m_collisionConfiguration = new btDefaultCollisionConfiguration();
	//m_collisionConfiguration->setConvexConvexMultipointIterations();

	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	btCollisionDispatcher *m_dispatcher = new	btCollisionDispatcher(m_collisionConfiguration);

	btDbvtBroadphase *m_broadphase = new btDbvtBroadphase();

	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	btSequentialImpulseConstraintSolver* sol = new btSequentialImpulseConstraintSolver;
	btSequentialImpulseConstraintSolver* m_solver = sol;

	btc = new btDiscreteDynamicsWorld(m_dispatcher,m_broadphase,m_solver,m_collisionConfiguration);

	btc->setGravity(btVector3(0,0,0));

	return btc;
}


void WarSpace::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> effects >> soundtime >> cs;
	if(!bdw)
		bdw = bulletInit();
}

void WarSpace::init(){
#ifdef _WIN32
	tell = NewTeline3D(2048, 128, 128);
	gibs = NewTeline3D(1024, 128, 128);
	tepl = new TefpolList(128, 32, 32);
#endif
}

WarSpace::WarSpace(Game *game) : st(game), ot(NULL), otroot(NULL), oti(0), ots(0), bdw(NULL){
	init();
}


WarSpace::WarSpace(CoordSys *acs) : st(acs), ot(NULL), otroot(NULL), oti(0), ots(0),
	effects(0), soundtime(0), bdw(NULL)
{
	init();
	bdw = bulletInit();
}

static void delete_bdwtime_log(){
	static bool called = false;
	if(!called){
		remove("logs/bdwtime_s.log");
		remove("logs/bdwtime_c.log");
		called = true;
	}
}

void WarSpace::anim(double dt){
	CoordSys *root = cs;

	aaanim(dt, this, &WarField::el, &Entity::anim);
//	fprintf(stderr, "otbuild %p %p %p %d\n", this->ot, this->otroot, this->ottemp);

	bdw->stepSimulation(dt / 1., 0);

#ifdef _WIN32
	CreateDirectory("logs", NULL);
#else
	mkdir("logs", 0755);
#endif
	delete_bdwtime_log();
	std::ofstream ofs("logs/bdwtime_s.log", std::ostream::app);
	ofs << game->universe->global_time << "\t" << getid() << "\t" << dt << std::endl;

	TRYBLOCK(ot_build(this, dt));
	aaanim(dt, this, &WarField::bl, &Entity::anim);
	TRYBLOCK(ot_check(this, dt));

#ifndef DEDICATED
	const struct tent3d_line_debug *tld = Teline3DDebug(tell);
	GLWchart::addSampleToCharts("tellcount", tld->teline_c);
	TRYBLOCK(AnimTeline3D(tell, dt));
	TRYBLOCK(AnimTeline3D(gibs, dt));

	GLWchart::addSampleToCharts("teplcount", tepl->getDebug()->tefpol_c);
	GLWchart::addSampleToCharts("tevertcount", tepl->getDebug()->tevert_c);
	TRYBLOCK(tepl->anim(dt));
#endif
}

void WarSpace::clientUpdate(double dt){
	aaanim(dt, this, &WarField::el, &Entity::callClientUpdate);
	aaanim(dt, this, &WarField::bl, &Entity::callClientUpdate);

	bdw->stepSimulation(dt / 1., 0);

#ifdef _WIN32
	CreateDirectory("logs", NULL);
#else
	mkdir("logs", 0755);
#endif
	delete_bdwtime_log();
	std::ofstream ofs("logs/bdwtime_c.log", std::ostream::app);
	ofs << game->universe->global_time << "\t" << getid() << "\t" << dt << std::endl;

#ifndef DEDICATED
	const struct tent3d_line_debug *tld = Teline3DDebug(tell);
	GLWchart::addSampleToCharts("tellcount", tld->teline_c);
	TRYBLOCK(AnimTeline3D(tell, dt));
	TRYBLOCK(AnimTeline3D(gibs, dt));

	GLWchart::addSampleToCharts("teplcount", tepl->getDebug()->tefpol_c);
	GLWchart::addSampleToCharts("tevertcount", tepl->getDebug()->tevert_c);
	TRYBLOCK(tepl->anim(dt));
#endif
}

bool WarSpace::unlink(const Observable *o){
	st::unlink(o);

	// Object tree manages pointers in its own with WeaPtrs.
//	ot = (otnt*)realloc(ot, ots = 0);
//	otroot = NULL;
//	ot_build(this, 0.);
	return true;
}


#ifndef _WIN32
void WarField::draw(WarDraw*){}
void WarField::drawtra(WarDraw*){}
void WarField::drawOverlay(WarDraw*){}
void WarSpace::draw(WarDraw*){}
void WarSpace::drawtra(WarDraw*){}
void WarSpace::drawOverlay(WarDraw*){}
#endif

EXPORT btRigidBody *newbtRigidBody(const btRigidBody::btRigidBodyConstructionInfo &ci){
	return new btRigidBody(ci);
}


Teline3List *WarSpace::getTeline3d(){return tell;}
TefpolList *WarSpace::getTefpol3d(){return tepl;}
WarSpace::operator WarSpace *(){return this;}

/// Returns rotation that represents which direction should be up and which should be down.
Quatd WarSpace::orientation(const Vec3d &)const{
	return quat_u;
}

/// Returns rigid body object of the surrounding world in a WarSpace.
/// The world is probably static, solid object that has approximately infinite mass.
///
/// \return NULL if no world rigid body is available.
btRigidBody *WarSpace::worldBody(){
	return NULL;
}





static bool strless(const char *a, const char *b){
	return strcmp(a, b) < 0;
}


/// Using Constructor on First Use idiom, described in C++ FAQ lite.
std::map<const char *, MessageStatic*, bool (*)(const char *, const char *)> &Message::ctormap(){
	static std::map<const char *, MessageStatic*, bool (*)(const char *, const char *)> s(strless);
	return s;
}

bool Message::derived(MessageID)const{
	return false;
}



IMPLEMENT_MESSAGE(GetCoverPointsMessage, "GetCoverPoints")


GetCoverPointsMessage::GetCoverPointsMessage(HSQUIRRELVM v, Entity &){
}

