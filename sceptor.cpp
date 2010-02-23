#include "sceptor.h"
#include "player.h"
#include "bullet.h"
#include "judge.h"
#include "serial_util.h"
#include "Warpable.h"
#include "Scarry.h"
#include "material.h"
//#include "worker.h"
//#include "glsl.h"
//#include "astro_star.h"
//#include "sensor.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/wavsound.h>
#include <clib/zip/UnZip.h>
}
#include <assert.h>
#include <string.h>


/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289

#define SCEPTOR_SCALE 1./10000
#define SCEPTOR_SMOKE_FREQ 20.
#define SCEPTOR_COOLTIME .1
#define SCEPTOR_ROLLSPEED (.2 * M_PI)
#define SCEPTOR_ROTSPEED (.3 * M_PI)
#define SCEPTOR_MAX_ANGLESPEED (M_PI * .5)
#define SCEPTOR_ANGLEACCEL (M_PI * .2)
#define SCEPTOR_MAX_GIBS 20
#define BULLETSPEED 2.
#define SCEPTOR_MAGAZINE 20
#define SCEPTOR_RELOADTIME 4.

Entity::Dockable *Sceptor::toDockable(){return this;}

/* color sequences */
extern const struct color_sequence cs_orangeburn, cs_shortburn;


struct hitbox Sceptor::hitboxes[] = {
	hitbox(Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(.005, .002, .003)),
};
const int Sceptor::nhitboxes = numof(Sceptor::hitboxes);

/*static const struct hitbox sceptor_hb[] = {
	hitbox(Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(.005, .002, .003)),
};*/

const char *Sceptor::idname()const{
	return "sceptor";
}

const char *Sceptor::classname()const{
	return "Sceptor";
}

const unsigned Sceptor::classid = registerClass("Sceptor", Conster<Sceptor>);
const unsigned Sceptor::entityid = registerEntity("Sceptor", Constructor<Sceptor>);

void Sceptor::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << aac; /* angular acceleration */
	sc.o << throttle;
	sc.o << fuel;
	sc.o << cooldown;
	sc.o << dest;
	sc.o << fcloak;
	sc.o << heat;
	sc.o << mother; // Mother ship
//	sc.o << hitsound;
	sc.o << paradec;
	sc.o << (int)task;
	sc.o << docked << returning << away << cloak;
}

void Sceptor::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> aac; /* angular acceleration */
	sc.i >> throttle;
	sc.i >> fuel;
	sc.i >> cooldown;
	sc.i >> dest;
	sc.i >> fcloak;
	sc.i >> heat;
	sc.i >> mother; // Mother ship
//	sc.i >> hitsound;
	sc.i >> paradec;
	sc.i >> (int&)task;
	sc.i >> docked >> returning >> away >> cloak;

	// Re-create temporary entity if flying in a WarField. It is possible that docked to something and w is NULL.
	WarSpace *ws;
	if(w && (ws = (WarSpace*)w))
		pf = AddTefpolMovable3D(ws->tepl, this->pos, this->velo, avec3_000, &cs_orangeburn, TEP3_THICK | TEP3_ROUGH, cs_orangeburn.t);
	else
		pf = NULL;
}

const char *Sceptor::dispname()const{
	return "Interceptor";
};

double Sceptor::maxhealth()const{
	return 700.;
}





Sceptor::Sceptor() : mother(NULL), mf(0), paradec(-1){
}

Sceptor::Sceptor(WarField *aw) : st(aw), mother(NULL), task(Idle), fuel(maxfuel()), mf(0), paradec(-1){
	Sceptor *const p = this;
//	EntityInit(ret, w, &SCEPTOR_s);
//	VECCPY(ret->pos, mother->st.st.pos);
	mass = 4e3;
//	race = mother->st.st.race;
	health = maxhealth();
	p->aac.clear();
	memset(p->thrusts, 0, sizeof p->thrusts);
	p->throttle = .5;
	p->cooldown = 1.;
	WarSpace *ws;
	if(w && (ws = (WarSpace*)w))
		p->pf = AddTefpolMovable3D(ws->tepl, this->pos, this->velo, avec3_000, &cs_orangeburn, TEP3_THICK | TEP3_ROUGH, cs_orangeburn.t);
	else
		p->pf = NULL;
//	p->mother = mother;
	p->hitsound = -1;
	p->docked = false;
//	p->paradec = mother->paradec++;
	p->magazine = SCEPTOR_MAGAZINE;
	p->task = Idle;
	p->fcloak = 0.;
	p->cloak = 0;
	p->heat = 0.;
/*	if(mother){
		scarry_dock(mother, ret, w);
		if(!mother->remainDocked)
			scarry_undock(mother, ret, w);
	}*/
}

const avec3_t Sceptor::gunPos[2] = {{35. * SCEPTOR_SCALE, -4. * SCEPTOR_SCALE, -15. * SCEPTOR_SCALE}, {-35. * SCEPTOR_SCALE, -4. * SCEPTOR_SCALE, -15. * SCEPTOR_SCALE}};

void Sceptor::cockpitView(Vec3d &pos, Quatd &q, int seatid)const{
	Player *ppl = w->pl;
	Vec3d ofs;
	static const Vec3d src[2] = {Vec3d(0., .001, -.002), Vec3d(0., .008, 0.020)};
	Mat4d mat;
	seatid = (seatid + 2) % 2;
	q = this->rot;
	ofs = q.trans(src[seatid]);
	pos = this->pos + ofs;
}

/*static void SCEPTOR_control(entity_t *pt, warf_t *w, input_t *inputs, double dt){
	SCEPTOR_t *p = (SCEPTOR_t*)pt;
	if(!pt->active || pt->health <= 0.)
		return;
	pt->inputs = *inputs;
}*/

int Sceptor::popupMenu(PopupMenu &list){
	int ret = st::popupMenu(list);
	list.append("Dock", 0, "dock").append("Military Parade Formation", 0, "parade_formation").append("Cloak", 0, "cloak");
	return ret;
}

Entity::Props Sceptor::props()const{
	std::vector<cpplib::dstring> ret = st::props();
	ret.push_back(cpplib::dstring("Task: ") << task);
	ret.push_back(cpplib::dstring("Fuel: ") << fuel << '/' << maxfuel());
	return ret;
}

