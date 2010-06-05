#include "arms.h"
#include "player.h"
#include "astro.h"
#include "viewer.h"
#include "argtok.h"
//#include "entity_p.h"
#include "antiglut.h"
#include "cmd.h"
#include "bullet.h"
//#include "aim9.h"
//#include "suflist.h"
//#include "yssurf.h"
//#include "walk.h"
#include "bullet.h"
#include "serial_util.h"
#include "material.h"
#include "Missile.h"
#include "EntityCommand.h"
#include "draw/effects.h"
extern "C"{
#include "calc/calc.h"
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/gl/gldraw.h>
#include <clib/suf/sufbin.h>
#include <clib/suf/sufdraw.h>
#include <clib/zip/UnZip.h>
#include <clib/zip/UniformLoader.h>
}
#include <limits.h>
#include <gl/glext.h>

const char *hardpoint_static::classname()const{
	return "hardpoint_static";
}

const unsigned hardpoint_static::classid = registerClass("hardpoint_static", Conster<hardpoint_static>);

void hardpoint_static::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << pos; /* base position relative to object origin */
	sc.o << rot; /* base rotation */
	sc.o << name;
	sc.o << flagmask;
}

void hardpoint_static::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	cpplib::dstring name;
	sc.i >> pos; /* base position relative to object origin */
	sc.i >> rot; /* base rotation */
	sc.i >> name;
	sc.i >> flagmask;

	this->name = strnewdup(name, name.len());
}



void ArmBase::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << base;
	sc.o << target;
	sc.o << hp;
	sc.o << ammo;
}

void ArmBase::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> base;
	sc.i >> target;
	sc.i >> hp;
	sc.i >> ammo;
}

void ArmBase::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext&)){
	st::dive(sc, method);
	const_cast<hardpoint_static*>(hp)->dive(sc, method);
}

void ArmBase::postframe(){
	// inter-CoordSys weapons may remember the target run away to another CoordSys?
	if(target && target->w != w)
		target = NULL;
}

double ArmBase::hitradius()const{
	return 0.;
}

Entity *ArmBase::getOwner(){
	return base;
}

bool ArmBase::isTargettable()const{
	return false;
}
bool ArmBase::isSelectable()const{return true;}

Entity::Props ArmBase::props()const{
	Props ret = st::props();
	ret.push_back(cpplib::dstring("Ammo: ") << ammo);
	return ret;
}

cpplib::dstring ArmBase::descript()const{
	return cpplib::dstring(idname());
};

void ArmBase::align(){
	// dead arms do not follow the base
	if(health <= 0. || !base)
		return;

	w = base->w;
	race = base->race;

	/* calculate tr(pb->pos) * pb->pyr * pt->pos to get global coords */
	Mat4d mat;
	base->transform(mat);
	pos = mat.vp3(hp->pos);
	velo = base->velo + base->omg.vp(base->rot.trans(hp->pos));
	this->rot = base->rot * hp->rot;
}

void drawmuzzleflash4(const Vec3d &pos, const Mat4d &rot, double rad, const Mat4d &irot, struct random_sequence *prs, const Vec3d &viewer){
	int i;
	glPushMatrix();
	gldTranslate3dv(pos);
/*	glRotated(deg_per_rad * (*pyr)[1], 0., -1., 0.);
	glRotated(deg_per_rad * (*pyr)[0], 1., 0., 0.);
	glRotated(deg_per_rad * (*pyr)[2], 0., 0., -1.);*/
	glMultMatrixd(rot);
	glScaled(rad, rad, rad);
	for(i = 0; i < 4; i++){
		Vec3d pz;
		glRotated(90, 0., 0., 1.);
		glBegin(GL_TRIANGLE_FAN);
		glColor4ub(255,255,31,255);
		pz[0] = drseq(prs) * .25 + .5;
		pz[1] = drseq(prs) * .3 - .15;
		pz[2] = -drseq(prs) * .25;
		glVertex3dv(pz);
		glVertex3d(0., 0., 0.);
		glColor4ub(255,0,0,0);
		pz[0] = drseq(prs) * .25 + .5;
		pz[1] = drseq(prs) * .15 + .15;
		pz[2] = -drseq(prs) * .25;
		glVertex3dv(pz);
		pz[0] = drseq(prs) * .25 + .75;
		pz[1] = drseq(prs) * .3 - .15;
		pz[2] = -drseq(prs) * .15 - .25;
		glVertex3dv(pz);
		pz[0] = drseq(prs) * .25 + .5;
		pz[1] = -drseq(prs) * .15 - .15;
		pz[2] = -drseq(prs) * .25;
		glVertex3dv(pz);
		glColor4ub(255,255,31,255);
		glVertex3d(0., 0., 0.);
		glEnd();
	}
	glPopMatrix();
/*	{
		struct gldBeamsData bd;
		amat4_t mat;
		avec3_t nh, nh0 = {0., 0., -.002}, v;
		pyrmat(*pyr, &mat);
		MAT4VP3(nh, mat, nh0);
		bd.cc = 0;
		bd.solid = 0;
		gldBeams(&bd, *viewer, *pos, rad * .2, COLOR32RGBA(255,255,31,255));
		VECADD(v, *pos, nh);
		gldBeams(&bd, *viewer, v, rad * .4, COLOR32RGBA(255,127,0,255));
		VECADDIN(v, nh);
		gldBeams(&bd, *viewer, v, rad * .01, COLOR32RGBA(255,127,0,255));
	}*/
/*	{
		static int init = 0;
		static GLuint list;
		struct gldBeamsData bd;
		amat4_t mat;
		avec3_t nh, nh0 = {0., 0., -.002}, v;
		if(!init){
			GLbyte buf[4][4] = {
				{255,255,255,0},
				{255,255,255,64},
				{255,255,255,191},
				{255,255,255,255}
			};
			glNewList(list = glGenLists(1), GL_COMPILE);
			glEnable(GL_TEXTURE_1D);
			glDisable(GL_BLEND);
			glTexImage1D(GL_TEXTURE_1D, 0, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
			glEndList();
			init = 1;
		}
		glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT);
		glCallList(list);
		pyrmat(*pyr, &mat);
		MAT4VP3(nh, mat, nh0);
		bd.cc = 0;
		bd.solid = 0;
			glEnable(GL_TEXTURE_1D);
		gldBeams(&bd, *viewer, *pos, rad * .2, COLOR32RGBA(255,255,31,255));
		VECADD(v, *pos, nh);
		gldBeams(&bd, *viewer, v, rad * .4, COLOR32RGBA(255,127,0,255));
		VECADDIN(v, nh);
		gldBeams(&bd, *viewer, v, rad * .01, COLOR32RGBA(255,127,0,255));
		glPopAttrib();
	}*/
}



#define STURRET_SCALE (.000005 * 3)
#define MTURRET_SCALE (.00005)
#define MTURRET_MAX_GIBS 30
#define MTURRET_BULLETSPEED 2.
#define MTURRET_VARIANCE (.001 * M_PI)
#define MTURRET_INTOLERANCE (M_PI / 20.)
#define MTURRETROTSPEED (.4*M_PI)
#define MTURRETMANUALROTSPEED (MTURRETROTSPEED * .5)

suf_t *MTurret::suf_turret = NULL;
suf_t *MTurret::suf_barrel = NULL;


MTurret::MTurret(Entity *abase, const hardpoint_static *ahp) : st(abase, ahp), cooldown(0), mf(0), forceEnemy(false){
	health = 1000;
	ammo = 1500;
	py[0] = 0;
	py[1] = 0;
}

const char *MTurret::idname()const{
	return "mturret";
};

const char *MTurret::classname()const{
	return "MTurret";
};

const unsigned MTurret::classid = registerClass("MTurret", Conster<MTurret>);

void MTurret::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << cooldown;
	sc.o << py[0] << py[1]; // pitch and yaw
	sc.o << mf; // muzzle flash time
	sc.o << forceEnemy;
}

