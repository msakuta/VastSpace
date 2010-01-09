#include "../sceptor.h"
//#include "player.h"
//#include "bullet.h"
//#include "coordsys.h"
//#include "viewer.h"
//#include "cmd.h"
//#include "glwindow.h"
//#include "arms.h"
#include "../material.h"
//#include "warutil.h"
#include "../judge.h"
//#include "astrodef.h"
//#include "stellar_file.h"
//#include "antiglut.h"
//#include "worker.h"
//#include "glsl.h"
//#include "astro_star.h"
//#include "sensor.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/suf/sufbin.h>
#include <clib/suf/sufdraw.h>
#include <clib/avec3.h>
#include <clib/amat4.h>
#include <clib/aquat.h>
#include <clib/GL/gldraw.h>
#include <clib/wavsound.h>
#include <clib/zip/UnZip.h>
}
#include <gl/glext.h>
#include <assert.h>
#include <string.h>


#define SCEPTER_SCALE 1./10000
#define SCEPTER_SMOKE_FREQ 10.
#define SCEPTER_RELOADTIME .3
#define SCEPTER_ROLLSPEED (.2 * M_PI)
#define SCEPTER_ROTSPEED (.3 * M_PI)
#define SCEPTER_MAX_ANGLESPEED (M_PI * .5)
#define SCEPTER_ANGLEACCEL (M_PI * .2)
#define SCEPTER_MAX_GIBS 20
#define BULLETSPEED 2.


/* color sequences */
extern const struct color_sequence cs_orangeburn, cs_shortburn;
#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_orangeburn[] = {
	{0.1, COLOR32RGBA(255,255,191,255)},
	{0.15, COLOR32RGBA(255,255,31,191)},
	{0.45, COLOR32RGBA(255,127,31,95)},
	{2.3, COLOR32RGBA(255,31,0,63)},
};
const struct color_sequence cs_orangeburn = DEFINE_COLSEQ(cnl_orangeburn, (COLOR32)-1, 3.);
static const struct color_node cnl_shortburn[] = {
	{0.1, COLOR32RGBA(255,255,191,255)},
	{0.15, COLOR32RGBA(255,255,31,191)},
	{0.25, COLOR32RGBA(255,127,31,0)},
};
const struct color_sequence cs_shortburn = DEFINE_COLSEQ(cnl_shortburn, (COLOR32)-1, 0.5);


static double g_nlips_factor = 0.;
static int g_shader_enable = 0;


#if 0
static int scepter_cull(entity_t *pt, wardraw_t *wd, double scale){
	scepter_t *p = (scepter_t*)pt;
	double pixels;
	if(p->task == scepter_undockque || glcullFrustum(pt->pos, .012 * scale, wd->pgc))
		return 1;
	pixels = .008 * fabs(glcullScale(&pt->pos, wd->pgc));
	if(pixels < 2)
		return 1;
	return 0;
}
#endif