bool Sceptor::undock(Docker *d){
	st::undock(d);
	task = Undock;
	mother = d;
	if(this->pf)
		ImmobilizeTefpol3D(this->pf);
	WarSpace *ws;
	if(w && w->getTefpol3d())
		this->pf = AddTefpolMovable3D(w->getTefpol3d(), this->pos, this->velo, avec3_000, &cs_orangeburn, TEP3_THICK | TEP3_ROUGH, cs_orangeburn.t);
	d->baycool += 1.;
	return true;
}

/*
void cmd_cloak(int argc, char *argv[]){
	extern struct player *ppl;
	if(ppl->cs->w){
		entity_t *pt;
		for(pt = ppl->selected; pt; pt = pt->selectnext) if(pt->vft == &SCEPTOR_s){
			((SCEPTOR_t*)pt)->cloak = !((SCEPTOR_t*)pt)->cloak;
		}
	}
}
*/
void Sceptor::shootDualGun(double dt){
	Vec3d velo, gunpos, velo0(0., 0., -BULLETSPEED);
	Mat4d mat;
	int i = 0;
	if(dt <= cooldown)
		return;
	transform(mat);
	do{
		Bullet *pb;
		double phi, theta;
		pb = new Bullet(this, 5, 10.);
		w->addent(pb);
		pb->pos = mat.vp3(gunPos[i]);
/*		phi = pt->pyr[1] + (drseq(&w->rs) - .5) * .005;
		theta = pt->pyr[0] + (drseq(&w->rs) - .5) * .005;
		VECCPY(pb->velo, pt->velo);
		pb->velo[0] +=  BULLETSPEED * sin(phi) * cos(theta);
		pb->velo[1] += -BULLETSPEED * sin(theta);
		pb->velo[2] += -BULLETSPEED * cos(phi) * cos(theta);*/
		pb->velo = mat.dvp3(velo0);
		pb->velo += this->velo;
		pb->life = 3.;
		this->heat += .025;
	} while(!i++);
//	shootsound(pt, w, p->cooldown);
//	pt->shoots += 2;
	if(0 < --magazine)
		this->cooldown += SCEPTOR_COOLTIME * (fuel <= 0 ? 3 : 1);
	else{
		magazine = SCEPTOR_MAGAZINE;
		this->cooldown += SCEPTOR_RELOADTIME;
	}
	this->mf = .1;
}

// find the nearest enemy
bool Sceptor::findEnemy(){
	Entity *pt2, *closest = NULL;
	double best = 1e2 * 1e2;
	for(pt2 = w->el; pt2; pt2 = pt2->next){

		if(!(pt2->isTargettable() && pt2 != this && pt2->w == w && pt2->health > 0. && pt2->race != -1 && pt2->race != this->race))
			continue;

/*		if(!entity_visible(pb, pt2))
			continue;*/

		double sdist = (pt2->pos - this->pos).slen();
		if(sdist < best){
			best = sdist;
			closest = pt2;
		}
	}
	if(closest)
		enemy = closest;
	return !!closest;
}

#if 1
static int space_collide_callback(const struct otjEnumHitSphereParam *param, Entity *pt){
	Entity *pt2 = (Entity*)param->hint;
	if(pt == pt2)
		return 0;
	const double &dt = param->dt;
	Vec3d dr = pt->pos - pt2->pos;
	double r = pt->hitradius(), r2 = pt2->hitradius();
	double sr = (r + r2) * (r + r2);
	if(r * 2. < r2){
		if(!pt2->tracehit(pt->pos, pt->velo, r, dt, NULL, NULL, NULL))
			return 0;
	}
	else if(r2 * 2. < r){
		if(!pt->tracehit(pt2->pos, pt2->velo, r2, dt, NULL, NULL, NULL))
			return 0;
	}

	return 1;
}

static void space_collide_reflect(Entity *pt, const Vec3d &netvelo, const Vec3d &n, double f){
	Vec3d dv = pt->velo - netvelo;
	if(dv.sp(n) < 0)
		pt->velo = -n * dv.sp(n) + netvelo;
	else
		pt->velo += n * f;
}

static void space_collide_resolve(Entity *pt, Entity *pt2, double dt){
	const double ff = .2;
	Vec3d dr = pt->pos - pt2->pos;
	double sd = dr.slen();
	double r = pt->hitradius(), r2 = pt2->hitradius();
	double sr = (r + r2) * (r + r2);
	double f = ff * dt / (/*sd */1) * (pt2->mass < pt->mass ? pt2->mass / pt->mass : 1.);
	Vec3d n;

	// If either one of concerned Entities are not solid, no hit check is needed.
	if(!pt->solid(pt2) || !pt2->solid(pt))
		return;

	// If bounding spheres are not intersecting each other, no resolving is performed.
	// Object tree should discard such cases, but that must be ensured here when the tree
	// is yet to be built.
	if(sr < sd)
		return;

	if(r * 2. < r2){
		if(!pt2->tracehit(pt->pos, pt->velo, r, dt, NULL, NULL, &n))
			return;
	}
	else if(r2 * 2. < r){
		if(!pt->tracehit(pt2->pos, pt2->velo, r2, dt, NULL, NULL, &n))
			return;
		n *= -1;
	}
	else{
		n = dr.norm();
	}
	if(1. < f) /* prevent oscillation */
		f = 1.;

	// Aquire momentum of center of mass
	Vec3d netmomentum = pt->velo * pt->mass + pt2->velo * pt2->mass;

	// Aquire velocity of netmomentum, which is exact velocity when the colliding object stick together.
	Vec3d netvelo = netmomentum / (pt->mass + pt2->mass);

	// terminate closing velocity component
	space_collide_reflect(pt, netvelo, n, f * pt2->mass / (pt->mass + pt2->mass));
	space_collide_reflect(pt2, netvelo, -n, f * pt->mass / (pt->mass + pt2->mass));
}

