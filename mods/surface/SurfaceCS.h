/** \file
 * \brief Definition of SurfaceCS class.
 */
#ifndef SURFACECS_H
#define SURFACECS_H
#include "CoordSys.h"
#include "WarMap.h"
#include "TIN.h"
#include "EntityCommand.h"
#include "msg/GetCoverPointsMessage.h"
#include "RoundAstrobj.h"
extern "C"{
#include <clib/gl/gldraw.h>
}
#include <btBulletDynamicsCommon.h>

class DrawMapCache;
DrawMapCache *CacheDrawMap(WarMap *);

class SurfaceCS : public CoordSys{
public:
	typedef CoordSys st;
	static ClassRegister<SurfaceCS> classRegister;
	const Static &getStatic()const;
	SurfaceCS(Game *game);
	SurfaceCS(const char *path, CoordSys *root);
	~SurfaceCS();

	virtual void anim(double dt);
	virtual void predraw(const Viewer *);
	virtual void draw(const Viewer *);
	bool readFile(StellarContext &sc, int argc, const char *argv[])override;
	bool readFileEnd(StellarContext &)override;

	WarMap *getWarMap(){return wm;}
	double getHeight(double x, double z, Vec3d *normal = NULL)const;
	bool traceHit(const Vec3d &start, const Vec3d &dir, double rad, double dt,
		double *ret = NULL, Vec3d *retp = NULL, Vec3d *retnormal = NULL)const;

	RoundAstrobj *getCelBody(){return cbody;}
protected:
	WarMap *wm;
	DrawMapCache *dmc;
	int map_checked;
	char *map_top;
	btCollisionShape *mapshape;
	btRigidBody *bbody;
	gltestp::dstring tinFileName;
	TIN *tin;
	int tinResample;
	bool initialized;
	RoundAstrobj *cbody;

	void init();
	friend class SurfaceWar;
};

struct GetCoverPointsCommand : EntityCommand{
	COMMAND_BASIC_MEMBERS(GetCoverPointsCommand, EntityCommand);
	GetCoverPointsCommand(){}
	GetCoverPointsCommand(HSQUIRRELVM v, Entity &e);
	CoverPointVector cpv;
};

#endif
