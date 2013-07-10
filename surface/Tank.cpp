/** \file
 * \brief Implementation of Tank class.
 */
#include "Tank.h"
#include "Entity.h"
//#include "WarUtil.h"
#include "Player.h"
#include "Viewer.h"
#include "cmd.h"
//#include "rigid.h"
#include "SurfaceCS.h"
#include "Bullet.h"
#include "judge.h"
#include "motion.h"
#include "draw/WarDraw.h"
#include "draw/material.h"
//#include "coordcnv.h"
#include "Game.h"
#include "glw/GLWchart.h"
extern "C"{
#include <clib/c.h>
#include <clib/mathdef.h>
#include <clib/cfloat.h>
#include <clib/aquat.h>
#include <clib/aquatrot.h>
#include <clib/suf/sufdraw.h>
#include <clib/wavsound.h>
#include <clib/gl/gldraw.h>
}

#include "BulletCollision/NarrowPhaseCollision/btGjkConvexCast.h"

//#include <ode/ode.h> /* Wheeled Vehicles are needed to be tested separately, sometimes... */


//#include "sufsrc/type90_base.h"
//#include "sufsrc/type90_turret.h"
//#include "sufsrc/type90_barrel.h"
//#include "sufsrc/type90_m2.h"



#define FORESPEED 0.004
#define TURRETROTSPEED (.2 * M_PI)
#define TURRETROTRANGE (.6 * M_PI)
#define ROTSPEED (.075 * M_PI)
#define TANKGUNSPEED 1.7
#define TANKGUNDAMAGE 500.
#define GUNVARIANCE .005
#define INTOLERANCE (M_PI / 50.)
#define BULLETRANGE 3.
#define RELOADTIME 3.
#define RETHINKTIME .5
#define TANK_MAX_GIBS 30
/*#define TANK_SCALE .0001*/
#define TANK_SCALE (3.33 / 200 * 1e-3)

static struct random_sequence gsrs = {1};

const static GLdouble
tank_offset[3] = {0., 0.001, 0.}, tank_radius = .001,
bullet_offset[3] = {0., 0., 0.}, bullet_radius = .0005;

    	const Vec3d points[] = {
			Vec3d(.00166, -.0007, .00375),
			Vec3d(.00166, -.0007, -.00375),
			Vec3d(-.00166, -.0007, -.00375),
			Vec3d(-.00166, -.0007, .00375),
/*			Vec3d(.002, 0., .002),
			Vec3d(.002, 0., -.002),
			Vec3d(-.002, 0., -.002),
			Vec3d(-.002, 0., .002),*/
/*			Vec3d(.0, 0.007, .0),*/
			Vec3d(.00166, .0012-.0007, .00375),
			Vec3d(.00166, .0012-.0007, -.00375),
			Vec3d(-.00166, .0012-.0007, -.00375),
			Vec3d(-.00166, .0012-.0007, .00375),
/*			Vec3d(.002, .002, .003),
			Vec3d(.002, .002, -.003),
			Vec3d(-.002, .002, -.003),
			Vec3d(-.002, .002, .003),*/
			Vec3d(.0, 0.003-.0007, .0),
/*			Vec3d(.0, 0.005, -.002),
			Vec3d(.002, 0.005, .002),
			Vec3d(-.002, 0.005, .002),*/
		};

/*static const double tank_sc[3] = {.05, .055, .075};*/
/*static const double beamer_sc[3] = {.05, .05, .05};*/
double Tank::defaultMass = 50000.; ///< Mass defaults 50 tons
double Tank::topSpeed = 70. / 3.600; /// < Default 70 km/h
double Tank::backSpeed = 35. / 3.600; /// < Default half a topSpeed
double Tank::mainGunCooldown = 3.;
double Tank::mainGunMuzzleSpeed = 1.7;
double Tank::mainGunDamage = 500.;
HitBoxList Tank::hitboxes;


const char *Tank::idname()const{return "tank";}
const char *Tank::classname()const{return "Tank";}

const unsigned Tank::classid = registerClass("Tank", Conster<Tank>);
Entity::EntityRegister<Tank> Tank::entityRegister("Tank");


Tank::Tank(Game *game) : st(game){
}

Tank::Tank(WarField *aw) : st(aw){
	init();
	steer = 0.;
	wheelspeed = wheelangle = 0.;
	turrety = barrelp = 0.;
	cooldown = cooldown2 = RELOADTIME;
	mountpy[0] = mountpy[1] = 0.;
	ammo[0] = 35; /* main gun ammunition */
	ammo[1] = 2000; /* coaxial gun ammo (type 74 7.64mm) */
	ammo[2] = 1500; /* mounted gun ammo (M2 Browning 12.7mm) */
	muzzle = 0;

	WarSpace *ws = *aw;
	if(ws && ws->bdw){
	}
}

bool Tank::buildBody(){
	if(!bbody){
		static btCompoundShape *shape = NULL;
		if(!shape){
			shape = new btCompoundShape();
			HitBoxList &hitboxes = getHitBoxes();
			for(HitBoxList::iterator i = hitboxes.begin(); i != hitboxes.end(); ++i){
				HitBox &it = *i;
				const Vec3d &sc = it.sc;
				const Quatd &rot = it.rot;
				const Vec3d &pos = it.org;
				btBoxShape *box = new btBoxShape(btvc(sc));
				btTransform trans = btTransform(btqc(rot), btvc(pos));
				shape->addChildShape(trans, box);
			}
		}
		btTransform startTransform;
		startTransform.setIdentity();
		startTransform.setOrigin(btvc(pos));

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			shape->calculateLocalInertia(mass,localInertia);

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,shape,localInertia);
		rbInfo.m_linearDamping = .05;
		rbInfo.m_angularDamping = .05;
		bbody = new btRigidBody(rbInfo);
		bbody->setActivationState(DISABLE_DEACTIVATION);

//		bbody->setSleepingThresholds(.0001, .0001);
	}
	return true;
}

void Tank::init(){
	static bool initialized = false;
	if(!initialized){
		SqInit(game->sqvm, modPath() << _SC("models/type90.nut"),
			SingleDoubleProcess(defaultMass, "mass") <<=
			SingleDoubleProcess(topSpeed, "topSpeed") <<=
			SingleDoubleProcess(backSpeed, "backSpeed") <<=
			SingleDoubleProcess(mainGunCooldown, "mainGunCooldown") <<=
			SingleDoubleProcess(mainGunMuzzleSpeed, "mainGunMuzzleSpeed") <<=
			SingleDoubleProcess(mainGunDamage, "mainGunDamage") <<=
			HitboxProcess(hitboxes));
		initialized = true;
	}
	mass = defaultMass;
}

void Tank::addRigidBody(WarSpace *ws){
	if(ws && ws->bdw){
		buildBody();
		//add the body to the dynamics world
		if(bbody)
			ws->bdw->addRigidBody(bbody, bbodyGroup(), bbodyMask());
	}
}

static const avec3_t tank_cog = {0., .0007, 0.};

Vec3d Tank::tankMuzzlePos(Vec3d *nh)const{
	static const Vec3d velo0(0., 0., -1.), pos0(0., 0/*.0025*/, -.005);
/*	const double *cog = tank_cog;*/
	Mat4d mat, rot;

	transform(mat);
	mat.translatein(-tank_cog[0], -tank_cog[1], -tank_cog[2]);
	rot = mat.roty(-turrety);
	rot.translatein(0, 90 * TANK_SCALE, -85 * TANK_SCALE);
	mat = rot.rotx(barrelp);

	if(nh)
		*nh = mat.dvp3(velo0);
	return mat.vp3(pos0);

}

