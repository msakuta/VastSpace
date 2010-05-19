#include "Frigate.h"
#include "player.h"
#include "bullet.h"
#include "coordsys.h"
#include "viewer.h"
#include "cmd.h"
#include "glwindow.h"
#include "judge.h"
#include "astrodef.h"
#include "serial_util.h"
#include "material.h"
#include "Sceptor.h"
#include "EntityCommand.h"
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


const struct Warpable::maneuve Frigate::frigate_mn = {
	BEAMER_ACCELERATE, /* double accel; */
	BEAMER_MAX_SPEED, /* double maxspeed; */
	BEAMER_ANGLEACCEL, /* double angleaccel; */
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
	AttackCommand *ac;
	if((ac = InterpretCommand<AttackCommand>(com)) || (ac = InterpretCommand<ForceAttackCommand>(com))){
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
	for(Entity *e = w->entlist(); e; e = e->next) if(e->getDocker() && (sl = (e->pos - this->pos).slen()) < best){
		mother = e->getDocker();
		pm = mother->e;
		best = sl;
	}
	return pm;
}


double Frigate::maxenergy()const{return frigate_mn.capacity;}
double Frigate::maxshield()const{return MAX_SHIELD_AMOUNT;}