void MTurret::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> cooldown;
	sc.i >> py[0] >> py[1]; // pitch and yaw
	sc.i >> mf; // muzzle flash time
	sc.i >> forceEnemy;
}

const char *MTurret::dispname()const{
	return "Mounted Turret";
};

void MTurret::cockpitView(Vec3d &pos, Quatd &rot, int)const{
	rot = this->rot * Quatd(0, sin(py[1]/2), 0, cos(py[1]/2)) * Quatd(sin(py[0]/2), 0, 0, cos(py[0]/2));
	pos = this->pos + rot.trans(Vec3d(.0, .005, .015));
}

void MTurret::draw(wardraw_t *wd){
	// Viewing volume culling
	if(wd->vw->gc->cullFrustum(pos, .03))
		return;
	// Scale too small culling
	if(fabs(wd->vw->gc->scale(pos)) * .03 < 2)
		return;
	MTurret *a = this;
	double scale;
	if(!suf_turret)
		suf_turret = CallLoadSUF("models/turretz1.bin");
	if(!suf_barrel)
		suf_barrel = CallLoadSUF("models/barrelz1.bin");

	{
		const double bscale = MTURRET_SCALE;
		static const GLfloat rotaxis2[16] = {
			-1,0,0,0,
			0,1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};

		glPushMatrix();
		gldTranslate3dv(pos);
		gldMultQuat(rot);
		glRotated(deg_per_rad * a->py[1], 0., 1., 0.);
		glPushMatrix();
		gldScaled(bscale);
		glMultMatrixf(rotaxis2);
		DrawSUF(suf_turret, SUF_ATR, NULL);
		glPopMatrix();
/*		if(5 < scale)*/{
			const Vec3d pos(0., .00075, -0.0015);
			gldTranslate3dv(pos);
			glRotated(deg_per_rad * a->py[0], 1., 0., 0.);
			gldTranslate3dv(-pos);
			gldScaled(bscale);
			glMultMatrixf(rotaxis2);
			DrawSUF(suf_barrel, SUF_ATR, NULL);
		}
		glPopMatrix();
	}
}

static const double mturret_ofs[3] = {0., 0., -.001};
static const double mturret_range[2][2] = {-M_PI / 16., M_PI / 2, -M_PI, M_PI};

void MTurret::findtarget(Entity *pb, const hardpoint_static *hp, const Entity *ignore_list[], int nignore_list){
	MTurret *pt = this;
	WarField *w = pb->w;
	double bulletrange = bulletspeed() * bulletlife(); /* sense range */
	double best = bulletrange * bulletrange;
	static const Vec3d right(1., 0., 0.), left(-1., 0., 0.);
	Entity *pt2, *closest = NULL;

	// Obtain reverse transformation matrix to the turret's local coordinate system.
	Mat4d mat2 = this->rot.cnj().tomat4().translatein(-this->pos);

	// Quit chasing target that is out of shooting range.
	if(target && best < (target->pos - pos).slen())
		target = NULL;

	for(pt2 = w->el; pt2; pt2 = pt2->next){
		Vec3d delta, ldelta;
		double theta, phi, f;

		f = findtargetproc(pb, hp, pt2);
		if(f == 0.)
			continue;

		if(!(pt2->isTargettable() && pt2 != pb && pt2->w == w && pt2->health > 0. && pt2->race != -1 && pt2->race != pb->race))
			continue;

/*		if(!entity_visible(pb, pt2))
			continue;*/

		// Do not passively attack Resource Station that can be captured instead of being destroyed.
		if(!strcmp(pt2->classname(), "RStation"))
			continue;

		ldelta = mat2.vp3(pt2->pos);
		phi = -atan2(ldelta[2], ldelta[0]);
		theta = atan2(ldelta[1], sqrt(ldelta[0] * ldelta[0] + ldelta[2] * ldelta[2]));

		// Ignore targets that are out of turret rotation or barrel pitch range.
		if(!(mturret_range[1][0] < phi && phi < mturret_range[1][1] && mturret_range[0][0] < theta && theta < mturret_range[0][1]))
			continue;

		double sdist = (pt2->pos - pb->pos).slen();
		if(bulletrange * bulletrange < sdist)
			continue;

		// Weigh already targetted enemy to keep shooting single enemy.
		sdist *= (pt2 == enemy ? .25 * .25 : 1.) / f;
		if(sdist < best){
			best = sdist;
			closest = pt2;
		}
	}
	if(closest)
		target = closest;
}

double MTurret::findtargetproc(const Entity *pb, const hardpoint_static *hp, const Entity *pt2){
	return 1.;
}

int MTurret::wantsFollowTarget()const{return 2;}

void MTurret::tryshoot(){
	if(ammo <= 0){
		this->cooldown += reloadtime();
		return;
	}
	static const avec3_t forward = {0., 0., -1.};
	Bullet *pz;
	Quatd qrot;
	pz = new Bullet(base, bulletlife(), 120.);
	w->addent(pz);
	Mat4d mat2;
	base->transform(mat2);
	mat2.translatein(hp->pos);
	Mat4d rot = hp->rot.tomat4();
	Mat4d mat = mat2 * rot;
	mat.translatein(0., .001, -0.002);
	mat2 = mat.roty(this->py[1] + (drseq(&w->rs) - .5) * MTURRET_VARIANCE);
	mat = mat2.rotx(this->py[0] + (drseq(&w->rs) - .5) * MTURRET_VARIANCE);
	pz->pos = mat.vp3(mturret_ofs);
	pz->velo = mat.dvp3(forward) * bulletspeed() + this->velo;
	this->cooldown += reloadtime();
	this->mf += .2;
	ammo--;
}

void MTurret::control(const input_t *inputs, double dt){
	this->inputs = *inputs;
}

void MTurret::anim(double dt){
	if(!base || !base->w)
		w = NULL;
	if(!w)
		return;
	MTurret *const a = this;
	Entity *pt = base;
//	Entity **ptarget = &target;
	Vec3d epos; /* estimated enemy position */
	double phi, theta; /* enemy direction */

	/* find enemy logic */
	if(!forceEnemy)
		findtarget(pt, hp);

/*	if(*ptarget && (w != (*ptarget)->w || (*ptarget)->health <= 0.))
		*ptarget = NULL;*/

	if(a->mf < dt)
		a->mf = 0.;
	else
		a->mf -= dt;

	if(w->pl && w->pl->control == this){
		double pydst[2] = {py[0], py[1]};
		if(inputs.press & PL_A)
			pydst[1] += MTURRETMANUALROTSPEED * dt;
		if(inputs.press & PL_D)
			pydst[1] -= MTURRETMANUALROTSPEED * dt;
		if(inputs.press & PL_W)
			pydst[0] += MTURRETMANUALROTSPEED * dt;
		if(inputs.press & PL_S)
			pydst[0] -= MTURRETMANUALROTSPEED * dt;
		a->py[1] = approach(a->py[1] + M_PI, pydst[1] + M_PI, MTURRETROTSPEED * dt, 2 * M_PI) - M_PI;
		a->py[0] = rangein(approach(a->py[0] + M_PI, pydst[0] + M_PI, MTURRETROTSPEED * dt, 2 * M_PI) - M_PI, mturret_range[0][0], mturret_range[0][1]);
		if(inputs.press & (PL_ENTER | PL_LCLICK)) while(a->cooldown < dt){
			tryshoot();
		}
	}
	else if(target && wantsFollowTarget()) do{/* estimating enemy position */
		double bulletRange = bulletspeed() * bulletlife();
		bool notReachable = bulletRange * bulletRange < (target->pos - this->pos).slen();

		// If not forced to attack certain target, forget about target that bullets have no way to reach.
		if(notReachable && !forceEnemy){
			target = NULL;
			break;
		}

		Vec3d pos, velo, pvelo, gepos;
		Mat4d rot;

		/* calculate tr(pb->pos) * pb->pyr * pt->pos to get global coords */
		Mat4d mat2 = this->rot.cnj().tomat4().translatein(-this->pos);
		pos = mat2.vp3(target->pos);
		velo = mat2.dvp3(target->velo);
		pvelo = mat2.dvp3(pt->velo);

		estimate_pos(epos, pos, velo, vec3_000, pvelo, bulletspeed(), w);
			
		/* these angles are in local coordinates */
		phi = -atan2(epos[0], -(epos[2]));
		theta = atan2(epos[1], sqrt(epos[0] * epos[0] + epos[2] * epos[2]));
		a->py[1] = approach(a->py[1] + M_PI, phi + M_PI, MTURRETROTSPEED * dt, 2 * M_PI) - M_PI;
		if(1 < wantsFollowTarget())
			a->py[0] = rangein(approach(a->py[0] + M_PI, theta + M_PI, MTURRETROTSPEED * dt, 2 * M_PI) - M_PI, mturret_range[0][0], mturret_range[0][1]);

		/* shooter logic */
		while(a->cooldown < dt){
			double yaw = a->py[1];
			double pitch = a->py[0];

			// Do not waste bullets at not reachable target.
			if(!notReachable && fabs(phi - yaw) < MTURRET_INTOLERANCE && fabs(pitch - theta) < MTURRET_INTOLERANCE){
				tryshoot();
			}
			else
				a->cooldown += .5 + (drseq(&w->rs) - .5) * .2;
		}
	}while(0);

	if(a->cooldown < dt)
		a->cooldown = 0.;
	else
		a->cooldown -= dt;
}