int Tank::shootcannon(double dt){
//	struct bullet *pb;
	double v = mainGunMuzzleSpeed;
	const Vec3d velo0(0, 0, -v);
	Vec3d dir;
	Vec3d gunPos = tankMuzzlePos(&dir);
	while(this->cooldown < dt){
		int i = 0;
		Bullet *pb = new Bullet(this, 2., mainGunDamage);
		w->addent(pb);

		pb->mass = .005;
		pb->pos = gunPos;
		pb->velo = rot.trans(velo0) + this->velo;
		for(int j = 0; j < 3; j++)
			pb->velo[j] += (drseq(&w->rs) - .5) * .005;
		pb->anim(dt - this->cooldown);
		this->cooldown += mainGunCooldown;
	}
#if 0
	{
/*		double phi = (rot ? phi0 : yaw) + (drseq(&gsrs) - .5) * variance;
		double theta = pt->pyr[0] + (drseq(&gsrs) - .5) * variance;
		theta += acos(vx / v);*/
		Vec3d dr, momentum, amomentum, pos;
		int i;

		tankmuzzlepos(this, &pos, &dir);
		if(!*mposa)
			mpos = pos;
		*mposa = 1;

#if 0
		if(weapon == 0){
			pb = APFSDSNew(w, pt, TANKGUNDAMAGE);
			pb->life = 60.;
			VECCPY(pb->pos, pos);
			pb->mass = 1.;
			VECSCALE(pb->velo, dir, v);
		}
#	if 1
		else{
			pb = BulletSwarmNew(w, pt, 5., .001, .05, 512);
			pb->life = 3.;
			VECCPY(pb->pos, pos);
			pb->mass = 1.;
			VECSCALE(pb->velo, dir, v);
		}
#	else
		else for(i = 0; i < 512; i++){
			avec3_t dir1;
			dir1[0] = dir[0] + (drseq(&w->rs) - .5) * .05;
			dir1[1] = dir[1] + (drseq(&w->rs) - .5) * .05;
			dir1[2] = dir[2] + (drseq(&w->rs) - .5) * .05;
			pb = ShotgunBulletNew(w, pt, 15.); /* tank shotgun */
			pb->life = 3.;
			VECCPY(pb->pos, pos);
			pb->mass = .001;
			VECSCALE(pb->velo, dir1, v);
		}
#	endif

#if 1
		VECSUB(dr, pb->pos, pt->pos);
		VECSUBIN(dr, tank_cog);
		VECSCALE(momentum, pb->velo, -pb->mass * v / 2.);
		VECSADD(pt->velo, momentum, 1. / pt->mass);

		/* vector product of dr and momentum becomes angular momentum */
		VECVP(amomentum, dr, momentum);
		VECSADD(pt->omg, amomentum, 1. / pt->moi);
#endif
#else
		phi += (drseq(&gsrs) - .5) * variance;
		theta += (drseq(&gsrs) - .5) * variance;
		VECCPY(pb->velo, pt->velo);
		pb->velo[0] +=  v * sin(phi) * cos(theta);
		pb->velo[1] +=  v * sin(theta);
		pb->velo[2] += -v * cos(phi) * cos(theta);
		pb->pos[0] = pt->pos[0] + .005 * sin(phi);
		pb->pos[1] = pt->pos[1] +  0.0025;
		pb->pos[2] = pt->pos[2] - .005 * cos(phi);
#endif
	}
	{
		avec3_t velo0, velo, pos;
		int i;
		VECSCALE(velo0, dir, .005);
		VECADDIN(velo0, pt->velo);
		for(i = 0; i < 8; i++){
			int j;
			COLOR32 col;
			for(j = 0; j < 3; j++)
				velo[j] = velo0[j] + (drseq(&w->rs) - .5) * .005;
			for(j = 0; j < 3; j++)
				pos[j] = pb->pos[j] + (drseq(&w->rs) - .5) * .005;
			if(i < 4)
				col = COLOR32RGBA(255, 191 + rseq(&w->rs) % 32, 127,255);
			else
				col = COLOR32RGBA(191 + rseq(&w->rs) % 64, 191, 191, 127);
			AddTeline3D(w->tell, pos, velo, .005, NULL, NULL, NULL, col, TEL3_SPRITE | TEL3_NOLINE | TEL3_INVROTATE, i < 4 ? .2 : 1.5);
		}
	}
	pt->shoots++;
	p->ammo[0]--;
	p->cooldown += RELOADTIME;
	p->muzzle |= 1;
	{
		double s;
		s = 1. / (VECSDIST(pb->pos, w->pl->pos) + 1.);
		playWave3DPitch("rc.zip/sound/tank_001.wav"/*"c0wg42.wav"*/, pt->pos, w->pl->pos, w->pl->pyr, s, .2, w->realtime, 255);
		playWave3DPitch("rc.zip/sound/tank_001_far2.wav"/*"c0wg42.wav"*/, pt->pos, w->pl->pos, w->pl->pyr, 1 - s, 20., w->realtime, 255);
	}
#endif
	return 1;
}

#if 0
static void tankssm(tank2_t *p, warf_t *w){
	tank2_t *pt = &p->st;
	struct ssm *pb;
	pb = add_ssm(w, pt->pos, NULL);
/*			VECCPY(pb->pos, pt->pos);*/
	pb->st.pos[1] += .005;
	VECCPY(pb->st.velo, pt->velo);
	pb->st.velo[1] += .01;
	pb->st.owner = pt;
	pb->target = pt->enemy;
	pb->st.damage = 150.;
/*			pb->life = LANDMINE_LIFE;*/
	pt->shoots++;
//	playWAVEFile("missile.wav");
	playWave3D("missile.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime + p->cooldown);
	pt->cooldown += 10.;
}
#endif

int Tank::tryshoot(double dt){
	if(dt <= this->cooldown)
		return 0;
//	double yaw = pyr[1] + p->turrety;
//	double pitch = /*pt->pyr[0]*/ + p->barrelp;
/*	double (*theta_phi)[2] = &p->desired;*/
//	if(!rot && (INTOLERANCE < fabs(phi0 - yaw) || INTOLERANCE < fabs(pitch - p->desired[0])))
//		return 0;

#if 1
	return shootcannon(dt);
#else
/*	if(!shoot_angle(pt->pos, epos, phi0, v, &theta_phi))
		return 0;*/

	pb = add_bullet(w);
	pb->owner = pt;
	pb->damage = damage;
	{
/*		double phi = (rot ? phi0 : yaw) + (drseq(&gsrs) - .5) * variance;
		double theta = pt->pyr[0] + (drseq(&gsrs) - .5) * variance;
		theta += acos(vx / v);*/
#if 1
		const avec3_t velo0 = {0., 0., -1.7}, pos0 = {0., .0025, .005};
		amat4_t mat;
		tankrot(&mat, pt);
		MAT4DVP3(pb->velo, mat, velo0);
		MAT4VP3(pb->pos, mat, pos0);
#else
		double phi, theta;
		phi = (rot ? phi0 : yaw) + (drseq(&gsrs) - .5) * variance;
		theta = pitch + (drseq(&gsrs) - .5) * variance;
		VECCPY(pb->velo, pt->velo);
		pb->velo[0] +=  v * sin(phi) * cos(theta);
		pb->velo[1] +=  v * sin(theta);
		pb->velo[2] += -v * cos(phi) * cos(theta);
		pb->pos[0] = pt->pos[0] + .005 * sin(phi);
		pb->pos[1] = pt->pos[1] +  0.0025;
		pb->pos[2] = pt->pos[2] - .005 * cos(phi);
#endif
	}
#endif
	return 1;
}

void Tank::find_enemy_logic(){
	double best = 10. * 10.; /* sense range */
	double sdist;
	Entity *closest = NULL;
/*			pt->enemy = &head[(i+1)%n];*/
	WarField::EntityList::iterator it = w->entlist().begin();
	for(; it != w->entlist().end(); it++) if(*it){
		Entity *pt2 = *it;
		if(pt2 != this && pt2->w == w && pt2->getHealth() > 0. && 0 <= pt2->race && pt2->race != race
			&& strcmp(pt2->idname(), "respawn") && (sdist = (pt2->pos - pos).slen()) < best){
			best = sdist;
			closest = pt2;
		}
	}
	if(closest)
		enemy = closest;
}




#define putstring(a) gldPutString(a)
/*extern void putstring(const char *);*/

#define VEC4SET(a,s0,s1,s2,s3) ((a)[0]=(s0),(a)[1]=(s1),(a)[2]=(s2),(a)[3]=(s3))
#define MATVEC4(r,m,v) (\
	(r)[0] = (r)[0]*(m)[0] + (r)[1]*(m)[4] + (r)[2]*(m)[8] + (m)[12],\
	(r)[1] = (r)[0]*(m)[1] + (r)[1]*(m)[5] + (r)[2]*(m)[9] + (m)[13],\
	(r)[2] = (r)[0]*(m)[2] + (r)[1]*(m)[6] + (r)[2]*(m)[10] + (m)[14],\
	(r)[3] = (r)[0]*(m)[3] + (r)[1]*(m)[7] + (r)[2]*(m)[11] + (m)[15])


/*static void vecrot(avec3_t vr, const avec3_t vaxis, double angle){
	aquat_t q;
	double sa;
	sa = sin(angle / 2.);
	q[3] = cos(angle / 2.);
	VECSCALE(q, vaxis, sa);
}*/

void Tank::vehicle_drive(double dt, Vec3d *points, int npoints){
	int in = inputs.press;
	int leng, reng;
	unsigned long contact;
	Vec3d fore0(0., 0., - -.01);
	Vec3d vleng, vreng;
	Vec3d *thrust[64] = {NULL};
	Mat4d mat;
	int engflags = 0, i;

	steer = approach(steer, (inputs.press & (PL_A | PL_D)) == PL_A ? -1. : (inputs.press & (PL_A | PL_D)) == PL_D ? 1. : 0., dt, 0);

	leng = in & PL_W ? 2 : in & PL_A ? 1 : in & PL_S ? -2 : in & PL_D ? -1 : 0;
	reng = in & PL_W ? 2 : in & PL_D ? 1 : in & PL_S ? -2 : in & PL_A ? -1 : 0;
	if(leng){
		vleng = fore0 * leng;
		thrust[0] = thrust[1] = &vleng;
	}
	if(reng){
		vreng = fore0 * reng;
		thrust[2] = thrust[3] = &vreng;
	}
//	memset(((tank2_t*)pt)->forces, 0, sizeof ((tank2_t*)pt)->forces);
//	memset(((tank2_t*)pt)->normals, 0, sizeof ((tank2_t*)pt)->normals);
//	contact = RigidPointHit(pt, w, dt, points, thrust, npoints, NULL, ((tank2_t*)pt)->forces, ((tank2_t*)pt)->normals);

//	if(contact & (1 << 8))
//		((struct entity_private_static*)pt->vft)->takedamage(pt, 100 * dt, w, 0);

//	if(contact & (1|2|4|8))
//		tankrot(&mat, pt);
/*	engflags |= reng < 0;
	engflags |= (0 < reng) << 1;
	engflags |= (0 < leng) << 2;
	engflags |= (leng < 0) << 3;
	for(i = 0; i < 4; i++) if(contact & engflags & (1 << i) && drseq(&w->rs) < dt / .1){
		avec3_t pos;
		mat4vp3(pos, mat, points[i]);
		pos[0] += (drseq(&w->rs) - .5) * .0005;
		pos[1] += (drseq(&w->rs) - .5) * .0005;
		pos[2] += (drseq(&w->rs) - .5) * .0005;
		AddTeline3D(w->tell, pos, NULL, .001, NULL, NULL, NULL, COLOR32RGBA(127,127,127,127), TEL3_SPRITE | TEL3_NOLINE | TEL3_INVROTATE, 1.);
	}*/
}

