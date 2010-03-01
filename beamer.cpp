#include "beamer.h"
#include "player.h"
#include "bullet.h"
#include "coordsys.h"
#include "viewer.h"
#include "cmd.h"
#include "glwindow.h"
#include "judge.h"
#include "astrodef.h"
#include "stellar_file.h"
#include "astro_star.h"
#include "serial_util.h"
#include "material.h"
//#include "sensor.h"
extern "C"{
#include "bitmap.h"
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



static const struct Warpable::maneuve beamer_mn = {
	BEAMER_ACCELERATE, /* double accel; */
	BEAMER_MAX_SPEED, /* double maxspeed; */
	BEAMER_ANGLEACCEL, /* double angleaccel; */
	BEAMER_MAX_ANGLESPEED, /* double maxanglespeed; */
	50000., /* double capacity; [MJ] */
	100., /* double capacitor_gen; [MW] */
};


Frigate::Frigate(WarField *aw) : st(aw){
	health = maxhealth();
	shieldAmount = maxshield();
}

Beamer::Beamer(WarField *aw) : st(aw){
	init();
}

void Beamer::init(){
	charge = 0.;
//	dock = NULL;
	undocktime = 0.f;
	cooldown = 0.;
	integral.clear();
	health = maxhealth();
	mass = 1e5;
}

const char *Beamer::idname()const{
	return "beamer";
}

const char *Beamer::classname()const{
	return "Beamer";
}

const unsigned Beamer::classid = registerClass("Beamer", Conster<Beamer>);

void Beamer::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << charge;
	sc.o << integral;
	sc.o << beamlen;
	sc.o << cooldown;
//	scarry_t *dock;
	sc.o << undocktime;
}

void Beamer::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> charge;
	sc.i >> integral;
	sc.i >> beamlen;
	sc.i >> cooldown;
//	scarry_t *dock;
	sc.i >> undocktime;
}

const char *Beamer::dispname()const{
	return "Lancer Class";
}

#if 0
void beamer_dock(beamer_t *p, scarry_t *pc, warf_t *w){
	p->dock = pc;
	VECCPY(p->st.st.pos, pc->st.st.pos);
}

void beamer_undock(beamer_t *p, scarry_t *pm){
	static const avec3_t pos0 = {-100 * SCARRY_SCALE, 50 * SCARRY_SCALE, 100 * SCARRY_SCALE};
	if(p->dock != pm)
		p->dock = pm;
	if(p->dock->baycool){
		VECCPY(p->st.st.pos, p->dock->st.st.pos);
		p->undocktime = 0.f;
	}
	else{
		entity_t *dock = &p->dock->st.st;
		p->dock->baycool = p->undocktime = 15.f;
		QUATCPY(p->st.st.rot, dock->rot);
		quatrot(p->st.st.pos, dock->rot, pos0);
		VECADDIN(p->st.st.pos, dock->pos);
		VECCPY(p->st.st.velo, dock->velo);
	}
}
#endif

#if 0
static void beamer_cockpitview(struct entity *pt, warf_t *w, double (*pos)[3], int *chasecamera){
	avec3_t ofs;
	static const avec3_t src[] = {
		{.002, .018, .022},
#if 1
		{0., 0., 1.0},
#else
		{0., 0./*05*/, 0.150},
		{0., 0./*1*/, 0.300},
		{0., 0., 1.000},
		{0., 0., 2.000},
#endif
	};
	amat4_t mat;
	aquat_t q;
	*chasecamera = (*chasecamera + numof(src)) % numof(src);
	if(*chasecamera){
		QUATMUL(q, pt->rot, w->pl->rot);
		quat2mat(&mat, q);
	}
	else
		quat2mat(&mat, pt->rot);
	mat4vp3(ofs, mat, src[*chasecamera]);
	if(*chasecamera)
		VECSCALEIN(ofs, g_viewdist);
	VECADD(*pos, pt->pos, ofs);
}

extern struct astrobj earth, island3, moon, jupiter;
extern struct player *ppl;
#endif


void Frigate::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << shieldAmount;
}

void Frigate::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> shieldAmount;
}


