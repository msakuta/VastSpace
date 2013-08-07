/** \file
 * \brief Implementation of F-15 class.
 */

#include "F15.h"
#include "Bullet.h"
#include "Missile.h"
#include "tefpol3d.h"
#include "sqadapt.h"
#include "SqInitProcess-ex.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
}


//#define VALKIE_WALK_SPEED (.015)
//#define VALKIE_WALK_PHASE_SPEED (M_PI)


/* color sequences */
#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_blueburn[] = {
	{0.1, COLOR32RGBA(0,203,255,15)},
	{0.15, COLOR32RGBA(63,255,255,13)},
	{0.45, COLOR32RGBA(255,127,31,7)},
	{2.3, COLOR32RGBA(255,31,0,0)},
};
static const struct color_sequence cs_blueburn = DEFINE_COLSEQ(cnl_blueburn, (COLOR32)-1, 3.);
static const struct color_node cnl0_vapor[] = {
	{0.4, COLOR32RGBA(255,255,255,0)},
	{0.4, COLOR32RGBA(255,255,255,191)},
	{0.4, COLOR32RGBA(191,191,191,127)},
	{.4, COLOR32RGBA(127,127,127,0)},
};
static struct color_node cnl_vapor[] = {
	{0.4, COLOR32RGBA(255,255,255,0)},
	{0.4, COLOR32RGBA(255,255,255,191)},
	{0.4, COLOR32RGBA(191,191,191,127)},
	{.4, COLOR32RGBA(127,127,127,0)},
};
static const struct color_sequence cs_vapor = DEFINE_COLSEQ(cnl_vapor, (COLOR32)-1, 1.6);
static const struct color_node cnl_bluework[] = {
	{0.03, COLOR32RGBA(255, 255, 255, 255)},
	{0.035, COLOR32RGBA(128, 128, 255, 255)},
	{0.035, COLOR32RGBA(32, 32, 128, 255)},
};
static const struct color_sequence cs_bluework = DEFINE_COLSEQ(cnl_bluework, (COLOR32)-1, 0.1);


/// Template instantiation for adding debugWings() static member function
template<> void Entity::EntityRegister<F15>::sq_defineInt(HSQUIRRELVM v){
	register_closure(v, _SC("debugWings"), F15::sqf_debugWings);
}

Entity::EntityRegister<F15> F15::entityRegister("F15");

// Global constants loaded from F15.nut.
double F15::modelScale = 0.001 / 30.0;
double F15::hitRadius = 0.012;
double F15::defaultMass = 12000.;
double F15::maxHealthValue = 500.;
HitBoxList F15::hitboxes;
double F15::thrustStrength = .010;
F15::WingList F15::wings0;
std::vector<Vec3d> F15::wingTips;
std::vector<Vec3d> F15::gunPositions;
double F15::bulletSpeed = .78;
double F15::shootCooldown = .07;
std::vector<Vec3d> F15::cameraPositions;
Vec3d F15::hudPos;
double F15::hudSize;
GLuint F15::overlayDisp;
bool F15::debugWings = false;

SQInteger F15::sqf_debugWings(HSQUIRRELVM v){
	SQBool b;
	if(SQ_SUCCEEDED(sq_getbool(v, 2, &b)))
		debugWings = b;
	sq_pushbool(v, debugWings);
	return 1;
}


F15::F15(Game *game) : st(game), pf(nullptr), destPos(0,0,0){
	vapor.resize(wingTips.size());
	for(auto &i : vapor)
		i = nullptr;
}

F15::F15(WarField *w) : st(w), destPos(0,0,0){
	moi = .2; /* kilograms * kilometer^2 */
#ifndef DEDICATED
	TefpolList *tl = w->getTefpol3d();
	if(tl){
		pf = tl->addTefpolMovable(pos, velo, vec3_000, &cs_blueburn, TEP3_THICKER | TEP3_ROUGH, cs_blueburn.t);
	}
/*	sd = AllocSUFDecal(gsuf_fly);
	sd->drawproc = bullethole_draw;*/
#endif
	init();
	vapor.resize(wingTips.size());
	for(auto &i : vapor)
		i = tl->addTefpolMovable(pos, velo, vec3_000, &cs_vapor, TEP3_FAINT | TEP3_ROUGH, cs_vapor.t * 10);
}

