/** \file
 * \brief Implementation of SurfaceBuilding class.
 */
#include "SurfaceBuilding.h"
#include "SurfaceCS.h"
#include "sqadapt.h"
#include "btadapt.h"

template<> void Entity::EntityRegister<SurfaceBuilding>::sq_defineInt(HSQUIRRELVM v){
	register_closure(v, _SC("setHitBoxes"), SurfaceBuilding::sqf_setHitBoxes, 2);
	register_closure(v, _SC("setNavLights"), SurfaceBuilding::sqf_setNavLights, 2);
}

Entity::EntityRegister<SurfaceBuilding> SurfaceBuilding::entityRegister("SurfaceBuilding");


void SurfaceBuilding::anim(double dt){
	if(WarSpace *ws = *w){
		if(!bbody && hitboxes.size() != 0){
			addRigidBody(ws);
		}

		// If the CoordSys is a SurfaceCS, we can expect ground in negative y direction.
		// dynamic_cast should be preferred.
		if(&w->cs->getStatic() == &SurfaceCS::classRegister){
			SurfaceCS *s = static_cast<SurfaceCS*>(w->cs);
			Vec3d normal;
			double height = s->getHeight(pos[0], pos[2], &normal);
			{
				Vec3d dest = Vec3d(pos[0], height, pos[2]) + landOffset;
				setPosition(&dest, NULL, NULL);
			}
		}
	}

}

int SurfaceBuilding::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	double best = dt;
	int reti = 0;
	double retf;
	for(int n = 0; n < hitboxes.size(); n++){
		hitbox &hb = hitboxes[n];
		Vec3d org = this->rot.trans(hb.org) + this->pos;
		Quatd rot = this->rot * hb.rot;
		Vec3d sc;
		for(int i = 0; i < 3; i++)
			sc[i] = hb.sc[i] + rad;
		if((jHitBox(org, sc, rot, src, dir, 0., best, &retf, retp, retn)) && (retf < best)){
			best = retf;
			if(ret) *ret = retf;
			reti = n + 1;
		}
	}
	return reti;
}

bool SurfaceBuilding::buildBody(){
	if(hitboxes.size() != 0 && !bbody){
		if(!shape){
			shape = new btCompoundShape();
			for(auto it : hitboxes){
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
		bool isDynamic = false;

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			shape->calculateLocalInertia(mass,localInertia);

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,shape,localInertia);
//		rbInfo.m_linearDamping = .5;
//		rbInfo.m_angularDamping = .5;
		bbody = new btRigidBody(rbInfo);

//		bbody->setSleepingThresholds(.0001, .0001);
	}
	return true;
}

void SurfaceBuilding::addRigidBody(WarSpace *ws){
	if(ws && ws->bdw){
		buildBody();
		//add the body to the dynamics world
		if(bbody)
			ws->bdw->addRigidBody(bbody, bbodyGroup(), bbodyMask());
	}
}


SQInteger SurfaceBuilding::sqGet(HSQUIRRELVM v, const SQChar *name)const{
	if(!strcmp(name, _SC("modelFile"))){
		sq_pushstring(v, this->modelFile, -1);
		return 1;
	}
	else if(!strcmp(name, _SC("modelScale"))){
		sq_pushfloat(v, this->modelScale);
		return 1;
	}
	else if(!strcmp(name, _SC("hitRadius"))){
		sq_pushfloat(v, this->hitRadius);
		return 1;
	}
	else if(!strcmp(name, _SC("landOffset"))){
		SQVec3d r;
		r.value = landOffset;
		r.push(v);
		return 1;
	}
	else
		st::sqGet(v, name);
}

SQInteger SurfaceBuilding::sqSet(HSQUIRRELVM v, const SQChar *name){
	if(!strcmp(name, _SC("modelFile"))){
		const SQChar *str;
		if(SQ_SUCCEEDED(sq_getstring(v, 3, &str))){
			modelFile = str;
			return 0;
		}
		else
			return sq_throwerror(v, _SC("Type not compatible to string for modelFile"));
	}
	else if(!strcmp(name, _SC("modelScale"))){
		SQFloat f;
		if(SQ_SUCCEEDED(sq_getfloat(v, 3, &f))){
			modelScale = f;
			return 0;
		}
		else
			return sq_throwerror(v, _SC("Type not compatible to float for modelScale"));
	}
	else if(!strcmp(name, _SC("hitRadius"))){
		SQFloat f;
		if(SQ_SUCCEEDED(sq_getfloat(v, 3, &f))){
			hitRadius = f;
			return 0;
		}
		else
			return sq_throwerror(v, _SC("Type not compatible to float for hitRadius"));
	}
	else if(!strcmp(name, _SC("landOffset"))){
		SQVec3d r;
		r.getValue(v, 3);
		landOffset = r.value;
	}
	else
		st::sqSet(v, name);
}

SQInteger SurfaceBuilding::sqf_setHitBoxes(HSQUIRRELVM v){
	struct HitboxSqInitEval : SqInitProcess::SqInitEval{
		HSQUIRRELVM v;
		HSQOBJECT o;
		HitboxSqInitEval(HSQUIRRELVM v, HSQOBJECT o) : v(v), o(o){}
		SQRESULT call()override{
			// Create a temporary table that contains only the hitbox as the member.
			sq_newtable(v);
			sq_pushstring(v, _SC("hitbox"), -1);
			sq_pushobject(v, o);
			sq_newslot(v, -3, SQFalse);
			return SQ_OK;
		}
		gltestp::dstring description()const override{
			return "HitboxSqInitEval";
		}
	};
	Entity *p = Entity::sq_refobj(v, 1);
	if(!p)
		return sq_throwerror(v, _SC("setHitBoxes() requires SurfaceBuilding"));
	HSQOBJECT o;
	if(SQ_SUCCEEDED(sq_getstackobj(v, 2, &o))){
		SqInitProcess::SqInit(v, HitboxSqInitEval(v, o), HitboxProcess(static_cast<SurfaceBuilding*>(p)->hitboxes));
	}
}

SQInteger SurfaceBuilding::sqf_setNavLights(HSQUIRRELVM v){
	struct NavLightsSqInitEval : SqInitProcess::SqInitEval{
		HSQUIRRELVM v;
		HSQOBJECT o;
		NavLightsSqInitEval(HSQUIRRELVM v, HSQOBJECT o) : v(v), o(o){}
		SQRESULT call()override{
			// Create a temporary table that contains only the navlights as the member.
			sq_newtable(v);
			sq_pushstring(v, _SC("navlights"), -1);
			sq_pushobject(v, o);
			sq_newslot(v, -3, SQFalse);
			return SQ_OK;
		}
		gltestp::dstring description()const override{
			return "NavLightsSqInitEval";
		}
	};
	Entity *p = Entity::sq_refobj(v, 1);
	if(!p)
		return sq_throwerror(v, _SC("setHitBoxes() requires SurfaceBuilding"));
	HSQOBJECT o;
	if(SQ_SUCCEEDED(sq_getstackobj(v, 2, &o))){
		SqInitProcess::SqInit(v, NavLightsSqInitEval(v, o), NavlightsProcess(static_cast<SurfaceBuilding*>(p)->navlights));
	}
}

#ifdef DEDICATED
void SurfaceBuilding::draw(WarDraw *){}
void SurfaceBuilding::drawtra(WarDraw *){}
#endif
