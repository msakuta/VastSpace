/** \file
 * \brief Implementation of Sceptor drawing codes.
 */
#include "../Sceptor.h"
#include "Viewer.h"
#include "Player.h"
#include "draw/material.h"
//#include "judge.h"
#include "draw/effects.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/mqoadapt.h"
#include "glw/glwindow.h"
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
#define BULLETSPEED 2000.


/* color sequences */
extern const struct color_sequence cs_orangeburn, cs_shortburn;
#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_orangeburn[] = {
	{0.1, COLOR32RGBA(255,255,191,0)},
	{0.1, COLOR32RGBA(255,255,191,255)},
	{0.15, COLOR32RGBA(255,255,31,191)},
	{0.45, COLOR32RGBA(255,127,31,95)},
	{0.3, COLOR32RGBA(255,31,0,63)},
};
const struct color_sequence cs_orangeburn = DEFINE_COLSEQ(cnl_orangeburn, (COLOR32)-1, 1.1);
static const struct color_node cnl_shortburn[] = {
	{0.1, COLOR32RGBA(255,255,191,255)},
	{0.15, COLOR32RGBA(255,255,31,191)},
	{0.25, COLOR32RGBA(255,127,31,0)},
};
const struct color_sequence cs_shortburn = DEFINE_COLSEQ(cnl_shortburn, (COLOR32)-1, 0.5);


double g_nlips_factor = 1.;
static int g_shader_enable = 0;


bool Sceptor::cull(Viewer &vw)const{
	double nf = nlipsFactor(vw);
	if(task == Sceptor::Undockque || vw.gc->cullFrustum(pos, 12. * nf))
		return true;
	double pixels = 8. * fabs(vw.gc->scale(pos)) * nf;
	if(pixels < 2)
		return true;
	return false;
}

/* NLIPS: Non-Linear Inverse Perspective Scrolling */
double Sceptor::nlipsFactor(Viewer &vw)const{
	double f = vw.fov * g_nlips_factor * .5 / vw.vp.m * 4. * ::sqrt((this->pos - vw.pos).len());
	return MAX(1., f);
}

