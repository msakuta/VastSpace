/** \file
 * \brief Implementation of Soldier class's drawing methods.
 */
#include "../Soldier.h"
#include "Viewer.h"
#include "Player.h"
#include "draw/WarDraw.h"
#include "draw/material.h"
#include "draw/OpenGLState.h"
#include "draw/mqoadapt.h"
#include "glsl.h"
#include "draw/ShadowMap.h"
#include "draw/ShaderBind.h"

extern "C"{
#include <clib/GL/multitex.h>
#include <clib/GL/gldraw.h>
}


#if 0
static void infantry_draw_arms(const char *name, infantry_t *p){

	if(!strcmp(name, "RHand")){
		int i = 1. < p->swapphase ? 1 : 0;
		glPushMatrix();
		glRotated(-90, 0, 1, 0);
		gldScaled(1. / (2./1e5));
		glTranslated(0, 0, -.0002);
		if(arms_static[p->arms[i].type].draw)
			arms_static[p->arms[i].type].draw(&p->arms[i], inf_pixels);
		glPopMatrix();
	}
	else if(!strcmp(name, "Body")){
		int i = 1. < p->swapphase ? 0 : 1;
		glPushMatrix();
		gldScaled(1. / (2./1e5));
		glTranslated(0.0000, 0.00, -.0002);
		glRotated(90, 1, 0, 0);
		glRotated(90, 0, 0, 1);
		glRotated(-30, 1, 0, 0);
		if(arms_static[p->arms[i].type].draw)
			arms_static[p->arms[i].type].draw(&p->arms[i], inf_pixels);
		glPopMatrix();
	}
}
#endif

Model *Soldier::model = NULL;
Motion *Soldier::motions[1] = {NULL};

