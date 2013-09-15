/** \file
 * \brief Implementation of SurfaceBuilding class graphics.
 */
#include "../SurfaceBuilding.h"
#include "draw/WarDraw.h"
#include "draw/mqoadapt.h"
#include "draw/OpenGLState.h"
extern "C"{
#include "clib/gl/gldraw.h"
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

	if(!model){
		model = LoadMQOModel(modelFile);
	}

	if(model){
		glPushMatrix();
		{
			Mat4d mat;
			transform(mat);
			glMultMatrixd(mat);
		}
//		gldTranslate3dv(landOffset);

		glScaled(modelScale, modelScale, modelScale);
		DrawMQOPose(model, NULL);
		glPopMatrix();
	}

}

void SurfaceBuilding::drawtra(wardraw_t *wd){
	st::drawtra(wd);
	drawNavlights(wd, navlights);
}