void Sceptor::draw(wardraw_t *wd){
	static int init = 0;
	static suf_t *sufbase = NULL;
	static suftex_t *suft;
	static GLuint shader = 0;
	static GLint fracLoc, cubeEnvLoc, textureLoc, invEyeMat3Loc, transparency;
	double scale = SCEPTER_SCALE;
	Sceptor *const p = this;
	if(!this->w || this->docked)
		return;

	/* NLIPS: Non-Linear Inverse Perspective Scrolling */
	scale *= 1. + wd->vw->fov * g_nlips_factor * 500. / wd->vw->vp.m * 2. * (this->pos - wd->vw->pos).len();

	/* cull object */
/*	if(scepter_cull(pt, wd, scale))
		return;*/
	wd->lightdraws++;

	draw_healthbar(this, wd, health / maxhealth(), .01, fuel / maxfuel(), -1.);

	if(init == 0) do{
		FILE *fp;
		sufbase = CallLoadSUF("interceptor0.bin");
//		scepter_s.sufbase = LZUC(lzw_interceptor0);
		if(!sufbase) break;
		{
//			BITMAPFILEHEADER *bfh;
//			suftexparam_t stp;
//			stp.bmi = lzuc(lzw_interceptor, sizeof lzw_interceptor, NULL);
			CacheSUFMaterials(sufbase);
//			CallCacheBitmap("interceptor.bmp", "interceptor.bmp", NULL, NULL);
/*			bfh = ZipUnZip("rc.zip", "interceptor.bmp", NULL);
			if(!bfh)
				return;
			stp.bmi = &bfh[1];
			stp.env = GL_MODULATE;
			stp.mipmap = 0;
			stp.alphamap = 0;
			stp.magfil = GL_LINEAR;
			CacheSUFMTex("interceptor.bmp", &stp, NULL);
			free(bfh);*/
//			stp.bmi = lzuc(lzw_engine, sizeof lzw_engine, NULL);
//			CallCacheBitmap("engine.bmp", "engine.bmp", NULL, NULL);
/*			bfh = ZipUnZip("rc.zip", "engine.bmp", NULL);
			if(!bfh)
				return;
			stp.bmi = &bfh[1];
			CacheSUFMTex("engine.bmp", &stp, NULL);
			free(bfh/*stp.bmi*);*/
		}
		suft = AllocSUFTex(sufbase);

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
			DecalDrawSUF(sufbase, SUF_ATR, NULL, suft, NULL, NULL);
		}
		glPopMatrix();

/*		if(0 < wd->light[1]){
			static const double normal[3] = {0., 1., 0.};
			ShadowSUF(scepter_s.sufbase, wd->light, normal, pt->pos, pt->pyr, scale, NULL);
		}*/
		glPopAttrib();
	}
}

#define COLIST4(a) COLOR32R(a),COLOR32G(a),COLOR32B(a),COLOR32A(a)

void Sceptor::drawtra(wardraw_t *wd){
	Sceptor *p = this;
	Mat4d mat;

	/* cull object */
/*	if(scepter_cull(pt, wd, 1.))
		return;*/
	if(p->throttle){
		Vec3d pos, pos0(0, 0, 40. * SCEPTER_SCALE);
		GLubyte col[4] = {COLIST4(cnl_shortburn[0].col)};
		pos = this->rot.trans(pos0);
		pos += this->pos;
		gldSpriteGlow(pos, p->throttle * .005, col, wd->vw->irot);
		pos0[0] = 34.5 * SCEPTER_SCALE;
		pos = this->rot.trans(pos0);
		pos += this->pos;
		gldSpriteGlow(pos, p->throttle * .002, col, wd->vw->irot);
		pos0[0] = -34.5 * SCEPTER_SCALE;
		pos = this->rot.trans(pos0);
		pos += this->pos;
		gldSpriteGlow(pos, p->throttle * .002, col, wd->vw->irot);
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

	if(mf) for(int i = 0; i < 2; i++){
		Vec3d pos = rot.trans(Vec3d(scepter_guns[i])) + this->pos;
		static GLuint texname = 0;
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		if(!texname){
			suftexparam_t stp;
			stp.flags = STP_ENV | STP_MAGFIL | STP_MINFIL | STP_WRAP_S;
			stp.env = GL_MODULATE;
			stp.magfil = GL_LINEAR;
			stp.minfil = GL_LINEAR;
			stp.wraps = GL_CLAMP_TO_BORDER;
			texname = CallCacheBitmap5("muzzle.bmp", "muzzle.bmp", &stp, NULL, NULL);
		}
		glCallList(texname);
/*		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glRotatef(-90, 0, 0, 1);
		glMatrixMode(GL_MODELVIEW);*/
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE); // Add blend
		float f = mf / .1 * 2., fi = 1. - mf / .1;
		glColor4f(f,f,f,1);
		gldTextureBeam(wd->vw->pos, pos, pos + rot.trans(-vec3_001) * .03 * fi, .01 * fi);
/*		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);*/
		glPopAttrib();
	}
}

