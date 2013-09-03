/** \file
 * \brief Implementation of SurfaceBuilding class graphics.
 */
#include "../SurfaceBuilding.h"
#include "draw/WarDraw.h"
#include "draw/mqoadapt.h"
#include "draw/OpenGLState.h"

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

	static Model *model = NULL;

	static OpenGLState::weak_ptr<bool> init;
	if(!init){
		delete model;
		model = LoadMQOModel(modelFile);
		init.create(*openGLState);
	};

	if(model){
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
