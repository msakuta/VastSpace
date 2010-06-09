#include "entity.h"
#include "EntityCommand.h"
extern "C"{
#include <clib/aquat.h>
}
#include "Beamer.h"
#include "Assault.h"
#include "judge.h"
#include "player.h"
#include "cmd.h"
#include "sceptor.h"
#include "Scarry.h"
#include "RStation.h"
#include "glwindow.h"
#include "serial_util.h"
#include "Destroyer.h"
#include "btadapt.h"
#include "sqadapt.h"
#include "stellar_file.h"
#include <btBulletDynamicsCommon.h>


Entity::Entity(WarField *aw) : pos(vec3_000), velo(vec3_000), omg(vec3_000), rot(quat_u), mass(1e3), moi(1e1), enemy(NULL), w(aw), inputs(), health(1), race(0), otflag(0), bbody(NULL){
//	The entity must be constructed before being contained by some surroundings.
//  This generic problem is fairly difficult whether the object should be constructed before being assigned to its container.
//  Objects like Entities in this program are always tied to a WarField and assumed valid only if containing WarField is defined.
//  Then, should we assign an Entity to a WarField in its construction? Previous answer is yes, and our answer is no later on.
//  The reason is that the Entity-derived object's vft is not pointing to derived class's vft here.
//  The user must call WarField::addent() explicitly to let an object attend to a WarField.
//	if(aw)
//		aw->addent(this);
}

Entity::~Entity(){
	if(bbody){
		WarSpace *ws;
		if(w && (ws = *w) && ws->bdw)
			ws->bdw->removeRigidBody(bbody);
		delete bbody;
	}
}

void Entity::init(){
	health = maxhealth();
}


Entity *Entity::create(const char *cname, WarField *w){
	int i;
	Entity *(*ctor)(WarField*) = entityCtorMap()[cname];
	if(!ctor)
		return NULL;
	Entity *e = ctor(w);
	if(e){
		w->addent(e);
	}
	return e;
}

unsigned Entity::registerEntity(std::string name, Entity *(*ctor)(WarField *)){
	EntityCtorMap &ctormap = entityCtorMap();
	if(ctormap.find(name) != ctormap.end())
		CmdPrintf(cpplib::dstring("WARNING: Duplicate class name: ") << name.c_str());
	ctormap[name] = ctor;
	return ctormap.size();
}

Entity::EntityCtorMap &Entity::entityCtorMap(){
	static EntityCtorMap ictormap;
	return ictormap;
}


const char *Entity::idname()const{
	return "entity";
}

const char *Entity::classname()const{
	return "Entity";
}

const char *Entity::dispname()const{
	return classname();
}

#if 0 // reference
	Vec3d pos;
	Vec3d velo;
	Vec3d omg;
	Quatd rot; /* rotation expressed in quaternion */
	double mass; /* [kg] */
	double moi;  /* moment of inertia, [kg m^2] should be a tensor */
//	double turrety, barrelp;
//	double desired[2];
	double health;
//	double cooldown, cooldown2;
	Entity *next;
	Entity *selectnext; /* selection list */
	Entity *enemy;
	int race;
//	int shoots, shoots2, kills, deaths;
	input_t inputs;
	WarField *w; // belonging WarField, NULL means being bestroyed. Assigning another WarField marks it to transit to new CoordSys.
	int otflag;
#endif

void Entity::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << w;
	sc.o << pos;
	sc.o << velo;
	sc.o << omg;
	sc.o << rot;
	sc.o << mass;
	sc.o << moi;
	sc.o << health;
	sc.o << next << selectnext;
	sc.o << enemy;
	sc.o << race;
	sc.o << otflag;
}

void Entity::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> w;
	sc.i >> pos;
	sc.i >> velo;
	sc.i >> omg;
	sc.i >> rot;
	sc.i >> mass;
	sc.i >> moi;
	sc.i >> health;
	sc.i >> next >> selectnext;
	sc.i >> enemy;
	sc.i >> race;
	sc.i >> otflag;
}