void Frigate::cockpitView(Vec3d &pos, Quatd &rot, int)const{
	pos = this->pos + this->rot.trans(Vec3d(.0, .05, .15));
	rot = this->rot;
}
void Frigate::anim(double dt){
	st::anim(dt);

	/* shield regeneration */
	if(0 < health){
		if(MAX_SHIELD_AMOUNT < shieldAmount + dt * 5.)
			shieldAmount = MAX_SHIELD_AMOUNT;
		else
			shieldAmount += dt * 5.;
	}
	else
		w = NULL;

	se.anim(dt);

}
double Frigate::hitradius()const{return .1;}
Entity::Dockable *Frigate::toDockable(){return this;}
const Warpable::maneuve &Frigate::getManeuve()const{return beamer_mn;}

void Beamer::anim(double dt){
	Mat4d mat;

	if(!w)
		return;

	/* forget about beaten enemy */
	if(enemy && (enemy->health <= 0. || enemy->w != w))
		enemy = NULL;

/*	if(p->dock && !p->undocktime){
		beamer_undock(p, p->dock);
		return;
	}*/

//	tankrot(&mat, pt);
	mat = Mat4d(mat4_u).translatein(pos) * rot.tomat4();

	if(0 < health){
		Entity *collideignore = NULL;
		int i, n;
/*		if(p->dock && p->undocktime){
			pt->inputs.press = PL_W;

			if(p->undocktime <= dt){
				p->undocktime = 0.;
				p->dock = NULL;
			}
			else
				p->undocktime -= dt;
		}
		else if(w->pl->control == pt){
		}
		else*/{
			Vec3d pos, dv;
			double dist;
			Vec3d opos;
			inputs.press = 0;
			if(!enemy){ /* find target */
				double best = 20. * 20.;
				Entity *t;
				for(t = w->el; t; t = t->next) if(t != this && t->race != -1 && t->race != race && 0. < t->health){
					double sdist = (this->pos - t->pos).slen();
					if(sdist < best){
						enemy = t;
						best = sdist;
					}
				}
			}

/*			if(p->dock && p->undocktime == 0){
				if(pt->enemy)
					beamer_undock(p, p->dock);
			}
			else*/{
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
						df = dt * MIN(df, beamer_mn.angleaccel);
						VECSCALEIN(omg, 1. - df);
						VECADD(pt->omg, omg, ptomg);
						len2 = VECLEN(xh);*/
						df = dt * MIN(len2, beamer_mn.angleaccel) / len2;
						df = MIN(df, 1);
						delta = diff - this->omg;
						delta += integral * .2;
						this->omg += delta * df;
						if(beamer_mn.maxanglespeed * beamer_mn.maxanglespeed < this->omg.slen()){
							this->omg.normin();
							this->omg *= beamer_mn.maxanglespeed;
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
	if(ws){
		space_collide(this, ws, dt, NULL, NULL);
	}

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
			hit->takedamage(500. * dt, besthitpart);
			pos = mat.vec3(2) * -best + this->pos;
			qrot = Quatd::direction(mat.vec3(2));
			if(drseq(&w->rs) * .1 < dt){
				avec3_t velo;
				int i;
				for(i = 0; i < 3; i++)
					velo[i] = (drseq(&w->rs) - .5) * .1;
				AddTeline3D(ws->tell, pos, velo, drseq(&w->rs) * .01 + .01, quat_u, vec3_000, vec3_000, COLOR32RGBA(0,127,255,95), TEL3_NOLINE | TEL3_GLOW | TEL3_INVROTATE, .5);
			}
			AddTeline3D(ws->tell, pos, NULL, drseq(&w->rs) * .25 + .25, qrot, vec3_000, vec3_000, COLOR32RGBA(0,255,255,255), TEL3_NOLINE | TEL3_CYLINDER | TEL3_QUAT, .1);
		}
		else
			beamlen = 10.;
	}
#if 1
	st::anim(dt);
#endif
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

bool Frigate::cull(wardraw_t *wd){
	double pixels;
	if(wd->vw->gc->cullFrustum(this->pos, .6))
		return true;
	pixels = .8 * fabs(wd->vw->gc->scale(this->pos));
	if(pixels < 2)
		return true;
	return false;
}

struct hitbox Frigate::hitboxes[] = {
	hitbox(Vec3d(0., 0., -.005), Quatd(0,0,0,1), Vec3d(.015, .015, .060)),
	hitbox(Vec3d(.025, -.015, .02), Quatd(0,0, -SIN15, COS15), Vec3d(.0075, .002, .02)),
	hitbox(Vec3d(-.025, -.015, .02), Quatd(0,0, SIN15, COS15), Vec3d(.0075, .002, .02)),
	hitbox(Vec3d(.0, .03, .0325), Quatd(0,0,0,1), Vec3d(.002, .008, .010)),
};
const int Frigate::nhitboxes = numof(Beamer::hitboxes);


void Beamer::cache_bridge(void){
	CallCacheBitmap("bridge.bmp", "bridge.bmp", NULL, NULL);
	CallCacheBitmap("beamer_panel.bmp", "beamer_panel.bmp", NULL, NULL);
/*	if(!FindTexCache("bridge.bmp")){
		suftexparam_t stp;
		stp.bmi = lzuc(lzw_bridge, sizeof lzw_bridge, NULL);
		stp.env = GL_MODULATE;
		stp.mipmap = 0;
		stp.alphamap = 0;
		stp.magfil = GL_LINEAR;
		CacheSUFMTex("bridge.bmp", &stp, NULL);
		free(stp.bmi);
	}
	if(!FindTexCache("beamer_panel.bmp")){
		BITMAPINFO *bmi;
		bmi = lzuc(lzw_beamer_panel, sizeof lzw_beamer_panel, NULL);
		CacheSUFTex("beamer_panel.bmp", bmi, 0);
		free(bmi);
	}*/
}

suf_t *Beamer::sufbase = NULL;
const double Beamer::sufscale = BEAMER_SCALE;

void Beamer::draw(wardraw_t *wd){
	Beamer *const p = this;
	static int init = 0;
	static suftex_t *pst;
	if(!w)
		return;

	/* cull object */
	if(cull(wd))
		return;
//	wd->lightdraws++;

	draw_healthbar(this, wd, health / BEAMER_HEALTH, .1, shieldAmount / MAX_SHIELD_AMOUNT, capacitor / beamer_mn.capacity);

	if(init == 0) do{
//		suftexparam_t stp, stp2;
/*		beamer_s.sufbase = LZUC(lzw_assault);*/
/*		beamer_s.sufbase = LZUC(lzw_beamer);*/
		sufbase = CallLoadSUF("models/beamer.bin");
		if(!sufbase) break;
		cache_bridge();
		pst = AllocSUFTex(sufbase);
/*		beamer_hb[1].rot[2] = sin(M_PI / 3.);
		beamer_hb[1].rot[3] = cos(M_PI / 3.);
		beamer_hb[2].rot[2] = sin(-M_PI / 3.);
		beamer_hb[2].rot[3] = cos(-M_PI / 3.);*/
		init = 1;
	} while(0);
	if(sufbase){
		static const double normal[3] = {0., 1., 0.};
		double scale = BEAMER_SCALE;
//		double x;
//		int i;
/*		static const GLdouble rotaxis[16] = {
			0,0,-1,0,
			-1,0,0,0,
			0,1,0,0,
			0,0,0,1,
		};*/
		static const GLdouble rotaxis[16] = {
			-1,0,0,0,
			0,1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};
		Mat4d mat;
/*		glBegin(GL_POINTS);
		glVertex3dv(pt->pos);
		glEnd();*/

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

/*		glTranslated(pt->pos[0], pt->pos[1], pt->pos[2]);
		glRotated(deg_per_rad * pt->pyr[1], 0., -1., 0.);
		glRotated(deg_per_rad * pt->pyr[0], -1.,  0., 0.);
		glRotated(deg_per_rad * pt->pyr[2], 0.,  0., -1.);*/
		glPushMatrix();
		glScaled(scale, scale, scale);
		glMultMatrixd(rotaxis);
/*		glPushAttrib(GL_TEXTURE_BIT);*/
/*		DrawSUF(beamer_s.sufbase, SUF_ATR, &g_gldcache);*/
		DecalDrawSUF(sufbase, SUF_ATR, NULL, pst, NULL, NULL);
/*		DecalDrawSUF(&suf_beamer, SUF_ATR, &g_gldcache, beamer_s.tex, pf->sd, &beamer_s);*/
/*		glPopAttrib();*/
		glPopMatrix();

/*		for(i = 0; i < numof(pf->turrets); i++)
			mturret_draw(&pf->turrets[i], pt, wd);*/
		glPopMatrix();

/*		if(0 < wd->light[1]){
			static const double normal[3] = {0., 1., 0.};
			ShadowSUF(&suf_beamer, wd->light, normal, pt->pos, pt->pyr, scale, rotaxis);
		}*/
	/*
		glPushMatrix();
		VECCPY(pyr, pt->pyr);
		pyr[0] += M_PI / 2;
		pyr[1] += pt->turrety;
		glMultMatrixd(rotaxis);
		ShadowSUF(train_s.sufbase, wd->light, normal, pt->pos, pt->pyr, scale);
		VECCPY(pyr, pt->pyr);
		pyr[1] += pt->turrety;
		ShadowSUF(train_s.sufturret, wd->light, normal, pt->pos, pyr, tscale);
		glPopMatrix();*/
	}
}

void Warpable::drawCapitalBlast(wardraw_t *wd, const Vec3d &nozzlepos, double scale){
	Mat4d mat;
	transform(mat);
	double vsp = -mat.vec3(2).sp(velo) / getManeuve().maxspeed;
	if(1. < vsp)
		vsp = 1.;

	if(0. < vsp){
		const Vec3d pos0 = nozzlepos + Vec3d(0,0, scale * vsp);
		static GLuint texname = 0;
		glPushAttrib(GL_TEXTURE_BIT);
		{
			Vec3d v = rot.cnj().trans(wd->vw->pos - mat.vp3(pos0));
			v[2] /= 2. * vsp;
			v.normin() *= (4. / 5.);

			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGendv(GL_S, GL_OBJECT_PLANE, Vec4d(v));
			glEnable(GL_TEXTURE_GEN_S);
		}
		glDisable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		if(!texname){
			static const GLubyte texture[4][4] = {
				{255,0,0,0},
				{255,127,0,127},
				{255,255,0,191},
				{255,255,255,255},
			};
			GLint er = glGetError();
			glGenTextures(1, &texname);
			glBindTexture(GL_TEXTURE_1D, texname);
			glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture);
			er = glGetError();
		}
		glEnable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D, texname);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glPushMatrix();
		glMultMatrixd(mat);
		gldTranslate3dv(pos0);
		glScaled(scale, scale, scale * 2. * (0. + vsp));
		glColor4f(1,1,1, vsp);
//		gldMultQuat(rot.cnj() * wd->vw->qrot.cnj());
		gldOctSphere(2);
		glPopMatrix();
		glPopAttrib();
	}
}

void Frigate::drawShield(wardraw_t *wd){
	Frigate *p = this;
	if(!wd->vw->gc->cullFrustum(pos, .1)){
		Mat4d mat;
		transform(mat);
		glColor4ub(255,255,9,255);
		glBegin(GL_LINES);
		glVertex3dv(mat.vp3(Vec3d(.001,0,0)));
		glVertex3dv(mat.vp3(Vec3d(-.001,0,0)));
		glEnd();
		{
			int i, n = 3;
			static const avec3_t pos0[] = {
				{0, 210 * BEAMER_SCALE, 240 * BEAMER_SCALE},
				{190 * BEAMER_SCALE, -120 * BEAMER_SCALE, 240 * BEAMER_SCALE},
				{-190 * BEAMER_SCALE, -120 * BEAMER_SCALE, 240 * BEAMER_SCALE},
				{0, .002 + 50 * BEAMER_SCALE, -60 * BEAMER_SCALE},
				{0, .002 + 60 * BEAMER_SCALE, 40 * BEAMER_SCALE},
				{0, -.002 + -50 * BEAMER_SCALE, -60 * BEAMER_SCALE},
				{0, -.002 + -60 * BEAMER_SCALE, 40 * BEAMER_SCALE},
			};
			avec3_t pos;
			double rad = .002;
			GLubyte col[4] = {255, 127, 127, 255};
			random_sequence rs;
			init_rseq(&rs, (unsigned long)this);
			if(fmod(wd->vw->viewtime + drseq(&rs) * 2., 2.) < .2){
				col[1] = col[2] = 255;
				n = numof(pos0);
				rad = .003;
			}
			for(i = 0 ; i < n; i++){
				mat4vp3(pos, mat, pos0[i]);
				gldSpriteGlow(pos, rad, col, wd->vw->irot);
			}
		}

		se.draw(wd, this, BEAMER_SHIELDRAD, p->shieldAmount / MAX_SHIELD_AMOUNT);
	}
}

void Beamer::drawtra(wardraw_t *wd){
	Beamer *p = this;
	Mat4d mat;

/*	if(p->dock && p->undocktime == 0)
		return;*/

	transform(mat);

	drawCapitalBlast(wd, Vec3d(0,-0.003,.06), .01);

	drawShield(wd);

#if 1
	if(charge){
		int i;
		GLubyte azure[4] = {63,0,255,95}, bright[4] = {127,63,255,255};
		Vec3d muzzle, muzzle0(0., 0., -.100);
		double glowrad;
		muzzle = mat.vp3(muzzle0);
/*		quatrot(muzzle, pt->rot, muzzle0);
		VECADDIN(muzzle, pt->pos);*/

		if(0. < charge && charge < 4.){
			double beamrad;
			Vec3d end, end0(0., 0., -10.);
			end0[2] = -beamlen;
			end = mat.vp3(end0);
/*			quatrot(end, pt->rot, end0);
			VECADDIN(end, pt->pos);*/
			beamrad = (charge < .5 ? charge / .5 : 3.5 < charge ? (4. - charge) / .5 : 1.) * .005;
			for(i = 0; i < 5; i++){
				Vec3d p0, p1;
				p0 = muzzle * i / 5.;
				p0 += end * (5 - i) / 5.;
				p1 = muzzle * (i + 1) / 5.;
				p1 += end * (5 - i - 1) / 5.;
				glColor4ub(63,191,255, GLubyte(MIN(1., charge / 2.) * 255));
				gldBeam(wd->vw->pos, p0, p1, beamrad);
				glColor4ub(63,0,255, GLubyte(MIN(1., charge / 2.) * 95));
				gldBeam(wd->vw->pos, p0, p1, beamrad * 5.);
			}
#if 0
			for(i = 0; i < 3; i++){
				int j;
				avec3_t pos1, pos;
				struct random_sequence rs;
				struct gldBeamsData bd;
				init_rseq(&rs, (unsigned long)pt + i);
				bd.cc = 0;
				bd.solid = 0;
				glColor4ub(63,0,255, MIN(1., p->charge / 2.) * 95);
				VECCPY(pos1, pt->pos);
				pos1[0] += (drseq(&rs) - .5) * .5;
				pos1[1] += (drseq(&rs) - .5) * .5;
				pos1[2] += (drseq(&rs) - .5) * .5;
				init_rseq(&rs, *(unsigned long*)&wd->gametime + i);
				for(j = 0; j <= 64; j++){
					int c;
					avec3_t p1;
					for(c = 0; c < 3; c++)
						p1[c] = (muzzle[c] * j + end[c] * (64 - j)) / 64. + (drseq(&rs) - .5) * .05;
/*					glColor4ub(63,127,255, MIN(1., p->charge / 2.) * 191);*/
					gldBeams(&bd, wd->view, p1, beamrad * .5, COLOR32RGBA(63,127,255,MIN(1., p->charge / 2.) * 191));
				}
			}
#endif
		}

#if 0
		if(0) for(i = 0; i < 64; i++){
			int j;
			avec3_t pos1, pos;
			struct random_sequence rs;
			struct gldBeamsData bd;
			init_rseq(&rs, (unsigned long)pt + i);
			bd.cc = 0;
			bd.solid = 0;
			glColor4ub(63,0,255, MIN(1., p->charge / 2.) * 95);
			VECCPY(pos1, pt->pos);
			pos1[0] += (drseq(&rs) - .5) * .5;
			pos1[1] += (drseq(&rs) - .5) * .5;
			pos1[2] += (drseq(&rs) - .5) * .5;
			init_rseq(&rs, *(unsigned long*)&wd->gametime + i);
			for(j = 0; j < 16; j++){
				int c;
				for(c = 0; c < 3; c++)
					pos[c] = (pt->pos[c] * j + pos1[c] * (16 - j)) / 16. + (drseq(&rs) - .5) * .01;
				gldBeams(&bd, wd->view, pos, .001, COLOR32RGBA(255, j * 256 / 16, j * 64 / 16, MIN(1., p->charge / 2.) * j * 128 / 16));
			}
		}
#endif
		glowrad = charge < 4. ? charge / 4. * .02 : (6. - charge) / 2. * .02;
		gldSpriteGlow(muzzle, glowrad * 3., azure, wd->vw->irot);
		gldSpriteGlow(muzzle, glowrad, bright, wd->vw->irot);
	}
#endif
}

#if 0
static void beamer_gib_draw(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	int i = (int)pv;
	suf_t *suf;
	double scalex, scaley;
	struct random_sequence rs;
	init_rseq(&rs, (unsigned long)pl);
	scalex = (drseq(&rs) + .5) * .1 * pl->len;
	scaley = (drseq(&rs) + .5) * 1. * pl->len;
	glPushAttrib(GL_CURRENT_BIT | GL_LIGHTING_BIT | GL_ENABLE_BIT);
	glDisable(GL_CULL_FACE);
	glPushMatrix();
	gldTranslate3dv(pl->pos);
	gldMultQuat(pl->rot);
	glBegin(GL_QUADS);
	glNormal3d(0, 0, 1.);
	glVertex2d(-scalex, -scaley);
	glVertex2d( scalex, -scaley);
	glVertex2d( scalex,  scaley);
	glVertex2d(-scalex,  scaley);
	glEnd();
	glPopMatrix();
	glPopAttrib();
}
#endif

void smokedraw(const struct tent3d_line_callback *p, const struct tent3d_line_drawdata *dd, void *private_data){
	glPushMatrix();
	gldTranslate3dv(p->pos);
//	glMultMatrixd(dd->invrot);
	{
		Quatd rot;
		rot = dd->rot;
		Vec3d delta = dd->viewpoint - p->pos;
		Vec3d ldelta = rot.trans(delta);
		Quatd qirot = Quatd::direction(ldelta);
		Quatd qret = rot.cnj() * qirot;
		gldMultQuat(qret);
	}
	gldScaled(p->len);
	struct random_sequence rs;
	init_rseq(&rs, (long)p);
	glRotated(rseq(&rs) % 360, 0, 0, 1);
//	gldMultQuat(Quatd::direction(Vec3d(p->pos) - Vec3d(dd->viewpoint)));
	static GLuint list = 0;
	glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT);
	if(!list){
		suftexparam_t stp;
		stp.flags = STP_ENV | STP_ALPHA | STP_ALPHATEX | STP_MAGFIL | STP_MINFIL;
		stp.env = GL_MODULATE;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		list = CallCacheBitmap5("smoke2.jpg", "smoke2.jpg", &stp, NULL, NULL);
	}
	glCallList(list);
	COLOR32 col = (COLOR32)private_data;
	glColor4f(COLOR32R(col) / 255., COLOR32G(col) / 255., COLOR32B(col) / 255., MIN(p->life * .25, 1.));
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);
	float alpha = MIN(p->life * .25, 1.);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, Vec4<float>(.5f, .5f, .5f, alpha));
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, Vec4<float>(.5f, .5f, .5f, alpha));
/*	glBegin(GL_QUADS);
	glTexCoord2f(0,0); glVertex2f(-1, -1);
	glTexCoord2f(1,0); glVertex2f(+1, -1);
	glTexCoord2f(1,1); glVertex2f(+1, +1);
	glTexCoord2f(0,1); glVertex2f(-1, +1);
	glEnd();*/
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f( .5,  .5); glNormal3f( 0,  0, 1); glVertex2f( 0,  0);
	glTexCoord2f( .0,  .0); glNormal3f(-1, -1, 0); glVertex2f(-1, -1);
	glTexCoord2f( 1.,  .0); glNormal3f( 1, -1, 0); glVertex2f( 1, -1);
	glTexCoord2f( 1.,  1.); glNormal3f( 1,  1, 0); glVertex2f( 1,  1);
	glTexCoord2f( 0.,  1.); glNormal3f(-1,  1, 0); glVertex2f(-1,  1);
	glTexCoord2f( 0.,  0.); glNormal3f(-1, -1, 0); glVertex2f(-1, -1);
	glEnd();
	glPopAttrib();
	glPopMatrix();
}


