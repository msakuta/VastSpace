/** \file
 * \brief Definition of Airport class.
 */
#ifndef SURFACE_AIRPORT_H
#define SURFACE_AIRPORT_H
#include "ModelEntity.h"
#include "Model-forward.h"
#include <btBulletDynamicsCommon.h>


class Airport : public ModelEntity{
public:
	typedef ModelEntity st;
	static const unsigned classid;
	static EntityRegister<Airport> entityRegister;

	Airport(Game *game) : st(game){}
	Airport(WarField *w) : st(w){init();}
	const char *classname()const override{return "Airport";}
	void init();
	void anim(double)override;
	void draw(WarDraw *)override;
	void drawtra(WarDraw *)override;
	int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal)override;
	double getHitRadius()const override{return hitRadius;}
	double getMaxHealth()const override{return maxHealthValue;}
	bool isTargettable()const override{return true;}
	bool isSelectable()const override{return false;}

	static gltestp::dstring modPath(){return _SC("surface/");}

protected:
	bool buildBody()override;

	static Model *model;
	static double modelScale;
	static double hitRadius;
	static double maxHealthValue;
	static Vec3d landOffset;
	static HitBoxList hitboxes;
	static std::vector<Navlight> navlights;
	static GLuint overlayDisp;
};

#endif