void MTurret::postframe(){
	if(target && !target->w){
		target = NULL;
		if(forceEnemy)
			forceEnemy = false;
	}
	if(base && !base->w)
		base = NULL;
}


void MTurret::drawtra(wardraw_t *wd){
	Entity *pb = base;
	double bscale = MTURRET_SCALE, tscale = .00002;
	if(this->mf){
		struct random_sequence rs;
		Mat4d mat2, mat, rot;
		Vec3d pos, const pos0(0, 0, -.008);
		init_rseq(&rs, (long)this ^ *(long*)&wd->vw->viewtime);
		this->transform(mat);
		mat2 = mat.roty(this->py[1]);
		mat2.translatein(0., .001, -0.002);
		mat = mat2.rotx(this->py[0]);
		pos = mat.vp3(pos0);
		rot = mat;
		rot.vec3(3).clear();
		drawmuzzleflash4(pos, rot, .01, wd->vw->irot, &rs, wd->vw->pos);

		static GLuint texname = 0;
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		if(!texname){
			suftexparam_t stp;
			stp.flags = STP_ENV | STP_MAGFIL | STP_MINFIL | STP_WRAP_S;
			stp.env = GL_MODULATE;
			stp.magfil = GL_LINEAR;
			stp.minfil = GL_LINEAR;
			stp.wraps = GL_CLAMP_TO_BORDER;
			texname = CallCacheBitmap5("muzzle.bmp", "muzzle.bmp", &stp, NULL, NULL);
		}
		glCallList(texname);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE); // Add blend
		float f = mf / .1, fi = 1. - mf / .2;
		glColor4f(f,f,f,1);
		gldTextureBeam(wd->vw->pos, pos, pos + rot.vp3(-vec3_001) * .075 * fi, .02 * fi);
		glPopAttrib();
	}
}

double MTurret::hitradius()const{
	return .005;
}

std::vector<cpplib::dstring> MTurret::props()const{
	std::vector<cpplib::dstring> ret = st::props();
	ret.push_back(cpplib::dstring("Cooldown: ") << cooldown);
	return ret;
}

cpplib::dstring MTurret::descript()const{
	return cpplib::dstring() << idname() << " " << health << " " << ammo;
}

float MTurret::reloadtime()const{
	return 2.;
}

double MTurret::bulletspeed()const{
	return 2.;
}

float MTurret::bulletlife()const{
	return 3.;
}

bool MTurret::command(EntityCommand *com){
	if(InterpretCommand<HaltCommand>(com)){
		target = NULL;
		forceEnemy = false;
		return true;
	}
	else if(AttackCommand *ac = InterpretCommand<AttackCommand>(com)){
		if(!ac->ents.empty()){
			Entity *e = *ac->ents.begin();
			if(e && e->race != race && e->getUltimateOwner() != getUltimateOwner()){
				target = e;
				forceEnemy = true;
				return true;
			}
		}
	}
	else if(ForceAttackCommand *fac = InterpretCommand<ForceAttackCommand>(com)){
		if(!fac->ents.empty()){
			Entity *e = *fac->ents.begin();

			// Though if force attacked, you cannot hurt yourself.
			if(e && e->getUltimateOwner() != getUltimateOwner()){
				target = e;
				forceEnemy = true;
				return true;
			}
		}
	}
	return false;
}







GatlingTurret::GatlingTurret() : barrelrot(0), barrelomg(0){}

GatlingTurret::GatlingTurret(Entity *abase, const hardpoint_static *hp) : st(abase, hp), barrelrot(0), barrelomg(0){
	ammo = 50;
}

const Vec3d GatlingTurret::barrelpos(0., 30, 0.);

const char *GatlingTurret::idname()const{return "GatlingTurret";}
const char *GatlingTurret::classname()const{return "GatlingTurret";}

const unsigned GatlingTurret::classid = registerClass("GatlingTurret", Conster<GatlingTurret>);

void GatlingTurret::serialize(SerializeContext &sc){
	st::serialize(sc);
}

void GatlingTurret::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
}

const char *GatlingTurret::dispname()const{
	return "Gatling Turret";
};

void GatlingTurret::anim(double dt){
	barrelrot += barrelomg * dt;
	barrelomg = MAX(0, barrelomg - dt * 2. * M_PI);
	st::anim(dt);
}

void GatlingTurret::draw(wardraw_t *wd){
	// Viewing volume culling
	if(wd->vw->gc->cullFrustum(pos, .03))
		return;
	// Scale too small culling
	if(fabs(wd->vw->gc->scale(pos)) * .03 < 2)
		return;
	static suf_t *suf_turret = NULL;
	static suf_t *suf_barrel = NULL;
	static suf_t *suf_barrels = NULL;
	double scale;
	if(!suf_turret)
		suf_turret = CallLoadSUF("models/turretg1.bin");
	if(!suf_barrel)
		suf_barrel = CallLoadSUF("models/barrelg1.bin");
	if(!suf_barrels)
		suf_barrels = CallLoadSUF("models/barrelsg1.bin");

	{
		const double bscale = MTURRET_SCALE;
		static const GLfloat rotaxis2[16] = {
			-1,0,0,0,
			0,1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};

		glPushMatrix();
		gldTranslate3dv(pos);
		gldMultQuat(rot);
		glRotated(deg_per_rad * this->py[1], 0., 1., 0.);
		glPushMatrix();
		gldScaled(bscale);
		glMultMatrixf(rotaxis2);
		DrawSUF(suf_turret, SUF_ATR, NULL);
		glPopMatrix();
/*		if(5 < scale)*/{
			gldScaled(bscale);
			gldTranslate3dv(barrelpos);
			glRotated(deg_per_rad * this->py[0], 1., 0., 0.);
			gldTranslate3dv(-barrelpos);
			glMultMatrixf(rotaxis2);
			DrawSUF(suf_barrel, SUF_ATR, NULL);
			gldTranslate3dv(barrelpos);
			glRotated(barrelrot * deg_per_rad/*w->war_time() * 360. / reloadtime() / 3*/, 0, 0, 1);
			gldTranslate3dv(-barrelpos);
			DrawSUF(suf_barrels, SUF_ATR, NULL);
		}
		glPopMatrix();
	}
}

