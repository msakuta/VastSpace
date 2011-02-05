/** \file
 * \brief Defines Attacker and AttackerDocker.
 */
#include "war.h"
#include "Warpable.h"
#include "material.h"
#include "btadapt.h"
#include "judge.h"
#include "Docker.h"
#include "arms.h"
#include "EntityCommand.h"
#include "draw/effects.h"
#include "draw/WarDraw.h"
extern "C"{
#include <clib/gl/gldraw.h>
}

class AttackerDocker;

/// A heavy assault ship with docking bay.
class Attacker : public Warpable{
	AttackerDocker *docker;
	ArmBase **turrets;
	static hardpoint_static *hardpoints;
	static int nhardpoints;
public:
	typedef Warpable st;
	static hitbox hitboxes[];
	static const unsigned nhitboxes;
	const char *classname()const;
	static const unsigned classid, entityid;
	Attacker();
	Attacker(WarField *);
	~Attacker();
	void static_init();
	virtual void anim(double dt);
	virtual void draw(wardraw_t *);
	virtual void drawOverlay(wardraw_t *);
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual bool command(EntityCommand *com);
	virtual double hitradius()const;
	virtual double maxhealth()const;
	virtual double maxenergy()const;
	virtual ArmBase *armsGet(int);
	virtual int armsCount()const;
	virtual const maneuve &getManeuve()const;
	virtual Docker *getDockerInt();
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn);
	virtual int takedamage(double damage, int hitpart);
	virtual void enterField(WarField *);
};

class AttackerDocker : public Docker{
public:
	int nextport;
	AttackerDocker(Entity *ae = NULL) : st(ae), nextport(0){}
	typedef Docker st;
	static const unsigned classid;
	const char *classname()const;
	virtual bool undock(Dockable *);
	virtual Vec3d getPortPos()const;
	virtual Quatd getPortRot()const;
};











hitbox Attacker::hitboxes[] = {
	hitbox(Vec3d(0., -0.0025, .050), Quatd(0,0,0,1), Vec3d(.125, .060, .150)),
	hitbox(Vec3d(0.070, 0.015, -.165), Quatd(0,0,0,1), Vec3d(.030, .035, .065)),
	hitbox(Vec3d(-0.070, 0.015, -.165), Quatd(0,0,0,1), Vec3d(.030, .035, .065)),
};
const unsigned Attacker::nhitboxes = numof(hitboxes);
struct hardpoint_static *Attacker::hardpoints = NULL;
int Attacker::nhardpoints = 0;

const char *Attacker::classname()const{return "Attacker";}
const unsigned Attacker::classid = registerClass("Attacker", Conster<Attacker>);
const unsigned Attacker::entityid = registerEntity("Attacker", new Constructor<Attacker>);

Attacker::Attacker() : docker(NULL){}

Attacker::Attacker(WarField *aw) : st(aw), docker(new AttackerDocker(this)){
	static_init();
	init();
	static int count = 0;
	turrets = new ArmBase*[nhardpoints];
	for(int i = 0; i < nhardpoints; i++){
		turrets[i] = (true || count % 2 ? (LTurretBase*)new LTurret(this, &hardpoints[i]) : (LTurretBase*)new LMissileTurret(this, &hardpoints[i]));
		if(aw)
			aw->addent(turrets[i]);
	}
	count++;
	mass = 2e8;
	health = maxhealth();
	capacitor = maxenergy();
}

Attacker::~Attacker(){delete docker;}

void Attacker::static_init(){
	if(!hardpoints){
		hardpoints = hardpoint_static::load("Attacker.hps", nhardpoints);
	}
}

void Attacker::anim(double dt){
	st::anim(dt);
	docker->anim(dt);
	for(int i = 0; i < nhardpoints; i++) if(turrets[i])
		turrets[i]->align();
}