void space_collide(Entity *pt, WarSpace *w, double dt, Entity *collideignore, Entity *collideignore2){
	Entity *pt2;
	if(1 && w->otroot){
		Entity *iglist[3] = {pt, collideignore, collideignore2};
//		struct entity_static *igvft[1] = {&rstation_s};
		struct otjEnumHitSphereParam param;
		param.root = w->otroot;
		param.src = &pt->pos;
		param.dir = &pt->velo;
		param.dt = dt;
		param.rad = pt->hitradius();
		param.pos = NULL;
		param.norm = NULL;
		param.flags = OTJ_IGVFT | OTJ_CALLBACK;
		param.callback = space_collide_callback;
/*		param.hint = iglist;*/
		param.hint = pt;
		param.iglist = iglist;
		param.niglist = 3;
//		param.igvft = igvft;
//		param.nigvft = 1;
		if(pt2 = otjEnumPointHitSphere(&param)){
			space_collide_resolve(pt, pt2, dt);
		}
	}
	else for(pt2 = w->el; pt2; pt2 = pt2->next) if(pt2 != pt && pt2 != collideignore && pt2 != collideignore2 && pt2->w == pt->w){
		space_collide_resolve(pt, pt2, dt);
	}
}
#endif

#if 0
void quat2mat(amat4_t *mat, const aquat_t q){
#if 1
	double *m = *mat;
	double x = q[0], y = q[1], z = q[2], w = q[3];
	m[0] = 1. - (2 * q[1] * q[1] + 2 * q[2] * q[2]); m[1] = 2 * q[0] * q[1] + 2 * q[2] * q[3]; m[2] = 2 * q[0] * q[2] - 2 * q[1] * q[3];
	m[4] = 2 * x * y - 2 * z * w; m[5] = 1. - (2 * x * x +  2 * z * z); m[6] = 2 * y * z + 2 * x * w;
	m[8] = 2 * x * z + 2 * y * w; m[9] = 2 * y * z - 2 * x * w; m[10] = 1. - (2 * x * x + 2 * y * y);
	m[3] = m[7] = m[11] = m[12] = m[13] = m[14] = 0.;
	m[15] = 1.;
#else
	aquat_t r = {1., 0., 0.}, qr;
	aquat_t qc;
	QUATCNJ(qc, q);
	VECTOQUAT(r, avec3_100);
	QUATMUL(qr, q, r);
	QUATMUL(&(*mat)[0], qr, qc);
	(*mat)[3] = 0.;
/*	assert(mat[3] == 0.);*/
	VECTOQUAT(r, avec3_010);
	QUATMUL(qr, q, r);
	QUATMUL(&(*mat)[4], qr, qc);
/*	assert(mat[7] == 0.);*/
	(*mat)[7] = 0.;
	VECTOQUAT(r, avec3_001);
	QUATMUL(qr, q, r);
	QUATMUL(&(*mat)[8], qr, qc);
/*	assert(mat[11] == 0.);*/
	(*mat)[11] = 0.;
	VECNULL(&(*mat)[12]);
	(*mat)[15]=1.;
/*	{
		amat4_t mat2;
		double tw, scale;
		double axis[3], angle;
		tw = acos(q[3]) * 2.;
		scale = sin(tw / 2.);
		axis[0] = q[0] / scale;
		axis[1] = q[1] / scale;
		axis[2] = q[2] / scale;
		angle = tw * 360 / 2. / M_PI;
		glPushMatrix();
		glLoadIdentity();
		glRotated(angle, axis[0], axis[1], axis[2]);
		glGetDoublev(GL_MODELVIEW_MATRIX, mat);
		glPopMatrix();
	}*/
#endif
}

void quat2imat(amat4_t *mat, const aquat_t pq){
	aquat_t q, qr;
	QUATCNJ(q, pq);
	quat2mat(mat, q);
	QUATMUL(qr, q, pq);
}

void mat2quat(aquat_t q, const amat4_t m){
	double s;
	double tr = m[0] + m[5] + m[10] + 1.0;
	if(tr >= 1.0){
		s = 0.5 / sqrt(tr);
		q[0] = (m[9] - m[6]) * s;
		q[1] = (m[2] - m[8]) * s;
		q[2] = (m[4] - m[1]) * s;
		q[3] = 0.25 / s;
		return;
    }
	else{
        double max;
        if(m[5] > m[10])
			max = m[5];
        else
			max = m[10];
        
        if(max < m[0]){
			double x;
			s = sqrt(m[0] - (m[5] + m[10]) + 1.0);
			x = s * 0.5;
			s = 0.5 / s;
 			q[0] = x;
			q[1] = (m[4] - m[1]) * s;
			q[2] = (m[2] - m[8]) * s;
			q[3] = (m[9] - m[6]) * s;
			return;
		}
		else if(max == m[5]){
			double y;
			s = sqrt(m[5] - (m[10] + m[0]) + 1.0);
			y = s * 0.5;
            s = 0.5 / s;
			q[0] = (m[4] - m[1]) * s;
			q[1] = y;
			q[2] = (m[9] + m[6]) * s;
			q[3] = (m[2] + m[8]) * s;
			return;
        }
		else{
			double z;
			s = sqrt(m[10] - (m[0] + m[5]) + 1.0);
			z = s * 0.5;
			s = 0.5 / s;
			q[0] = (m[2] + m[8]) * s;
			q[1] = (m[9] + m[6]) * s;
			q[2] = z;
			q[3] = (m[4] + m[1]) * s;
			return;
        }
    }

}

void imat2quat(aquat_t q, const amat4_t m){
	aquat_t q1;
	mat2quat(q1, m);
	QUATCNJ(q, q1);
}

void quat2pyr(const aquat_t quat, avec3_t euler){
/*	double cx,sx,x;
	double cy,sy,y,yr;
	double cz,sz;*/
	amat4_t nmat;
	aquat_t q;
/*	avec3_t v;*/
/*	rbd_anim(pt, dt, &q, &nmat);*/
	QUATNORM(q,quat);
/*	quat2mat(nmat, q);*/

	/* we make inverse rotation matrix out of the quaternion, to make it
	  possible to do inverse transformations to gain roll. */
	quat2imat(&nmat, q);

	imat2pyr(nmat, euler);

}
#endif


void Sceptor::steerArrival(double dt, const Vec3d &atarget, const Vec3d &targetvelo, double speedfactor, double minspeed){
	Vec3d target(atarget);
	Vec3d rdr = target - this->pos; // real dr
	Vec3d rdrn = rdr.norm();
	Vec3d dv = targetvelo - this->velo;
	Vec3d dvLinear = rdrn.sp(dv) * rdrn;
	Vec3d dvPlanar = dv - dvLinear;
	double dist = rdr.len();
	if(rdrn.sp(dv) < 0) // estimate only when closing
		target += dvPlanar * dist / dvLinear.len();
	Vec3d dr = this->pos - target;
	this->throttle = dr.len() * speedfactor + minspeed;
	this->omg = 3 * this->rot.trans(vec3_001).vp(dr.norm());
	if(SCEPTOR_ROTSPEED * SCEPTOR_ROTSPEED < this->omg.slen())
		this->omg.normin().scalein(SCEPTOR_ROTSPEED);
	this->rot = this->rot.quatrotquat(this->omg * dt);
}

