/** \file
 * \brief Definition of drawing methods for Defender.
 */
#include "../defender.h"
#include "Player.h"
#include "draw/material.h"
#include "judge.h"
#include "draw/effects.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/mqoadapt.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/suf/sufbin.h>
#include <clib/suf/sufdraw.h>
#include <clib/suf/sufvbo.h>
#include <clib/avec3.h>
#include <clib/amat4.h>
#include <clib/aquat.h>
#include <clib/GL/gldraw.h>
#include <clib/GL/multitex.h>
#include <clib/wavsound.h>
#include <clib/zip/UnZip.h>
}
#include <cpplib/vec4.h>
#include <gl/glext.h>
#include <assert.h>
#include <string.h>

#define SQRT2P2 (M_SQRT2/2.)

extern double g_nlips_factor;

Model *Defender::model = NULL;
Motion *Defender::motions[2] = {NULL};

bool Defender::cull(Viewer &vw)const{
	double nf = nlipsFactor(vw);
	if(task == Defender::Undockque || vw.gc->cullFrustum(pos, 12. * nf))
		return true;
	double pixels = 8. * fabs(vw.gc->scale(pos)) * nf;
	if(pixels < 2)
		return true;
	return false;
}

/// Returns NLIPS factor.
/// NLIPS: Non-Linear Inverse Perspective Scrolling
double Defender::nlipsFactor(Viewer &vw)const{
	double f = vw.fov * g_nlips_factor * .5 / vw.vp.m * 4. * ::sqrt((this->pos - vw.pos).len());
	return MAX(1., f);
}

void Defender::draw(wardraw_t *wd){
	static OpenGLState::weak_ptr<bool> init;
	static suf_t *sufbase = NULL, *sufbase1 = NULL;
	static suf_t *sufengine = NULL, *sufengine1 = NULL;
	static suf_t *sufmagazine = NULL, *sufmagazine1 = NULL;
	static VBO *vbo[5] = {NULL};
	static suftex_t *suft, *suft1, *suft2, *suftengine, *suftengine1, *suftmagazine;
	static GLuint shader = 0;
	static GLint fracLoc, cubeEnvLoc, textureLoc, invEyeMat3Loc, transparency;

	double nf = nlipsFactor(*wd->vw);
	double scale = modelScale * nf;
	Defender *const p = this;
	if(!this->w /*|| this->docked*/)
		return;

	/* cull object */
	if(cull(*wd->vw))
		return;
	wd->lightdraws++;

	double pixels = 10. * fabs(wd->vw->gc->scale(pos)) * nf;

	draw_healthbar(this, wd, health / getMaxHealth(), 10. * nf, fuel / maxfuel(), -1.);

/*	struct TextureParams{
		Defender *p;
		WarDraw *wd;
		static void onBeginTextureMagazine(void *pv){
			TextureParams *p = (TextureParams*)pv;
			if(p && p->wd)
				p->wd->setAdditive(true);
		}
		static void onEndTextureMagazine(void *pv){
			TextureParams *p = (TextureParams*)pv;
			if(p && p->wd)
				p->wd->setAdditive(false);
		}
	} tp = {this, wd};*/

	static int magazineIndex = -1;

	if(!init) do{
		delete model;
		model = LoadMQOModel("models/defender.mqo");
		motions[0] = LoadMotion("models/defender_deploy.mot");
		motions[1] = LoadMotion("models/defender_reload.mot");

/*		if(suftmagazine) for(int i = 0; i < suftmagazine->n; i++) if(sufmagazine->a[i].name == gltestp::dstring("defender_magazine")){
			suftmagazine->a[i].onBeginTexture = TextureParams::onBeginTextureMagazine;
			suftmagazine->a[i].onEndTexture = TextureParams::onEndTextureMagazine;
			magazineIndex = i;
		}*/

		init.create(*openGLState);
	} while(0);

/*	if(suftmagazine){
		if(0 <= magazineIndex){
			suftmagazine->a[magazineIndex].onBeginTextureData = &tp;
			suftmagazine->a[magazineIndex].onEndTextureData = &tp;
		}
	}*/

	if(model){
		static const double normal[3] = {0., 1., 0.};
		double x;
		double pyr[3];
		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

		glPushMatrix();
		gldTranslate3dv(this->pos);
		gldMultQuat(this->rot);
		gldScaled(scale);
//		glScalef(-1, 1, -1);

#if 1
		MotionPose mp[2];
		getPose(mp);

		DrawMQOPose(model, mp);
#else
#if 0
		if(g_shader_enable && p->fcloak){
			extern float g_shader_frac;
			extern Star sun;
			extern coordsys *g_galaxysystem;
			GLuint cubetex;
			amat4_t rot, rot2;
			GLfloat irot3[9];
			tocsm(rot, wd->vw->cs, g_galaxysystem);
			mat4mp(rot2, rot, wd->vw->irot);
			MAT4TO3(irot3, rot2);
		glUseProgram(shader);
			glUniform1f(fracLoc, .5 * (1. - p->fcloak) + g_shader_frac * p->fcloak);
			glUniform1f(transparency, p->fcloak);
			glUniformMatrix3fv(invEyeMat3Loc, 1, GL_FALSE, irot3);
			glUniform1i(textureLoc, 0);
			if(glActiveTextureARB){
				glUniform1i(cubeEnvLoc, 1);
				cubetex = Island3MakeCubeMap(wd->vw, NULL, &sun);
				glActiveTextureARB(GL_TEXTURE1_ARB);
				glBindTexture(GL_TEXTURE_CUBE_MAP, cubetex);
				glActiveTextureARB(GL_TEXTURE0_ARB);
			}
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	/*		glColor4ub(0,0,0,255);
			DrawSUF(scepter_s.sufbase, SUF_ATR, &g_gldcache);*/
			DecalDrawSUF(scepter_s.sufbase, SUF_ATR, NULL, suft, NULL, NULL, NULL);
		glUseProgram(0);
			if(glActiveTextureARB){
				glActiveTextureARB(GL_TEXTURE1_ARB);
				glBindTexture(GL_TEXTURE_2D, 0);
				glActiveTextureARB(GL_TEXTURE0_ARB);
			}
		}
		else
#endif
#endif
		glPopMatrix();

		glPopAttrib();
	}
}