void Soldier::draw(WarDraw *wd){
	static OpenGLState::weak_ptr<bool> init;
//	static int init = 0;
//	static suf_t *sufbody;
//	static suftex_t *suft_body;
	Soldier *p = this;
//	struct entity_private_static *vft = (struct entity_private_static*)pt->vft;
	double pixels;

//	if(!pt->active)
//		return;

	draw_healthbar(this, wd, health / maxhealth(), getHitRadius(), 0, 0);

	/* cull object */
	if(wd->vw->gc->cullFrustum(pos, .003))
		return;
//	inf_pixels = pixels = .001 * fabs(glcullScale(&pt->pos, wd->pgc));
//	if(pixels < 2)
//		return;
	wd->lightdraws++;


	if(!init){
//		CallCacheBitmap("infantry.bmp", "infantry.bmp", NULL, NULL);
//		CacheSUFTex("infantry.bmp", (BITMAPINFO*)lzuc(lzw_infantry, sizeof lzw_infantry, NULL), 0);
		model = LoadMQOModel("models/Soldier.mqo");
		motions[0] = LoadMotion("models/Soldier_aim.mot");
//		dnm = LoadYSDNM_MQO(/*CvarGetString("valkie_dnm")/*/"infantry2.dnm", "infantry2.mqo");
/*		sufbody = &suf_roughbody *//*LoadSUF("roughbody2.suf")*/;
/*		vft->sufbase = sufbody;*/
/*		CacheSUFTex("face.bmp", (BITMAPINFO*)lzuc(lzw_face, sizeof lzw_face, NULL), 0);*/
/*		if(sufbody)
			suft_body = AllocSUFTex(sufbody);*/
		init.create(*openGLState);
	}
#if 1
	if(!model)
		return;

	// Interpolate motion poses.
	MotionPose mp[1];
	motions[0]->interpolate(mp[0], 10.);

	double scale = modelScale;
	glPushMatrix();

	Mat4d mat;
	transform(mat);
	glMultMatrixd(mat);
	glScaled(-scale, scale, -scale);
	glTranslated(0, 54.4, 0);
#if DNMMOT_PROFILE
	{
		timemeas_t tm;
		TimeMeasStart(&tm);
#endif
		DrawMQOPose(model, mp);
#if DNMMOT_PROFILE
		printf("motdraw %lg\n", TimeMeasLap(&tm));
	}
#endif
//			if(10 < pixels)
//				TransYSDNM_V(dnm, dnmv, infantry_draw_arms, p);
//			g_gldcache.valid = 0;
/*			TransYSDNM(dnm, names, rot, numof(valkie_bonenames), NULL, 0, draw_arms, p);*/
//			glPopAttrib();
	glPopMatrix();
#endif

	if((hookshot || hooked || hookretract) && (!wd->shadowMap || wd->shadowMap->shadowLevel() == 0 || wd->shadowMap->shadowLevel() >= 3)){
		glPushAttrib(GL_LIGHTING_BIT | GL_TEXTURE_BIT);
		const ShaderBind *sb = wd->getShaderBind();
		if(sb)
			glUseProgram(0);
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		if(!glActiveTextureARB)
			MultiTextureInit();
		if(glActiveTextureARB){
			for(int i = 1; i < 5; i++){
				glActiveTextureARB(GL_TEXTURE0_ARB + i);
				glDisable(GL_TEXTURE_2D);
				glActiveTextureARB(GL_TEXTURE0_ARB);
			}
		}
		glColor4f(1,0.5,0,1);
		glBegin(GL_LINES);
		glVertex3dv(mat.vp3(Vec3d(-0.0003, -0.0002, 0.0)));
		glVertex3dv(this->hookpos);
		glEnd();
		if(sb)
			glUseProgram(sb->shader);
		glPopAttrib();
	}

#if 0
	if(!sufbody){
		double pos[3];
		GLubyte col[4] = {255,255,0,255};
		gldPseudoSphere(pos, .015, col);
	}
	else{
		double scale = 1./180000;
		static const GLdouble rotaxis[16] = {
			-1,0,0,0,
			0,0,1,0,
			0,1,0,0,
			0,0,0,1,
		};

/*		if(0 < sun.pos[1])
			ShadowSUF(sufbody, sun.pos, n, pos[i], NULL, scale, &rotaxisd);*/
		glPushMatrix();
		glTranslated(pt->pos[0], pt->pos[1], pt->pos[2]);
		glRotated(deg_per_rad * pt->pyr[1], 0., -1., 0.);
		glRotated(deg_per_rad * pt->pyr[0], -1.,  0., 0.);
		glRotated(deg_per_rad * pt->pyr[2], 0.,  0., -1.);
		glScaled(scale, scale, scale);
		glMultMatrixd(rotaxis);
		DecalDrawSUF(sufbody, SUF_ATR, &g_gldcache, suft_body, NULL, NULL);
		glPopMatrix();
	}
#endif
#if !defined NDEBUG && 0
	{
		int i;
		avec3_t org;
		amat4_t mat;
		struct hitbox *hb = pt->health <= 0. || p->state == STATE_PRONE ? infantry_dead_hb : infantry_hb;
/*		aquat_t rot, roty;
		roty[0] = 0.;
		roty[1] = sin(pt->turrety / 2.);
		roty[2] = 0.;
		roty[3] = cos(pt->turrety / 2.);
		QUATMUL(rot, pt->rot, roty);*/

		glPushMatrix();
/*		gldTranslate3dv(pt->pos);
		gldMultQuat(rot);*/
		tankrot(&mat, pt);
		glMultMatrixd(mat);
/*		gldTranslate3dv(pt->pos);*/
/*		glRotated(pt->turrety * deg_per_rad, 0, 1., 0.);*/
		VECSCALE(org, avec3_001, .001 * (p->state == STATE_STANDING ? p->shiftphase : 1. - p->shiftphase));
		gldTranslate3dv(org);

		for(i = 0; i < numof(infantry_hb); i++){
			amat4_t rot;
			glPushMatrix();
			gldTranslate3dv(hb[i].org);
			quat2mat(rot, hb[i].rot);
			glMultMatrixd(rot);
			hitbox_draw(pt, hb[i].sc);
			glPopMatrix();

			glPushMatrix();
			gldTranslate3dv(hb[i].org);
			quat2imat(rot, hb[i].rot);
			glMultMatrixd(rot);
			hitbox_draw(pt, hb[i].sc);
			glPopMatrix();
		}

		glPopMatrix();
	}
#endif
}

void Soldier::drawOverlay(WarDraw *wd){
	glCallList(overlayDisp);
}

#if 0
static void infantry_gib_draw(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	int i = (int)pv;
	double scale = INFANTRY_SCALE;
	amat4_t mat;
	glPushMatrix();
	glTranslated(pl->pos[0], pl->pos[1], pl->pos[2]);
	glScaled(scale, scale, scale);
	quat2mat(&mat, pl->rot);
	glMultMatrixd(mat);
	DrawSUF(dnm->p[i].suf, SUF_ATR, &g_gldcache);
	glPopMatrix();
}



