/** \file
 * \brief Implementation of Shipyard classe's drawing methods.
 */
#include "Shipyard.h"
#include "judge.h"
#include "draw/material.h"
#include "../Sceptor.h"
#include "draw/WarDraw.h"
#include "draw/mqoadapt.h"
#include "draw/OpenGLState.h"
#include "draw/effects.h"
#include "glw/PopupMenu.h"
#include "sqadapt.h"
#include "Game.h"
extern "C"{
#include <clib/gl/gldraw.h>
}
#include <cpplib/CRC32.h>

#define texture(e) glMatrixMode(GL_TEXTURE); e; glMatrixMode(GL_MODELVIEW);


int Shipyard::popupMenu(PopupMenu &list){
	int ret = st::popupMenu(list);
	list.append("Build Window", 'b', "buildmenu");
	list.append("Dock Window", 'd', "dockmenu");
	return ret;
}

Motion *Shipyard::motions[2];

void Shipyard::draw(wardraw_t *wd){
	Shipyard *const p = this;
	static OpenGLState::weak_ptr<bool> init;
	static suftex_t *pst;
	static suf_t *sufbase = NULL;
	if(!Entity::w)
		return;

	/* cull object */
/*	if(beamer_cull(this, wd))
		return;*/
//	wd->lightdraws++;

	draw_healthbar(this, wd, health / getMaxHealth(), getHitRadius(), -1., capacitor / maxenergy());

	if(wd->vw->gc->cullFrustum(pos, getHitRadius()))
		return;
#if 1
	if(!init) do{
		motions[0] = LoadMotion("models/shipyard_door.mot");
		motions[1] = LoadMotion("models/shipyard_door2.mot");
		init.create(*openGLState);
	} while(0);

	Model *model = getModel();
	if(model){
		MotionPose mp[2];
		motions[0]->interpolate(mp[0], doorphase[0] * 10.);
		motions[1]->interpolate(mp[1], doorphase[1] * 10.);
		mp[0].next = &mp[1];

		const double scale = modelScale;
		Mat4d mat;

		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);
		gldScaled(scale);
		glScalef(-1, 1, -1);

#if 0
		for(int i = 0; i < nhitboxes; i++){
			Mat4d rot;
			glPushMatrix();
			gldTranslate3dv(hitboxes[i].org);
			rot = hitboxes[i].rot.tomat4();
			glMultMatrixd(rot);
			hitbox_draw(this, hitboxes[i].sc);
			glPopMatrix();
		}
#endif

		DrawMQOPose(model, mp);

		glPopMatrix();
		glPopAttrib();
	}
#endif
}

void Shipyard::drawtra(wardraw_t *wd){
	st::drawtra(wd);
	Shipyard *p = this;
	Shipyard *pt = this;
	Mat4d mat;
	Vec3d pa, pb, pa0(.01, 0, 0), pb0(-.01, 0, 0);
	double scale;

/*	if(scarry_cull(pt, wd))
		return;*/

	if(wd->vw->gc->cullFrustum(pos, getHitRadius()))
		return;

	scale = fabs(wd->vw->gc->scale(this->pos));

	transform(mat);

	pa = pt->rot.trans(pa0);
	pa += pt->pos;
	pb = pt->rot.trans(pb0);
	pb += pt->pos;
	glColor4ub(255,255,9,255);
	glBegin(GL_LINES);
	glVertex3dv(pa);
	glVertex3dv(pb);
	glEnd();

	{
		random_sequence rs;
		init_rseq(&rs, (unsigned long)this);

		double t0 = drseq(&rs);
		drawNavlights(wd, navlights, &mat);

		if(undockingFrigate){
			double t = fmod(wd->vw->viewtime + t0, 2.);
			/* runway lights */
			if(1 < scale * .01){
				Vec3d pos;
				GLubyte col[4];
				col[0] = 0;
				col[1] = 191;
				col[2] = 255;
				for(int i = 0 ; i <= 10; i++){
					avec3_t pos0;
					pos0[0] = 180 * modelScale;
					pos0[1] = 60 * modelScale;
					pos0[2] = (i * -460 + (10 - i) * -960) * modelScale / 10;
					double rad = .005 * (1. - fmod(i / 10. + t / 2., 1.));
					col[3] = 255/*rad * 255 / .01*/;
					mat4vp3(pos, mat, pos0);
					gldSpriteGlow(pos, rad, col, wd->vw->irot);
					pos0[0] = 20 * modelScale;
					mat4vp3(pos, mat, pos0);
					gldSpriteGlow(pos, rad, col, wd->vw->irot);
					pos0[1] = -60 * modelScale;
					mat4vp3(pos, mat, pos0);
					gldSpriteGlow(pos, rad, col, wd->vw->irot);
					pos0[0] = 180 * modelScale;
					mat4vp3(pos, mat, pos0);
					gldSpriteGlow(pos, rad, col, wd->vw->irot);
				}
			}
		}

/*		for(i = 0; i < numof(p->turrets); i++)
			mturret_drawtra(&p->turrets[i], pt, wd);*/
	}

}