void F15::init(){
	static bool initialized = false;
	if(!initialized){
		SqInit(game->sqvm, _SC(modPath() << "models/F15.nut"),
			SingleDoubleProcess(modelScale, "modelScale") <<=
			SingleDoubleProcess(hitRadius, "hitRadius") <<=
			SingleDoubleProcess(defaultMass, "mass") <<=
			SingleDoubleProcess(maxHealthValue, "maxhealth", false) <<=
			HitboxProcess(hitboxes) <<=
			SingleDoubleProcess(thrustStrength, "thrust") <<=
			WingProcess(wings0, "wings") <<=
			Vec3dListProcess(wingTips, "wingTips") <<=
			Vec3dListProcess(gunPositions, "gunPositions") <<=
			SingleDoubleProcess(bulletSpeed, "bulletSpeed", false) <<=
			SingleDoubleProcess(shootCooldown, "shootCooldown", false) <<=
			Vec3dListProcess(cameraPositions, "cameraPositions") <<=
			Vec3dProcess(hudPos, "hudPos") <<=
			SingleDoubleProcess(hudSize, "hudSize") <<=
			DrawOverlayProcess(overlayDisp));
		initialized = true;
	}
	mass = defaultMass;
	health = maxHealthValue;
	force.resize(wings0.size());
	this->weapon = 0;
	this->aileron = 0.;
	this->elevator = 0.;
	this->rudder = 0.;
	this->gearphase = 0.;
	this->cooldown = 0.;
	this->muzzle = 0;
	this->brk = 1;
	this->afterburner = 0;
	this->navlight = 0;
	this->gear = 0;
	this->missiles = 10;
	this->throttle = 0.;
/*	for(i = 0; i < numof(fly->bholes)-1; i++)
		fly->bholes[i].next = &fly->bholes[i+1];
	fly->bholes[numof(fly->bholes)-1].next = NULL;
	fly->frei = fly->bholes;
	for(i = 0; i < fly->sd->np; i++)
		fly->sd->p[i] = NULL;
	fly->st.inputs.press = fly->st.inputs.change = 0;*/
}


#if 0
static void cmd_afterburner(int argc, char *argv[]){
	if(ppl && ppl->control){
		if(ppl->control->vft == &fly_s || ppl->control->vft == &valkie_s){
			fly_t *p = (fly_t*)ppl->control;
			p->afterburner = !p->afterburner;
			if(p->afterburner && p->throttle < .7)
				p->throttle = .7;
		}
	}
}

static void fly_start_control(entity_t *pt, warf_t *w){
	static int init = 0;
	const char *s;
	if(!init){
		init = 1;
		RegisterAvionics();
		CmdAdd("afterburner", cmd_afterburner);
	}
	s = CvarGetString("fly_start_control");
	if(s){
		CmdExec(s/*"exec fly_control.cfg"*/);
	}
}

static void fly_end_control(entity_t *pt, warf_t *w){
	const char *s;
	s = CvarGetString("fly_end_control");
	if(s){
		CmdExec(s);
	}
/*	CmdExec("exec fly_uncontrol.cfg");*/
}

const static GLdouble
train_offset[3] = {0., 0.0025, 0.}, fly_radius = .002;

static int tryshoot(warf_t *w, entity_t *pt, const double epos[3], double phi0, double v, double damage, double variance, const avec3_t pos){
	fly_t *p = (fly_t*)pt;
	struct bullet *pb;
	double yaw = pt->pyr[1];
	double pitch = pt->pyr[0];
	double desired[2];
	double (*theta_phi)[2];

	pb = BulletNew(w, pt, damage);
	{
/*		double phi = (rot ? phi0 : yaw) + (drseq(&gsrs) - .5) * variance;
		double theta = pt->pyr[0] + (drseq(&gsrs) - .5) * variance;
		theta += acos(vx / v);*/
		double phi, theta;
		phi = phi0 + (drseq(&w->rs) - .5) * variance;
		theta = pitch + (drseq(&w->rs) - .5) * variance;
		VECCPY(pb->velo, pt->velo);
		pb->velo[0] +=  v * sin(phi) * cos(theta);
		pb->velo[1] +=  v * sin(theta);
		pb->velo[2] += -v * cos(phi) * cos(theta);
		pb->pos[0] = pt->pos[0] + pos[0];
		pb->pos[1] = pt->pos[1] + pos[1];
		pb->pos[2] = pt->pos[2] + pos[2];
	}
	return 1;
}
#endif