void debrigib(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	if(dd->pgc && (dd->pgc->cullFrustum(pl->pos, .01) || (dd->pgc->scale(pl->pos) * .01) < 5))
		return;

	static suf_t *sufs[5] = {NULL};
	static VBO *vbo[5];
	static GLuint lists[numof(sufs)] = {0};
	if(!sufs[0]){
		char buf[64];
		for(int i = 0; i < numof(sufs); i++){
			sprintf(buf, "models/debris%d.bin", i);
			sufs[i] = CallLoadSUF(buf);
			vbo[i] = CacheVBO(sufs[i]);
		}
	}
	glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
//	glBindTexture(GL_TEXTURE_2D, texname);
	glPushMatrix();
	gldTranslate3dv(pl->pos);
	gldMultQuat(pl->rot);
	gldScaled(.0001);
	struct random_sequence rs;
	initfull_rseq(&rs, 13230354, (unsigned long)pl);
	unsigned id = rseq(&rs) % numof(sufs);
//	if(!lists[id]){
//		glNewList(lists[id] = glGenLists(1), GL_COMPILE_AND_EXECUTE);
//		DrawSUF(sufs[id], SUF_ATR, NULL);
		DrawVBO(vbo[id], SUF_ATR, NULL);
//		glEndList();
//	}
//	else
//		glCallList(lists[id]);
	glPopMatrix();
	glPopAttrib();
}