void Entity::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	st::dive(sc, method);
	if(next)
		next->dive(sc, method);
}


double Entity::maxhealth()const{return 100.;}
void Entity::enterField(WarField *){}
void Entity::leaveField(WarField *aw){
	WarSpace *ws = *aw;
	if(ws->bdw && bbody)
		ws->bdw->removeRigidBody(bbody);
}
void Entity::setPosition(const Vec3d *apos, const Quatd *arot, const Vec3d *avelo, const Vec3d *aavelo){
	if(apos && apos != &pos) pos = *apos;
	if(arot && arot != &rot) rot = *arot;
	if(avelo && avelo != &velo) velo = *avelo;
	if(aavelo && aavelo != &omg) omg = *aavelo;

	if(bbody){
		if(apos && arot)
			bbody->setCenterOfMassTransform(btTransform(btqc(*arot), btvc(*apos)));
		else if(apos)
			bbody->setCenterOfMassTransform(btTransform(bbody->getOrientation(), btvc(*apos)));
		else if(arot)
			bbody->setCenterOfMassTransform(btTransform(btqc(*arot), bbody->getCenterOfMassPosition()));
		if(avelo)
			bbody->setLinearVelocity(btvc(*avelo));
		if(aavelo)
			bbody->setAngularVelocity(btvc(*aavelo));
	}
}
void Entity::getPosition(Vec3d *apos, Quatd *arot, Vec3d *avelo, Vec3d *aavelo)const{
	if(apos) *apos = pos;
	if(arot) *arot = rot;
	if(avelo) *avelo = velo;
	if(aavelo) *aavelo = omg;
}
void Entity::anim(double){}
void Entity::postframe(){if(enemy && !enemy->w) enemy = NULL;}
void Entity::control(const input_t *i, double){inputs = *i;}
unsigned Entity::analog_mask(){return 0;}
void Entity::cockpitView(Vec3d &pos, Quatd &rot, int)const{pos = this->pos; rot = this->rot;}
int Entity::numCockpits()const{return 1;}
void Entity::draw(wardraw_t *){}
void Entity::drawtra(wardraw_t *){}
void Entity::drawHUD(wardraw_t *){}
bool Entity::solid(const Entity *)const{return true;} // Default is to check hits
void Entity::bullethit(const Bullet *){}
Entity *Entity::getOwner(){return NULL;}
bool Entity::isSelectable()const{return false;}
int Entity::armsCount()const{return 0;}
ArmBase *Entity::armsGet(int){return NULL;}
std::vector<cpplib::dstring> Entity::props()const{
	using namespace cpplib;
	std::vector<cpplib::dstring> ret;
	ret.push_back(cpplib::dstring("Class: ") << classname());
	ret.push_back(cpplib::dstring("CoordSys: ") << w->cs->getpath());
	ret.push_back(cpplib::dstring("Health: ") << dstring(health) << "/" << dstring(maxhealth()));
	ret.push_back(cpplib::dstring("Race: ") << dstring(race));
	return ret;
}
double Entity::getRU()const{return 0.;}
Builder *Entity::getBuilderInt(){return NULL;}
Docker *Entity::getDockerInt(){return NULL;}
bool Entity::dock(Docker*){return false;}
bool Entity::undock(Docker*){return false;}
bool Entity::command(EntityCommand *){return false;}

int Entity::tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *fret, Vec3d *retp, Vec3d *retnormal){
	Vec3d retpos;
	bool bret = !!jHitSpherePos(pos, this->hitradius() + rad, start, dir, 1., fret, &retpos);
	if(bret && retp)
		*retp = retpos;
	if(bret && retnormal)
		*retnormal = (retpos - pos).normin();
	return bret;
//	(hitpart = 0, 0. < (sdist = -VECSP(delta, &mat[8]) - rad))) && sdist < best
}

