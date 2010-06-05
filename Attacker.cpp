#include "war.h"
#include "Warpable.h"
#include "material.h"
#include "btadapt.h"
#include "judge.h"
#include "Scarry.h"
#include "EntityCommand.h"
#include "Sceptor.h"
extern "C"{
#include <clib/gl/gldraw.h>
}

class AttackerDocker;

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
	~Attacker(){delete docker;}
	void static_init();
	virtual void anim(double dt);
	virtual void draw(wardraw_t *);
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
	hitbox(Vec3d(0., 0., .050), Quatd(0,0,0,1), Vec3d(.125, .055, .150)),
	hitbox(Vec3d(0.070, 0.015, -.165), Quatd(0,0,0,1), Vec3d(.030, .035, .065)),
	hitbox(Vec3d(-0.070, 0.015, -.165), Quatd(0,0,0,1), Vec3d(.030, .035, .065)),
};
const unsigned Attacker::nhitboxes = numof(hitboxes);
struct hardpoint_static *Attacker::hardpoints = NULL;
int Attacker::nhardpoints = 0;

const char *Attacker::classname()const{return "Attacker";}
const unsigned Attacker::classid = registerClass("Attacker", Conster<Attacker>);
const unsigned Attacker::entityid = registerEntity("Attacker", Constructor<Attacker>);

Attacker::Attacker() : docker(NULL){}

Attacker::Attacker(WarField *aw) : st(aw), docker(new AttackerDocker(this)){
	static_init();
	init();
	static int count = 0;
	turrets = new ArmBase*[nhardpoints];
	for(int i = 0; i < nhardpoints; i++){
		turrets[i] = (count % 2 ? (LTurretBase*)new LTurret(this, &hardpoints[i]) : (LTurretBase*)new LMissileTurret(this, &hardpoints[i]));
		if(aw)
			aw->addent(turrets[i]);
	}
	count++;
	mass = 2e8;
	health = maxhealth();

	for(int i = 0; i < 6; i++)
		docker->addent(new Sceptor(docker));

	if(!aw)
		return;
	WarSpace *ws = *aw;
	if(ws && ws->bdw){
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

		//add the body to the dynamics world
		ws->bdw->addRigidBody(bbody, 2, ~0);
	}
}

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
		static const double normal[3] = {0., 1., 0.};
		double scale = .001;
		Mat4d mat;

		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);

#if 1
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
double Attacker::maxenergy()const{return 100000.;}

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
		150000., /* double capacity; [MJ] */
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
































const unsigned AttackerDocker::classid = registerClass("AttackerDocker", Conster<AttackerDocker>);

const char *AttackerDocker::classname()const{return "AttackerDocker";}

bool AttackerDocker::undock(Entity::Dockable *pe){
	if(st::undock(pe)){
		pe->pos = e->pos + e->rot.trans(Vec3d(-.085 * (nextport * 2 - 1), -0.015, 0));
		pe->velo = e->velo;
		pe->rot = e->rot * Quatd(0, 0, sin(-(nextport * 2 - 1) * 5. * M_PI / 4. / 2.), cos(5. * M_PI / 4. / 2.));
		if(pe->bbody){
			pe->bbody->setCenterOfMassTransform(btTransform(btqc(pe->rot), btvc(pe->pos)));
			pe->bbody->setLinearVelocity(btvc(pe->velo));
		}
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
