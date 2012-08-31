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
#include "sqadapt.h"
#include "GetFov.h"

extern "C"{
#include <clib/mathdef.h>
#include <clib/cfloat.h>
}

#include <sstream>




#define putstring(a) gldPutString(a)

#define TURRETROTSPEED (2. * M_PI)
#define BULLETSPEED .5
#define INFANTRY_WALK_PHASE_SPEED (M_PI * 1.3)

/// \brief An EntityCommand that passively alter rotation of a Soldier in the server.
struct SetSoldierRotCommand : public SerializableCommand{
	COMMAND_BASIC_MEMBERS(SetSoldierRotCommand, SerializableCommand);
	SetSoldierRotCommand(const Quatd &rot = Quatd(0,0,0,1)) : rot(rot){} 
	void serialize(SerializeContext &sc){
		sc.o << rot;
	}
	void unserialize(UnserializeContext &sc){
		sc.i >> rot;
	}
	Quatd rot;
};



Entity::EntityRegister<Soldier> Soldier::entityRegister("Soldier");
const unsigned Soldier::classid = registerClass("Soldier", Conster<Soldier>);
const char *Soldier::classname()const{return "Soldier";}

HitBoxList Soldier::hitboxes;
double Soldier::modelScale = 2e-6;
double Soldier::hitRadius = 0.002;
double Soldier::defaultMass = 60; // kilograms
double Soldier::maxHealthValue = 10.;
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



void Soldier::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << forcedRot;
	sc.o << forcedEnemy;
	sc.o << aiming;
	sc.o << swapphase;
	sc.o << hurt;
	sc.o << hurtdir;
	sc.o << cooldown2;
	sc.o << arms[0];
	sc.o << arms[1];
	sc.o << hookpos;
	sc.o << hookvelo;
	sc.o << hookedEntity;
	sc.o << hookshot;
	sc.o << hooked;
	sc.o << hookretract;
	if(controller){
		sc.o << kick[0];
		sc.o << kick[1];
		sc.o << kickvelo[0];
		sc.o << kickvelo[1];
	}

	// Once we set the rotation, we can forget about forcing the client to obey it.
	forcedRot = false;
}

void Soldier::unserialize(UnserializeContext &usc){
	Quatd oldrot = this->rot;

	st::unserialize(usc);
	usc.i >> forcedRot;
	usc.i >> forcedEnemy;
	usc.i >> aiming;
	usc.i >> swapphase;
	usc.i >> hurt;
	usc.i >> hurtdir;
	usc.i >> cooldown2;
	usc.i >> arms[0];
	usc.i >> arms[1];
	usc.i >> hookpos;
	usc.i >> hookvelo;
	usc.i >> hookedEntity;
	usc.i >> hookshot;
	usc.i >> hooked;
	usc.i >> hookretract;
	if(controller){
		usc.i >> kick[0];
		usc.i >> kick[1];
		usc.i >> kickvelo[0];
		usc.i >> kickvelo[1];
	}

	// Restore original rotation if it's not forced from the server.
	if(!forcedRot)
		this->rot = oldrot;
}

DERIVE_COMMAND_EXT(ReloadWeaponCommand, SerializableCommand);
DERIVE_COMMAND_EXT(SwitchWeaponCommand, SerializableCommand);

IMPLEMENT_COMMAND(ReloadWeaponCommand, "ReloadWeapon")
IMPLEMENT_COMMAND(SwitchWeaponCommand, "SwitchWeapon")



double Soldier::getHitRadius()const{
	return hitRadius;
}