// Find mother if has none
Entity *Sceptor::findMother(){
	Entity *pm = NULL;
	double best = 1e10 * 1e10, sl;
	for(Entity *e = w->entlist(); e; e = e->next) if(e->getDocker() && (sl = (e->pos - this->pos).slen()) < best){
		mother = e->getDocker();
		pm = mother->e;
		best = sl;
	}
	return pm;
}


void Sceptor::anim(double dt){
	WarField *oldw = w;
	Entity *pt = this;
	Sceptor *const pf = this;
	Sceptor *const p = this;
//	scarry_t *const mother = pf->mother;
	Entity *pm = mother && mother->e ? mother->e : NULL;
	Mat4d mat, imat;

	if(Docker *docker = *w){
		fuel = min(fuel + dt * 20., maxfuel()); // it takes 6 seconds to fully refuel
		health = min(health + dt * 100., maxhealth()); // it takes 7 seconds to be fully repaired
		if(fuel == maxfuel() && health == maxhealth() && !docker->remainDocked)
			docker->postUndock(this);
		return;
	}

/*	if(!mother){
		if(p->pf){
			ImmobilizeTefpol3D(p->pf);
			p->pf = NULL;
		}
		pt->active = 0;
		return;
	}*/

	if(pf->pf)
		MoveTefpol3D(pf->pf, pt->pos, avec3_000, cs_orangeburn.t, 0/*pf->docked*/);

#if 0
	if(pf->docked){
		if(pm->enemy && mother->baycool == 0.){
			static int counter = 0;
			static const avec3_t pos0[2] = {{-70. * SCARRY_SCALE, 30. * SCARRY_SCALE, 0.}, {-130. * SCARRY_SCALE, 30. * SCARRY_SCALE, 0.},};
			QUATCPY(pt->rot, pm->rot);
			quatrot(pt->pos, pm->rot, pos0[counter++ % 2]);
			VECADDIN(pt->pos, pm->pos);
			VECCPY(pt->velo, pm->velo);
			pf->docked = pf->returning = 0;
			p->task = SCEPTOR_undock;
			mother->baycool = SCARRY_BAYCOOL;
		}
		else{
			VECCPY(pt->pos, pm->pos);
			pt->pos[1] += .1;
			VECCPY(pt->velo, pm->velo);
			pt->race = pm->race;
			return;
		}
	}
#endif

	/* forget about beaten enemy */
	if(pt->enemy && pt->enemy->health <= 0.)
		pt->enemy = NULL;

	transform(mat);
//	quat2imat(imat, mat);

#if 0
	if(mother->st.enemy && 0 < pt->health && VECSLEN(pt->velo) < .1 * .1){
		double acc[3];
		double len;
		avec3_t acc0 = {0., 0., -0.025};
		amat4_t mat;
		pyrmat(pt->pyr, mat);
		MAT4VP3(acc, mat, acc0);
		len = drseq(&w->rs) + .5;
		VECSCALEIN(acc, len);
		VECSADD(pt->velo, acc, dt);
	/*	VECSADD(pt->velo, w->gravity, dt);*/
		VECSADD(pt->pos, pt->velo, dt);
		movesound3d(pf->hitsound, pt->pos);
		return;
	}
#endif

	{
		int i;
		for(i = 0; i < 2; i++){
			if(p->thrusts[0][i] < dt * 2.)
				p->thrusts[0][i] = 0.;
			else
				p->thrusts[0][i] -= dt * 2.;
		}
	}

//	if(pm)
//		pt->race = pm->race;

	if(0 < pt->health){
//		double oldyaw = pt->pyr[1];
		bool controlled = w->pl->control == this;
		int parking = 0;
		Entity *collideignore = NULL;
		if(!controlled)
			pt->inputs.press = 0;
		if(false/*pf->docked*/);
		else if(p->task == Undockque){
/*			if(pm->w != w){
				if(p->pf){
					ImmobilizeTefpol3D(p->pf);
					p->pf = NULL;
				}
				pt->active = 0;
				return;
			}
			else if(p->mother->baycool == 0.){
				static const avec3_t pos0[2] = {{-70. * SCARRY_SCALE, 30. * SCARRY_SCALE, 0.}, {-130. * SCARRY_SCALE, 30. * SCARRY_SCALE, 0.},};
				QUATCPY(pt->rot, pm->rot);
				quatrot(pt->pos, pm->rot, pos0[mother->undockc++ % 2]);
				VECADDIN(pt->pos, pm->pos);
				VECCPY(pt->velo, pm->velo);
				p->task = SCEPTOR_undock;
				p->mother->baycool = SCARRY_BAYCOOL;
				if(p->pf)
					ImmobilizeTefpol3D(p->pf);
				p->pf = AddTefpolMovable3D(w->tepl, pt->pos, pt->velo, avec3_000, &cs_shortburn, TEP3_THICK | TEP3_ROUGH, cs_shortburn.t);
			}
			else
				return;*/
		}
/*		else if(w->pl->control == pt){
		}*/
/*		else if(pf->returning){
			avec3_t delta;
			VECSUB(delta, pt->pos, mother->st.pos);
			pt->pyr[1] = approach(pt->pyr[1], fmod(atan2(-delta[0], delta[2]) + 2 * M_PI, 2 * M_PI), SCEPTOR_ROTSPEED * dt, 2 * M_PI);
			pt->pyr[0] = approach(pt->pyr[0], atan2(delta[1], sqrt(delta[0] * delta[0] + delta[2] * delta[2])), SCEPTOR_ROTSPEED * dt, 2 * M_PI);
			pt->pyr[2] = approach(pt->pyr[2] + M_PI, (pt->pyr[1] - oldyaw) / dt + M_PI, SCEPTOR_ROLLSPEED * dt, 2 * M_PI) - M_PI;
			if(VECSDIST(pt->pos, mother->st.pos) < 1. * 1.)
				pf->returning = 0;
		}
		else if(5. * 5. < VECSDIST(pt->pos, mother->st.pos)){
			pf->returning = 1;
		}*/
		else{
			double pos[3], dv[3], dist;
			Vec3d delta;
			int i, n, trigger = 1;
			Vec3d opos;
//			pt->enemy = pm->enemy;
			if(pt->enemy /*&& VECSDIST(pt->pos, pm->pos) < 15. * 15.*/){
				double sp;
/*				const avec3_t guns[2] = {{.002, .001, -.005}, {-.002, .001, -.005}};*/
				Vec3d xh, dh, vh;
				Vec3d epos;
				double phi;
				estimate_pos(epos, pt->enemy->pos, pt->enemy->velo, pt->pos, pt->velo, BULLETSPEED, w);
/*				xh[0] = pt->velo[2];
				xh[1] = pt->velo[1];
				xh[2] = -pt->velo[0];*/
				delta = epos - pt->pos;
/*				{
					avec3_t rv;
					double eposlen;
					VECSUB(rv, pt->enemy->velo, pt->velo);
					eposlen = VECDIST(epos, pt->pos) / (VECSP(delta, rv) + BULLETSPEED);
					VECSADD(epos, w->gravity, -eposlen);
				}
				VECSUB(delta, pt->pos, epos);
				delta[1] += .001;
				if(VECSLEN(delta) < 2. * 2.){
					VECNORM(dh, delta);
					VECNORM(vh, pt->velo);
					sp = VECSP(dh, vh);
					if(sp < -.99)
						pt->inputs.press |= PL_ENTER;
				}*/
			}
/*			else
			{
				delta = pm->pos - pt->pos;
				trigger = 0;
			}*/

			if(p->task == Undock){
				if(!pm)
					p->task = Idle;
				else{
					double sp;
					Vec3d dm = this->pos - pm->pos;
					Vec3d mzh = this->rot.trans(vec3_001);
					sp = -mzh.sp(dm);
					p->throttle = 1.;
					if(1. < sp)
						p->task = Parade;
				}
			}
			else if(w->pl->control != pt) do{
				if((task == Attack || task == Away) && !pt->enemy || p->task == Idle || p->task == Parade){
					if(pm && (pt->enemy = pm->enemy)){
						p->task = Attack;
					}
					else if(findEnemy()){
						p->task = Attack;
					}
					if(!enemy || (p->task == Idle || p->task == Parade)){
						if(pm)
							p->task = Parade;
						else
							p->task = Idle;
					}
				}
				else if(p->task == Moveto){
					Vec3d target;
					Quatd q2, q1;
					Vec3d dr = pt->pos - p->dest;
					if(dr.slen() < .03 * .03){
						q1 = Quatd(0,0,0,1);
//						QUATCPY(q1, pm->rot);
						p->throttle = 0.;
						parking = 1;
						pt->velo += dr * -dt * .5;
						p->task = Idle;
					}
					else{
						p->throttle = dr.slen() / 5. + .03;
						q1 = Quatd::direction(dr);
					}
					q2 = Quatd::slerp(pt->rot, q1, 1. - exp(-dt));
					pt->rot = q2;
					if(1. < p->throttle)
						p->throttle = 1.;
				}
				else if(pt->enemy && (p->task == Attack || p->task == Away)){
					Vec3d dv, forward;
					Vec3d xh, yh;
					double sx, sy, len, len2, maxspeed = SCEPTOR_MAX_ANGLESPEED * dt;
					Quatd qres, qrot;

					// If a mother could not be aquired, fight to the death alone.
					if(p->fuel < 30. && (pm || (pm = findMother()))){
						p->task = Dockque;
						break;
					}

/*					VECSUB(dv, pt->enemy->pos, pt->pos);*/
					dv = delta;
					double awaybase = pt->enemy->hitradius() * 1.2 + .1;
					double attackrad = awaybase < .6 ? awaybase * 5. : .6 * 5.;
					if(p->task == Attack && dv.slen() < awaybase * awaybase){
						p->task = Away;
					}
					else if(p->task == Away && attackrad * attackrad < dv.slen()){
						p->task = Attack;
					}
					dv.normin();
					forward = pt->rot.trans(avec3_001);
					if(p->task == Attack)
						forward *= -1;
		/*				sx = VECSP(&mat[0], dv);
					sy = VECSP(&mat[4], dv);
					pt->inputs.press |= (sx < 0 ? PL_4 : 0 < sx ? PL_6 : 0) | (sy < 0 ? PL_2 : 0 < sy ? PL_8 : 0);*/
					p->throttle = 1.;
					if(0 < fuel){
						struct random_sequence rs;
						init_rseq(&rs, (unsigned long)this ^ (unsigned long)(w->war_time() / .1));
						Vec3d randomvec;
						for(i = 0; i < 3; i++)
							randomvec[i] = drseq(&rs) - .5;
						pt->velo += randomvec * dt * .5;
					}
					if(p->task == Attack || forward.sp(dv) < -.5){
						xh = forward.vp(dv);
						len = len2 = xh.len();
						len = asin(len);
						if(maxspeed < len){
							len = maxspeed;
						}
						len = sin(len / 2.);
						if(len && len2){
							Vec3d omg, laac;
							qrot = xh * len / len2;
							qrot[3] = sqrt(1. - len * len);
							qres = qrot * pt->rot;
							pt->rot = qres.norm();

							/* calculate angular acceleration for displaying thruster bursts */
							omg = qrot.operator Vec3d&() * 1. / dt;
							p->aac = omg - pt->omg;
							p->aac *= 1. / dt;
							pt->omg = omg;
							laac = pt->rot.cnj().trans(p->aac);
							if(laac[0] < 0) p->thrusts[0][0] += -laac[0];
							if(0 < laac[0]) p->thrusts[0][1] += laac[0];
							p->thrusts[0][0] = min(p->thrusts[0][0], 1.);
							p->thrusts[0][1] = min(p->thrusts[0][1], 1.);
						}
						if(trigger && p->task == Attack && .99 < dv.sp(forward)){
							pt->inputs.change |= PL_ENTER;
							pt->inputs.press |= PL_ENTER;
						}
					}
					else{
						p->aac.clear();
					}
				}
				else if(!pt->enemy && (p->task == Attack || p->task == Away)){
					p->task = Parade;
				}
				if(p->task == Parade){
					if(mother){
						if(paradec == -1)
							paradec = mother->enumParadeC(mother->Fighter);
						Vec3d target, target0(-1., 0., -1.);
						Quatd q2, q1;
						target0[0] += p->paradec % 10 * -.05;
						target0[2] += p->paradec / 10 * -.05;
						target = pm->rot.trans(target0);
						target += pm->pos;
						Vec3d dr = pt->pos - target;
						if(dr.slen() < .03 * .03){
							q1 = pm->rot;
							p->throttle = 0.;
							parking = 1;
							pt->velo += dr * (-dt * .5);
							q2 = Quatd::slerp(pt->rot, q1, 1. - exp(-dt));
							pt->rot = q2;
						}
						else{
//							p->throttle = dr.slen() / 5. + .01;
							steerArrival(dt, target, pm->velo, 1. / 10., .001);
						}
						if(1. < p->throttle)
							p->throttle = 1.;
					}
					else
						task = Idle;
				}
				else if(p->task == Dockque || p->task == Dock){
					if(!pm)
						pm = findMother();

					// It is possible that no one is glad to become a mother.
					if(!pm)
						p->task = Idle;
					else{
						Vec3d target0(-100. * SCARRY_SCALE, -50. * SCARRY_SCALE, 0.);
						Quatd q2, q1;
						collideignore = pm;
						if(p->task == Dockque)
							target0[2] += -1.;
						Vec3d target = pm->rot.trans(target0);
						target += pm->pos;
						steerArrival(dt, target, pm->velo, p->task == Dockque ? .2 : .025, .01);
						double dist = (target - this->pos).len();
						if(dist < .03){
							if(p->task == Dockque)
								p->task = Dock;
							else{
								mother->dock(pt);
								if(p->pf){
									ImmobilizeTefpol3D(p->pf);
									p->pf = NULL;
								}
								p->docked = true;
	/*							if(mother->cargo < scarry_cargousage(mother)){
									scarry_undock(mother, pt, w);
								}*/
								return;
							}
						}
						if(1. < p->throttle)
							p->throttle = 1.;
					}
				}
				if(p->task == Idle)
					p->throttle = 0.;
			}while(0);
			else{
				double common = 0., normal = 0.;
				Vec3d qrot;
				Vec3d pos, velo;
				int i;
/*				{
					Vec3d pyr;
					quat2pyr(w->pl->rot, pyr);
					common = -pyr[0];
					normal = -pyr[1];
					VECNULL(w->pl->rot);
					w->pl->rot[3] = 1.;
				}*/
				if(common){
					avec3_t th0[2] = {{0., 0., .003}, {0., 0., -.003}};
					qrot = mat.vec3(0) * common;
					pt->rot = pt->rot.quatrotquat(qrot);
/*					for(i = 0; i < 2; i++){
						MAT4VP3(pos, mat, th0[i]);
						MAT4DVP3(velo, mat, avec3_010);
						VECSCALEIN(velo, .02 * signof(common) * (i * 2 - 1));
						VECADDIN(velo, pt->velo);
						AddTeline3D(w->tell, pos, velo, .0005, NULL, NULL, NULL, COLOR32RGBA(255,255,255,255), TEL3_SPRITE | TEL3_INVROTATE, .4);
					}*/
				}
				if(normal){
					avec3_t th0[2] = {{.005, 0., .0}, {-.005, 0., 0.}};
					qrot = mat.vec3(1) * normal;
					pt->rot = pt->rot.quatrotquat(qrot);
/*					for(i = 0; i < 2; i++){
						MAT4VP3(pos, mat, th0[i]);
						MAT4DVP3(velo, mat, avec3_001);
						VECSCALEIN(velo, .02 * signof(normal) * (i * 2 - 1));
						VECADDIN(velo, pt->velo);
						AddTeline3D(w->tell, pos, velo, .0005, NULL, NULL, NULL, COLOR32RGBA(255,255,255,255), TEL3_SPRITE | TEL3_INVROTATE, .4);
					}*/
				}
			}

			if(pt->inputs.press & (PL_ENTER | PL_LCLICK)){
				shootDualGun(dt);
			}

			if(p->cooldown < dt)
				p->cooldown = 0;
			else
				p->cooldown -= dt;

#if 0
			if((pf->away ? 1. * 1. : .3 * .3) < VECSLEN(delta)/* && 0 < VECSP(pt->velo, pt->pos)*/){
				avec3_t xh;
				xh[0] = pt->velo[2];
				xh[1] = pt->velo[1];
				xh[2] = -pt->velo[0];
				pt->pyr[1] = approach(pt->pyr[1], fmod(atan2(-delta[0], delta[2]) + 2 * M_PI, 2 * M_PI), SCEPTOR_ROTSPEED * dt, 2 * M_PI);
				pt->pyr[0] = approach(pt->pyr[0], atan2(delta[1], sqrt(delta[0] * delta[0] + delta[2] * delta[2])), SCEPTOR_ROTSPEED * dt, 2 * M_PI);
				pf->away = 0;
			}
			else if(.2 * .2 < VECSLEN(delta)){
				avec3_t xh;
				xh[0] = pt->velo[2];
				xh[1] = pt->velo[1];
				xh[2] = -pt->velo[0];
				if(!pf->away && VECSP(pt->velo, delta) < 0){
					double desired;
					desired = fmod(atan2(-delta[0], delta[2]) + 2 * M_PI, 2 * M_PI);

					/* roll */
/*					pt->pyr[2] = approach(pt->pyr[2], (pt->pyr[1] < desired ? 1 : desired < pt->pyr[1] ? -1 : 0) * M_PI / 6., .1 * M_PI * dt, 2 * M_PI);*/

					pt->pyr[1] = approach(pt->pyr[1], desired, SCEPTOR_ROTSPEED * dt, 2 * M_PI);

					if(pt->enemy && pt->pyr[1] == desired){
					}

					pt->pyr[0] = approach(pt->pyr[0], atan2(delta[1], sqrt(delta[0] * delta[0] + delta[2] * delta[2])), SCEPTOR_ROTSPEED * dt, 2 * M_PI);
				}
				else{
					const double desiredy = pt->pos[1] < .1 ? -M_PI / 6. : 0.;
					pf->away = 1;
					pt->pyr[0] = approach(pt->pyr[0], desiredy, .1 * M_PI * dt, 2 * M_PI);
				}
			}
			else{
				double desiredp;
				desiredp = fmod(atan2(-delta[0], delta[2]) + M_PI + 2 * M_PI, 2 * M_PI);
				pt->pyr[1] = approach(pt->pyr[1], desiredp, SCEPTOR_ROTSPEED * dt, 2 * M_PI);
				pt->pyr[0] = approach(pt->pyr[0], pt->pos[1] < .1 ? -M_PI / 6. : 0., SCEPTOR_ROTSPEED * dt, 2 * M_PI);
			}
#endif
		}

		if(pt->inputs.press & PL_A){
			Vec3d qrot = mat.vec3(1) * dt;
			pt->rot = pt->rot.quatrotquat(qrot);
		}
		if(pt->inputs.press & PL_D){
			Vec3d qrot = mat.vec3(1) * -dt;
			pt->rot = pt->rot.quatrotquat(qrot);
		}
		if(pt->inputs.press & PL_W){
			Vec3d qrot = mat.vec3(0) * dt;
			pt->rot = pt->rot.quatrotquat(qrot);
		}
		if(pt->inputs.press & PL_S){
			Vec3d qrot = mat.vec3(0) * -dt;
			pt->rot = pt->rot.quatrotquat(qrot);
		}

		if(controlled){
			if(inputs.press & PL_Q)
				p->throttle = MIN(throttle + dt, 1.);
			if(inputs.press & PL_Z)
				p->throttle = MAX(throttle - dt, 0.);
		}

		/* you're not allowed to accel further than certain velocity. */
		const double maxvelo = .5, speed = -p->velo.sp(mat.vec3(2));
		if(maxvelo < speed)
			p->throttle = 0.;
		else{
			if(!controlled && (p->task == Attack || p->task == Away))
				p->throttle = 1.;
			if(1. - speed / maxvelo < throttle)
				throttle = 1. - speed / maxvelo;
		}

		/* Friction (in space!) */
		{
			double f = 1. / (1. + dt / (parking ? 1. : 3.));
			pt->velo *= f;
		}

		{
			double consump = dt * (pf->throttle + p->fcloak * 4.); /* cloaking consumes fuel extremely */
			Vec3d acc, acc0(0., 0., -1.);
			if(p->fuel <= consump){
				if(.05 < pf->throttle)
					pf->throttle = .05;
				if(p->cloak)
					p->cloak = 0;
				p->fuel = 0.;
			}
			else
				p->fuel -= consump;
			double spd = pf->throttle * (p->task != Attack ? .01 : .005);
			acc = pt->rot.trans(acc0);
			pt->velo += acc * spd;
		}

		/* heat dissipation */
		p->heat *= exp(-dt * .2);

/*		pt->pyr[2] = approach(pt->pyr[2] + M_PI, (pt->pyr[1] - oldyaw) / dt + M_PI, SCEPTOR_ROLLSPEED * dt, 2 * M_PI) - M_PI;
		{
			double spd = pf->away ? .2 : .11;
			pt->velo[0] = spd * cos(pt->pyr[0]) * sin(pt->pyr[1]);
			pt->velo[1] = -spd * sin(pt->pyr[0]);
			pt->velo[2] = -spd * cos(pt->pyr[0]) * cos(pt->pyr[1]);
		}*/
/*		{
			double spd = pf->throttle * (pf->away ? .02 : .011);
			avec3_t acc, acc0 = {0., 0., -1.};
			quatrot(acc, pt->rot, acc0);
			VECSADD(pt->velo, acc, spd * dt);
		}*/
		if(p->task != Undock && p->task != Undockque && task != Dock){
			WarSpace *ws = (WarSpace*)w;
			if(ws)
				space_collide(pt, ws, dt, collideignore, NULL);
		}
		pt->pos += pt->velo * dt;

		p->fcloak = approach(p->fcloak, p->cloak, dt, 0.);
	}
	else{
		pt->health += dt;
		if(0. < pt->health){
			struct tent3d_line_list *tell = w->getTeline3d();
//			effectDeath(w, pt);
//			playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .1, w->realtime);
/*			if(w->gibs && ((struct entity_private_static*)pt->vft)->sufbase){
				int i, n, base;
				struct entity_private_static *vft = (struct entity_private_static*)pt->vft;
				int m = vft->sufbase->np;
				n = m <= SCEPTOR_MAX_GIBS ? m : SCEPTOR_MAX_GIBS;
				base = m <= SCEPTOR_MAX_GIBS ? 0 : rseq(&w->rs) % m;
				for(i = 0; i < n; i++){
					double velo[3], omg[3];
					int j;
					j = (base + i) % m;
					velo[0] = pt->velo[0] + (drseq(&w->rs) - .5) * .1;
					velo[1] = pt->velo[1] + (drseq(&w->rs) - .5) * .1;
					velo[2] = pt->velo[2] + (drseq(&w->rs) - .5) * .1;
					omg[0] = 3. * 2 * M_PI * (drseq(&w->rs) - .5);
					omg[1] = 3. * 2 * M_PI * (drseq(&w->rs) - .5);
					omg[2] = 3. * 2 * M_PI * (drseq(&w->rs) - .5);
					AddTelineCallback3D(w->gibs, pt->pos, velo, 0.01, pt->pyr, omg, w->gravity, vft->gib_draw, (void*)j, TEL3_NOLINE | TEL3_REFLECT, 1.5 + drseq(&w->rs));
				}
			}*/
/*			pt->pos = pm->pos;
			pt->pos[1] += .01;
			VECCPY(pt->velo, pm->velo);
			pt->health = 350.;
			if(p->pf){
				ImmobilizeTefpol3D(p->pf);
				p->pf = NULL;
			}
			pt->active = 0;*/
/*			pf->docked = 1;*/
			this->w = NULL;
		}
		else{
			struct tent3d_line_list *tell = w->getTeline3d();
			if(tell){
				double pos[3], dv[3], dist;
				Vec3d gravity = w->accel(this->pos, this->velo) / 2.;
				int i, n;
				n = (int)(dt * SCEPTOR_SMOKE_FREQ + drseq(&w->rs));
				for(i = 0; i < n; i++){
					pos[0] = pt->pos[0] + (drseq(&w->rs) - .5) * .01;
					pos[1] = pt->pos[1] + (drseq(&w->rs) - .5) * .01;
					pos[2] = pt->pos[2] + (drseq(&w->rs) - .5) * .01;
					dv[0] = .5 * pt->velo[0] + (drseq(&w->rs) - .5) * .01;
					dv[1] = .5 * pt->velo[1] + (drseq(&w->rs) - .5) * .01;
					dv[2] = .5 * pt->velo[2] + (drseq(&w->rs) - .5) * .01;
//					AddTeline3D(w->tell, pos, dv, .01, NULL, NULL, gravity, COLOR32RGBA(127 + rseq(&w->rs) % 32,127,127,255), TEL3_SPRITE | TEL3_INVROTATE | TEL3_NOLINE | TEL3_REFLECT, 1.5 + drseq(&w->rs) * 1.5);
					AddTelineCallback3D(tell, pos, dv, .02, quat_u, vec3_000, gravity, smokedraw, NULL, TEL3_INVROTATE | TEL3_NOLINE, 1.5 + drseq(&w->rs) * 1.5);
				}
			}
			pt->pos += pt->velo * dt;
		}
	}

	if(mf < dt)
		mf = 0.;
	else
		mf -= dt;

	st::anim(dt);

	// if we are transitting WarField or being destroyed, trailing tefpols should be marked for deleting.
	if(this->pf && w != oldw)
		ImmobilizeTefpol3D(this->pf);
//	movesound3d(pf->hitsound, pt->pos);
}

