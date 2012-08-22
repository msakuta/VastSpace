/** \file
 * \brief Implementation of Soldier class.
 */
#ifdef _WIN32
#include <winsock2.h>
#endif
#include "Soldier.h"
#include "Player.h"
#include "Bullet.h"
#include "arms.h"
#include "cmd.h"
#include "motion.h"
#include "Game.h"
#include "btadapt.h"
#include "ClientMessage.h"
#include "Server.h"
#include "Application.h"
#include "EntityCommand.h"
#include "judge.h"
#include "libmotion.h"

extern "C"{
#include <clib/mathdef.h>
}

#include <sstream>

//#include "BulletCollision/NarrowPhaseCollision/btGjkConvexCast.h"

//#include "sufsrc/roughbody.h"
//#include "sufsrc/bike.h"
//#include "sufsrc/shotshell1.h"
//#include "sufsrc/m16.h"


#define BMPSRC_LINKAGE static
//#include "bmpsrc/face.h"
//#include "bmpsrc/infantry.h"

#define putstring(a) gldPutString(a)

#define TURRETROTSPEED (2. * M_PI)
#define BULLETSPEED .5
#define INFANTRY_WALK_PHASE_SPEED (M_PI * 1.3)


Entity::EntityRegister<Soldier> Soldier::entityRegister("Soldier");
const unsigned Soldier::classid = registerClass("Soldier", Conster<Soldier>);
const char *Soldier::classname()const{return "Soldier";}

double Soldier::modelScale = 2e-6;
double Soldier::hitRadius = 0.002;
double Soldier::defaultMass = 60; // kilograms
double Soldier::hookSpeed = 0.2;
double Soldier::hookRange = 0.2;
double Soldier::hookPullAccel = 0.05;
double Soldier::hookStopRange = 0.025;
Autonomous::ManeuverParams Soldier::maneuverParams = {
	0.01, // accel
	0.01, // maxspeed
	M_PI * 0.1, // angleaccel
	M_PI * 0.1, // maxanglespeed
	1, // capacity
	1, // capacitor_gen
};
GLuint Soldier::overlayDisp = 0;
double Soldier::muzzleFlashRadius[2] = {.0004, .00025};
Vec3d Soldier::muzzleFlashOffset[2] = {Vec3d(-0.00080, 0.00020, 0.0), Vec3d(-0.00110, 0.00020, 0.0)};


HardPointList soldierHP;

#if 0
static void infantry_cockpitview(entity_t *pt, warf_t *, double (*pos)[3], int *);
static void infantry_anim(entity_t *pt, warf_t *w, double dt);
static void infantry_draw(entity_t *pt, wardraw_t *wd);
static void infantry_drawtra(entity_t *pt, wardraw_t *wd);
static void infantry_bullethit(entity_t *pt, warf_t *w, const bullet_t *pb);
static int infantry_takedamage(entity_t *pt, double damage, warf_t *w, int hitpart);
static void infantry_gib_draw(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv);
static void infantry_postframe(entity_t *pt);
static void infantry_control(entity_t *, warf_t *, input_t *inputs, double dt);
static int infantry_getrot(struct entity*, warf_t*, double (*)[4]);
static const char *infantry_idname(entity_t *pt){return "infantry";}
static const char *infantry_classname(entity_t *pt){return "Infantry";}
static void infantry_drawHUD(entity_t *pt, warf_t *wf, const double irot[16], void (*gdraw)(void));
static unsigned infantry_analog_mask(const entity_t *pt, const warf_t *wf){return 3;}
static int infantry_tracehit(struct entity *pt, warf_t *w, const double src[3], const double dir[3], double rad, double dt, double *ret, double (*retp)[3], double (*ret_normal)[3]);

static struct entity_private_static infantry_s = {
	{
		infantry_drawHUD,
		infantry_cockpitview,
		infantry_control,
		NULL, /* destruct */
		infantry_getrot,
		NULL, /* getrotq */
		NULL, /* drawCockpit */
		NULL, /* is_warping */
		NULL, /* warp_dest */
		infantry_idname,
		infantry_classname,
		NULL, /* start_control */
		NULL, /* end_control */
		infantry_analog_mask,
	},
	infantry_anim,
	infantry_draw,
	infantry_drawtra,
	infantry_takedamage,
	infantry_gib_draw,
	infantry_postframe,
	falsefunc,
	0., -M_PI / 2., M_PI / 2., /* turretrange, barrelmin, barrelmax */
	NULL, NULL, NULL, NULL, /* suf */
	0, /* reuse */
	BULLETSPEED, /* bulletspeed */
	0.002, /* getHitRadius */
	1./180000, /* sufscale */
	0, 1, /* hitsuf, altaxis */
	NULL, /* bullethole */
	{0.},
	infantry_bullethit,
	infantry_tracehit, /* tracehit */
	{NULL}, /* hitmdl */
	infantry_draw,
};
#endif


void Soldier::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << cooldown2;
	sc.o << arms[0];
	sc.o << arms[1];
	sc.o << hookpos;
	sc.o << hookvelo;
	sc.o << hookedEntity;
	sc.o << hookshot;
	sc.o << hooked;
	sc.o << hookretract;
}

void Soldier::unserialize(UnserializeContext &usc){
	st::unserialize(usc);
	usc.i >> cooldown2;
	usc.i >> arms[0];
	usc.i >> arms[1];
	usc.i >> hookpos;
	usc.i >> hookvelo;
	usc.i >> hookedEntity;
	usc.i >> hookshot;
	usc.i >> hooked;
	usc.i >> hookretract;
}

DERIVE_COMMAND_EXT(SwitchWeaponCommand, SerializableCommand);

IMPLEMENT_COMMAND(SwitchWeaponCommand, "SwitchWeapon")



double Soldier::getHitRadius()const{
	return hitRadius;
}

double Soldier::maxhealth()const{
	return 5.;
}

bool Soldier::isTargettable()const{
	return true;
}

bool Soldier::isSelectable()const{
	return true;
}

#define STATE_STANDING 0
#define STATE_CROUCH 1
#define STATE_PRONE 2

#define F(a) (1<<(a))

#ifdef DEDICATED
void Soldier::draw(WarDraw *){}
void Soldier::drawtra(WarDraw *){}
void Soldier::drawHUD(WarDraw *){}
void Soldier::drawOverlay(WarDraw *){}
void Soldier::hookHitEffect(const otjEnumHitSphereParam &param){}
void M16::draw(WarDraw *){}
void M40::draw(WarDraw *){}
#endif

/*static const struct hardpoint_static infantry_hardpoints[2] = {
	{{-.00015, .001, -.0005}, {0,0,0,1}, "Right Hand", 0, F(armc_none) | F(armc_infarm)},
	{{-.000, .001, .0005}, {0,0,0,1}, "Back", 0, F(armc_none) | F(armc_infarm)},
};

static arms_t infantry_arms[numof(infantry_hardpoints)] = {
	{arms_m16rifle, 20},
	{arms_shotgun, 8},
};*/

int infantry_random_weapons = 0;


Soldier::Soldier(Game *g) : st(g){
	arms[0] = arms[1] = NULL;
}

Soldier::Soldier(WarField *w) : st(w){
	init();
}

void Soldier::init(){
	static bool initialized = false;
	if(!initialized){
		sq_init(_SC("models/Soldier.nut"),
			ModelScaleProcess(modelScale) <<=
			SingleDoubleProcess(hitRadius, "hitRadius", false) <<=
			MassProcess(defaultMass) <<=
			SingleDoubleProcess(hookSpeed, "hookSpeed", false) <<=
			SingleDoubleProcess(hookRange, "hookRange", false) <<=
			SingleDoubleProcess(hookPullAccel, "hookPullAccel", false) <<=
			SingleDoubleProcess(hookStopRange, "hookStopRange", false) <<=
			ManeuverParamsProcess(maneuverParams) <<=
			HardPointProcess(soldierHP) <<=
			DrawOverlayProcess(overlayDisp) <<=
			SingleDoubleProcess(muzzleFlashRadius[0], "muzzleFlashRadius1", false) <<=
			SingleDoubleProcess(muzzleFlashRadius[1], "muzzleFlashRadius2", false) <<=
			Vec3dProcess(muzzleFlashOffset[0], "muzzleFlashOffset1", false) <<=
			Vec3dProcess(muzzleFlashOffset[1], "muzzleFlashOffset2", false)
			);
		initialized = true;
	}

	mass = defaultMass;
	health = maxhealth();
	swapphase = 0.;
	pitch = 0.;
	reloadphase = 0.;
	damagephase = 0.;
	kick[0] = kick[1] = 0.;
	kickvelo[0] = kickvelo[1] = 0.;
	state = STATE_STANDING;
	reloading = 0;
	aiming = 0;
	muzzle = 0;
	arms[0] = new M16(this, soldierHP[0]);
	arms[1] = new M40(this, soldierHP[1]);
	if(w) for(int i = 0; i < numof(arms); i++) if(arms[i])
		w->addent(arms[i]);
	hookshot = false;
	hooked = false;
	hookretract = false;
//	infantry_s.hitmdl.suf = NULL/*&suf_roughbody*/;
//	MAT4IDENTITY(infantry_s.hitmdl.trans);
//	MAT4SCALE(infantry_s.hitmdl.trans, infantry_s.sufscale, infantry_s.sufscale, infantry_s.sufscale);
//	infantry_s.hitmdl.valid = 0;
/*	if(infantry_random_weapons){
		int i, n = 0, k;
		for(i = 0; i < num_armstype; i++) if(arms_static[i].cls == armc_infarm)
			n++;
		k = rseq(&w->rs) % n;
		for(i = 0; i < num_armstype; i++) if(arms_static[i].cls == armc_infarm && !k--){
			ret->arms[0].type = i;
			ret->arms[0].ammo = arms_static[i].maxammo;
			break;
		}
		k = rseq(&w->rs) % (n-1);
		for(i = 0; i < num_armstype; i++) if(arms_static[i].cls == armc_infarm && ret->arms[0].type != i && !k--){
			ret->arms[1].type = i;
			ret->arms[1].ammo = arms_static[i].maxammo;
			break;
		}
	}
	else
		memcpy(ret->arms, infantry_arms, sizeof ret->arms);*/
/*	sufmodel_normalize(&ret->mdl);*/

#if 0
	/* calculate cost */
	if(0 <= race && race < numof(w->races)){
		int i;
		for(i = 0; i < 2; i++) if(ret->arms[i].type != arms_none){
			w->races[race].money -= arms_static[ret->arms[i].type].emptyprice + arms_static[ret->arms[i].type].ammoprice * ret->arms[i].ammo;
/*			ret->st.mass += arms_static[ret->arms[i].type].emptyweight;*/
		}
	}
#endif
}

void Soldier::setPosition(const Vec3d *pos, const Quatd *rot, const Vec3d *velo, const Vec3d *avelo){
	if(bbody){
		btTransform worldTransform = bbody->getWorldTransform();
		if(pos)
			worldTransform.setOrigin(btvc(*pos));
		if(rot)
			worldTransform.setRotation(btqc(*rot));
		bbody->setWorldTransform(worldTransform);
	}
	else
		st::setPosition(pos, rot, velo, avelo);
}

