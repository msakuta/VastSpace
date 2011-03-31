#include "Assault.h"
#include "Beamer.h"
#include "judge.h"
#include "Player.h"
#include "serial_util.h"
#include "material.h"
#include "EntityCommand.h"
#include "judge.h"
#include "btadapt.h"
#include "glw/glwindow.h"
#include "motion.h"
extern "C"{
#include <clib/mathdef.h>
#include <clib/gl/gldraw.h>
}
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.h>
#include <BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.h>


#define BEAMER_SCALE .0002




#define SQRT2P2 (M_SQRT2/2.)

struct hardpoint_static *Assault::hardpoints = NULL/*[5] = {
	hardpoint_static(Vec3d(.000, 50 * BEAMER_SCALE, -110 * BEAMER_SCALE), Quatd(0,0,0,1), "Top Turret", 0),
	hardpoint_static(Vec3d(.000, -50 * BEAMER_SCALE, -110 * BEAMER_SCALE), Quatd(0,0,1,0), "Bottom Turret", 0),
	hardpoint_static(Vec3d(40 * BEAMER_SCALE,  .000, -225 * BEAMER_SCALE), Quatd(0,0,-SQRT2P2,SQRT2P2), "Right Turret", 0),
	hardpoint_static(Vec3d(-40 * BEAMER_SCALE,  .000, -225 * BEAMER_SCALE), Quatd(0,0,SQRT2P2,SQRT2P2), "Left Turret", 0),
	hardpoint_static(Vec3d(0, 0, 0), Quatd(0,0,0,1), "Shield Generator", 0),
}*/;
int Assault::nhardpoints = 0;



suf_t *Assault::sufbase = NULL;

Assault::Assault(WarField *aw) : st(aw), formPrev(NULL){
	st::init();
	init();
	for(int i = 0; i < nhardpoints; i++){
		turrets[i] = (0 || i % 2 ? new MTurret(this, &hardpoints[i]) : new GatlingTurret(this, &hardpoints[i]));
		if(aw)
			aw->addent(turrets[i]);
	}

	if(!aw)
		return;
	WarSpace *ws = *aw;
	if(ws && ws->bdw){
		buildBody();
	}
}

void Assault::init(){
	if(!hardpoints){
		hardpoints = hardpoint_static::load("assault.hb", nhardpoints);
	}
	turrets = new ArmBase*[nhardpoints];
	mass = 1e5;
	mother = NULL;
	paradec = -1;

}

const char *Assault::idname()const{
	return "assault";
}

const char *Assault::classname()const{
	return "Assault";
}

const unsigned Assault::classid = registerClass("Assault", Conster<Assault>);
const unsigned Assault::entityid = registerEntity("Assault", new Constructor<Assault>);

void Assault::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << formPrev;
	for(int i = 0; i < nhardpoints; i++)
		sc.o << turrets[i];
}

void Assault::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> (Entity*&)formPrev;
	for(int i = 0; i < nhardpoints; i++)
		sc.i >> turrets[i];

	WarSpace *ws = w ? *w : NULL;
	if(ws){
		// Assumes environment WarSpace is serialized first. Otherwise, this code would try to
		// add a rigid body to uninitialized dynamics world.
		buildBody();
		ws->bdw->addRigidBody(bbody);
	}
}