/// \brief Casts a ray in a Bullet Dynamics World to see if any rigid body object is hit.
///
/// This function is merely an adaptor that calls btCollisionWorld::rayTest() with callback results expanded in arguments.
///
/// \param bdw The target dynamics world.
/// \param rayFrom The ray's starting position
/// \param rayTo The ray's ending position
/// \param fraction How far the hitting point is from rayFrom.
/// \param worldNormal Normal vector at hitting point in world coordinates.
/// \param worldHitPoint Position of hitting point in world coordinates.
static bool rayTest(const btCollisionWorld *bdw, const btVector3& rayFrom, const btVector3& rayTo,
					btScalar &fraction, btVector3& worldNormal, btVector3& worldHitPoint)
{
	btCollisionWorld::ClosestRayResultCallback resultCallback(rayFrom,rayTo);

	bdw->rayTest(rayFrom, rayTo, resultCallback);

	if(resultCallback.hasHit()){
		fraction = resultCallback.m_closestHitFraction;
		worldNormal = resultCallback.m_hitNormalWorld;
		worldHitPoint = resultCallback.m_hitPointWorld;
		return true;
	}
	else
		return false;
};

/// \brief Aligns given vector v to a plane n.
static btVector3 alignPlane(const btVector3 &v, const btVector3 &n){
	return v - n.dot(v) * n;
};