double perlin_noise_pixel(int x, int y, int bit);
void drawmuzzleflasha(const double (*pos)[3], const double (*org)[3], double rad, const double (*irot)[16]){
	static GLuint texname = 0;
	static const GLfloat envcolor[4] = {1.,1,1,1};
	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
	if(!texname){
		GLubyte texbits[64][64][2];
		struct random_sequence rs;
		int i, j;
		init_rseq(&rs, 3526262);
		for(i = 0; i < 64; i++) for(j = 0; j < 64; j++){
			double x = (i - 32.) / 32., y = (j - 32.) / 32., a, r;
			r = 1.5 * (1. - x * x - y * y + (perlin_noise_pixel(i, j, 3) - .5) * .5);
			texbits[i][j][0] = (GLubyte)rangein(r * 256, 0, 255);
			a = 1.5 * (1. - x * x - y * y + (perlin_noise_pixel(i + 64, j + 64, 3) - .5) * .5);
			texbits[i][j][1] = (GLubyte)rangein(a * 256, 0, 255);
		}
		glGenTextures(1, &texname);
		glBindTexture(GL_TEXTURE_2D, texname);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, 64, 64, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, texbits);
	}
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, /*GL_ADD/*/GL_BLEND);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, envcolor);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1, .5, 0., 1.);
	/*glDisable/*/glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	glDisable(GL_CULL_FACE);
	glBindTexture(GL_TEXTURE_2D, texname);
	glPushMatrix();
	gldTranslate3dv(*pos);
	glMultMatrixd(*irot);
	gldScaled(rad);
	glBegin(GL_QUADS);
	glTexCoord2i(0, 0); glVertex2i(-1, -1);
	glTexCoord2i(1, 0); glVertex2i( 1, -1);
	glTexCoord2i(1, 1); glVertex2i( 1,  1);
	glTexCoord2i(0, 1); glVertex2i(-1,  1);
	glEnd();
	glPopMatrix();
	glPopAttrib();
}

static wardraw_t *s_wd;

static void infantry_drawtra_arms(const char *name, amat4_t *hint){
/*	amat4_t mat, mat2, irot;
	avec3_t pos, pos0 = {0., 0., -.001}, pos1 = {0., 0., -.0013};*/

	if(!strcmp(name, "RHand")){
		static const avec3_t pos = {.13, -.37, 3.};
		amat4_t mat;
		glGetDoublev(GL_MODELVIEW_MATRIX, *hint);
/*		mat4vp3(*hint, mat, pos);*/
/*		int i = 1. < p->swapphase ? 1 : 0;
		gldScaled(1. / INFANTRY_SCALE);
		mat4rotz(irot, s_wd->irot, drseq(&s_wd->w->rs) * 2. * M_PI);
		MAT4IDENTITY(mat);
		mat4vp3(pos, mat, pos0);
		drawmuzzleflasha(&pos, &s_wd->view, .0004, &irot);
		mat4vp3(pos, mat, pos1);
		drawmuzzleflasha(&pos, &s_wd->view, .00025, &irot);*/
	}
}
#endif