bool Assault::buildBody(){
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

const char *Assault::dispname()const{
	return "Sabre class";
}

void Assault::anim(double dt){
	WarSpace *ws;
	if(!(ws = *w)){
		st::anim(dt);
		return;
	}

	if(bbody && !warping){
		const btTransform &tra = bbody->getCenterOfMassTransform();
		pos = btvc(tra.getOrigin());
		rot = btqc(tra.getRotation());
		velo = btvc(bbody->getLinearVelocity());
	}

	if(health <= 0){
		w = NULL;
		return;
	}

	Entity *pm = mother ? mother->e : NULL;

	if(task == sship_undock){
		if(!mother || !mother->e)
			task = sship_idle;
		else{
			double sp;
			Vec3d dm = this->pos - mother->e->pos;
			Vec3d mzh = this->rot.trans(vec3_001);
			sp = -mzh.sp(dm);
			inputs.press |= PL_W;
			if(1. < sp)
				task = sship_parade;
		}
	}
	else if(w->getPlayer()->control != this){
		inputs.press = 0;
		if(!enemy){ /* find target */
			double best = 20. * 20.;
			Entity *t;
			for(t = w->entlist(); t; t = t->next) if(t != this && t->race != -1 && t->race != this->race && 0. < t->health && hitradius() / 2. < t->hitradius()/* && t->vft != &rstation_s*/){
				double sdist = (this->pos - t->pos).slen();
				if(sdist < best){
					this->enemy = t;
					best = sdist;
				}
			}
		}

		// In parade formation mode, be passive to battle.
		if(task == sship_parade && enemy && 5. * 5. < (enemy->pos - pos).slen()){
			enemy = NULL;
		}

		if(task == sship_delta){
			int nwingmen = 1; // Count yourself
			Assault *leader = NULL;
			for(Assault *wingman = formPrev; wingman; wingman = wingman->formPrev){
				nwingmen++;
				if(!wingman->formPrev)
					leader = wingman;
			}
			if(leader){
				double spacing = .2;
				Vec3d dp((nwingmen % 2 * 2 - 1) * (nwingmen / 2 * spacing), 0., nwingmen / 2 * spacing);
				dest = (leader->task == sship_moveto ? leader->dest : leader->pos) + dp;
				Vec3d dr = this->pos - dest;
				if(dr.slen() < .03 * .03){
					Quatd q1 = quat_u;
					this->velo += dr * (-dt * .5);
					this->rot = Quatd::slerp(this->rot, q1, 1. - exp(-dt));
				}
				else
					steerArrival(dt, dest, leader->velo, 1., .001);
			}
		}
		if(!enemy && task == sship_parade){
			if(mother){
				if(paradec == -1)
					paradec = mother->enumParadeC(mother->Frigate);
				Vec3d target, target0(1.5, -1., -1.);
				Quatd q2, q1;
				target0[0] += paradec % 10 * .30;
				target0[2] += paradec / 10 * -.30;
				target = pm->rot.trans(target0);
				target += pm->pos;
				Vec3d dr = this->pos - target;
				if(dr.slen() < .10 * .10){
					q1 = pm->rot;
//					inputs.press |= PL_W;
//					parking = 1;
					this->velo += dr * (-dt * .5);
					q2 = Quatd::slerp(this->rot, q1, 1. - exp(-dt));
					this->rot = q2;
				}
				else{
	//							p->throttle = dr.slen() / 5. + .01;
					steerArrival(dt, target, pm->velo, 1. / 10., .001);
				}
			}
			else
				task = sship_idle;
		}
/*		if(p->dock && p->undocktime == 0){
			if(pt->enemy)
				assault_undock(p, p->dock);
		}
		else */
		if(this->enemy){
			Vec3d xh, yh;
			long double sx, sy, len, len2, maxspeed = getManeuve().maxanglespeed * dt;
			Quatd qres, qrot;
			Vec3d dv = this->enemy->pos - this->pos;
			dv.normin();
			Vec3d forward = this->rot.trans(avec3_001);
			forward *= -1;
			xh = forward.vp(dv);
			len = len2 = xh.len();
			len = asinl(len);
			if(maxspeed < len){
				len = maxspeed;
			}
			len = sinl(len / 2.);
			if(len && len2){
				(Vec3d&)qrot = xh * (len / len2);
				qrot[3] = sqrt(1. - len * len);
				qres = qrot * this->rot;
				this->rot = qres.norm();
			}
			if(.9 < dv.sp(forward)){
				this->inputs.change |= PL_ENTER;
				this->inputs.press |= PL_ENTER;
				if(5. * 5. < (this->enemy->pos - this->pos).slen())
					this->inputs.press |= PL_W;
				else if((this->enemy->pos, this->pos).slen() < 1. * 1.)
					this->inputs.press |= PL_S;
				else
					this->inputs.press |= PL_A; /* strafe around */
			}
		}
	}

	if(ws)
		space_collide(this, ws, dt, NULL, NULL);

	st::anim(dt);
	for(int i = 0; i < nhardpoints; i++) if(turrets[i])
		turrets[i]->align();
}

void Assault::postframe(){
	st::postframe();
	for(int i = 0; i < nhardpoints; i++) if(turrets[i] && turrets[i]->w != w)
		turrets[i] = NULL;
	if(mother && (!mother->e || !mother->e->w))
		mother = NULL;
}

void Assault::draw(wardraw_t *wd){
	Assault *const p = this;
	static int init = 0;
	static suftex_t *pst;
	static GLuint list = 0;
	if(!w)
		return;

	/* cull object */
	if(cull(wd))
		return;
//	wd->lightdraws++;

	draw_healthbar(this, wd, health / maxhealth(), .1, shieldAmount / maxshield(), capacitor / maxenergy());

	if(init == 0) do{
		sufbase = CallLoadSUF("models/assault.bin");
		if(!sufbase) break;
		Beamer::cache_bridge();
		pst = AllocSUFTex(sufbase);
		init = 1;
	} while(0);
	if(sufbase){
		static const double normal[3] = {0., 1., 0.};
		double scale = BEAMER_SCALE;
		static const GLdouble rotaxis[16] = {
			-1,0,0,0,
			0,1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};
		Mat4d mat;

		// testing global mesh
/*		glColor4ub(255,0,255,255);
		glBegin(GL_LINES);
		for(int i = -10; i <= 10; i++){
			glVertex3d(i / 10., -1., 0.);
			glVertex3d(i / 10.,  1., 0.);
			glVertex3d(-1., i / 10., 0.);
			glVertex3d( 1., i / 10., 0.);
		}
		glEnd();*/

		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);
/*		if(!list){
		glNewList(list = glGenLists(1), GL_COMPILE);*/
#if 0
		for(int i = 0; i < nhitboxes; i++){
			Mat4d rot;
			glPushMatrix();
			gldTranslate3dv(hitboxes[i].org);
			rot = hitboxes[i].rot.tomat4();
			glMultMatrixd(rot);
			hitbox hb(mat.vp3(hitboxes[i].org), this->rot * hitboxes[i].rot, hitboxes[i].sc);
			hitbox_draw(this, hitboxes[i].sc/*, jHitBoxPlane(hb, Vec3d(0,0,0), Vec3d(0,0,1))*/);
			glPopMatrix();
		}
#endif

		glPushMatrix();
		glScaled(scale, scale, scale);
		glMultMatrixd(rotaxis);
/*		glPushAttrib(GL_TEXTURE_BIT);*/
/*		DrawSUF(beamer_s.sufbase, SUF_ATR, &g_gldcache);*/
		DecalDrawSUF(sufbase, SUF_ATR, NULL, pst, NULL, NULL);
/*		DecalDrawSUF(&suf_beamer, SUF_ATR, &g_gldcache, beamer_s.tex, pf->sd, &beamer_s);*/
/*		glPopAttrib();*/
		glPopMatrix();
/*		glEndList();
		}

		glCallList(list);*/

/*		for(i = 0; i < numof(pf->turrets); i++)
			mturret_draw(&pf->turrets[i], pt, wd);*/
		glPopMatrix();
	}
}

