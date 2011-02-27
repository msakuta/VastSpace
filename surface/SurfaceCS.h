#include "CoordSys.h"
#include "WarMap.h"
extern "C"{
#include <clib/gl/gldraw.h>
}

class DrawMapCache;
DrawMapCache *CacheDrawMap(WarMap *);

class SurfaceCS : public CoordSys{
public:
	typedef CoordSys st;
	static ClassRegister<SurfaceCS> classRegister;
	const Static &getStatic()const;
	SurfaceCS();
	SurfaceCS(const char *path, CoordSys *root);
	~SurfaceCS();

	virtual void predraw(const Viewer *);
	virtual void draw(const Viewer *);

	WarMap *getWarMap(){return wm;}
protected:
	WarMap *wm;
	DrawMapCache *dmc;
	int map_checked;
	char *map_top;
};