int Entity::takedamage(double damage, int hitpart){
	if(health <= damage){
		health = 0;
		w = NULL;
		return 0;
	}
	health -= damage;
	return 1;
}

// This function's purpose is unclear.
void Entity::bullethole(sufindex, double, const Vec3d &pos, const Quatd &rot){}

int Entity::popupMenu(PopupMenu &list){
	return 0;
}

Warpable *Entity::toWarpable(){return NULL;}
Entity::Dockable *Entity::toDockable(){return NULL;}

void Entity::transit_cs(CoordSys *cs){
	Vec3d pos;
	Mat4d mat;
	if(w == cs->w)
		return;

//	leaveField(w);

	// Transform position to target CoordSys
	this->pos = cs->tocs(this->pos, w->cs);
	velo = cs->tocsv(velo, pos, w->cs);
	if(!cs->w){
		cs->w = new WarSpace(cs);
	}

	Quatd q1 = w->cs->tocsq(cs);
	this->rot = q1 * this->rot;

	Player *pl = w->getPlayer();
	if(pl && pl->chase == this){
		pl->transit_cs(cs);
	}

//	cs->w->addent(this);
	w = cs->w;
}

class GLWprop : public GLwindowSizeable{
public:
	typedef GLwindowSizeable st;
	GLWprop(const char *title, Entity *e = NULL) : st(title), a(e){
		xpos = 120;
		ypos = 40;
		width = 200;
		height = 200;
		flags |= GLW_COLLAPSABLE | GLW_CLOSE;
	}
	virtual void draw(GLwindowState &ws, double t);
	virtual void postframe();
protected:
	Entity *a;
};

int Entity::cmd_property(int argc, char *argv[], void *pv){
	static int counter = 0;
	Player *ppl = (Player*)pv;
	if(!ppl || !ppl->selected)
		return 0;
	/*glwAppend*/(new GLWprop(cpplib::dstring("Entity Property ") << counter++, ppl->selected));
	return 0;
}

void GLWprop::draw(GLwindowState &ws, double t){
	if(!a)
		return;
	std::vector<cpplib::dstring> pl = a->props();
	int i = 0;
	for(std::vector<cpplib::dstring>::iterator e = pl.begin(); e != pl.end(); e++){
		glColor4f(0,1,1,1);
		glwpos2d(xpos, ypos + (2 + i++) * getFontHeight());
		glwprintf(*e);
	}
}

void GLWprop::postframe(){
	if(a && !a->w)
		a = NULL;
}


/* estimate target position based on distance and approaching speed */
int estimate_pos(Vec3d &ret, const Vec3d &pos, const Vec3d &velo, const Vec3d &srcpos, const Vec3d &srcvelo, double speed, const WarField *w){
	double dist;
	Vec3d grv;
	double posy;
	double h = 0.;
	double h0 = 0.;
	if(w){
		Vec3d mid = (pos + srcpos) * .5;
		Vec3d evelo = (pos - srcpos).norm() * -speed;
		grv = w->accel(mid, evelo);
	}
	else
		VECNULL(grv);
	dist = (pos - srcpos).len();
	posy = pos[1] + (velo[1] + grv[1]) * dist / speed;
/*	(*ret)[0] = pos[0] + (velo[0] - srcvelo[0] + grv[0] * dist / speed / 2.) * dist / speed;
	(*ret)[1] = posy - srcvelo[1] * dist / speed;
	(*ret)[2] = pos[2] + (velo[2] - srcvelo[2] + grv[2] * dist / speed / 2.) * dist / speed;*/
	ret = pos + (velo - srcvelo + grv * dist / speed / 2.) * dist / speed;
	return 1;
}


static bool strless(const char *a, const char *b){
	return strcmp(a, b) < 0;
}

std::map<const char *, EntityCommandCreatorFunc*, bool (*)(const char*,const char*)> EntityCommand::ctormap(strless);


