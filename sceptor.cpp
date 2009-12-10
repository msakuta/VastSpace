#include "sceptor.h"
#include "player.h"
#include "bullet.h"
#include "coordsys.h"
#include "viewer.h"
#include "cmd.h"
#include "glwindow.h"
#include "arms.h"
#include "material.h"
//#include "warutil.h"
#include "judge.h"
#include "astrodef.h"
#include "stellar_file.h"
#include "antiglut.h"
//#include "worker.h"
//#include "glsl.h"
//#include "astro_star.h"
#include "glextcall.h"
//#include "sensor.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/suf/sufbin.h>
#include <clib/suf/sufdraw.h>
#include <clib/avec3.h>
#include <clib/amat4.h>
#include <clib/aquat.h>
#include <clib/GL/gldraw.h>
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

#define VIEWDIST_FACTOR 1.2
#define MTURRET_SCALE (.00005)
#define MTURRET_MAX_GIBS 30
#define MTURRET_BULLETSPEED 2.
#define MTURRET_VARIANCE (.001 * M_PI)
#define MTURRET_INTOLERANCE (M_PI / 20.)
#define MTURRETROTSPEED (.4*M_PI)
#define RETHINKTIME .5
#define SCEPTER_SCALE 1./10000
#define SCEPTER_SMOKE_FREQ 10.
#define SCEPTER_RELOADTIME .3
#define SCEPTER_ROLLSPEED (.2 * M_PI)
#define SCEPTER_ROTSPEED (.3 * M_PI)
#define SCEPTER_MAX_ANGLESPEED (M_PI * .5)
#define SCEPTER_ANGLEACCEL (M_PI * .2)
#define SCEPTER_MAX_GIBS 20
#define BULLETSPEED 2.
#define SCARRY_MAX_HEALTH 200000
#define SCARRY_MAX_SPEED .03
#define SCARRY_ACCELERATE .01
#define SCARRY_MAX_ANGLESPEED (.005 * M_PI)
#define SCARRY_ANGLEACCEL (.002 * M_PI)
#define BUILDQUESIZE SCARRY_BUILDQUESIZE
#define numof(a) (sizeof(a)/sizeof*(a))
#define signof(a) ((a)<0?-1:0<(a)?1:0)


#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef ABS
#define ABS(a) ((a)<0?-(a):(a))
#endif


/* color sequences */
#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_orangeburn[] = {
	{0.1, COLOR32RGBA(255,255,191,255)},
	{0.15, COLOR32RGBA(255,255,31,191)},
	{0.45, COLOR32RGBA(255,127,31,95)},
	{2.3, COLOR32RGBA(255,31,0,63)},
};
static const struct color_sequence cs_orangeburn = DEFINE_COLSEQ(cnl_orangeburn, (COLOR32)-1, 3.);
static const struct color_node cnl_shortburn[] = {
	{0.1, COLOR32RGBA(255,255,191,255)},
	{0.15, COLOR32RGBA(255,255,31,191)},
	{0.25, COLOR32RGBA(255,127,31,0)},
};
static const struct color_sequence cs_shortburn = DEFINE_COLSEQ(cnl_shortburn, (COLOR32)-1, 0.5);


static double g_nlips_factor = 0.;
static int g_shader_enable = 0;


const char *Sceptor::idname()const{
	return "sceptor";
}

const char *Sceptor::classname()const{
	return "Interceptor";
}

double Sceptor::maxhealth()const{
	return 1000.;
}


enum Sceptor::Task{
	scepter_idle = sship_idle,
	scepter_undock = sship_undock,
	scepter_undockque = sship_undockque,
	scepter_dock = sship_dock,
	scepter_dockque = sship_dockque,
	scepter_moveto = sship_moveto,
	scepter_parade = sship_parade,
	scepter_attack = sship_attack,
	scepter_away = sship_away,
	num_scepter_task
};






Sceptor::Sceptor(WarField *aw) : st(aw), mother(NULL), task(scepter_idle), fuel(maxfuel()){
	Sceptor *const p = this;
//	EntityInit(ret, w, &scepter_s);
//	VECCPY(ret->pos, mother->st.st.pos);
	mass = 4e3;
//	race = mother->st.st.race;
	health = maxhealth();
	p->aac.clear();
	memset(p->thrusts, 0, sizeof p->thrusts);
	p->throttle = .5;
	p->cooldown = 1.;
	p->pf = AddTefpolMovable3D(w->tepl, this->pos, this->velo, avec3_000, &cs_shortburn, TEP3_THICK | TEP3_ROUGH, cs_shortburn.t);
//	p->mother = mother;
	p->hitsound = -1;
	p->docked = false;
//	p->paradec = mother->paradec++;
	p->task = scepter_idle;
	p->fcloak = 0.;
	p->cloak = 0;
	p->heat = 0.;
/*	if(mother){
		scarry_dock(mother, ret, w);
		if(!mother->remainDocked)
			scarry_undock(mother, ret, w);
	}*/
}