// Docking and undocking will never stack.
bool Sceptor::solid(const Entity *o)const{
	return !(task == Undock || task == Dock);
}

int Sceptor::takedamage(double damage, int hitpart){
	int ret = 1;
	struct tent3d_line_list *tell = w->getTeline3d();
	if(this->health < 0.)
		return 1;
//	this->hitsound = playWave3D("hit.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
	if(0 < health && health - damage <= 0){
		int i;
		ret = 0;
/*		effectDeath(w, pt);*/
		if(tell) for(i = 0; i < 32; i++){
			double pos[3], velo[3];
			velo[0] = drseq(&w->rs) - .5;
			velo[1] = drseq(&w->rs) - .5;
			velo[2] = drseq(&w->rs) - .5;
			VECNORMIN(velo);
			VECCPY(pos, this->pos);
			VECSCALEIN(velo, .1);
			VECSADD(pos, velo, .1);
			AddTeline3D(tell, pos, velo, .005, quat_u, vec3_000, w->accel(this->pos, this->velo), COLOR32RGBA(255, 31, 0, 255), TEL3_HEADFORWARD | TEL3_THICK | TEL3_FADEEND | TEL3_REFLECT, 1.5 + drseq(&w->rs));
		}
/*		((SCEPTOR_t*)pt)->pf = AddTefpolMovable3D(w->tepl, pt->pos, pt->velo, nullvec3, &cs_firetrail, TEP3_THICKER | TEP3_ROUGH, cs_firetrail.t);*/
//		((SCEPTOR_t*)pt)->hitsound = playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
		health = -2.;
//		pt->deaths++;
	}
	else
		health -= damage;
	return ret;
}