int Frigate::takedamage(double damage, int hitpart){
	Frigate *p = this;
	struct tent3d_line_list *tell = w->getTeline3d();
	int ret = 1;

	if(hitpart == 1000){
		se.takedamage(damage / maxshield());
		if(damage < p->shieldAmount)
			p->shieldAmount -= damage;
		else
			p->shieldAmount = -50.;
		return 1;
	}
	else if(p->shieldAmount < 0.)
		p->shieldAmount = -50.;

/*	if(tell){
		int j, n;
		frexp(damage, &n);
		for(j = 0; j < n; j++){
			double velo[3];
			velo[0] = .15 * (drseq(&w->rs) - .5);
			velo[1] = .15 * (drseq(&w->rs) - .5);
			velo[2] = .15 * (drseq(&w->rs) - .5);
			AddTeline3D(tell, pt->pos, velo, .001, NULL, NULL, w->gravity,
				j % 2 ? COLOR32RGBA(255,255,255,255) : COLOR32RGBA(255,191,63,255),
				TEL3_HEADFORWARD | TEL3_REFLECT | TEL3_FADEEND, .5 + drseq(&w->rs) * .5);
		}
	}*/
//	playWave3D("hit.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
	if(0 < p->health && p->health - damage <= 0){
		int i;
		ret = 0;
/*		effectDeath(w, pt);*/
		WarSpace *ws = *w;
		if(ws){
#if 0
			for(i = 0; i < 32; i++){
				Vec3d pos, velo;
				velo[0] = drseq(&w->rs) - .5;
				velo[1] = drseq(&w->rs) - .5;
				velo[2] = drseq(&w->rs) - .5;
				velo.normin();
				pos = p->pos;
				velo *= .1;
				pos += velo * .1;
				AddTeline3D(ws->tell, pos, velo, .005, NULL, NULL, NULL, COLOR32RGBA(255, 31, 0, 255), TEL3_HEADFORWARD | TEL3_THICK | TEL3_FADEEND, 15. + drseq(&w->rs) * 5.);
			}
#endif

			if(ws->gibs) for(i = 0; i < 32; i++){
				double pos[3], velo[3], omg[3];
				/* gaussian spread is desired */
				velo[0] = .025 * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				velo[1] = .025 * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				velo[2] = .025 * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				omg[0] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				omg[1] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				omg[2] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				VECCPY(pos, this->pos);
				VECSADD(pos, velo, .1);
				AddTelineCallback3D(ws->gibs, pos, velo, .010, quat_u, omg, vec3_000, debrigib, NULL, TEL3_QUAT | TEL3_NOLINE, 15. + drseq(&w->rs) * 5.);
			}

			/* smokes */
			for(i = 0; i < 32; i++){
				double pos[3], velo[3];
				COLOR32 col = 0;
				VECCPY(pos, p->pos);
				pos[0] += .1 * (drseq(&w->rs) - .5);
				pos[1] += .1 * (drseq(&w->rs) - .5);
				pos[2] += .1 * (drseq(&w->rs) - .5);
				col |= COLOR32RGBA(rseq(&w->rs) % 32 + 127,0,0,0);
				col |= COLOR32RGBA(0,rseq(&w->rs) % 32 + 127,0,0);
				col |= COLOR32RGBA(0,0,rseq(&w->rs) % 32 + 127,0);
				col |= COLOR32RGBA(0,0,0,191);
	//			AddTeline3D(w->tell, pos, NULL, .035, NULL, NULL, NULL, col, TEL3_NOLINE | TEL3_GLOW | TEL3_INVROTATE, 60.);
				AddTelineCallback3D(ws->tell, pos, vec3_000, .03, quat_u, vec3_000, vec3_000, smokedraw, (void*)col, TEL3_INVROTATE | TEL3_NOLINE, 60.);
			}

			{/* explode shockwave thingie */
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
				p = sqrt(1. - q[3] * q[3]) / VECLEN(v);
				q = v * p;

				AddTeline3D(tell, this->pos, vec3_000, 5., q, vec3_000, vec3_000, COLOR32RGBA(255,191,63,255), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_QUAT, 1.);
				AddTeline3D(tell, this->pos, vec3_000, 2., quat_u, vec3_000, vec3_000, COLOR32RGBA(255,255,255,127), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_INVROTATE, 1.);
			}
		}
//		playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
//		p->w = NULL;
	}
	p->health -= damage;
	return ret;
}

