/** \file
 * \brief Implementation of TorusStation class's graphics related routines.
 */
#include "../TorusStation.h"
#include "astrodraw.h"
#include "CoordSys.h"
#include "Universe.h"
#include "Player.h"
#include "war.h"
#include "glsl.h"
#include "glstack.h"
#include "draw/material.h"
#include "cmd.h"
#include "btadapt.h"
#include "judge.h"
#include "bitmap.h"
#include "Game.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/ShadowMap.h"
#include "draw/ShaderBind.h"
#include "glw/popup.h"
#include "serial_util.h"
#include "draw/mqoadapt.h"
extern "C"{
#include <clib/c.h>
#include <clib/gl/multitex.h>
#include <clib/amat3.h>
#include <clib/suf/suf.h>
#include <clib/suf/sufbin.h>
#include <clib/gl/gldraw.h>
#include <clib/gl/cull.h>
#include <clib/lzw/lzw.h>
#include <clib/timemeas.h>
}
#include <gl/glu.h>
#include <gl/glext.h>
#include <assert.h>
#include "BulletCollision/NarrowPhaseCollision/btGjkConvexCast.h"

using namespace gltestp;



Model *TorusStation::model = NULL;


void TorusStation::predraw(const Viewer *vw){
	st::predraw(vw);
}



void TorusStation::draw(const Viewer *vw){
#if 0
	if(1 < vw->zslice) // No way we can draw it without z buffering.
		return;
	if(vw->shadowmap && vw->shadowmap->isDrawingShadow())
		return;

	bool farmap = !!vw->zslice;
	GLcull *gc2 = vw->gclist[0];

	if(vw->gc->cullFrustum(vw->cs->tocs(pos, parent), 50.))
		return;

	// If any part of the colony has chance to go beyond far clipping plane of z slice of 0,
	// it's enough to decide cullLevel to 1.
	if(gc2->cullFar(vw->cs->tocs(pos, parent), -40.))
		cullLevel = 1;
	else
		cullLevel = 0;
#else
	st::draw(vw);
#endif
}

void TorusStation::drawtra(const Viewer *vw){
#if 0
	GLcull *gc = vw->gc, *gc2 = vw->gclist[0];
	double x0, srad;
	double pixels;

/*	for(csint = children; csint; csint = csint->next) if(csint->vft == &g_spacecolony_s){
		break;
	}
	if(!csint || !csint->w)
		return;*/
	Vec3d pos = vw->cs->tocs(avec3_000, this);
	if(gc->cullFrustum(pos, 50.))
		return;
	pixels = fabs(gc->scale(pos)) * 32.;
	if(pixels < 1)
		return;
#endif
}






void TorusStationEntity::drawOverlay(wardraw_t *){
	glScaled(10, 10, 1);
	glBegin(GL_LINE_LOOP);
	glVertex2d(-.10,  .00);
	glVertex2d(-.09, -.02);
	glVertex2d( .09, -.02);
	glVertex2d( .10,  .00);
	glVertex2d( .09,  .02);
	glVertex2d(-.09,  .02);
	glEnd();
	glBegin(GL_LINES);
	glVertex2d( .09,  .02);
	glVertex2d(-.09,  .10);
	glVertex2d( .09, -.02);
	glVertex2d(-.09, -.10);
	glEnd();
}

Entity::Props TorusStationEntity::props()const{
	Props ret = st::props();
	ret.push_back(gltestp::dstring("Gases: ") << astro->gases);
	ret.push_back(gltestp::dstring("Solids: ") << astro->solids);
	ret.push_back(gltestp::dstring("Population: ") << astro->people);
	return ret;		
}


Model *TorusStationEntity::loadModel(){
	static OpenGLState::weak_ptr<bool> init;

	if(!init){
		TorusStation::model = LoadMQOModel(modPath() << "models/TorusStation.mqo");
		init.create(*openGLState);
	}

	return TorusStation::model;
}

Model *TorusStationEntity::loadHubModel(){
	static OpenGLState::weak_ptr<bool> init;
	static Model *model = NULL;

	if(!init){
		model = LoadMQOModel(modPath() << "models/TorusStation_Hub.mqo");
		init.create(*openGLState);
	}

	return model;
}

// Docking bays
void TorusStationEntity::draw(WarDraw *wd){
#if 1
	{
		Model *model = loadModel();
		static const double normal[3] = {0., 1., 0.};
		static const double dscale = .0001;
		static const GLdouble rotaxis[16] = {
			-1,0,0,0,
			0,0,-1,0,
			0,-1,0,0,
			0,0,0,1,
		};

		glPushMatrix();
		gldTranslate3dv(pos);
		gldMultQuat(rot);

//		GLattrib gla(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);

		glPushMatrix();
		glTranslated(0, 0., 0.);
		gldScaled(dscale);
		glRotatef(90, 0, 1, 0);
		DrawMQOPose(loadHubModel(), NULL);
		glPopMatrix();

		for(int i = 0; i < 8; i++){
			glPushMatrix();
			glRotatef(i * 360 / 8, 0, 0, 1);
			glTranslated(0, -0.12, 0.);
			gldScaled(dscale);
			glRotatef(90, 0, 1, 0);
			DrawMQOPose(model, NULL);
			glPopMatrix();
		}

		glPopMatrix();

		glBindTexture(GL_TEXTURE_2D, 0);
	}
#endif
}


int TorusStationEntity::popupMenu(PopupMenu &list){
	int ret = st::popupMenu(list);
	list.append("Dock Window", 'd', "dockmenu");
	return ret;
}