void Tank::anim(double dt){
	double h;
	Vec3d epos(0,0,0); /* estimated enemy position */
	Vec3d mpos; /* muzzle position */
	Vec3d normal(0,1,0);
	int mposa = 0; /* muzzle position availability */
/*	{
		double n[3] = {0., 1., 0.};
		h = w->map ? warmapheightr(w->map, pt->pos[0], pt->pos[2], &n) : 0.;
		normal[0] = n[0];
		normal[1] = n[2];
		normal[2] = n[1];
		VECNORMIN(normal);
	}*/
/*	if(&w->cs->getStatic() == &SurfaceCS::classRegister){
		SurfaceCS *s = (SurfaceCS*)w->cs;
		WarMap *wm = s->getWarMap();
		if(wm)
			pos[1] = wm->height(pos[0], pos[2], NULL);
	}*/

	bool floorTouch = false;
	btVector3 worldNormal;
	if(WarSpace *ws = *w){
		static const btVector3 offset(0, 0.0010, 0);
		const btvc btPos = bbody->getWorldTransform().getOrigin() - offset;
		const btvc btVelo = bbody->getLinearVelocity();
		GLWchart::addSampleToCharts("tankvelo", btVelo.len());
		const Vec3d delta(0, -0.02 - btVelo[1] * dt, 0);
		const Vec3d start(btPos - delta);
		Vec3d normal;
		double height = ((SurfaceCS*)w->cs)->getHeight(btPos[0], btPos[2], &normal);
		worldNormal = btvc(normal);
		if(btPos[1] - offset[1] < height){
			Vec3d dest(btPos[0], height + offset[1], btPos[2]);
			Vec3d newVelo = (btVelo - worldNormal.dot(btVelo) * worldNormal) * exp(-dt);
			setPosition(&dest, NULL, &newVelo);
			btVector3 btOmega = bbody->getAngularVelocity() - worldNormal.cross(bbody->getWorldTransform().getBasis().getColumn(1)) * 5. * dt;
			bbody->setAngularVelocity(btOmega * exp(-3. * dt));
			floorTouch = true;
		}
	}

	if(!w || controller){
	}
	else if(0 < getHealth()){
		double phi; /* enemy direction */

		/* find enemy logic */
		if(!enemy){
			find_enemy_logic();
		}


		/* estimating enemy position and wheeling towards enemy */
		inputs.press = 0;
		if(enemy){
			double diff, dphi;
			Vec3d dpos, dp, pyr, mdir;
			Mat4d mat, rot;
			double bulletspeed;
			int subweapon;

			mpos = tankMuzzlePos(&mdir);

			subweapon = !ammo[0] /*|| ((struct entity_private_static*)pt->enemy->vft)->flying(pt->enemy)*/
				|| normal.sp(mdir) < -.2
				|| enemy->getHealth() < 50. && (pos - enemy->pos).slen() < .1 * .1;
			bulletspeed = subweapon ? .8 : TANKGUNSPEED;

/*			if(enemy->flying())
				VECCPY(epos, pt->enemy->pos);
			else*/
				estimate_pos(epos, enemy->pos, enemy->velo, pos, velo, bulletspeed, w);
			mposa = 1;
			dpos = epos - mpos;
			quat2imat(~mat, rot);
			dp = mat.vp3(dpos);
//			pyrmat(pyr, &rot);
			phi = atan2(epos[0] - pos[0], -(epos[2] - pos[2]));
			dphi = atan2(dp[0], -dp[2]);
//			shoot_angle(avec3_000, dp, dphi, bulletspeed, &p->desired);
//			p->turrety = rangein(approach(p->turrety + M_PI, p->desired[1] + M_PI, TURRETROTSPEED * dt, 2 * M_PI) - M_PI, -vft->turretrange, vft->turretrange);
//			p->barrelp = rangein(approach(p->barrelp + M_PI, p->desired[0] /*- pt->pyr[0]*/ + M_PI, TURRETROTSPEED * dt, 2 * M_PI) - M_PI, vft->barrelmin, vft->barrelmax);
/*			if(pt->pos[1] <= h)
				pt->pyr[1] = approach(pt->pyr[1] + M_PI, phi + M_PI, ROTSPEED * dt, 2 * M_PI) - M_PI;*/
/*
			diff = fmod(phi - pt->pyr[1] + M_PI, 2. * M_PI) - M_PI;
			if(VECSDIST(epos, pt->pos) < .05 * .05);
			else if(diff < -.1 * M_PI)
				pt->inputs.press |= PL_A;
			else if(.1 * M_PI < diff)
				pt->inputs.press |= PL_D;
			else
				pt->inputs.press |= PL_W;*/

			inputs.press |= PL_ENTER;
	

//			playWAVEFile("c0wg42.wav");
/*			{
				avec3_t delta, xhat;
				VECSUB(delta, pos, w->pl->pos);
				xhat[0] = cos(w->pl->pyr[1]);
				xhat[1] = 0;
				xhat[2] = sin(w->pl->pyr[1]);
				playWaveCustom("c0wg42.wav", (unsigned short)(256 / (1. + 100 * VECSDIST(w->pl->pos, pos))), 0, -128 * VECSP(delta, xhat) / VECLEN(delta));
			}*/
			//playWave3D("c0wg42.wav", pos, w->pl->pos, w->pl->pyr, .2, .01, w->realtime);
		}

#if 0 /* chaingun */
		if(pt->cooldown2 < dt){
			struct ssm *pb;
			pb = bul;
			for(; pb; pb = pb->next) if(pb->vft == &ssm_s && pb->target == pt) break;
			if(pb && tank_tryshoot(pt, 1, pb->pos, atan2(pb->pos[0] - pt->pos[0], -(pb->pos[2] - pt->pos[2])), 1., 1., .015)){
				pt->shoots2++;
				pt->cooldown2 = 0.1;
			}
			else if(pt->enemy && tank_tryshoot(pt, 1, pt->enemy->pos, atan2(pt->enemy->pos[0] - pt->pos[0], -(pt->enemy->pos[2] - pt->pos[2])), 1., 1., .015)){
				pt->shoots2++;
				pt->cooldown2 = 0.1;
			}
			else
				pt->cooldown2 = 0.1;
		}
		else
			pt->cooldown2 -= dt;
#endif
	}

#if 0
    if(pt->weapon == 1 && pt->inputs.press & (PL_LCLICK | PL_ENTER)) while(p->cooldown2 < dt){
		/* Type 74 */
		struct bullet *pb;
		static const double scale = 3.4 / 200. * 1e-3;
		avec3_t pos, pos0 = {8 * scale, 60 * scale, -8 * scale}, velo, velo0 = {0, 0, -.8};
		aquat_t qp = {0}, qy = {0}, q, qr;
		double dev[2];

		dev[0] = p->barrelp + (drseq(&w->rs) - .5) * .0007 * M_PI;
		dev[1] = p->turrety + (drseq(&w->rs) - .5) * .0007 * M_PI;

		qy[1] = sin(-dev[1] / 2.);
		qy[3] = cos(-dev[1] / 2.);
		qp[0] = sin(dev[0] / 2.);
		qp[3] = cos(dev[0] / 2.);
		QUATMUL(q, pt->rot, qy);
		QUATMUL(qr, q, qp);

		quatrot(pos, q, pos0);
		VECADDIN(pos, pt->pos);
		quatrot(velo, qr, velo0);
		pb = BulletNew(w, pt, 2.);
		pb->mass = .008;
		VECCPY(pb->pos, pos);
		VECADD(pb->velo, velo, pt->velo);
		pb->life = 20.;
		pt->shoots2++;
		p->ammo[1]--;
		pb->vft->anim(pb, w, w->tell, dt - p->cooldown2);
		p->cooldown2 += 0.06;
		p->muzzle |= 2;

		playWave3DPitch("rc.zip/sound/rifle.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .02 * .02, w->realtime + dt - p->cooldown2, 256/* + rseq(&w->rs) % 256*/);
	}
	else if(pt->weapon == 2 && pt->inputs.press & (PL_LCLICK | PL_ENTER)) while(p->cooldown2 < dt){
		/* M2 */
		struct bullet *pb;
		static const double scale = 3.4 / 200. * 1e-3;
		avec3_t pos, pos0 = {32 * scale, 125 * scale, -45 * scale}, velo, velo0 = {0, 0, -.88};
		aquat_t qp = {0}, qy = {0}, q, q1, qr;
		double dev[2];
		dev[0] = p->mountpy[0] + (drseq(&w->rs) - .5) * .001 * M_PI;
		dev[1] = p->mountpy[1] + (drseq(&w->rs) - .5) * .001 * M_PI;

		qy[1] = sin(-p->turrety / 2.);
		qy[3] = cos(-p->turrety / 2.);
/*		qp[0] = sin(p->barrelp / 2.);
		qp[3] = cos(p->barrelp / 2.);*/
		QUATMUL(q, pt->rot, qy);
/*		QUATMUL(qr, q, qp);*/
		qy[1] = sin(-dev[1] / 2.);
		qy[3] = cos(-dev[1] / 2.);
		qp[0] = sin(dev[0] / 2.);
		qp[3] = cos(dev[0] / 2.);
		QUATMUL(q1, q, qy);
		QUATMUL(qr, q1, qp);

		quatrot(pos, q, pos0);
		VECADDIN(pos, pt->pos);
		quatrot(velo, qr, velo0);
		pb = BulletNew(w, pt, 5.);
		pb->mass = .008;
		VECCPY(pb->pos, pos);
		VECADD(pb->velo, velo, pt->velo);
		pb->life = 20.;
		pt->shoots2++;
		p->ammo[2]--;
		pb->vft->anim(pb, w, w->tell, dt - p->cooldown2);
		p->cooldown2 += 0.1;
		p->muzzle |= 4;

		playWave3DPitch("rc.zip/sound/rifle.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .02 * .02, w->realtime + dt - p->cooldown2, 192/* + rseq(&w->rs) % 256*/);
	}
#endif

	if(0 < getHealth()){
		if(inputs.press & PL_ENTER){
			if(cooldown == 0.){
				if(!tryshoot(dt))
					cooldown += RETHINKTIME;
			}
		}

		if(cooldown < dt)
			cooldown = 0.;
		else
			cooldown -= dt;

		if(cooldown2 < dt)
			cooldown2 = 0.;
		else
			cooldown2 -= dt;
	}

	{
		Vec3d accel = w->accel(pos, velo);
		velo += accel * dt;
/*	pt->velo[1] += gravity[1] * dt;*/
	}

	if(floorTouch){
		if(inputs.press & PL_W){
			bbody->applyCentralForce(alignPlane(btvc(rot.trans(Vec3d(0,0,-1)) * mass * topSpeed), worldNormal));
		}
		if(inputs.press & PL_S){
			bbody->applyCentralForce(alignPlane(btvc(rot.trans(Vec3d(0,0,-1)) * mass * -backSpeed), worldNormal));
		}
		if(inputs.press & PL_A){
			bbody->applyTorque(btvc(rot.trans(Vec3d(0,1,0)) * mass * 0.5 * 1e-5));
		}
		if(inputs.press & PL_D){
			bbody->applyTorque(btvc(rot.trans(Vec3d(0,1,0)) * mass * -0.5 * 1e-5));
		}
//		vehicle_drive(dt, points, numof(points));
	}
#if 0
	else{
		int in = pt->inputs.press;
		int leng, reng;
		unsigned long contact;
		avec3_t fore0 = {0., 0., - -.01};
		avec3_t vleng, vreng;
		avec3_t *thrust[numof(points)] = {NULL};
		amat4_t mat;
		int engflags = 0, i;
		leng = in & PL_W ? 2 : in & PL_A ? 1 : in & PL_S ? -2 : in & PL_D ? -1 : 0;
		reng = in & PL_W ? 2 : in & PL_D ? 1 : in & PL_S ? -2 : in & PL_A ? -1 : 0;
		if(leng){
			VECSCALE(vleng, fore0, leng);
			thrust[0] = thrust[1] = &vleng;
		}
		if(reng){
			VECSCALE(vreng, fore0, reng);
			thrust[2] = thrust[3] = &vreng;
		}
		memset(((tank2_t*)pt)->forces, 0, sizeof ((tank2_t*)pt)->forces);
		memset(((tank2_t*)pt)->normals, 0, sizeof ((tank2_t*)pt)->normals);
		contact = RigidPointHit(pt, w, dt, points, thrust, numof(points), NULL, ((tank2_t*)pt)->forces, ((tank2_t*)pt)->normals);

		if(contact & (1 << 8))
			((struct entity_private_static*)pt->vft)->takedamage(pt, 100 * dt, w, 0);

		if(contact & (1|2|4|8))
			tankrot(&mat, pt);
		engflags |= reng < 0;
		engflags |= (0 < reng) << 1;
		engflags |= (0 < leng) << 2;
		engflags |= (leng < 0) << 3;
		for(i = 0; i < 4; i++) if(contact & engflags & (1 << i) && drseq(&w->rs) < dt / .1){
			avec3_t pos;
			mat4vp3(pos, mat, points[i]);
			pos[0] += (drseq(&w->rs) - .5) * .0005;
			pos[1] += (drseq(&w->rs) - .5) * .0005;
			pos[2] += (drseq(&w->rs) - .5) * .0005;
			AddTeline3D(w->tell, pos, NULL, .001, NULL, NULL, NULL, COLOR32RGBA(127,127,127,127), TEL3_SPRITE | TEL3_NOLINE | TEL3_INVROTATE, 1.);
		}
	}
#endif

	pos += velo * dt;

#if 0
	{
		amat4_t nmat;
		aquat_t q;
		rbd_anim(pt, dt, &q, &nmat);
		QUATCPY(pt->rot, q);
		quat2pyr(pt->rot, pt->pyr);
	}
#endif
/*	{
		amat4_t nmat;
		aquat_t q;
		rbd_anim(pt, dt, &q, &nmat);
		QUATCPY(pt->rot, q);
		assert(-1. <= nmat[5] && nmat[5] <= 1.);
		pt->pyr[0] = atan2(nmat[9], sqrt(nmat[8] * nmat[8] + nmat[10] * nmat[10]));
		assert(-1. <= nmat[2] && nmat[2] <= 1.);
		assert(-1. <= nmat[0] && nmat[0] <= 1.);
		pt->pyr[1] = atan2(-nmat[8], nmat[10]);
		assert(-1. <= nmat[9] && nmat[9] <= 1.);
		pt->pyr[2] = -atan2(nmat[1], sqrt(nmat[0] * nmat[0] + nmat[2] * nmat[2]));
	}*/

//	omg /= dt + 1.;

	if(bbody){
		pos = btvc(bbody->getCenterOfMassPosition());
		rot = btqc(bbody->getOrientation());
	}

}

int Tank::takedamage(double damage, int hitpart){
#if 0
	tank2_t *p = (tank2_t*)pt;
	struct tent3d_line_list *tell = w->tell;

	/* tank armor is bullet resistant */
	if(hitpart == 1)
		damage *= .3;
	else if(hitpart == 3)
		damage *= .5;

	if(pt->health <= damage){ /* death! */
		entity_t *pt2;
		avec3_t gravity;
		if(w->vft->accel)
			w->vft->accel(w, &gravity, &pt->pos, &pt->velo);
		else
			VECNULL(gravity);
		if(tell){
			int j;
			for(j = 0; j < 20; j++){
				double velo[3];
				velo[0] = .15 * (drseq(&gsrs) - .5);
				velo[1] = .15 * (drseq(&gsrs) - .5);
				velo[2] = .15 * (drseq(&gsrs) - .5);
				AddTeline3D(tell, pt->pos, velo, .0025, NULL, NULL, gravity,
					j % 2 ? COLOR32RGBA(255,255,255,255) : COLOR32RGBA(255,191,63,255), TEL3_HEADFORWARD | TEL3_FADEEND | TEL3_REFLECT, 1. + .5 * drseq(&gsrs));
			}
/*			for(j = 0; j < 10; j++){
				double velo[3];
				velo[0] = .025 * (drseq(&gsrs) - .5);
				velo[1] = .025 * (drseq(&gsrs));
				velo[2] = .025 * (drseq(&gsrs) - .5);
				AddTefpol3D(w->tepl, pt->pos, velo, gravity, &cs_firetrail,
					TEP3_REFLECT | TEP_THICK | TEP_ROUGH, 3. + 1.5 * drseq(&gsrs));
			}*/

			{/* explode shockwave thingie */
				static const double pyr[3] = {M_PI / 2., 0., 0.};
				AddTeline3D(tell, pt->pos, NULL, .1, pyr, NULL, NULL, COLOR32RGBA(255,191,63,255), TEL3_EXPANDISK/*/TEL3_EXPANDTORUS*/ | TEL3_NOLINE, 1.);
				if(pt->pos[1] <= 0.) /* no scorch in midair */
					AddTeline3D(tell, pt->pos, NULL, .01, pyr, NULL, NULL, COLOR32RGBA(0,0,0,255), TEL3_STATICDISK | TEL3_NOLINE, 20.);
			}
			if(w->gibs && ((struct entity_private_static*)pt->vft)->sufbase && ((struct entity_private_static*)pt->vft)->sufturret){
				int i, n, base;
				struct entity_private_static *vft = (struct entity_private_static*)pt->vft;
				int m = vft->sufbase->np + vft->sufturret->np;
				n = m <= TANK_MAX_GIBS ? m : TANK_MAX_GIBS;
				base = m <= TANK_MAX_GIBS ? 0 : rseq(&gsrs) % m;
				for(i = 0; i < n; i++){
					double velo[3], omg[3];
					j = (base + i) % m;
					velo[0] = pt->velo[0] + (drseq(&gsrs) - .5) * .1;
					velo[1] = pt->velo[1] + (drseq(&gsrs) - .5) * .1;
					velo[2] = pt->velo[2] + (drseq(&gsrs) - .5) * .1;
					omg[0] = 3. * 2 * M_PI * (drseq(&gsrs) - .5);
					omg[1] = 3. * 2 * M_PI * (drseq(&gsrs) - .5);
					omg[2] = 3. * 2 * M_PI * (drseq(&gsrs) - .5);
					AddTelineCallback3D(w->gibs, pt->pos, velo, 0.01, pt->pyr, omg, gravity, vft->gib_draw, (void*)j, TEL3_NOLINE | TEL3_REFLECT, 1.5 + drseq(&gsrs));
				}
			}
		}
		pt->deaths++;
		if(pt->race < numof(w->races))
			w->races[pt->race].deaths++;
		if(pt->enemy)
			pt->enemy = NULL;

		/* forget about dead enemy */
		for(pt2 = w->tl; pt2; pt2 = pt2->next) if(pt2->enemy == pt)
			pt2->enemy = NULL;

		pt->pos[0] = drseq(&gsrs) - .5;
		pt->pos[2] = drseq(&gsrs) - .5;
		pt->pos[1] = .01 + warmapheight(w->map, pt->pos[0], pt->pos[2], NULL);
		VECNULL(pt->pyr);
		VECNULL(pt->rot);
		pt->rot[3] = 1.;
/*		MATIDENTITY(pt->rot);
		pt->pyr[1] = drseq(&gsrs) * 2 * M_PI;*/
		VECNULL(pt->omg);
		pt->health = 500.;
		p->ammo[0] = 35;
		playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .1, w->realtime);
		return 0;
	}
	else{
		pt->health -= damage;
/*		playWave3D("hit.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .001);*/
		return 1;
	}
#endif
	return 0;
}

void Tank::postframe(){
	if(enemy && (!enemy->w || !w))
		enemy = NULL;
}

#if 0
int Tank::getrot(double (*ret)[16]){
/*	amat4_t mat, mat2;
	quat2imat(mat, pt->rot);
	mat4roty(mat2, mat, -pt->turrety);
	mat4rotx(ret, mat2, pt->barrelp);*/
	quat2imat(ret, pt->rot);
	return 1;
}
#endif

/*static void mattransin(double mat[16]){
	double buf[16];
	int i, j;
	for(i = 0; i < 4; i++) for(j = 0; j < 4; j++)
		buf[i + 4 * j] = mat[j + 4 * i];
	for(i = 0; i < 16; i++)
		mat[i] = buf[i];
}*/



void Tank::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
/*	double yaw = pt->pyr[1] + ((tank2_t*)pt)->turrety;*/
	static  const Vec3d ofs[2] = {Vec3d(-.0006, .0010, -.0005), Vec3d(0., .010, .025)};
	Mat4d mat, mat2;
	int camera = seatid;
	transform(mat);
/*	MAT3TO4(mat, pt->rot);*/
/*	pyrmat(pt->pyr, mat);
	MAT4TRANSLATE(mat, - 0.005 * sin(pt->turrety), .005, 0.005 * cos(pt->turrety));*/
/*	VECCPY(&mat[12], pt->pos);*/
	camera = camera < -2 ? 0 : (camera + 3) % 3;
	if(camera == 2){
/*		struct bullet *pb;
		for(pb = w->bl; pb; pb = pb->next) if(pb && pb->active && pb->owner == pt){
			VECCPY(*pos, pb->pos);
			return;
		}*/
		camera = 0;
	}
	else if(camera){
		if(weapon == 2){
			mat4roty(mat2, mat, -(turrety + mountpy[1]));
			mat4rotx(mat, mat2, mountpy[0]);
		}
		else{
			mat4roty(mat2, mat, -turrety);
			mat4rotx(mat, mat2, barrelp);
		}
	}
	pos = mat.vp3(ofs[camera]);
	rot = this->rot;
/*	(*pos)[0] = pt->pos[0] - 0.005 * sin(yaw);
	(*pos)[1] = pt->pos[1] + 0.005;
	(*pos)[2] = pt->pos[2] + 0.005 * cos(yaw);*/
}

