#include "../Frigate.h"
#include "../Docker.h"
#include "../player.h"
//#include "bullet.h"
#include "../coordsys.h"
#include "../viewer.h"
#include "../cmd.h"
//#include "glwindow.h"
#include "../judge.h"
#include "../astrodef.h"
#include "../stellar_file.h"
#include "../astro_star.h"
#include "../serial_util.h"
#include "../material.h"
//#include "sensor.h"
#include "../motion.h"
extern "C"{
#include "../bitmap.h"
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/suf/sufbin.h>
#include <clib/suf/sufdraw.h>
#include <clib/suf/sufvbo.h>
#include <clib/GL/gldraw.h>
#include <clib/GL/cull.h>
#include <clib/GL/multitex.h>
#include <clib/wavsound.h>
#include <clib/zip/UnZip.h>
}
#include <assert.h>
#include <string.h>
#include <gl/glext.h>



/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289

#define MAX_SHIELD_AMOUNT 5000.
#define BEAMER_HEALTH 15000.
#define BEAMER_SCALE .0002
#define BEAMER_MAX_SPEED .1
#define BEAMER_ACCELERATE .05
#define BEAMER_MAX_ANGLESPEED .4
#define BEAMER_ANGLEACCEL .2
#define BEAMER_SHIELDRAD .09



class ContainerHead : public Frigate{
public:
	typedef Frigate st;
protected:
	double charge;
	Vec3d integral;
	double beamlen;
	double cooldown;
//	struct tent3d_fpol *pf[1];
//	scarry_t *dock;
	float undocktime;
	static const double sufscale;
public:
	ContainerHead(){init();}
	ContainerHead(WarField *w);
	void init();
	const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid, entityid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual void anim(double);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual double maxhealth()const;
	virtual Props props()const;
	virtual bool undock(Docker*);
	static void cache_bridge(void);
	static Entity *create(WarField *w, Builder *);
//	static const Builder::BuildStatic builds;
};


ContainerHead::ContainerHead(WarField *aw) : st(aw){
	init();
}

void ContainerHead::init(){
	charge = 0.;
//	dock = NULL;
	undocktime = 0.f;
	cooldown = 0.;
	integral.clear();
	health = maxhealth();
	mass = 1e5;
}

const char *ContainerHead::idname()const{
	return "ContainerHead";
}

const char *ContainerHead::classname()const{
	return "ContainerHead";
}

const unsigned ContainerHead::classid = registerClass("ContainerHead", Conster<ContainerHead>);
const unsigned ContainerHead::entityid = registerEntity("ContainerHead", new Constructor<ContainerHead>);

void ContainerHead::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << charge;
	sc.o << integral;
	sc.o << beamlen;
	sc.o << cooldown;
//	scarry_t *dock;
	sc.o << undocktime;
}

void ContainerHead::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> charge;
	sc.i >> integral;
	sc.i >> beamlen;
	sc.i >> cooldown;
//	scarry_t *dock;
	sc.i >> undocktime;
}

const char *ContainerHead::dispname()const{
	return "Container Ship";
}