#if 0
static void infantry_drawtra(entity_t *pt, wardraw_t *wd){
	infantry_t *p = (infantry_t*)pt;
	struct entity_private_static *vft = (struct entity_private_static *)pt->vft;
	double pixels;

	if(!pt->active)
		return;

	/* cull object */
	if(glcullFrustum(&pt->pos, .003, wd->pgc))
		return;
	pixels = .002 * fabs(glcullScale(&pt->pos, wd->pgc));
	if(pixels < 2)
		return;

#if 1
	if(3 < pixels && p->muzzle){
		avec3_t pos, pos0 = {0., 0., -.001}, pos1 = {0., 0., -.0013};
		amat4_t mat, mat2, gunmat, irot;
		avec3_t muzzlepos;
		double scale = INFANTRY_SCALE;
		ysdnmv_t *dnmv;

		MAT4IDENTITY(gunmat);

		dnmv = infantry_boneset(p);

		s_wd = wd;
		glPushMatrix();
		glLoadIdentity();
		glRotated(180, 0, 1, 0);
		glScaled(-scale, scale, scale);
		glTranslated(0, 54.4, 0);
		TransYSDNM_V(dnm, dnmv, infantry_drawtra_arms, &gunmat);
		MAT4SCALE(gunmat, 1. / scale, 1. / scale, 1. / scale);
/*		VECSCALEIN(&gunmat[12], scale);*/
		glPopMatrix();

/*		gldTranslate3dv(pt->pos);
		glRotated(deg_per_rad * pt->pyr[1], 0., -1., 0.);
		glRotated(deg_per_rad * pt->pyr[0], -1.,  0., 0.);
		glRotated(deg_per_rad * pt->pyr[2], 0.,  0., -1.);
		glScaled(-scale, scale, scale);
		glTranslated(0, 54.4, 0);*/

/*		pos0[1] = pos1[1] = 220. * infantry_s.sufscale;*/
		mat4rotz(irot, wd->irot, drseq(&wd->w->rs) * 2. * M_PI);
		MAT4IDENTITY(mat);
		VECADDIN(&mat[12], pt->pos);
		mat4roty(mat2, mat, -pt->pyr[1]);
		mat4rotx(mat, mat2, pt->pyr[0]);
		mat4mp(mat2, mat, gunmat);
		mat4roty(mat, mat2, -90 / deg_per_rad);
		mat4vp3(pos, mat, pos0);
		drawmuzzleflasha(&pos, &wd->view, .0004, &irot);
		mat4vp3(pos, mat, pos1);
		drawmuzzleflasha(&pos, &wd->view, .00025, &irot);
	}
#else
	if(p->muzzle){
		amat4_t mat, mat2, irot;
		avec3_t pos, pos0 = {0., 0., -.001}, pos1 = {0., 0., -.0013};
		pos0[1] = pos1[1] = 220. * infantry_s.sufscale;
		mat4rotz(irot, wd->irot, drseq(&wd->w->rs) * 2. * M_PI);
		MAT4IDENTITY(mat);
		mat4translate(mat, pt->pos[0], pt->pos[1], pt->pos[2]);
		mat4roty(mat2, mat, -pt->pyr[1]);
		mat4rotx(mat, mat2, pt->pyr[0] + pt->barrelp);
		mat4vp3(pos, mat, pos0);
		drawmuzzleflasha(&pos, &wd->view, .0004, &irot);
		mat4vp3(pos, mat, pos1);
		drawmuzzleflasha(&pos, &wd->view, .00025, &irot);
/*		p->muzzle = 0;*/
	}
#endif

#if 0
	{
		int i;
		double (*cuts)[2];
		avec3_t velo;
		amat4_t rot;
		glPushMatrix();
		glTranslated(pt->pos[0], pt->pos[1], pt->pos[2]);
		pyrmat(pt->pyr, &rot);
		glMultMatrixd(rot);
		glTranslated(vft->hitmdl.bs[0], vft->hitmdl.bs[1], vft->hitmdl.bs[2]);
		pyrimat(pt->pyr, &rot);
		glMultMatrixd(rot);
		glMultMatrixd(wd->irot);
		cuts = CircleCuts(16);
		glColor3ub(255,255,0);
		glBegin(GL_LINE_LOOP);
		for(i = 0; i < 16; i++)
			glVertex3d(cuts[i][0] * vft->hitmdl.rad, cuts[i][1] * vft->hitmdl.rad, 0.);
		glEnd();
		glPopMatrix();
	}
#endif
}

static void infantry_postframe(entity_t *pt){
	if(pt->enemy && !pt->enemy->active)
		pt->enemy = NULL;
}

