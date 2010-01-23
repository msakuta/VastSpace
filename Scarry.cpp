#include "Scarry.h"
#include "judge.h"
#include "serial_util.h"
#include "Player.h"
#include "antiglut.h"
#include "sceptor.h"
#include "Assault.h"
#include "Beamer.h"
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


const Builder::BuildStatic *Builder::builder0[] = {/*&scontainer_build, &worker_build,*/ &sceptor_build, &Beamer::builds, &Assault::builds};
const unsigned Builder::nbuilder0 = numof(builder0);

void Builder::serialize(SerializeContext &sc){
	// Do NOT call Entity::serialize here because this class is a branch class.
	sc.o << build;
	sc.o << nbuildque;
	for(int i = 0; i < nbuildque; i++){
		sc.o << buildque[i].num/* << buildque[i].st*/;
	}
}

void Builder::unserialize(UnserializeContext &sc){
	// Do NOT call Entity::unserialize here because this class is a branch class.
	sc.i >> build;
	sc.i >> nbuildque;
	for(int i = 0; i < nbuildque; i++){
		sc.i >> buildque[i].num/* >> buildque[i].st*/;
		buildque[i].st = &sceptor_build;
	}
}

bool Builder::addBuild(const BuildStatic *st){
	if(numof(buildque) <= nbuildque && buildque[nbuildque-1].st != st)
		return false;
	if(buildque[nbuildque-1].st == st)
		buildque[nbuildque-1].num++;
	else{
		buildque[nbuildque].st = st;
		buildque[nbuildque].num = 1;
		if(0 == nbuildque)
			build = buildque[nbuildque].st->buildtime;
		nbuildque++;
	}
	return true;
}

bool Builder::cancelBuild(int index, bool recalc){
	if(index < 0 || nbuildque <= index)
		return false;
	if(!--buildque[index].num){
		::memmove(&buildque[index], &buildque[index+1], (--nbuildque - index) * sizeof *buildque);
	}
	if(recalc && index == 0 && nbuildque)
		build = buildque[0].st->buildtime;
	return true;
}

void Builder::doneBuild(Entity *child){}

void Builder::anim(double dt){
	static const double g_buildtimescale = 10.;
	dt *= g_buildtimescale;
	while(nbuildque && build < dt){
		dt -= build;
		Entity *created = buildque[0].st->create(this->w, this);
		doneBuild(created);
		cancelBuild(0, false);
		build += buildque[0].st->buildtime;
	}
	if(nbuildque)
		build -= dt;
}






static int select_tab(GLwindow *wnd, int ix, int iy, const char *s, int *selected, int mx, int my){
	int ix0 = ix;
	ix += 3;
	ix += 8 * strlen(s);
	ix += 3;
	*selected = ix0 < mx && mx < ix && iy < my + 12 && my + 12 < iy + 12;
	return ix;
}

