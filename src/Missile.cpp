/** \file
 * \brief Implementation of Missile class.
 */
#define NOMINMAX
#include "Missile.h"
#include "Viewer.h"
#include "Game.h"
#include "glw/GLWchart.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
}

#include <algorithm>

const double Missile::modelScale = 1e-2;
const float Missile::maxfuel = 120000.;
const double Missile::maxspeed = 1000.;
const double Missile::acceleration = 100.;
const double Missile::speedDecayRate = 1.;

/// Construct on first use idiom.
Missile::TargetMap &Missile::targetmap(){
	static TargetMap *map = new TargetMap();
	return *map;
}


Missile::Missile(Entity *parent, float life, double damage, Entity *target) : st(parent, life, damage), ft(0), fuel(maxfuel), throttle(0)
{
	WarSpace *ws = *parent->w;
	initFpol();
	enemy = target;

	// Make list of missiles targetting to the same Entity.
	if(target){
		targetmap()[target].insert(this);
	}
}

Missile::~Missile(){
#ifndef DEDICATED
	if(pf)
		pf->immobilize();
#endif
	if(enemy)
		unlinkTarget();
}

const unsigned Missile::classid = registerClass("Missile", Conster<Missile>);
const char *Missile::classname()const{return "Missile";}

#define SSM_BLAST_FREQ 10.
#define SSM_MAX_GIBS 10
#define SSM_ROTSPEED (M_PI)

void Missile::steerHoming(double dt, const Vec3d &atarget, const Vec3d &targetvelo, double speedfactor, double minspeed){
	Vec3d target(atarget);
	Vec3d rdr = target - this->pos; // real dr
	Vec3d rdrn = rdr.norm();
	Vec3d dv = targetvelo - this->velo;
	Vec3d dvLinear = rdrn.sp(dv) * rdrn;
	Vec3d dvPlanar = dv - dvLinear;
	double dist = rdr.len();
	if(rdrn.sp(dv) < 0) // estimate only when closing
		target += dvPlanar * dist / dvLinear.len();
	Vec3d dr = this->pos - target;
	this->throttle = float(dr.len() * speedfactor + minspeed);
	this->omg = 3 * this->rot.trans(vec3_001).vp(dr.norm());
	if(SSM_ROTSPEED * SSM_ROTSPEED < this->omg.slen())
		this->omg.normin().scalein(SSM_ROTSPEED);
	this->rot = this->rot.quatrotquat(this->omg * dt);
}



