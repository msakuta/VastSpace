/** \file
 * \brief Implementation of SurfaceBuilding class.
 */
#include "SurfaceBuilding.h"
#include "SurfaceCS.h"

Entity::EntityRegister<SurfaceBuilding> SurfaceBuilding::entityRegister("SurfaceBuilding");


void SurfaceBuilding::anim(double dt){
	if(WarSpace *ws = *w){
		static const btVector3 offset(0, getLandOffset(), 0);

		// If the CoordSys is a SurfaceCS, we can expect ground in negative y direction.
		// dynamic_cast should be preferred.
		if(&w->cs->getStatic() == &SurfaceCS::classRegister){
			SurfaceCS *s = static_cast<SurfaceCS*>(w->cs);
			Vec3d normal;
			double height = s->getHeight(pos[0], pos[2], &normal);
			{
				Vec3d dest(pos[0], height + offset[1], pos[2]);
				setPosition(&dest, NULL, NULL);
			}
		}
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
	else
		st::sqSet(v, name);
}

#ifdef DEDICATED
void SurfaceBuilding::draw(WarDraw *){}
#endif