void Sceptor::postframe(){
	if(mother && mother->e && (!mother->e->w || docked && mother->e->w != w)){
		mother = NULL;

		// If the mother is lost, give up docking sequence.
		if(task == Dock || task == Dockque)
			task = Idle;
	}
	if(enemy && enemy->w != w)
		enemy = NULL;
	st::postframe();
}

bool Sceptor::isTargettable()const{
	return true;
}
bool Sceptor::isSelectable()const{return true;}

double Sceptor::hitradius()const{
	return .01;
}

int Sceptor::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	double sc[3];
	double best = dt, retf;
	int reti = 0, i, n;
	for(n = 0; n < nhitboxes; n++){
		Vec3d org;
		Quatd rot;
		org = this->rot.itrans(hitboxes[n].org) + this->pos;
		rot = this->rot * hitboxes[n].rot;
		int i;
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

#if 0
static int SCEPTOR_visibleFrom(const entity_t *viewee, const entity_t *viewer){
	SCEPTOR_t *p = (SCEPTOR_t*)viewee;
	return p->fcloak < .5;
}

static double SCEPTOR_signalSpectrum(const entity_t *pt, double lwl){
	SCEPTOR_t *p = (SCEPTOR_t*)pt;
	double x;
	double ret = 0.5;

	/* infrared emission */
	x = (lwl - -4) * .8; /* around 100 um */
	ret += (p->heat) * exp(-x * x);

	/* radar reflection */
	x = (lwl - log10(1)) * .5;
	ret += .5 * exp(-x * x); /* VHF */

	/* cloaking hides visible signatures */
	x = lwl - log10(475e-9);
	ret *= 1. - .95 * p->fcloak * exp(-x * x);

	/* engine burst emission cannot be hidden by cloaking */
	x = (lwl - -4) * .2; /* around 100 um */
	ret += 3. * (p->throttle) * exp(-x * x);

	return ret;
}


static warf_t *SCEPTOR_warp_dest(entity_t *pt, const warf_t *w){
	SCEPTOR_t *p = (SCEPTOR_t*)pt;
	if(!p->mother || !p->mother->st.st.vft->warp_dest)
		return NULL;
	return p->mother->st.st.vft->warp_dest(p->mother, w);
}
#endif

void Sceptor::dockCommand(Docker *){
	task = Dockque;
}

double Sceptor::maxfuel()const{
	return 120.;
}

bool Sceptor::command(unsigned commid, std::set<Entity*>*){
	if(commid == cid_parade_formation){
		task = Parade;
		if(!mother)
			findMother();
		return true;
	}
	else if(commid == cid_dock){
		task = Dockque;
	}
}


Entity *Sceptor::create(WarField *w, Builder *mother){
	Sceptor *ret = new Sceptor(NULL);
	ret->pos = mother->pos;
	ret->velo = mother->velo;
	ret->rot = mother->rot;
	ret->omg = mother->omg;
	ret->race = mother->race;
	ret->task = Undock;
//	w->addent(ret);
	return ret;
}

int Sceptor::cmd_dock(int argc, char *argv[], void *pv){
	Player *pl = (Player*)pv;
	for(Entity *e = pl->selected; e; e = e->selectnext){
		e->command(cid_dock);
	}
	return 0;
}

int Sceptor::cmd_parade_formation(int argc, char *argv[], void *pv){
	Player *pl = (Player*)pv;
	for(Entity *e = pl->selected; e; e = e->selectnext){
		e->command(cid_parade_formation);
	}
	return 0;
}

const unsigned Sceptor::cid_parade_formation = registerCommand();
const unsigned Sceptor::cid_dock = registerCommand();