void GatlingTurret::drawtra(wardraw_t *wd){
	Entity *pb = base;
	double bscale = MTURRET_SCALE;
	if(this->mf){
		struct random_sequence rs;
		Mat4d mat2, mat, rot;
		Vec3d pos, const muzzlepos(0, .003 * .5, -.0134 * .5);
		init_rseq(&rs, (long)this ^ *(long*)&wd->vw->viewtime);
		this->transform(mat);
		mat2 = mat.roty(this->py[1]);
		mat2.translatein(barrelpos * bscale);
		mat = mat2.rotx(this->py[0]);
		mat.translatein(-barrelpos * bscale);
		pos = mat.vp3(muzzlepos);
		rot = mat;
		rot.vec3(3).clear();
		drawmuzzleflash4(pos, rot, .005, wd->vw->irot, &rs, wd->vw->pos);

		static GLuint texname = 0;
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		if(!texname){
			suftexparam_t stp;
			stp.flags = STP_ENV | STP_MAGFIL | STP_MINFIL | STP_WRAP_S;
			stp.env = GL_MODULATE;
			stp.magfil = GL_LINEAR;
			stp.minfil = GL_LINEAR;
			stp.wraps = GL_CLAMP_TO_BORDER;
			texname = CallCacheBitmap5("muzzle.bmp", "muzzle.bmp", &stp, NULL, NULL);
		}
		glCallList(texname);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE); // Add blend
		float f = mf / .075 * 2, fi = 1. - mf / .075;
		glColor4f(f,f,f,1);
		gldTextureBeam(wd->vw->pos, pos, pos + rot.vp3(-vec3_001) * .03 * fi, .01 * fi);
		glPopAttrib();
	}
}

float GatlingTurret::reloadtime()const{
	return .1;
}

double GatlingTurret::bulletspeed()const{
	return 4.;
}

void GatlingTurret::tryshoot(){
	if(ammo <= 0)
		return;
	static const avec3_t forward = {0., 0., -1.};
	Bullet *pz;
	Quatd qrot;
	pz = new Bullet(base, 3., 10.);
	w->addent(pz);
	Mat4d mat;
	this->transform(mat);
	mat.translatein(barrelpos * MTURRET_SCALE);
	Mat4d mat2 = mat.roty(this->py[1] + (drseq(&w->rs) - .5) * MTURRET_VARIANCE);
	mat = mat2.rotx(this->py[0] + (drseq(&w->rs) - .5) * MTURRET_VARIANCE);
	pz->pos = mat.vp3(mturret_ofs);
	pz->velo = mat.dvp3(forward) * bulletspeed() + this->velo;
	this->cooldown += reloadtime();
	this->mf += .075;
	this->barrelomg = 2. * M_PI / reloadtime() / 3.;
	ammo--;
	if(!ammo){
		ammo = 50;
		this->cooldown += 5.;
	}
}

















const char *LTurret::classname()const{return "LTurret";}
const unsigned LTurret::classid = registerClass("LTurret", Conster<LTurret>);
/*
void LTurret::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << cooldown;
	sc.o << py[0] << py[1]; // pitch and yaw
	sc.o << mf; // muzzle flash time
	sc.o << forceEnemy;
}

void LTurret::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> cooldown;
	sc.i >> py[0] >> py[1]; // pitch and yaw
	sc.i >> mf; // muzzle flash time
	sc.i >> forceEnemy;
}
*/

double LTurret::hitradius()const{return .03;}

void LTurret::anim(double dt){
	st::anim(dt);
	blowback += blowbackspeed * dt;
	if(.010 < blowback)
		blowbackspeed = 0;
	blowbackspeed *= exp(-dt);
	blowback *= exp(-dt);
}

void LTurret::draw(wardraw_t *wd){
	// Viewing volume culling
	if(wd->vw->gc->cullFrustum(pos, .03))
		return;
	// Scale too small culling
	if(fabs(wd->vw->gc->scale(pos)) * .03 < 2)
		return;
	static suf_t *suf_turret = NULL, *suf_barrel = NULL;
	double scale;
	if(!suf_turret)
		suf_turret = CallLoadSUF("models/lturret.bin");
	if(!suf_barrel)
		suf_barrel = CallLoadSUF("models/lbarrel.bin");

	const double bscale = .001;
	static const GLfloat rotaxis2[16] = {
		-1,0,0,0,
		0,1,0,0,
		0,0,-1,0,
		0,0,0,1,
	};

	glPushMatrix();
	gldTranslate3dv(pos);
	gldMultQuat(rot);
	glRotated(deg_per_rad * this->py[1], 0., 1., 0.);
	glPushMatrix();
	gldScaled(bscale);
	glMultMatrixf(rotaxis2);
	DrawSUF(suf_turret, SUF_ATR, NULL);
	glPopMatrix();
	for(int i = 0; i < 2; i++){
		Vec3d pos(.005 * (i * 2 - 1), .005, -0.0025);
		glPushMatrix();
		gldTranslate3dv(pos);
		glRotated(deg_per_rad * this->py[0], 1., 0., 0.);
		glTranslated(0, 0, blowback);
		gldScaled(bscale);
		glMultMatrixf(rotaxis2);
		DrawSUF(suf_barrel, SUF_ATR, NULL);
		glPopMatrix();
	}
	glPopMatrix();
}

void LTurret::drawtra(wardraw_t *wd){
	Entity *pb = base;
	double bscale = 1;
	if(this->mf) for(int i = 0; i < 2; i++){
		struct random_sequence rs;
		Mat4d mat2, mat, rot;
		Vec3d pos, const barrelpos(.005 * (i * 2 - 1), .005, -.0025), const muzzlepos(0, .0, -.030 + blowback);
		init_rseq(&rs, (long)this ^ *(long*)&wd->vw->viewtime);
		this->transform(mat);
		mat2 = mat.roty(this->py[1]);
		mat2.translatein(barrelpos * bscale);
		mat = mat2.rotx(this->py[0]);
//		mat.translatein(-barrelpos * bscale);
		pos = mat.vp3(muzzlepos);
		rot = mat;
		rot.vec3(3).clear();
//		drawmuzzleflash4(pos, rot, .005, wd->vw->irot, &rs, wd->vw->pos);

		static GLuint texname = 0;
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		if(!texname){
			suftexparam_t stp;
			stp.flags = STP_ENV | STP_MAGFIL | STP_MINFIL | STP_WRAP_S;
			stp.env = GL_MODULATE;
			stp.magfil = GL_LINEAR;
			stp.minfil = GL_LINEAR;
			stp.wraps = GL_CLAMP_TO_BORDER;
			texname = CallCacheBitmap5("muzzle.bmp", "muzzle.bmp", &stp, NULL, NULL);
		}
		glCallList(texname);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE); // Add blend
		float f = mf / .3 * 2, fi = 1. - mf / .3;
		glColor4f(f,f,f,1);
		gldTextureBeam(wd->vw->pos, pos, pos + rot.vp3(-vec3_001) * .200 * fi, .060 * fi);
		glPopAttrib();
	}
}

float LTurret::reloadtime()const{return 4.;}
float LTurret::bulletlife()const{return 5.;}