static void bloodsmoke(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	double pixels;
	double (*cuts)[2];
	double rad;
	amat4_t mat;
	avec3_t velo;
	struct random_sequence rs;
	int i;
	GLubyte col[4] = {255,0,0,255}, colp[4] = {255,0,0,255}, cold[4] = {255,0,0,0};

	if(glcullFrustum(&pl->pos, .0003, dd->pgc))
		return;
	pixels = .0002 * fabs(glcullScale(&pl->pos, dd->pgc));

	if(pixels < 1)
		return;

	col[3] = 128 * pl->life / .5;
	colp[3] = 255 * pl->life / .5 * (1. - 1. / (pixels - 2.));
	BlendLight(col);
	BlendLight(colp);
	BlendLight(cold);

	cuts = CircleCuts(10);
	init_rseq(&rs, (int)pl);
	glPushMatrix();
	glTranslated(pl->pos[0], pl->pos[1], pl->pos[2]);
	glMultMatrixd(dd->invrot);
	rad = (pl->len - .0005 * pl->life * pl->life);
	glScaled(rad, rad, rad);
	glBegin(GL_TRIANGLE_FAN);
	glColor4ubv(col);
	glVertex3d(0., 0., 0.);
	glColor4ubv(cold);
	{
		double first;
		first = drseq(&rs) + .5;
		glVertex3d(first * cuts[0][1], first * cuts[0][0], 0.);
		for(i = 1; i < 10; i++){
			int k = i % 10;
			double r;
			r = drseq(&rs) + .5;
			glVertex3d(r * cuts[k][1], r * cuts[k][0], 0.);
		}
		glVertex3d(first * cuts[0][1], first * cuts[0][0], 0.);
	}
	glEnd();
	glPopMatrix();

	if(pixels < 3)
		return;

	glPushMatrix();
	glTranslated(pl->pos[0], pl->pos[1], pl->pos[2]);
	glGetDoublev(GL_MODELVIEW_MATRIX, mat);

	glBegin(GL_LINES);
	for(i = 0; i < 8; i++){
		avec3_t v0, v1, v2;
		VECCPY(velo, pl->velo);
		velo[0] = (drseq(&rs) - .5) * .0015;
		velo[1] = (drseq(&rs) - .5) * .0015;
		velo[2] = (drseq(&rs) - .5) * .0015;
		v0[0] = (drseq(&rs) - .5) * .00005;
		v0[1] = (drseq(&rs) - .5) * .00005;
		v0[2] = (drseq(&rs) - .5) * .00005;
		VECCPY(v1, v0);
		VECSADD(v1, velo, .5 - pl->life);
		VECCPY(v2, v1);
		VECADDIN(velo, pl->velo);
		VECSADD(v1, velo, .03);
		VECSADD(v2, velo, -.03);
		glColor4ubv(col);
		glVertex3dv(v1);
		glColor4ubv(cold);
		glVertex3dv(v2);
	}
	glEnd();
	glPopMatrix();
}
#endif

void Soldier::drawHUD(WarDraw *wd){
	char buf[128];
	Soldier *p = this;
	glPushMatrix();

	glLoadIdentity();
	{
		GLint vp[4];
		int w, h, m;
		double left, bottom, *pyr;
		int tkills, tdeaths, ekills, edeaths;
		Entity *pt2;
		amat4_t projmat;
		glGetIntegerv(GL_VIEWPORT, vp);
		w = vp[2], h = vp[3];
		m = w < h ? h : w;
		left = -(double)w / m;
		bottom = -(double)h / m;

		glGetDoublev(GL_PROJECTION_MATRIX, projmat);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(left, -left, bottom, -bottom, -1, 1);
		glMatrixMode(GL_MODELVIEW);

		Player *player = this->w->getPlayer();
		if((player->mover == player->cockpitview || player->mover == player->freelook) && !controller){
			if(fmod(this->w->pl->gametime, 1.) < .5){
				glRasterPos3d(.0 - 8. * (sizeof"AI CONTROLLED"-1) / m, .0 - 10. / m, -0.);
				gldprintf("AI CONTROLLED");
			}
		}
		else{
/*			glRasterPos3d(-left - 200. / m, bottom + 60. / m, -1);
			gldprintf("%c sub-machine gun", !pt->weapon ? '>' : ' ');
			glRasterPos3d(-left - 200. / m, bottom + 40. / m, -1);
			gldprintf("%c ???", pt->weapon ? '>' : ' ');*/
		}

		glRasterPos3d(left, -bottom - 20. / m, -0.);
		gldprintf("health: %g", health);
//		glRasterPos3d(left, -bottom - 40. / m, -0.);
//		sprintf(buf, "shots: %d", shoots2);
//		putstring(buf);
//		glRasterPos3d(left, -bottom - 60. / m, -0.);
//		sprintf(buf, "kills: %d", kills);
//		putstring(buf);
//		glRasterPos3d(left, -bottom - 80. / m, -0.);
//		sprintf(buf, "deaths: %d", deaths);
//		putstring(buf);

		glRasterPos3d(left, bottom + 10. / m, -0.);
		gldprintf("cooldown: %g", p->cooldown2);
//		glRasterPos3d(left, bottom + 30. / m, -0.);
//		gldprintf("ammo: %d", p->arms[0]->ammo);
//		glRasterPos3d(left, bottom + 50. / m, -0.);
//		gldprintf("angle: %g", deg_per_rad * this->pyr[1]);

		if(p->damagephase != 0.){
			glColor4f(1., 0., 0., MIN(.5, p->damagephase * .5));
			glBegin(GL_QUADS);
			glVertex3i(-1, -1, -0);
			glVertex3i( 1, -1, -0);
			glVertex3i( 1,  1, -0);
			glVertex3i(-1,  1, -0);
			glEnd();
		}

#if 0
		if(controller){
			glTranslated(0,0,-1);
			glRotated(deg_per_rad * wf->pl->pyr[2], 0., 0., 1.);
			glRotated(deg_per_rad * (wf->pl->pyr[0] + p->pitch), 1., 0., 0.);
			glRotated(deg_per_rad * wf->pl->pyr[1], 0., 1., 0.);
			glTranslated(0,0,1);
/*			glRotated(deg_per_rad * pt->pyr[2], 0., 0., 1.);
			glRotated(deg_per_rad * pt->pyr[0], 1., 0., 0.);
			glRotated(deg_per_rad * pt->pyr[1], 0., 1., 0.);*/
		}
#endif

#if 0
		if(chasecamera == 0 || chasecamera == 3){
			glBegin(GL_LINES);
			glVertex3d(-.15, 0., -0.);
			glVertex3d(-.025, 0., -0.);
			glVertex3d( .15, 0., -0.);
			glVertex3d( .025, 0., -0.);
			glVertex3d(0., -.15, -0.);
			glVertex3d(0., -.025, -0.);
			glVertex3d(0., .15, -0.);
			glVertex3d(0., .025, -0.);
			glEnd();
		}
#endif

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixd(projmat);
		glMatrixMode(GL_MODELVIEW);
	}
	glPopMatrix();
}






