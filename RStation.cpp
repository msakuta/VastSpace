#include "RStation.h"
#include "Warpable.h"
#include "material.h"
#include "Player.h"
#include "Bullet.h"
//#include "entity_p.h"
#include "astrodraw.h"
#include "Viewer.h"
#include "CoordSys.h"
#include "cmd.h"
//#include "glwindow.h"
#include "judge.h"
//#include "sensor.h"
#include "serial_util.h"
#include "draw/WarDraw.h"
#include "glw/popup.h"
extern "C"{
#include <clib/suf/sufdraw.h>
#include <clib/suf/sufbin.h>
#include <clib/gl/gldraw.h>
#include <clib/c.h>
#include <clib/mathdef.h>
#include <clib/avec3.h>
#include <clib/amat4.h>
#include <clib/aquat.h>
#include <clib/timemeas.h>
}

#ifdef NDEBUG
#define hitbox_draw
#endif


#define RSTATION_SCALE .1
#define RSTATION_MAX_RU 10000.

double g_rstation_occupy_time = 10.;

#define SQRT2P2 (M_SQRT2/2.)

static const double rstation_sc[3] = {.05, .055, .075};
/*static const double beamer_sc[3] = {.05, .05, .05};*/
static struct hitbox rstation_hb[] = {
	hitbox(Vec3d(0., 0., -22. * RSTATION_SCALE), Quatd(0,0,0,1), Vec3d(9. * RSTATION_SCALE, .29 * RSTATION_SCALE, 10. * RSTATION_SCALE)),
	hitbox(Vec3d(-22. * RSTATION_SCALE, 0., 0.), Quatd(0,SQRT2P2,0,SQRT2P2), Vec3d(9. * RSTATION_SCALE, .29 * RSTATION_SCALE, 10. * RSTATION_SCALE)),
	hitbox(Vec3d(0., 0.,  22. * RSTATION_SCALE), Quatd(1,0,0,0), Vec3d(9. * RSTATION_SCALE, .29 * RSTATION_SCALE, 10. * RSTATION_SCALE)),
	hitbox(Vec3d(22. * RSTATION_SCALE, 0., 0.), Quatd(0,-SQRT2P2,0,SQRT2P2), Vec3d(9. * RSTATION_SCALE, .29 * RSTATION_SCALE, 10. * RSTATION_SCALE)),
	hitbox(Vec3d(0., 0., 0), Quatd(0,0,0,1), Vec3d(11. * RSTATION_SCALE, .29 * RSTATION_SCALE, 11. * RSTATION_SCALE)),
	hitbox(Vec3d(0., -5. * RSTATION_SCALE, 0), Quatd(0,0,0,1), Vec3d(5. * RSTATION_SCALE, 5. * RSTATION_SCALE, 5. * RSTATION_SCALE)),
	hitbox(Vec3d(0., 10. * RSTATION_SCALE, 0), Quatd(0,0,0,1), Vec3d(.5 * RSTATION_SCALE, 10. * RSTATION_SCALE, .5 * RSTATION_SCALE)),
};

RStation::RStation(WarField *aw) : st(aw){
	mass = 1e7;
	race = -1;
	health = 1500000.;
	ru = 1000.;
	occupytime = g_rstation_occupy_time;
	occupyrace = -1;
}

const char *RStation::idname()const{return "rstation";}
const char *RStation::classname()const{return "RStation";}
const unsigned RStation::classid = registerClass("RStation", Conster<RStation>);
const unsigned RStation::entid = registerEntity("RStation", new Constructor<RStation>);
const char *RStation::dispname()const{return "Resource St.";}
bool RStation::isTargettable()const{return true;}
bool RStation::isSelectable()const{return true;}
double RStation::maxhealth()const{return 1500000.;}

void RStation::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << ru << occupytime << occupyrace;
}

void RStation::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> ru >> occupytime >> occupyrace;
}

/*double *__fastcall rstation_ru(rstation_t *p){
	return &p->ru;
}*/

