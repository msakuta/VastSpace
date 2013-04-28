/** \file
 * \brief Implementation of Worker drawing codes.
 */
#include "../Worker.h"
#include "Viewer.h"
#include "Player.h"
#include "draw/material.h"
//#include "judge.h"
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
#include <gl/glext.h>
#include <assert.h>
#include <string.h>


#define SCEPTER_SMOKE_FREQ 10.
#define SCEPTER_RELOADTIME .3
#define SCEPTER_ROLLSPEED (.2 * M_PI)
#define SCEPTER_ROTSPEED (.3 * M_PI)
#define SCEPTER_MAX_ANGLESPEED (M_PI * .5)
#define SCEPTER_ANGLEACCEL (M_PI * .2)
#define SCEPTER_MAX_GIBS 20
#define BULLETSPEED 2.


/* color sequences */
#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_orangeburn[] = {
	{0.1, COLOR32RGBA(255,255,191,0)},
	{0.1, COLOR32RGBA(255,255,191,255)},
	{0.15, COLOR32RGBA(255,255,31,191)},
	{0.45, COLOR32RGBA(255,127,31,95)},
	{0.3, COLOR32RGBA(255,31,0,63)},
};
static const struct color_sequence cs_orangeburn = DEFINE_COLSEQ(cnl_orangeburn, (COLOR32)-1, 1.1);
static const struct color_node cnl_shortburn[] = {
	{0.1, COLOR32RGBA(255,255,191,255)},
	{0.15, COLOR32RGBA(255,255,31,191)},
	{0.25, COLOR32RGBA(255,127,31,0)},
};
static const struct color_sequence cs_shortburn = DEFINE_COLSEQ(cnl_shortburn, (COLOR32)-1, 0.5);


extern double g_nlips_factor;
static int g_shader_enable = 0;


bool Worker::cull(Viewer &vw)const{
	double nf = nlipsFactor(vw);
	if(task == Worker::Undockque || vw.gc->cullFrustum(pos, .012 * nf))
		return true;
	double pixels = .008 * fabs(vw.gc->scale(pos)) * nf;
	if(pixels < 2)
		return true;
	return false;
}

/* NLIPS: Non-Linear Inverse Perspective Scrolling */
double Worker::nlipsFactor(Viewer &vw)const{
	double f = vw.fov * g_nlips_factor * 500. / vw.vp.m * 4. * ::sqrt((this->pos - vw.pos).len());
	return MAX(1., f);
}

void Worker::draw(wardraw_t *wd){
	static OpenGLState::weak_ptr<bool> init;
	static suf_t *sufbase = NULL, *sufbase1 = NULL;
	static suf_t *sufrev = NULL;
	static VBO *vbo[3] = {NULL};
	static suftex_t *suft, *suft1, *suft2;
	static GLuint shader = 0;
	static GLint fracLoc, cubeEnvLoc, textureLoc, invEyeMat3Loc, transparency;
	static Model *model = NULL;
	static Motion *lodMotion = NULL;
	static Motion *reverserMotion = NULL;
	double nf = nlipsFactor(*wd->vw);
	double scale = modelScale * nf;
	Worker *const p = this;
	if(!this->w /*|| this->docked*/)
		return;

	/* cull object */
	if(cull(*wd->vw))
		return;
	wd->lightdraws++;

	double pixels = .005 * fabs(wd->vw->gc->scale(pos)) * nf;

	draw_healthbar(this, wd, health / getMaxHealth(), .01 * nf, fuel / maxfuel(), -1.);

	if(!init) do{
		model = LoadMQOModel("models/Worker.mqo");
//		lodMotion = LoadMotion("models/interceptor_lod.mot");
//		reverserMotion = LoadMotion("models/interceptor_reverser.mot");

/*		do{
			GLuint vtx, frg;
			vtx = glCreateShader(GL_VERTEX_SHADER);
			frg = glCreateShader(GL_FRAGMENT_SHADER);
			if(!glsl_load_shader(vtx, "shaders/refract.vs") || !glsl_load_shader(frg, "shaders/refract.fs"))
				break;
			shader = glsl_register_program(vtx, frg);

			cubeEnvLoc = glGetUniformLocation(shader, "envmap");
			textureLoc = glGetUniformLocation(shader, "texture");
			invEyeMat3Loc = glGetUniformLocation(shader, "invEyeRot3x3");
			fracLoc = glGetUniformLocation(shader, "frac");
			transparency = glGetUniformLocation(shader, "transparency");
		}while(0);*/

		init.create(*openGLState);
	} while(0);

	if(model){
		static const double normal[3] = {0., 1., 0.};
		double x;
		double pyr[3];
		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

		glPushMatrix();
		gldTranslate3dv(this->pos);
		gldMultQuat(this->rot);
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

/*		MotionPose mp[2];
		lodMotion->interpolate(mp[0], pixels < 15 ? 10. : 0.);
		reverserMotion->interpolate(mp[1], reverser * 10.);
		mp[0].next = &mp[1];*/

		DrawMQOPose(model, NULL);

		glPopMatrix();

		glPopAttrib();
	}
}