double Tank::getHitRadius()const{
	return 0.007;
}

void Tank::control(const input_t *in, double dt){
	st::control(in, dt);
#if 0
	static int prev = 0;
	double h, n[3], pyr[3];
	int inputs = in->press;
	h = w->map ? warmapheight(w->map, pt->pos[0], pt->pos[2], &n) : 0.;

#if 1
	pt->inputs = *in;
#else
	if(pt->pos[1] <= h){
		if(inputs & PL_A)
			pt->pyr[1] = fmod(pt->pyr[1] - ROTSPEED * dt + M_PI, 2 * M_PI) - M_PI;
		if(inputs & PL_D)
			pt->pyr[1] = fmod(pt->pyr[1] + ROTSPEED * dt + M_PI, 2 * M_PI) - M_PI;
		if(inputs & PL_W){
			pt->velo[0] =  FORESPEED * sin(pt->pyr[1]);
			pt->velo[2] = -FORESPEED * cos(pt->pyr[1]);
		}
		else if(inputs & PL_S){
			pt->velo[0] = -FORESPEED * sin(pt->pyr[1]);
			pt->velo[2] =  FORESPEED * cos(pt->pyr[1]);
		}
		else{
			pt->velo[0] = pt->velo[2] = 0;
		}
	}
#endif
	quat2pyr(w->pl->rot, pyr);
	if(pt->weapon != 2){
		p->turrety = approach(p->turrety, pyr[1] - floor(pyr[1] / 2. / M_PI) * 2. * M_PI, TURRETROTSPEED * dt, 2 * M_PI);
		p->barrelp = rangein(approach(p->barrelp, -pyr[0], TURRETROTSPEED * dt, 2 * M_PI), vft->barrelmin, vft->barrelmax);
	}
	else{
		pyr[1] -= p->turrety;
		p->mountpy[1] = approach(p->mountpy[1], pyr[1] - floor(pyr[1] / 2. / M_PI) * 2. * M_PI, 2 * TURRETROTSPEED * dt, 2 * M_PI);
		p->mountpy[0] = rangein(approach(p->mountpy[0], -pyr[0], 2 * TURRETROTSPEED * dt, 2 * M_PI), vft->barrelmin, vft->barrelmax);
	}

/*	if(inputs & PL_4)
		pt->turrety = rangein(fmod(pt->turrety + M_PI - TURRETROTSPEED * dt, 2 * M_PI) - M_PI, -vft->turretrange, vft->turretrange);
	if(inputs & PL_6)
		pt->turrety = rangein(fmod(pt->turrety + M_PI + TURRETROTSPEED * dt, 2 * M_PI) - M_PI, -vft->turretrange, vft->turretrange);
	if(inputs & PL_8)
		pt->barrelp = rangein(fmod(pt->barrelp + M_PI + TURRETROTSPEED * dt, 2 * M_PI) - M_PI, vft->barrelmin, vft->barrelmax);
	if(inputs & PL_2)
		pt->barrelp = rangein(fmod(pt->barrelp + M_PI - TURRETROTSPEED * dt, 2 * M_PI) - M_PI, vft->barrelmin, vft->barrelmax);*/

	if((inputs & in->change) & (PL_TAB | PL_RCLICK)){
		pt->weapon = (pt->weapon + 1) % 4;
	}

	if(inputs & PL_SPACE && !(prev & PL_SPACE)){
		double best = 3. * 3.;
		double mini = pt->enemy ? VECSDIST(pt->pos, pt->enemy->pos) : 0.;
		double sdist;
		entity_t *t = w->tl;
		entity_t *target = NULL;
		for(t = w->tl; t; t = t->next) if(t != pt && t != pt->enemy && t->active && 0. < t->health && mini < (sdist = VECSDIST(pt->pos, t->pos)) && sdist < best){
			best = sdist;
			target = t;
		}
		if(target)
			pt->enemy = target;
		else{
			for(t = w->tl; t; t = t->next) if(t != pt && t != pt->enemy && t->active && 0. < t->health && (sdist = VECSDIST(pt->pos, t->pos)) < best){
				best = sdist;
				target = t;
			}
			pt->enemy = target;
		}
	}

	if(inputs & (PL_ENTER | PL_LCLICK) && p->cooldown < dt){
		avec3_t mpos;
		int mposa = 0;
		if(!pt->weapon || pt->weapon == 3){
			shootcannon(w, p, pt->pyr[1] + p->turrety, pt->pyr[0] + p->barrelp, GUNVARIANCE, &mpos, &mposa);
		}
		else if(pt->enemy && pt->enemy->active){
/*			tankssm(pt, w);*/
		}
	}
	if(p->cooldown < dt)
		p->cooldown = 0.;
	else p->cooldown -= dt;

	prev = inputs;
#endif
}