void Attacker::draw(wardraw_t *wd){
	static suf_t *sufbase, *sufbridge;
	static suftex_t *pst, *pstbridge;
	static bool init = false;

	draw_healthbar(this, wd, health / maxhealth(), .3, -1, capacitor / maxenergy());

	if(wd->vw->gc->cullFrustum(pos, hitradius()))
		return;

	if(!init) do{
		sufbase = CallLoadSUF("models/attack_body.bin");
		if(!sufbase) break;
		sufbridge = CallLoadSUF("models/attack_bridge.bin");
//		CallCacheBitmap("bricks.bmp", "bricks.bmp", NULL, NULL);
//		CallCacheBitmap("runway.bmp", "runway.bmp", NULL, NULL);
		suftexparam_t stp;
		stp.flags = STP_MAGFIL | STP_MINFIL | STP_ENV;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		stp.env = GL_ADD;
		stp.mipmap = 0;
		CallCacheBitmap5("attacker_engine.bmp", "models/attacker_engine_br.bmp", &stp, "models/attacker_engine.bmp", NULL);
		CacheSUFMaterials(sufbase);
		CacheSUFMaterials(sufbridge);
		pst = AllocSUFTex(sufbase);
		pstbridge = AllocSUFTex(sufbridge);
		init = true;
	} while(0);

	if(sufbase){
		double scale = .001;
		Mat4d mat;

		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);

#if 0
		for(int i = 0; i < nhitboxes; i++){
			Mat4d rot;
			glPushMatrix();
			gldTranslate3dv(hitboxes[i].org);
			rot = hitboxes[i].rot.tomat4();
			glMultMatrixd(rot);
			hitbox_draw(this, hitboxes[i].sc);
			glPopMatrix();
		}
#endif

		glPushMatrix();
		glScaled(-scale, scale, -scale);
		DecalDrawSUF(sufbase, SUF_ATR, NULL, pst, NULL, NULL);
		glPushMatrix();
		glScaled(-1,1,1);
		glFrontFace(GL_CW);
		DecalDrawSUF(sufbase, SUF_ATR, NULL, pst, NULL, NULL);
		glFrontFace(GL_CCW);
		glPopMatrix();
		DecalDrawSUF(sufbridge, SUF_ATR, NULL, pstbridge, NULL, NULL);
		glPopMatrix();

		glPopMatrix();
		glPopAttrib();
	}
}

void Attacker::drawOverlay(wardraw_t *){
	glBegin(GL_LINE_LOOP);
	glVertex2d(-.15,  1.0);
	glVertex2d(-.4,  1.0);
	glVertex2d(-.5,  0.9);
	glVertex2d(-.5,  0.2);
	glVertex2d(-.9, -0.5);
	glVertex2d(-.9, -0.7);
	glVertex2d(-.5, -0.8);
	glVertex2d(-.5, -0.9);
	glVertex2d(-.2, -1.0);
	glVertex2d( .2, -1.0);
	glVertex2d( .5, -0.9);
	glVertex2d( .5, -0.8);
	glVertex2d( .9, -0.7);
	glVertex2d( .9, -0.5);
	glVertex2d( .5,  0.2);
	glVertex2d( .5,  0.9);
	glVertex2d( .4,  1.0);
	glVertex2d( .15,  1.0);
	glVertex2d( .15,  0.3);
	glVertex2d(-.15,  0.3);
	glEnd();
}

void Attacker::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	static const Vec3d offsets[2] = {Vec3d(0, .065, .110), Vec3d(0, .165, .350)};
	rot = this->rot;
	pos = rot.trans(offsets[seatid % 2]) + this->pos;
}

bool Attacker::command(EntityCommand *com){
	return st::command(com);
}

double Attacker::hitradius()const{return .3;}
double Attacker::maxhealth()const{return 100000.;}
double Attacker::maxenergy()const{return 200000.;}

ArmBase *Attacker::armsGet(int index){
	if(0 <= index && index < nhardpoints)
		return turrets[index];
	return NULL;
}

int Attacker::armsCount()const{return nhardpoints;}

const Warpable::maneuve &Attacker::getManeuve()const{
	static const struct Warpable::maneuve mn = {
		.025, /* double accel; */
		.1, /* double maxspeed; */
		5000000 * .1, /* double angleaccel; */
		.2, /* double maxanglespeed; */
		200000., /* double capacity; [MJ] */
		300., /* double capacitor_gen; [MW] */
	};
	return mn;
}

Docker *Attacker::getDockerInt(){
	return docker;
}

int Attacker::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
#if 0
	return st::tracehit(src, dir, rad, dt, ret, retp, retn);
#else
	double sc[3];
	double best = dt, retf;
	int reti = 0, i, n;
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
#endif
}