int GLWbuild::mouse(GLwindowState &ws, int mbutton, int state, int mx, int my){
	GLWbuild *p = this;
	int ind, sel0, sel1, ix;
	int fonth = (int)getFontHeight();
	if(st::mouse(ws, mbutton, state, mx, my))
		return 1;
	ind = (my - 3 * 12) / 12;
	if((mbutton == GLUT_RIGHT_BUTTON || mbutton == GLUT_LEFT_BUTTON) && state == GLUT_UP || mbutton == GLUT_WHEEL_UP || mbutton == GLUT_WHEEL_DOWN){
		int num = 1, i;
#ifdef _WIN32
		if(HIBYTE(GetKeyState(VK_SHIFT)))
			num = 10;
#endif
		ix = select_tab(this, ix = 2, 2 * fonth, "Build", &sel0, mx, my);
		select_tab(this, ix, 2 * fonth, "Queue", &sel1, mx, my);
		if(sel0) p->tabindex = 0;
		if(sel1) p->tabindex = 1;
		if(p->tabindex == 0){
#if 1
			if(0 <= ind && ind < Builder::nbuilder0){
				builder->addBuild(builder->builder0[ind]);
			}
#else
			if(0 <= ind && ind < 1) for(i = 0; i < num; i++) switch(ind){
				case 0:
/*					add_buildque(p->p, &scontainer_build);
					break;
				case 1:
					add_buildque(p->p, &worker_build);
					break;
				case 2:*/
					builder->addBuild(&sceptor_build);
					break;
/*				case 3:
					add_buildque(p->p, &beamer_build);
					break;
				case 4:
					if(mbutton == GLUT_RIGHT_BUTTON){
						int j;
						glwindow *wnd2;
						for(j = 0; j < p->p->narmscustom; j++) if(p->p->armscustom[j].builder == &assault_build)
							break;
						if(j == p->p->narmscustom){
							p->p->armscustom = realloc(p->p->armscustom, ++p->p->narmscustom * sizeof *p->p->armscustom);
							p->p->armscustom[j].builder = &assault_build;
							p->p->armscustom[j].c = numof(assault_hardpoints);
							p->p->armscustom[j].a = malloc(numof(assault_hardpoints) * sizeof *p->p->armscustom[j].a);
							memcpy(p->p->armscustom[j].a, assault_arms, numof(assault_hardpoints) * sizeof *p->p->armscustom[j].a);
						}
						wnd2 = ArmsShowWindow(AssaultNew, assault_build.cost, offsetof(assault_t, arms), p->p->armscustom[j].a, assault_hardpoints, numof(assault_hardpoints));
						wnd->modal = wnd2;
					}
					else
						add_buildque(p->p, &assault_build);
					break;*/
			}
#endif
		}
		else{
			if(builder->build != 0. && ind == 0 || ind == 1){
				builder->build = 0.;
				builder->cancelBuild(ind);
			}
/*			else if(2 <= ind && ind < pf->nbuildque + 2){
				int i;
				int ptr;
				ind -= 2;
				ptr = (pf->buildque0 + ind) % numof(pf->buildque);
				if(1 < pf->buildquenum[ptr])
					pf->buildquenum[ptr]--;
				else{
					for(i = ind; i < pf->nbuildque - 1; i++){
						int ptr = (pf->buildque0 + i) % numof(pf->buildque);
						int ptr1 = (pf->buildque0 + i + 1) % numof(pf->buildque);
						pf->buildque[ptr] = pf->buildque[ptr1];
						pf->buildquenum[ptr] = pf->buildquenum[ptr1];
					}
					pf->nbuildque--;
				}
			}*/
		}
		return 1;
	}
	return 0;
}

void GLWbuild::postframe(){
	if(builder && !builder->w)
		builder = NULL;
}

int cmd_build(int argc, char *argv[], void *pv){
	Player &pl = *(Player*)pv;
	if(!pl.selected || !pl.selected->w)
		return 0;
	Builder *pb = pl.selected->getBuilder();
	if(pb)
		new GLWbuild("Build", pb);
	return 0;
}

Docker::Docker(Entity *ae) : st(NULL), baycool(0), e(ae), undockque(NULL), remainDocked(false){
	for(int i = 0; i < numof(paradec); i++)
		paradec[i] = 0;
	if(ae && ae->w && ae->w->cs)
		cs = ae->w->cs;
}

Docker::~Docker(){
	// Docked entities die with their base.
	for(Entity *e = el; e;){
		Entity *next = e->next;
		delete e;
		e = next;
	}
	for(Entity *e = undockque; e;){
		Entity *next = e->next;
		delete e;
		e = next;
	}
}

void Docker::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << e;
	sc.o << baycool;
	sc.o << undockque;
	for(int i = 0; i < numof(paradec); i++)
		sc.o << paradec[i];
	sc.o << remainDocked;
}

void Docker::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> e;
	sc.i >> baycool;
	sc.i >> undockque;
	for(int i = 0; i < numof(paradec); i++)
		sc.i >> paradec[i];
	sc.i >> remainDocked;
}

void Docker::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	// Do NOT call st::dive here because this class is a virtual branch class.
	(this->*method)(sc);
	if(el)
		el->dive(sc, method);
	if(undockque)
		undockque->dive(sc, method);
}