const avec3_t scepter_guns[2] = {{35. * SCEPTER_SCALE, -4. * SCEPTER_SCALE, -15. * SCEPTER_SCALE}, {-35. * SCEPTER_SCALE, -4. * SCEPTER_SCALE, -15. * SCEPTER_SCALE}};

void Sceptor::cockpitView(Vec3d &pos, Quatd &q, int seatid)const{
	Player *ppl = w->pl;
	Vec3d ofs, src[3] = {Vec3d(0., .001, -.002), Vec3d(0., 0., .1), Vec3d(0., .008, 0.020)};
	Mat4d mat;
//	*chasecamera = (*chasecamera + 3) % 3;
	if(seatid == 1){
		double f;
		q = this->rot * w->pl->rot;
		src[2][2] = .1 /** g_viewdist*/;
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

/*static void scepter_control(entity_t *pt, warf_t *w, input_t *inputs, double dt){
	scepter_t *p = (scepter_t*)pt;
	if(!pt->active || pt->health <= 0.)
		return;
	pt->inputs = *inputs;
}*/

#if 0
static int scepter_getrot(struct entity *pt, warf_t *w, double (*rot)[16]){
/*	quat2imat(*rot, pt->rot);
	return w->pl->control != pt;*/
	aquat_t rotq;
	int ret;
	ret = sentity_getrotq(pt, w, &rotq);
	quat2mat(*rot, rotq);
	return ret;
}
#endif

int Sceptor::popupMenu(char ***const titles, int **keys, char ***cmds, int *pnum){
	Entity *pt = this;
	static const char *titles1[] = {"Dock", "Military Parade Formation", "cloak"};
	static const char *cmds1[] = {"dock", "parade_formation", "cloak"};
	int i, n;
	int num;
	int add = numof(titles1);
	int acty = (1 << numof(titles1)) - 1;
	for(n = 0; n < numof(titles1); n++) for(i = 0; i < *pnum; i++) if(!strcmp((*titles)[i], titles1[n]) && !strcmp((*cmds)[i], cmds1[n])){
		acty &= ~(1 << n);
		add--;
		break;
	}
	if(!acty)
		return 0;
	i = *pnum;
	num = *pnum += add + 1;
	*titles = (char**)realloc(*titles, num * sizeof **titles);
	*keys = (int*)realloc(*keys, num * sizeof **keys);
	*cmds = (char**)realloc(*cmds, num * sizeof **cmds);
	const_cast<const char**>(*titles)[i] = glwMenuSeparator;
	(*keys)[i] = '\0';
	(*cmds)[i] = NULL;
	for(n = 0; n < numof(titles1); n++) if(acty & (1 << n)){
		i++;
		const_cast<const char**>(*titles)[i] = titles1[n];
		(*keys)[i] = '\0';
		const_cast<const char**>(*cmds)[i] = cmds1[n];
	}
/*	int i = *pnum;
	int num = *pnum += 3;
	*titles = realloc(*titles, num * sizeof **titles);
	*keys = realloc(*keys, num * sizeof **keys);
	*cmds = realloc(*cmds, num * sizeof **cmds);
	(*titles)[i] = glwMenuSeparator;
	(*keys)[i] = '\0';
	(*cmds)[i] = NULL;
	i++;
	(*titles)[i] = "Dock";
	(*keys)[i] = '\0';
	(*cmds)[i] = "dock";
	i++;
	(*titles)[i] = "Military Parade Formation";
	(*keys)[i] = '\0';
	(*cmds)[i] = "parade_formation";*/
	return 0;
}

int Sceptor::popupMenu(PopupMenuItem **list){
	int ret = st::popupMenu(list);
	return ret;
}

std::vector<cpplib::dstring> Sceptor::props()const{
	std::vector<cpplib::dstring> ret = st::props();
	ret.push_back(cpplib::dstring("Task: ") << task);
	ret.push_back(cpplib::dstring("Fuel: ") << fuel << '/' << maxfuel());
	return ret;
}

/*
void cmd_cloak(int argc, char *argv[]){
	extern struct player *ppl;
	if(ppl->cs->w){
		entity_t *pt;
		for(pt = ppl->selected; pt; pt = pt->selectnext) if(pt->vft == &scepter_s){
			((scepter_t*)pt)->cloak = !((scepter_t*)pt)->cloak;
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
		pb = new Bullet(this, 5, 5.);
		pb->pos = mat.vp3(scepter_guns[i]);
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
	this->cooldown += SCEPTER_RELOADTIME;
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

/*static int space_collide_callback(const struct otjEnumHitSphereParam *param, entity_t *pt){
	if(((entity_t**)param->hint)[0] == pt || ((entity_t**)param->hint)[1] == pt)
		return 0;
	else return 1;
}*/

#if 0
void space_collide(entity_t *pt, warf_t *w, double dt, entity_t *collideignore, entity_t *collideignore2){
	const double ff = .2;
	entity_t *pt2;
	if(1 && w->otroot){
		entity_t *iglist[3] = {pt, collideignore, collideignore2};
		struct entity_static *igvft[1] = {&rstation_s};
		struct otjEnumHitSphereParam param;
		param.root = w->otroot;
		param.src = &pt->pos;
		param.dir = &pt->velo;
		param.dt = dt;
		param.rad = ((struct entity_private_static*)pt->vft)->hitradius;
		param.pos = NULL;
		param.norm = NULL;
		param.flags = OTJ_IGLIST | OTJ_IGVFT;
/*		param.callback = space_collide_callback;
		param.hint = iglist;*/
		param.iglist = iglist;
		param.niglist = 3;
		param.igvft = igvft;
		param.nigvft = 1;
		if(pt2 = otjEnumPointHitSphere(&param)){
			double sd = VECSDIST(pt->pos, pt2->pos);
			double r = ((struct entity_private_static*)pt->vft)->hitradius, r2 = ((struct entity_private_static*)pt2->vft)->hitradius;
			double sr = (r + r2) * (r + r2);
			avec3_t dr;
			double f = ff * dt / (sd / sr) * (pt2->mass < pt->mass ? pt2->mass / pt->mass : 1.);
			VECSUB(dr, pt->pos, pt2->pos);
			if(1. < f) /* prevent oscillation */
				f = 1.;
			VECSADD(pt->velo, dr, f);
		}
	}
	else for(pt2 = w->tl; pt2; pt2 = pt2->next) if(pt2 != pt && pt2 != collideignore && pt2 != collideignore2 && pt2->active){
		double sd = VECSDIST(pt->pos, pt2->pos);
		double r = ((struct entity_private_static*)pt->vft)->hitradius, r2 = ((struct entity_private_static*)pt2->vft)->hitradius;
		double sr = (r + r2) * (r + r2);
		if(sd != 0. && sd < sr){
			avec3_t dr;
			double f = ff * dt / (sd / sr) * (pt2->mass < pt->mass ? pt2->mass / pt->mass : 1.);
			VECSUB(dr, pt->pos, pt2->pos);
			if(1. < f) /* prevent oscillation */
				f = 1.;
			VECSADD(pt->velo, dr, f);
		}
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

void Sceptor::anim(double dt){
	Entity *pt = this;
	Sceptor *const pf = this;
	Sceptor *const p = this;
//	scarry_t *const mother = pf->mother;
//	Entity *pm = &mother->st.st;
	Mat4d mat, imat;

/*	if(!mother){
		if(p->pf){
			ImmobilizeTefpol3D(p->pf);
			p->pf = NULL;
		}
		pt->active = 0;
		return;
	}*/

	if(pf->pf)
		MoveTefpol3D(pf->pf, pt->pos, avec3_000, cs_shortburn.t, pf->docked);

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
			p->task = scepter_undock;
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
		int parking = 0;
		Entity *collideignore = NULL;
		if(w->pl->control != pt)
			pt->inputs.press = 0;
		if(pf->docked);
		else if(p->task == scepter_undockque){
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
				p->task = scepter_undock;
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
			pt->pyr[1] = approach(pt->pyr[1], fmod(atan2(-delta[0], delta[2]) + 2 * M_PI, 2 * M_PI), SCEPTER_ROTSPEED * dt, 2 * M_PI);
			pt->pyr[0] = approach(pt->pyr[0], atan2(delta[1], sqrt(delta[0] * delta[0] + delta[2] * delta[2])), SCEPTER_ROTSPEED * dt, 2 * M_PI);
			pt->pyr[2] = approach(pt->pyr[2] + M_PI, (pt->pyr[1] - oldyaw) / dt + M_PI, SCEPTER_ROLLSPEED * dt, 2 * M_PI) - M_PI;
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

			if(p->task == scepter_undock){
/*				if(!p->mother)
					p->task = scepter_idle;
				else{
					avec3_t mzh, dm;
					double sp;
					VECSUB(dm, pt->pos, pm->pos);
					quatrot(mzh, pm->rot, avec3_001);
					sp = -VECSP(mzh, dm);
					p->throttle = 1.;
					if(1. < sp)
						p->task = scepter_parade;
				}*/
			}
			else if(w->pl->control != pt) do{
				if(!pt->enemy || p->task == scepter_idle || p->task == scepter_parade){
					if(p->mother && mother->enemy){
						pt->enemy = mother->enemy;
						p->task = scepter_attack;
					}
					else if(findEnemy()){
						p->task = scepter_attack;
					}
				}
				else if(p->task == scepter_moveto){
					avec3_t target, dr;
					Quatd q2, q1;
					VECSUB(dr, pt->pos, p->dest);
					if(VECSLEN(dr) < .03 * .03){
						q1 = Quatd(0,0,0,1);
//						QUATCPY(q1, pm->rot);
						p->throttle = 0.;
						parking = 1;
						VECSADD(pt->velo, dr, -dt * .5);
						p->task = scepter_idle;
					}
					else{
						p->throttle = VECSLEN(dr) / 5. + .03;
						q1 = Quatd::direction(dr);
					}
					q2 = Quatd::slerp(pt->rot, q1, 1. - exp(-dt));
					pt->rot = q2;
					if(1. < p->throttle)
						p->throttle = 1.;
				}
				else if(pt->enemy && (p->task == scepter_attack || p->task == scepter_away)){
					Vec3d dv, forward;
					Vec3d xh, yh;
					double sx, sy, len, len2, maxspeed = SCEPTER_MAX_ANGLESPEED * dt;
					aquat_t qres, qrot;
					if(p->fuel < 30.){
						p->task = scepter_dockque;
						break;
					}
/*					VECSUB(dv, pt->enemy->pos, pt->pos);*/
					VECCPY(dv, delta);
					if(p->task == scepter_attack && VECSLEN(dv) < (pt->enemy->hitradius() * 1.2 + .5) * (pt->enemy->hitradius() * 1.2 + .5)){
						p->task = scepter_away;
					}
					else if(p->task == scepter_away && 5. * 5. < VECSLEN(dv)){
						p->task = scepter_attack;
					}
					VECNORMIN(dv);
					forward = pt->rot.trans(avec3_001);
					if(p->task == scepter_attack)
						VECSCALEIN(forward, -1);
		/*				sx = VECSP(&mat[0], dv);
					sy = VECSP(&mat[4], dv);
					pt->inputs.press |= (sx < 0 ? PL_4 : 0 < sx ? PL_6 : 0) | (sy < 0 ? PL_2 : 0 < sy ? PL_8 : 0);*/
					p->throttle = 1.;
					{
						avec3_t randomvec;
						for(i = 0; i < 3; i++)
							randomvec[i] = drseq(&w->rs) - .5;
						VECSADD(pt->velo, randomvec, dt * .5);
					}
					if(p->task == scepter_attack || VECSP(forward, dv) < -.5){
						VECVP(xh, forward, dv);
						len = len2 = VECLEN(xh);
						len = asin(len);
						if(maxspeed < len){
							len = maxspeed;
						}
						len = sin(len / 2.);
						if(len && len2){
							Vec3d omg, laac;
							VECSCALE(qrot, xh, len / len2);
							qrot[3] = sqrt(1. - len * len);
							QUATMUL(qres, qrot, pt->rot);
							QUATNORM(pt->rot, qres);

							/* calculate angular acceleration for displaying thruster bursts */
							VECSCALE(omg, qrot, 1. / dt);
							VECSUB(p->aac, omg, pt->omg);
							VECSCALEIN(p->aac, 1. / dt);
							VECCPY(pt->omg, omg);
							laac = pt->rot.cnj().trans(p->aac);
							if(laac[0] < 0) p->thrusts[0][0] += -laac[0];
							if(0 < laac[0]) p->thrusts[0][1] += laac[0];
							p->thrusts[0][0] = min(p->thrusts[0][0], 1.);
							p->thrusts[0][1] = min(p->thrusts[0][1], 1.);
						}
						if(trigger && p->task == scepter_attack && .99 < VECSP(dv, forward)){
							pt->inputs.change |= PL_ENTER;
							pt->inputs.press |= PL_ENTER;
						}
					}
					else{
						VECNULL(p->aac);
					}
				}
				else if(!pt->enemy && (p->task == scepter_attack || p->task == scepter_away)){
					p->task = scepter_parade;
				}
#if 0
				if(p->task == scepter_parade){
					avec3_t target, target0 = {-1., 0., -1.}, dr;
					aquat_t q2, q1;
					target0[0] += p->paradec % 10 * -.05;
					target0[2] += p->paradec / 10 * -.05;
					quatrot(target, pm->rot, target0);
					VECADDIN(target, pm->pos);
					VECSUB(dr, pt->pos, target);
					if(VECSLEN(dr) < .03 * .03){
						QUATCPY(q1, pm->rot);
						p->throttle = 0.;
						parking = 1;
						VECSADD(pt->velo, dr, -dt * .5);
					}
					else{
						p->throttle = VECSLEN(dr) / 5. + .03;
						quatdirection(q1, dr);
					}
					quatslerp(q2, pt->rot, q1, 1. - exp(-dt));
					QUATCPY(pt->rot, q2);
					if(1. < p->throttle)
						p->throttle = 1.;
				}
				else if(p->task == scepter_dockque || p->task == scepter_dock){
					avec3_t target, target0 = {-100. * SCARRY_SCALE, -50. * SCARRY_SCALE, 0.}, dr;
					aquat_t q2, q1;
					collideignore = pm;
					if(p->task == scepter_dockque)
						target0[2] += -1.;
					quatrot(target, pm->rot, target0);
					VECADDIN(target, pm->pos);
					VECSUB(dr, pt->pos, target);
					if(VECSLEN(dr) < .03 * .03){
						if(p->task == scepter_dockque)
							p->task = scepter_dock;
						else{
							scarry_dock(mother, pt, w);
							if(p->pf){
								ImmobilizeTefpol3D(p->pf);
								p->pf = NULL;
							}
							p->pf = NULL;
							p->docked = 1;
							if(mother->cargo < scarry_cargousage(mother)){
								scarry_undock(mother, pt, w);
							}
							return;
						}
					}
					p->throttle = VECSLEN(dr) * (p->task == scepter_dockque ? .2 : .05) + .03;
					quatdirection(q1, dr);
					quatslerp(q2, pt->rot, q1, 1. - exp(-dt));
					VECVP(pt->omg, q1, q2);
					QUATCPY(pt->rot, q2);
					if(1. < p->throttle)
						p->throttle = 1.;
				}
				else if(p->task == scepter_idle)
					p->throttle = 0.;
#else
				if(p->task == scepter_idle)
					p->throttle = 0.;
#endif
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
				pt->pyr[1] = approach(pt->pyr[1], fmod(atan2(-delta[0], delta[2]) + 2 * M_PI, 2 * M_PI), SCEPTER_ROTSPEED * dt, 2 * M_PI);
				pt->pyr[0] = approach(pt->pyr[0], atan2(delta[1], sqrt(delta[0] * delta[0] + delta[2] * delta[2])), SCEPTER_ROTSPEED * dt, 2 * M_PI);
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

					pt->pyr[1] = approach(pt->pyr[1], desired, SCEPTER_ROTSPEED * dt, 2 * M_PI);

					if(pt->enemy && pt->pyr[1] == desired){
					}

					pt->pyr[0] = approach(pt->pyr[0], atan2(delta[1], sqrt(delta[0] * delta[0] + delta[2] * delta[2])), SCEPTER_ROTSPEED * dt, 2 * M_PI);
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
				pt->pyr[1] = approach(pt->pyr[1], desiredp, SCEPTER_ROTSPEED * dt, 2 * M_PI);
				pt->pyr[0] = approach(pt->pyr[0], pt->pos[1] < .1 ? -M_PI / 6. : 0., SCEPTER_ROTSPEED * dt, 2 * M_PI);
			}
#endif
		}

		if(pt->inputs.press & PL_A){
			Vec3d qrot;
			qrot = mat.vec3(0) * dt;
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

		/* you're not allowed to accel further than certain velocity. */
		if(.3  < -p->velo.sp(mat.vec3(2)))
			p->throttle = 0.;
		else if(p->task == scepter_attack || p->task == scepter_away)
			p->throttle = 1.;

		/* Friction (in space!) */
		{
			double f = 1. / (1. + dt / (parking ? 1. : 3.));
			pt->velo *= f;
		}

		{
			double spd = pf->throttle * (p->task != scepter_attack ? .01 : .005);
			double consump = dt * (pf->throttle + p->fcloak * 4.); /* cloaking consumes fuel extremely */
			Vec3d acc, acc0(0., 0., -1.);
			if(p->fuel <= consump){
				if(.1 < pf->throttle)
					pf->throttle = .1;
				if(p->cloak)
					p->cloak = 0;
				p->fuel = 0.;
			}
			else
				p->fuel -= consump;
			acc = pt->rot.trans(acc0);
			pt->velo += acc * spd;
		}

		/* heat dissipation */
		p->heat *= exp(-dt * .2);

/*		pt->pyr[2] = approach(pt->pyr[2] + M_PI, (pt->pyr[1] - oldyaw) / dt + M_PI, SCEPTER_ROLLSPEED * dt, 2 * M_PI) - M_PI;
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
/*		if(p->task != scepter_undock && p->task != scepter_undockque){
			space_collide(pt, w, dt, collideignore, NULL);
		}*/
		pt->pos += pt->velo * dt;

		p->fcloak = approach(p->fcloak, p->cloak, dt, 0.);
	}
	else{
		pt->health += dt;
		if(0. < pt->health){
			struct tent3d_line_list *tell = w->tell;
//			effectDeath(w, pt);
//			playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .1, w->realtime);
/*			if(w->gibs && ((struct entity_private_static*)pt->vft)->sufbase){
				int i, n, base;
				struct entity_private_static *vft = (struct entity_private_static*)pt->vft;
				int m = vft->sufbase->np;
				n = m <= SCEPTER_MAX_GIBS ? m : SCEPTER_MAX_GIBS;
				base = m <= SCEPTER_MAX_GIBS ? 0 : rseq(&w->rs) % m;
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
			if(w->tell){
				double pos[3], dv[3], dist;
				Vec3d gravity = w->accel(this->pos, this->velo) / 2.;
				int i, n;
				n = (int)(dt * SCEPTER_SMOKE_FREQ + drseq(&w->rs));
				for(i = 0; i < n; i++){
					pos[0] = pt->pos[0] + (drseq(&w->rs) - .5) * .01;
					pos[1] = pt->pos[1] + (drseq(&w->rs) - .5) * .01;
					pos[2] = pt->pos[2] + (drseq(&w->rs) - .5) * .01;
					dv[0] = .5 * pt->velo[0] + (drseq(&w->rs) - .5) * .01;
					dv[1] = .5 * pt->velo[1] + (drseq(&w->rs) - .5) * .01;
					dv[2] = .5 * pt->velo[2] + (drseq(&w->rs) - .5) * .01;
					AddTeline3D(w->tell, pos, dv, .01, NULL, NULL, gravity, COLOR32RGBA(127 + rseq(&w->rs) % 32,127,127,255), TEL3_SPRITE | TEL3_INVROTATE | TEL3_NOLINE | TEL3_REFLECT, 1.5 + drseq(&w->rs) * 1.5);
				}
			}
			pt->pos += pt->velo * dt;
		}
	}
	st::anim(dt);
//	movesound3d(pf->hitsound, pt->pos);
}


#if 0
static int scepter_cull(entity_t *pt, wardraw_t *wd, double scale){
	scepter_t *p = (scepter_t*)pt;
	double pixels;
	if(p->task == scepter_undockque || glcullFrustum(pt->pos, .012 * scale, wd->pgc))
		return 1;
	pixels = .008 * fabs(glcullScale(&pt->pos, wd->pgc));
	if(pixels < 2)
		return 1;
	return 0;
}
#endif

void Sceptor::draw(wardraw_t *wd){
	static int init = 0;
	static suf_t *sufbase = NULL;
	static suftex_t *suft;
	static GLuint shader = 0;
	static GLint fracLoc, cubeEnvLoc, textureLoc, invEyeMat3Loc, transparency;
	double scale = SCEPTER_SCALE;
	Sceptor *const p = this;
	if(!this->w || this->docked)
		return;

	/* NLIPS: Non-Linear Inverse Perspective Scrolling */
	scale *= 1. + wd->vw->fov * g_nlips_factor * 500. / wd->vw->vp.m * 2. * (this->pos - wd->vw->pos).len();

	/* cull object */
/*	if(scepter_cull(pt, wd, scale))
		return;*/
	wd->lightdraws++;

	draw_healthbar(this, wd, health / maxhealth(), .01, fuel / maxfuel(), -1.);

	if(init == 0) do{
		FILE *fp;
		sufbase = CallLoadSUF("interceptor0.bin");
//		scepter_s.sufbase = LZUC(lzw_interceptor0);
		if(!sufbase) break;
		{
//			BITMAPFILEHEADER *bfh;
//			suftexparam_t stp;
//			stp.bmi = lzuc(lzw_interceptor, sizeof lzw_interceptor, NULL);
			CacheSUFMaterials(sufbase);
//			CallCacheBitmap("interceptor.bmp", "interceptor.bmp", NULL, NULL);
/*			bfh = ZipUnZip("rc.zip", "interceptor.bmp", NULL);
			if(!bfh)
				return;
			stp.bmi = &bfh[1];
			stp.env = GL_MODULATE;
			stp.mipmap = 0;
			stp.alphamap = 0;
			stp.magfil = GL_LINEAR;
			CacheSUFMTex("interceptor.bmp", &stp, NULL);
			free(bfh);*/
//			stp.bmi = lzuc(lzw_engine, sizeof lzw_engine, NULL);
//			CallCacheBitmap("engine.bmp", "engine.bmp", NULL, NULL);
/*			bfh = ZipUnZip("rc.zip", "engine.bmp", NULL);
			if(!bfh)
				return;
			stp.bmi = &bfh[1];
			CacheSUFMTex("engine.bmp", &stp, NULL);
			free(bfh/*stp.bmi*);*/
		}
		suft = AllocSUFTex(sufbase);

/*		do{
			GLuint vtx, frg;
			vtx = glCreateShader(GL_VERTEX_SHADER);
			frg = glCreateShader(GL_FRAGMENT_SHADER);
			if(!glsl_load_shader(vtx, "shaders/refract.vs") || !glsl_load_shader(frg, "shaders/refract.fs"))
				break;
			shader = glsl_register_program(vtx, frg);

			cubeEnvLoc = glGetUniformLocation(shader, "envmap");
			textureLoc = glGetUniformLocation(shader, "texture");
			invEyeMat3Loc = glGetUniformLocation(shader, "invEyeRot3x3");
			fracLoc = glGetUniformLocation(shader, "frac");
			transparency = glGetUniformLocation(shader, "transparency");
		}while(0);*/

		init = 1;
	} while(0);
	if(!sufbase){
		double pos[3];
		GLubyte col[4] = {255,255,0,255};
		gldPseudoSphere(pos, .005, col);
	}
	else{
		static const double normal[3] = {0., 1., 0.};
		double x;
		double pyr[3];
		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

		glPushMatrix();
		gldTranslate3dv(this->pos);
		gldScaled(scale);
		gldMultQuat(this->rot);
		glScalef(-1, 1, -1);
#if 0
		if(g_shader_enable && p->fcloak){
			extern float g_shader_frac;
			extern Star sun;
			extern coordsys *g_galaxysystem;
			GLuint cubetex;
			amat4_t rot, rot2;
			GLfloat irot3[9];
			tocsm(rot, wd->vw->cs, g_galaxysystem);
			mat4mp(rot2, rot, wd->vw->irot);
			MAT4TO3(irot3, rot2);
		glUseProgram(shader);
			glUniform1f(fracLoc, .5 * (1. - p->fcloak) + g_shader_frac * p->fcloak);
			glUniform1f(transparency, p->fcloak);
			glUniformMatrix3fv(invEyeMat3Loc, 1, GL_FALSE, irot3);
			glUniform1i(textureLoc, 0);
			if(glActiveTextureARB){
				glUniform1i(cubeEnvLoc, 1);
				cubetex = Island3MakeCubeMap(wd->vw, NULL, &sun);
				glActiveTextureARB(GL_TEXTURE1_ARB);
				glBindTexture(GL_TEXTURE_CUBE_MAP, cubetex);
				glActiveTextureARB(GL_TEXTURE0_ARB);
			}
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	/*		glColor4ub(0,0,0,255);
			DrawSUF(scepter_s.sufbase, SUF_ATR, &g_gldcache);*/
			DecalDrawSUF(scepter_s.sufbase, SUF_ATR, NULL, suft, NULL, NULL, NULL);
		glUseProgram(0);
			if(glActiveTextureARB){
				glActiveTextureARB(GL_TEXTURE1_ARB);
				glBindTexture(GL_TEXTURE_2D, 0);
				glActiveTextureARB(GL_TEXTURE0_ARB);
			}
		}
		else
#endif
		{
			DecalDrawSUF(sufbase, SUF_ATR, NULL, suft, NULL, NULL);
		}
		glPopMatrix();

/*		if(0 < wd->light[1]){
			static const double normal[3] = {0., 1., 0.};
			ShadowSUF(scepter_s.sufbase, wd->light, normal, pt->pos, pt->pyr, scale, NULL);
		}*/
		glPopAttrib();
	}
}

#define COLIST4(a) COLOR32R(a),COLOR32G(a),COLOR32B(a),COLOR32A(a)

void Sceptor::drawtra(wardraw_t *wd){
	Sceptor *p = this;
	Mat4d mat;

	/* cull object */
/*	if(scepter_cull(pt, wd, 1.))
		return;*/
	if(p->throttle){
		Vec3d pos, pos0(0, 0, 40. * SCEPTER_SCALE);
		GLubyte col[4] = {COLIST4(cnl_shortburn[0].col)};
		pos = this->rot.trans(pos0);
		pos += this->pos;
		gldSpriteGlow(pos, p->throttle * .005, col, wd->vw->irot);
		pos0[0] = 34.5 * SCEPTER_SCALE;
		pos = this->rot.trans(pos0);
		pos += this->pos;
		gldSpriteGlow(pos, p->throttle * .002, col, wd->vw->irot);
		pos0[0] = -34.5 * SCEPTER_SCALE;
		pos = this->rot.trans(pos0);
		pos += this->pos;
		gldSpriteGlow(pos, p->throttle * .002, col, wd->vw->irot);
	}

#if 0 /* thrusters appear unimpressing */
	tankrot(mat, pt);
	{
		avec3_t v;
		int j;
		VECADD(v, pt->omg, pt->pos);
		glColor4ub(255,127,0,255);
		glBegin(GL_LINES);
		glVertex3dv(pt->pos);
		glVertex3dv(v);
		glEnd();
		VECADD(v, p->aac, pt->pos);
		glColor4ub(255,127,255,255);
		glBegin(GL_LINES);
		glVertex3dv(pt->pos);
		glVertex3dv(v);
		glEnd();

		v[0] = p->thrusts[0][0];
		v[1] = p->thrusts[0][1];
		for(j = 0; j < 2; j++) if(v[j]){
			avec3_t v00[2] = {{0., 0., .003}, {0., 0., -.003}};
			int i;
			for(i = 0; i < 2; i++){
				struct gldBeamsData bd;
				avec3_t end;
				int lumi;
				struct random_sequence rs;
				int sign = (i + j) % 2 * 2 - 1;
				avec3_t v0;
				VECCPY(v0, v00[i]);
				bd.cc = bd.solid = 0;
				init_rseq(&rs, (long)(wd->gametime * 1e6) + i + pt);
				lumi = rseq(&rs) % 256 * min(fabs(v[j]) / .2, 1.);
				v0[1] += sign * .001;
				mat4vp3(end, mat, v0);
				gldBeams(&bd, wd->view, end, .00001, COLOR32RGBA(255,223,128,0));
				v0[1] += sign * .0003;
				mat4vp3(end, mat, v0);
				gldBeams(&bd, wd->view, end, .00007, COLOR32RGBA(255,255,255,lumi));
				v0[1] += sign * .0003;
				mat4vp3(end, mat, v0);
				gldBeams(&bd, wd->view, end, .00009, COLOR32RGBA(255,255,223,lumi));
				v0[1] += sign * .0003;
				mat4vp3(end, mat, v0);
				gldBeams(&bd, wd->view, end, .00005, COLOR32RGBA(255,255,191,0));
			}
		}
	}
#endif
}

int Sceptor::takedamage(double damage, int hitpart){
	int ret = 1;
	if(this->health < 0.)
		return 1;
	if(w->tell){
/*		int j, n;
		frexp(damage, &n);
		for(j = 0; j < n; j++){
			double velo[3];
			velo[0] = .15 * (drseq(&w->rs) - .5);
			velo[1] = .15 * (drseq(&w->rs) - .5);
			velo[2] = .15 * (drseq(&w->rs) - .5);
			AddTeline3D(tell, pt->pos, velo, .001, NULL, NULL, w->gravity,
				j % 2 ? COLOR32RGBA(255,255,255,255) : COLOR32RGBA(255,191,63,255),
				TEL3_HEADFORWARD | TEL3_REFLECT | TEL3_FADEEND, .5 + drseq(&w->rs) * .5);
		}*/
	}
//	this->hitsound = playWave3D("hit.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
	if(0 < health && health - damage <= 0){
		int i;
		ret = 0;
/*		effectDeath(w, pt);*/
		for(i = 0; i < 32; i++){
			double pos[3], velo[3];
			velo[0] = drseq(&w->rs) - .5;
			velo[1] = drseq(&w->rs) - .5;
			velo[2] = drseq(&w->rs) - .5;
			VECNORMIN(velo);
			VECCPY(pos, this->pos);
			VECSCALEIN(velo, .1);
			VECSADD(pos, velo, .1);
			AddTeline3D(w->tell, pos, velo, .005, NULL, NULL, w->accel(this->pos, this->velo), COLOR32RGBA(255, 31, 0, 255), TEL3_HEADFORWARD | TEL3_THICK | TEL3_FADEEND | TEL3_REFLECT, 1.5 + drseq(&w->rs));
		}
/*		((scepter_t*)pt)->pf = AddTefpolMovable3D(w->tepl, pt->pos, pt->velo, nullvec3, &cs_firetrail, TEP3_THICKER | TEP3_ROUGH, cs_firetrail.t);*/
//		((scepter_t*)pt)->hitsound = playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
		health = -2.;
//		pt->deaths++;
	}
	else
		health -= damage;
	return ret;
}

void Sceptor::postframe(){
	if(mother && (!mother->w || docked && mother->w != w))
		mother = NULL;
	if(enemy && enemy->w != w)
		enemy = NULL;
	st::postframe();
}

bool Sceptor::isTargettable()const{
	return true;
}
bool Sceptor::isSelectable()const{return true;}

double Sceptor::hitradius(){
	return .01;
}

#if 0
static int scepter_visibleFrom(const entity_t *viewee, const entity_t *viewer){
	scepter_t *p = (scepter_t*)viewee;
	return p->fcloak < .5;
}

static double scepter_signalSpectrum(const entity_t *pt, double lwl){
	scepter_t *p = (scepter_t*)pt;
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


static warf_t *scepter_warp_dest(entity_t *pt, const warf_t *w){
	scepter_t *p = (scepter_t*)pt;
	if(!p->mother || !p->mother->st.st.vft->warp_dest)
		return NULL;
	return p->mother->st.st.vft->warp_dest(p->mother, w);
}
#endif

double Sceptor::maxfuel()const{
	return 120.;
}