void F15::anim(double dt){

	Mat4d mat;
	transform(mat);

	{
		bool skip = !(.1 < -this->velo.sp(mat.vec3(2)));
		for(int i = 0; i < wingTips.size(); i++) if(vapor[i]){
			Vec3d pos = mat.vp3(wingTips[i]);
			vapor[i]->move(pos, vec3_000, cs_vapor.t, skip);
		}
	}
	if(this->pf){
		Vec3d pos = mat.vp3(Vec3d(0., .001, .0065));
		this->pf->move(pos, vec3_000, cs_blueburn.t, 0);
//		MoveTefpol3D(((fly_t *)pt)->pf, pos, avec3_000, cs_blueburn.t, 0);
	}

	if(0 < health && !controller){
		// Automatic stabilizer (Auto Pilot)
		Vec3d x_along_y = Vec3d(mat.vec3(0)[0], 0, mat.vec3(0)[2]).normin();
		double croll = -x_along_y.sp(mat.vec3(1)); // Current Roll
		double roll = croll * 0.25 + mat.vec3(2).sp(omg); // P + D
		Vec3d deltaPos(destPos - this->pos); // Delta position towards the destination
		deltaPos[1] = 0.; // Ignore height component
		double sdist = deltaPos.slen();
		if(0.5 * 0.5 < sdist){
			double sp = deltaPos.sp(mat.vec3(2));
			if(3. * 3. < sdist && 0.75 < sp)
				roll += 0.25;
			else if(3. * 3. < sdist || sp < 0){
				deltaPos.normin();
				roll += 0.25 * deltaPos.vp(-mat.vec3(2))[1];
			}
		}
		aileron = rangein(aileron + roll * dt, -1, 1);

		double trim = mat.vec3(2)[1] + velo[1] + fabs(croll) * 0.05; // P + D
		elevator = rangein(elevator + trim * dt, -1, 1);

		throttle = approach(throttle, rangein((0.5 - velo.len()) * 2., 0, 1), dt, 0);
	}


	st::anim(dt);
}

void F15::shoot(double dt){
	Vec3d velo0(0., 0., -bulletSpeed);
	if(dt <= this->cooldown)
		return;
/*	if(pt->vft == &valkie_s){
		valkie_t *p = (valkie_t *)pt;
		mat4rotx(mat, mat0, -p->torsopitch);
		if(p->arms[0].type != arms_none){
			reloadtime = arms_static[p->arms[0].type].cooldown;
		}
		if(p->arms[0].type == arms_valkierifle){
			New = BeamNew;
		}
	}
	else*/
	Mat4d mat;
	transform(mat);
	if(this->weapon){
		static const Vec3d fly_hardpoint[2] = {Vec3d(.005, .0005, -.000), Vec3d(-.005, .0005, -.000)};
		// Missiles are so precious that semi-automatic triggering is more desirable.
		// Also, do not try to shoot if there's no enemy locked on.
		while(0 < missiles && this->cooldown < dt && enemy){
			Missile *m = new Missile(this, 30., 500., enemy);
			w->addent(m);
			Vec3d v = mat.vp3(Vec3d(fly_hardpoint[missiles % 2]));
			m->setPosition(&v, &this->rot, &this->velo, &this->omg);
			this->missiles--;
			this->cooldown += 2.0;
//			playWAVEFile("missile.wav");
//			playWave3D("missile.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime + p->cooldown);
		}
	}
	else while(this->cooldown < dt){
		int i = 0;
		for(auto &it : gunPositions){
			Bullet *pb = new Bullet(this, 2., 5.);
			w->addent(pb);
			
			pb->mass = .005;
#if 0
			if(pt->vft == &valkie_s && dnm){
				avec3_t org;
				glPushMatrix();
				glLoadIdentity();
				TransYSDNM_V(dnm, valkie_boneset((valkie_t*)pt, NULL, NULL, 0), find_gun, org);
				VECSCALEIN(org, 1e-3);
				mat4vp3(pb->pos, mat, org);
				glPopMatrix();
			}
			else
#endif
				pb->pos = mat.vp3(it);
			pb->velo = mat.dvp3(velo0) + this->velo;
			for(int j = 0; j < 3; j++)
				pb->velo[j] += (drseq(&w->rs) - .5) * .005;
			pb->anim(dt - this->cooldown);
		};
		this->cooldown += shootCooldown;
//		this->shoots += 1;
	}
//	shootsound(pt, w, p->cooldown);
	this->muzzle = 1;
}


