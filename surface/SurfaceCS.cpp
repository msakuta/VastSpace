#include "SurfaceCS.h"
#include "WarMap.h"
#include "drawmap.h"
extern "C"{
#include <clib/gl/gldraw.h>
}

ClassRegister<SurfaceCS> SurfaceCS::classRegister("Surface");
const CoordSys::Static &SurfaceCS::getStatic()const{
	return classRegister;
}

SurfaceCS::SurfaceCS() : wm(NULL), map_top(NULL){
}

SurfaceCS::SurfaceCS(const char *path, CoordSys *root) : st(path, root), map_top(NULL){
	wm = OpenHGTMap("N36W113.av.zip");
	dmc = CacheDrawMap(wm);
}

SurfaceCS::~SurfaceCS(){
	delete wm;
	delete dmc;
}

/// Reset the flag to instruct the drawmap function to recalculate the pyramid buffer.
void SurfaceCS::predraw(const Viewer *vw){
	map_checked = 0;
}

/// Note that this function is going to be called at least twice a frame.
void SurfaceCS::draw(const Viewer *vw){
	if(wm && vw->zslice < 2){
		glPushMatrix();
		if(vw->zslice == 1){
			gldTranslate3dv(-vw->pos);
		}
		drawmap(wm, vw->pos, 0, vw->viewtime, vw->gc, &map_top, &map_checked, dmc);
		glPopMatrix();
	}
}