bool EntityCommand::derived(EntityCommandID)const{return false;}
IMPLEMENT_COMMAND(HaltCommand, "Halt")
IMPLEMENT_COMMAND(AttackCommand, "Attack")

AttackCommand::AttackCommand(HSQUIRRELVM v, Entity &){
	int argc = sq_gettop(v);
	if(argc < 3)
		throw SQFArgumentError();
	Entity *ent;
	if(sqa_refobj(v, (SQUserPointer*)&ent, NULL, 3)){
		ents.insert(ent);
	}
}

IMPLEMENT_COMMAND(ForceAttackCommand, "ForceAttack")
IMPLEMENT_COMMAND(MoveCommand, "Move")
IMPLEMENT_COMMAND(ParadeCommand, "Parade")
IMPLEMENT_COMMAND(DeltaCommand, "Delta")
IMPLEMENT_COMMAND(DockCommand, "Dock")

IMPLEMENT_COMMAND(SetAggressiveCommand, "SetAggressive")
IMPLEMENT_COMMAND(SetPassiveCommand, "SetPassive")

IMPLEMENT_COMMAND(WarpCommand, "Warp")

IMPLEMENT_COMMAND(RemainDockedCommand, "RemainDocked")

RemainDockedCommand::RemainDockedCommand(HSQUIRRELVM v, Entity &){
	int argc = sq_gettop(v);
	if(argc < 3)
		throw SQFArgumentError();
	SQBool b;
	if(SQ_FAILED(sq_getbool(v, 3, &b)))
		throw SQFArgumentError();
	enable = b;
}

MoveCommand::MoveCommand(HSQUIRRELVM v, Entity &){
	int argc = sq_gettop(v);
	if(argc < 2)
		throw SQFArgumentError();
	SQUserPointer typetag;
	if(OT_INSTANCE != sq_gettype(v, 3) || (sq_gettypetag(v, 3, &typetag), typetag != tt_Vec3d))
		throw SQFError(_SC("Incompatible argument type"));
	sq_pushstring(v, _SC("a"), -1);
	if(SQ_FAILED(sq_get(v, 3)))
		throw SQFError(_SC("Corrupt Vec3d data"));
	Vec3d *pvec3;
	sq_getuserdata(v, -1, (SQUserPointer*)&pvec3, NULL);
	destpos = *pvec3;
}

WarpCommand::WarpCommand(HSQUIRRELVM v, Entity &e){
	int argc = sq_gettop(v);
	if(argc < 3)
		throw SQFArgumentError();
	const SQChar *destname;
	if(SQ_SUCCEEDED(sq_getstring(v, 3, &destname))){
		WarField *w = e.w;
		CoordSys *pa = NULL;
		CoordSys *pcs;
		double landrad;
		double dist, cost;
		extern coordsys *g_galaxysystem;
		teleport *tp = Player::findTeleport(destname, TELEPORT_WARP);
		if(tp){
			destpos = tp->pos;
			destcs = tp->cs;
		}
		else if(pa = w->cs->findcspath(destname)){
			destpos = vec3_000;
			destcs = pa;
		} 
		else
			throw SQFError();
	}
	else if(sqa_refobj(v, (SQUserPointer*)&destcs, NULL, 3)){
		destpos = vec3_000;
	}
	else
		throw SQFArgumentError();
	SQUserPointer typetag;
	if(OT_INSTANCE != sq_gettype(v, 4) || (sq_gettypetag(v, 4, &typetag), typetag != tt_Vec3d))
		throw SQFError(_SC("Incompatible argument type"));
	sq_pushstring(v, _SC("a"), -1);
	if(SQ_FAILED(sq_get(v, 4)))
		throw SQFError(_SC("Corrupt Vec3d data"));
	Vec3d *pvec3;
	sq_getuserdata(v, -1, (SQUserPointer*)&pvec3, NULL);
	destpos = *pvec3;
}
