/** \file
 * \brief Impelementation of Launcher class and its companions.
 */
#define NOMINMAX
#include "Launcher.h"
#ifndef DEDICATED
#include "tefpol3d.h"
#endif
#include "Game.h"

extern "C"{
#include <clib/mathdef.h>
}



//-----------------------------------------------------------------------------
//	HydraRocket implementation
//-----------------------------------------------------------------------------

#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_firetrail[] = {
	{0.05, COLOR32RGBA(255, 255, 212, 0)},
	{0.05, COLOR32RGBA(255, 191, 191, 255)},
	{0.4, COLOR32RGBA(111, 111, 111, 255)},
	{0.3, COLOR32RGBA(63, 63, 63, 127)},
};
const struct color_sequence cs_firetrail = DEFINE_COLSEQ(cnl_firetrail, (COLOR32)-1, .8);

Entity::EntityRegister<HydraRocket> HydraRocket::entityRegister("HydraRocket");

HydraRocket::HydraRocket(WarField *w) : st(w), pf(NULL), fuel(3.){
	mass = 6.2;
}

void HydraRocket::anim(double dt){
	if(0. < fuel){
		static const double accel = .20;
		double delta[3], dist;
		int flying = 0;

		Vec3d forward = -rot.trans(vec3_001);

		double burnt = dt * accel * 300. / damage/** (flying ? 3 : 1)*/;
		double thrust = mass / 10. * burnt / (1. + fuel / 20.)/* / dist*/;
		velo += forward * thrust;
		fuel -= burnt;
	}

	commonUpdate(dt);
	st::anim(dt);
}

void HydraRocket::clientUpdate(double dt){
	commonUpdate(dt);
}

void HydraRocket::enterField(WarField *w){
#ifndef DEDICATED
#endif
}

void HydraRocket::leaveField(WarField *w){
#ifndef DEDICATED
	if(pf){
		pf->immobilize();
		pf = NULL;
	}
#endif
}

