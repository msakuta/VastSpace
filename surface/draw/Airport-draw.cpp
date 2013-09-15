/** \file
 * \brief Implementation of Airport class graphics.
 */
#include "../Airport.h"
#include "draw/WarDraw.h"
#include "draw/mqoadapt.h"
#include "draw/OpenGLState.h"

void Airport::draw(WarDraw *wd){
	if(!w)
		return;

	/* cull object */
	if(wd->vw->gc->cullFrustum(pos, hitRadius))
		return;
	double pixels = hitRadius * fabs(wd->vw->gc->scale(pos));
	if(pixels < 2)
		return;
	wd->lightdraws++;

	static OpenGLState::weak_ptr<bool> init;
	if(!init){
		model = LoadMQOModel(modPath() << "models/airport.mqo");
		init.create(*openGLState);
	}

	if(model){
		glPushMatrix();
		Mat4d mat;
		transform(mat);
		glMultMatrixd(mat);
		glScaled(modelScale, modelScale, modelScale);
		DrawMQOPose(model, NULL);
		glPopMatrix();
	}

}

void Airport::drawtra(wardraw_t *wd){
	st::drawtra(wd);
	drawNavlights(wd, navlights);
}
