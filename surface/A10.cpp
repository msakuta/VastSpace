/** \file
 * \brief Implementation of A-10 class.
 */

#include "A10.h"
#include "ExplosiveBullet.h"
#include "Missile.h"
#include "tefpol3d.h"
#include "sqadapt.h"
#include "Launcher.h"
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
template<> void Entity::EntityRegister<A10>::sq_defineInt(HSQUIRRELVM v){
	register_closure(v, _SC("debugWings"), A10::sqf_debugWings);
}

Entity::EntityRegister<A10> A10::entityRegister("A10");

// Global constants loaded from A10.nut.
double A10::modelScale = 0.001 / 30.0;
double A10::hitRadius = 0.012;
double A10::defaultMass = 12000.;
double A10::maxHealthValue = 500.;
HSQOBJECT A10::sqFire = sq_nullobj();
HitBoxList A10::hitboxes;
double A10::thrustStrength = .010;
A10::WingList A10::wings0;
std::vector<Vec3d> A10::wingTips;
std::vector<Vec3d> A10::gunPositions;
double A10::bulletSpeed = .78;
double A10::shootCooldown = .07;
std::vector<Vec3d> A10::cameraPositions;
Vec3d A10::hudPos;
double A10::hudSize;
GLuint A10::overlayDisp;
bool A10::debugWings = false;
std::vector<hardpoint_static*> A10::hardpoints;
StringList A10::defaultArms;
StringList A10::weaponList;

SQInteger A10::sqf_debugWings(HSQUIRRELVM v){
	SQBool b;
	if(SQ_SUCCEEDED(sq_getbool(v, 2, &b)))
		debugWings = b;
	sq_pushbool(v, debugWings);
	return 1;
}


A10::A10(Game *game) : st(game), pf(nullptr), destPos(0,0,0){
	vapor.resize(wingTips.size());
	for(auto &i : vapor)
		i = nullptr;
}

A10::A10(WarField *w) : st(w), destPos(0,0,0){
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

void A10::init(){
	static bool initialized = false;
	if(!initialized){
		SqInit(game->sqvm, _SC(modPath() << "models/A10.nut"),
			SingleDoubleProcess(modelScale, "modelScale") <<=
			SingleDoubleProcess(hitRadius, "hitRadius") <<=
			SingleDoubleProcess(defaultMass, "mass") <<=
			SingleDoubleProcess(maxHealthValue, "maxhealth", false) <<=
			SqCallbackProcess(sqFire, "fire") <<=
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
			DrawOverlayProcess(overlayDisp) <<=
			HardPointProcess(hardpoints) <<=
			StringListProcess(defaultArms, "defaultArms") <<=
			StringListProcess(weaponList, "weaponList"));
		assert(0 < weaponList.size());
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

	// Setup default configuration for arms
	for(int i = 0; i < hardpoints.size(); i++){
		if(defaultArms.size() <= i)
			return;
		ArmBase *arm = defaultArms[i] == "HydraRocketLauncher" ? (ArmBase*)new HydraRocketLauncher(this, hardpoints[i]) :
			defaultArms[i] == "HellfireLauncher" ? (ArmBase*)new HellfireLauncher(this, hardpoints[i]) :
			defaultArms[i] == "SidewinderLauncher" ? (ArmBase*)new SidewinderLauncher(this, hardpoints[i]) : NULL;
		if(arm){
			arms.push_back(arm);
			if(w)
				w->addent(arm);
		}
	}

/*	for(i = 0; i < numof(fly->bholes)-1; i++)
		fly->bholes[i].next = &fly->bholes[i+1];
	fly->bholes[numof(fly->bholes)-1].next = NULL;
	fly->frei = fly->bholes;
	for(i = 0; i < fly->sd->np; i++)
		fly->sd->p[i] = NULL;
	fly->st.inputs.press = fly->st.inputs.change = 0;*/
}




void A10::anim(double dt){

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

	// Align belonging arms at the end of the frame
	for(ArmList::iterator it = arms.begin(); it != arms.end(); ++it) if(*it)
		(*it)->align();
}

void A10::shoot(double dt){
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
	if(true || this->weapon){
		// Missiles are so precious that semi-automatic triggering is more desirable.
		// Also, do not try to shoot if there's no enemy locked on.
		HSQUIRRELVM v = game->sqvm;
		StackReserver sr(v);
		sq_pushobject(v, sqFire);
		sq_pushroottable(v);
		Entity::sq_pushobj(v, this);
		sq_pushfloat(v, dt);
		sq_call(v, 3, SQFalse, SQTrue);
	}
	else while(this->cooldown < dt){
		int i = 0;
		for(auto &it : gunPositions){
			Bullet *pb = new ExplosiveBullet(this, 2., 50.);
			w->addent(pb);

			pb->mass = .010;
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


void A10::cockpitView(Vec3d &pos, Quatd &rot, int chasecam)const{
	Mat4d mat;
	int camera;
	{
		camera = chasecam;
		camera = MAX(0, MIN(cameraPositions.size()+2, camera));
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
#if 0
	else if(camera == cameraPositions.size()){
		const Player *player = game->player;
		Vec3d ofs = mat.dvp3(vec3_001);
		if(camera)
			ofs *= player ? player->viewdist : 1.;
		pos = this->pos + ofs;
	}
#else
	else if(camera == cameraPositions.size()){
		if(lastMissile){
			pos = lastMissile->pos + lastMissile->rot.trans(Vec3d(0, 0.002, 0.005));
			rot = lastMissile->rot;
			return;
		}
		else
			pos = mat.vp3(cameraPositions[0]);
	}
#endif
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

int A10::takedamage(double damage, int hitpart){
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

SQInteger A10::sqGet(HSQUIRRELVM v, const SQChar *name)const{
	if(!scstrcmp(name, _SC("cooldown"))){
		sq_pushfloat(v, cooldown);
		return 1;
	}
	else if(!scstrcmp(name, _SC("lastMissile"))){
		Entity::sq_pushobj(v, lastMissile);
		return 1;
	}
	else if(!scstrcmp(name, _SC("arms"))){
		// Prepare an empty array in Squirrel VM for adding arms.
		sq_newarray(v, 0); // array

		// Retrieve library-provided "append" method for an array.
		// We'll reuse the method for all the elements, which is not exactly the same way as
		// an ordinally Squirrel codes evaluate.
		sq_pushstring(v, _SC("append"), -1); // array "append"
		if(SQ_FAILED(sq_get(v, -2))) // array array.append
			return sq_throwerror(v, _SC("append not found"));

		for(ArmList::const_iterator it = arms.begin(); it != arms.end(); it++){
			ArmBase *arm = *it;
			if(arm){
				sq_push(v, -2); // array array.append array
				Entity::sq_pushobj(v, arm); // array array.append array Entity-instance
				sq_call(v, 2, SQFalse, SQFalse); // array array.append
			}
		}

		// Pop the saved "append" method
		sq_poptop(v); // array

		return 1;
	}
	else
		return st::sqGet(v, name);
}

SQInteger A10::sqSet(HSQUIRRELVM v, const SQChar *name){
	if(!scstrcmp(name, _SC("cooldown"))){
		SQFloat retf;
		if(SQ_FAILED(sq_getfloat(v, 3, &retf)))
			return SQ_ERROR;
		cooldown = retf;
		return 0;
	}
	else if(!scstrcmp(name, _SC("lastMissile"))){
		lastMissile = dynamic_cast<Bullet*>(sq_refobj(v, 3));
		return 0;
	}
	else
		return st::sqSet(v, name);
}
