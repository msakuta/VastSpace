/** \file
 * \brief Implementation of Apache's graphics.
 */
#define NOMINMAX
#include "../Apache.h"
#include "arms.h"
#include "draw/WarDraw.h"
#include "draw/mqoadapt.h"
#include "draw/OpenGLState.h"
#include "draw/effects.h"
extern "C"{
#include <clib/gl/gldraw.h>
}


void Apache::draw(WarDraw *wd){
	/* cull object */
	if(wd->vw->gc->cullFrustum(this->pos, getHitRadius()))
		return;
	double pixels = getHitRadius() * fabs(wd->vw->gc->scale(this->pos));
	if(pixels < 2)
		return;
	wd->lightdraws++;

//	static Motion *rotorxMotion = NULL;
//	static Motion *rotorzMotion = NULL;
	static Motion *rotorMotion = NULL;
	static Motion *tailRotorMotion = NULL;
	static Motion *featherMotions[4] = {NULL};

	static OpenGLState::weak_ptr<bool> init;
	if(!init) do{
		FILE *fp;
		model = LoadMQOModel(modPath() << "models/apache.mqo");
//		rotorxMotion = LoadMotion(modPath() << "models/apache-rotorx.mot");
//		rotorzMotion = LoadMotion(modPath() << "models/apache-rotorz.mot");
		rotorMotion = LoadMotion(modPath() << "models/apache-rotor.mot");
		tailRotorMotion = LoadMotion(modPath() << "models/apache-tailrotor.mot");

		auto findBone = [](Model *model, const char *name){
			auto inner = [&](Bone *bone) -> Bone*{
				for(Bone *b = bone->children; b != NULL; b = b->nextSibling){
					if(b->name == name)
						return b;
				}
				return NULL;
			};
			return inner(model->bones[0]);
		};

		for(int i = 0; i < 4; i++){
			gltestp::dstring name = gltestp::dstring() << "apache_blade" << (i + 1);
//			Bone *b = findBone(model, name);
//			if(b){
				featherMotions[i] = new Motion;
				for(int j = -1; j <= 1; j++){
					Motion::keyframe &kf = featherMotions[i]->addKeyframe(10);
					MotionNode *node = kf.addNode(name);
					node->rot = Quatd::rotation(j * M_PI * 0.5, sin(i * M_PI * 2 / 4), 0, cos(i * M_PI * 2 / 4));
				}
//			}
		}
		init.create(*openGLState);
	} while(0);

	if(!model)
		return;

	MotionPose mp[6 + numof(featherMotions)];
//	rotorxMotion->interpolate(mp[0], rotoraxis[0] / (M_PI / 2.) * 10. + 10.);
//	rotorzMotion->interpolate(mp[1], rotoraxis[1] / (M_PI / 2.) * 10. + 10.);
//	mp[0].next = &mp[1];
	static const double maxPitchFactor = 3.;
	double mrotor = fmod(rotor, M_PI / 2.);
	double rotorPhase = mrotor / (M_PI / 2.) * 10.;
	rotorMotion->interpolate(mp[2], rotorPhase);
//	mp[1].next = &mp[2];
	tailRotorMotion->interpolate(mp[3], fmod(tailrotor, M_PI) / (M_PI) * 20.);
	mp[2].next = &mp[3];
	gunMotion(&mp[4]);
	mp[3].next = &mp[4];
	
	// The gyroscopic precession make lift torque appear 90 degrees ahead of swash plate phase with highest blade pitch.
	// http://en.wikipedia.org/wiki/Gyroscopic_precession#Torque-induced
	double swash = atan2(rotoraxis[1], rotoraxis[0]) + M_PI;
	double pitch = sqrt(rotoraxis[1] * rotoraxis[1] + rotoraxis[0] * rotoraxis[0]);
	for(int i = 0; i < numof(featherMotions); i++){
		featherMotions[i]->interpolate(mp[5 + i], 10 + maxPitchFactor * (feather + pitch * sin(mrotor + swash + i * M_PI * 2 / numof(featherMotions))));
		mp[5 + i - 1].next = &mp[5 + i];
	}

	glPushMatrix();

	Mat4d mat;
	transform(mat);
	glMultMatrixd(mat);

	glScaled(-modelScale, modelScale, -modelScale);

	DrawMQOPose(model, &mp[2]);

	glPopMatrix();

}

