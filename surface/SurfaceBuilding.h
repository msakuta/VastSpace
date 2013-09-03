/** \file
 * \brief Definition of SurfaceBuilding class.
 */
#ifndef SURFACE_SURFACEBUILDING_H
#define SURFACE_SURFACEBUILDING_H
#include "ModelEntity.h"
#include "Model-forward.h"


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

#endif