void Sceptor::draw(wardraw_t *wd){
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
	Sceptor *const p = this;
	if(!this->w /*|| this->docked*/)
		return;

	/* cull object */
	if(cull(*wd->vw))
		return;
	wd->lightdraws++;

	double pixels = 5. * fabs(wd->vw->gc->scale(pos)) * nf;

	if(!controller)
		draw_healthbar(this, wd, health / getMaxHealth(), 10. * nf, fuel / maxfuel(), -1.);

	if(!init) do{
		model = LoadMQOModel("models/interceptor.mqo");
		lodMotion = LoadMotion("models/interceptor_lod.mot");
		reverserMotion = LoadMotion("models/interceptor_reverser.mot");

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

	{
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

		MotionPose mp[2];
		lodMotion->interpolate(mp[0], pixels < 15 ? 10. : 0.);
		reverserMotion->interpolate(mp[1], reverser * 10.);
		mp[0].next = &mp[1];

		DrawMQOPose(model, mp);

		glPopMatrix();

		glPopAttrib();
	}
}

#define COLIST4(a) COLOR32R(a),COLOR32G(a),COLOR32B(a),COLOR32A(a)

void Sceptor::drawtra(wardraw_t *wd){
	Sceptor *p = this;
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
		const EnginePosList &engines = 0. < throttle ? enginePos : enginePosRev;
		for(int i = 0; i < engines.size(); i++){
			const EnginePos &e = engines[i];
			Vec3d basePos = this->pos + this->rot.trans(e.pos);
			gldScrollTextureBeam(wd->vw->pos, basePos,
				basePos + (this->rot * e.rot).trans(Vec3d(0,0,.010*amp)),
				0.0025*amp, tim + 100. * rs.nextd());
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
		gldTextureBeam(wd->vw->pos, pos, pos + rot.trans(-vec3_001) * 30. * fi, 10. * fi);
/*		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);*/
		glPopAttrib();
	}
}

void Sceptor::drawHUD(WarDraw *wd){

	timemeas_t tm;
	TimeMeasStart(&tm);
	st::drawHUD(wd);

	// If the camera is in tactical mode, HUD won't make much sense.
	if(!(game->player && game->player->mover == game->player->cockpitview))
		return;

	glPushAttrib(GL_LINE_BIT);
	glDisable(GL_LINE_SMOOTH);

	// Get viewport size
	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);
	int w = vp[2], h = vp[3];
	int maxSize = w < h ? h : w;
	int minSize = w < h ? w : h;
	double vpLeft = -(double)w / minSize;
	double vpBottom = -(double)h / minSize;

	// Correct aspect ratio
	glPushMatrix();
	glLoadIdentity();
	glScaled(1./*(double)mi / w*/, 1./*(double)mi / h*/, (double)maxSize / minSize);

	const double fontSize = 0.0025;

	{
		const double size = 0.1;
		const double gap = 0.025;
		// Crosshair
		glBegin(GL_LINES);
		glVertex3d(-size, 0, -1.);
		glVertex3d(-gap, 0, -1.);
		glVertex3d(gap, 0, -1.);
		glVertex3d(size, 0, -1.);
		glVertex3d(0, -size, -1.);
		glVertex3d(0, -gap, -1.);
		glVertex3d(0, gap, -1.);
		glVertex3d(0, size, -1.);
		glEnd();
	}

	// Flight path vector (prograde marker)
	if(FLT_EPSILON < this->velo.slen()){
		Vec3d lheading = rot.itrans(this->velo.norm());
		if(0 < fabs(lheading[2])){
			lheading[0] /= -lheading[2];
			lheading[1] /= -lheading[2];
			double hudFov = game->player->fov * maxSize / minSize;
			if(lheading[2] < 0. && -hudFov < lheading[0] && lheading[0] < hudFov && -hudFov < lheading[1] && lheading[1] < hudFov){
				glPushMatrix();
				glTranslated(lheading[0] * hudFov, lheading[1] * hudFov, -1.);
				glScaled(0.02, 0.02, 1.);

				// Draw the circle
				glBegin(GL_LINE_LOOP);
				const double (*cuts)[2] = CircleCuts(16);
				for(int i = 0; i < 16; i++){
					double phase = i * 2 * M_PI / 16;
					glVertex2dv(cuts[i]);
				}
				glEnd();

				// Draw the stabilizers
				glBegin(GL_LINES);
				glVertex2d(0, 1);
				glVertex2d(0, 2);
				glVertex2d(1, 0);
				glVertex2d(3, 0);
				glVertex2d(-1, 0);
				glVertex2d(-3, 0);
				glEnd();

				glPopMatrix();
			}
		}
	}

	{
		const double left = vpLeft + 0.2;
		const double right = left + 0.05;
		const double top = vpBottom + 0.3;
		const double bottom = vpBottom + 0.1;
		const double height = top - bottom;
		const double vcenter = (top + bottom) / 2;

		// Box surrounding throttle indicator
		glBegin(GL_LINE_LOOP);
		glVertex3d(left, top, -1.);
		glVertex3d(left, bottom, -1.);
		glVertex3d(right, bottom, -1.);
		glVertex3d(right, top, -1.);
		glEnd();

		// Indicator for current target throttle setting
		glBegin(GL_LINE_LOOP);
		glVertex3d(left, vcenter + targetThrottle * height / 2, -1.);
		glVertex3d(left - 0.02, vcenter + targetThrottle * height / 2 + 0.01, -1.);
		glVertex3d(left - 0.02, vcenter + targetThrottle * height / 2 - 0.01, -1.);
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex3d(right, vcenter + targetThrottle * height / 2, -1.);
		glVertex3d(right + 0.02, vcenter + targetThrottle * height / 2 + 0.01, -1.);
		glVertex3d(right + 0.02, vcenter + targetThrottle * height / 2 - 0.01, -1.);
		glEnd();

		glBegin(GL_LINES);
		glVertex3d(left, vcenter + targetThrottle * height / 2, -1.);
		glVertex3d(right, vcenter + targetThrottle * height / 2, -1.);
		glEnd();

		// Indicator for actual throttle
		glBegin(GL_QUADS);
		glVertex3d(left, vcenter + throttle * height / 2, -1.);
		glVertex3d(left, vcenter, -1.);
		glVertex3d(right, vcenter, -1.);
		glVertex3d(right, vcenter + throttle * height / 2, -1.);
		glEnd();

		// Speed value readout
		char speed[32];
		sprintf(speed, "%5.1lf m/s", velo.len());
		glPushMatrix();
		glTranslated(left - 0.075, top + 0.01, -1.);
		glScaled(fontSize, -fontSize, .1);
		glwPutTextureString(speed, 16);
		glPopMatrix();
	}

	{
		const double left = vpLeft + 0.4;
		const double right = left + 0.03;
		const double top = vpBottom + 0.3;
		const double bottom = vpBottom + 0.1;
		const double height = top - bottom;

		// Box surrounding fuel meter
		glBegin(GL_LINE_LOOP);
		glVertex3d(left, top, -1.);
		glVertex3d(left, bottom, -1.);
		glVertex3d(right, bottom, -1.);
		glVertex3d(right, top, -1.);
		glEnd();

		// Fuel indicator
		glBegin(GL_QUADS);
		glVertex3d(left, bottom + fuel / maxFuelValue * height, -1.);
		glVertex3d(left, bottom, -1.);
		glVertex3d(right, bottom, -1.);
		glVertex3d(right, bottom + fuel / maxFuelValue * height, -1.);
		glEnd();

		// Fuel readout
		glPushMatrix();
		glTranslated((left + right) / 2. - fontSize * glwGetSizeTextureString("FUEL", 16) / 2., top + 0.01, -1.);
		glScaled(fontSize, -fontSize, .1);
		glwPutTextureString("FUEL", 16);
		glPopMatrix();
	}


	{
		const double left = -vpLeft - 0.3;
		const double bottom = vpBottom + 0.1;
		const double top = bottom + fontSize * 16;
		const double vcenter = (top + bottom) / 2.;
		const double squareSize = fontSize * 16 * .8;
		const double squareMargin = squareSize * 0.2;

		// Flight assist indicator
		glPushMatrix();
		glTranslated(left, bottom, -1.);
		glScaled(fontSize, -fontSize, .1);
		glwPutTextureString("ASSIST", 16);
		glPopMatrix();

		glBegin(flightAssist ? GL_QUADS : GL_LINE_LOOP);
		glVertex3d(left - squareMargin, vcenter + squareSize / 2., -1.);
		glVertex3d(left - squareMargin, vcenter - squareSize / 2., -1.);
		glVertex3d(left - squareMargin - squareSize, vcenter - squareSize / 2., -1.);
		glVertex3d(left - squareMargin - squareSize, vcenter + squareSize / 2., -1.);
		glEnd();
	}

	glPopMatrix();
	glPopAttrib();

}

void Sceptor::drawOverlay(wardraw_t *){
	glCallList(overlayDisp);
}

