/** \file
 * \brief Implementation of SurfaceBuilding class.
 */
#include "ModelEntity.h"
#include "draw/WarDraw.h"
#include "draw/mqoadapt.h"
#include "draw/OpenGLState.h"
#include "SurfaceCS.h"


class SurfaceBuilding : public ModelEntity{
public:
	typedef ModelEntity st;
	static const unsigned classid;
	static EntityRegister<SurfaceBuilding> entityRegister;

	SurfaceBuilding(Game *game) : st(game), model(NULL), modelScale(0.01), hitRadius(0.1){}
	SurfaceBuilding(WarField *w) : st(w), model(NULL), modelScale(0.01), hitRadius(0.1){race = -1;}
	void anim(double)override;
	void draw(WarDraw *)override;
	double getHitRadius()const override{return hitRadius;}
	bool isTargettable()const override{return true;}
	bool isSelectable()const override{return false;}

	static gltestp::dstring modPath(){return _SC("surface/");}
	static double getLandOffset(){return 0.;}

protected:
	SQInteger sqGet(HSQUIRRELVM v, const SQChar *name)const override;
	SQInteger sqSet(HSQUIRRELVM v, const SQChar *name)override;

	gltestp::dstring modelFile; ///< The model file name in relative path to the project root.
	Model *model;
	double modelScale; ///< Model scale is different from model to model.
	double hitRadius; ///< Hit radius (extent sphere radius) is different from model to model.
};

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

void SurfaceBuilding::draw(WarDraw *wd){
	if(!w || !modelFile.len())
		return;

	/* cull object */
	if(wd->vw->gc->cullFrustum(pos, hitRadius))
		return;
	double pixels = hitRadius * fabs(wd->vw->gc->scale(pos));
	if(pixels < 2)
		return;
	wd->lightdraws++;

	static Model *model = NULL;

	static OpenGLState::weak_ptr<bool> init;
	if(!init){
		delete model;
		model = LoadMQOModel(modelFile);
		init.create(*openGLState);
	};

	if(model){
		glPushMatrix();
		{
			Mat4d mat;
			transform(mat);
			glMultMatrixd(mat);
		}

		glScaled(modelScale, modelScale, modelScale);
		DrawMQOPose(model, NULL);
		glPopMatrix();
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