void HydraRocket::commonUpdate(double dt){
#ifndef DEDICATED
	if(pf)
		pf->move(pos, vec3_000, cs_firetrail.t, 0);
	else{
		// Moved from enterField() because enterField() can be called before position is initialized
		WarSpace *ws = *w;
		if(ws && game->isClient())
			pf = ws->tepl->addTefpolMovable(pos, velo, avec3_000, &cs_firetrail, TEP3_THICKER | TEP3_ROUGH, cs_firetrail.t);
		else
			pf = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
//	HydraRocketLauncher implementation
//-----------------------------------------------------------------------------

void HydraRocketLauncher::anim(double dt){
	if(!base || !base->w)
		w = NULL;
	if(!w)
		return;

	if(online){
	}

	if(cooldown < dt)
		cooldown = 0.;
	else
		cooldown -= dt;
}

//-----------------------------------------------------------------------------
//	Hellfire implementation
//-----------------------------------------------------------------------------

Entity::EntityRegister<Hellfire> Hellfire::entityRegister("Hellfire");

Hellfire::Hellfire(WarField *w) : st(w), pf(NULL), fuel(3.){
	def[0] = def[1] = 0.;
	mass = 6.2;
}

void Hellfire::anim(double dt){
	commonUpdate(dt);
	st::anim(dt);
}

void Hellfire::clientUpdate(double dt){
	commonUpdate(dt);
	st::clientUpdate(dt);
}

void Hellfire::enterField(WarField *w){
}

void Hellfire::leaveField(WarField *w){
#ifndef DEDICATED
	if(pf){
		pf->immobilize();
		pf = NULL;
	}
#endif
}

SQInteger Hellfire::sqGet(HSQUIRRELVM v, const SQChar *name)const{
	if(!scstrcmp(name, _SC("target"))){
		sq_pushobj(v, const_cast<Hellfire*>(this));
		return 1;
	}
	else
		return st::sqGet(v, name);
}

SQInteger Hellfire::sqSet(HSQUIRRELVM v, const SQChar *name){
	if(!scstrcmp(name, _SC("target"))){
		target = sq_refobj(v, 3);
		return 0;
	}
	else
		return st::sqSet(v, name);
}

void Hellfire::commonUpdate(double dt){
	static const double SSM_ACCEL = .20;

	if(0. < fuel){
		Vec3d dv, dv2;

		Vec3d forward = -this->rot.trans(vec3_001);

		if(target) do{
			static const double samspeed = .8;
			int i, n;
			double vaa;
			Vec3d delta = target->pos - this->pos;

			Vec3d zh = delta.norm();
			Vec3d accel = w->accel(this->pos, this->velo);

			Quatd qc = this->rot.cnj();
			dv = zh
				+ this->rot.trans(vec3_100) * def[0]
				+ this->rot.trans(vec3_010) * def[1];
			dv.normin();
			double dif = forward.sp(dv);
			Vec3d localdef = qc.trans(dv);
			Vec3d deflection = zh - forward * dif;
			if(.96 < dif){
				const double k = (7e2 * dt, 7e2 * dt, .5e2 * dt);
				integral += 1e5 * deflection.slen() * dt;
				def[0] += k * localdef[0];
				def[1] += k * localdef[1];
			}
			else if(0. < dif){
				def[0] = def[1] = 0.;
			}
			else{
				target = NULL;
				break;
			}

			Vec3d deltavelo = target->velo - velo;
			deltavelo -= zh * deltavelo.sp(zh);
			double speed = std::max(0.5, velo.sp(zh));
			double dist = (target->pos - this->pos).len();
			double t = std::min(1., dist / speed);
			dv = (target->pos - this->pos).norm()
				+ this->rot.trans(vec3_100) * def[0]
				+ this->rot.trans(vec3_010) * def[1];
			dv.normin();
			dv2 = dv;
		}while(0);
		else{
			dv = forward;
			dv2 = forward;
		}

		{
			long double maxspeed = M_PI * (this->velo.len() + 1.) * dt;
			Vec3d xh = forward.vp(dv);
			long double len = xh.len();
			long double angle = asinl(len);
			if(maxspeed < angle){
				angle = maxspeed;
				if(!centered)
					centered = true;
			}
			if(len && angle){
				this->rot *= Quatd::rotation(angle, xh);
			}
		}

		double sp = forward.sp(dv);
		if(0 < sp){
			double burnt = dt * 3. * SSM_ACCEL;
			double thrust = burnt / (1. + fuel / 20.);
			sp = exp(-thrust);
			this->velo *= sp;
			this->velo += forward * (1. - sp);
			fuel -= burnt;
		}
	}

	// Damping by air
	this->omg *= 1. / (dt * .4 + 1.);
	this->velo *= 1. / (dt * .01 + 1.);

#ifndef DEDICATED
	if(0 < fuel && pf)
		pf->move(pos, vec3_000, cs_firetrail.t, 0);
	else{
		// Moved from enterField() because enterField() can be called before position is initialized
		WarSpace *ws = *w;
		if(ws && game->isClient())
			pf = ws->tepl->addTefpolMovable(pos, velo, avec3_000, &cs_firetrail, TEP3_THICKER | TEP3_ROUGH, cs_firetrail.t);
		else
			pf = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
//	HellfireLauncher implementation
//-----------------------------------------------------------------------------

void HellfireLauncher::anim(double dt){
	if(!base || !base->w)
		w = NULL;
	if(!w)
		return;

	if(online){
	}

	if(cooldown < dt)
		cooldown = 0.;
	else
		cooldown -= dt;
}

//-----------------------------------------------------------------------------
//	Sidewinder implementation
//-----------------------------------------------------------------------------

Entity::EntityRegister<Sidewinder> Sidewinder::entityRegister("Sidewinder");

//-----------------------------------------------------------------------------
//	SidewinderLauncher implementation
//-----------------------------------------------------------------------------

void SidewinderLauncher::anim(double dt){
	if(!base || !base->w)
		w = NULL;
	if(!w)
		return;

	if(online){
	}

	if(cooldown < dt)
		cooldown = 0.;
	else
		cooldown -= dt;
}

//-----------------------------------------------------------------------------
//	Launcher implementation
//-----------------------------------------------------------------------------

SQInteger Launcher::sqGet(HSQUIRRELVM v, const SQChar *name)const{
	if(!scstrcmp(name, _SC("ammo"))){
		sq_pushfloat(v, ammo);
		return 1;
	}
	else
		return st::sqGet(v, name);
}

SQInteger Launcher::sqSet(HSQUIRRELVM v, const SQChar *name){
	if(!scstrcmp(name, _SC("ammo"))){
		SQInteger reti;
		if(SQ_FAILED(sq_getinteger(v, 3, &reti)))
			return SQ_ERROR;
		ammo = reti;
		return 0;
	}
	else
		return st::sqSet(v, name);
}