void Apache::drawtra(WarDraw *wd){
	Mat4d mat;
	transform(mat);
	if(muzzle){
		muzzle = 0;
		if(!model)
			return;
		static const Vec3d pvo(.0, -1.50, -3.20);
		Mat4d rmat = this->rot.tomat4().rotx(gun[0]).roty(gun[1]);
		MotionPose mp[2];
		gunMotion(mp);
		Vec3d pos;
		Quatd rot;
		if(model->getBonePos("apache_muzzle", mp[0], &pos, &rot)){
			mat.scalein(-modelScale, modelScale, -modelScale);
			drawmuzzleflash4(mat.vp3(pos), (this->rot.rotate(M_PI, 0, 1, 0) * rot).rotate(M_PI, 0, 1, 0).tomat4(),
				0.0025, mat4_u, &w->rs, wd->vw->pos);
		}
	}
}

void Apache::drawHUD(WarDraw *wd){
	st::drawHUD(wd);

	glPushMatrix();
	glLoadIdentity();

	// Show target marker if locked on
	drawTargetMarker(wd);

	{
		GLint vp[4];
		int w, h, m, mi, i;
		double left, bottom;
		int k, ind[7] = {0, 1}, nind = 2;

		glGetIntegerv(GL_VIEWPORT, vp);
		w = vp[2], h = vp[3];
		m = w < h ? h : w;
		mi = w < h ? w : h;
		left = -(double)w / m;
		bottom = -(double)h / m;

		double velo = this->velo.len();
//		w->orientation(wf, &ort, pt->pos);
		Mat3d ort = mat3d_u();
		glRasterPos3d(left + 200. / m, -bottom - 100. / m, -1);
		gldprintf("%lg km/s", velo);
		glRasterPos3d(left + 200. / m, -bottom - 120. / m, -1);
		gldprintf("%lg kt", 1944. * velo);
		glRasterPos3d(left + 200. / m, -bottom - 140. / m, -1);
		gldprintf("climb %lg km/s", this->velo.sp(ort.vec3(1)));
		if(brake){
			glRasterPos3d(left + 200. / m, -bottom - 200. / m, -1);
			gldprintf("BRK");
		}

		glScaled(1./*(double)mi / w*/, 1./*(double)mi / h*/, (double)m / mi);

		/* throttle */
		glBegin(GL_LINE_LOOP);
		glVertex3d(-.8, -0.7, -1.);
		glVertex3d(-.8, -0.8, -1.);
		glVertex3d(-.78, -0.8, -1.);
		glVertex3d(-.78, -0.7, -1.);
		glEnd();
		glBegin(GL_LINES);
		glVertex3d(-.775, -0.8 + throttle * .1, -1.);
		glVertex3d(-.805, -0.8 + throttle * .1, -1.);
		/* main feather */
		glVertex3d(-.76, -0.8 + feather * .1, -1.);
		glVertex3d(-.75, -0.8 + feather * .1, -1.);
		glEnd();
		glBegin(GL_QUADS);
		glVertex3d(-.8, -0.8 + throttle * feather * .1, -1.);
		glVertex3d(-.8, -0.8, -1.);
		glVertex3d(-.78, -0.8, -1.);
		glVertex3d(-.78, -0.8 + throttle * feather * .1, -1.);
		glEnd();

		glTranslated(-.5, -.7, -1.);
		glScaled(.1, .1, 1.);

	}

	glPopMatrix();
}

void Apache::drawCockpit(WarDraw *wd){
	const Vec3d &seat = cockpitOfs;
	Player *player = game->player;
	if(!isCockpitView(player->getChaseCamera()) || player->mover != player->cockpitview || player->mover != player->nextmover)
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

	glPushMatrix();

	gldTranslaten3dv(seat);
	glScaled(-modelScale, modelScale, -modelScale);
	DrawMQOPose(cockpitModel, NULL);

	glPopMatrix();

	Quatd rq = Quatd::rotation(-gun[0], vec3_100).rotate(gun[1], vec3_010);
	drawCockpitHUD(hudPos, hudSize, seat, rq.trans(Vec3d(0,0,1)));

	glPopMatrix();

	glPopAttrib();

}

void Apache::drawOverlay(WarDraw *){
	glCallList(overlayDisp);
}

void Apache::gunMotion(MotionPose *mp){
	static Motion *gunYawMotion = NULL;
	static Motion *gunPitchMotion = NULL;

	static OpenGLState::weak_ptr<bool> init;
	if(!init) do{
		gunYawMotion = LoadMotion(modPath() << "models/apache-gunyaw.mot");
		gunPitchMotion = LoadMotion(modPath() << "models/apache-gunpitch.mot");
		init.create(*openGLState);
	} while(0);

	gunYawMotion->interpolate(mp[0], gun[1] / (M_PI) * 20. + 10.);
	gunPitchMotion->interpolate(mp[1], -gun[0] / (M_PI) * 20. + 10.);
	mp[0].next = &mp[1];
}
