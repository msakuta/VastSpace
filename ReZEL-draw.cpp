#include "ReZEL.h"
#include "Player.h"
#include "draw/material.h"
//#include "judge.h"
#include "draw/effects.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/mqoadapt.h"
#include "yssurf.h"
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

const double ReZEL::sufscale = 1./40000;


bool ReZEL::cull(Viewer &vw)const{
	double nf = nlipsFactor(vw);
	if(task == ReZEL::Undockque || vw.gc->cullFrustum(pos, .012 * nf))
		return true;
	double pixels = .008 * fabs(vw.gc->scale(pos)) * nf;
	if(pixels < 2)
		return true;
	return false;
}

/* NLIPS: Non-Linear Inverse Perspective Scrolling */
double ReZEL::nlipsFactor(Viewer &vw)const{
	double f = vw.fov * g_nlips_factor * 500. / vw.vp.m * 4. * ::sqrt((this->pos - vw.pos).len());
	return MAX(1., f);
}

Model *ReZEL::model = NULL;
ysdnm_motion *ReZEL::motions[7];

void ReZEL::getMotionTime(double (*motion_time)[numof(motions)]){
	double motion_time1[numof(motions)] = {
		10. * fwaverider,
		10. * (1. - fwaverider),
		10. * (1. - fwaverider) * (freload == 0. ? 1. - fweapon : 0.),
		10. * (1. - fwaverider) * fweapon,
		(-twist * (1. - fwaverider) + 1.) * 10.,
		(-pitch * (1. - fwaverider) + 1.) * 10.,
		(1. - fwaverider) * (freload != 0. ? min(2. - 2. * freload / reloadTime, 2. * freload / reloadTime) * 20. + 10. : 0.),
	};
	memcpy(*motion_time, motion_time1, sizeof motion_time1);
}

void ReZEL::draw(wardraw_t *wd){
	static OpenGLState::weak_ptr<bool> init;
	double nf = nlipsFactor(*wd->vw);
	double scale = sufscale * nf;
	ReZEL *const p = this;
	if(!this->w /*|| this->docked*/)
		return;

	/* cull object */
	if(cull(*wd->vw))
		return;
	wd->lightdraws++;

	double pixels = .005 * fabs(wd->vw->gc->scale(pos)) * nf;

	draw_healthbar(this, wd, health / maxhealth(), .01 * nf, fuel / maxfuel(), -1.);

	if(!init) do{
/*		for(int i = 0 ; i < numof(models); i++){
			suf[i] = CallLoadSUF(models[i]);
			if(suf[i]){
				vbo[i] = CacheVBO(suf[i]);
				CacheSUFMaterials(suf[i]);
				suft[i] = gltestp::AllocSUFTex(suf[i]);
			}
		}*/
		model = LoadMQOModel("gundam/models/ReZEL.mqo");
		motions[0] = YSDNM_MotionLoad("gundam/models/ReZEL_waverider.mot");
		motions[1] = YSDNM_MotionLoad("gundam/models/ReZEL_stand.mot");
		motions[2] = YSDNM_MotionLoad("gundam/models/ReZEL_aim.mot");
		motions[3] = YSDNM_MotionLoad("gundam/models/ReZEL_aimsub.mot");
		motions[4] = YSDNM_MotionLoad("gundam/models/ReZEL_twist.mot");
		motions[5] = YSDNM_MotionLoad("gundam/models/ReZEL_pitch.mot");
		motions[6] = YSDNM_MotionLoad("gundam/models/ReZEL_reload.mot");

		init.create(*openGLState);
	} while(0);
	
	if(true){
		static const double normal[3] = {0., 1., 0.};
		double x;
		double pyr[3];
		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

		glPushMatrix();
		gldTranslate3dv(this->pos);
		gldMultQuat(this->rot);
		gldScaled(scale);
		glScalef(-1, 1, -1);

#if 1
		double motion_time[numof(motions)];
		getMotionTime(&motion_time);
		ysdnm_var *v = YSDNM_MotionInterpolate(motions, motion_time, numof(motions));
		DrawMQO_V(model, v);
		YSDNM_MotionInterpolateFree(v);
#else
		for(int i = 0; i < numof(models); i++){
//			for(int j = 0; j < 2; j++)
			{
/*				glPushMatrix();
				if(j){
					glScalef(-1, 1, 1);
					glFrontFace(GL_CW);
				}*/
				if(vbo[i])
					DrawVBO(vbo[i], wd->shadowmapping ? 0 : SUF_ATR | SUF_TEX, suft[i]);
				else
					DecalDrawSUF(suf[i], wd->shadowmapping ? 0 : SUF_ATR, NULL, suft[i], NULL, NULL);
//				glPopMatrix();
			}
//			glFrontFace(GL_CCW);
		}
#endif

		glPopMatrix();

/*		if(0 < wd->light[1]){
			static const double normal[3] = {0., 1., 0.};
			ShadowSUF(scepter_s.sufbase, wd->light, normal, pt->pos, pt->pyr, scale, NULL);
		}*/
		glPopAttrib();
	}
}

#define COLIST4(a) COLOR32R(a),COLOR32G(a),COLOR32B(a),COLOR32A(a)

