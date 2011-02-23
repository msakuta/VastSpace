#include "CoordSys.h"
#include "WarMap.h"
#include "drawmap.h"

class SurfaceCS : public CoordSys{
public:
	typedef CoordSys st;
	static ClassRegister<SurfaceCS> classRegister;
	SurfaceCS();
	SurfaceCS(const char *path, CoordSys *root);
	~SurfaceCS();

	virtual void draw(const Viewer *);
protected:
	WarMap *wm;
	char *map_top;
};

ClassRegister<SurfaceCS> SurfaceCS::classRegister("Surface");

SurfaceCS::SurfaceCS() : wm(NULL), map_top(NULL){
}

SurfaceCS::SurfaceCS(const char *path, CoordSys *root) : st(path, root), map_top(NULL){
	wm = OpenHGTMap("N36W113.av.zip");
}

SurfaceCS::~SurfaceCS(){
	delete wm;
}


void SurfaceCS::draw(const Viewer *vw){
	int map_checked = 0;
	if(wm)
		drawmap(wm, vw->pos, 0, vw->viewtime, vw->gc, &map_top, &map_checked);
}
