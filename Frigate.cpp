#include "Frigate.h"
#include "Player.h"
#include "Bullet.h"
#include "CoordSys.h"
#include "Viewer.h"
#include "cmd.h"
//#include "glwindow.h"
#include "judge.h"
#include "astrodef.h"
#include "serial_util.h"
#include "draw/material.h"
#include "Sceptor.h"
#include "EntityCommand.h"
#include "draw/effects.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
//#include "sensor.h"
#include "glw/popup.h"
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
#include <float.h>



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
#define BEAMER_ACCELERATE .025
#define BEAMER_MAX_ANGLESPEED .4
#define BEAMER_SHIELDRAD .09


const struct Warpable::maneuve Frigate::frigate_mn = {
	BEAMER_ACCELERATE, /* double accel; */
	BEAMER_MAX_SPEED, /* double maxspeed; */
	100., /* double angleaccel; */
	BEAMER_MAX_ANGLESPEED, /* double maxanglespeed; */
	50000., /* double capacity; [MJ] */
	100., /* double capacitor_gen; [MW] */
};


Frigate::Frigate(WarField *aw) : st(aw), shieldGenSpeed(0), paradec(-1){
	health = maxhealth();
	shieldAmount = maxshield();
}


void Frigate::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << shieldAmount;
	sc.o << shieldGenSpeed;
	sc.o << mother;
	sc.o << paradec;
}

void Frigate::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> shieldAmount;
	sc.i >> shieldGenSpeed;
	sc.i >> mother;
	sc.i >> paradec;
}


void Frigate::cockpitView(Vec3d &pos, Quatd &rot, int)const{
	pos = this->pos + this->rot.trans(Vec3d(.0, .05, .15));
	rot = this->rot;
}
void Frigate::anim(double dt){
	st::anim(dt);

	// If docked
	if(Docker *docker = *w){
		health = min(health + dt * 300., maxhealth()); // it takes several seconds to be fully repaired
		if(health == maxhealth() && !docker->remainDocked)
			docker->postUndock(this);
		return;
	}

	/* shield regeneration */
	if(0 < health){
		const double shieldPerEnergy = .5;
		shieldGenSpeed = MIN(shieldGenSpeed + 5. * dt, 300.);
		double gen = shieldGenSpeed * dt;
		if(capacitor < gen / shieldPerEnergy)
			gen = capacitor * shieldPerEnergy;
		if(MAX_SHIELD_AMOUNT < shieldAmount + gen){
			shieldAmount = maxshield();
			capacitor -= (maxshield() - shieldAmount) / shieldPerEnergy;
		}
		else{
			shieldAmount += gen * shieldPerEnergy;
			capacitor -= gen / shieldPerEnergy;
		}
	}
	else
		w = NULL;

	se.anim(dt);

}
double Frigate::hitradius()const{return .1;}
Entity::Dockable *Frigate::toDockable(){return this;}
const Warpable::maneuve &Frigate::getManeuve()const{return frigate_mn;}


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
const int Frigate::nhitboxes = numof(Frigate::hitboxes);


void Warpable::drawCapitalBlast(wardraw_t *wd, const Vec3d &nozzlepos, double scale){
	Mat4d mat;
	transform(mat);
	double vsp = -mat.vec3(2).sp(velo) / getManeuve().maxspeed;
	if(1. < vsp)
		vsp = 1.;

	if(DBL_EPSILON < vsp){
		const Vec3d pos0 = nozzlepos + Vec3d(0,0, scale * vsp);
		class Tex1d{
		public:
			GLuint tex[2];
			Tex1d(){
				glGenTextures(2, tex);
			}
			~Tex1d(){
				glDeleteTextures(2, tex);
			}
		};
		static OpenGLState::weak_ptr<Tex1d> texname;
		glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
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
			texname.create(*openGLState);
			static const GLubyte texture[4][4] = {
				{255,0,0,0},
				{255,127,0,127},
				{255,255,0,191},
				{255,255,255,255},
			};
			glBindTexture(GL_TEXTURE_1D, texname->tex[0]);
			glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			static const GLubyte texture2[4][4] = {
				{255,127,64,63},
				{255,191,127,127},
				{255,255,255,191},
				{191,255,255,255},
			};
			glBindTexture(GL_TEXTURE_1D, texname->tex[1]);
			glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture2);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		if(!texname)
			return;
		glEnable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D, texname->tex[0]);
		if(glActiveTextureARB){
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGendv(GL_S, GL_OBJECT_PLANE, Vec4d(0,0,-.5,.5));
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_1D);
			glBindTexture(GL_TEXTURE_1D, texname->tex[1]);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
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




int Frigate::takedamage(double damage, int hitpart){
	if(!w)
		return 1;
	Frigate *p = this;
	struct tent3d_line_list *tell = w->getTeline3d();
	int ret = 1;

	if(hitpart == 1000){
		se.takedamage(damage / maxshield());
		if(damage < p->shieldAmount)
			p->shieldAmount -= damage;
		else
			p->shieldAmount = -50.;
		shieldGenSpeed = 0.;
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
				double pos[3], velo[3] = {0}, omg[3];
				/* gaussian spread is desired */
				for(int j = 0; j < 6; j++)
					velo[j / 2] += .0125 * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				omg[0] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				omg[1] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				omg[2] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				VECCPY(pos, this->pos);
				for(int j = 0; j < 3; j++)
					pos[j] += hitradius() * (drseq(&w->rs) - .5);
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

			/* explode shockwave thingie */
			AddTeline3D(tell, this->pos, vec3_000, 3., quat_u, vec3_000, vec3_000, COLOR32RGBA(255,255,255,127), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_INVROTATE, 2.);
		}
//		playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
//		p->w = NULL;
	}
	p->health -= damage;
	return ret;
}

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
	Props ret = st::props();
//	ret.push_back(cpplib::dstring("Shield: ") << shieldAmount << '/' << maxshield());
	return ret;
}

int Frigate::popupMenu(PopupMenu &list){
	int ret = st::popupMenu(list);
	list.append("Dock", 0, "dock").append("Military Parade Formation", 0, "parade_formation").append("Cloak", 0, "cloak");
	return ret;
}

bool Frigate::solid(const Entity *o)const{
	return !(task == sship_undock || task == sship_warp);
}

bool Frigate::command(EntityCommand *com){
	if(InterpretCommand<ParadeCommand>(com)){
		findMother();
		task = sship_parade;
		enemy = NULL; // Temporarily forget about enemy
		return true;
	}
	else if(AttackCommand *ac = InterpretDerivedCommand<AttackCommand>(com)){
		if(!ac->ents.empty()){
			enemy = *ac->ents.begin();
			task = sship_attack;
		}
		return true;
	}
	else return st::command(com);
}

Entity *Frigate::findMother(){
	Entity *pm = NULL;
	double best = 1e10 * 1e10, sl;
	for(WarField::EntityList::iterator it = w->entlist().begin(); it != w->entlist().end(); it++) if(*it){
		Entity *e = *it;
		if(e->getDocker() && (sl = (e->pos - this->pos).slen()) < best){
			mother = e->getDocker();
			pm = mother->e;
			best = sl;
		}
	}
	return pm;
}


double Frigate::maxenergy()const{return frigate_mn.capacity;}
double Frigate::maxshield()const{return MAX_SHIELD_AMOUNT;}
