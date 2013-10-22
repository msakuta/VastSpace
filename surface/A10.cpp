/** \file
 * \brief Implementation of A-10 class.
 */
#define NOMINMAX
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
	register_closure(v, _SC("gunFireEffect"), A10::sqf_gunFireEffect);
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
Vec3d A10::gunDirection(0,0,-1);
double A10::bulletSpeed = .78;
double A10::shootCooldown = .07;
A10::CameraPosList A10::cameraPositions;
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


A10::A10(Game *game) : st(game), pf(nullptr){
	vapor.resize(wingTips.size());
	for(auto &i : vapor)
		i = nullptr;
}

A10::A10(WarField *w) : st(w){
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
			Vec3dProcess(gunDirection, "gunDirection") <<=
			SingleDoubleProcess(bulletSpeed, "bulletSpeed", false) <<=
			SingleDoubleProcess(shootCooldown, "shootCooldown", false) <<=
			CameraPosProcess(cameraPositions, "cameraPositions") <<=
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
	this->brake = true;
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

	bool onfeet = taxi(dt);

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
		animAI(dt, onfeet);
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
			Bullet *pb = new ExplosiveBullet(this, 2., 50., 0.020, false);
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
	calcCockpitView(pos, rot, cameraPositions[chasecam % cameraPositions.size()]);
}

int A10::takedamage(double damage, int hitpart){
	Teline3List *tell = w->getTeline3d();
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

btCompoundShape *A10::getShape(){
	static btCompoundShape *shape = buildShape();
	return shape;
}
