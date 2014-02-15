/** \file
 * \brief Implementation of Airport class.
 */
#include "Airport.h"
#include "SurfaceCS.h"
#include "btadapt.h"
#include "sqadapt.h"
#include "SqInitProcess-ex.h"
#include "cmd.h"

Entity::EntityRegister<Airport> Airport::entityRegister("Airport");

Model *Airport::model = NULL;
double Airport::modelScale = 0.01;
double Airport::hitRadius = 0.1;
double Airport::maxHealthValue;
Vec3d Airport::landOffset = Vec3d(0,0,0);
Vec3d Airport::landingSite = Vec3d(0,0,0);
HitBoxList Airport::hitboxes;
std::vector<Airport::Navlight> Airport::navlights;
HSQOBJECT Airport::sqGetCoverPoints = sq_nullobj();
GLuint Airport::overlayDisp;

void Airport::init(){
	static bool initialized = false;
	if(!initialized){
		SqInit(game->sqvm, _SC(modPath() << "models/airport.nut"),
			SingleDoubleProcess(modelScale, "modelScale") <<=
			SingleDoubleProcess(hitRadius, "hitRadius") <<=
			SingleDoubleProcess(maxHealthValue, "maxhealth", false) <<=
			Vec3dProcess(landOffset, "landOffset") <<=
			Vec3dProcess(landingSite, "landingSite") <<=
			HitboxProcess(hitboxes) <<=
			NavlightsProcess(navlights) <<=
			SqCallbackProcess(sqGetCoverPoints, "getCoverPoints") <<=
			DrawOverlayProcess(overlayDisp));
		initialized = true;
	}
	mass = 1e4;
	race = -1;
}

void Airport::anim(double dt){
	if(WarSpace *ws = *w){
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

bool Airport::command(EntityCommand *com){
	if(GetILSCommand *gic = InterpretCommand<GetILSCommand>(com)){
		gic->pos = this->pos + rot.trans(landingSite);
		Vec3d dir = this->rot.trans(Vec3d(0,0,1));
		gic->heading = atan2(dir[0], dir[2]);
		return true;
	}
	else if(GetCoverPointsCommand *gcc = dynamic_cast<GetCoverPointsCommand*>(com)){
		// Call a Squirrel callback function to obtain the list of CoverPoints.
		// This is not hard-coded because we don't know anything about the airport model and
		// geometry at the time of compilation.
		try{
			HSQUIRRELVM v = game->sqvm;
			StackReserver sr(v);
			sq_pushobject(v, sqGetCoverPoints);
			sq_pushroottable(v);
			Entity::sq_pushobj(v, this);
			if(SQ_FAILED(sq_call(v, 2, SQTrue, SQTrue)))
				return 0;
			SQInteger len = sq_getsize(v, -1);
			if(-1 == len)
				throw SQFError(_SC("getCoverPoints: size could not be acquired"));
			gcc->cpv.resize(len);
			for(int i = 0; i < len; i++){
				sq_pushinteger(v, i); // root obj i
				if(SQ_FAILED(sq_get(v, -2))) // root obj obj[i]
					continue;
				CoverPoint &cp = gcc->cpv[i];

				sq_pushstring(v, _SC("type"), -1); // root obj obj[i] "type"
				if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].type
					const SQChar *s;
					if(SQ_FAILED(sq_getstring(v, -1, &s)))
						throw SQFError(gltestp::dstring("getCoverPoints: [") << i << _SC("].type not convertible to string"));
					if(!scstrcmp(s, _SC("RightEdge")))
						cp.type = cp.RightEdge;
					else if(!scstrcmp(s, _SC("LeftEdge")))
						cp.type = cp.LeftEdge;
					else
						throw SQFError(gltestp::dstring("getCoverPoints: [") << i << _SC("].type not matching pre-defined strings"));
					sq_poptop(v); // root obj obj[i]
				}
				else
					throw SQFError(gltestp::dstring("getCoverPoints: [") << i << _SC("].type not defined"));

				sq_pushstring(v, _SC("pos"), -1); // root obj obj[i] "pos"
				if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].pos
					SQVec3d r;
					r.getValue(v, -1);
					cp.pos = r.value;
					sq_poptop(v); // root obj obj[i]
				}
				else
					throw SQFError(gltestp::dstring("getCoverPoints: [") << i << _SC("].pos not defined"));

				sq_pushstring(v, _SC("rot"), -1); // root obj obj[i] "rot"
				if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].rot
					SQQuatd r;
					r.getValue(v, -1);
					cp.rot = r.value;
					sq_poptop(v); // root obj obj[i]
				}
				else
					throw SQFError(gltestp::dstring("getCoverPoints: [") << i << _SC("].rot not defined"));

				sq_poptop(v); // root obj
			}
			sq_poptop(v); // root
		}
		catch(SQFError &e){
			CmdPrint(gltestp::dstring("Airport::command(GetCoverPointsCommand) Error: ") << e.what());
		}
	}
	else
		return st::command(com);
}

int Airport::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
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

SQInteger Airport::sqGet(HSQUIRRELVM v, const SQChar *name)const{
	if(!scstrcmp(name, _SC("ILSPos"))){
		SQVec3d q(this->pos + rot.trans(landingSite));
		q.push(v);
		return 1;
	}
	else if(!scstrcmp(name, _SC("ILSHeading"))){
		Vec3d dir = this->rot.trans(Vec3d(0,0,1));
		sq_pushfloat(v, atan2(dir[0], dir[2]));
		return 1;
	}
	else
		return st::sqGet(v, name);
}

bool Airport::buildBody(){
	if(hitboxes.size() != 0 && !bbody){
		btCompoundShape *shape = new btCompoundShape();
		for(auto it : hitboxes){
			const Vec3d &sc = it.sc;
			const Quatd &rot = it.rot;
			const Vec3d &pos = it.org;
			btBoxShape *box = new btBoxShape(btvc(sc));
			btTransform trans = btTransform(btqc(rot), btvc(pos));
			shape->addChildShape(trans, box);
		}
		btTransform startTransform;
		startTransform.setIdentity();
		startTransform.setOrigin(btvc(pos));

		// Airport is stationary structure. In fact, it has been SurfaceBuilding.
		btVector3 localInertia(0,0,0);

		// Static structure doesn't need motion state.
		btRigidBody::btRigidBodyConstructionInfo rbInfo(0,nullptr,shape,localInertia);
		bbody = new btRigidBody(rbInfo);
	}
	return true;
}


#ifdef DEDICATED
void Airport::draw(WarDraw *){}
void Airport::drawtra(WarDraw *){}
#endif


IMPLEMENT_COMMAND(GetILSCommand, "GetILS");

GetILSCommand::GetILSCommand(HSQUIRRELVM v, Entity &e){}