void M16::draw(WarDraw *wd){
	static OpenGLState::weak_ptr<bool> init;
	static Model *model = NULL;
	double pixels;

	/* cull object */
	if(wd->vw->gc->cullFrustum(pos, .003))
		return;
	wd->lightdraws++;

	if(!init){
		model = LoadMQOModel("models/m16.mqo");
		init.create(*openGLState);
	}
	if(!model)
		return;

	double scale = Soldier::getModelScale();
	Mat4d mat;
	base->transform(mat);

	if(base->armsGet(0) == this){
		// Obtain gun position from soldier's arm bones.
		MotionPose mp[1];
		Soldier::motions[0]->interpolate(mp[0], 10.);

		Vec3d handpos;
		Quatd handrot;
		Soldier::model->getBonePos("rhand", *mp, &handpos, &handrot);

		glPushMatrix();
		glMultMatrixd(mat);
		glScaled(-scale, scale, -scale);
		gldTranslate3dv(handpos);
		gldMultQuat(handrot);
		glRotated(90, 0, -1, 0);
	//	glTranslated(0, 54.4, 0);
		DrawMQOPose(model, NULL);
		glPopMatrix();
	}
	else{
		glPushMatrix();
		glMultMatrixd(mat);
		glTranslated(0, 0, 0.00025);
		glScaled(-scale, scale, -scale);
		glRotated(30, 0, 0, -1);
		glRotated(90, -1, 0, 0);
		glRotated(90, 0, 0, 1);
	//	glTranslated(0, 54.4, 0);
		DrawMQOPose(model, NULL);
		glPopMatrix();
	}
}


void M40::draw(WarDraw *wd){
	static OpenGLState::weak_ptr<bool> init;
	static Model *model = NULL;
	double pixels;

	/* cull object */
	if(wd->vw->gc->cullFrustum(pos, .003))
		return;
	wd->lightdraws++;

	if(!init){
		model = LoadMQOModel("models/m40.mqo");
		init.create(*openGLState);
	}
	if(!model)
		return;

	double scale = Soldier::getModelScale();
	Mat4d mat;
	base->transform(mat);

	if(base->armsGet(0) == this){
		// Obtain gun position from soldier's arm bones.
		MotionPose mp[1];
		Soldier::motions[0]->interpolate(mp[0], 10.);

		Vec3d handpos;
		Quatd handrot;
		Soldier::model->getBonePos("rhand", *mp, &handpos, &handrot);

		glPushMatrix();
		glMultMatrixd(mat);
		glScaled(-scale, scale, -scale);
		gldTranslate3dv(handpos);
		gldMultQuat(handrot);
		glRotated(90, 0, -1, 0);
	//	glTranslated(0, 54.4, 0);
		DrawMQOPose(model, NULL);
		glPopMatrix();
	}
	else{
		glPushMatrix();
		glMultMatrixd(mat);
		glTranslated(0, 0, 0.00025);
		glScaled(-scale, scale, -scale);
		glRotated(30, 0, 0, -1);
		glRotated(90, -1, 0, 0);
		glRotated(90, 0, 0, 1);
	//	glTranslated(0, 54.4, 0);
		DrawMQOPose(model, NULL);
		glPopMatrix();
	}
}