void F15::cockpitView(Vec3d &pos, Quatd &rot, int chasecam)const{
	Mat4d mat;
	int camera;
	{
		camera = chasecam;
		camera = MAX(0, MIN(cameraPositions.size(), camera));
//		*chasecam = camera;
	}
	transform(mat);
	if(camera == cameraPositions.size()+1){
		pos = this->pos;
		pos[0] += .05 * cos(w->war_time() * 2. * M_PI / 15.);
		pos[1] += .02;
		pos[2] += .05 * sin(w->war_time() * 2. * M_PI / 15.);
	}
#if 0
	else if(camera == 4 && pt->vft == &valkie_s){
		valkie_t *p = (valkie_t*)pt;
		amat4_t mat2;
		mat4roty(mat2, mat, /*5 * M_PI / 4.*/ + p->batphase * M_PI);
		mat4vp3(*pos, mat2, src[camera]);
	}
#endif
	else if(camera == cameraPositions.size()){
		const Player *player = game->player;
		Vec3d ofs = mat.dvp3(vec3_001);
		if(camera)
			ofs *= player ? player->viewdist : 1.;
		pos = this->pos + ofs;
	}
	else if(camera == cameraPositions.size()+2){
		Vec3d pos0;
		const double period = this->velo.len() < .1 * .1 ? .5 : 1.;
		struct contact_info ci;
		pos0[0] = floor(this->pos[0] / period + .5) * period;
		pos0[1] = 0./*floor(pt->pos[1] / period + .5) * period*/;
		pos0[2] = floor(this->pos[2] / period + .5) * period;
/*		if(w && w->->pointhit && w->vft->pointhit(w, pos0, avec3_000, 0., &ci))
			pos0 += ci.normal * ci.depth;
		else if(w && w->vft->spherehit && w->vft->spherehit(w, pos0, .002, &ci))
			pos0 += ci.normal * ci.depth;*/
		pos = pos0;
	}
	else{
		pos = mat.vp3(cameraPositions[camera]);
	}
	rot = this->rot;
}

int F15::takedamage(double damage, int hitpart){
	struct tent3d_line_list *tell = w->getTeline3d();
	int ret = 1;
	if(tell){
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
//	playWave3D("hit.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .1, w->realtime);
	if(0 < health && health - damage <= 0){
		int i;
		ret = 0;
/*		effectDeath(w, pt);*/
		for(i = 0; i < 32; i++){
			Vec3d pos = this->pos;
			Vec3d velo = this->velo;
			for(int j = 0; j < 3; j++)
				velo[j] = drseq(&w->rs) - .5;
			velo.normin() *= 0.1;
			pos += velo, 0.1;
			AddTeline3D(w->getTeline3d(), pos, velo, .005, quat_u, vec3_000, w->accel(pos, velo),
				COLOR32RGBA(255, 31, 0, 255), TEL3_HEADFORWARD | TEL3_THICK | TEL3_FADEEND | TEL3_REFLECT, 1.5 + drseq(&w->rs));
		}
		TefpolList *tepl = w->getTefpol3d();
		if(pf)
			pf->immobilize();
		pf = tepl->addTefpolMovable(this->pos, this->velo, vec3_000, &cs_firetrail, TEP3_THICKER | TEP3_ROUGH, cs_firetrail.t);
//		playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
	}
	health -= damage;
	if(health < -1000.){
		if(game->isServer())
			delete this;
		return 0;
	}
	return ret;
}