void Docker::anim(double dt){
/*	if(!remainDocked && el){
		Dockable **end = &undockque;
		for(; *end; end = &(*end)->next);
		*end = el;
		el = NULL;
	}*/
	for(Dockable *pe = el; pe; pe = pe->next)
		pe->anim(dt);
	while(undockque && baycool < dt){
		Entity *next = undockque->next;
		undock(undockque);
		undockque = next ? next->toDockable() : NULL;
	}

	if(baycool < dt){
		baycool = 0.;
	}
	else
		baycool -= dt;
}

void Docker::dock(Dockable *e){
	e->dock(this);
	if(e->w)
		e->w = this;
	else{
		e->w = this;
		addent(e);
	}
/*	e->next = el;
	el = e;*/
}

bool Docker::postUndock(Dockable *e){
	Dockable **pp;
	for(pp = &el; *pp; pp = reinterpret_cast<Dockable**>(&(*pp)->next)) if(*pp == e){
		*pp = reinterpret_cast<Dockable*>(e->next);
		Dockable **qend = &undockque;
		if(*qend) for(; *qend; qend = reinterpret_cast<Dockable**>(&(*qend)->next)); // Search until end
		*qend = e;
		e->next = NULL; // Appending to end means next is equal to NULL
		return true;
	}
	return false;
}

Entity *Docker::addent(Entity *e){
	e->next = el;
	return el = e;
}

Docker::operator Docker *(){return this;}

bool Docker::undock(Dockable *e){
	e->w = this->e->w;
	this->e->w->addent(e);
	e->undock(this);
	return true;
}

const unsigned ScarryDocker::classid = registerClass("ScarryDocker", Conster<ScarryDocker>);

const char *ScarryDocker::classname()const{return "ScarryDocker";}

int GLWdock::mouse(GLwindowState &ws, int mbutton, int state, int mx, int my){
	GLWdock *p = this;
	int ind, sel0, sel1, ix;
	int fonth = (int)getFontHeight();
	if(st::mouse(ws, mbutton, state, mx, my))
		return 1;
	ind = (my - 5 * 12) / 12;
	if(docker && (mbutton == GLUT_RIGHT_BUTTON || mbutton == GLUT_LEFT_BUTTON) && state == GLUT_UP || mbutton == GLUT_WHEEL_UP || mbutton == GLUT_WHEEL_DOWN){
		int num = 1, i;
		if(ind == -2)
			docker->remainDocked = !docker->remainDocked;
		if(ind == -1)
			grouping = !grouping;
		else if(grouping){
			std::map<cpplib::dstring, int> map;

			for(Entity *e = docker->el; e; e = e->next ? e->next->toDockable() : NULL){
				map[e->dispname()]++;
			}
			for(std::map<cpplib::dstring, int>::iterator it = map.begin(); it != map.end(); it++) if(!ind--){
				for(Entity *e = docker->el; e;){
					Entity *next = e->next;
					if(!strcmp(e->dispname(), it->first))
						docker->postUndock(e->toDockable());
					e = next;
				}
				break;
			}
		}
		else for(Entity *e = docker->el; e; e = e->next) if(!ind--){
			docker->postUndock(e->toDockable());
			break;
		}
		return 1;
	}
	return 0;
}

void GLWdock::postframe(){
	if(docker && !docker->e->w)
		docker = NULL;
}


int cmd_dockmenu(int argc, char *argv[], void *pv){
	Player &pl = *(Player*)pv;
	if(!pl.selected || !pl.selected->w)
		return 0;
	Docker *pb = pl.selected->getDocker();
	if(pb)
		new GLWdock("Dock", pb);
	return 0;
}


#define SCARRY_MAX_HEALTH 200000
#define SCARRY_MAX_SPEED .03
#define SCARRY_ACCELERATE .01
#define SCARRY_MAX_ANGLESPEED (.005 * M_PI)
#define SCARRY_ANGLEACCEL (.002 * M_PI)

