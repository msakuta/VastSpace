/** \file
 * \brief Implementation of Tank class.
 */
#define NOMINMAX
#include "Tank.h"
#include "EntityRegister.h"
#include "SurfaceCS.h"
#include "Bullet.h"
#include "motion.h"
#include "glw/GLWchart.h"
#include "SqInitProcess-ex.h"
#include "audio/playSound.h"
#include "audio/wavemixer.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
}



#define FORESPEED 0.004
#define ROTSPEED (.075 * M_PI)
#define GUNVARIANCE .005
#define INTOLERANCE (M_PI / 50.)
#define BULLETRANGE 3.
#define RETHINKTIME .5
#define TANK_MAX_GIBS 30


Model *Tank::model = NULL;
double Tank::modelScale = 3.33 / 200 * 1e-3;
double Tank::landOffset = 0.0007;
double Tank::defaultMass = 50000.; ///< Mass defaults 50 tons
double Tank::maxHealthValue = 800.;
double Tank::topSpeed = 70. / 3.600; /// < Default 70 km/h
double Tank::backSpeed = 35. / 3.600; /// < Default half a topSpeed
double Tank::mainGunCooldown = 3.;
double Tank::mainGunMuzzleSpeed = 1.7;
double Tank::mainGunDamage = 500.;
double Tank::turretYawSpeed = 0.1 * M_PI;
double Tank::barrelPitchSpeed = 0.05 * M_PI;
double Tank::barrelPitchMin = -0.05 * M_PI;
double Tank::barrelPitchMax = 0.3 * M_PI;
double Tank::sightCheckInterval = 1.;
HitBoxList Tank::hitboxes;


const char *Tank::idname()const{return "tank";}
const char *Tank::classname()const{return "Tank";}

const unsigned Tank::classid = registerClass("Tank", Conster<Tank>);
Entity::EntityRegister<Tank> Tank::entityRegister("Tank");


Tank::Tank(Game *game) : st(game), gunsid(0){
	init();
}

Tank::Tank(WarField *aw) : st(aw), gunsid(0){
	init();
	health = getMaxHealth();
	steer = 0.;
	wheelspeed = wheelangle = 0.;
	turrety = barrelp = 0.;
	cooldown = cooldown2 = mainGunCooldown;
	mountpy[0] = mountpy[1] = 0.;
	ammo[0] = 35; /* main gun ammunition */
	ammo[1] = 2000; /* coaxial gun ammo (type 74 7.64mm) */
	ammo[2] = 1500; /* mounted gun ammo (M2 Browning 12.7mm) */
	muzzle = 0;
	sightCheckTime = aw->rs.nextd() * sightCheckInterval; // Randomize check phase to diverse load over frames
	sightCheck = false;
}

bool LandVehicle::buildBody(){
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
			SingleDoubleProcess(modelScale, "modelScale") <<=
			SingleDoubleProcess(landOffset, "landOffset") <<=
			SingleDoubleProcess(defaultMass, "mass") <<=
			SingleDoubleProcess(maxHealthValue, "maxhealth", false) <<=
			SingleDoubleProcess(topSpeed, "topSpeed") <<=
			SingleDoubleProcess(backSpeed, "backSpeed") <<=
			SingleDoubleProcess(mainGunCooldown, "mainGunCooldown") <<=
			SingleDoubleProcess(mainGunMuzzleSpeed, "mainGunMuzzleSpeed") <<=
			SingleDoubleProcess(mainGunDamage, "mainGunDamage") <<=
			SingleDoubleProcess(turretYawSpeed, "turretYawSpeed") <<=
			SingleDoubleProcess(barrelPitchSpeed, "barrelPitchSpeed") <<=
			SingleDoubleProcess(barrelPitchMin, "barrelPitchMin") <<=
			SingleDoubleProcess(barrelPitchMax, "barrelPitchMax") <<=
			SingleDoubleProcess(sightCheckInterval, "sightCheckInterval") <<=
			HitboxProcess(hitboxes));
		initialized = true;
	}
	mass = defaultMass;
}