#define COLIST4(a) COLOR32R(a),COLOR32G(a),COLOR32B(a),COLOR32A(a)

void Defender::drawtra(wardraw_t *wd){
	Defender *p = this;
	double nlips = nlipsFactor(*wd->vw);
	double scale = modelScale * nlips;

#if PIDAIM_PROFILE
	glBegin(GL_LINES);
	glColor4ub(255,0,0,255);
	glVertex3dv(pos);
	glVertex3dv(epos);
	glColor4ub(255,255,0,255);
	glVertex3dv(pos);
	glVertex3dv(iepos);
	glEnd();
#endif

	if(wd->r_move_path && (task == Moveto || task == DeltaFormation)){
		glBegin(GL_LINES);
		glColor4ub(0,0,255,255);
		glVertex3dv(pos);
		glColor4ub(0,255,255,255);
		glVertex3dv(dest);
		glEnd();
	}

	/* cull object */
	if(cull(*wd->vw))
		return;
	if(p->throttle || Dodge0 <= task && task <= Dodge3){
		Vec3d pos0(0, 0, 40. * scale);

		GLuint texname = 0;
		const gltestp::TexCacheBind *tcb = gltestp::FindTexture("textures/blast.jpg");
		if(tcb)
			texname = tcb->getList();
		else{
			suftexparam_t stp, stp2;
			stp.flags = STP_MAGFIL;
			stp.magfil = GL_LINEAR;
			stp2.flags = STP_ENV | STP_MAGFIL;
			stp2.env = GL_MODULATE;
			stp2.magfil = GL_LINEAR;
//			texname = CallCacheBitmap("textures/blast.jpg", "textures/blast.jpg", &stp, NULL);
			texname = CallCacheBitmap5("textures/blast.jpg", "textures/blast.jpg", &stp, "textures/noise.jpg", &stp2);
		}

		glPushMatrix();
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		glCallList(texname);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Add blend
		float amp = MIN(float(fabs(this->throttle)) * 30.f, 1.);
		double tim = -30. * wd->w->war_time();
		RandomSequence rs((unsigned long)this);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glRotated(147.43462, 0, 0, 1);
		glScaled(.2, .2, .2);
		glMatrixMode(GL_MODELVIEW);
		glColor4f(1., 1., 1., Dodge0 <= task && task <= Dodge3 ? 1. : amp);
		if(Dodge0 <= task && task <= Dodge3){
			int i = task - Dodge0;
			if(fdodge < .5)
				i ^= 3; // Reverting thrust direction. If you're not familiar with bitwise operations, try not to understand...
			Mat4d legmat = legTransform(i);
			Vec3d org(0, 0, 130. * scale);
			gldScrollTextureBeam(wd->vw->pos, legmat.vp3(org), legmat.vp3(org + Vec3d(0, 0, 20.)), 5., tim + 100. * rs.nextd());
		}
		else{
			double ofs = 5.*scale;
			Vec3d pos = this->pos /*+ this->rot.trans(Vec3d(0,0,35.*scale))*/;
			for(int i = 0; i < 4; i++){
				Mat4d legmat = legTransform(i);
				Vec3d org(0, 0, throttle < 0. ? 0. : 130. * scale);
				Vec3d dst(0, 0, throttle < 0. ? -20. * amp : 20. * amp);
				gldScrollTextureBeam(wd->vw->pos, legmat.vp3(org), legmat.vp3(org + dst), 5. * amp, tim + 100. * rs.nextd());
			}
		}
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glPopAttrib();
		glPopMatrix();
	}

	if(mf){
		Vec3d pos = rot.trans(gunPos) * nlips + this->pos;
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		gldSpriteGlow(pos, 10., Vec4<GLubyte>(127,255,255,min(mf*255,255)), wd->vw->irot);
		glPopAttrib();
	}
}

void Defender::drawOverlay(WarDraw *){
	glCallList(overlayDisp);
}