double RStation::hitradius()const{
	return 4.;
}

void RStation::cockpitview(Vec3d &pos, Quatd &rot, int seatid)const{
	Player *ppl = w->pl;
	double g_viewdist = 1.;
	Vec3d ofs, src[3] = {Vec3d(0., .001, -.002), Vec3d(0., 0., 10.)};
	Mat4d mat;
	Quatd q;
	seatid = (seatid + 3) % 3;
	if(seatid == 1){
		double f;
		q = this->rot * w->pl->getrot();
		src[2][2] = .1 * g_viewdist;
		f = src[2][2] * .001 < .001 ? src[2][2] * .001 : .001;
		src[2][2] += f * sin(ppl->gametime * M_PI / 2. + M_PI / 2.);
		src[2][0] = f * sin(ppl->gametime * M_PI / 2.);
		src[2][1] = f * cos(ppl->gametime * M_PI / 2.);
	}
	else
		q = this->rot;
	ofs = q.trans(src[seatid]);
	pos = this->pos + ofs;
}

/*static int rstation_getrot(struct entity *pt, warf_t *w, double (*rot)[16]){
	quat2imat(*rot, pt->rot);
	return w->pl->control != pt;
}*/

int RStation::popupMenu(PopupMenu &pm){
	pm.appendSeparator();
	return 0;
}

void RStation::anim(double dt){
	RStation *p = this;
	Entity *pt2;
	int orace = -1, mix = 0;

	if(p->ru + dt * 2. < RSTATION_MAX_RU)
		p->ru += dt * 2.;
	else
		p->ru = RSTATION_MAX_RU;

	/* Rstations can only be occupied by staying space vehicles in distance of
	  5 kilometers with no other races. */
	for(pt2 = w->el; pt2; pt2 = pt2->next)
		if(this != pt2 && w == pt2->w && 0 <= pt2->race &&
			(this->pos - pt2->pos).slen() < 8. * 8.)
	{
		if(orace < 0)
			orace = pt2->race;
		else if(orace != pt2->race){
			break;
		}
	}
	if(!pt2 && (0 <= orace) && orace != this->race){
		p->occupyrace = this->race < 0 ? orace : -1;
		if(p->occupytime < dt){
			this->race = p->occupyrace;
			p->occupytime = 0.;
		}
		else
			p->occupytime -= dt;
	}
	else{
		if(p->occupytime + dt < g_rstation_occupy_time)
			p->occupytime += dt;
		else
			p->occupytime = g_rstation_occupy_time;
	}
}

void RStation::draw(wardraw_t *wd){
	static int init = 0;
	static suf_t *sufbase = NULL;
	static suftex_t *suft;
	if(!w)
		return;

	/* cull object */
/*	if(scepter_cull(pt, wd))
		return;*/
	wd->lightdraws++;

	draw_healthbar(this, wd, this->health / maxhealth(), 3., this->occupytime / 10., this->ru / RSTATION_MAX_RU);

	if(init == 0) do{
		sufbase = CallLoadSUF("models/rstation.bin");
		if(!sufbase) break;
		CacheSUFMaterials(sufbase);
		suft = AllocSUFTex(sufbase);
		init = 1;
	} while(0);
	if(!sufbase){
		double pos[3];
		GLubyte col[4] = {255,255,0,255};
		gldPseudoSphere(pos, 33. * RSTATION_SCALE, col);
	}
	else{
		static const double normal[3] = {0., 1., 0.};
		const double scale = RSTATION_SCALE;

		glPushMatrix();
		gldTranslate3dv(this->pos);
		gldScaled(scale);
		gldMultQuat(this->rot);
		glScalef(-1, 1, -1);
		DecalDrawSUF(sufbase, SUF_ATR, NULL, suft, NULL, NULL);
		glPopMatrix();
	}
}

