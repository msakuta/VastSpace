#include "Missile.h"
#include "material.h"
#include "Viewer.h"
#include "draw/WarDraw.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/suf/sufdraw.h>
#include <clib/gl/gldraw.h>
}

#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_firetrail[] = {
	{0.05, COLOR32RGBA(255, 255, 212, 0)},
	{0.05, COLOR32RGBA(255, 191, 191, 255)},
	{0.4, COLOR32RGBA(111, 111, 111, 255)},
	{0.3, COLOR32RGBA(63, 63, 63, 127)},
};
const struct color_sequence cs_firetrail = DEFINE_COLSEQ(cnl_firetrail, (COLOR32)-1, .8);

const float Missile::maxfuel = 120.;
const double Missile::maxspeed = 1.;
std::map<const Entity *, Missile *> Missile::targetmap;


Missile::Missile(Entity *parent, float life, double damage, Entity *target) : st(parent, life, damage), ft(0), fuel(maxfuel), throttle(0){
	WarSpace *ws = *parent->w;
	if(ws)
		pf = AddTefpolMovable3D(ws->tepl, pos, velo, avec3_000, &cs_firetrail, TEP3_THICK | TEP3_ROUGH, cs_firetrail.t);
	else
		pf = NULL;
	enemy = target;

	// Make list of missiles targetting to the same Entity.
	targetlist = targetmap[target];
	targetmap[target] = this;
}

Missile::~Missile(){
	if(enemy)
		unlinkTarget();
}

const unsigned Missile::classid = registerClass("Missile", Conster<Missile>);
const char *Missile::classname()const{return "Missile";}

#define SSM_ACCEL .10
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
	this->throttle = dr.len() * speedfactor + minspeed;
	this->omg = 3 * this->rot.trans(vec3_001).vp(dr.norm());
	if(SSM_ROTSPEED * SSM_ROTSPEED < this->omg.slen())
		this->omg.normin().scalein(SSM_ROTSPEED);
	this->rot = this->rot.quatrotquat(this->omg * dt);
}