void ReZEL::drawtra(wardraw_t *wd){
	ReZEL *p = this;
	Mat4d mat;
	double nlips = nlipsFactor(*wd->vw);
	double scale = sufscale * nlips;

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

	if(model){
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		Vec3d pos;
		Quatd lrot;
		static const Quatd rotaxis(0, 1., 0., 0.);
		double motion_time[numof(motions)];
		getMotionTime(&motion_time);
		ysdnm_var *v = YSDNM_MotionInterpolate(motions, motion_time, numof(motions));

		if(0 < muzzleFlash[0]){
			model->getBonePos("ReZEL_riflemuzzle", *v, &pos, &lrot);
			pos *= scale;
			pos[0] *= -1;
			pos[2] *= -1;
			pos = rot.trans(pos) + this->pos;
			lrot = rot * rotaxis * lrot;
			gldSpriteGlow(pos, .0015 + (.5 - muzzleFlash[0]) * .0015, Vec4<GLubyte>(127,255,255,min(muzzleFlash[0] / .3 * 255, 255)), wd->vw->irot);
		}

		if(0 < muzzleFlash[1]){
			model->getBonePos("ReZEL_shieldmuzzle", *v, &pos, &lrot);
			pos *= scale;
			pos[0] *= -1;
			pos[2] *= -1;
			pos = rot.trans(pos) + this->pos;
			lrot = rot * rotaxis * lrot;
			gldSpriteGlow(pos, .0010 + (.3 - muzzleFlash[1]) * .001, Vec4<GLubyte>(255,127,255,min(muzzleFlash[1] / .3 * 255, 255)), wd->vw->irot);
		}

		YSDNM_MotionInterpolateFree(v);
		glPopAttrib();
	}

	if(false && p->throttle){
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
			gldScrollTextureBeam(wd->vw->pos, this->pos + this->rot.trans(Vec3d(0,0,30.*scale)), this->pos + this->rot.trans(Vec3d(0,0,30.*scale+.010*amp)), .0025*amp, tim + 100. * rs.nextd());
			gldScrollTextureBeam(wd->vw->pos, this->pos + this->rot.trans(Vec3d( 34.5*scale,0,40.*scale)), this->pos + this->rot.trans(Vec3d( 34.5*scale,0,40.*scale+.005*amp)), .00125*amp, tim + 100. * rs.nextd());
			gldScrollTextureBeam(wd->vw->pos, this->pos + this->rot.trans(Vec3d(-34.5*scale,0,40.*scale)), this->pos + this->rot.trans(Vec3d(-34.5*scale,0,40.*scale+.005*amp)), .00125*amp, tim + 100. * rs.nextd());
		}
		else{
			double ofs = 5.*scale;
			pos = this->pos + this->rot.trans(Vec3d(0,0,35.*scale));
			Quatd upangle = this->rot * Quatd(sin(M_PI*5./6./2.),0.,0.,cos(M_PI*5./6./2.));
			Quatd dnangle = this->rot * Quatd(sin(-M_PI*5./6./2.),0.,0.,cos(-M_PI*5./6./2.));
			gldScrollTextureBeam(wd->vw->pos, pos + upangle.trans(Vec3d( 34.5*scale,0,ofs)), pos + upangle.trans(Vec3d( 34.5*scale,0,ofs+.005*amp)), .00125*amp, tim + 100. * rs.nextd());
			gldScrollTextureBeam(wd->vw->pos, pos + dnangle.trans(Vec3d( 34.5*scale,0,ofs)), pos + dnangle.trans(Vec3d( 34.5*scale,0,ofs+.005*amp)), .00125*amp, tim + 100. * rs.nextd());
			gldScrollTextureBeam(wd->vw->pos, pos + upangle.trans(Vec3d(-34.5*scale,0,ofs)), pos + upangle.trans(Vec3d(-34.5*scale,0,ofs+.005*amp)), .00125*amp, tim + 100. * rs.nextd());
			gldScrollTextureBeam(wd->vw->pos, pos + dnangle.trans(Vec3d(-34.5*scale,0,ofs)), pos + dnangle.trans(Vec3d(-34.5*scale,0,ofs+.005*amp)), .00125*amp, tim + 100. * rs.nextd());
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

#if 0
	if(false && mf) for(int i = 0; i < 2; i++){
		Vec3d pos = rot.trans(Vec3d(gunPos[i])) * nlips + this->pos;
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		glCallList(muzzle_texture());
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
#endif
}

void ReZEL::drawOverlay(wardraw_t *){
	glScaled(10, 10, 1);
	glBegin(GL_LINE_LOOP);
	glVertex2d(-.10, -.10);
	glVertex2d(-.05,  .00);
	glVertex2d(-.10,  .10);
	glVertex2d( .00,  .05);
	glVertex2d( .10,  .10);
	glVertex2d( .05,  .00);
	glVertex2d( .10, -.10);
	glVertex2d( .00, -.05);
	glEnd();
}

void ReZEL::smokedraw(const struct tent3d_line_callback *p, const struct tent3d_line_drawdata *dd, void *private_data){
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