/*static void beamer_gib_draw(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	int i = (int)pv;
	suf_t *suf;
	double scale;
	if(i < beamer_s.sufbase->np){
		suf = beamer_s.sufbase;
		scale = .0001;
		gib_draw(pl, suf, scale, i);
	}
}*/

void Frigate::bullethit(const Bullet *pb){
	Frigate *p = this;
	if(pb->damage < p->shieldAmount){
		se.bullethit(this, pb, maxshield());
	}
}

int Frigate::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	Frigate *p = this;
	double sc[3];
	double best = dt, retf;
	int reti = 0, i, n;
	if(0 < p->shieldAmount){
		Vec3d hitpos;
		if(jHitSpherePos(pos, BEAMER_SHIELDRAD + rad, src, dir, dt, ret, &hitpos)){
			if(retp) *retp = hitpos;
			if(retn) *retn = (hitpos - pos).norm();
			return 1000; /* something quite unlikely to reach */
		}
	}
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

Entity::Props Frigate::props()const{
	std::vector<cpplib::dstring> ret = st::props();
	ret.push_back(cpplib::dstring("Shield: ") << shieldAmount << '/' << maxshield());
	return ret;
}

Entity::Props Beamer::props()const{
	std::vector<cpplib::dstring> ret = st::props();
	ret.push_back(cpplib::dstring("Cooldown: ") << cooldown);
	return ret;
}

Entity *Beamer::create(WarField *w, Builder *mother){
	Beamer *ret = new Beamer(NULL);
	ret->pos = mother->pos;
	ret->velo = mother->velo;
	ret->rot = mother->rot;
	ret->omg = mother->omg;
	ret->race = mother->race;
//	w->addent(ret);
	return ret;
}

const Builder::BuildStatic Beamer::builds = {
	"Lancer class",
	Beamer::create,
	100.,
	600.,
};



double Beamer::maxhealth()const{return BEAMER_HEALTH;}
double Frigate::maxenergy()const{return beamer_mn.capacity;}
double Frigate::maxshield()const{return MAX_SHIELD_AMOUNT;}
