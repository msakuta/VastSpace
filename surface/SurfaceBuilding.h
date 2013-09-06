/** \file
 * \brief Definition of SurfaceBuilding class.
 */
#ifndef SURFACE_SURFACEBUILDING_H
#define SURFACE_SURFACEBUILDING_H
#include "ModelEntity.h"
#include "Model-forward.h"
#include <btBulletDynamicsCommon.h>


class SurfaceBuilding : public ModelEntity{
public:
	typedef ModelEntity st;
	static const unsigned classid;
	static EntityRegister<SurfaceBuilding> entityRegister;

	SurfaceBuilding(Game *game) : st(game), model(NULL), modelScale(0.01), hitRadius(0.1), landOffset(0,0,0), shape(NULL){}
	SurfaceBuilding(WarField *w) : st(w), model(NULL), modelScale(0.01), hitRadius(0.1), landOffset(0,0,0), shape(NULL){race = -1;}
	const char *classname()const override{return "SurfaceBuilding";}
	void anim(double)override;
	void draw(WarDraw *)override;
	int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal)override;
	double getHitRadius()const override{return hitRadius;}
	bool isTargettable()const override{return true;}
	bool isSelectable()const override{return false;}

	static gltestp::dstring modPath(){return _SC("surface/");}

	static SQInteger sqf_setHitBoxes(HSQUIRRELVM);

protected:
	SQInteger sqGet(HSQUIRRELVM v, const SQChar *name)const override;
	SQInteger sqSet(HSQUIRRELVM v, const SQChar *name)override;

	bool buildBody();
	void SurfaceBuilding::addRigidBody(WarSpace *ws);

	gltestp::dstring modelFile; ///< The model file name in relative path to the project root.
	Model *model;
	double modelScale; ///< Model scale is different from model to model.
	double hitRadius; ///< Hit radius (extent sphere radius) is different from model to model.
	Vec3d landOffset;
	HitBoxList hitboxes;
	btCompoundShape *shape;
};

#endif
