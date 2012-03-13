#include "Shipyard.h"
#include "judge.h"
#include "serial_util.h"
#include "Player.h"
#include "antiglut.h"
#include "Sceptor.h"
#include "Beamer.h"
#include "sqadapt.h"
#include "glw/popup.h"
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
#define SQRT2P2 (M_SQRT2/2.)




#if 1
const unsigned ShipyardDocker::classid = registerClass("ShipyardDocker", Conster<ShipyardDocker>);

const char *ShipyardDocker::classname()const{return "ShipyardDocker";}


#define SCARRY_MAX_HEALTH 200000
#define SCARRY_MAX_SPEED .03
#define SCARRY_ACCELERATE .01
#define SCARRY_MAX_ANGLESPEED (.005 * M_PI)
#define SCARRY_ANGLEACCEL (.002 * M_PI)

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

Shipyard::Shipyard(WarField *w) : st::st(w), st(w), docker(new ShipyardDocker(this)), Builder(this->Entity::w){
	st::init();
	init();
//	for(int i = 0; i < nhardpoints; i++)
//		turrets[i] = NULL;
//		w->addent(turrets[i] = new MTurret(this, &hardpoints[i]));
}

void Shipyard::init(){
	ru = 0.;
/*	if(!hardpoints){
		hardpoints = hardpoint_static::load("scarry.hb", nhardpoints);
	}
	turrets = new ArmBase*[nhardpoints];*/
	mass = 5e6;
}

Shipyard::~Shipyard(){
	delete docker;
}

const char *Shipyard::idname()const{return "Shipyard";}
const char *Shipyard::classname()const{return "Shipyard";}

const unsigned Shipyard::classid = registerClass("Shipyard", Conster<Shipyard>);
Entity::EntityRegister<Shipyard> Shipyard::entityRegister("Shipyard");

void Shipyard::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << docker;
	for(int i = 0; i < nhardpoints; i++)
		sc.o << turrets[i];
}

void Shipyard::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> docker;
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

double Shipyard::maxhealth()const{
	return 200000;
}

double Shipyard::hitradius()const{
	return 1.;
}

double Shipyard::maxenergy()const{
	return 5e8;
}

void Shipyard::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	pos = this->pos + this->rot.trans(Vec3d(.100, .22, -.610));
	rot = this->rot;
}

void Shipyard::anim(double dt){
	st::anim(dt);
	docker->anim(dt);
//	Builder::anim(dt);
//	for(int i = 0; i < nhardpoints; i++) if(turrets[i])
//		turrets[i]->align();
}

int Shipyard::popupMenu(PopupMenu &list){
	int ret = st::popupMenu(list);
//	list.append("Build Window", 'b', "buildmenu");
	list.append("Dock Window", 'd', "dockmenu");
	return ret;
}

Entity::Props Shipyard::props()const{
	Props ret = st::props();
//	ret.push_back(cpplib::dstring("?: "));
	return ret;
}

int Shipyard::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	Shipyard *p = this;
	double sc[3];
	double best = dt, retf;
	int reti = 0, i, n;
#if 0
	if(0 < p->shieldAmount){
		if(jHitSpherePos(pos, BEAMER_SHIELDRAD + rad, src, dir, dt, ret, retp))
			return 1000; /* something quite unlikely to reach */
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

int Shipyard::armsCount()const{
	return nhardpoints;
}

ArmBase *Shipyard::armsGet(int i){
	return turrets[i];
}

double Shipyard::getRU()const{return ru;}
Builder *Shipyard::getBuilderInt(){return this;}
Docker *Shipyard::getDockerInt(){return docker;}


const Shipyard::maneuve Shipyard::mymn = {
	SCARRY_ACCELERATE, /* double accel; */
	SCARRY_MAX_SPEED, /* double maxspeed; */
	SCARRY_ANGLEACCEL, /* double angleaccel; */
	SCARRY_MAX_ANGLESPEED, /* double maxanglespeed; */
	5e8, /* double capacity; */
	1e6, /* double capacitor_gen; */
};

const Warpable::maneuve &Shipyard::getManeuve()const{return mymn;}

struct hitbox Shipyard::hitboxes[] = {
/*	hitbox(Vec3d(0., 0., -.02), Quatd(0,0,0,1), Vec3d(.015, .015, .075)),
	hitbox(Vec3d(.025, -.015, .02), Quatd(0,0, -SIN15, COS15), Vec3d(.0075, .002, .02)),
	hitbox(Vec3d(-.025, -.015, .02), Quatd(0,0, SIN15, COS15), Vec3d(.0075, .002, .02)),
	hitbox(Vec3d(.0, .03, .0325), Quatd(0,0,0,1), Vec3d(.002, .008, .010)),*/
	hitbox(Vec3d(0., 0., .050), Quatd(0,0,0,1), Vec3d(.3, .2, .500)),
/*	{{0.105, 0., .0}, {0,0,0,1}, {.100, .2, .050}},*/
	hitbox(Vec3d(-.105, 0., -.700), Quatd(0,0,0,1), Vec3d(.095, .020, .250)),
	hitbox(Vec3d( .100, 0., -.640), Quatd(0,0,0,1), Vec3d(.100, .060, .180)),
	hitbox(Vec3d( .100, .0, -.600), Quatd(0,0,0,1), Vec3d(.040, .260, .040)),
};
const int Shipyard::nhitboxes = numof(Shipyard::hitboxes);


void Shipyard::doneBuild(Entity *e){
	Entity::Dockable *d = e->toDockable();
	if(d)
		docker->dock(d);
	else{
		e->w = this->Entity::w;
		this->Entity::w->addent(e);
		e->pos = pos + rot.trans(Vec3d(-.10, 0.05, 0));
		e->velo = velo;
		e->rot = rot;
	}
/*	e->pos = pos + rot.trans(Vec3d(-.15, 0.05, 0));
	e->velo = velo;
	e->rot = rot;
	static_cast<Dockable*>(e)->undock(this);*/
}

bool ShipyardDocker::undock(Entity::Dockable *pe){
	if(st::undock(pe)){
		Vec3d pos = e->pos + e->rot.trans(Vec3d(-.10, 0.05, 0));
		pe->setPosition(&pos, &e->rot, &e->velo, &e->omg);
		return true;
	}
	return false;
}

Vec3d ShipyardDocker::getPortPos()const{
	return Vec3d(-100. * SCARRY_SCALE, -50. * SCARRY_SCALE, 0.);
}

Quatd ShipyardDocker::getPortRot()const{
	return quat_u;
}
#endif

#ifndef _WIN32
void Shipyard::draw(WarDraw*){}
void Shipyard::drawtra(WarDraw*){}
#endif