void LTurret::tryshoot(){
	if(ammo <= 0)
		return;
	static const avec3_t forward = {0., 0., -1.};
	Mat4d mat2;
	base->transform(mat2);
	mat2.translatein(hp->pos);
	Mat4d rot = hp->rot.tomat4();
	Mat4d mat = mat2 * rot;
	mat.translatein(0., .005, -0.0025);
	double yaw = this->py[1] + (drseq(&w->rs) - .5) * MTURRET_VARIANCE;
	double pitch = this->py[0] + (drseq(&w->rs) - .5) * MTURRET_VARIANCE;
	mat2 = mat.roty(yaw);
	mat = mat2.rotx(pitch);
	Quatd qrot = base->rot * hp->rot * Quatd(0, sin(yaw / 2.), 0, cos(yaw / 2.)) * Quatd(sin(pitch / 2.), 0, 0, cos(pitch / 2.));
	for(int i = 0; i < 2; i++){
		Vec3d lturret_ofs(.005 * (i * 2 - 1), 0, -0.030);
		Vec3d direction = mat.dvp3(forward);
		Bullet *pz;
		pz = new Bullet(base, bulletlife(), 800.);
		w->addent(pz);
		pz->pos = mat.vp3(lturret_ofs);
		pz->velo = direction * bulletspeed() + this->velo;
		pz->rot = qrot;

		if(WarSpace *ws = *w) for(int j = 0; j < 3; j++){
			Vec3d pos;
			COLOR32 col = 0;
			pos[0] = .02 * (drseq(&w->rs) - .5);
			pos[1] = .02 * (drseq(&w->rs) - .5);
			pos[2] = .02 * (drseq(&w->rs) - .5);
			col |= COLOR32RGBA(rseq(&w->rs) % 32 + 127,0,0,0);
			col |= COLOR32RGBA(0,rseq(&w->rs) % 32 + 95,0,0);
			col |= COLOR32RGBA(0,0,rseq(&w->rs) % 32 + 80,0);
			col |= COLOR32RGBA(0,0,0,191);
			Vec3d smokevelo = direction * .02 * (drseq(&w->rs) + .5) + this->velo;
			smokevelo[0] += .02 * (drseq(&w->rs) - .5);
			smokevelo[1] += .02 * (drseq(&w->rs) - .5);
			smokevelo[2] += .02 * (drseq(&w->rs) - .5);
			AddTelineCallback3D(ws->tell, pos + pz->pos, smokevelo, .015, quat_u, Vec3d(0, 0, M_PI * (drseq(&w->rs) - .5)),
				vec3_000, smokedraw_swirl, (void*)col, TEL3_INVROTATE | TEL3_NOLINE, 1.5);
		}
		if(WarSpace *ws = *w) for(int j = 0; j < 15; j++){
			double triangle = drseq(&w->rs);
			triangle += drseq(&w->rs); // Triangular distributed random value
			Vec3d smokevelo = direction * .1 * triangle + this->velo;
			smokevelo[0] += .05 * (drseq(&w->rs) - .5);
			smokevelo[1] += .05 * (drseq(&w->rs) - .5);
			smokevelo[2] += .05 * (drseq(&w->rs) - .5);
			AddTelineCallback3D(ws->tell, pz->pos, smokevelo, .0004, quat_u, vec3_000,
				vec3_000, sparkdraw, NULL, TEL3_NOLINE, 0.5 + .25 * (drseq(&w->rs) - .5));
		}
	}
	this->cooldown += reloadtime();
	this->mf += .3;
	blowbackspeed += .05;
	ammo -= 2;
}

// prefer bigger targets
double LTurret::findtargetproc(const Entity *pb, const hardpoint_static *hp, const Entity *pt2){
	return pt2->hitradius() / .01;
}










LMissileTurret::LMissileTurret(){
}

LMissileTurret::LMissileTurret(Entity *abase, const hardpoint_static *hp) : st(abase, hp), deploy(0){
	ammo = 6;
}

LMissileTurret::~LMissileTurret(){
}

const char *LMissileTurret::classname()const{return "LMissileTurret";}
const unsigned LMissileTurret::classid = registerClass("LMissileTurret", Conster<LMissileTurret>);
double LMissileTurret::hitradius()const{return .03;}

const double LMissileTurret::bscale = .0001 / 2.;

int LMissileTurret::wantsFollowTarget()const{
	return cooldown < 2. * reloadtime();
}

void LMissileTurret::anim(double dt){
	st::anim(dt);
	if(target && cooldown < 2. * reloadtime()){
		deploy = approach(deploy, 1., dt, 0.);
	}
	else{
		deploy = approach(deploy, 0., dt, 0.);
		py[0] = approach(py[0], 0., dt, 0.);
		py[0] = rangein(approach(py[0] + M_PI, 0. + M_PI, MTURRETROTSPEED * dt, 2 * M_PI) - M_PI, mturret_range[0][0], mturret_range[0][1]);
//		py[1] = approach(py[1], 0., dt, 0.);
	}
}

void LMissileTurret::draw(wardraw_t *wd){
	// Viewing volume culling
	if(wd->vw->gc->cullFrustum(pos, .03))
		return;
	// Scale too small culling
	if(fabs(wd->vw->gc->scale(pos)) * .03 < 2)
		return;
	static suf_t *suf_turret = NULL, *suf_barrel = NULL;
	static suftex_t *pst_barrel;
	double scale;
	if(!suf_turret)
		suf_turret = CallLoadSUF("models/missile_launcher.bin");
	if(!suf_barrel){
		suf_barrel = CallLoadSUF("models/missile_launcher_barrel.bin");
		if(suf_barrel){
			CacheSUFMaterials(suf_barrel);
			pst_barrel = AllocSUFTex(suf_barrel);
		}
	}

/*	static const GLfloat rotaxis2[16] = {
		-1,0,0,0,
		0,1,0,0,
		0,0,-1,0,
		0,0,0,1,
	};*/

	glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_ENABLE_BIT);
	glPushMatrix();
	gldTranslate3dv(pos);
	gldMultQuat(rot);
	glRotated(deg_per_rad * this->py[1], 0., 1., 0.);
	glPushMatrix();
	gldScaled(bscale);
//	glMultMatrixf(rotaxis2);
	glScalef(-1,1,-1);
	if(suf_turret)
		DrawSUF(suf_turret, SUF_ATR, NULL);
	glPopMatrix();
	if(suf_barrel){
		const Vec3d pos = Vec3d(0, 200, 0) * deploy;
		Vec3d joint = Vec3d(0, 120, 60);
		gldScaled(bscale);
		gldTranslate3dv(pos + joint);
		glRotated(deg_per_rad * this->py[0], 1., 0., 0.);
		gldTranslate3dv(-joint);
//		glMultMatrixf(rotaxis2);
		glScalef(-1,1,-1);
		DecalDrawSUF(suf_barrel, SUF_ATR, NULL, pst_barrel, NULL, NULL);
	}
	glPopMatrix();
	glPopAttrib();
}

void LMissileTurret::drawtra(wardraw_t *wd){
	Entity *pb = base;
//	double bscale = 1;
	if(this->mf){
		struct random_sequence rs;
		Mat4d mat2, mat, rot;
		Vec3d const barrelpos = bscale * Vec3d(0, 200, 0) * deploy;
		Vec3d const joint = bscale * Vec3d(0, 120, 60);
		Vec3d pos, const muzzlepos = bscale * Vec3d(80. * (ammo % 3 - 1), (40. + 40. + 80. * (ammo / 3)), 0);
		init_rseq(&rs, (long)this ^ *(long*)&wd->vw->viewtime);
		this->transform(mat);
		mat2 = mat.roty(this->py[1]);
		mat2.translatein(barrelpos + joint);
		mat = mat2.rotx(this->py[0]);
		mat.translatein(-joint);
		pos = mat.vp3(muzzlepos);
		rot = mat;
		rot.vec3(3).clear();
//		drawmuzzleflash4(pos, rot, .005, wd->vw->irot, &rs, wd->vw->pos);

		static GLuint texname = 0;
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		if(!texname){
			suftexparam_t stp;
			stp.flags = STP_ENV | STP_MAGFIL | STP_MINFIL | STP_WRAP_S;
			stp.env = GL_MODULATE;
			stp.magfil = GL_LINEAR;
			stp.minfil = GL_LINEAR;
			stp.wraps = GL_CLAMP_TO_BORDER;
			texname = CallCacheBitmap5("muzzle.bmp", "muzzle.bmp", &stp, NULL, NULL);
		}
		glCallList(texname);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE); // Add blend
		float f = mf / .3 * 2, fi = 1. - mf / .3;
		glColor4f(f,f,f,1);
		gldTextureBeam(wd->vw->pos, pos, pos + rot.vp3(-vec3_001) * .050 * fi, .020 * fi);
		glPopAttrib();
	}
}

