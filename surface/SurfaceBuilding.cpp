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

	SurfaceBuilding(Game *game) : st(game){}
	SurfaceBuilding(WarField *w) : st(w){race = -1;}
	void anim(double)override;
	void draw(WarDraw *)override;
	double getHitRadius()const override{return 0.05;}
	bool isTargettable()const override{return true;}
	bool isSelectable()const override{return false;}

	static gltestp::dstring modPath(){return _SC("surface/");}
	static double getLandOffset(){return 0.;}
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
	if(!w)
		return;

	/* cull object */
	if(wd->vw->gc->cullFrustum(pos, 0.2))
		return;
	double pixels = 0.2 * fabs(wd->vw->gc->scale(pos));
	if(pixels < 2)
		return;
	wd->lightdraws++;

	static Model *model = NULL;

	static OpenGLState::weak_ptr<bool> init;
	if(!init){
		model = LoadMQOModel(modPath() << _SC("models/bigsight.mqo"));
		init.create(*openGLState);
	};

	if(model){
		static const double modelScale = 0.001;

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