int Tank::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn, const hitbox *hb, int nhb){
/*	const struct hitbox *hb = tank_hb;*/
	double sc[3];
	double best = dt, retf;
	int reti = 0, i, n;
/*	aquat_t rot, roty;
	roty[0] = 0.;
	roty[1] = sin(pt->turrety / 2.);
	roty[2] = 0.;
	roty[3] = cos(pt->turrety / 2.);
	QUATMUL(rot, pt->rot, roty);*/
#if 0
	for(n = 0; n < nhb; n++){
		avec3_t org;
		aquat_t rot;
		quatrot(org, pt->rot, hb[n].org);
		VECADDIN(org, pt->pos);
		QUATMUL(rot, pt->rot, hb[n].rot);
		for(i = 0; i < 3; i++)
			sc[i] = hb[n].sc[i] + rad;
		if((jHitBox(org, sc, rot, src, dir, 0., best, &retf, retp, retn)) && (retf < best)){
			best = retf;
			if(ret) *ret = retf;
			reti = n+1;
		}
	}
#endif
	return reti;
}

int Tank::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	return tracehit(src, dir, rad, dt, ret, retp, retn, &hitboxes.front(), hitboxes.size());
}



































#if 0
const static GLdouble
m3track_offset[3] = {0., 0.001, 0.}, m3track_radius = .001;
static const avec3_t m3track_cog = {0., .0007, 0.}, m3track_cockpit_ofs = {-.0005, .00175, -.0006};

    	static const avec3_t m3track_points[] = {
			{.001, -.0007, .003},
			{.001, -.0007, -.003},
			{-.001, -.0007, -.003},
			{-.001, -.0007, .003},
/*			{.002, 0., .002},
			{.002, 0., -.002},
			{-.002, 0., -.002},
			{-.002, 0., .002},*/
/*			{.0, 0.007, .0},*/
			{.001, .0012-.0007, .003},
			{.001, .0012-.0007, -.003},
			{-.001, .0012-.0007, -.003},
			{-.001, .0012-.0007, .003},
/*			{.002, .002, .003},
			{.002, .002, -.003},
			{-.002, .002, -.003},
			{-.002, .002, .003},*/
			{.0, 0.003-.0007, .0},
/*			{.0, 0.005, -.002},
			{.002, 0.005, .002},
			{-.002, 0.005, .002},*/
		};

		/*
static const struct hitbox m3track_hb[] = {
	{{0., .000, -.0015}, {0,0,0,1}, {.001, .0005, .0015}},
	{{0., .0001, .0015}, {0,0,0,1}, {.001, .0006, .0015}},
	{{0., .0009, -.0005}, {0,0,0,1}, {.001, .0004, .0005}},
};*/



static void m3track_cockpitview(entity_t *pt, warf_t *w, double (*pos)[3], int *camera);
static void m3track_control(entity_t *pt, warf_t *w, const input_t *in, double dt);
static void m3track_drawCockpit(struct entity *pt, const warf_t *w, wardraw_t *wd);
static void m3track_anim(entity_t *pt, warf_t *w, double dt);
static void m3track_draw(entity_t *, wardraw_t *);
static int m3track_tracehit(struct entity *pt, warf_t *w, const double src[3], const double dir[3], double rad, double dt, double *ret, double (*retp)[3], double (*retn)[3]);

static struct entity_private_static m3track_s = {
	{
	tank_drawHUD,
	m3track_cockpitview,
	m3track_control,
	NULL, /* destruct */
	tank_getrot,
	NULL, /* getrotq */
	m3track_drawCockpit, /* drawCockpit */
	NULL, /* is_warping */
	NULL, /* warp_dest */
	tank_idname,
	tank_classname,
	},
	m3track_anim,
	m3track_draw,
	tank_drawtra,
	tank_takedamage,
	tank_gib_draw,
	tank_postframe,
	falsefunc,
	TURRETROTRANGE,
	-M_PI / 8., M_PI / 4.,
	NULL, NULL, NULL, NULL,
	0, /* reuse */
	TANKGUNSPEED, /* bulletspeed */
	0.004, /* getHitRadius */
	.0001, /* sufscale */
	0, 0, /* hitsuf, altaxis */
	NULL, /* bullethole */
	{0.}, /* cog */
	NULL, /* bullethit */
	m3track_tracehit, /* tracehit */
	{NULL}, /* hitmdl */
	tank_draw, /* shadowdraw */
};

entity_t *M3TrackNew(warf_t *w){
	tank2_t *p;
	entity_t *ret;
	p = malloc(sizeof *p);
	ret = &p->st;
	EntityInit(ret, w, &m3track_s);
	ret->health = 100.;
	ret->mass = 9000.;
	p->steer = 0.;
	p->wheelspeed = p->wheelangle = 0.;
	p->turrety = p->barrelp = 0.;
	p->cooldown = p->cooldown2 = RELOADTIME;
	p->mountpy[0] = p->mountpy[1] = 0.;
	p->ammo[0] = 35; /* main gun ammunition */
	p->ammo[1] = 2000; /* coaxial gun ammo (type 74 7.64mm) */
	p->ammo[2] = 1500; /* mounted gun ammo (M2 Browning 12.7mm) */
	p->muzzle = 0;
	return ret;
}