hardpoint_static *Scarry::hardpoints = NULL/*[10] = {
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
int Scarry::nhardpoints = 0;

Scarry::Scarry(WarField *w) : st::st(w), st(w), docker(new ScarryDocker(this)){
	st::init();
	init();
	for(int i = 0; i < nhardpoints; i++)
		w->addent(turrets[i] = new MTurret(this, &hardpoints[i]));
}

void Scarry::init(){
	ru = 0.;
	if(!hardpoints){
		hardpoints = hardpoint_static::load("scarry.hb", nhardpoints);
	}
	turrets = new ArmBase*[nhardpoints];
	mass = 5e6;
}

Scarry::~Scarry(){
	delete docker;
}

const char *Scarry::idname()const{return "scarry";}
const char *Scarry::classname()const{return "Scarry";}

const unsigned Scarry::classid = registerClass("Scarry", Conster<Scarry>);

void Scarry::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << docker;
	for(int i = 0; i < nhardpoints; i++)
		sc.o << turrets[i];
	Builder::serialize(sc);
}

void Scarry::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> docker;
	for(int i = 0; i < nhardpoints; i++)
		sc.i >> turrets[i];
	Builder::unserialize(sc);
}

void Scarry::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	st::dive(sc, method);
	docker->dive(sc, method);
}

const char *Scarry::dispname()const{
	return "Space Carrier";
};

double Scarry::maxhealth()const{
	return 200000;
}

double Scarry::hitradius(){
	return 1.;
}

double Scarry::maxenergy()const{
	return 5e8;
}

void Scarry::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	pos = this->pos + this->rot.trans(Vec3d(.100, .22, -.610));
	rot = this->rot;
}

void Scarry::anim(double dt){
	st::anim(dt);
	docker->anim(dt);
	Builder::anim(dt);
	for(int i = 0; i < nhardpoints; i++) if(turrets[i])
		turrets[i]->align();
}

int Scarry::popupMenu(PopupMenu &list){
	int ret = st::popupMenu(list);
	list.append("Build Window", 'b', "buildmenu").append("Dock Window", 'd', "dockmenu");
	return ret;
}

Entity::Props Scarry::props()const{
	Props ret = st::props();
//	ret.push_back(cpplib::dstring("?: "));
	return ret;
}

int Scarry::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	Scarry *p = this;
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

int Scarry::armsCount()const{
	return nhardpoints;
}

const ArmBase *Scarry::armsGet(int i)const{
	return turrets[i];
}

double Scarry::getRU()const{return ru;}
Builder *Scarry::getBuilderInt(){return this;}
Docker *Scarry::getDockerInt(){return docker;}


const Scarry::maneuve Scarry::mymn = {
	SCARRY_ACCELERATE, /* double accel; */
	SCARRY_MAX_SPEED, /* double maxspeed; */
	SCARRY_ANGLEACCEL, /* double angleaccel; */
	SCARRY_MAX_ANGLESPEED, /* double maxanglespeed; */
	5e8, /* double capacity; */
	1e6, /* double capacitor_gen; */
};

const Warpable::maneuve &Scarry::getManeuve()const{return mymn;}

struct hitbox Scarry::hitboxes[] = {
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
const int Scarry::nhitboxes = numof(Scarry::hitboxes);


void Scarry::doneBuild(Entity *e){
	Entity::Dockable *d = e->toDockable();
	if(d)
		docker->dock(d);
	else{
		e->w = this->w;
		w->addent(e);
		e->pos = pos + rot.trans(Vec3d(-.10, 0.05, 0));
		e->velo = velo;
		e->rot = rot;
	}
/*	e->pos = pos + rot.trans(Vec3d(-.15, 0.05, 0));
	e->velo = velo;
	e->rot = rot;
	static_cast<Dockable*>(e)->undock(this);*/
}

bool ScarryDocker::undock(Entity::Dockable *pe){
	if(st::undock(pe)){
		pe->pos = e->pos + e->rot.trans(Vec3d(-.10, 0.05, 0));
		pe->velo = e->velo;
		pe->rot = e->rot;
		return true;
	}
	return false;
}