void Shipyard::drawOverlay(WarDraw *wd){
	glCallList(disp);
/*	HSQUIRRELVM v = game->sqvm;
	StackReserver sr(v);
	sq_pushobject(v, sq_drawOverlayProc);
	sq_pushroottable(v);
	sq_call(v, 1, 0, SQTrue);*/
}

struct RandomSequenceCRC : RandomSequence{
	template<typename T> RandomSequenceCRC(const T &a)
		: RandomSequence(cpplib::crc32(&a, sizeof a)){}
	template<typename T1, typename T2> RandomSequenceCRC(const T1 &a, const T2 &b)
		: RandomSequence(cpplib::crc32(&a, sizeof a, cpplib::crc32(&b, sizeof b))){}
	double nextd0(){ return nextd() - 0.5; }
	double nextGauss2(){ return nextGauss() + nextGauss(); }
	Vec3d nextVec3d(double (RandomSequenceCRC::*method)() = &nextd0){
		return Vec3d((this->*method)(), (this->*method)(), (this->*method)());
	}
};


void Shipyard::dyingEffects(double dt){
	if(!game->isClient())
		return;

	Teline3List *tell = w->getTeline3d();
	static const double smokeFreq = 10.;
	if(tell){
		Vec3d halfLen = hitboxes.size() < 1 ? Vec3d(getHitRadius(), getHitRadius(), getHitRadius()) : hitboxes[0].sc;
		RandomSequenceCRC rs(id, game->universe->global_time);
		Vec3d gravity = w->accel(this->pos, this->velo) / 2.;
		int n = (int)(dt * smokeFreq + rs.nextd());
		for(int i = 0; i < n; i++){
			Vec3d pos = rs.nextVec3d(&RandomSequenceCRC::nextd0);
			for(int j = 0; j < 3; j++)
				pos[j] *= halfLen[j] * 2.;
			AddTelineCallback3D(tell, this->pos + this->rot.trans(pos),
				this->velo + rs.nextVec3d(&RandomSequenceCRC::nextGauss) * 0.01,
				.2, quat_u, vec3_000, gravity, firesmokedraw, NULL, TEL3_INVROTATE | TEL3_NOLINE, 2.5 + rs.nextd() * 1.5);
		}
	}
}

void Shipyard::deathEffects(){
	if(!game->isClient())
		return;
	WarSpace *ws = *w;
	if(!ws)
		return;
	Teline3List *tell = w->getTeline3d();
	RandomSequenceCRC rs(this);

	if(ws->gibs) for(int i = 0; i < 128; i++){
		/* gaussian spread is desired */
		AddTelineCallback3D(ws->gibs,
			this->pos + getHitRadius() * rs.nextVec3d(),
			this->velo + 0.02 * rs.nextVec3d(&RandomSequenceCRC::nextGauss2),
			.010, quat_u,
			M_PI * 2. * rs.nextVec3d(&RandomSequenceCRC::nextGauss),
			vec3_000, debrigib, NULL, TEL3_QUAT | TEL3_NOLINE,
			15. + rs.nextd() * 5.);
	}

	/* smokes */
	for(int i = 0; i < 64; i++){
		COLOR32 col = COLOR32RGBA(rs.next() % 32 + 127, rs.next() % 32 + 127, rs.next() % 32 + 127, 191);
		AddTelineCallback3D(ws->tell,
			this->pos + getHitRadius() * 0.5 * rs.nextVec3d(&RandomSequenceCRC::nextGauss),
			vec3_000, .14, quat_u, vec3_000,
			vec3_000, smokedraw, (void*)col, TEL3_INVROTATE | TEL3_NOLINE, 20.);
	}

	// explode shockwave
	AddTeline3D(tell, this->pos, vec3_000, 3., quat_u, vec3_000, vec3_000, COLOR32RGBA(255,255,255,127), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_INVROTATE, 2.);
}
