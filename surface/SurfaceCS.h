#include "CoordSys.h"
#include "WarMap.h"
#include "TIN.h"
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
protected:
	WarMap *wm;
	DrawMapCache *dmc;
	int map_checked;
	char *map_top;
	btCollisionShape *mapshape;
	btRigidBody *bbody;
	gltestp::dstring tinFileName;
	TIN *tin;
	bool initialized;

	void init();
	friend class SurfaceWar;
};