double LMissileTurret::bulletspeed()const{return 1.;}
float LMissileTurret::reloadtime()const{return .5;}

void LMissileTurret::tryshoot(){
	if(ammo <= 0)
		return;
	static const Vec3d forward(0., 0., -1.);
	Mat4d mat2;
	base->transform(mat2);
	mat2.translatein(hp->pos);
	Mat4d rot = hp->rot.tomat4();
	Mat4d mat = mat2 * rot;
	mat.translatein(0., .01, 0.);
	double yaw = this->py[1] + (drseq(&w->rs) - .5) * MTURRET_VARIANCE;
	double pitch = this->py[0] + (drseq(&w->rs) - .5) * MTURRET_VARIANCE;
	const Vec3d barrelpos = bscale * Vec3d(0, 200, 0) * deploy;
	const Vec3d joint = bscale * Vec3d(0, 120, 60);
	mat2 = mat.roty(yaw);
	mat2.translatein(joint + barrelpos);
	mat = mat2.rotx(pitch);
	mat.translatein(-joint);
	mat.translatein(0., -.01, 0.);
	Quatd qrot = base->rot * hp->rot * Quatd(0, sin(yaw / 2.), 0, cos(yaw / 2.)) * Quatd(sin(pitch / 2.), 0, 0, cos(pitch / 2.));
	ammo--;
	{
		Vec3d lturret_ofs = bscale * Vec3d(80. * (ammo % 3 - 1), (40. + 40. + 80. * (ammo / 3)), 0);
		Bullet *pz;
		pz = new Missile(base, 15., 500., target);
		w->addent(pz);
		pz->pos = mat.vp3(lturret_ofs);
		pz->velo = mat.dvp3(forward) * .1*bulletspeed() + this->velo;
		pz->rot = qrot;
	}
	targets[numof(targets) - ammo - 1] = target;
	if(!ammo){
		this->cooldown += 17.;
		ammo = 6;
	}
	else
		this->cooldown += reloadtime();
	this->mf += .3;
}

double LMissileTurret::findtargetproc(const Entity *pb, const hardpoint_static *hp, const Entity *pt2){
	double accumdamage = 0.;
	std::map<const Entity *, Missile *>::iterator it = Missile::targetmap.find(pt2);
	if(it != Missile::targetmap.end()) for(Missile *pm = it->second; pm; pm = pm->targetlist){
		accumdamage += pm->damage;
		if(pt2->health < accumdamage)
			return 0.;
	}
	return 1. / pt2->hitradius(); // precede small objects that conventional guns can hardly hit.
}

#if 0
const struct arms_static_info arms_static[num_armstype] = {
	{"N/A",				armc_none,			0,	0,		NULL,			NULL,	NULL,	0., 0., 0., 0., 0.},
	{"Hydra 70",		armc_launcher,		0,	16,		m261_draw,		NULL,	NULL,	0.2, 25., 2., 20., 5., 100.},
	{"Hellfire",		armc_launcher,		0,	4,		hellfire_draw,	NULL,	NULL,	1., 20., 45., 10., 25., 300.},
	{"AIM-9",			armc_missile,		0,	1,		aim9_draw,		NULL,	NULL,	4., 2., 20., 10., 84., 300.},
	{"Beam Cannon",		armc_launcher,		0,	10,		beamcannon_draw,NULL,	beamcannon_anim,	4., 120., 1., 1000., 200., 100.},
	{"Avenger",			armc_launcher,		0,	1000,	avenger_draw,	NULL,	avenger_anim,	0.1, 150., .5, 200., .1, 10.},
	{"LONGBOW Radar",	armc_radome,		0,	0,		NULL,			NULL,	NULL,	0., 50., 0., 200., 0.},
	{"Shield Dome",		armc_radome,		0,	0,		NULL,			NULL,	NULL,	0., 60., 0., 10000., 0.},
	{"S. Gun Turret",	armc_smallturret,	0,	100,	sturret_draw,	NULL,	sturret_anim,	1., 150., 0.5, 300., 12., 10.},
	{"VF-25 Gunpod",	armc_robothand,		0,	1000,	gunpod_draw,	NULL,	NULL,	.2, 150., 0.5, 600., 1., 10.},
	{"VF-25 Rifle",		armc_robothand,		0,	20,		gunpod_draw,	NULL,	NULL,	4., 200., 2, 1000., 20., 100.},
	{"M. Gun Turret",	armc_mediumturret,	0,	2000,	mturret_draw,	mturret_drawtra,	mturret_anim,	2., 2000., 20, 50000., 200., 80.},
	{"M. Beam Turret",	armc_mediumturret,	0,	100,	mturret_draw,	mturret_drawtra,	mturret_anim,	5., 2000., 20, 50000., 200., 80.},
	{"M. Mis. Turret",	armc_mediumturret,	0,	50,		mturret_draw,	mturret_drawtra,	mturret_anim,	5., 2000., 20, 50000., 200., 300.},
	{"M16A1 AR",		armc_infarm,		0,	20,		m16_draw,		NULL,	NULL,	.1, 4., .004, 2., .0001},
	{"Shotgun",			armc_infarm,		0,	8,		shotgun_draw,	NULL,	NULL,	1., 5., .015, 1.5, .0002},
	{"M40A1 Sniper",	armc_infarm,		0,	5,		m40_draw,		NULL,	NULL,	1.5, 6.57, .0095, 3., .00015},
	{"RPG-7",			armc_infarm,		0,	1,		rpg7_draw,		NULL,	NULL,	3., 7., 2.6, 1., .5},
	{"Mortar",			armc_infarm,		0,	1,		rpg7_draw,		NULL,	NULL,	2., 10., 4, 6., 1.5},
	{"PLG",				armc_infarm,		0,	1,		rpg7_draw,		NULL,	NULL,	2., 8., 4, 6., 1.5},
};



static GLfloat show_light_pos[4];

static void show_light_on(void){
	glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
	{
		int i;
		GLfloat mat_specular[] = {0., 0., 0., 1.}/*{ 1., 1., .1, 1.0 }*/;
		GLfloat mat_diffuse[] = { .5, .5, .5, 1.0 };
		GLfloat mat_ambient_color[] = { 0.25, 0.25, 0.25, 1.0 };
		GLfloat mat_shininess[] = { 50.0 };
		GLfloat color[] = {1., 1., 1., .8}, amb[] = {1., 1., 1., 1.};
		avec3_t pos;

		glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_color);
		glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
		glLightfv(GL_LIGHT0, GL_POSITION, show_light_pos);
		glLightfv(GL_LIGHT0, GL_SPECULAR, mat_specular);
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, color);
	}
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_NORMALIZE);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
}

static void show_light_off(void){
	glPopAttrib();
}

static void show_init_wardraw(wardraw_t *wd, Viewer *vw){
	wd->listLight = 0;
	wd->light_on = show_light_on;
	wd->light_off = show_light_off;
	VECCPY(wd->view, vw->pos);
	wd->light[0] = 1.;
	wd->light[1] = 1.;
	wd->light[2] = 1.;
	VECNORMIN(wd->light);
	wd->ambient = COLOR32RGBA(255,255,255,255);
	VECSCALE(wd->viewdir, vw->rot, -1.);
	wd->fov = 1.;
	wd->pgc = NULL;
	wd->vw = vw;
	wd->gametime = 0.;
	wd->rot = &mat4identity;
	MAT4CPY(wd->irot, &mat4identity);
	wd->irot_init = 1;
	wd->maprange = 1.;
}













static void armswar_think(warf_t *w, double dt){}
static void armswar_draw(warf_t *w, wardraw_t *wd){}
static int armswar_pointhit(warf_t *w, const avec3_t *pos, const avec3_t *velo, double dt, struct contact_info *ci){return 0;}
static void armswar_accel(warf_t *w, avec3_t *dst, const avec3_t *srcpos, const avec3_t *srcvelo){VECCPY(*dst, avec3_000);}
static double armswar_pressure(warf_t *w, const avec3_t *pos){return 1.;}
static double armswar_sonic_speed(warf_t *w, const avec3_t *pos){return .340;}
int WarOrientation(struct war_field*, amat3_t *dst, const avec3_t *pos);
double WarNearPlane(const warf_t *);
double WarFarPlane(const warf_t *);