void Missile::anim(double dt){
	WarField *oldw = w;
	if(pf)
		MoveTefpol3D(pf, pos, avec3_000, cs_firetrail.t, 0/*pf->docked*/);

	{
		Entity *target = enemy;
#if 1
		Vec3d epos, dv;
		if(0. < this->fuel){
			Vec3d delta;
			double dist, burnt, thrust;
			int flying = 0;

			if(target){
				static const double samspeed = .8;
				int i, n;
				delta = target->pos - pos;

				/* if the target goes behind, loose focus */
	/*			if(VECSP(delta, pb->velo) < 0.){
					pb->target = NULL;
					VECCPY(dv, pb->velo);
				}
				else*/{
					double speed;
					flying = 0/*((struct entity_private_static*)pb->target->vft)->flying(pb->target)*/;
					Vec3d zh = delta.norm();
			/*		estimate_pos(&epos, pb->target->pos, pb->target->velo, pb->pos, pb->velo, &sgravity, BULLETSPEED);*/
					speed = -velo.sp(zh);
					speed = MAX(maxspeed * 2., speed);
					dist = (target->pos - this->pos).len();
					epos = target->pos + (target->velo - velo) * (dist / speed * 1.2);
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
					dist = dv.len();
					burnt = dt * SSM_ACCEL /** (flying ? 3 : 1)*/;
					thrust = burnt / (1. + this->fuel / 20.)/* / dist*/;
					velo += zh * thrust;
					this->fuel -= burnt;
				}
			}
		}
#endif
#if 1
		if(target){
			double f = exp(-1.*dt);
			Vec3d lvelo = velo - target->velo; // Convert to target based velocity space
			lvelo = lvelo * f + dv.norm() * (1. - f);
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
				burnt = dt * 3. * SSM_ACCEL /** (flying ? 3 : 1)*/;
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

	// if we are transitting WarField or being destroyed, trailing tefpols should be marked for deleting.
	if(pf && w != oldw)
		ImmobilizeTefpol3D(pf);
}

void Missile::postframe(){
	if(w == NULL && enemy)
		unlinkTarget();
	if(enemy && enemy->w != w){
		targetmap.erase(enemy);
		enemy = NULL;
	}

	// The super class deals with enemy member, so we process enemy member before calling st::postframe.
	st::postframe();
}

void Missile::draw(wardraw_t *wd){
	static suf_t *suf;
	static suftex_t *suft;
	static bool init = false;

	if(wd->vw->gc->cullFrustum(pos, .01))
		return;
	double pixels = .005 * fabs(wd->vw->gc->scale(pos));
	if(pixels < 5)
		return;

	if(!init) do{
		init = true;
		FILE *fp;
		suf = CallLoadSUF("models/missile.bin");
		if(!suf)
			break;
		CallCacheBitmap("missile_body.bmp", "models/missile_body.bmp", NULL, NULL);
		suft = AllocSUFTex(suf);
	} while(0);
	if(suf){
		static const double normal[3] = {0., 1., 0.};
		double x;
		double pyr[3];
		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

		glPushMatrix();
		gldTranslate3dv(this->pos);
		gldMultQuat(this->rot);
#if 0
		for(int i = 0; i < nhitboxes; i++){
			Mat4d rot;
			glPushMatrix();
			gldTranslate3dv(hitboxes[i].org);
			rot = hitboxes[i].rot.tomat4();
			glMultMatrixd(rot);
			hitbox_draw(this, hitboxes[i].sc);
			glPopMatrix();
		}
#endif
		gldScaled(1e-5);
		glScalef(-1, 1, -1);
		DecalDrawSUF(suf, SUF_ATR, NULL, suft, NULL, NULL);
		glPopMatrix();

		glPopAttrib();
	}
}

void Missile::drawtra(wardraw_t *wd){
	Mat4d mat = rot.tomat4();
	mat.vec3(3) += pos;

	/* burning trail */
	if(/*p->target &&*/ 0. < fuel){
		struct gldBeamsData bd;
		Vec3d v0(.0, .0, .00300);
		const Vec3d &viewpos = wd->vw->pos;
		int lumi;
		struct random_sequence rs;
		double viewdist;
		const double scale = 10.;
		bd.cc = bd.solid = 0;
		init_rseq(&rs, (long)(wd->vw->viewtime * 1e6) + (unsigned long)this);
		lumi = rseq(&rs) % 256;
		Vec3d end = mat.vp3(v0);

		viewdist = (end - viewpos).len();
//		drawglow(end, wd->irot, .0015, COLOR32RGBA(255,255,255, 127 / (1. + viewdist)));
		glColor4ub(255,255,255,255);
		Vec4<GLubyte> col(255,255,255, 127 / (1. + viewdist));
		gldTextureGlow(end, .0020, col, wd->vw->irot);

		gldBeams(&bd, viewpos, end, .00001 * scale, COLOR32RGBA(3,127,191,0));
		v0[2] += .0003 * scale;
		end = mat.vp3(v0);
		gldBeams(&bd, viewpos, end, .00007 * scale, COLOR32RGBA(255,255,255,lumi));
		v0[2] += .0003 * scale;
		end = mat.vp3(v0);
		gldBeams(&bd, viewpos, end, .00009 * scale, COLOR32RGBA(255,127,63,lumi));
		v0[2] += .0003 * scale;
		end = mat.vp3(v0);
		gldBeams(&bd, viewpos, end, .00005 * scale, COLOR32RGBA(255,0,0,0));
	}
}

// proximity fuse
double Missile::hitradius()const{
	return .010;
}

void Missile::unlinkTarget(){
	// This case should not happen
	if(!targetmap[enemy]){
		targetmap.erase(enemy);
		return;
	}
	if(targetmap[enemy] == this){
		targetmap[enemy] = targetlist;
	}
	else{
		Missile *prev;
		for(prev = targetmap[enemy]; prev; prev = prev->targetlist) if(prev->targetlist != this){
			prev->targetlist = targetlist;
			break;
		}
	}
	if(!targetmap[enemy])
		targetmap.erase(enemy);
}