void Soldier::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	const Soldier *p = this;
	static const Vec3d ofs[2] = {Vec3d(.003, .001, -.002), Vec3d(.00075, .002, .003),};
	Mat4d mat, mat2;
	seatid = (seatid + 4) % 4;
	rot = this->rot;
	transform(mat);
	if(seatid == 1){
		pos = mat.vp3(ofs[0]);
		rot *= Quatd::rotation(M_PI / 2., 0, 1, 0);
	}
	else if(seatid == 2){
		pos = mat.vp3(ofs[1]);
	}
	else{
		mat4translate(mat, 0., p->state == STATE_STANDING ? .0016 : .0005, 0.); /* eye position */
		pos = mat.vec3(3);
		if(seatid == 3){
			WarField::EntityList::iterator it;
			for(it = w->bl.begin(); it != w->bl.end(); ++it){
				Entity *pb = *it;
				if(pb && pb->getOwner() == this){
					pos = pb->pos;
					break;
				}
			}
		}
	}
}

#if 0
void small_brass_draw(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	double scale = pv ? .00000045 : .000000225;
	const double size = scale * 100.;
	double pixels;
	pixels = size * glcullScale(&pl->pos, dd->pgc);
	if(pixels < 2)
		return;
	if(dd->pgc && glcullFrustum(pl->pos, size, dd->pgc))
		return;
	glPushMatrix();
	glEnable(GL_CULL_FACE);
	glTranslated(pl->pos[0], pl->pos[1], pl->pos[2]);
	if(pv){
		static suf_t *shotshell1 = NULL;
		if(!shotshell1)
			shotshell1 = CallLoadSUF("shotshell1.bin")/*LZUC(lzw_shotshell1)*/;
		gldScaled(scale);
		gldMultQuat(pl->rot);
		DrawSUF(shotshell1, SUF_ATR, &g_gldcache);
	}
	else{
		gldScaled(scale);
		gldMultQuat(pl->rot);
	/*	glRotated(-pl->pyr[1] * 360 / 2. / M_PI, 0., 1., 0.);
		glRotated(pl->pyr[0] * 360 / 2. / M_PI, 1., 0., 0.);*/
		if(pixels < 10)
			DrawSUF(gsuf_brass0, SUF_ATR, &g_gldcache);
		else if(pixels < 30)
			DrawSUF(gsuf_brass1, SUF_ATR, &g_gldcache);
		else
			DrawSUF(gsuf_brass2, SUF_ATR, &g_gldcache);
	}
	glDisable(GL_CULL_FACE);
	glPopMatrix();
}
#endif

int Soldier::shoot_infgun(double phi0, double theta0, double v, double damage, double variance, double t, Mat4d &gunmat){
	Soldier *p = this;
	Bullet *pb;
	static const avec3_t pos0 = {0., 0., -.0007}, nh0 = {0., 0., -1}, xh0 = {1., 0., 0.};
	avec3_t xh;
	double phi, theta;
	double hei = (p->state == STATE_PRONE ? .0005 / .0016 : 1.) * 220. * modelScale;

	phi = -phi0 + (drseq(&w->rs) - .5) * variance;
	phi += (drseq(&w->rs) - .5) * variance;
	theta = theta0 + (drseq(&w->rs) - .5) * variance;
	theta += (drseq(&w->rs) - .5) * variance;
	Mat4d mat = mat4_u;
	mat.vec3(3) = this->pos;
	Mat4d mat2 = mat.roty(phi);
	gunmat = mat2.rotx(theta);
/*	MAT4VP3(pos, *rot, *ofs);*/
/*	if(p->arms[0].type == arms_rpg7){
		pb = add_rpg7(w, pt->pos);
		pb->damage = damage;
		pb->owner = pt;
	}
	else
		pb = (p->arms[0].type == arms_plg ? BeamNew : p->arms[0].type == arms_mortar ? MortarHeadNew : p->arms[0].type == arms_m16rifle && p->arms[0].ammo < 5 ? TracerBulletNew : p->arms[0].type == arms_shotgun ? ShotgunBulletNew : NormalBulletNew)(w, pt, damage);*/
	pb = new Bullet(this, 1., damage);
	pb->velo = this->velo;
	Vec3d nh = gunmat.dvp3(nh0);
#if 0
	if(p->arms[0].type == arms_rpg7){
		avec3_t nnh;
		nh[1] += .08;
		VECSCALE(nnh, nh, -1);
		quatdirection(((struct hydra*)pb)->rot, nnh);
/*		QUATCNJIN(((struct hydra*)pb)->rot);*/
	}
	else{
		pb->mass = .01;
	}
#endif
	pb->velo += nh * v;
	Vec3d zh(
		- .0015 * sin(phi),
		0.,
		- .0015 * cos(phi)
	);
	VECADDIN(&gunmat[12], zh);
	gunmat[13] += hei;
	pb->pos = this->pos + zh;
	pb->pos[1] += hei;
//	pb->life = p->arms[0].type == arms_mortar ? 60. : p->arms[0].type == arms_rpg7 ? 5. : 2.;
	pb->life = 5.;
	pb->anim(t);
/*	VECADD(pb->pos, pt->pos, pos);*/


/*	pb = BulletNew(w, pt, damage);
	{
		double phi = phi0 + (drseq(&w->rs) - .5) * variance;
		double theta = theta0 + (drseq(&w->rs) - .5) * variance;

		phi += (drseq(&w->rs) - .5) * variance;
		theta += (drseq(&w->rs) - .5) * variance;
		VECCPY(pb->velo, pt->velo);
		pb->velo[0] +=  v * sin(phi) * cos(theta);
		pb->velo[1] +=  v * sin(theta);
		pb->velo[2] += -v * cos(phi) * cos(theta);
		pb->pos[0] = pt->pos[0] + .0015 * sin(phi);
		pb->pos[1] = pt->pos[1] +  0.0011;
		pb->pos[2] = pt->pos[2] - .0015 * cos(phi);
	}*/
	return 1;
}

#if 0
static int infantry_brass(WarField *w, Entity *pt, int shotshell, double t, Mat4d mat){
	avec3_t pos;
	static const avec3_t xh0 = {1., 0., 0.}, zh0 = {0., 0., -.0015};
	avec3_t nh, xh, zh;
	double hei = (((Soldier*)pt)->state == STATE_PRONE ? .0005 / .0016 : 1.) * 220. * infantry_s.sufscale;

	if(w->gibs){
		double velo[3], pyr[3], omg[3];
		mat4dvp3(zh, mat, zh0);
		VECCPY(pos, pt->pos);
		VECSADD(pos, zh, .0005 / .0015);
		pos[1] += hei;
		mat4dvp3(xh, mat, xh0);
		velo[0] = pt->velo[0] + (drseq(&w->rs) + .5) * .002 * xh[0];
		velo[1] = pt->velo[1] + .001 + drseq(&w->rs) * .001;
		velo[2] = pt->velo[2] + (drseq(&w->rs) + .5) * .002 * xh[2];
		VECCPY(pyr, pt->pyr);
		VECSADD(pos, velo, t);
		pyr[0] = M_PI / 4.;
		omg[0] = .5 * 2 * M_PI * (drseq(&w->rs) - .5);
		omg[1] = .5 * 2 * M_PI * (drseq(&w->rs) - .5);
		omg[2] = 3. * 2 * M_PI * (drseq(&w->rs) - .5);
		AddTelineCallback3D(w->gibs, pos, velo, shotshell ? .02 : 0.01, pyr, omg, w->gravity, small_brass_draw, (void*)shotshell, TEL3_NOLINE | TEL3_REFLECT, 1.5 + drseq(&w->rs));
	}
	return 1;
}
#endif


void Soldier::swapWeapon(){
	if(cooldown2 <= 0.){
		Firearm *tmp = arms[1];
		arms[1] = arms[0];
		arms[0] = tmp;
		cooldown2 = 2.;
		swapphase = 2.;
	}
}

void Soldier::reload(){
	if(cooldown2 != 0.)
		return;
/*	if(arms[0].type != arms_shotgun){
		arms[0].ammo = arms_static[p->arms[0].type].maxammo;
		cooldown2 += 2.5 + drseq(&w->rs) * 1.;
	}
	else*/{
		cooldown2 += 2.5 / 3.;
		reloading = 1;
	}
	reloadphase = 2.5;
	reloading = 1;
//	if(arms[0].type != arms_mortar)
//		aiming = 0;
}

extern struct player *ppl;

/* explicit reloading */
/*void cmd_reload(int argc, const char *argv[]){
	if(ppl->control && ppl->control->vft == &infantry_s){
		infantry_t *p = (infantry_t*)ppl->control;
		infantry_reload(p, ppl->cs->w);
	}
}*/

#ifndef DEDICATED
typedef struct wavecache{
	const char fname[MAX_PATH];
	WAVEFORMATEX format;
	WAVEHDR head;
	struct wavecache *hi, *lo;
} wavc_t;
#endif

double g_recoil_kick_factor = 10.;

//static ysdnm_t *dnm = NULL;

//void quat2pyr(const aquat_t quat, avec3_t euler);
//void find_enemy_logic(entity_t *pt, warf_t *w);
static void bloodsmoke(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv);