void LandVehicle::addRigidBody(WarSpace *ws){
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
	rot.translatein(0, 90 * modelScale, -85 * modelScale);
	mat = rot.rotx(barrelp);

	if(nh)
		*nh = mat.dvp3(velo0);
	return mat.vp3(pos0);

}

int Tank::shootcannon(double dt){
//	struct bullet *pb;
	const double v = mainGunMuzzleSpeed;
	Vec3d dir;
	const Vec3d gunPos = tankMuzzlePos(&dir);
	while(this->cooldown < dt){
		int i = 0;
		Bullet *pb = new APFSDS(this, 2., mainGunDamage);
		w->addent(pb);

		pb->mass = .005;
		pb->pos = gunPos;
		pb->velo = dir * v + this->velo;
		for(int j = 0; j < 3; j++)
			pb->velo[j] += (drseq(&w->rs) - .5) * .005;
		pb->anim(dt - this->cooldown);

#if 1

/*		double phi = (rot ? phi0 : yaw) + (drseq(&gsrs) - .5) * variance;
		double theta = pt->pyr[0] + (drseq(&gsrs) - .5) * variance;
		theta += acos(vx / v);*/
#if 0
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
#endif
#if 1
		if(WarSpace *ws = *w){
			const Vec3d velo0 = dir * .005 + this->velo;
			for(int i = 0; i < 8; i++){
				Vec3d velo, pos;
				for(int j = 0; j < 3; j++)
					velo[j] = velo0[j] + (drseq(&w->rs) - .5) * .005;
				for(int j = 0; j < 3; j++)
					pos[j] = gunPos[j] + (drseq(&w->rs) - .5) * .005;
				COLOR32 col;
				if(i < 4)
					col = COLOR32RGBA(255, 191 + rseq(&w->rs) % 32, 127,255);
				else
					col = COLOR32RGBA(191 + rseq(&w->rs) % 64, 191, 191, 127);
				AddTeline3D(ws->tell, pos, velo, 0.005, quat_u, vec3_000, vec3_000, col, TEL3_SPRITE | TEL3_NOLINE | TEL3_INVROTATE, i < 4 ? .2 : 1.5);
			}
		}
//		pt->shoots++;
//		p->ammo[0]--;
		this->cooldown += mainGunCooldown;
		this->muzzle |= 1;

#		ifndef DEDICATED
		gunsid = playSound3D(modPath() << "sound/tank-gun.wav", this->pos, 1., .2, w->realtime);
#		endif
#endif
#endif
	}
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

void LandVehicle::find_enemy_logic(){
	double best = 100. * 100.; /* sense range */
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

static btVector3 alignAxis(const btVector3 &v, const btVector3 &a){
	return a * (a.normalized().dot(v));
}

void LandVehicle::anim(double dt){
	double h;
	Vec3d normal(0,1,0);

	bool floorTouch = false;
	btVector3 worldNormal(0,1,0);
	if(WarSpace *ws = *w){
		const btVector3 offset(0, getLandOffset(), 0);
		const btvc btPos = bbody->getWorldTransform().getOrigin() - offset;
		const btvc btVelo = bbody->getLinearVelocity();
		GLWchart::addSampleToCharts("tankvelo", btVelo.len());
		const Vec3d delta(0, -0.02 - btVelo[1] * dt, 0);
		const Vec3d start(btPos - delta);

		// If the CoordSys is a SurfaceCS, we can expect ground in negative y direction.
		// dynamic_cast should be preferred.
		if(&w->cs->getStatic() == &SurfaceCS::classRegister){
			SurfaceCS *s = static_cast<SurfaceCS*>(w->cs);
			RoundAstrobj *cbody = s->getCelBody();
			if(cbody){
				Vec3d basepos = cbody->tocs(btPos, s);
				Vec3d basenorm = basepos.norm();
				double terrain = cbody->getTerrainHeight(basenorm) * cbody->rad;
				if(basepos.slen() < terrain * terrain){
					basepos = basenorm * terrain;
					Vec3d lpos = s->tocs(basepos, cbody);
					worldNormal = btvc(s->tocs(basenorm, cbody, true));
					Vec3d newVelo = (btVelo - worldNormal.dot(btVelo) * worldNormal) * exp(-dt);
					setPosition(&lpos, NULL, &newVelo);
					floorTouch = true;
				}
			}
			else{
				double height = s->getHeight(btPos[0], btPos[2], &normal);
				worldNormal = btvc(normal);
				if(btPos[1] - offset[1] < height){
					Vec3d dest(btPos[0], height + offset[1], btPos[2]);
					Vec3d newVelo = (btVelo - worldNormal.dot(btVelo) * worldNormal) * exp(-dt);
					setPosition(&dest, NULL, &newVelo);
					floorTouch = true;
				}
			}
		}
	}

	if(floorTouch){
		btVector3 btOmega = bbody->getAngularVelocity() - worldNormal.cross(bbody->getWorldTransform().getBasis().getColumn(1)) * 5. * dt;
		bbody->setAngularVelocity(btOmega * exp(-3. * dt));
	}

	if(!w || controller){
	}
	else if(0 < getHealth()){
		aiControl(dt, normal);
		inputs.press |= PL_W;
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

	if(!bbody){
		Vec3d accel = w->accel(pos, velo);
		velo += accel * dt;
	}

	if(floorTouch){
		if(inputs.press & PL_W){
			bbody->applyCentralForce(alignPlane(btvc(rot.trans(Vec3d(0,0,-1)) * mass * getTopSpeed()), worldNormal));
		}
		if(inputs.press & PL_S){
			bbody->applyCentralForce(alignPlane(btvc(rot.trans(Vec3d(0,0,-1)) * mass * -getBackSpeed()), worldNormal));
		}

		// The behavior of steering handle differs between tracked vehicle and wheeled vehicle.
		// Notably, backward steering direction is opposite.
		if(isTracked()){
			if(inputs.press & PL_A){
				bbody->applyTorque(btvc(rot.trans(Vec3d(0,1,0)) * mass * 0.5 * 1e-5));
			}
			if(inputs.press & PL_D){
				bbody->applyTorque(btvc(rot.trans(Vec3d(0,1,0)) * mass * -0.5 * 1e-5));
			}
		}
		else{
			// If either one of steering keys is pressed, head to that direction.
			if(inputs.press & PL_A)
				steer = approach(steer, -getMaxSteeringAngle(), dt * getMaxSteeringAngle(), 0);
			if(inputs.press & PL_D)
				steer = approach(steer, getMaxSteeringAngle(), dt * getMaxSteeringAngle(), 0);
			if(!(inputs.press & (PL_A | PL_D)))
				steer = approach(steer, 0., dt * getMaxSteeringAngle(), 0); // Otherwise approach to neutral

			// Head to steered direction.
			btTransform worldTrans = bbody->getWorldTransform();
			double speed = worldTrans.getBasis().getColumn(2).dot(bbody->getLinearVelocity());
			worldTrans.setRotation(worldTrans.getRotation() * btQuaternion(btVector3(0, 1, 0), speed * steer / 2. * dt / getWheelBase()));
			bbody->setWorldTransform(worldTrans);
		}

		// Cancel lateral velocity
		bbody->setLinearVelocity(alignAxis(bbody->getLinearVelocity(), bbody->getWorldTransform().getBasis().getColumn(2)));

		if(!(inputs.press & (PL_W | PL_S))){
			// Precisely, friction should be affected by gravity acceleration.
			const double friction = 0.005 * dt;
			btVector3 btVelo = bbody->getLinearVelocity();
			if(btVelo.length2() < friction * friction)
				btVelo.setZero();
			else
				btVelo *= 1. - friction / btVelo.length();
			bbody->setLinearVelocity(btVelo);
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
		velo = btvc(bbody->getLinearVelocity());
		rot = btqc(bbody->getOrientation());
		omg = btvc(bbody->getAngularVelocity());
	}

}

int Tank::takedamage(double damage, int hitpart){
	/* tank armor is bullet resistant */
	if(hitpart == 1)
		damage *= .3;
	else if(hitpart == 3)
		damage *= .5;

#if 1
	if(health <= damage){ /* death! */
		deathEffects();

#if 0
		pt->deaths++;
		if(pt->race < numof(w->races))
			w->races[pt->race].deaths++;
		if(pt->enemy)
			pt->enemy = NULL;
#endif

		/* forget about dead enemy */
		for(WarField::EntityList::iterator it = w->entlist().begin(); it != w->entlist().end(); ++it) if(*it == this->enemy)
			this->enemy = NULL;

#if 0
		pt->pos[0] = w->rs.nextd() - .5;
		pt->pos[2] = w->rs.nextd() - .5;
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
#endif
		if(game->isServer())
			delete this;
		return 0;
	}
	else{
		health -= damage;
/*		playWave3D("hit.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .001);*/
		return 1;
	}
#endif
	return 0;
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



void LandVehicle::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	static  const Vec3d ofs[2] = {Vec3d(-.0006, .0010, -.0005), Vec3d(0., .010, .025)};
	Mat4d mat, mat2;
	int camera = seatid;
	transform(mat);
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

	// Face to the turret's aiming point
	rot = this->rot.rotate(turrety, 0, -1, 0).rotate(barrelp, 1, 0, 0);
}

double Tank::getHitRadius()const{
	return 0.007;
}

static double fpmod(double a, double d){
	return a - floor(a / d) * d;
}

void Tank::control(const input_t *in, double dt){
	st::control(in, dt);

	static const double mouseSensitivity = 0.01;

	// Rotate the turret and barrel
	desired[0] += in->analog[0] * turretYawSpeed * mouseSensitivity;
	desired[0] = fpmod(desired[0], 2 * M_PI);
	desired[1] += -in->analog[1] * barrelPitchSpeed * mouseSensitivity;
	desired[1] = rangein(desired[1], barrelPitchMin, barrelPitchMax);
	turrety = approach(turrety, desired[0], dt * turretYawSpeed, 2 * M_PI);
	barrelp = approach(barrelp, desired[1], dt * barrelPitchSpeed, 0);

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

int LandVehicle::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	double best = dt;
	int reti = 0, i = 0;
	double retf;
	HitBoxList &hitboxes = getHitBoxes();
	for(HitBoxList::iterator it = hitboxes.begin(); it != hitboxes.end(); ++it){
		hitbox &hb = *it;
		Vec3d org = this->rot.trans(hb.org) + this->pos;
		Quatd rot = this->rot * hb.rot;
		double sc[3];
		for(int j = 0; j < 3; j++)
			sc[j] = hb.sc[j] + rad;
		if((jHitBox(org, sc, rot, src, dir, 0., best, &retf, retp, retn)) && (retf < best)){
			best = retf;
			if(ret) *ret = retf;
			reti = i + 1;
		}
		++i;
	}
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

void Tank::anim(double dt){
	st::anim(dt);

	if(0 < getHealth()){
		// Main trigger
		if(inputs.press & (PL_ENTER | PL_LCLICK)){
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

#ifndef DEDICATED
	if(gunsid != 0)
		movesound3d(gunsid, this->pos);
#endif

}

void Tank::aiControl(double dt, const Vec3d &normal){
	Vec3d epos(0,0,0); /* estimated enemy position */
		/* find enemy logic */
		if(!enemy){
			find_enemy_logic();
		}

		/* estimating enemy position and wheeling towards enemy */
		inputs.press = 0;
		if(enemy){
			Vec3d mdir;
			Vec3d mpos = tankMuzzlePos(&mdir); // Muzzle position

			bool subweapon = !ammo[0] /*|| ((struct entity_private_static*)pt->enemy->vft)->flying(pt->enemy)*/
				|| normal.sp(mdir) < -.2
				|| enemy->getHealth() < 50. && (pos - enemy->pos).slen() < .1 * .1;
			double bulletspeed = subweapon ? .8 : mainGunMuzzleSpeed;

			/* calculate tr(pb->pos) * pb->pyr * pt->pos to get global coords */
			Mat4d mat2 = this->rot.cnj().tomat4().translatein(-this->pos);
			Vec3d pos = mat2.vp3(enemy->pos);
			Vec3d velo = mat2.dvp3(enemy->velo);
			Vec3d pvelo = mat2.dvp3(this->velo);

			estimate_pos(epos, pos, velo, vec3_000, pvelo, bulletspeed, NULL);
			double phi = atan2(epos[0], -epos[2]);
			double theta = atan2(epos[1], sqrt(epos[0] * epos[0] + epos[2] * epos[2]));

			// Rotate the turret and barrel
			turrety = approach(turrety, phi, dt * turretYawSpeed, 2 * M_PI);
			barrelp = rangein(approach(barrelp + M_PI, theta + M_PI, dt * barrelPitchSpeed, 2 * M_PI) - M_PI, barrelPitchMin, barrelPitchMax);

			// If you're too close to the enemy, do not dare enclosing further.  This threshold is hard-coded for now.
			if(epos.slen() < .05 * .05)
				; // Do nothing
			else if(phi < -.1 * M_PI)
				inputs.press |= PL_A;
			else if(.1 * M_PI < phi)
				inputs.press |= PL_D;
			else
				inputs.press |= PL_W;

			if(fabs(turrety - phi) < 0.01 * M_PI && fabs(barrelp - theta) < 0.01 * M_PI){
				// If we're in a SurfaceCS, check if we can see the target clearly.
				// dynamic_cast should be preferred.
				if(&w->cs->getStatic() == &SurfaceCS::classRegister){
					SurfaceCS *s = static_cast<SurfaceCS*>(w->cs);
					if(sightCheckTime < dt){
						if(!s->traceHit(this->pos, (enemy->pos - this->pos).norm(), 0, 100., NULL, NULL, NULL)){
							inputs.press |= PL_ENTER;
							sightCheck = true; // Remember for the next frames
						}
						else
							sightCheck = false; // Remember for the next frames
						sightCheckTime += sightCheckInterval - dt;
					}
					else{
						sightCheckTime -= dt;
						if(sightCheck) // Recall memory
							inputs.press |= PL_ENTER;
					}
				}
				else
					inputs.press |= PL_ENTER;
			}
	

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

#ifdef DEDICATED
void Tank::deathEffects(){}
void Tank::draw(WarDraw*){}
void Tank::drawtra(WarDraw*){}
void APFSDS::draw(WarDraw*){}
void APFSDS::drawtra(WarDraw*){}
#endif


































//-----------------------------------------------------------------------------
//    Implementation for M3Truck
//-----------------------------------------------------------------------------

Model *M3Truck::model = NULL;
double M3Truck::modelScale = 3.4 / 200. * 1e-3;
double M3Truck::landOffset = 0.0007;
double M3Truck::defaultMass = 9000.; ///< Mass defaults 9 tons
double M3Truck::maxHealthValue = 150.;
double M3Truck::topSpeed = 100. / 3.600; ///< Default 100 km/h
double M3Truck::backSpeed = 40. / 3.600; ///< Default half a topSpeed
double M3Truck::turretCooldown = 0.2;
double M3Truck::turretMuzzleSpeed = 0.7;
double M3Truck::turretDamage = 10;
double M3Truck::turretYawSpeed = 0.3 * M_PI;
double M3Truck::barrelPitchSpeed = 0.2 * M_PI;
double M3Truck::barrelPitchMin = -0.05 * M_PI;
double M3Truck::barrelPitchMax = 0.3 * M_PI;
double M3Truck::sightCheckInterval = 1.;
std::vector<Vec3d> M3Truck::cameraPositions;
HitBoxList M3Truck::hitboxes;

const char *M3Truck::idname()const{return "M3Truck";}
const char *M3Truck::classname()const{return "M3Truck";}

const unsigned M3Truck::classid = registerClass("M3Truck", Conster<M3Truck>);
Entity::EntityRegister<M3Truck> M3Truck::entityRegister("M3Truck");


M3Truck::M3Truck(WarField *w) : st(w){
	init();
	health = maxHealthValue;
	mass = defaultMass;
	turrety = barrelp = 0.;
	cooldown = cooldown2 = turretCooldown;
//	steer = 0.;
//	wheelspeed = p->wheelangle = 0.;
	sightCheckTime = w->rs.nextd() * sightCheckInterval; // Randomize check phase to diverse load over frames
	sightCheck = false;
}

void M3Truck::init(){
	static bool initialized = false;
	if(!initialized){
		SqInit(game->sqvm, modPath() << _SC("models/m3truck.nut"),
			SingleDoubleProcess(modelScale, "modelScale") <<=
			SingleDoubleProcess(landOffset, "landOffset") <<=
			SingleDoubleProcess(defaultMass, "mass") <<=
			SingleDoubleProcess(maxHealthValue, "maxhealth", false) <<=
			SingleDoubleProcess(topSpeed, "topSpeed") <<=
			SingleDoubleProcess(backSpeed, "backSpeed") <<=
			SingleDoubleProcess(turretCooldown, "turretCooldown") <<=
			SingleDoubleProcess(turretMuzzleSpeed, "turretMuzzleSpeed") <<=
			SingleDoubleProcess(turretDamage, "turretDamage") <<=
			SingleDoubleProcess(turretYawSpeed, "turretYawSpeed") <<=
			SingleDoubleProcess(barrelPitchSpeed, "barrelPitchSpeed") <<=
			SingleDoubleProcess(barrelPitchMin, "barrelPitchMin") <<=
			SingleDoubleProcess(barrelPitchMax, "barrelPitchMax") <<=
			SingleDoubleProcess(sightCheckInterval, "sightCheckInterval") <<=
			Vec3dListProcess(cameraPositions, "cameraPositions") <<=
			HitboxProcess(hitboxes));
		initialized = true;
	}
	mass = defaultMass;
}


void M3Truck::anim(double dt){
	st::anim(dt);

	if(0 < getHealth()){
		// Main trigger
		if(inputs.press & (PL_ENTER | PL_LCLICK)){
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
}

int M3Truck::takedamage(double damage, int hitpart){

	if(health <= damage){ /* death! */
		deathEffects();

		if(game->isServer())
			delete this;
		return 0;
	}
	else{
		health -= damage;
/*		playWave3D("hit.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .001);*/
		return 1;
	}
	return 0;
}

void M3Truck::aiControl(double dt, const Vec3d &normal){
	Vec3d epos(0,0,0); /* estimated enemy position */

	/* find enemy logic */
	if(!enemy){
		find_enemy_logic();
	}

	/* estimating enemy position and wheeling towards enemy */
	inputs.press = 0;
	if(enemy){
		Vec3d mdir;
		Vec3d mpos = turretMuzzlePos(&mdir); // Muzzle position

		bool subweapon = !ammo[0] /*|| ((struct entity_private_static*)pt->enemy->vft)->flying(pt->enemy)*/
			|| normal.sp(mdir) < -.2
			|| enemy->getHealth() < 50. && (pos - enemy->pos).slen() < .1 * .1;
		double bulletspeed = subweapon ? .8 : turretMuzzleSpeed;

		/* calculate tr(pb->pos) * pb->pyr * pt->pos to get global coords */
		Mat4d mat2 = this->rot.cnj().tomat4().translatein(-mpos);
		Vec3d pos = mat2.vp3(enemy->pos);
		Vec3d velo = mat2.dvp3(enemy->velo);
		Vec3d pvelo = mat2.dvp3(this->velo);

		estimate_pos(epos, pos, velo, vec3_000, pvelo, bulletspeed, NULL);
		double phi = atan2(epos[0], -epos[2]);
		double theta = atan2(epos[1], sqrt(epos[0] * epos[0] + epos[2] * epos[2]));

		// Rotate the turret and barrel
		turrety = approach(turrety, phi, dt * turretYawSpeed, 2 * M_PI);
		barrelp = rangein(approach(barrelp + M_PI, theta + M_PI, dt * barrelPitchSpeed, 2 * M_PI) - M_PI, barrelPitchMin, barrelPitchMax);

		// If you're too close to the enemy, do not dare enclosing further.  This threshold is hard-coded for now.
		if(epos.slen() < .05 * .05)
			; // Do nothing
		else{
			if(phi < -.1 * M_PI)
				inputs.press |= PL_A;
			else if(.1 * M_PI < phi)
				inputs.press |= PL_D;
			inputs.press |= PL_W;
		}

		// Do not shoot outside effective range of M2 machine gun.
		if(epos.slen() < 2 * 2 && fabs(turrety - phi) < 0.01 * M_PI && fabs(barrelp - theta) < 0.01 * M_PI){
			// If we're in a SurfaceCS, check if we can see the target clearly.
			// dynamic_cast should be preferred.
			if(&w->cs->getStatic() == &SurfaceCS::classRegister){
				SurfaceCS *s = static_cast<SurfaceCS*>(w->cs);
				if(sightCheckTime < dt){
					if(!s->traceHit(this->pos, (enemy->pos - this->pos).norm(), 0, 100., NULL, NULL, NULL)){
						inputs.press |= PL_ENTER;
						sightCheck = true; // Remember for the next frames
					}
					else
						sightCheck = false; // Remember for the next frames
					sightCheckTime += sightCheckInterval - dt;
				}
				else{
					sightCheckTime -= dt;
					if(sightCheck) // Recall memory
						inputs.press |= PL_ENTER;
				}
			}
			else
				inputs.press |= PL_ENTER;
		}

	}
}


bool M3Truck::tryshoot(double dt){
	if(dt <= this->cooldown)
		return false;

//	struct bullet *pb;
	const double v = turretMuzzleSpeed;
	Vec3d dir;
	const Vec3d gunPos = turretMuzzlePos(&dir);
	while(this->cooldown < dt){
		int i = 0;
		Bullet *pb = new Bullet(this, 2., turretDamage);
		w->addent(pb);

		pb->mass = .005;
		pb->pos = gunPos;
		pb->velo = dir * v + this->velo;
		for(int j = 0; j < 3; j++)
			pb->velo[j] += (drseq(&w->rs) - .5) * .005;
		pb->anim(dt - this->cooldown);

		/* M2 */
		static const double scale = modelScale;
		avec3_t pos, pos0 = {0,0.002,0}, velo, velo0 = {0, 0, -.88};
		aquat_t qp = {0}, qy = {0}, q, q1, qr;
		double dev[2];

		this->cooldown += turretCooldown;
		this->muzzle |= 1;

//		playWave3DPitch("rc.zip/sound/rifle.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .02 * .02, w->realtime + dt - p->cooldown2, 192/* + rseq(&w->rs) % 256*/);
	}
	return true;
}

Vec3d M3Truck::turretMuzzlePos(Vec3d *nh)const{
	static const Vec3d velo0(0., 0., -1.);
	const Vec3d pos0 = Vec3d(0., 200, -50) * modelScale;
	Mat4d mat, rot;

	transform(mat);
	mat.translatein(-tank_cog[0], -tank_cog[1], -tank_cog[2]);
	rot = mat.roty(-turrety);
	rot.translatein(0, 90 * modelScale, -85 * modelScale);
	mat = rot.rotx(barrelp);

	if(nh)
		*nh = mat.dvp3(velo0);
	return mat.vp3(pos0);

}

void M3Truck::cockpitView(Vec3d &pos, Quatd &rot, int chasecam)const{
	int camera = std::max(0, std::min(int(cameraPositions.size()) + 1, chasecam));

	Mat4d orgmat;
	transform(orgmat);
	Mat4d mat = orgmat.roty(-turrety).rotx(barrelp);

	if(camera == cameraPositions.size()){
		const Player *player = game->player;
		Vec3d ofs = mat.dvp3(vec3_001);
		if(camera)
			ofs *= player ? player->viewdist : 1.;
		pos = this->pos + ofs;
	}
	else if(camera == cameraPositions.size() + 1){
		Vec3d pos0;
		const double period = this->velo.len() < .1 * .1 ? .5 : 1.;
		struct contact_info ci;
		pos0[0] = floor(this->pos[0] / period + .5) * period;
		pos0[1] = floor(this->pos[1] / period + .5) * period;
		pos0[2] = floor(this->pos[2] / period + .5) * period;
		pos = pos0;
	}
	else if(camera == 0)
		pos = orgmat.vp3(cameraPositions[camera]);
	else
		pos = mat.vp3(cameraPositions[camera]);

	// Face to the turret's aiming point
	rot = this->rot.rotate(turrety, 0, -1, 0).rotate(barrelp, 1, 0, 0);
}


void M3Truck::control(const input_t *in, double dt){
	st::control(in, dt);

	static const double mouseSensitivity = 0.01;

	// Rotate the turret and barrel
	desired[0] += in->analog[0] * turretYawSpeed * mouseSensitivity;
	desired[0] = fpmod(desired[0], 2 * M_PI);
	desired[1] += -in->analog[1] * barrelPitchSpeed * mouseSensitivity;
	desired[1] = rangein(desired[1], barrelPitchMin, barrelPitchMax);
	turrety = approach(turrety, desired[0], dt * turretYawSpeed, 2 * M_PI);
	barrelp = approach(barrelp, desired[1], dt * barrelPitchSpeed, 0);

#if 0
	if(pt->weapon != 2){
		p->turrety = approach(p->turrety, pyr[1] - floor(pyr[1] / 2. / M_PI) * 2. * M_PI, TURRETROTSPEED * dt, 2 * M_PI);
		p->barrelp = rangein(approach(p->barrelp, -pyr[0], TURRETROTSPEED * dt, 2 * M_PI), vft->barrelmin, vft->barrelmax);
	}
	else{
		pyr[1] -= p->turrety;
		p->mountpy[1] = approach(p->mountpy[1], pyr[1] - floor(pyr[1] / 2. / M_PI) * 2. * M_PI, 2 * TURRETROTSPEED * dt, 2 * M_PI);
		p->mountpy[0] = rangein(approach(p->mountpy[0], -pyr[0], 2 * TURRETROTSPEED * dt, 2 * M_PI), vft->barrelmin, vft->barrelmax);
	}
#endif

	if((in->press & in->change) & (PL_TAB | PL_RCLICK)){
		weapon = (weapon + 1) % 4;
	}

	if(in->press & in->change & PL_SPACE){
		double best = 3. * 3.;
		double mini = enemy ? (pos - enemy->pos).slen() : 0.;
		Entity *target = NULL;
		for(WarField::EntityList::iterator it = w->entlist().begin(); it != w->entlist().end(); ++it){
			Entity *t = *it;
			double sdist;
			if(t != this && t != enemy && 0. < t->getHealth() && mini < (sdist = (pos - t->pos).slen()) && sdist < best){
				best = sdist;
				target = t;
			}
		}
		if(target)
			enemy = target;
		else{
			for(WarField::EntityList::iterator it = w->entlist().begin(); it != w->entlist().end(); ++it){
				Entity *t = *it;
				double sdist;
				if(t != this && t != enemy && 0. < t->getHealth() && (sdist = (pos - t->pos).slen()) < best){
					best = sdist;
					target = t;
				}
			}
			enemy = target;
		}
	}

}


#ifdef DEDICATED
void M3Truck::deathEffects(){}
void M3Truck::draw(WarDraw*){}
void M3Truck::drawCockpit(WarDraw *wd){}
#endif


