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


bool Defender::cull(Viewer &vw)const{
	double nf = nlipsFactor(vw);
	if(task == Defender::Undockque || vw.gc->cullFrustum(pos, .012 * nf))
		return true;
	double pixels = .008 * fabs(vw.gc->scale(pos)) * nf;
	if(pixels < 2)
		return true;
	return false;
}

/// Returns NLIPS factor.
/// NLIPS: Non-Linear Inverse Perspective Scrolling
double Defender::nlipsFactor(Viewer &vw)const{
	double f = vw.fov * g_nlips_factor * 500. / vw.vp.m * 4. * ::sqrt((this->pos - vw.pos).len());
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
	double scale = modelScale() * nf;
	Defender *const p = this;
	if(!this->w /*|| this->docked*/)
		return;

	/* cull object */
	if(cull(*wd->vw))
		return;
	wd->lightdraws++;

	double pixels = .010 * fabs(wd->vw->gc->scale(pos)) * nf;

	draw_healthbar(this, wd, health / maxhealth(), .01 * nf, fuel / maxfuel(), -1.);

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
		free(sufbase);
		free(sufbase1);
		free(sufengine);
		free(sufengine1);
		free(sufmagazine);
		sufbase = CallLoadSUF("models/defender0_body.bin");
		sufbase1 = CallLoadSUF("models/defender1_body.bin");
		sufengine = CallLoadSUF("models/defender0_engine.bin");
		sufengine1 = CallLoadSUF("models/defender1_engine.bin");
		sufmagazine = CallLoadSUF("models/defender0_magazine.bin");
		vbo[0] = CacheVBO(sufbase);
		vbo[1] = CacheVBO(sufbase1);
		vbo[2] = CacheVBO(sufengine);
		vbo[3] = CacheVBO(sufengine1);
		vbo[4] = CacheVBO(sufmagazine);
		if(!sufbase) break;
/*		TexParam stp;
		stp.flags = STP_ENV;
		stp.env = GL_ADD;
		AddMaterial("defender_magazine.png", "models/defender_magazine_br.png", &stp, "models/defender_magazine.png", NULL);*/
		CacheSUFMaterials(sufbase);
		CacheSUFMaterials(sufmagazine);
		suft = gltestp::AllocSUFTex(sufbase);
//		suft1 = gltestp::AllocSUFTex(sufbase1);
		suftengine = gltestp::AllocSUFTex(sufengine);
		suftengine1 = gltestp::AllocSUFTex(sufengine1);
		suftmagazine = gltestp::AllocSUFTex(sufmagazine);

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

	if(!sufbase){
		double pos[3];
		GLubyte col[4] = {255,255,0,255};
		gldPseudoSphere(pos, .005, col);
	}
	else{
		static const double normal[3] = {0., 1., 0.};
		double x;
		double pyr[3];
		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

		glPushMatrix();
		gldTranslate3dv(this->pos);
		gldMultQuat(this->rot);
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
		gldScaled(scale);
		glScalef(-1, 1, -1);
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
		{
			if(pixels < 25){
				if(vbo[1])
					DrawVBO(vbo[1], SUF_ATR | SUF_TEX, suft);
				else
					DecalDrawSUF(sufbase1, SUF_ATR, NULL, suft, NULL, NULL);
			}
			else{
				if(vbo[0])
					DrawVBO(vbo[0], SUF_ATR | SUF_TEX, suft);
				else
					DecalDrawSUF(sufbase, SUF_ATR, NULL, suft, NULL, NULL);
			}

			glPushMatrix();
			glRotated(90. * rangein((frotate / rotateTime * 2. - .5), 0., 1.), 0, 0, 1);
			if(vbo[4])
				DrawVBO(vbo[4], wd->shadowmapping ? 0 : SUF_ATR | SUF_TEX, suftmagazine);
			else
				DecalDrawSUF(sufmagazine, wd->shadowmapping ? 0 : SUF_ATR, NULL, suftmagazine, NULL, NULL);
			glPopMatrix();

			for(int ix = 0; ix < 2; ix++) for(int iy = 0; iy < 2; iy++){
				glFrontFace((ix + iy) % 2 ? GL_CW : GL_CCW);
				glPushMatrix();
				glScalef(1 - ix * 2, 1 - iy * 2, 1);
				glTranslated(22.5, 22.5, -30);
				glRotated(MIN(fdeploy * 135, 90), 1, 0, 0);
				glRotated(MAX(fdeploy * 135 - 90, 0), 0, -1, 0);
				glTranslated(-22.5, -22.5, 30);
				if(pixels < 25){
					if(vbo[3])
						DrawVBO(vbo[3], SUF_ATR | SUF_TEX, suftengine1);
					else
						DecalDrawSUF(sufengine1, SUF_ATR, NULL, suftengine1, NULL, NULL);
				}
				else{
					if(vbo[2])
						DrawVBO(vbo[2], SUF_ATR | SUF_TEX, suftengine);
					else
						DecalDrawSUF(sufengine, SUF_ATR, NULL, suftengine, NULL, NULL);
				}
				glPopMatrix();
			}
			glFrontFace(GL_CCW);

/*			for(int i = 0; i < 2; i++){
				glPushMatrix();
				if(i){
					glScalef(1, -1, 1);
					glFrontFace(GL_CW);
				}
				if(0. < reverser){
					glTranslated(0, 0, -25);
					glRotated(reverser * 30, -1, 0, 0);
					glTranslated(0, 0, 25);
				}
				if(vbo[2])
					DrawVBO(vbo[2], SUF_ATR | SUF_TEX, suft);
				else
					DecalDrawSUF(sufrev, SUF_ATR, NULL, suft, NULL, NULL);
				glPopMatrix();
			}
			glFrontFace(GL_CCW);*/
		}
		glPopMatrix();

		glPopAttrib();
	}
}