void Assault::drawtra(wardraw_t *wd){
	st::drawtra(wd);
	if(cull(wd))
		return;
	drawCapitalBlast(wd, Vec3d(0,-0.003,.06), .01);
	drawShield(wd);
}

void Assault::drawOverlay(wardraw_t *){
	glBegin(GL_LINE_LOOP);
	glVertex2d(-.40, -.25);
	glVertex2d(-.80, -.1);
	glVertex2d(-.80,  .05);
	glVertex2d(-.4 ,  .3);
	glVertex2d( .1 ,  .3);
	glVertex2d( .7 ,  .6);
	glVertex2d( .7 ,  .3);
	glVertex2d( .8 ,  .3);
	glVertex2d( .8 , -.2);
	glVertex2d( .7 , -.2);
	glVertex2d( .7 , -.6);
	glVertex2d( .4 , -.4);
	glVertex2d( .3 , -.25);
	glEnd();
}

double Assault::maxhealth()const{return 10000.;}

int Assault::armsCount()const{return numof(turrets);}

ArmBase *Assault::armsGet(int i){
	if(i < 0 || armsCount() <= i)
		return NULL;
	return turrets[i];
}

bool Assault::undock(Docker *d){
//	if(!st::undock(d))
//		return false;
	task = sship_undock;
	mother = d;
	for(int i = 0; i < 4; i++) if(turrets[i])
		d->e->w->addent(turrets[i]);
	d->baycool += 5.;
	return true;
}