void Soldier::anim(double dt){
//	struct entity_private_static *vft = (struct entity_private_static*)pt->vft;	
	Soldier *p = this;
	double af, sdist;
	int i = inputs.press, ic = inputs.change;
	static int init = 0;
	avec3_t normal;
	int x = i & PL_D ? 1 : i & PL_A ? -1 : 0,
		y = i & PL_Q ? 1 : i & PL_Z ? -1 : 0,
		z = i & PL_W ? -1 : i & PL_S ? 1 : 0;
	double h = 0.;

	if(!init){
		init = 1;
		CvarAdd("g_recoil_kick_factor", &g_recoil_kick_factor, cvar_double);
	}

	if(game->isServer() && health <= 0. && 120. < walkphase){
		delete this;
		return;
	}

	WarSpace *ws = *w;

	/* takes so much damage that body tears apart */
	if(health < -50.){
/*		if(w->gibs && dnm){
			int i, n, base;
			struct entity_private_static *vft = (struct entity_private_static*)pt->vft;
			n = dnm->np;
			for(i = 0; i < n; i++){
				double pos[3], velo[3], omg[3];
				quatrot(pos, pt->rot, avec3_010);
				VECSCALEIN(pos, .001);
				VECADDIN(pos, pt->pos);
				velo[0] = pt->velo[0] + (drseq(&w->rs) - .5) * .008;
				velo[1] = pt->velo[1] + (drseq(&w->rs) - .5) * .008;
				velo[2] = pt->velo[2] + (drseq(&w->rs) - .5) * .008;
				omg[0] = 1. * 2 * M_PI * (drseq(&w->rs) - .5);
				omg[1] = 1. * 2 * M_PI * (drseq(&w->rs) - .5);
				omg[2] = 1. * 2 * M_PI * (drseq(&w->rs) - .5);
				AddTelineCallback3D(w->gibs, pos, velo, 0.01, pt->rot, omg, NULL, vft->gib_draw, (void*)i, TEL3_NOLINE | TEL3_QUAT | TEL3_REFLECT | TEL3_HITFUNC | TEL3_ACCELFUNC, 5.5 + drseq(&w->rs));
			}
		}*/

		/* create additional gore */
/*		tent3d_line_list *tell = w->getTeline3d();
		if(tell) for(i = 0; i < 8; i++){
			double pos[3], velo[3];
			VECCPY(pos, this->pos);
			VECCPY(velo, this->velo);
			pos[0] += (drseq(&w->rs) - .5) * .001;
			pos[1] += (drseq(&w->rs)) * .0015;
			pos[2] += (drseq(&w->rs) - .5) * .001;
			velo[0] += (drseq(&w->rs) - .5) * .005;
			velo[1] += (drseq(&w->rs)) * .005;
			velo[2] += (drseq(&w->rs) - .5) * .005;
			AddTelineCallback3D(tell, pos, velo, .0005, NULL, NULL, vec3_000, bloodsmoke, w->irot, 0, .5);
		}*/
		if(game->isServer())
			delete this;
	}

//	h = warmapheight(w->map, pt->pos[0], pt->pos[2], &normal);


#if 0
	if(!pt->active){
		pt->health = 5.;
		pt->pos[0] = (drseq(&w->rs) - .5) * .1;
		pt->pos[2] = pt->race * .4 - .2 + (drseq(&w->rs) - .5) * .1;
		pt->pos[1] = warmapheight(w->map, pt->pos[0], pt->pos[2], NULL);
		pt->pyr[1] = drseq(&w->rs) * M_PI * 2.;
		pt->inputs.press = 0;
		pt->enemy = NULL;
		pt->weapon = 0;
		p->swapphase = 0.;
/*		p->ammo = 25;*/
		memcpy(p->arms, infantry_arms, sizeof p->arms);
		p->muzzle = 0;
		pt->active = 1;
		return;
	}
#else
//	if(!pt->active)
//		return;
#endif

	if(health <= 0.){
		i = inputs.press = inputs.change = 0;
	}

#if 0
	/* be runned over by a vehicle */
	if(0. < health){
		entity_t *pt2;
		for(pt2 = w->tl; pt2; pt2 = pt2->next) if(pt2->active && ((struct entity_private_static*)pt2->vft)->anim == tank_anim && VECSDIST(pt->pos, pt2->pos) < .002 * .002){
			pt->health = 0.;
			if(pt2->race != pt->race){
				pt2->kills++;
				if(pt2->race < numof(w->races))
					w->races[pt2->race].kills++;
			}
			else{ /* teamkill penalty! */
				pt2->kills--;
				if(pt2->race < numof(w->races))
					w->races[pt2->race].kills++;
			}
			if(pt2->enemy == pt)
				pt2->enemy = NULL;
			pt->deaths++;
			return;
		}
	}
#endif

#if 0
	if(!controller && 0 < health){
		Vec3d epos;

		/* find enemy logic */
		if(!enemy){
			findEnemy();
		}

		if(enemy && enemy->health <= 0.)
			enemy = NULL;

		if(arms[0]->ammo <= 0 && cooldown2 == 0.){
			reload();
		}

		/* estimating enemy position and wheeling towards enemy */
		if(enemy){
//			double bulletspeed = p->arms[0].type == arms_mortar ? .05 : p->arms[0].type == arms_m40rifle ? .770 : p->arms[0].type == arms_shotgun ? BULLETSPEED * .75 : BULLETSPEED;
			double bulletspeed = BULLETSPEED;
			double phi;
			double sd;
			double desired[2];
			avec3_t pos0 = {0, .0015, 0};
			int shootable = 0;
/*			if(((struct entity_private_static*)pt->enemy->vft)->flying(pt->enemy))
				VECCPY(epos, pt->enemy->pos);
			else
				estimate_pos(&epos, pt->enemy->pos, pt->enemy->velo, pt->pos, pt->velo, &sgravity, bulletspeed, w->map, w, ((struct entity_private_static*)pt->enemy->vft)->flying(pt->enemy));*/
			epos = enemy->pos;
			if(enemy->classname() == this->classname() && ((Soldier*)static_cast<Entity*>(enemy))->state == STATE_PRONE)
				epos -= pos0;
			if(p->state == STATE_PRONE)
				epos += pos0;
			phi = atan2(epos[0] - this->pos[0], -(epos[2] - this->pos[2]));
			sd = (epos - this->pos).slen();
			z = sd < .01 * .01 ? 1 : -1;
//			pyr[1] = approach(pyr[1], phi, TURRETROTSPEED * dt, 2 * M_PI);
//			pyr2quat(pt->rot, pt->pyr);

#define EPSILON 1e-5
			/* only shoot when on feet */
/*			if(pt->pos[1] - .0016 <= h && shoot_angle2(pt->pos, epos, phi, bulletspeed, &desired, p->arms[0].type == arms_mortar)){
				double dst = desired[0] - pt->pyr[0];
				avec3_t pyr;
				amat4_t mat;
				shootable = 1;
				p->pitch = rangein(approach(p->pitch + M_PI, dst + M_PI, TURRETROTSPEED * dt, 2 * M_PI) - M_PI, vft->barrelmin, vft->barrelmax);
				pyr[0] = p->pitch;
				pyr[1] = pt->pyr[1];
				pyr[2] = 0.;
				if(!(p->arms[0].type == arms_rpg7 && (pyrmat(pyr, mat), VECSP(normal, &mat[8]) < 0.)) &&
					desired[0] - EPSILON <= p->pitch && p->pitch <= desired[0] + EPSILON &&
					desired[1] - EPSILON <= pt->pyr[1] && pt->pyr[1] <= desired[1] + EPSILON &&
					(p->kick[0] + p->kick[0] + p->kick[1] * p->kick[1]) < 1. / (sd / .01 / .01) * M_PI / 8. * M_PI / 8.)
					i |= PL_ENTER;
			}

			if(p->arms[0].type == arms_mortar && shootable){
				if(!p->aiming){
					i |= PL_RCLICK;
					ic |= PL_RCLICK;
				}
			}
			else if(p->aiming){
				i |= PL_RCLICK;
				ic |= PL_RCLICK;
			}*/

/*			pt->weapon = sd < .02 * .02;*/

/*			i |= PL_W;

			if(sd < .02 * .02){
				if(p->arms[0].type != arms_shotgun && p->arms[1].type == arms_shotgun)
					infantry_swapweapon(p);
			}
			else{
				if(p->arms[0].type != arms_m16rifle && p->arms[1].type == arms_m16rifle)
					infantry_swapweapon(p);
			}

			if(p->arms[0].type == arms_m40rifle && p->state != STATE_PRONE){
				p->state = p->state == STATE_STANDING ? STATE_PRONE : STATE_STANDING;
				p->shiftphase = 1.;
			}*/
		}
	}
#endif

	if(ic & i & PL_RCLICK){
		p->aiming = !p->aiming;
	}

	struct HookHit{
		static int hit_callback(const struct otjEnumHitSphereParam *param, Entity *pt){
			Vec3d *retpos = param->pos;
			Vec3d *retnorm = param->norm;
			Vec3d pos, nh;
			int ret = 1;

			if(param->hint == pt)
				return 0;

			if(!jHitSphere(pt->pos, pt->getHitRadius() + param->rad, *param->src, *param->dir, param->dt))
				return 0;
			ret = pt->tracehit(*param->src, *param->dir, param->rad, param->dt, NULL, &pos, &nh);
			if(!ret)
				return 0;
//					else if(rethitpart)
//						*rethitpart = ret;

			{
//						p->pos += pb->velo * dt;
				if(retpos)
					*retpos = pos;
				if(retnorm)
					*retnorm = nh;
			}
			return ret;
		}
	} hh;

	if(hookshot){
		if(!hooked){
			otjEnumHitSphereParam param;
			Vec3d hitpos, nh;
			param.root = ws->otroot;
			param.src = &hookpos;
			param.dir = &hookvelo;
			param.dt = dt;
			param.rad = 0.;
			param.pos = &hitpos;
			param.norm = &nh;
			param.flags = OTJ_CALLBACK;
			param.callback = hh.hit_callback;
			param.hint = this;
			if(ws->ot){
				Entity *pt = otjEnumHitSphere(&param);
				if(pt){
					hooked = true;
					hookedEntity = pt;
					hookpos = pt->rot.itrans(hitpos - pt->pos); // Assign hook position to precisely calculated hit position. 
					hookHitEffect(param);
				}
			}
			else{
				for(WarField::EntityList::iterator it = ws->el.begin(); it != ws->el.end(); it++) if(*it)
					if(hh.hit_callback(&param, *it)){
						hooked = true;
						hookedEntity = *it;
						hookpos = (*it)->rot.itrans(hitpos - (*it)->pos); // Assign hook position to precisely calculated hit position.
						hookHitEffect(param);
						break;
					}
			}

			if(!hooked){
				hookpos += hookvelo * dt;
				if(hookRange * hookRange < (hookpos - this->pos).slen()){
					hookshot = false;
					hookretract = true;
				}
			}
		}
	}

	if(hooked && hookedEntity){
		Vec3d worldhookpos = hookedEntity->pos + hookedEntity->rot.trans(this->hookpos);
		Vec3d delta = worldhookpos - this->pos;
		// Avoid zero division
		if(FLT_EPSILON < delta.slen()){
			Vec3d dir = delta.norm();
			// Accelerate towards hooked point
			velo += dir * hookPullAccel * dt;
			if(bbody)
				bbody->setLinearVelocity(btvc(velo));
			if(delta.slen() < hookStopRange * hookStopRange){
				// Rapid brake, based on the relative velocity to hooked point.
				Vec3d tvelo = hookedEntity->velo + hookedEntity->omg.vp(hookedEntity->rot.trans(hookpos));
				velo = tvelo + (velo - tvelo) * exp(-dt * 2.);
				// Cancel closing velocity
/*				velo -= velo.sp(dir) * dir;
				if(bbody)
					bbody->setLinearVelocity(btvc(velo));
				hooked = false;
				hookshot = false;
				hookretract = true;*/
			}
		}
	}

	if(hookretract){
		Vec3d delta = hookpos - this->pos;
		if(delta.slen() < 0.005 * 0.005){
			hookretract = false;
		}
		else
			hookpos -= delta.norm() * hookSpeed * dt;
	}

/*	if(p->arms[0].type == arms_shotgun && p->reloading){
		if(3. * dt < p->reloadphase){
			p->reloadphase -= 3. * dt;
			if(p->arms[0].type != arms_mortar)
				p->aiming = 0;
		}
		else if(p->arms[0].ammo < arms_static[p->arms[0].type].maxammo){
			p->arms[0].ammo++;
			if(p->arms[0].ammo < arms_static[p->arms[0].type].maxammo){
				p->reloadphase += 2.5;
				p->cooldown2 = p->reloadphase / 3.;
			}
			else
				p->reloadphase = 0., p->reloading = 0;
		}
		else
			p->reloadphase = 0., p->reloading = 0;
	}
	else*/
	if(arms[0] && reloading){
		if(p->reloadphase < dt){
			arms[0]->reload();
			p->reloadphase = 0.;
			p->reloading = 0;
		}
		else{
			p->reloadphase -= dt;
//			if(p->arms[0].type != arms_mortar)
//				p->aiming = 0;
		}
	}

//	pyr2quat(pt->rot, pt->pyr);
//	QUATCNJIN(pt->rot);

	/* shooter logic */
	p->muzzle = 0;
	if(!p->reloading && i & (PL_ENTER | PL_LCLICK) && !(i & PL_SHIFT)) while(p->cooldown2 < dt){
		amat4_t gunmat;
		double kickf = g_recoil_kick_factor * (p->state == STATE_PRONE ? .3 : 1.);
		gunmat[15] = 0.;

		/* reload gun */
		if(p->arms[0]->ammo <= 0){
			if(1){
				p->cooldown2 = 0.;
				reload();
			}
			break;
		}
		else{
			cooldown2 += arms[0]->shootCooldown();
			if(game->isServer()){
				arms[0]->shoot();
			}
			else
				muzzle |= 1;
		}
#if 0
		else if(p->arms[0]->type == arms_shotgun/*pt->weapon*/){
			int j;
			playWave3DPitch("gun21.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .02 * .02, w->realtime + dt - p->cooldown2, 256/* + rseq(&w->rs) % 256*/);
			for(j = 0; j < 8; j++)
				shoot_infgun(w, pt, pt->pyr[1] + p->kick[1], pt->pyr[0] + p->kick[0] + p->pitch, BULLETSPEED * .75, .5, .025, dt - p->cooldown2, gunmat);
			infantry_brass(w, pt, 1, dt - p->cooldown2, gunmat);
			pt->shoots2++;
			p->kickvelo[0] += kickf * (drseq(&w->rs) - .3) * M_PI / 64.;
			p->kickvelo[1] += kickf * (drseq(&w->rs) - .5) * M_PI / 64.;
			p->arms[0].ammo--;
			p->muzzle |= 1;
			p->cooldown2 += .9 + drseq(&w->rs) * .2;
		}
		else if(p->arms[0].type == arms_m16rifle){
			static wavc_t *wavc = NULL;
#if 0
			if(!wavc)
				wavc = cacheWaveDataZ("rc.zip", "sound/rifle.wav");
			if(wavc)
				playMemoryWave3D(wavc->head.lpData, wavc->head.dwBufferLength, 256/* + rseq(&w->rs) % 256*/, pt->pos, w->pl->pos, w->pl->pyr, 1., .02 * .02, w->realtime + dt - p->cooldown2);
#else
			playWave3DPitch("rc.zip/sound/rifle.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .02 * .02, w->realtime + dt - p->cooldown2, 256/* + rseq(&w->rs) % 256*/);
#endif
			shoot_infgun(w, pt, pt->pyr[1] + p->kick[1], pt->pyr[0] + p->kick[0] + p->pitch, BULLETSPEED, 1., .005, dt - p->cooldown2, gunmat);
			infantry_brass(w, pt, 0, dt - p->cooldown2, gunmat);
			pt->shoots2++;
			p->kickvelo[0] += kickf * (drseq(&w->rs) - .3) * M_PI / 80.;
			p->kickvelo[1] += kickf * (drseq(&w->rs) - .5) * M_PI / 80.;
			p->arms[0].ammo--;
			p->muzzle |= 1;
			p->cooldown2 += 0.07;
		}
		else if(p->arms[0].type == arms_m40rifle){
			playWave3DPitch("sound/rifle.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .02 * .02, w->realtime + dt - p->cooldown2, 256/* + rseq(&w->rs) % 256*/);
			shoot_infgun(w, pt, pt->pyr[1] + p->kick[1], pt->pyr[0] + p->kick[0] + p->pitch, .770, 3., .001, dt - p->cooldown2, gunmat);
			infantry_brass(w, pt, 0, dt - p->cooldown2, gunmat);
			pt->shoots2++;
			p->kickvelo[0] += kickf * (drseq(&w->rs) - .3) * M_PI / 48.;
			p->kickvelo[1] += kickf * (drseq(&w->rs) - .5) * M_PI / 48.;
			p->arms[0].ammo--;
			p->muzzle |= 1;
			p->cooldown2 += arms_static[p->arms[0].type].cooldown;
		}
		else if(p->arms[0].type == arms_rpg7){
			playWave3DPitch(CvarGetString("sound_missile"), pt->pos, w->pl->pos, w->pl->pyr, 1., .02 * .02, w->realtime + dt - p->cooldown2, 256/* + rseq(&w->rs) % 256*/);
			shoot_infgun(w, pt, pt->pyr[1] + p->kick[1], pt->pyr[0] + p->kick[0] + p->pitch, .01, 100., .005, dt - p->cooldown2, gunmat);
			pt->shoots2++;
			p->kickvelo[0] += kickf * (drseq(&w->rs) - .3) * M_PI / 16.;
			p->kickvelo[1] += kickf * (drseq(&w->rs) - .5) * M_PI / 16.;
			p->arms[0].ammo--;
			p->muzzle |= 1;
			p->cooldown2 += arms_static[p->arms[0].type].cooldown;
		}
		else if(p->arms[0].type == arms_mortar){
			if(!p->aiming)
				break;
			playWave3DPitch(CvarGetString("sound_missile"), pt->pos, w->pl->pos, w->pl->pyr, 1., .02 * .02, w->realtime + dt - p->cooldown2, 256/* + rseq(&w->rs) % 256*/);
			shoot_infgun(w, pt, pt->pyr[1] + p->kick[1], pt->pyr[0] + p->kick[0] + p->pitch, .05, 150., .050, dt - p->cooldown2, gunmat);
/*			if(w->tell){
				avec3_t velo;
				VECSCALE(velo, &gunmat[8], -.001);
				AddTeline3D(w->tell, &gunmat[12], velo, .001, NULL, NULL, NULL, COLOR32RGBA(255,255,255,255), TEL3_INVROTATE | TEL3_GLOW, 1.);
			}*/
			pt->shoots2++;
/*			p->kickvelo[0] += kickf * (drseq(&w->rs) - .3) * M_PI / 16.;
			p->kickvelo[1] += kickf * (drseq(&w->rs) - .5) * M_PI / 16.;*/
			p->arms[0].ammo--;
			p->muzzle |= 1;
			p->cooldown2 += arms_static[p->arms[0].type].cooldown;
		}
		else if(p->arms[0].type == arms_plg){
			playWave3DPitch(CvarGetString("sound_missile"), pt->pos, w->pl->pos, w->pl->pyr, 1., .02 * .02, w->realtime + dt - p->cooldown2, 256/* + rseq(&w->rs) % 256*/);
			shoot_infgun(w, pt, pt->pyr[1] + p->kick[1], pt->pyr[0] + p->kick[0] + p->pitch, 5., 15., .005, dt - p->cooldown2, gunmat);
/*			if(w->tell){
				avec3_t velo;
				VECSCALE(velo, &gunmat[8], -.001);
				AddTeline3D(w->tell, &gunmat[12], velo, .001, NULL, NULL, NULL, COLOR32RGBA(255,255,255,255), TEL3_INVROTATE | TEL3_GLOW, 1.);
			}*/
			pt->shoots2++;
/*			p->kickvelo[0] += kickf * (drseq(&w->rs) - .3) * M_PI / 16.;
			p->kickvelo[1] += kickf * (drseq(&w->rs) - .5) * M_PI / 16.;*/
			p->arms[0].ammo--;
			p->muzzle |= 1;
			p->cooldown2 += arms_static[p->arms[0].type].cooldown;
		}
#endif
	}
	if(p->cooldown2 < dt)
		p->cooldown2 = 0.;
	else
		p->cooldown2 -= dt;

	if(0 < dt){
		double dt2;
		int t;
		int divide = /*(int)(dt / .1)*/ + 1; /* frametime of 0.02 sec is assured to prevent vibration */
		dt2 = dt / divide;
/*		printf("n %d\n", divide);*/
		for(t = 0; t < divide; t++){
			double relax, relax2, prelax;
			double accel[2];
			int i;
			relax = /*MIN(1., .5 * dt2)/*/exp(-5. * dt2);
			relax2 = (1. - relax) / 5.;
			prelax = /*MIN(1., 50. * dt2)/*/exp(-2. * dt2);
			for(i = 0; i < 2; i++){
				accel[i] = (p->kickvelo[i] * (relax - 1.)/* - p->kick[0] * prelax*/)/* + p->kick[0]*/;
				p->kickvelo[i] += accel[i];
				accel[i] /= dt2;
/*			p->kick[0] += p->kickvelo[0] * dt2 + accel[0] * dt2 * dt2 / 2.;
			p->kick[1] += p->kickvelo[1] * dt2 + accel[1] * dt2 * dt2 / 2.;*/
				p->kick[i] += p->kickvelo[i] * relax2;
				p->kick[i] *= prelax;
			}
/*			printf("accel %lg %lg\n", accel[0], accel[1]);*/
		}
/*		printf("velo %lg %lg\n", p->kickvelo[0], p->kickvelo[1]);
		printf("kick %lg %lg\n", p->kick[0], p->kick[1]);*/
	}

	if(p->swapphase < dt)
		p->swapphase = 0.;
	else
		p->swapphase -= dt;

	if(p->damagephase < dt)
		p->damagephase = 0.;
	else
		p->damagephase -= dt;

	Vec3d accel = w->accel(this->pos, this->velo);
	if(!bbody){
		this->velo += accel * dt;
	}

//	af = (p->state == STATE_PRONE ? .25 : 1.) * (i & (PL_CTRL) ? .6 : i & PL_SHIFT ? 3. : 1.) * (x && z ? 1. / 1.41421356 : 1.) * 60. / (pt->mass + arms_static[p->arms[0].type].emptyweight + arms_static[p->arms[1].type].emptyweight) /** dt * .0075*/;
	af = 1.;

	if(0 < health){
		// In zero-G
		if(accel.slen() < 1e-1){
			Mat4d mat;
			transform(mat);
			maneuver(mat, dt, &getManeuve());
		}
		else if(shiftphase == 0.){
			if(!(/*arms[0].type == arms_mortar && */p->aiming)){
				double phasespeed = INFANTRY_WALK_PHASE_SPEED * af;
				int inputs = i;
				if(inputs & PL_W){
					p->walkphase = p->walkphase + phasespeed * dt;
				}
				if(inputs & PL_S){
					p->walkphase = p->walkphase - phasespeed * dt;
				}
				if(inputs & PL_A){
					p->walkphase = p->walkphase + phasespeed * dt;
				}
				if(inputs & PL_D){
					p->walkphase = p->walkphase + phasespeed * dt;
				}
				if(!(inputs & (PL_W | PL_S | PL_A | PL_D)))
					p->walkphase = 0.;
			}
		}
		else if(p->shiftphase < dt){
			p->shiftphase = 0.;
		}
		else
			p->shiftphase -= dt;
	}
	else{
		p->walkphase += dt;
	}

/*	p->rad = i & PL_Z ? .001 : .0016;*/

#if 0
	if(this->pos[1] - .0016 <= h){
/*		entity_t *p = pt;*/
		double l, f, t;
		Vec3d dest;
		/* can't walk in midair */
		if((x || z) && !(/*p->arms[0].type == arms_mortar &&*/ p->aiming)){
#if 1
			double pyr[3] = {0,0,0};
			Vec3d vec = Vec3d(
				af * (/*1. / (p->velolen + .001) +*/ .003) * (x * cos(pyr[1]) - z * sin(pyr[1])),
				0.,
				af * (/*1. / (p->velolen + .001) +*/ .003) * (z * cos(pyr[1]) + x * sin(pyr[1])));
			dest = vec - this->velo;
#else
			pt->velo[0] += /*(.001 - p->velo[0]) **/ af * (x * cos(p->pyr[1]) - z * sin(p->pyr[1]));
			pt->velo[2] += /*(.001 - p->velo[2]) **/ af * (z * cos(p->pyr[1]) + x * sin(p->pyr[1]));
#endif
		}
		else{
			dest = -this->velo;
		}
		if(pos[1] <= h && 0 < y)
			velo[1] += .005;

		/* surface friction */
		l = dest.len();
/*		t = -log(l);*/
		f = (1. - exp(-5. * (dt)));
		if(f < 1.)
			this->velo += dest * f;
		else
			this->velo += dest;
	}
#endif

	if(i & ~ic & PL_TAB){
		if(game->isServer())
			swapWeapon();
		else{
			SwitchWeaponCommand com;
			CMEntityCommand::s.send(this, com);
		}
	}

#if 0
	{
		WarField::EntityList::iterator it;
		for(it = w->el.begin(); it != w->el.end(); ++it){
			Entity *pt2 = *it;
			if(/*active &&*/ pt2->classname() == this->classname() && (pt2->pos - this->pos).slen() < .0005 * .0005){
				Vec3d dr = pt2->pos - this->pos;
				double sp = dr.sp(this->velo);
				if(0. < sp){
					sp /= dr.slen();
					this->velo += dr * -sp;
					break;
				}
			}
		}
	}
#endif

	if(bbody){
		this->pos = btvc(bbody->getWorldTransform().getOrigin());
		this->rot = btqc(bbody->getWorldTransform().getRotation());
	}
	else{
		// Manually resolve collision if we do not have bbody.
		otjEnumHitSphereParam param;
		Vec3d hitpos, nh;
		param.root = ws->otroot;
		param.src = &pos;
		param.dir = &velo;
		param.dt = dt;
		param.rad = getHitRadius();
		param.pos = &hitpos;
		param.norm = &nh;
		param.flags = OTJ_CALLBACK;
		param.callback = hh.hit_callback;
		param.hint = this;
		if(ws->ot){
			Entity *pt = otjEnumHitSphere(&param);
			if(pt && this->velo.sp(nh) < 0.){
				this->velo -= this->velo.sp(nh) * nh;
				this->pos = hitpos;
			}
		}
		else{
			for(WarField::EntityList::iterator it = ws->el.begin(); it != ws->el.end(); it++) if(*it)
				if(hh.hit_callback(&param, *it)){
					this->velo -= this->velo.sp(nh) * nh;
					this->pos = hitpos;
					break;
				}
		}
		this->pos += this->velo * dt;
		this->rot = this->rot.quatrotquat(this->omg * dt);
	}

//	h = warmapheight(w->map, pt->pos[0], pt->pos[2], NULL);

//	if(pt->pos[1] < h)
//		pt->pos[1] = h;
	for(int i = 0; i < numof(arms); i++) if(arms[i]){
		arms[i]->align();
	}
}

void Soldier::clientUpdate(double dt){
	anim(dt);
}

void Soldier::control(const input_t *inputs, double dt){
	const double speed = .001 / 2.;
	st::control(inputs, dt);

	Quatd arot;
	getPosition(NULL, &arot);

	// Mouse-aim direction
	Vec3d xomg = arot.trans(Vec3d(0, -inputs->analog[0] * speed, 0));
	Vec3d yomg = arot.trans(Vec3d(-inputs->analog[1] * speed, 0, 0));
	arot = arot.quatrotquat(xomg);
	arot = arot.quatrotquat(yomg);

	setPosition(NULL, &arot, NULL, NULL);

	if(inputs->change & inputs->press & PL_B){
		if(!hookshot && !hookretract && !hooked){
			hookshot = true;
			hookpos = this->pos;
			hookvelo = this->velo + this->rot.trans(Vec3d(0,0,-hookSpeed));
			hooked = false;
		}
		else if(hookshot){
			if(hookedEntity){
				hookpos = hookedEntity->pos + hookedEntity->rot.trans(hookpos);
				hookedEntity = NULL;
			}
			hookretract = true;
			hookshot = false;
			hooked = false;
		}
	}
}

// find the nearest enemy
bool Soldier::findEnemy(){
	if(forcedEnemy)
		return !!enemy;
	Entity *closest = NULL;
	double best = 1e2 * 1e2;
	for(WarField::EntityList::iterator it = w->el.begin(); it != w->el.end(); it++) if(*it){
		Entity *pt2 = *it;

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
	if(closest){
		enemy = closest;
		integral[0] = integral[1] = 0.f;
//		evelo = vec3_000;
	}
	return !!closest;
}

Entity::Props Soldier::props()const{
	Props ret = st::props();
	ret.push_back(gltestp::dstring("Ammo: ") << (arms[0] ? arms[0]->ammo : 0));
	ret.push_back(gltestp::dstring("Cooldown: ") << cooldown2);
	return ret;
}

int Soldier::armsCount()const{
	return numof(arms);
}

ArmBase *Soldier::armsGet(int i){
	return arms[i];
}

bool Soldier::command(EntityCommand *com){
	if(InterpretCommand<SwitchWeaponCommand>(com)){
		swapWeapon();
		return true;
	}
	if(GetGunPosCommand *ggp = InterpretCommand<GetGunPosCommand>(com)){
		return getGunPos(*ggp);
	}
	else
		return st::command(com);
}

const Autonomous::ManeuverParams &Soldier::getManeuve()const{
	return maneuverParams;
}


#define DNMMOT_PROFILE 0

#if 0
static ysdnmv_t *infantry_boneset(infantry_t *p){
	static struct ysdnm_motion *motions[2], *oldmotions[2];
	static struct ysdnm_motion *motaim, *motshoulder, *motswap, *motreload, *motstand, *motwalk, *motfall;
	static struct ysdnm_motion_instance *motiaim, *motishoulder, *motiswap, *motireload, *motistand, *motiwalk, *motifall;
	static ysdnmv_t *dnmv = NULL, gunmove = {0};
	static double lasttimes[2];
	double times[2];
	if(!motions[0]){
		motaim = YSDNM_MotionLoad("infantry_aim.mot");
		motshoulder = YSDNM_MotionLoad("infantry_shoulder.mot");
		motswap = YSDNM_MotionLoad("infantry_swapweapon.mot");
		motreload = YSDNM_MotionLoad("infantry_reload.mot");
		motstand = YSDNM_MotionLoad("infantry_stand.mot");
		motwalk = YSDNM_MotionLoad("infantry_walk.mot");
		motfall = YSDNM_MotionLoad("infantry_fall.mot");
		motiaim = YSDNM_MotionInstanciate(dnm, motaim);
		motishoulder = YSDNM_MotionInstanciate(dnm, motshoulder);
		motiswap = YSDNM_MotionInstanciate(dnm, motswap);
		motireload = YSDNM_MotionInstanciate(dnm, motreload);
		motistand = YSDNM_MotionInstanciate(dnm, motstand);
		motiwalk = YSDNM_MotionInstanciate(dnm, motwalk);
		motifall = YSDNM_MotionInstanciate(dnm, motfall);
	}

	/* upper torso motion */
	if(0 < p->st.health && 0. < p->swapphase){
		motions[1] = motiswap;
		times[1] = (p->swapphase < 1. ? p->swapphase : 2. - p->swapphase) * 10.;
	}
	else if(0 < p->st.health && 0. < p->reloadphase){
		motions[1] = motireload;
		times[1] = (p->reloadphase < 1.25 ? p->reloadphase / 1.25 : (2.5 - p->reloadphase) / 1.25) * 20.;
	}
	else{
		motions[1] = p->arms[0].type == arms_rpg7 || p->arms[0].type == arms_mortar || p->arms[0].type == arms_plg ? motishoulder : motiaim;
		times[1] = p->st.health <= 0. ? (p->walkphase < 1. ? p->walkphase : 1.) * 10. : 0.;
	}

	/* legs/body motion */
	if(p->st.health <= 0.){
		motions[0] = motifall;
		times[0] = (p->walkphase < 1. ? p->walkphase : 1.) * 20.;
	}
	else if(0 < p->shiftphase || p->state == STATE_PRONE){
		motions[0] = motifall;
		times[0] = (p->state == STATE_STANDING ? p->shiftphase : 1. - p->shiftphase) * 20.;
	}
	else if(p->walkphase){
		motions[0] = motiwalk;
		times[0] = (p->walkphase / (M_PI) - floor(p->walkphase / (M_PI))) * 40.;
	}
	else{
		motions[0] = motistand;
		times[0] = 0;
	}

	if(dnmv && !memcmp(motions, oldmotions, sizeof motions) && !memcmp(lasttimes, times, sizeof times))
		goto control_surfaces;
	else if(dnmv)
		YSDNM_MotionInterpolateFree(dnmv);
#if DNMMOT_PROFILE
	{
		timemeas_t tm;
		TimeMeasStart(&tm);
#endif
		dnmv = YSDNM_MotionInterpolate(motions, times, 2);
#if DNMMOT_PROFILE
		printf("motint %lg\n", TimeMeasLap(&tm));
	}
#endif
	memcpy(oldmotions, motions, sizeof motions);
	memcpy(lasttimes, times, sizeof lasttimes);

/*	{
		ysdnmv_t **ppv;
		for(ppv = &dnmv; *ppv; ppv = &(*ppv)->next);
		*ppv = &gunmove;
	}*/
control_surfaces:
#if 1
	/*if(p->st.barrelp != 0.)*/{
		static const char *infantry_bonenames[4] = {
			"LArm1",
			"RArm1",
			"Head",
			"Body",
		};
		static double infantry_rot[4][7];
		ysdnmv_t *v = &gunmove;
		int i;
		double pitch = (p->st.health <= 0. ? p->walkphase < 1. ? 1. - p->walkphase : 0. : 1.) * (-(p->pitch + p->kick[0]) + (p->state == STATE_STANDING ? p->shiftphase : 1. - p->shiftphase) * -M_PI / 2.);
		aquat_t q, qx, qy;
		qx[0] = sin(pitch / 2.);
		qx[1] = 0.;
		qx[2] = 0.;
		qx[3] = cos(pitch / 2.);
		qy[0] = 0.;
		qy[1] = sin(-(-p->kick[1]) / 2.);
		qy[2] = 0.;
		qy[3] = cos(-(p->kick[1]) / 2.);
		QUATMUL(q, qx, qy);
		v->bonenames = infantry_bonenames;
		v->bones = numof(infantry_bonenames);
		v->bonerot = infantry_rot;
		for(i = 0; i < 3; i++){
			QUATCPY(infantry_rot[i], q);
			VECNULL(&infantry_rot[i][4]);
		}
		VECSCALE(&infantry_rot[3][4], avec3_001, -.001 / INFANTRY_SCALE * (p->state == STATE_STANDING ? p->shiftphase : 1. - p->shiftphase));
		VECNULL(infantry_rot[3]);
		infantry_rot[3][3] = 1.;
		v->next = dnmv;
		return v;
	}
#endif
	return dnmv;
}


#define SQRT2P2 (1.41421356/2.)


static const double infantry_sc[3] = {.05, .055, .075};
/*static const double beamer_sc[3] = {.05, .05, .05};*/
static struct hitbox infantry_hb[] = {
	{{0., 0.00075, .0}, {0,0,0,1}, {.0003, .00075, .0003}},
	{{.0, .00165, .0}, {0,SQRT2P2,0,SQRT2P2}, {.00025, .00025, .00025}},
}, infantry_dead_hb[] = {
	{{0., 0.00025, -.00075}, {0,0,0,1}, {.0003, .00025, .00075}},
	{{.0, .00025, -.00165}, {0,SQRT2P2,0,SQRT2P2}, {.00025, .00025, .00025}},
};

#ifdef NDEBUG
#define hitbox_draw
#else
static void hitbox_draw(const entity_t *pt, const double sc[3]){
		glPushMatrix();
		glScaled(sc[0], sc[1], sc[2]);
		glPushAttrib(GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glDisable(GL_POLYGON_SMOOTH);
		glColor4ub(255,0,0,255);
		glBegin(GL_LINES);
		{
			int i, j, k;
			for(i = 0; i < 3; i++) for(j = -1; j <= 1; j += 2) for(k = -1; k <= 1; k += 2){
				double v[3];
				v[i] = j;
				v[(i+1)%3] = k;
				v[(i+2)%3] = -1.;
				glVertex3dv(v);
				v[(i+2)%3] = 1.;
				glVertex3dv(v);
			}
		}
		glEnd();
		glPopAttrib();
		glPopMatrix();
}
#endif

static double inf_pixels;

static void infantry_draw_arms(const char *name, infantry_t *p){

	if(!strcmp(name, "RHand")){
		int i = 1. < p->swapphase ? 1 : 0;
		glPushMatrix();
		glRotated(-90, 0, 1, 0);
		gldScaled(1. / (2./1e5));
		glTranslated(0, 0, -.0002);
		if(arms_static[p->arms[i].type].draw)
			arms_static[p->arms[i].type].draw(&p->arms[i], inf_pixels);
		glPopMatrix();
	}
	else if(!strcmp(name, "Body")){
		int i = 1. < p->swapphase ? 0 : 1;
		glPushMatrix();
		gldScaled(1. / (2./1e5));
		glTranslated(0.0000, 0.00, -.0002);
		glRotated(90, 1, 0, 0);
		glRotated(90, 0, 0, 1);
		glRotated(-30, 1, 0, 0);
		if(arms_static[p->arms[i].type].draw)
			arms_static[p->arms[i].type].draw(&p->arms[i], inf_pixels);
		glPopMatrix();
	}
}

static void infantry_draw(entity_t *pt, wardraw_t *wd){
	static int init = 0;
	static suf_t *sufbody;
	static suftex_t *suft_body;
	infantry_t *p = ((infantry_t*)pt);
	struct entity_private_static *vft = (struct entity_private_static*)pt->vft;
	double pixels;

	if(!pt->active)
		return;

	/* cull object */
	if(glcullFrustum(&pt->pos, .003, wd->pgc))
		return;
	inf_pixels = pixels = .001 * fabs(glcullScale(&pt->pos, wd->pgc));
	if(pixels < 2)
		return;
	wd->lightdraws++;


	if(init == 0){
		CallCacheBitmap("infantry.bmp", "infantry.bmp", NULL, NULL);
//		CacheSUFTex("infantry.bmp", (BITMAPINFO*)lzuc(lzw_infantry, sizeof lzw_infantry, NULL), 0);
		dnm = LoadYSDNM_MQO(/*CvarGetString("valkie_dnm")/*/"infantry2.dnm", "infantry2.mqo");
/*		sufbody = &suf_roughbody *//*LoadSUF("roughbody2.suf")*/;
/*		vft->sufbase = sufbody;*/
/*		CacheSUFTex("face.bmp", (BITMAPINFO*)lzuc(lzw_face, sizeof lzw_face, NULL), 0);*/
/*		if(sufbody)
			suft_body = AllocSUFTex(sufbody);*/
		init = 1;
	}
#if 1
		if(dnm){
			double scale = 2./1e5;
			ysdnmv_t *dnmv;
			glPushMatrix();
/*			glMultMatrixf(rotmqo);*/

			dnmv = infantry_boneset(p);

			gldTranslate3dv(pt->pos);
			glRotated(deg_per_rad * pt->pyr[1], 0., -1., 0.);
			glRotated(deg_per_rad * pt->pyr[0], -1.,  0., 0.);
			glRotated(deg_per_rad * pt->pyr[2], 0.,  0., -1.);
			glRotated(180, 0, 1, 0);
			glScaled(-scale, scale, scale);
			glTranslated(0, 54.4, 0);
/*			glTranslated(0, p->batphase * -7, 0.);
			glTranslated(0, 0, 5. * p->wing[0] / (M_PI / 4.));*/
			glPushAttrib(GL_POLYGON_BIT);
			glFrontFace(GL_CW);
/*			DrawYSDNM(dnm, names, rot, numof(valkie_bonenames), skipnames, numof(skipnames) - (p->bat ? 0 : 2));*/
/*			DrawYSDNM_V(dnm, &v);*/
#if DNMMOT_PROFILE
			{
				timemeas_t tm;
				TimeMeasStart(&tm);
#endif
				DrawYSDNM_V(dnm, dnmv);
#if DNMMOT_PROFILE
				printf("motdraw %lg\n", TimeMeasLap(&tm));
			}
#endif
			if(10 < pixels)
				TransYSDNM_V(dnm, dnmv, infantry_draw_arms, p);
			g_gldcache.valid = 0;
/*			TransYSDNM(dnm, names, rot, numof(valkie_bonenames), NULL, 0, draw_arms, p);*/
			glPopAttrib();
			glPopMatrix();
		}
		else
#endif
	if(!sufbody){
		double pos[3];
		GLubyte col[4] = {255,255,0,255};
		gldPseudoSphere(pos, .015, col);
	}
	else{
		double scale = 1./180000;
		static const GLdouble rotaxis[16] = {
			-1,0,0,0,
			0,0,1,0,
			0,1,0,0,
			0,0,0,1,
		};

/*		if(0 < sun.pos[1])
			ShadowSUF(sufbody, sun.pos, n, pos[i], NULL, scale, &rotaxisd);*/
		glPushMatrix();
		glTranslated(pt->pos[0], pt->pos[1], pt->pos[2]);
		glRotated(deg_per_rad * pt->pyr[1], 0., -1., 0.);
		glRotated(deg_per_rad * pt->pyr[0], -1.,  0., 0.);
		glRotated(deg_per_rad * pt->pyr[2], 0.,  0., -1.);
		glScaled(scale, scale, scale);
		glMultMatrixd(rotaxis);
		DecalDrawSUF(sufbody, SUF_ATR, &g_gldcache, suft_body, NULL, NULL);
		glPopMatrix();
	}
#ifndef NDEBUG
	{
		int i;
		avec3_t org;
		amat4_t mat;
		struct hitbox *hb = pt->health <= 0. || p->state == STATE_PRONE ? infantry_dead_hb : infantry_hb;
/*		aquat_t rot, roty;
		roty[0] = 0.;
		roty[1] = sin(pt->turrety / 2.);
		roty[2] = 0.;
		roty[3] = cos(pt->turrety / 2.);
		QUATMUL(rot, pt->rot, roty);*/

		glPushMatrix();
/*		gldTranslate3dv(pt->pos);
		gldMultQuat(rot);*/
		tankrot(&mat, pt);
		glMultMatrixd(mat);
/*		gldTranslate3dv(pt->pos);*/
/*		glRotated(pt->turrety * deg_per_rad, 0, 1., 0.);*/
		VECSCALE(org, avec3_001, .001 * (p->state == STATE_STANDING ? p->shiftphase : 1. - p->shiftphase));
		gldTranslate3dv(org);

		for(i = 0; i < numof(infantry_hb); i++){
			amat4_t rot;
			glPushMatrix();
			gldTranslate3dv(hb[i].org);
			quat2mat(rot, hb[i].rot);
			glMultMatrixd(rot);
			hitbox_draw(pt, hb[i].sc);
			glPopMatrix();

			glPushMatrix();
			gldTranslate3dv(hb[i].org);
			quat2imat(rot, hb[i].rot);
			glMultMatrixd(rot);
			hitbox_draw(pt, hb[i].sc);
			glPopMatrix();
		}

		glPopMatrix();
	}
#endif
}

static void infantry_gib_draw(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	int i = (int)pv;
	double scale = INFANTRY_SCALE;
	amat4_t mat;
	glPushMatrix();
	glTranslated(pl->pos[0], pl->pos[1], pl->pos[2]);
	glScaled(scale, scale, scale);
	quat2mat(&mat, pl->rot);
	glMultMatrixd(mat);
	DrawSUF(dnm->p[i].suf, SUF_ATR, &g_gldcache);
	glPopMatrix();
}



double perlin_noise_pixel(int x, int y, int bit);
void drawmuzzleflasha(const double (*pos)[3], const double (*org)[3], double rad, const double (*irot)[16]){
	static GLuint texname = 0;
	static const GLfloat envcolor[4] = {1.,1,1,1};
	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
	if(!texname){
		GLubyte texbits[64][64][2];
		struct random_sequence rs;
		int i, j;
		init_rseq(&rs, 3526262);
		for(i = 0; i < 64; i++) for(j = 0; j < 64; j++){
			double x = (i - 32.) / 32., y = (j - 32.) / 32., a, r;
			r = 1.5 * (1. - x * x - y * y + (perlin_noise_pixel(i, j, 3) - .5) * .5);
			texbits[i][j][0] = (GLubyte)rangein(r * 256, 0, 255);
			a = 1.5 * (1. - x * x - y * y + (perlin_noise_pixel(i + 64, j + 64, 3) - .5) * .5);
			texbits[i][j][1] = (GLubyte)rangein(a * 256, 0, 255);
		}
		glGenTextures(1, &texname);
		glBindTexture(GL_TEXTURE_2D, texname);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, 64, 64, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, texbits);
	}
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, /*GL_ADD/*/GL_BLEND);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, envcolor);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1, .5, 0., 1.);
	/*glDisable/*/glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	glDisable(GL_CULL_FACE);
	glBindTexture(GL_TEXTURE_2D, texname);
	glPushMatrix();
	gldTranslate3dv(*pos);
	glMultMatrixd(*irot);
	gldScaled(rad);
	glBegin(GL_QUADS);
	glTexCoord2i(0, 0); glVertex2i(-1, -1);
	glTexCoord2i(1, 0); glVertex2i( 1, -1);
	glTexCoord2i(1, 1); glVertex2i( 1,  1);
	glTexCoord2i(0, 1); glVertex2i(-1,  1);
	glEnd();
	glPopMatrix();
	glPopAttrib();
}

static wardraw_t *s_wd;

static void infantry_drawtra_arms(const char *name, amat4_t *hint){
/*	amat4_t mat, mat2, irot;
	avec3_t pos, pos0 = {0., 0., -.001}, pos1 = {0., 0., -.0013};*/

	if(!strcmp(name, "RHand")){
		static const avec3_t pos = {.13, -.37, 3.};
		amat4_t mat;
		glGetDoublev(GL_MODELVIEW_MATRIX, *hint);
/*		mat4vp3(*hint, mat, pos);*/
/*		int i = 1. < p->swapphase ? 1 : 0;
		gldScaled(1. / INFANTRY_SCALE);
		mat4rotz(irot, s_wd->irot, drseq(&s_wd->w->rs) * 2. * M_PI);
		MAT4IDENTITY(mat);
		mat4vp3(pos, mat, pos0);
		drawmuzzleflasha(&pos, &s_wd->view, .0004, &irot);
		mat4vp3(pos, mat, pos1);
		drawmuzzleflasha(&pos, &s_wd->view, .00025, &irot);*/
	}
}


static void infantry_drawtra(entity_t *pt, wardraw_t *wd){
	infantry_t *p = (infantry_t*)pt;
	struct entity_private_static *vft = (struct entity_private_static *)pt->vft;
	double pixels;

	if(!pt->active)
		return;

	/* cull object */
	if(glcullFrustum(&pt->pos, .003, wd->pgc))
		return;
	pixels = .002 * fabs(glcullScale(&pt->pos, wd->pgc));
	if(pixels < 2)
		return;

#if 1
	if(3 < pixels && p->muzzle){
		avec3_t pos, pos0 = {0., 0., -.001}, pos1 = {0., 0., -.0013};
		amat4_t mat, mat2, gunmat, irot;
		avec3_t muzzlepos;
		double scale = INFANTRY_SCALE;
		ysdnmv_t *dnmv;

		MAT4IDENTITY(gunmat);

		dnmv = infantry_boneset(p);

		s_wd = wd;
		glPushMatrix();
		glLoadIdentity();
		glRotated(180, 0, 1, 0);
		glScaled(-scale, scale, scale);
		glTranslated(0, 54.4, 0);
		TransYSDNM_V(dnm, dnmv, infantry_drawtra_arms, &gunmat);
		MAT4SCALE(gunmat, 1. / scale, 1. / scale, 1. / scale);
/*		VECSCALEIN(&gunmat[12], scale);*/
		glPopMatrix();

/*		gldTranslate3dv(pt->pos);
		glRotated(deg_per_rad * pt->pyr[1], 0., -1., 0.);
		glRotated(deg_per_rad * pt->pyr[0], -1.,  0., 0.);
		glRotated(deg_per_rad * pt->pyr[2], 0.,  0., -1.);
		glScaled(-scale, scale, scale);
		glTranslated(0, 54.4, 0);*/

/*		pos0[1] = pos1[1] = 220. * infantry_s.sufscale;*/
		mat4rotz(irot, wd->irot, drseq(&wd->w->rs) * 2. * M_PI);
		MAT4IDENTITY(mat);
		VECADDIN(&mat[12], pt->pos);
		mat4roty(mat2, mat, -pt->pyr[1]);
		mat4rotx(mat, mat2, pt->pyr[0]);
		mat4mp(mat2, mat, gunmat);
		mat4roty(mat, mat2, -90 / deg_per_rad);
		mat4vp3(pos, mat, pos0);
		drawmuzzleflasha(&pos, &wd->view, .0004, &irot);
		mat4vp3(pos, mat, pos1);
		drawmuzzleflasha(&pos, &wd->view, .00025, &irot);
	}
#else
	if(p->muzzle){
		amat4_t mat, mat2, irot;
		avec3_t pos, pos0 = {0., 0., -.001}, pos1 = {0., 0., -.0013};
		pos0[1] = pos1[1] = 220. * infantry_s.sufscale;
		mat4rotz(irot, wd->irot, drseq(&wd->w->rs) * 2. * M_PI);
		MAT4IDENTITY(mat);
		mat4translate(mat, pt->pos[0], pt->pos[1], pt->pos[2]);
		mat4roty(mat2, mat, -pt->pyr[1]);
		mat4rotx(mat, mat2, pt->pyr[0] + pt->barrelp);
		mat4vp3(pos, mat, pos0);
		drawmuzzleflasha(&pos, &wd->view, .0004, &irot);
		mat4vp3(pos, mat, pos1);
		drawmuzzleflasha(&pos, &wd->view, .00025, &irot);
/*		p->muzzle = 0;*/
	}
#endif

#if 0
	{
		int i;
		double (*cuts)[2];
		avec3_t velo;
		amat4_t rot;
		glPushMatrix();
		glTranslated(pt->pos[0], pt->pos[1], pt->pos[2]);
		pyrmat(pt->pyr, &rot);
		glMultMatrixd(rot);
		glTranslated(vft->hitmdl.bs[0], vft->hitmdl.bs[1], vft->hitmdl.bs[2]);
		pyrimat(pt->pyr, &rot);
		glMultMatrixd(rot);
		glMultMatrixd(wd->irot);
		cuts = CircleCuts(16);
		glColor3ub(255,255,0);
		glBegin(GL_LINE_LOOP);
		for(i = 0; i < 16; i++)
			glVertex3d(cuts[i][0] * vft->hitmdl.rad, cuts[i][1] * vft->hitmdl.rad, 0.);
		glEnd();
		glPopMatrix();
	}
#endif
}

static void infantry_postframe(entity_t *pt){
	if(pt->enemy && !pt->enemy->active)
		pt->enemy = NULL;
}

static void bloodsmoke(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	double pixels;
	double (*cuts)[2];
	double rad;
	amat4_t mat;
	avec3_t velo;
	struct random_sequence rs;
	int i;
	GLubyte col[4] = {255,0,0,255}, colp[4] = {255,0,0,255}, cold[4] = {255,0,0,0};

	if(glcullFrustum(&pl->pos, .0003, dd->pgc))
		return;
	pixels = .0002 * fabs(glcullScale(&pl->pos, dd->pgc));

	if(pixels < 1)
		return;

	col[3] = 128 * pl->life / .5;
	colp[3] = 255 * pl->life / .5 * (1. - 1. / (pixels - 2.));
	BlendLight(col);
	BlendLight(colp);
	BlendLight(cold);

	cuts = CircleCuts(10);
	init_rseq(&rs, (int)pl);
	glPushMatrix();
	glTranslated(pl->pos[0], pl->pos[1], pl->pos[2]);
	glMultMatrixd(dd->invrot);
	rad = (pl->len - .0005 * pl->life * pl->life);
	glScaled(rad, rad, rad);
	glBegin(GL_TRIANGLE_FAN);
	glColor4ubv(col);
	glVertex3d(0., 0., 0.);
	glColor4ubv(cold);
	{
		double first;
		first = drseq(&rs) + .5;
		glVertex3d(first * cuts[0][1], first * cuts[0][0], 0.);
		for(i = 1; i < 10; i++){
			int k = i % 10;
			double r;
			r = drseq(&rs) + .5;
			glVertex3d(r * cuts[k][1], r * cuts[k][0], 0.);
		}
		glVertex3d(first * cuts[0][1], first * cuts[0][0], 0.);
	}
	glEnd();
	glPopMatrix();

	if(pixels < 3)
		return;

	glPushMatrix();
	glTranslated(pl->pos[0], pl->pos[1], pl->pos[2]);
	glGetDoublev(GL_MODELVIEW_MATRIX, mat);

	glBegin(GL_LINES);
	for(i = 0; i < 8; i++){
		avec3_t v0, v1, v2;
		VECCPY(velo, pl->velo);
		velo[0] = (drseq(&rs) - .5) * .0015;
		velo[1] = (drseq(&rs) - .5) * .0015;
		velo[2] = (drseq(&rs) - .5) * .0015;
		v0[0] = (drseq(&rs) - .5) * .00005;
		v0[1] = (drseq(&rs) - .5) * .00005;
		v0[2] = (drseq(&rs) - .5) * .00005;
		VECCPY(v1, v0);
		VECSADD(v1, velo, .5 - pl->life);
		VECCPY(v2, v1);
		VECADDIN(velo, pl->velo);
		VECSADD(v1, velo, .03);
		VECSADD(v2, velo, -.03);
		glColor4ubv(col);
		glVertex3dv(v1);
		glColor4ubv(cold);
		glVertex3dv(v2);
	}
	glEnd();
	glPopMatrix();
}

static void infantry_bullethit(entity_t *pt, warf_t *w, const bullet_t *pb){
	if(w->tell){
		double pos[3], velo[3];
		VECCPY(pos, pb->pos);
		VECNORM(velo, pb->velo);
		pos[0] += (drseq(&w->rs) - .5) * .0005;
		pos[1] += (drseq(&w->rs) - .5) * .0005;
		pos[2] += (drseq(&w->rs) - .5) * .0005;
		VECSCALEIN(velo, .005);
		VECADDIN(velo, pt->velo);
		AddTelineCallback3D(w->tell, pos, velo, .0005, NULL, NULL, w->gravity, bloodsmoke, w->irot, 0, .5);
	}
}


int headshots[2] = {0};
int blast_deaths[2] = {0};

static int infantry_takedamage(entity_t *pt, double damage, warf_t *w, int hitpart){
	struct tent3d_line_list *tell = w->tell;
	infantry_t *p = (infantry_t*)pt;
	int ret = 1;
	if(!pt->active)
		return 0;

	/* headshot! */
	if(hitpart == 2){
		damage *= 5.;
		if(0 <= pt->race && pt->race < 2){
			headshots[pt->race]++;
/*			CmdPrintf("Headshot! [%d,%d] %p", headshots[0], headshots[1], pt);*/
		}
	}

/*	playWave3DPitch("hit2.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime, 256 + rseq(&w->rs) % 256);*/
/*	playWave3D("hit.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .1);*/
	if(0 < pt->health && pt->health - damage <= 0){ /* death! */
		int i;
/*		effectDeath(w, pt);*/

		/* blast death */
		if(hitpart == 0){
			if(0 <= pt->race && pt->race < 2)
				blast_deaths[pt->race]++;
		}

		pt->deaths++;
		if(0 <= pt->race && pt->race < numof(w->races))
			w->races[pt->race].deaths++;
		if(pt->enemy)
			pt->enemy = NULL;
/*		pt->active = 0;*/
		p->walkphase = 0.;
		ret = 0;

		/* death effect of flesh! */
		if(w->tell) for(i = 0; i < 8; i++){
			double pos[3], velo[3];
			VECCPY(pos, pt->pos);
			VECCPY(velo, pt->velo);
			pos[0] += (drseq(&w->rs) - .5) * .001;
			pos[1] += (drseq(&w->rs)) * .0015;
			pos[2] += (drseq(&w->rs) - .5) * .001;
			velo[0] += (drseq(&w->rs) - .5) * .0025;
			velo[1] += (drseq(&w->rs)) * .0025;
			velo[2] += (drseq(&w->rs) - .5) * .0025;
			AddTelineCallback3D(w->tell, pos, velo, .0005, NULL, NULL, w->gravity, bloodsmoke, w->irot, 0, .5);
		}
	}
	pt->health -= damage;
	p->damagephase += damage;
	return ret;
}

static void infantry_control(entity_t *pt, warf_t *w, input_t *inputs, double dt){
	struct entity_private_static *vft = (struct entity_private_static*)pt->vft;
	infantry_t *p = (infantry_t*)pt;
	avec3_t pyr;

#if 1
	if(0 < pt->health){
		pt->pyr[1] += .001 * inputs->analog[0];
		pyr2quat(pt->rot, pt->pyr);
		QUATCNJIN(pt->rot);
		p->pitch -= .001 * inputs->analog[1];
	}
#else
	quat2pyr(w->pl->rot, pyr);

	pt->pyr[1] = pyr[1];
	pt->barrelp = -/*2**/pyr[0];
#endif

	if(0 < pt->health)
		pt->inputs = *inputs;

	if(inputs->change & PL_TAB){
		infantry_swapweapon(p);
/*		pt->weapon = !pt->weapon;*/
	}

	if(inputs->change & inputs->press & PL_Z){
		p->state = p->state == STATE_STANDING ? STATE_PRONE : STATE_STANDING;
		p->shiftphase = 1.;
	}

	{
		extern double fov;
		fov = p->arms[0].type != arms_mortar && p->aiming ? p->arms[0].type == arms_m40rifle ? .25 : .75 : 1.;
	}
}

static int infantry_getrot(struct entity *pt, warf_t *w, double (*rot)[16]){
	infantry_t *p = (infantry_t*)pt;
#if 1
	aquat_t q0, q, q1;
	q0[0] = 0.;
	q0[1] = sin((pt->pyr[1] + p->kick[1]) * .5);
	q0[2] = 0.;
	q0[3] = cos((pt->pyr[1] + p->kick[1]) * .5);
	q[0] = sin((p->pitch + p->kick[0]) * -.5);
	q[1] = 0.;
	q[2] = 0.;
	q[3] = cos((p->pitch + p->kick[0]) * -.5);
	QUATMUL(q1, q, q0);
	quat2mat(*rot, q1);
	return w->pl->control != pt;
#else
	if(w->pl->control == pt){
		avec3_t pyr;
		quat2pyr(w->pl->rot, pyr);
		VECSCALEIN(pyr, -1);
/*		pyr[0] += pt->barrelp;*/
		pyrimat(pyr, *rot);
	}
	else
		pyrmat(pt->pyr, *rot);
	return w->pl->control != pt;
#endif
/*	if(w->pl->control == pt){
		if(!p->controlled){
			p->controlled = 1;
			VECCPY(w->pl->pyr, pt->pyr);
			w->pl->pyr[0] -= pt->barrelp;
		}
		VECNULL(*pyr);
		return;
	}
	else{
		VECCPY(*pyr, pt->pyr);
	}*/
}

void infantry_drawHUD(entity_t *pt, warf_t *wf, const double irot[16], void (*gdraw)(void)){
	char buf[128];
	infantry_t *p = (infantry_t*)pt;
	glPushMatrix();

	glLoadIdentity();
	{
		GLint vp[4];
		int w, h, m;
		double left, bottom, *pyr;
		int tkills, tdeaths, ekills, edeaths;
		entity_t *pt2;
		amat4_t projmat;
		glGetIntegerv(GL_VIEWPORT, vp);
		w = vp[2], h = vp[3];
		m = w < h ? h : w;
		left = -(double)w / m;
		bottom = -(double)h / m;

		glGetDoublev(GL_PROJECTION_MATRIX, projmat);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(left, -left, bottom, -bottom, -1, 1);
		glMatrixMode(GL_MODELVIEW);

		if(wf->pl->control != pt){
			if(fmod(wf->pl->gametime, 1.) < .5){
				glRasterPos3d(.0 - 8. * (sizeof"AI CONTROLLED"-1) / m, .0 - 10. / m, -0.);
				gldprintf("AI CONTROLLED");
			}
		}
		else{
/*			glRasterPos3d(-left - 200. / m, bottom + 60. / m, -1);
			gldprintf("%c sub-machine gun", !pt->weapon ? '>' : ' ');
			glRasterPos3d(-left - 200. / m, bottom + 40. / m, -1);
			gldprintf("%c ???", pt->weapon ? '>' : ' ');*/
		}

		glRasterPos3d(left, -bottom - 20. / m, -0.);
		gldprintf("health: %g", pt->health);
		glRasterPos3d(left, -bottom - 40. / m, -0.);
		sprintf(buf, "shots: %d", pt->shoots2);
		putstring(buf);
		glRasterPos3d(left, -bottom - 60. / m, -0.);
		sprintf(buf, "kills: %d", pt->kills);
		putstring(buf);
		glRasterPos3d(left, -bottom - 80. / m, -0.);
		sprintf(buf, "deaths: %d", pt->deaths);
		putstring(buf);

		glRasterPos3d(left, bottom + 10. / m, -0.);
		gldprintf("cooldown: %g", p->cooldown2);
		glRasterPos3d(left, bottom + 30. / m, -0.);
		gldprintf("ammo: %d", p->arms[0].ammo);
		glRasterPos3d(left, bottom + 50. / m, -0.);
		gldprintf("angle: %g", deg_per_rad * pt->pyr[1]);

		if(p->damagephase != 0.){
			glColor4f(1., 0., 0., MIN(.5, p->damagephase * .5));
			glBegin(GL_QUADS);
			glVertex3i(-1, -1, -0);
			glVertex3i( 1, -1, -0);
			glVertex3i( 1,  1, -0);
			glVertex3i(-1,  1, -0);
			glEnd();
		}

		if(wf->pl->control != pt){
			glTranslated(0,0,-1);
			glRotated(deg_per_rad * wf->pl->pyr[2], 0., 0., 1.);
			glRotated(deg_per_rad * (wf->pl->pyr[0] + p->pitch), 1., 0., 0.);
			glRotated(deg_per_rad * wf->pl->pyr[1], 0., 1., 0.);
			glTranslated(0,0,1);
/*			glRotated(deg_per_rad * pt->pyr[2], 0., 0., 1.);
			glRotated(deg_per_rad * pt->pyr[0], 1., 0., 0.);
			glRotated(deg_per_rad * pt->pyr[1], 0., 1., 0.);*/
		}

		if(wf->pl->chasecamera == 0 || wf->pl->chasecamera == 3){
			glBegin(GL_LINES);
			glVertex3d(-.15, 0., -0.);
			glVertex3d(-.025, 0., -0.);
			glVertex3d( .15, 0., -0.);
			glVertex3d( .025, 0., -0.);
			glVertex3d(0., -.15, -0.);
			glVertex3d(0., -.025, -0.);
			glVertex3d(0., .15, -0.);
			glVertex3d(0., .025, -0.);
			glEnd();
		}

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixd(projmat);
		glMatrixMode(GL_MODELVIEW);
	}
	glPopMatrix();
}

static int infantry_tracehit(struct entity *pt, warf_t *w, const double src[3], const double dir[3], double rad, double dt, double *ret, double (*retp)[3], double (*retn)[3]){
	infantry_t *p = (infantry_t*)pt;
	struct hitbox *hb = pt->health <= 0. || p->state == STATE_PRONE ? infantry_dead_hb : infantry_hb;
	double sc[3];
	double best = dt, retf;
	int reti = 0, i, n;
/*	aquat_t rot, roty;
	roty[0] = 0.;
	roty[1] = sin(pt->turrety / 2.);
	roty[2] = 0.;
	roty[3] = cos(pt->turrety / 2.);
	QUATMUL(rot, pt->rot, roty);*/
	for(n = 0; n < numof(infantry_hb); n++){
		avec3_t org;
		aquat_t rot;
		quatrot(org, pt->rot, hb[n].org);
		VECADDIN(org, pt->pos);
		VECSADD(org, avec3_001, .001 * (p->state == STATE_STANDING ? p->shiftphase : 1. - p->shiftphase));
		QUATMUL(rot, pt->rot, hb[n].rot);
		for(i = 0; i < 3; i++)
			sc[i] = hb[n].sc[i] + rad;
		if((jHitBox(org, sc, rot, src, dir, 0., best, &retf, retp, retn)) && (retf < best)){
			best = retf;
			if(ret) *ret = retf;
			reti = n+1;
		}
	}
	return reti;
}

#endif




void Firearm::shoot(){
	if(!game->isServer())
		return;
	Entity *p = base;
	static const Vec3d nh0(0., 0., -1);
	double v = bulletSpeed();

	Mat4d gunmat;
	p->transform(gunmat);
	Bullet *pb = new Bullet(this, 1., bulletDamage());
	pb->velo = this->velo;
	Vec3d nh = gunmat.dvp3(nh0);

	// Generate variance vector from random number generator.
	// It's not really isotropic distribution, I'm afraid.
	Vec3d vecvar = Vec3d(w->rs.nextGauss(), w->rs.nextGauss(), w->rs.nextGauss()) * bulletVariance() * v;

	// Remove component parallel to heading direction.
	vecvar -= nh.sp(vecvar) * nh;

	// Accumulate desired direction and variance vector to determine the bullet's direction.
	pb->velo += (nh + vecvar) * v;

	// Offset for hand (and to make the bullet easier to see from eyes)
	Vec3d relpos(0.0003, -0.0002, -0.0003);
	pb->pos = this->pos + gunmat.dvp3(relpos);

	pb->life = bulletLifeTime();
//	pb->anim(t);

	w->addent(pb);

	--ammo;

}

void Firearm::reload(){
	ammo = maxammo();
}



const unsigned M16::classid = registerClass("M16", Conster<M16>);
const char *M16::classname()const{return "M16";}



M16::M16(Entity *abase, const hardpoint_static *hp) : st(abase, hp){
	reload();
}


const unsigned M40::classid = registerClass("M40", Conster<M40>);
const char *M40::classname()const{return "M40";}



M40::M40(Entity *abase, const hardpoint_static *hp) : st(abase, hp){
	reload();
}


/// Ignore invocation of GetGunPosCommand from Squirrel. It's not really a command
/// that may instruct the Entity to do something, but just returns parameters.
template<>
void EntityCommandSq<GetGunPosCommand>(HSQUIRRELVM, Entity &){}

IMPLEMENT_COMMAND(GetGunPosCommand, "GetGunPos")

