/** \file
 * \brief Implementation of RStation class's graphics.
 */
#include "../RStation.h"
#include "Autonomous.h" // draw_healthbar
#include "draw/material.h"
#include "astrodraw.h"
#include "draw/WarDraw.h"
#include "draw/mqoadapt.h"
#include "draw/OpenGLState.h"
#include "glw/PopupMenu.h"
#include "Viewer.h"
#include "Game.h"
#include "sqadapt.h"
extern "C"{
#include <clib/suf/sufdraw.h>
#include <clib/suf/sufbin.h>
#include <clib/gl/gldraw.h>
}

Model *RStation::model = NULL;

void RStation::draw(WarDraw *wd){
	if(!w)
		return;

	/* cull object */
/*	if(scepter_cull(pt, wd))
		return;*/
	wd->lightdraws++;

	draw_healthbar(this, wd, this->health / getMaxHealth(), 3., this->occupytime / 10., this->ru / maxRU);

	static OpenGLState::weak_ptr<bool> init;
	if(init == 0){
		model = LoadMQOModel(modelFile);
		init.create(*openGLState);
	}

	if(model){
		glPushMatrix();
		gldTranslate3dv(this->pos);
		gldScaled(modelScale);
		gldMultQuat(this->rot);
		glScalef(-1, 1, -1);
		DrawMQOPose(model, NULL);
		glPopMatrix();
	}
}

void RStation::drawtra(WarDraw *wd){
	drawNavlights(wd, navlights);
}

void RStation::drawOverlay(WarDraw *){
	// Because a RStation instance may be created in a stellar structure definition file
	// interpretation, a display list for overlay icon could be unable to be compiled
	// at SqInit() due to lack of OpenGL state.  We can defer execution of drawOverlay()
	// until it's actually necessary to get around this problem.
	// We could compile the graphics into a display list in construct on first use scheme,
	// but it's so simple drawing logic that it would make no noticeable difference.
	HSQUIRRELVM v = game->sqvm;
	StackReserver sr(v);
	sq_pushobject(v, overlayProc);
	sq_pushroottable(v);
	sq_call(v, 1, SQFalse, SQFalse);
}