bool Assault::command(EntityCommand *com){
	if(InterpretCommand<DeltaCommand>(com)){
		Entity *e;
		for(e = next; e; e = e->next) if(e->classname() == classname() && e->race == race){
			formPrev = static_cast<Assault*>(e->toWarpable());
			task = sship_delta;
			break;
		}
		return true;
	}
	if(com->derived(AttackCommand::sid)){
		for(int i = 0; i < nhardpoints; i++) if(turrets[i])
			turrets[i]->command(com);
		return st::command(com);
	}
	return st::command(com);
}



/*Entity *Assault::create(WarField *w, Builder *mother){
	Assault *ret = new Assault(NULL);
	ret->pos = mother->pos;
	ret->velo = mother->velo;
	ret->rot = mother->rot;
	ret->omg = mother->omg;
	ret->race = mother->race;
//	w->addent(ret);
	return ret;
}*/

/*const Builder::BuildStatic Assault::builds = {
	"Sabre class",
	Assault::create,
	100.,
	600.,
};*/











class GLWarms : public GLwindowSizeable{
	Entity *a;
public:
	typedef GLwindowSizeable st;
	GLWarms(const char *title, Entity *a);
	virtual void draw(GLwindowState &ws, double t);
	virtual void postframe();
};

GLWarms::GLWarms(const char *atitle, Entity *aa) : st(atitle), a(aa){
	xpos = 120;
	ypos = 40;
	width = 200;
	height = 200;
	flags |= GLW_CLOSE;
}

void GLWarms::draw(GLwindowState &ws, double t){
	if(!a)
		return;
	glColor4f(0,1,1,1);
	glwpos2d(xpos, ypos + (2) * getFontHeight());
	glwprintf(a->classname());
	glColor4f(1,1,0,1);
	glwpos2d(xpos, ypos + (3) * getFontHeight());
	glwprintf("%lg / %lg", a->health, a->maxhealth());
	for(int i = 0; i < a->armsCount(); i++){
		const ArmBase *arm = a->armsGet(i);
		glColor4f(0,1,1,1);
		glwpos2d(xpos, ypos + (4 + 2 * i) * getFontHeight());
		glwprintf(arm->hp->name);
		glColor4f(1,1,0,1);
		glwpos2d(xpos, ypos + (5 + 2 * i) * getFontHeight());
		glwprintf(arm ? arm->descript() : "N/A");
	}
}

void GLWarms::postframe(){
	if(a && !a->w)
		a = NULL;
}

int cmd_armswindow(int argc, char *argv[], void *pv){
	Player *ppl = (Player*)pv;
	if(!ppl || !ppl->selected)
		return 0;
	glwAppend(new GLWarms("Arms", ppl->selected));
	return 0;
}