double Soldier::maxhealth()const{
	return maxHealthValue;
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
void Soldier::bullethit(const Bullet *){}
void Soldier::hookHitEffect(const otjEnumHitSphereParam &param){}
bool Soldier::getGunPos(GetGunPosCommand &ggp){
	ggp.pos = this->pos;
	ggp.rot = this->rot;
	ggp.gunRot = this->rot.rotate(kick[1], 0, 1, 0).rotate(kick[0], -1, 0, 0);
	return true;
}
void M16::draw(WarDraw *){}
void M40::draw(WarDraw *){}
#endif


int infantry_random_weapons = 0;


Soldier::Soldier(Game *g) : st(g){
	init();
}

Soldier::Soldier(WarField *w) : st(w){
	init();
}

Soldier::~Soldier(){
	if(game->isServer()){
		// This Entity "owns" the equipments, so delete them along with.
		// The equipments could be left alive for being picked up by other Soldiers, but we need to
		// implement them to survive without an owner.
		for(int i = 0; i < numof(arms); i++){
			delete arms[i];
		}
	}
}

void Soldier::init(){
	// There could be the case the Game is both a server and a client.
	// In that case, we want to run both initializers.
	if(game->isClient()){
		static bool initialized = false;
		if(!initialized){
			sq_init(modPath() << _SC("models/Soldier.nut"),
				HitboxProcess(hitboxes) <<=
				ModelScaleProcess(modelScale) <<=
				SingleDoubleProcess(hitRadius, "hitRadius", false) <<=
				MassProcess(defaultMass) <<=
				SingleDoubleProcess(maxHealthValue, "maxhealth", false) <<=
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

		arms[0] = arms[1] = NULL;
	}
	if(game->isServer()){
		static bool initialized = false;
		if(!initialized){
			sq_init(modPath() << _SC("models/Soldier.nut"),
				HitboxProcess(hitboxes) <<=
				ModelScaleProcess(modelScale) <<=
				SingleDoubleProcess(hitRadius, "hitRadius", false) <<=
				MassProcess(defaultMass) <<=
				SingleDoubleProcess(maxHealthValue, "maxhealth", false) <<=
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
		state = STATE_STANDING;
		reloading = 0;
		muzzle = 0;
		aiming = false;
		arms[0] = new M16(this, soldierHP[0]);
		arms[1] = new M40(this, soldierHP[1]);
		if(w) for(int i = 0; i < numof(arms); i++) if(arms[i])
			w->addent(arms[i]);
		hookshot = false;
		hooked = false;
		hookretract = false;
	}
	hurtdir = vec3_000;
	hurt = 0.;
	forcedEnemy = false;
	kick[0] = kick[1] = 0.;
	kickvelo[0] = kickvelo[1] = 0.;
	aimphase = 0.;
	walkphase = 0.;
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

	// Remember that this rotation is set explicitly; if it's the server, the client must obey it.
	if(rot)
		forcedRot = true;
}

void Soldier::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	const Soldier *p = this;
	static const Vec3d ofs[2] = {Vec3d(.003, .001, -.002), Vec3d(.00075, .002, .003),};
	Mat4d mat, mat2;
	seatid = (seatid + 4) % 4;

	rot = this->rot.rotate(p->kick[1], 0, 1, 0).rotate(p->kick[0], -1, 0, 0);

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
	if(cooldown2 <= 0. && reloadphase <= 0.){
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
		aiming = false;
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

	if(game->isServer() && health <= 0. /*&& 120. < walkphase*/){
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

#if 1
	if(!controller && 0 < health){
		Vec3d epos;

		/* find enemy logic */
		if(!enemy){
			findEnemy();
		}

		if(enemy && enemy->health <= 0.)
			enemy = NULL;

		if(arms[0]->ammo <= 0 && cooldown2 == 0. && !reloading){
			reload();
		}

		/* estimating enemy position and wheeling towards enemy */
		if(enemy){
//			double bulletspeed = p->arms[0].type == arms_mortar ? .05 : p->arms[0].type == arms_m40rifle ? .770 : p->arms[0].type == arms_shotgun ? BULLETSPEED * .75 : BULLETSPEED;
			double bulletspeed = BULLETSPEED;
			double desired[2];
			static const Vec3d pos0(0, .0015, 0);
			int shootable = 0;
/*			if(((struct entity_private_static*)pt->enemy->vft)->flying(pt->enemy))
				VECCPY(epos, pt->enemy->pos);
			else
				estimate_pos(&epos, pt->enemy->pos, pt->enemy->velo, pt->pos, pt->velo, &sgravity, bulletspeed, w->map, w, ((struct entity_private_static*)pt->enemy->vft)->flying(pt->enemy));*/
			epos = enemy->pos;
//			if(enemy->classname() == this->classname() && ((Soldier*)static_cast<Entity*>(enemy))->state == STATE_PRONE)
//				epos -= pos0;
			if(p->state == STATE_PRONE)
				epos += pos0;
//			double phi = atan2(epos[0] - this->pos[0], -(epos[2] - this->pos[2]));
			Vec3d enemyDelta = epos - this->pos;
			double sd = enemyDelta.slen();
			z = sd < .01 * .01 ? 1 : -1;
			Vec3d enemyDirection = enemyDelta.norm();
			Vec3d forward = this->rot.trans(Vec3d(0,0,-1));
			this->rot = this->rot.quatrotquat(forward.vp(enemyDirection) * dt);

			// It's no use approaching closer than 10 meters.
			// Altering inputs member is necessary because Autonomous::maneuver() use it.
			inputs.press = 0.01 * 0.01 < sd && 0.9 < forward.sp(enemyDirection);

//			pyr[1] = approach(pyr[1], phi, TURRETROTSPEED * dt, 2 * M_PI);
//			pyr2quat(pt->rot, pt->pyr);

#define EPSILON 1e-5
			{
/*				double dst = desired[0] - pt->pyr[0];
				avec3_t pyr;
				amat4_t mat;
				shootable = 1;
				p->pitch = rangein(approach(p->pitch + M_PI, dst + M_PI, TURRETROTSPEED * dt, 2 * M_PI) - M_PI, vft->barrelmin, vft->barrelmax);
				pyr[0] = p->pitch;
				pyr[1] = pt->pyr[1];
				pyr[2] = 0.;*/
				if(/*!(p->arms[0].type == arms_rpg7 && (pyrmat(pyr, mat), VECSP(normal, &mat[8]) < 0.)) &&
					desired[0] - EPSILON <= p->pitch && p->pitch <= desired[0] + EPSILON &&
					desired[1] - EPSILON <= pt->pyr[1] && pt->pyr[1] <= desired[1] + EPSILON &&*/
					(p->kick[0] + p->kick[0] + p->kick[1] * p->kick[1]) < 1. / (sd / .01 / .01) * M_PI / 8. * M_PI / 8.)
					i |= PL_ENTER;
			}

/*			if(p->arms[0].type == arms_mortar && shootable){
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

	if(0. < swapphase){
		aiming = false;
		aimphase = 0.;
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
				p->aiming = 0;
		}
	}

//	pyr2quat(pt->rot, pt->pyr);
//	QUATCNJIN(pt->rot);

	/* shooter logic */
	p->muzzle = 0;
	if(!p->reloading && i & (PL_ENTER | PL_LCLICK) && !(i & PL_SHIFT)) while(p->cooldown2 < dt){
		amat4_t gunmat;
		double kickf = arms[0]->shootRecoil() * g_recoil_kick_factor * (p->state == STATE_PRONE ? .3 : 1.);
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
			p->kickvelo[0] += kickf * (w->rs.nextd() - .3);
			p->kickvelo[1] += kickf * (w->rs.nextd() - .5);
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

	// You actually change weapon at the middle of swap motion.
	if(1. < swapphase && swapphase - dt < 1.){
		Firearm *tmp = arms[1];
		arms[1] = arms[0];
		arms[0] = tmp;
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
	aimphase = approach(aimphase, !!aiming, 3. * dt, 0.);

	// Hurt effect decay.
	hurt = approach(hurt, 0, dt, 0.);

	// Reset hurt direction if the pain is gone.
	if(hurt == 0.)
		hurtdir = vec3_000;
}

void Soldier::clientUpdate(double dt){
	anim(dt);
}

void Soldier::control(const input_t *inputs, double dt){
	const double speed = .001 / 2. * getFov();
	st::control(inputs, dt);

	// Mouse-aim direction is only controlled by the client; server is too slow to process it.
	Quatd arot;
	getPosition(NULL, &arot);

	Vec3d xomg = arot.trans(Vec3d(0, -inputs->analog[0] * speed, 0));
	Vec3d yomg = arot.trans(Vec3d(-inputs->analog[1] * speed, 0, 0));
	arot = arot.quatrotquat(xomg);
	arot = arot.quatrotquat(yomg);

	// Restore forcedRot if setPosition() alters it, because it shouldn't be forced from control().
	bool oldForcedRot = forcedRot;
	setPosition(NULL, &arot, NULL, NULL);
	forcedRot = oldForcedRot;

	if(!game->isServer()){
		SetSoldierRotCommand cmd(arot);
		CMEntityCommand::s.send(this, cmd);
	}

	// Toggle aim with mouse right click (rise edge)
	if(inputs->change & inputs->press & PL_RCLICK){
		aiming = !aiming;
	}

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

double Soldier::getFov()const{
	if(arms[0])
		return aimphase * arms[0]->aimFov() + (1. - aimphase) * 1.;
	else
		return 1.;
}

int Soldier::tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal){
	int iret = st::tracehit(start, dir, rad, dt, ret, retp, retnormal);
	// Remember hit direction for HUD effect.
	// TODO: ignore irrelevant tracehits (checking line of sights, easy collision checking, etc.)
	if(iret)
		hurtdir = dir.norm();
	return iret;
}


int Soldier::takedamage(double damage, int hitpart){
	st::takedamage(damage, hitpart);
	// Accumulate hurt value for HUD effect.
	hurt += damage;
	return 1;
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
	else if(InterpretCommand<ReloadWeaponCommand>(com)){
		reload();
		return true;
	}
	else if(GetGunPosCommand *ggp = InterpretCommand<GetGunPosCommand>(com)){
		return getGunPos(*ggp);
	}
	else if(SetSoldierRotCommand *ssr = InterpretCommand<SetSoldierRotCommand>(com)){
		bool oldForcedRot = forcedRot;
		setPosition(NULL, &ssr->rot, NULL, NULL);
		forcedRot = oldForcedRot;
		return true;
	}
	else if(GetFovCommand *gf = InterpretCommand<GetFovCommand>(com)){
		gf->fov = getFov();
		return true;
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

	Quatd gunRot = p->rot;
	GetGunPosCommand ggp(0);
	if(base->command(&ggp)){
		gunRot = ggp.gunRot;
	}

	Bullet *pb = new Bullet(this, 1., bulletDamage());
	pb->mass = 0.03 * bulletDamage(); // 10 grams
	pb->velo = this->velo;
	Vec3d nh = gunRot.trans(nh0);

	// Generate variance vector from random number generator.
	// It's not really isotropic distribution, I'm afraid.
	Vec3d vecvar = Vec3d(w->rs.nextGauss(), w->rs.nextGauss(), w->rs.nextGauss()) * bulletVariance() * v;

	// Remove component parallel to heading direction.
	vecvar -= nh.sp(vecvar) * nh;

	// Accumulate desired direction and variance vector to determine the bullet's direction.
	pb->velo += (nh + vecvar) * v;

	// Offset for hand (and to make the bullet easier to see from eyes)
	Vec3d relpos(0.0002, -0.000, -0.0003);
	pb->pos = this->pos + p->rot.trans(relpos);

	// Impulse
	base->velo -= pb->velo * pb->mass / base->mass;

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
int M16::maxAmmoValue = 20;
double M16::shootCooldownValue = 0.1;
double M16::bulletSpeedValue = 0.7;
double M16::bulletDamageValue = 1.0;
double M16::bulletVarianceValue = 0.01;
double M16::aimFovValue = 0.7;
double M16::shootRecoilValue = M_PI / 128.;



M16::M16(Entity *abase, const hardpoint_static *hp) : st(abase, hp){
	init();
	reload();
}

void M16::init(){
	static bool initialized = false;
	if(!initialized){
		SqInit(game->sqvm, modPath() << _SC("models/M16.nut"),
			IntProcess(maxAmmoValue, "maxammo", false) <<=
			SingleDoubleProcess(shootCooldownValue, "shootCooldown", false) <<=
			SingleDoubleProcess(bulletSpeedValue, "bulletSpeed", false) <<=
			SingleDoubleProcess(bulletDamageValue, "bulletDamage", false) <<=
			SingleDoubleProcess(bulletVarianceValue, "bulletVariance", false) <<=
			SingleDoubleProcess(aimFovValue, "aimFov", false) <<=
			SingleDoubleProcess(shootRecoilValue, "shootRecoil", false)
			);
		initialized = true;
	}
}


const unsigned M40::classid = registerClass("M40", Conster<M40>);
const char *M40::classname()const{return "M40";}
int M40::maxAmmoValue = 5;
double M40::shootCooldownValue = 1.5;
double M40::bulletSpeedValue = 1.0;
double M40::bulletDamageValue = 5.0;
double M40::bulletVarianceValue = 0.001;
double M40::aimFovValue = 0.2;
double M40::shootRecoilValue = M_PI / 32.;


M40::M40(Entity *abase, const hardpoint_static *hp) : st(abase, hp){
	init();
	reload();
}

void M40::init(){
	static bool initialized = false;
	if(!initialized){
		SqInit(game->sqvm, modPath() << _SC("models/M40.nut"),
			IntProcess(maxAmmoValue, "maxammo", false) <<=
			SingleDoubleProcess(shootCooldownValue, "shootCooldown", false) <<=
			SingleDoubleProcess(bulletSpeedValue, "bulletSpeed", false) <<=
			SingleDoubleProcess(bulletDamageValue, "bulletDamage", false) <<=
			SingleDoubleProcess(bulletVarianceValue, "bulletVariance", false) <<=
			SingleDoubleProcess(aimFovValue, "aimFov", false) <<=
			SingleDoubleProcess(shootRecoilValue, "shootRecoil", false)
			);
		initialized = true;
	}
}


/// Ignore invocation of GetGunPosCommand from Squirrel. It's not really a command
/// that may instruct the Entity to do something, but just returns parameters.
template<>
void EntityCommandSq<GetGunPosCommand>(HSQUIRRELVM, Entity &){}

IMPLEMENT_COMMAND(GetGunPosCommand, "GetGunPos")


template<>
void EntityCommandSq<SetSoldierRotCommand>(HSQUIRRELVM, Entity &){}

IMPLEMENT_COMMAND(SetSoldierRotCommand, "SetSoldierRot")