#define COLIST4(a) COLOR32R(a),COLOR32G(a),COLOR32B(a),COLOR32A(a)

void Worker::drawtra(wardraw_t *wd){
	Worker *p = this;
	Mat4d mat;
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
	if(p->throttle){
		Vec3d pos, pos0(0, 0, 40. * scale);
/*		GLubyte col[4] = {COLIST4(cnl_shortburn[0].col)};
		pos = this->rot.trans(pos0);
		pos += this->pos;
		gldSpriteGlow(pos, p->throttle * .005, col, wd->vw->irot);
		pos0[0] = 34.5 * scale;
		pos = this->rot.trans(pos0);
		pos += this->pos;
		gldSpriteGlow(pos, p->throttle * .002, col, wd->vw->irot);
		pos0[0] = -34.5 * scale;
		pos = this->rot.trans(pos0);
		pos += this->pos;
		gldSpriteGlow(pos, p->throttle * .002, col, wd->vw->irot);*/

		GLuint texname = 0;
		const gltestp::TexCacheBind *tcb = gltestp::FindTexture("textures/blast.jpg");
		if(!tcb){
			suftexparam_t stp, stp2;
			stp.flags = STP_MAGFIL;
			stp.magfil = GL_LINEAR;
			stp2.flags = STP_ENV | STP_MAGFIL;
			stp2.env = GL_MODULATE;
			stp2.magfil = GL_LINEAR;
//			texname = CallCacheBitmap("textures/blast.jpg", "textures/blast.jpg", &stp, NULL);
			texname = CallCacheBitmap5("textures/blast.jpg", "textures/blast.jpg", &stp, "textures/noise.jpg", &stp2);
		}
		else
			texname = tcb->getList();

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
		glColor4f(1., 1., 1., amp);
		if(0. < throttle){
			for(int i = 0; i < enginePos.size(); i++){
				const Vec3d &pos = enginePos[i];
				gldScrollTextureBeam(wd->vw->pos, this->pos + this->rot.trans(pos), this->pos + this->rot.trans(pos + Vec3d(0,0,.010*amp)), .0025*amp, tim + 100. * rs.nextd());
			}
		}
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glPopAttrib();
		glPopMatrix();
	}

#if 0 /* thrusters appear unimpressing */
	tankrot(mat, pt);
	{
		avec3_t v;
		int j;
		VECADD(v, pt->omg, pt->pos);
		glColor4ub(255,127,0,255);
		glBegin(GL_LINES);
		glVertex3dv(pt->pos);
		glVertex3dv(v);
		glEnd();
		VECADD(v, p->aac, pt->pos);
		glColor4ub(255,127,255,255);
		glBegin(GL_LINES);
		glVertex3dv(pt->pos);
		glVertex3dv(v);
		glEnd();

		v[0] = p->thrusts[0][0];
		v[1] = p->thrusts[0][1];
		for(j = 0; j < 2; j++) if(v[j]){
			avec3_t v00[2] = {{0., 0., .003}, {0., 0., -.003}};
			int i;
			for(i = 0; i < 2; i++){
				struct gldBeamsData bd;
				avec3_t end;
				int lumi;
				struct random_sequence rs;
				int sign = (i + j) % 2 * 2 - 1;
				avec3_t v0;
				VECCPY(v0, v00[i]);
				bd.cc = bd.solid = 0;
				init_rseq(&rs, (long)(wd->gametime * 1e6) + i + pt);
				lumi = rseq(&rs) % 256 * min(fabs(v[j]) / .2, 1.);
				v0[1] += sign * .001;
				mat4vp3(end, mat, v0);
				gldBeams(&bd, wd->view, end, .00001, COLOR32RGBA(255,223,128,0));
				v0[1] += sign * .0003;
				mat4vp3(end, mat, v0);
				gldBeams(&bd, wd->view, end, .00007, COLOR32RGBA(255,255,255,lumi));
				v0[1] += sign * .0003;
				mat4vp3(end, mat, v0);
				gldBeams(&bd, wd->view, end, .00009, COLOR32RGBA(255,255,223,lumi));
				v0[1] += sign * .0003;
				mat4vp3(end, mat, v0);
				gldBeams(&bd, wd->view, end, .00005, COLOR32RGBA(255,255,191,0));
			}
		}
	}
#endif

	if(muzzleFlash) for(int i = 0; i < 2; i++){
		Vec3d pos = rot.trans(Vec3d(gunPos[i])) * nlips + this->pos;
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		glCallList(muzzle_texture());
/*		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glRotatef(-90, 0, 0, 1);
		glMatrixMode(GL_MODELVIEW);*/
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE); // Add blend
		GLfloat f = GLfloat(muzzleFlash / .1 * 2.);
		double fi = 1. - muzzleFlash / .1;
		glColor4f(f,f,f,1);
		gldTextureBeam(wd->vw->pos, pos, pos + rot.trans(-vec3_001) * .03 * fi, .01 * fi);
/*		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);*/
		glPopAttrib();
	}
}

void Worker::drawOverlay(wardraw_t *){
	glCallList(overlayDisp);
}