void ContainerHead::anim(double dt){
	if(!w->operator WarSpace *()){
		st::anim(dt);
		return;
	}

	/* forget about beaten enemy */
	if(enemy && (enemy->health <= 0. || enemy->w != w))
		enemy = NULL;

	Mat4d mat;
	transform(mat);

	if(0 < health){
		Entity *collideignore = NULL;
		int i, n;
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
		else if(w->getPlayer()->control == this){
		}
		else if(!enemy && task == sship_parade){
			Entity *pm = mother ? mother->e : NULL;
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
					inputs.press &= ~PL_W;
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
		else{
			Vec3d pos, dv;
			double dist;
			Vec3d opos;
			inputs.press = 0;
			if((task == sship_idle || task == sship_attack) && !enemy){ /* find target */
				double best = 20. * 20.;
				Entity *t;
				for(t = w->el; t; t = t->next) if(t != this && t->race != -1 && t->race != race && 0. < t->health){
					double sdist = (this->pos - t->pos).slen();
					if(sdist < best){
						enemy = t;
						best = sdist;
						task = sship_attack;
					}
				}
			}
			if(task == sship_attack && !enemy)
				task = sship_idle;

			if(task == sship_attack){
				if(this->enemy){
					Vec3d dv, forward;
					Vec3d xh, yh;
					long double sx, sy, len, len2, maxspeed = BEAMER_MAX_ANGLESPEED * dt;
					Quatd qres, qrot;
					dv = (enemy->pos - this->pos).normin();
					forward = -this->rot.trans(avec3_001);
					xh = forward.vp(dv);
					len = len2 = xh.len();
					len = asinl(len);
					if(maxspeed < len){
						len = maxspeed;
					}
					len = sinl(len / 2.);
					if(len && len2){
						double sd, df, drl, decay;
						Vec3d ptomg, omg, dvv, delta, diff;
/*						VECSCALE(qrot, xh, len / len2);
						qrot[3] = sqrt(1. - len * len);
						QUATMUL(qres, qrot, pt->rot);
						QUATNORM(pt->rot, qres);*/
/*						drl = VECDIST(pt->enemy->pos, pt->pos);
						VECSUB(dvv, pt->enemy->velo, pt->velo);
						VECVP(ptomg, dv, dvv);*/
						VECCPY(diff, xh);
/*						VECSCALEIN(xh, 1.);
						VECSADD(xh, ptomg, .0 / drl);
						VECSUB(omg, pt->omg, ptomg);
						sd = VECSDIST(omg, xh);
						df = 1. / (.1 + sd);
						df = dt * MIN(df, frigate_mn.angleaccel);
						VECSCALEIN(omg, 1. - df);
						VECADD(pt->omg, omg, ptomg);
						len2 = VECLEN(xh);*/
						df = dt * MIN(len2, frigate_mn.angleaccel) / len2;
						df = MIN(df, 1);
						delta = diff - this->omg;
						delta += integral * .2;
						this->omg += delta * df;
						if(frigate_mn.maxanglespeed * frigate_mn.maxanglespeed < this->omg.slen()){
							this->omg.normin();
							this->omg *= frigate_mn.maxanglespeed;
						}
						if(.25 < len2)
							diff *= .25 / len2;
						integral += diff * dt;
						decay = exp(-.1 * dt);
						integral *= decay;
						inputs.press |= PL_2 | PL_8;
					}
					if(.9 < dv.sp(forward)){
						inputs.change |= PL_ENTER;
						inputs.press |= PL_ENTER;
						if(5. * 5. < (enemy->pos - this->pos).slen())
							inputs.press |= PL_W;
						else if((enemy->pos - this->pos).slen() < 1. * 1.)
							inputs.press |= PL_S;
/*						else
							inputs.press |= PL_A; *//* strafe around */
					}
				}

				/* follow another */
				if(0 && !enemy && !warping){
					Entity *pt2;
					Warpable *leader = NULL;
					for(pt2 = w->el; pt2; pt2 = pt2->next) if(this != pt2 && race == pt2->race && pt2->toWarpable() && w->pl->control == pt2){
						leader = pt2->toWarpable();
						break;
					}

/*					if(leader && leader->charge){
						pt->inputs.change |= PL_ENTER;
						pt->inputs.press |= PL_ENTER;
					}*/

					/*for(pt2 = w->tl; pt2; pt2 = pt2->next) if(pt != pt2 && pt2->vft == pt->vft && w->pl->control == pt2 && ((beamer_t*)pt2)->warping)*/
					if(leader && leader->warping && leader->warpdstcs != w->cs){
						Warpable *p2 = leader;
						int k;
						warping = p2->warping;
						warpcs = NULL;
						warpdst = p2->warpdst;
						for(k = 0; k < 3; k++)
							warpdst[k] += drseq(&w->rs) - .5;
						warpdstcs = p2->warpdstcs;
	/*					break;*/
					}
				}
			}
		}
	}
	else{
		this->w = NULL;
		return;
	}

	if(cooldown == 0. && inputs.press & (PL_ENTER | PL_LCLICK)){
		charge = 6.;
		cooldown = 10.;
	}

	if(charge < dt)
		charge = 0.;
	else
		charge -= dt;

	if(cooldown < dt)
		cooldown = 0.;
	else
		cooldown -= dt;

	WarSpace *ws = *w;
/*	if(ws){
		space_collide(this, ws, dt, NULL, NULL);
	}*/

	if(0. < charge && charge < 4.){
		Entity *pt2, *hit = NULL;
		Vec3d start, dir, start0(0., 0., -.04), dir0(0., 0., -10.);
		double best = 10., sdist;
		int besthitpart = 0, hitpart;
		start = rot.trans(start0) + pos;
		dir = rot.trans(dir0);
		for(pt2 = w->el; pt2; pt2 = pt2->next){
			double rad = pt2->hitradius();
			Vec3d delta = pt2->pos - pos;
			if(pt2 == this)
				continue;
			if(!jHitSphere(pt2->pos, rad + .005, start, dir, 1.))
				continue;
			if((hitpart = pt2->tracehit(start, dir, .005, 1., &sdist, NULL, NULL)) && (sdist *= -dir0[2], 1)){
				hit = pt2;
				best = sdist;
				besthitpart = hitpart;
			}
		}
		if(ws && hit){
			Vec3d pos;
			Quatd qrot;
			beamlen = best;
			hit->takedamage(1000. * dt, besthitpart);
			pos = mat.vec3(2) * -best + this->pos;
			qrot = Quatd::direction(mat.vec3(2));
			if(drseq(&w->rs) * .1 < dt){
				avec3_t velo;
				int i;
				for(i = 0; i < 3; i++)
					velo[i] = (drseq(&w->rs) - .5) * .1;
				AddTeline3D(ws->tell, pos, velo, drseq(&w->rs) * .01 + .01, quat_u, vec3_000, vec3_000, COLOR32RGBA(0,127,255,95), TEL3_NOLINE | TEL3_GLOW | TEL3_INVROTATE, .5);
			}
			AddTeline3D(ws->tell, pos, vec3_000, drseq(&w->rs) * .25 + .25, qrot, vec3_000, vec3_000, COLOR32RGBA(0,255,255,255), TEL3_NOLINE | TEL3_CYLINDER | TEL3_QUAT, .1);
		}
		else
			beamlen = 10.;
	}
	st::anim(dt);
#if 0
	if(p->pf){
		int i;
		avec3_t pos, pos0[numof(p->pf)] = {
			{.0, -.003, .045},
		};
		for(i = 0; i < numof(p->pf); i++){
			MAT4VP3(pos, mat, pos0[i]);
			MoveTefpol3D(p->pf[i], pos, avec3_000, cs_orangeburn.t, 0);
		}
	}
#endif
}



void ContainerHead::cache_bridge(void){
	CallCacheBitmap("bridge.bmp", "bridge.bmp", NULL, NULL);
	CallCacheBitmap("beamer_panel.bmp", "beamer_panel.bmp", NULL, NULL);
}

const double ContainerHead::sufscale = BEAMER_SCALE;

void ContainerHead::draw(wardraw_t *wd){
	ContainerHead *const p = this;
	if(!w)
		return;

	/* cull object */
	if(cull(wd))
		return;
//	wd->lightdraws++;

//	draw_healthbar(this, wd, health / BEAMER_HEALTH, .1, shieldAmount / MAX_SHIELD_AMOUNT, capacitor / frigate_mn.capacity);

	static bool init = false;
	static suf_t *sufs[3] = {NULL};
	static VBO *vbo[3] = {NULL};
	static suftex_t *pst[3] = {NULL};
	if(!init){

		// Register alpha test texture
		suftexparam_t stp;
		stp.flags = STP_ALPHA | STP_ALPHA_TEST;
		AddMaterial("containerrail.bmp", "models/containerbrail.bmp", &stp, NULL, NULL);

		static const char *names[3] = {"models/containerhead.bin", "models/gascontainer.bin", "models/containertail.bin"};
		for(int i = 0; i < 3; i++){
			sufs[i] = CallLoadSUF(names[i]);
			vbo[i] = CacheVBO(sufs[i]);
			CacheSUFMaterials(sufs[i]);
			pst[i] = AllocSUFTex(sufs[i]);
		}
		init = true;
	}
	{
		static const double normal[3] = {0., 1., 0.};
		double scale = sufscale;
		static const GLdouble rotaxis[16] = {
			-1,0,0,0,
			0,1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};
		Mat4d mat;

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
		glScaled(scale, scale, scale);
		glMultMatrixd(rotaxis);
		glTranslated(0, 0, 350);
		if(vbo[0])
			DrawVBO(vbo[0], SUF_ATR | SUF_TEX, pst[0]);
		else if(sufs[0])
			DecalDrawSUF(sufs[0], SUF_ATR | SUF_TEX, NULL, pst[0], NULL, NULL);
		glTranslated(0, 0, -150);
		for(int i = 0; i < 3; i++){
			if(vbo[1])
				DrawVBO(vbo[1], SUF_ATR | SUF_TEX, pst[1]);
			else if(sufs[1])
				DecalDrawSUF(sufs[1], SUF_ATR | SUF_TEX, NULL, pst[1], NULL, NULL);
			glTranslated(0, 0, -300);
		}
		glTranslated(0, 0, 150);
		if(vbo[2])
			DrawVBO(vbo[2], SUF_ATR | SUF_TEX, pst[2]);
		else if(sufs[2])
			DecalDrawSUF(sufs[2], SUF_ATR | SUF_TEX, NULL, pst[2], NULL, NULL);
		glPopMatrix();

		glPopMatrix();
	}
}

void ContainerHead::drawtra(wardraw_t *wd){
	st::drawtra(wd);
	ContainerHead *p = this;
	Mat4d mat;

/*	if(p->dock && p->undocktime == 0)
		return;*/

	transform(mat);

	drawCapitalBlast(wd, Vec3d(0,-0.003,.06), .01);

	drawShield(wd);
}

Entity::Props ContainerHead::props()const{
	Props ret = st::props();
//	std::stringstream ss;
//	ss << "Cooldown: " << cooldown;
//	std::stringbuf *pbuf = ss.rdbuf();
//	ret.push_back(pbuf->str());
	ret.push_back(gltestp::dstring("I am ContainerHead!"));
	ret.push_back(gltestp::dstring("Cooldown: ") << cooldown);
	return ret;
}

bool ContainerHead::undock(Docker *d){
	task = sship_undock;
	mother = d;
	return true;
}

double ContainerHead::maxhealth()const{return BEAMER_HEALTH;}