static struct war_field_static armswar_static = {
	armswar_think,
	WarPostFrame,
	WarEndFrame,
	armswar_draw,
	armswar_pointhit,
	armswar_accel,
	armswar_pressure,
	armswar_sonic_speed,
	NULL,
	WarOrientation,
	WarNearPlane,
	WarFarPlane,
	NULL,
};
static warf_t armswar = {
	&armswar_static,
	NULL, /* pl */
	NULL, /* tl */
	NULL, /* bl */
	NULL, /* tell */
	NULL, /* gibs */
	NULL, /* tefl */
	NULL, /* map */
	NULL, /* wmd */
/*	NULL, *//* patriot_home */
	0.,
	NULL,
	0,
	{0, 0}
};



struct glwindowarms{
	glwindow st;
	entity_t *pt;
	double baseprice;
	double lastt;
	double fov;
	avec3_t focus;
	aquat_t rot;
	struct hardpoint_static *hp;
	int nhp;
	arms_t *arms, *retarms;
};

static int buttonid(struct glwindow *wnd, int mx, int my){
	return 320 <= mx && mx <= 380 && wnd->h - 72 <= my && my <= wnd->h - 60 ? 1
		: 400 <= mx && mx <= 460 && wnd->h - 72 <= my && my <= wnd->h - 60 ? 2 : 0;
}