static void m3track_anim(entity_t *pt, warf_t *w, double dt){
	double h;
	double n[3] = {0., 1., 0.};
	double epos[3] = {0}; /* estimated enemy position */
	double mpos[3]; /* muzzle position */
	int mposa = 0; /* muzzle position availability */
	tank2_t *p = (tank2_t*)pt;
	h = w->map ? warmapheightr(w->map, pt->pos[0], pt->pos[2], &n) : 0.;

	if(!pt->active || w->pl->control == pt){
	}
	else{
/*		struct tent3d_line_list *tell = w->tell;*/
		struct entity_private_static *vft = (struct entity_private_static*)pt->vft;
/*		int j;*/
/*		struct bullet *pb, **ppb;*/
		double phi; /* enemy direction */

		/* find enemy logic */
		if(!pt->enemy){
			find_enemy_logic(pt, w);
		}

		/* estimating enemy position and wheeling towards enemy */
		pt->inputs.press = 0;
		if(pt->enemy){
			double diff, dphi;
			avec3_t dpos, dp, pyr;
			amat4_t mat, rot;
			double bulletspeed;
			int subweapon;

			subweapon = !p->ammo[0] /*|| ((struct entity_private_static*)pt->enemy->vft)->flying(pt->enemy)*/
				|| VECSDIST(pt->pos, pt->enemy->pos) < .1 * .1;
			bulletspeed = subweapon ? .8 : TANKGUNSPEED;

			if(((struct entity_private_static*)pt->enemy->vft)->flying(pt->enemy))
				VECCPY(epos, pt->enemy->pos);
			else
				estimate_pos(&epos, pt->enemy->pos, pt->enemy->velo, pt->pos, pt->velo, &sgravity, bulletspeed, w->map, w, ((struct entity_private_static*)pt->enemy->vft)->flying(pt->enemy));
			tankmuzzlepos(p, &mpos, NULL);
			mposa = 1;
			VECSUB(dpos, epos, mpos);
			quat2imat(&mat, pt->rot);
			MAT4VP3(dp, mat, dpos);
			pyrmat(pyr, &rot);
			phi = atan2(epos[0] - pt->pos[0], -(epos[2] - pt->pos[2]));
			dphi = atan2(dp[0], -dp[2]);
			shoot_angle(avec3_000, dp, dphi, bulletspeed, &p->desired);
			p->turrety = rangein(approach(p->turrety + M_PI, p->desired[1] + M_PI, TURRETROTSPEED * dt, 2 * M_PI) - M_PI, -vft->turretrange, vft->turretrange);
			p->barrelp = rangein(approach(p->barrelp + M_PI, p->desired[0] /*- pt->pyr[0]*/ + M_PI, TURRETROTSPEED * dt, 2 * M_PI) - M_PI, vft->barrelmin, vft->barrelmax);
/*			if(pt->pos[1] <= h)
				pt->pyr[1] = approach(pt->pyr[1] + M_PI, phi + M_PI, ROTSPEED * dt, 2 * M_PI) - M_PI;*/

			diff = fmod(phi - pt->pyr[1] + M_PI, 2. * M_PI) - M_PI;
			if(VECSDIST(epos, pt->pos) < .05 * .05);
			else if(diff < -.1 * M_PI)
				pt->inputs.press |= PL_A;
			else if(.1 * M_PI < diff)
				pt->inputs.press |= PL_D;
			else
				pt->inputs.press |= PL_W;

			{
				double yaw = pt->pyr[1] + p->turrety;
				double pitch = /*pt->pyr[0]*/ + p->barrelp;
				if(!rot && (INTOLERANCE < fabs(phi - yaw) || INTOLERANCE < fabs(pitch - p->desired[0])))
					pt->inputs.press &= ~PL_ENTER;
				else{
					pt->weapon = 2;
					pt->inputs.press |= PL_ENTER;
				}
			}

		}

		if(p->cooldown < dt)
			p->cooldown = 0.;
		else
			p->cooldown -= dt;
	}

    if(pt->weapon == 1 && pt->inputs.press & (PL_LCLICK | PL_ENTER)){
	}
	else if(pt->weapon == 2 && pt->inputs.press & (PL_LCLICK | PL_ENTER)) while(p->cooldown2 < dt){
		/* M2 */
		struct bullet *pb;
		static const double scale = 3.4 / 200. * 1e-3;
		avec3_t pos, pos0 = {0,0.002,0}, velo, velo0 = {0, 0, -.88};
		aquat_t qp = {0}, qy = {0}, q, q1, qr;
		double dev[2];
		dev[0] = p->mountpy[0] + (drseq(&w->rs) - .5) * .001 * M_PI;
		dev[1] = p->mountpy[1] + (drseq(&w->rs) - .5) * .001 * M_PI;

		qy[1] = sin(-p->turrety / 2.);
		qy[3] = cos(-p->turrety / 2.);
/*		qp[0] = sin(p->barrelp / 2.);
		qp[3] = cos(p->barrelp / 2.);*/
		QUATMUL(q, pt->rot, qy);
/*		QUATMUL(qr, q, qp);*/
		qy[1] = sin(-dev[1] / 2.);
		qy[3] = cos(-dev[1] / 2.);
		qp[0] = sin(dev[0] / 2.);
		qp[3] = cos(dev[0] / 2.);
		QUATMUL(q1, q, qy);
		QUATMUL(qr, q1, qp);

		quatrot(pos, q, pos0);
		VECADDIN(pos, pt->pos);
		quatrot(velo, qr, velo0);
		pb = BulletNew(w, pt, 5.);
		pb->mass = .008;
		VECCPY(pb->pos, pos);
		VECADD(pb->velo, velo, pt->velo);
		pb->life = 20.;
		pt->shoots2++;
		p->ammo[2]--;
		pb->vft->anim(pb, w, w->tell, dt - p->cooldown2);
		p->cooldown2 += 0.1;
		p->muzzle |= 4;

		playWave3DPitch("rc.zip/sound/rifle.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .02 * .02, w->realtime + dt - p->cooldown2, 192/* + rseq(&w->rs) % 256*/);
	}

	if(p->cooldown2 < dt)
		p->cooldown2 = 0.;
	else
		p->cooldown2 -= dt;

	{
		avec3_t accel;
		w->vft->accel(w, &accel, &pt->pos, &pt->velo);
		VECSADD(pt->velo, accel, dt);
/*	pt->velo[1] += gravity[1] * dt;*/
	}

	if(1){
		if(pt->inputs.press & PL_W)
			p->wheelspeed = approach(p->wheelspeed, 2. * M_PI * 5., dt, 0.);
/*			p->wheelangle += M_PI * dt;*/
		if(pt->inputs.press & PL_S)
			p->wheelspeed = approach(p->wheelspeed, -2. * M_PI * 5., dt, 0.);
/*			p->wheelangle -= M_PI * dt;*/
		p->wheelangle += p->wheelspeed * dt;
	}
	{
		avec3_t *points = m3track_points;
		int npoints = numof(m3track_points);
		entity_t *pt = &p->st;
		int in = pt->inputs.press;
		int leng, reng;
		unsigned long contact;
		avec3_t fore0 = {0., 0., - -.01};
		avec3_t vfronteng, vreareng;
		avec3_t *thrust[numof(m3track_points)] = {NULL};
		double frictions[numof(m3track_points)];
		amat4_t mat;
		int engflags = 0, i;

		p->steer = approach(p->steer, (pt->inputs.press & (PL_A | PL_D)) == PL_A ? -1. : (pt->inputs.press & (PL_A | PL_D)) == PL_D ? 1. : 0., dt, 0);

		leng = (in & PL_W ? 2 : 0) + (in & PL_S ? -2 : 0);
		reng = (in & PL_W ? 2 : 0) + (in & PL_S ? -2 : 0);
		if(leng | reng){
/*			VECSCALE(vrear, fore0, leng);*/
			vfronteng[0] = sin(p->steer) * fore0[2] * leng;
			vfronteng[1] = 0.;
			vfronteng[2] = cos(p->steer) * fore0[2] * leng;
		}
		for(i = 0; i < npoints; i++)
			frictions[i] = 1.;
		if(leng){
/*			VECSCALE(vleng, fore0, leng);*/
			thrust[0] /*= thrust[1]*/ = vfronteng;
			frictions[0] = 2.;
			frictions[1] = 0.5;
		}
		if(reng){
/*			VECSCALE(vreng, fore0, reng);*/
			thrust[2] /*= thrust[3]*/ = vfronteng;
			frictions[2] = 2.;
			frictions[3] = 0.5;
		}
		memset(((tank2_t*)pt)->forces, 0, sizeof ((tank2_t*)pt)->forces);
		memset(((tank2_t*)pt)->normals, 0, sizeof ((tank2_t*)pt)->normals);
		contact = RigidPointHit(pt, w, dt, points, thrust, npoints, frictions, ((tank2_t*)pt)->forces, ((tank2_t*)pt)->normals);

		tankrot(&mat, pt);
		for(i = 0; i < 4; i++){
			avec3_t pos;
			mat4vp3(pos, mat, points[i]);
			if(pos[1] < warmapheight(w->map, pos[0], pos[2], NULL) + .0005)
				contact |= 1 << i;
		}

		if(contact & (1 << 8))
			((struct entity_private_static*)pt->vft)->takedamage(pt, 100 * dt, w, 0);

		engflags |= reng < 0;
		engflags |= (0 < reng) << 1;
		engflags |= (0 < leng) << 2;
		engflags |= (leng < 0) << 3;
		for(i = 0; i < 4; i++) if(contact & engflags & (1 << i) && drseq(&w->rs) < dt / .1){
			avec3_t pos;
			mat4vp3(pos, mat, points[i]);
			pos[0] += (drseq(&w->rs) - .5) * .0005;
			pos[1] += (drseq(&w->rs) - .5) * .0005;
			pos[2] += (drseq(&w->rs) - .5) * .0005;
			AddTeline3D(w->tell, pos, NULL, .001, NULL, NULL, NULL, COLOR32RGBA(127,127,127,127), TEL3_SPRITE | TEL3_NOLINE | TEL3_INVROTATE, 1.);
		}
	}

	pt->pos[0] += pt->velo[0] * dt;
	pt->pos[1] += pt->velo[1] * dt;
	pt->pos[2] += pt->velo[2] * dt;

	{
		amat4_t nmat;
		aquat_t q;
		rbd_anim(pt, dt, &q, &nmat);
		QUATCPY(pt->rot, q);
		quat2pyr(pt->rot, pt->pyr);
	}

	VECSCALEIN(pt->omg, 1. / (dt + 1.));

}

static void m3track_draw(entity_t *pt, wardraw_t *wd){
	static int init = 0;
	static suf_t *suf = NULL, *sufwheel = NULL;
	tank2_t *p = (tank2_t*)pt;
	double pixels;
	if(!pt->active)
		return;

	/* cull object */
	if(glcullFrustum(&pt->pos, .007, wd->pgc))
		return;
	pixels = .005 * fabs(glcullScale(&pt->pos, wd->pgc));
	if(pixels < 2)
		return;
	wd->lightdraws++;

	if(!suf){
		suf = CallLoadSUF("m3track1_body.bin");
		sufwheel = CallLoadSUF("m3track1_wheel.bin");
	}
	if(!suf){
		double pos[3];
		GLubyte col[4] = {255,255,0,255};
		VECADD(pos, pt->pos, tank_offset);
		gldPseudoSphere(pos, tank_radius, col);
	}
	else{
		GLfloat fv[4] = {.8f, .5f, 0.f, 1.f}, ambient[4] = {.4f, .25f, 0.f, 1.f};
		static const double normal[3] = {0., 1., 0.}, mountpos[3] = {-33, 125, 45}, wheelpos[2][3] = {{90, 50, 211}, {-90, 50, 211}};
		double scale = 1e-5, tscale = scale, bscale = scale;
		if(pt->race % 2)
			fv[0] = 0., fv[2] = .8f, ambient[0] = 0., ambient[2] = .4f;

		glPushMatrix();
		{
			amat4_t mat;
			tankrot(&mat, pt);
			glMultMatrixd(mat);
		}

		glTranslated(0, -.0007, 0);

		glScaled(-scale, scale, -scale);

		DrawSUF(suf, SUF_ATR, &g_gldcache);

		glPushMatrix();
		gldTranslate3dv(wheelpos[0]);
		glRotated(30 * p->steer, 0., -1., 0.);
		glRotated(deg_per_rad * p->wheelangle, 1., 0., 0.);
		gldTranslaten3dv(wheelpos[0]);
		DrawSUF(sufwheel, SUF_ATR, &g_gldcache);
		glPopMatrix();

		gldTranslate3dv(wheelpos[1]);
		glRotated(30 * p->steer, 0., -1., 0.);
		glRotated(deg_per_rad * p->wheelangle, 1., 0., 0.);
		gldTranslaten3dv(wheelpos[0]);
		DrawSUF(sufwheel, SUF_ATR, &g_gldcache);
#if 0
		glRotated(deg_per_rad * p->turrety, 0., -1., 0.);
		if(wd->w->pl->chase != pt || wd->w->pl->chasecamera)
			DrawSUF(tank_s.sufturret, SUF_ATR, &g_gldcache);

		glPushMatrix();
		gldTranslaten3dv(mountpos);
		glRotated(deg_per_rad * p->mountpy[1], 0., -1., 0.);
		glRotated(deg_per_rad * p->mountpy[0], 1., 0., 0.);
		gldTranslate3dv(mountpos);
		DrawSUF(sufm2, SUF_ATR, &g_gldcache);
		glPopMatrix();

		glTranslated(0., 90., -85.);
		glRotated(deg_per_rad * p->barrelp, 1., 0., 0.);

		/* level of detail */
		if(0 && 100 < pixels){
			glScaled(1. / tscale, 1. / tscale, 1. / tscale);
	/*		glTranslated(0., .0025, -0.0015);*/
			glScaled(bscale, bscale, bscale);
			DrawSUF(tank_s.sufbarrel1, SUF_ATR, &g_gldcache);
		}
		else{
			glTranslated(0., -90., 85.);
/*			glTranslated(0., -.0025 / tscale, 0.0015 / tscale);*/
			DrawSUF(tank_s.sufbarrel, SUF_ATR, &g_gldcache);
		}
#endif

		glPopMatrix();
	}
#ifndef NDEBUG
	{
		int i;
		avec3_t org;
		amat4_t mat;
		struct hitbox *hb = m3track_hb;
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
/*		glRotated(pt->turrety * deg_per_rad, 0, 1., 0.);*/
/*		gldTranslate3dv(org);*/

		for(i = 0; i < numof(tank_hb); i++){
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

static void m3track_drawCockpit(struct entity *pt, const warf_t *w, wardraw_t *wd){
	static int init = 0;
	static suf_t *sufcockpit = NULL, *sufhandle = NULL;
	tank2_t *p = (tank2_t*)pt;
	double scale = .00001 /*wd->pgc->znear / wd->pgc->zfar*/;
	double sonear = scale * wd->pgc->znear;
	double wid = sonear * wd->pgc->fov * wd->pgc->width / wd->pgc->res;
	double hei = sonear * wd->pgc->fov * wd->pgc->height / wd->pgc->res;
	double sufscale = 1e-5;
	const double *seat = m3track_cockpit_ofs;
	amat4_t rot;
	if(w->pl->chasecamera)
		return;

	if(!init){
		init = 1;
		sufcockpit = CallLoadSUF("m3track1_int.bin");
		sufhandle = CallLoadSUF("m3track1_handle.bin");
	}

	glLoadMatrixd(*wd->rot);
	quat2mat(&rot, pt->rot);
	glMultMatrixd(rot);

	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glClearDepth(1.);
	glClear(GL_DEPTH_BUFFER_BIT);

	glPushMatrix();

	glMatrixMode (GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glFrustum (-wid, wid,
		-hei, hei,
		sonear, wd->pgc->znear * 50.);
	glMatrixMode (GL_MODELVIEW);

	{
		gldTranslaten3dv(seat);
		gldScaled(sufscale);
		glRotated(180, 0, 1, 0);
		DrawSUF(sufcockpit, SUF_ATR, &g_gldcache);
		glTranslated(50, 130, 0);
		glRotated(p->steer * deg_per_rad, 0, 0, 1.);
		glTranslated(-50, -130, 0);
		DrawSUF(sufhandle, SUF_ATR, &g_gldcache);
	}

	glPopMatrix();

	glMatrixMode (GL_PROJECTION);
	glPopMatrix();
	glMatrixMode (GL_MODELVIEW);

	glPopAttrib();
}

static void m3track_cockpitview(entity_t *pt, warf_t *w, double (*pos)[3], int *pcamera){
	tank2_t *p = (tank2_t*)pt;
	static const avec3_t offset0 = {0., .010, .025};
	static const double *const ofs[2] = {m3track_cockpit_ofs, offset0};
	amat4_t mat, mat2;
	int camera = *pcamera;

/*	tankrot(&mat, pt);*/
	quat2mat(&mat, pt->rot);
	VECADDIN(&(mat)[12], pt->pos);
	mat4translaten3dv(mat, m3track_cog);

	camera = *pcamera = camera < -2 ? 0 : (camera + 3) % 3;
	if(camera == 2){
		struct bullet *pb;
		for(pb = w->bl; pb; pb = pb->next) if(pb && pb->active && pb->owner == pt){
			VECCPY(*pos, pb->pos);
			return;
		}
		camera = 0;
	}
	else if(camera){
		if(pt->weapon == 2){
			mat4roty(mat2, mat, -(p->turrety + p->mountpy[1]));
			mat4rotx(mat, mat2, p->mountpy[0]);
		}
		else{
			mat4roty(mat2, mat, -p->turrety);
			mat4rotx(mat, mat2, p->barrelp);
		}
	}
	mat4vp3(*pos, mat, ofs[camera]);
}

static void m3track_control(entity_t *pt, warf_t *w, const input_t *in, double dt){
	static int prev = 0;
	tank2_t *p = (tank2_t*)pt;
	struct entity_private_static *vft = (struct entity_private_static*)pt->vft;
	double h, n[3], pyr[3];
	int inputs = in->press;
	h = w->map ? warmapheight(w->map, pt->pos[0], pt->pos[2], &n) : 0.;

	pt->inputs = *in;
	quat2pyr(w->pl->rot, pyr);
	if(pt->weapon != 2){
		p->turrety = approach(p->turrety, pyr[1] - floor(pyr[1] / 2. / M_PI) * 2. * M_PI, TURRETROTSPEED * dt, 2 * M_PI);
		p->barrelp = rangein(approach(p->barrelp, -pyr[0], TURRETROTSPEED * dt, 2 * M_PI), vft->barrelmin, vft->barrelmax);
	}
	else{
		pyr[1] -= p->turrety;
		p->mountpy[1] = approach(p->mountpy[1], pyr[1] - floor(pyr[1] / 2. / M_PI) * 2. * M_PI, 2 * TURRETROTSPEED * dt, 2 * M_PI);
		p->mountpy[0] = rangein(approach(p->mountpy[0], -pyr[0], 2 * TURRETROTSPEED * dt, 2 * M_PI), vft->barrelmin, vft->barrelmax);
	}

	if((inputs & in->change) & (PL_TAB | PL_RCLICK)){
		pt->weapon = (pt->weapon + 1) % 4;
	}

	if(inputs & PL_SPACE && !(prev & PL_SPACE)){
		double best = 3. * 3.;
		double mini = pt->enemy ? VECSDIST(pt->pos, pt->enemy->pos) : 0.;
		double sdist;
		entity_t *t = w->tl;
		entity_t *target = NULL;
		for(t = w->tl; t; t = t->next) if(t != pt && t != pt->enemy && t->active && 0. < t->health && mini < (sdist = VECSDIST(pt->pos, t->pos)) && sdist < best){
			best = sdist;
			target = t;
		}
		if(target)
			pt->enemy = target;
		else{
			for(t = w->tl; t; t = t->next) if(t != pt && t != pt->enemy && t->active && 0. < t->health && (sdist = VECSDIST(pt->pos, t->pos)) < best){
				best = sdist;
				target = t;
			}
			pt->enemy = target;
		}
	}

	if(inputs & (PL_ENTER | PL_LCLICK) && p->cooldown < dt){
	}
	if(p->cooldown < dt)
		p->cooldown = 0.;
	else p->cooldown -= dt;

	prev = inputs;
}

static int m3track_tracehit(struct entity *pt, warf_t *w, const double src[3], const double dir[3], double rad, double dt, double *ret, double (*retp)[3], double (*retn)[3]){
	entity_tracehit(pt, w, src, dir, rad, dt, ret, retp, retn, m3track_hb, numof(m3track_hb));
}

#endif