void RStation::drawtra(wardraw_t *wd){
	if(!wd->vw->gc->cullFrustum(this->pos, 35.)){
		int i, n = 3;
		static const avec3_t pos0[] = {
			{0, 20.5 * RSTATION_SCALE, 0},
			{4.71 * RSTATION_SCALE, 0, -33.7 * RSTATION_SCALE},
			{-4.71 * RSTATION_SCALE, 0, -33.7 * RSTATION_SCALE},
			{4.71 * RSTATION_SCALE, 0, 33.7 * RSTATION_SCALE},
			{-4.71 * RSTATION_SCALE, 0, 33.7 * RSTATION_SCALE},
			{33.7 * RSTATION_SCALE, 0, -4.71 * RSTATION_SCALE},
			{-33.7 * RSTATION_SCALE, 0, -4.71 * RSTATION_SCALE},
			{33.7 * RSTATION_SCALE, 0, 4.71 * RSTATION_SCALE},
			{-33.7 * RSTATION_SCALE, 0, 4.71 * RSTATION_SCALE},
		};
		avec3_t pos;
		double rad = .05;
		Mat4d mat;
		double t;
		GLubyte col[4] = {255, 31, 31, 255};
		transform(mat);

		/* color calculation of static navlights */
		t = fmod(w->pl->gametime * .3, 2.);
		if(t < 1.){
			rad *= (t + 1.) / 2.;
			col[3] = GLubyte(col[3] * t);
		}
		else{
			rad *= (2. - t + 1.) / 2.;
			col[3] *= GLubyte(2. - t);
		}
		for(i = 0 ; i < numof(pos0); i++){
			mat4vp3(pos, mat, pos0[i]);
			gldSpriteGlow(pos, rad, col, wd->vw->irot);
		}
		for(i = 0; i < numof(rstation_hb); i++){
			glPushMatrix();
			gldTranslate3dv(this->pos);
			gldTranslate3dv(rstation_hb[i].org);
			Mat4d rot = rstation_hb[i].rot.tomat4();
			glMultMatrixd(rot);
			hitbox_draw(this, rstation_hb[i].sc);
			glPopMatrix();
		}
	}
}

int RStation::takedamage(double damage, int hitpart){
	struct tent3d_line_list *tell = w->getTeline3d();
	int ret = 1;
	if(this->health < 0.)
		return 1;
//	playWave3D("hit.wav", this->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
	if(0 < health && health - damage <= 0){
		int i;
		ret = 0;
		Vec3d accel = w->accel(pos, velo);
/*		effectDeath(w, pt);*/
		if(tell) for(i = 0; i < 32; i++){
			Vec3d pos, velo;
			velo[0] = drseq(&w->rs) - .5;
			velo[1] = drseq(&w->rs) - .5;
			velo[2] = drseq(&w->rs) - .5;
			velo.normin();
			pos = this->pos + velo * .1 * .1;
			AddTeline3D(tell, pos, velo, .005, quat_u, vec3_000, accel, COLOR32RGBA(255, 31, 0, 255), TEL3_HEADFORWARD | TEL3_THICK | TEL3_FADEEND | TEL3_REFLECT, 1.5 + drseq(&w->rs));
		}
		health = -2.;
	}
	else
		health -= damage;
	return ret;
}

int RStation::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	double sc[3];
	double best = dt, retf;
	int reti = 0, i, n;
	for(n = 0; n < numof(rstation_hb); n++){
		Vec3d org;
		Quatd rot;
		org = this->rot.itrans(rstation_hb[n].org);
		org += this->pos;
		rot = this->rot * rstation_hb[n].rot;
		for(i = 0; i < 3; i++)
			sc[i] = rstation_hb[n].sc[i] + rad;
		Vec3d n;
		if((jHitBox(org, sc, rot, src, dir, 0., best, &retf, retp, &n)) && (retf < best)){
			best = retf;
			if(ret) *ret = retf;
			reti = i + 1;
			if(retn) *retn = n;
		}
	}
	return reti;
}

Entity::Props RStation::props()const{
	Props ret = st::props();
//	ret.push_back(cpplib::dstring("RUs: ") << ru << "/" << RSTATION_MAX_RU);
	return ret;
}