static int ArmsShow(struct glwindow *wnd, int mx, int my, double gametime){
	struct glwindowarms *p = (struct glwindowarms *)wnd;
	wardraw_t wd;
	Viewer viewer;
	struct glcull glc;
	extern warf_t warf;
	double dt = p->lastt ? gametime - p->lastt : 0.;
	GLfloat light_position[4] = { 1., 1., 1., 0.0 };
/*	extern double g_left, g_right, g_bottom, g_top, g_near, g_far;*/
	double totalweight = 0., totalprice = 0., totalattack = 0.;
	int button = 0;
	int i, ind = wnd && 0 <= my && 300 <= mx && mx <= wnd->w ? (my) / 24 : -1, ammosel = wnd && 310 + 8 * 12 < mx;

	button = buttonid(wnd, mx, my);
	if(button)
		ind = -1;

	p->lastt = gametime;

	if(0 <= mx && mx < 300 && 0 <= my && my <= 300){
		glPushMatrix();
		glTranslated(wnd->x, wnd->y + 12, 0);
		glColor4ub(0,0,255,255);
		glBegin(GL_LINE_LOOP);
		glVertex2i(1, 1);
		glVertex2i(299, 1);
		glVertex2i(299, 299);
		glVertex2i(1, 299);
		glEnd();
		glPopMatrix();
	}

	MAT4IDENTITY(viewer.rot);
	quat2mat(viewer.rot, p->rot);
/*	MAT4ROTY(viewer.rot, mat4identity, gametime * .5);*/
	MAT4IDENTITY(viewer.irot);
	quat2imat(viewer.irot, p->rot);
/*	MAT4ROTY(viewer.irot, mat4identity, -gametime * .5);*/
	MAT4CPY(viewer.relrot, viewer.rot);
	MAT4CPY(viewer.relirot, viewer.irot);
	VECCPY(viewer.pos, &viewer.irot[8]);
	VECSCALEIN(viewer.pos, ((struct entity_private_static*)p->pt->vft)->hitradius * 3.);
	MAT4DVP3(show_light_pos, viewer.irot, light_position);
	VECNULL(viewer.velo);
	VECNULL(viewer.pyr);
	viewer.velolen = 0.;
	viewer.fov = 1.;
	viewer.cs = NULL;
	viewer.relative = 0;
	viewer.detail = 0;
	viewer.vp.w = viewer.vp.h = viewer.vp.m = 100;
	show_init_wardraw(&wd, &viewer);
	wd.gametime = gametime;
	wd.pgc = &glc;
	wd.w = NULL;
	VECNULL(p->pt->pos);
	VECNULL(p->pt->velo);
	VECNULL(p->pt->omg);
	VECNULL(p->pt->pyr);
	VECNULL(p->pt->rot);
	p->pt->rot[3] = 1.;
	p->pt->active = 1;

	if(wnd) for(i = 0; i < p->nhp; i++){
		char buf[128];
		i == ind && !ammosel ? glColor4ub(0,255,255,255) : glColor4ub(255,255,255,255);
		glRasterPos2d(wnd->x + 300, wnd->y + (i + 1) * 24);
		gldPutString(p->hp[i].name);
		glRasterPos2d(wnd->x + 310, wnd->y + (i + 1) * 24 + 12);
		if(arms_static[p->arms[i].type].maxammo)
			gldPutStringN(arms_static[p->arms[i].type].name, 15);
		else
			gldPutString(arms_static[p->arms[i].type].name);
		if(arms_static[p->arms[i].type].maxammo){
			i == ind && ammosel ? glColor4ub(0,255,255,255) : glColor4ub(255,255,255,255);
			glRasterPos2d(wnd->x + 310 + 8 * 15, wnd->y + (i + 1) * 24 + 12);
			sprintf(buf, "x %d", p->arms[i].ammo);
			gldPutString(buf);
		}
		totalweight += arms_static[p->arms[i].type].emptyweight + p->arms[i].ammo * arms_static[p->arms[i].type].ammoweight;
		totalprice += arms_static[p->arms[i].type].emptyprice + p->arms[i].ammo * arms_static[p->arms[i].type].ammoprice;
		totalattack += arms_static[p->arms[i].type].cooldown ? arms_static[p->arms[i].type].ammodamage / arms_static[p->arms[i].type].cooldown : 0.;
	}
	{
		char buf[128];
		if(button == 1)
			glColor4ub(0,255,255,255);
		else
			glColor4ub(255,255,255,255);
		glRasterPos2d(wnd->x + 320, wnd->y + wnd->h - 48);
		gldPutString("OK");
		if(button == 2)
			glColor4ub(0,255,255,255);
		else
			glColor4ub(255,255,255,255);
		glRasterPos2d(wnd->x + 400, wnd->y + wnd->h - 48);
		gldPutString("Cancel");

		glColor4ub(255,255,255,255);
		glwpos2d(wnd->x + 300, wnd->y + wnd->h - 36);
		glwprintf("Attack: %lg Points/sec", totalattack);
		sprintf(buf, "Weight: %lg kg", totalweight + p->pt->mass);
		glRasterPos2d(wnd->x + 300, wnd->y + wnd->h - 24);
		gldPutString(buf);
		if(totalprice + p->baseprice < 1e6)
			sprintf(buf, "Price: $ %lg k", totalprice + p->baseprice);
		else
			sprintf(buf, "Price: $ %lg M", (totalprice + p->baseprice) * 1e-3);
		glRasterPos2d(wnd->x + 300, wnd->y + wnd->h - 12);
		gldPutString(buf);
	}

	{
	amat4_t proj;
	avec3_t dst;
	double f;
	GLint vp[4];
	f = 1. - exp(-dt / .05);
	if(wnd){
		RECT rc;
		int mi = MIN(wnd->w, wnd->h - 12);
		GetClientRect(WindowFromDC(wglGetCurrentDC()), &rc);
		glGetIntegerv(GL_VIEWPORT, vp);
		glViewport(wnd->x, rc.bottom - rc.top - (wnd->y + wnd->h), mi, mi);
	}
	glClear(GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glGetDoublev(GL_PROJECTION_MATRIX, &proj);
	glLoadIdentity();
	{
		double dst;
		if(wnd && 0 <= ind && ind < p->nhp)
			dst = .1;
		else
			dst = .5;
		p->fov += (dst - p->fov) * f;
		glFrustum (-p->fov * 1e-3, p->fov * 1e-3, -p->fov * 1e-3, p->fov * 1e-3, 1.5 * 1e-3, 10.);
	}
	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
	glLoadMatrixd(viewer.rot);
	glcullInit(&glc, 1., &viewer.pos, &viewer.irot, -10., 10.);
	VECNULL(dst);
	if(wnd && 0 <= ind && ind < p->nhp){
		VECADDIN(dst, p->hp[ind].pos);
		/*gldTranslaten3dv(arms_s[ind].pos);*/
	}
	{
		avec3_t delta;
		VECSUB(delta, dst, p->focus);
		VECSCALEIN(delta, f);
		VECADDIN(p->focus, delta);
	}
	VECADDIN(viewer.pos, p->focus);
	gldTranslaten3dv(viewer.pos);
	wd.light_on(&wd);
	((struct entity_private_static*)p->pt->vft)->draw(p->pt, &wd);
	wd.light_off(&wd);
	if(wnd && 0 <= ind && ind < p->nhp) for(i = 0; i < p->nhp; i++){
		i == ind ? glColor4ub(0,255,255,255) : glColor4ub(255,255,255,255);
		glRasterPos3dv(p->hp[i].pos);
		gldPutString(p->hp[i].name);
	}
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(proj);
	glMatrixMode(GL_MODELVIEW);

	if(wnd){
		glViewport(vp[0], vp[1], vp[2], vp[3]);
	}
	}

	return 0;
}

static void ArmsShowDestruct(struct glwindow *wnd){
	struct glwindowarms *p = (struct glwindowarms *)wnd;
	if(p->pt)
		free(p->pt);
}

static int ArmsShowMouse(struct glwindow *wnd, int mbutton, int state, int mx, int my){
	struct glwindowarms *p = (struct glwindowarms *)wnd;
	int i, ind = wnd && 300 <= mx && mx <= wnd->w ? (my) / 24 : -1, ammosel = wnd && 310 + 8 * 12 < mx;
	int button;

	button = buttonid(wnd, mx, my);
	if(button)
		ind = -1;

	if(state == GLUT_UP && button == 1){
		memcpy(p->retarms, p->arms, p->nhp * sizeof *p->retarms);
		if(!(wnd->flags & GLW_PINNED))
			wnd->flags |= GLW_TODELETE;
	}
	else if(state == GLUT_UP && button == 2){
		wnd->flags |= GLW_TODELETE;
	}
	else if(state == GLUT_KEEP_DOWN && ind == -1 && 12 < my){
		aquat_t q, q2, qr;
		q[0] = (150 - (my)) / 150. * .5;
		q[1] = q[2] = 0.;
		q[3] = sqrt(1. - q[0] * q[0]);
		q2[0] = 0.;
		q2[1] = -sin(((150 - (mx)) / 150. * M_PI) / 2.);
		q2[2] = 0.;
		q2[3] = sqrt(1. - q2[1] * q2[1]);
		QUATMUL(qr, q, q2);
		QUATZERO(q);
		q[1] = 1.;
		QUATMUL(q2, q, qr);
		QUATCPY(p->rot, q2);
	}
	if(state != GLUT_UP)
		return 1;
	if(wnd && 0 <= ind && ind < p->nhp) if(ammosel){
		if(arms_static[p->arms[ind].type].maxammo)
			p->arms[ind].ammo = (p->arms[ind].ammo + (mbutton ? -1 : 1) + arms_static[p->arms[ind].type].maxammo + 1) % (arms_static[p->arms[ind].type].maxammo + 1);
	}
	else{
		do{
			p->arms[ind].type = (p->arms[ind].type + (mbutton ? -1 : 1) + num_armstype) % num_armstype;
		} while(!(p->hp[ind].clsmask[arms_static[p->arms[ind].type].cls / (sizeof(unsigned) * CHAR_BIT)] & (1 << arms_static[p->arms[ind].type].cls % (sizeof(unsigned) * CHAR_BIT))));
		p->arms[ind].ammo = arms_static[p->arms[ind].type].maxammo;
	}
	return 1;
}

glwindow *ArmsShowWindow(entity_t *creator(warf_t *), double baseprice, size_t armsoffset, arms_t *retarms, struct hardpoint_static *hardpoints, int count){
	glwindow *ret;
	glwindow **ppwnd;
	struct glwindowarms *p;
	int i;
	static const char *windowtitle = "Arms Fitting";
	for(ppwnd = &glwlist; *ppwnd; ppwnd = &(*ppwnd)->next) if((*ppwnd)->title == windowtitle){
		glwActivate(ppwnd);
		return 0;
	}
	p = malloc(sizeof *p);
	ret = &p->st;
	ret->x = 50;
	ret->y = 50;
	ret->w = 500;
	ret->h = 312;
	ret->title = windowtitle;
	ret->flags = GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE;
	ret->modal = NULL;
	ret->draw = ArmsShow;
	ret->mouse = ArmsShowMouse;
	ret->key = NULL;
	ret->destruct = ArmsShowDestruct;
	glwActivate(glwAppend(ret));
/*	memset(p->arms, 0, sizeof p->arms);*/
	p->pt = creator(&armswar);
	p->baseprice = baseprice;
	p->arms = (arms_t*)&((char*)p->pt)[armsoffset];
	p->retarms = retarms;
	memcpy(p->arms, retarms, count * sizeof *retarms);
	p->lastt = 0.;
	p->fov = .5;
	VECNULL(p->focus);
	QUATZERO(p->rot);
	p->rot[1] = 1.;
	p->hp = hardpoints;
	p->nhp = count;
/*	memcpy(p->arms, apachearms_def, sizeof apachearms_def);*/
	return ret;
}





#endif



hardpoint_static *hardpoint_static::load(const char *fname, int &num){
	char buf[512];
	UniformLoader *ss;
	ss = ULzopen("rc.zip", fname, UL_TRYFILE);
	if(!ss){
		num = 0;
		return NULL;
	}
	hardpoint_static *ret = NULL;
	struct varlist vl = {0, NULL, calc_mathvars()};
	num = 0;
	while(ULfgets(buf, sizeof buf, ss)){
		int argc, c = 0;
		char *argv[16], *post;
		argc = argtok(argv, buf, &post, numof(argv));
		if(argc < 1)
			continue;

		// definition of identifier in expressions.
		if(argc == 3 && !strcmp(argv[0], "define")){
			struct var *v;
			vl.l = (var*)realloc(vl.l, ++vl.c * sizeof *vl.l);
			v = &vl.l[vl.c-1];
			v->name = (char*)malloc(strlen(argv[1]) + 1);
			strcpy(v->name, argv[1]);
			v->type = var::CALC_D;
			v->value.d = 2 < argc ? calc3(&argv[2], &vl, NULL) : 0.;
			continue;
		}

		hardpoint_static *a = new hardpoint_static[num+1];
		for(int i = 0; i < num; i++)
			a[i] = ret[i];
		if(ret)
			delete[] ret;
		ret = a;

		// tokenize with ,
		for(int i = 0; i < argc; i++) if(char *p = strchr(argv[i], ','))
			*p = '\0';

		hardpoint_static &h = ret[num++];
		for(int i = 0; i < 3; i++)
			h.pos[i] = c < argc ? calc3(&argv[c++], &vl, NULL) : 0;
		for(int i = 0; i < 4; i++)
			h.rot[i] = c < argc ? calc3(&argv[c++], &vl, NULL) : i == 3;
		h.name = new char[c < argc ? strlen(argv[c]) + 1 : 2];
		strcpy(const_cast<char*>(h.name), c < argc ? argv[c++] : "?");
		h.flagmask = c < argc ? atoi(argv[c++]) : 0;
	}
	for(int i = 0; i < vl.c; i++)
		free(vl.l[i].name);
	free(vl.l);
	ULclose(ss);
	return ret;
}