void Missile::anim(double dt){
	static const double speedEpsilon = 1e-10;
	WarField *oldw = w;
	updateFpol();

#ifndef DEDICATED
	{
		// We plot size of missile target map.
		int serverMissiles = 0;
		for(TargetMap::iterator it = targetmap().begin(); it != targetmap().end(); ++it){
			if(it->first->getGame()->isServer())
				serverMissiles++;
		}

		gltestp::dstring labels[2] = {"ServerMissileMapSize", "ClientMissileMapSize"};
		double values[2] = {double(serverMissiles), double(targetmap().size() - serverMissiles)};
		GLWchart::addSamplesToCharts(labels, values, 2);
	}
#endif

	{
		Entity *target = enemy;
#if 1
		Vec3d epos, dv;
		if(0. < this->fuel){
			int flying = 0;

			if(target){
				Vec3d delta = target->pos - pos;

				/* if the target goes behind, loose focus */
	/*			if(VECSP(delta, pb->velo) < 0.){
					pb->target = NULL;
					VECCPY(dv, pb->velo);
				}
				else*/{
					flying = 0/*((struct entity_private_static*)pb->target->vft)->flying(pb->target)*/;
					Vec3d zh = delta.norm();
			/*		estimate_pos(&epos, pb->target->pos, pb->target->velo, pb->pos, pb->velo, &sgravity, BULLETSPEED);*/
					double speed = std::max(maxspeed * 2., -velo.sp(zh));
					double dist = (target->pos - this->pos).len();
					if(speedEpsilon < speed)
						epos = target->pos + (target->velo - velo) * (dist / speed * 1.2);
					else
						epos = target->pos;
					dv = epos - this->pos;
				}
			}
			else{
				dv = velo;
			}

	/*		VECSADD(pb->st.velo, dv, thrust);*/
			{
				const Vec3d zh0(0., 0., -1.);
				Mat4d mat = this->rot.tomat4();
				Vec3d zh = mat.vp3(zh0);
				if(0 < zh.sp(dv)){
					double burnt = dt * acceleration /** (flying ? 3 : 1)*/;
					double thrust = burnt / (1. + this->fuel / 60000.)/* / dist*/;
					velo += zh * thrust;
					this->fuel -= float(burnt);
				}
			}
		}
#endif
#if 1
		if(target && speedEpsilon < dv.slen()){
			double f = exp(-dt / speedDecayRate);
			Vec3d lvelo = velo - target->velo; // Convert to target based velocity space
			lvelo = lvelo * f + maxspeed * dv.norm() * (1. - f);
			velo = lvelo + target->velo;
			Vec3d omega = velo.norm().vp(rot.trans(vec3_001));
			rot = rot.quatrotquat(omega);
		}
#elif 1
	Vec3d epos, dv, dv2;
	if(0. < fuel){
		double dist, burnt, thrust;
		int flying = 0;

		Vec3d forward = -this->rot.trans(vec3_001);

		if(target) do{
			static const double samspeed = .8;
			int i, n;
			double vaa;
			Vec3d delta = target->pos - this->pos;

			/* if the target goes behind, loose focus */
/*			if(VECSP(delta, pb->velo) < 0.){
				pb->target = NULL;
				VECCPY(dv, pb->velo);
			}
			else*/
			if(0 && !centered){
				dv = (target->pos - this->pos).norm();
			}
			else{
				double speed, t;
				double dif;
				flying = 0/*((struct entity_private_static*)p->target->vft)->flying(p->target)*/;
				Vec3d zh = delta.norm();
				Vec3d deltavelo = target->velo - velo;
				speed = -deltavelo.sp(zh);
				Vec3d accel = w->accel(pos, velo);
/*				vaa = VECSP(deltavelo, accel);
				p->integral += 1e5 * vaa * dt;*/

				Quatd qc = rot.cnj();
				dv = zh;
				{
					Vec3d x = rot.trans(vec3_100);
					Vec3d y = rot.trans(vec3_010);
					dv += x * def[0];
					dv += y * def[1];
				}
				dv.normin();
				dif = forward.sp(dv);
				Vec3d localdef = qc.trans(dv);
				Vec3d deflection = zh;
				deflection += forward * -dif;
				if(.96 < dif){
					const double k = (7e2 * dt, 7e2 * dt, .5e2 * dt);
					integral += 1e6 * deflection.slen()/*(1. - dif)*/ * dt;
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

				deltavelo += zh * speed;
		/*		estimate_pos(&epos, pb->target->pos, pb->target->velo, pb->pos, pb->velo, &sgravity, BULLETSPEED);*/
				speed = velo.sp(zh);
				speed = MAX(.5, speed);
/*				speed = flying ? samspeed : BULLETSPEED * 2.;*/
				dist = (target->pos - pos).len();
				t = MIN(1., dist / speed);
/*				VECSADD(deltavelo, accel, -t * t);*/
				epos = target->pos /*+ deltavelo[i] * dist / speed*/;
		/*		epos[1] += .002;*/
				dv = epos - pos;
				dv.normin();
/*				VECSADD(dv, deflection, -1.);*/
/*				VECSADD(dv, deflection, p->integral);*/
				{
					double spx, spy;
					Vec3d x = rot.trans(vec3_100);
					Vec3d y = rot.trans(vec3_010);
					spx =  + 1 * def[0];
					spy =  + 1 * def[1];
					dv += x * spx;
					dv += y * spy;
				}
				dv.normin();
				dv2 = dv;
/*				t = dist / (speed * .8);
				epos[0] = p->target->pos[0] + deltavelo[0] * t;
				epos[1] = p->target->pos[1] + deltavelo[1] * t;
				epos[2] = p->target->pos[2] + deltavelo[2] * t;
				VECSUB(dv2, epos, p->st.pos);*/
			}
		} while(0);

		if(!target){
			dv = forward;
			dv2 = forward;
		}

		/* we gave up to fully simulate aerodynamic tensors, which makes it
		  unnecessarily harder. modified algorithm of auto-standup is used instead. */
#if 0
		{
			int i;
			for(i = 0; i < 10; i++){
				double sp, scale;
				avec3_t omega;
				aquat_t qc;
				quatrot(forward, p->rot, avec3_001);
				VECSCALEIN(forward, -1.);
				VECVP(omega, dv, forward);
				scale = -.15 * dt;
				if(scale < -1.)
					scale = -1.;
				VECSCALEIN(omega, scale);
				quatrotquat(p->rot, omega, p->rot);
			}
		}
#elif 1
		{
			long double sx, sy, len, len2, maxspeed;
/*			quatrot(xh, p->rot, avec3_100);
			quatrot(yh, p->rot, avec3_010);
			sx = VECSP(xh, dv);
			sy = VECSP(yh, dv);
			len = sqrt(sx * sx + sy * sy);*/
			maxspeed = M_PI * (velo.len() + 1.) * dt;
			Vec3d xh = forward.vp(dv);
			len = len2 = xh.len();
			len = asinl(len);
			if(maxspeed < len){
				len = maxspeed;
				if(!centered)
					centered = 1;
			}
			len = sinl(len / 2.);
			if(len && len2){
				Quatd qrot = xh * len / len2;
				qrot[3] = sqrt(1. - len * len);
				Quatd qres = qrot * rot;
				rot = qres.norm();
			}
		}
#elif 1 /* force direction (euler angles' pitch and yaw) */
		{
			double x = dv[0], y = dv[1], z = dv[2], phi, theta;
			phi = (atan2(x, z) + M_PI) / 2.;
			theta = (atan2(dv[1], sqrt(x * x + z * z)) + 0) / 2.;
/*			glRotated(phi * 360 / 2. / M_PI, 0., 1., 0.);
			glRotated(theta * 360 / 2. / M_PI, -1., 0., 0.);*/

			{
				aquat_t q, qi = {0,0,0,1}, qrot = {0};
				avec3_t omg = {0};
				qrot[1] = sin(phi);
				qrot[3] = cos(phi);
				QUATMUL(q, qi, qrot);
				qrot[1] = 0.;
				qrot[0] = sin(theta);
				qrot[3] = cos(theta);
				QUATMUL(p->rot, q, qrot);
			}
		}
#else /* force direction (quaternion native) */
		{
			avec3_t dr, v;
			aquat_t q;
			amat4_t mat;
			double pp;
			VECSCALE(dr, dv, -1);

			/* half-angle formula of trigonometry replaces expensive tri-functions to square root */
			q[3] = sqrt((dr[2] + 1.) / 2.) /*cos(acos(dr[2]) / 2.)*/;

			VECVP(v, avec3_001, dr);
			pp = sqrt(1. - q[3] * q[3]) / VECLEN(v);
			VECSCALE(q, v, pp);
			QUATCPY(p->rot, q);
		}
#endif

/*		VECSADD(pb->st.velo, dv, thrust);*/
		{
/*			amat4_t mat;
			avec3_t zh, zh0 = {0., 0., -1.};
			quat2mat(&mat, p->rot);
			mat4vp3(zh, mat, zh0);*/
			double sp;
			sp = forward.sp(dv);
			if(0 < sp){
				dist = dv.len();
				burnt = dt * 3. * acceleration /** (flying ? 3 : 1)*/;
				thrust = burnt / (1. + fuel / 20.)/* / dist*/;
				sp = exp(-thrust);
				velo *= sp;
				velo += forward * 2. * (1. - sp);
/*				VECSADD(p->st.velo, forward, thrust);*/
				fuel -= burnt;
			}
		}
	}
	else{
		if(this->pf)
			ImmobilizeTefpol3D(pf);
		w = NULL;
	}

#elif 1
		if(target)
			steerHoming(dt, target->pos, target->velo, .5, .1);
		{
			double spd = throttle * (.01);
			double consump = dt * throttle;
			Vec3d acc, acc0(0., 0., -1.);
			if(fuel <= consump){
				if(.1 < throttle)
					throttle = .1;
				fuel = 0.;
			}
			else
				fuel -= consump;
			acc = rot.trans(acc0);
			velo += acc * spd;
		}

#else
		{
			int i, j;
//			Mat3d rot, irot;
			double air = 0;
			double aileron, elevator, rudder;

			Mat4d rot4 = this->rot.tomat4();
			Mat4d irot4 = this->rot.cnj().tomat4();

//			air = exp(-pos[1] / 10.);
			aileron = 0/*rangein(-rot4[1] / 40., -M_PI / 4., M_PI / 4.)*/;
			elevator = 0/*rangein(-(pb->st.velo[1] - .025) * 2e0, -M_PI / 4., M_PI / 4.)*/;
			{
				Vec3d localdv;
				Quatd qpred, qomg;
				Mat4d irot;
				double dest, deste;
				qomg = this->omg * .5 / 2.;
				qomg[3] = 0.;
				qpred = qomg * this->rot;
				qpred += this->rot;
				irot = qpred.cnj().tomat4();
				if(0. < this->fuel && target && target->w == w && (localdv = irot4.dvp3(dv), localdv[2] < 0.)){
					deste = atan2(localdv[1], -localdv[2]) / 4.;
					dest = atan2(-localdv[0], -localdv[2]) / 4.;
//					VECCPY(((double(*)[3])&pb[1])[numof(wings0)], localdv);
				}
				else{
//					VECNULL(((double(*)[3])&pb[1])[numof(wings0)]);
					deste = dest = 0.;
				}
				elevator = rangein(deste, -M_PI / 4., M_PI / 4.);
				rudder = rangein(dest, -M_PI / 4., M_PI / 4.);
			}

			//MAT4TO3(rot, rot4);
			//MAT4TO3(irot, irot4);

			/* leave trail only if we're chasing */
			if(0. < this->fuel && this->pf){
				static const Vec3d nozzle0(0., 0., .0022);
				Vec3d nozzle = rot4.vp3(nozzle0);
				nozzle += this->pos;
				MoveTefpol3D(this->pf, nozzle, avec3_000, this->ft, 0);
			}
		}
#endif
	}
	st::anim(dt);

#ifndef DEDICATED
	// if we are transitting WarField or being destroyed, trailing tefpols should be marked for deleting.
//	if(pf && w != oldw)
//		ImmobilizeTefpol3D(pf);
#endif
}

void Missile::clientUpdate(double dt){
	st::clientUpdate(dt);
	updateFpol();
}

void Missile::postframe(){
	if(w == NULL && enemy)
		unlinkTarget();
	if(enemy && enemy->w != w){
		targetmap().erase(enemy);
		enemy = NULL;
	}

	// The super class deals with enemy member, so we process enemy member before calling st::postframe.
	st::postframe();
}

// proximity fuse
double Missile::getHitRadius()const{
	return 10.;
}

void Missile::unlinkTarget(){
}

void Missile::enterField(WarField *w){
	st::enterField(w);
	initFpol();
}


#ifdef DEDICATED
void Missile::initFpol(){pf = NULL;}
void Missile::updateFpol(){}
void Missile::draw(WarDraw*){}
void Missile::drawtra(WarDraw*){}
#endif