int Attacker::takedamage(double damage, int hitpart){
	struct tent3d_line_list *tell = w->getTeline3d();
	int ret = 1;

//	playWave3D("hit.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
	if(0 < health && health - damage <= 0){
		int i;
		ret = 0;
		WarSpace *ws = *w;
		if(ws){

#if 1
			if(ws->gibs)
				AddTelineCallback3D(ws->gibs, pos, velo, .010, quat_u, omg, vec3_000, debrigib_multi, (void*)64, TEL3_QUAT | TEL3_NOLINE, 20.);
#else
			if(ws->gibs) for(i = 0; i < 128; i++){
				double pos[3], velo[3] = {0}, omg[3];
				/* gaussian spread is desired */
				for(int j = 0; j < 6; j++)
					velo[j / 2] += .025 * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				omg[0] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				omg[1] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				omg[2] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				VECCPY(pos, this->pos);
				for(int j = 0; j < 3; j++)
					pos[j] += hitradius() * (drseq(&w->rs) - .5);
				AddTelineCallback3D(ws->gibs, pos, velo, .010, quat_u, omg, vec3_000, debrigib, NULL, TEL3_QUAT | TEL3_NOLINE, 15. + drseq(&w->rs) * 5.);
			}
#endif

			/* smokes */
			for(i = 0; i < 64; i++){
				Vec3d pos;
//				COLOR32 col = 0;
				pos = this->pos
					+ Vec3d(.3 * (w->rs.nextd() - .5),
							.3 * (w->rs.nextd() - .5),
							.3 * (w->rs.nextd() - .5));
/*				col |= COLOR32RGBA(rseq(&w->rs) % 32 + 127,0,0,0);
				col |= COLOR32RGBA(0,rseq(&w->rs) % 32 + 127,0,0);
				col |= COLOR32RGBA(0,0,rseq(&w->rs) % 32 + 127,0);
				col |= COLOR32RGBA(0,0,0,191);*/
				static smokedraw_swirl_data sdata = {COLOR32RGBA(127,127,127,191), false};
				AddTelineCallback3D(ws->tell, pos, vec3_000, .07, quat_u, Vec3d(0., 0., .05 * M_PI * w->rs.nextGauss()),
					vec3_000, smokedraw_swirl, (void*)&sdata, TEL3_INVROTATE | TEL3_NOLINE, 20.);
			}

			{/* explode shockwave thingie */
#if 0 // Shockwave effect is difficult in that which direction it should expand and how to sort order among other tranparencies.
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
				double vl = v.len();
				if(vl){
					p = sqrt(1. - q[3] * q[3]) / VECLEN(v);
					q = v * p;
				}
				else
					q = quat_u;

				AddTeline3D(tell, this->pos, vec3_000, 5., q, vec3_000, vec3_000, COLOR32RGBA(255,191,63,255), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_QUAT, 2.);
#endif
				AddTeline3D(tell, this->pos, vec3_000, 3., quat_u, vec3_000, vec3_000, COLOR32RGBA(255,255,255,127), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_INVROTATE, 2.);
			}
		}
//		playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
		this->w = NULL;
	}
	health -= damage;
	return ret;
}

void Attacker::enterField(WarField *target){
	WarSpace *ws = *target;
	if(!ws || !ws->bdw)
		return;

	// If a rigid body object is brought from the old WarSpace, reuse it rather than to reconstruct.
	if(!bbody){
		static btCompoundShape *shape = NULL;
		if(!shape){
			shape = new btCompoundShape();
			for(int i = 0; i < nhitboxes; i++){
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

		bbody = new btRigidBody(rbInfo);

//		bbody->setSleepingThresholds(.0001, .0001);
	}

	//add the body to the dynamics world
	ws->bdw->addRigidBody(bbody, 2, ~0);
}






























const unsigned AttackerDocker::classid = registerClass("AttackerDocker", Conster<AttackerDocker>);

const char *AttackerDocker::classname()const{return "AttackerDocker";}

bool AttackerDocker::undock(Entity::Dockable *pe){
	// Forbid undocking while warping
	if(e->toWarpable()->warping)
		return false;
	if(st::undock(pe)){
		pe->setPosition(&Vec3d(e->pos + e->rot.trans(Vec3d(-.085 * (nextport * 2 - 1), -0.015, 0))),
			&Quatd(e->rot * Quatd(0, 0, sin(-(nextport * 2 - 1) * 5. * M_PI / 4. / 2.), cos(5. * M_PI / 4. / 2.))),
			&Vec3d(e->velo),
			&Vec3d(e->omg));
		nextport = (nextport + 1) % 2;
		return true;
	}
	return false;
}

Vec3d AttackerDocker::getPortPos()const{
	return Vec3d(0., -0.035, 0.070);
}

Quatd AttackerDocker::getPortRot()const{
	return Quatd(1, 0, 0, 0);
}
