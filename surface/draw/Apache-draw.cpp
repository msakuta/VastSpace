/** \file
 * \brief Implementation of Apache's graphics.
 */
#include "../Apache.h"
#include "draw/WarDraw.h"
#include "draw/mqoadapt.h"
#include "draw/OpenGLState.h"
extern "C"{
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/gl/gldraw.h>
}


void Apache::draw(WarDraw *wd){
	/* cull object */
	if(wd->vw->gc->cullFrustum(this->pos, .03))
		return;
	double pixels = .015 * fabs(wd->vw->gc->scale(this->pos));
	if(pixels < 2)
		return;
	wd->lightdraws++;

	static Model *model = NULL;
	static Motion *rotorxMotion = NULL;
	static Motion *rotorzMotion = NULL;
	static Motion *rotorMotion = NULL;

	static OpenGLState::weak_ptr<bool> init;
	if(!init) do{
		FILE *fp;
		model = LoadMQOModel(modPath() << "models/apache.mqo");
		rotorxMotion = LoadMotion(modPath() << "models/apache-rotorx.mot");
		rotorzMotion = LoadMotion(modPath() << "models/apache-rotorz.mot");
		rotorMotion = LoadMotion(modPath() << "models/apache-rotor.mot");
		init.create(*openGLState);
	} while(0);

	if(!model)
		return;

	MotionPose mp[3];
	rotorxMotion->interpolate(mp[0], rotoraxis[0] / (M_PI / 2.) * 10. + 10.);
	rotorzMotion->interpolate(mp[1], rotoraxis[1] / (M_PI / 2.) * 10. + 10.);
	mp[0].next = &mp[1];
	rotorMotion->interpolate(mp[2], fmod(rotor, M_PI / 2.) / (M_PI / 2.) * 10.);
	mp[1].next = &mp[2];

	glPushMatrix();

	Mat4d mat;
	transform(mat);
	glMultMatrixd(mat);

	glScaled(-modelScale, modelScale, -modelScale);

	DrawMQOPose(model, mp);

	glPopMatrix();

}

void Apache::drawCockpit(WarDraw *wd){
	const double modelScale = .00001 /*wd->pgc->znear / wd->pgc->zfar*/;
	const Vec3d &seat = cockpitOfs;
	Player *player = game->player;
	if(player->getChaseCamera() != 0 || player->mover != player->cockpitview || player->mover != player->nextmover)
		return;

	static Model *cockpitModel = NULL;

	static OpenGLState::weak_ptr<bool> init;
	if(!init){
		cockpitModel = LoadMQOModel(modPath() << "models/apache_int.mqo");
		init.create(*openGLState);
	}

	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glClearDepth(1.);
	glClear(GL_DEPTH_BUFFER_BIT);

	glPushMatrix();

	glLoadMatrixd(wd->vw->rot);
	gldMultQuat(this->rot);
	gldTranslaten3dv(seat);

	glScaled(-modelScale, modelScale, -modelScale);
	DrawMQOPose(cockpitModel, NULL);

	glPopMatrix();

	glPopAttrib();

}
