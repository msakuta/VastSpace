/** \file
 * \brief Definition of drawing methods for Defender.
 */
#include "../defender.h"
#include "../player.h"
#include "../material.h"
#include "../judge.h"
#include "effects.h"
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
	static int init = 0;
	static suf_t *sufbase = NULL/*, *sufbase1 = NULL*/;
	static suf_t *sufengine = NULL, *sufengine1 = NULL;
	static suf_t *sufrev = NULL;
	static VBO *vbo[4] = {NULL};
	static suftex_t *suft, *suft1, *suft2, *suftengine, *suftengine1;
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

	if(init == 0) do{
//		FILE *fp;
		sufbase = CallLoadSUF("models/defender0_body.bin");
//		sufbase1 = CallLoadSUF("models/interceptor1.bin");
		sufengine = CallLoadSUF("models/defender0_engine.bin");
		sufengine1 = CallLoadSUF("models/defender1_engine.bin");
//		sufrev = CallLoadSUF("models/interceptor0_reverser0.bin");
		vbo[0] = CacheVBO(sufbase);
//		vbo[1] = CacheVBO(sufbase1);
		vbo[2] = CacheVBO(sufengine);
		vbo[3] = CacheVBO(sufengine1);
//		vbo[2] = CacheVBO(sufrev);
		if(!sufbase) break;
		CacheSUFMaterials(sufbase);
		suft = AllocSUFTex(sufbase);
//		suft1 = AllocSUFTex(sufbase1);
		suftengine = AllocSUFTex(sufengine);
		suftengine1 = AllocSUFTex(sufengine1);
//		suft2 = AllocSUFTex(sufrev);

		init = 1;
	} while(0);
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
#if 1
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
			if(vbo[0])
				DrawVBO(vbo[0], SUF_ATR | SUF_TEX, suft);
			else
//				DrawSUF(sufbase, SUF_ATR, NULL);
				DecalDrawSUF(sufbase, SUF_ATR, NULL, suft, NULL, NULL);

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

		static bool init = false;
		static GLuint texname = 0;
		if(!init){
			init = true;
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

void Defender::drawOverlay(wardraw_t *wd){
	Player *ppl = w->getPlayer();
	double pixels;
	if(ppl && ppl->r_overlay && 0. < (pixels = wd->vw->gc->scale(this->pos) * hitradius()) && pixels * 10. < wd->vw->vp.m){
		Vec4d spos = wd->vw->trans.vp(Vec4d(this->pos) + Vec4d(0,0,0,1));
		glPushMatrix();
		glLoadIdentity();
		glTranslated((spos[0] / spos[3] + 1.) * wd->vw->vp.w / 2., (1. - spos[1] / spos[3]) * wd->vw->vp.h / 2., 0.);
		glScaled(200, 200, 1);
		glColor4f(1, 1, 1, 1. - pixels * 10. / wd->vw->vp.m);
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
		glPopMatrix();
	}
}

void Defender::smokedraw(const struct tent3d_line_callback *p, const struct tent3d_line_drawdata *dd, void *private_data){
	glPushMatrix();
	gldTranslate3dv(p->pos);
	glMultMatrixd(dd->invrot);
	gldScaled(p->len);
	struct random_sequence rs;
	init_rseq(&rs, (long)p);
	glRotated(rseq(&rs) % 360, 0, 0, 1);
//	gldMultQuat(Quatd::direction(Vec3d(p->pos) - Vec3d(dd->viewpoint)));
	static GLuint lists[2] = {0};
	glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
	if(!lists[0]){
		suftexparam_t stp;
		stp.flags = STP_ENV | STP_ALPHA | STP_ALPHATEX | STP_MAGFIL | STP_MINFIL;
		stp.env = GL_MODULATE;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		lists[0] = CallCacheBitmap5("textures/smoke.bmp", "textures/smoke.bmp", &stp, NULL, NULL);
		lists[1] = CallCacheBitmap5("textures/smokefire.bmp", "textures/smokefire.bmp", &stp, NULL, NULL);
	}
	for(int i = 0; i < 2; i++){
		glCallList(lists[i]);
		glColor4f(1, 1, 1, i == 0 ? MIN(p->life * 1., 1.) : MAX(MIN(p->life * 1. - 1., 1.), 0));
/*		glBegin(GL_QUADS);
		glTexCoord2f(0,0); glVertex2f(-1, -1);
		glTexCoord2f(1,0); glVertex2f(+1, -1);
		glTexCoord2f(1,1); glVertex2f(+1, +1);
		glTexCoord2f(0,1); glVertex2f(-1, +1);
		glEnd();*/
		glBegin(GL_TRIANGLE_FAN);
		glTexCoord2f( .5,  .5); glNormal3f( 0,  0, 1); glVertex2f( 0,  0);
		glTexCoord2f( .0,  .0); glNormal3f(-1, -1, 0); glVertex2f(-1, -1);
		glTexCoord2f( 1.,  .0); glNormal3f( 1, -1, 0); glVertex2f( 1, -1);
		glTexCoord2f( 1.,  1.); glNormal3f( 1,  1, 0); glVertex2f( 1,  1);
		glTexCoord2f( 0.,  1.); glNormal3f(-1,  1, 0); glVertex2f(-1,  1);
		glTexCoord2f( 0.,  0.); glNormal3f(-1, -1, 0); glVertex2f(-1, -1);
		glEnd();
	}
	glPopAttrib();
	glPopMatrix();
}