#define COLIST4(a) COLOR32R(a),COLOR32G(a),COLOR32B(a),COLOR32A(a)

void Defender::drawtra(wardraw_t *wd){
	Defender *p = this;
	Mat4d mat;
	double nlips = nlipsFactor(*wd->vw);
	double scale = modelScale() * nlips;

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

	Player *ppl = w->getPlayer();

	if(ppl && ppl->r_move_path && (task == Moveto || task == DeltaFormation)){
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
			gldScrollTextureBeam(wd->vw->pos, legmat.vp3(org), legmat.vp3(org + Vec3d(0, 0, .020)), .005, tim + 100. * rs.nextd());
		}
		else{
			double ofs = 5.*scale;
			Vec3d pos = this->pos /*+ this->rot.trans(Vec3d(0,0,35.*scale))*/;
			for(int i = 0; i < 4; i++){
				Mat4d legmat = legTransform(i);
				Vec3d org(0, 0, throttle < 0. ? 0. : 130. * scale);
				Vec3d dst(0, 0, throttle < 0. ? -.020 * amp : .020 * amp);
				gldScrollTextureBeam(wd->vw->pos, legmat.vp3(org), legmat.vp3(org + dst), .005 * amp, tim + 100. * rs.nextd());
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
		Vec3d pos = rot.trans(Vec3d(gunPos[0])) * nlips + this->pos;
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		gldSpriteGlow(pos, .01, Vec4<GLubyte>(127,255,255,min(mf*255,255)), wd->vw->irot);
		glPopAttrib();
	}
}

void Defender::drawOverlay(wardraw_t *){
	glScaled(10, 10, 1);
	glBegin(GL_LINE_LOOP);
	glVertex2d(-.10,  .02);
	glVertex2d(-.10, -.02);
	glVertex2d(-.03, -.02);
	glVertex2d(-.06, -.06);
	glVertex2d(-.03, -.09);
	glVertex2d( .00, -.04);
	glVertex2d( .03, -.09);
	glVertex2d( .06, -.06);
	glVertex2d( .03, -.02);
	glVertex2d( .10, -.02);
	glVertex2d( .10,  .02);
	glVertex2d( .03,  .02);
	glVertex2d( .06,  .06);
	glVertex2d( .03,  .09);
	glVertex2d( .00,  .04);
	glVertex2d(-.03,  .09);
	glVertex2d(-.06,  .06);
	glVertex2d(-.03,  .02);
	glEnd();
}
