/** \file
 * \brief Implementation of Soldier class's drawing methods.
 */
#include "../Soldier.h"
#include "Viewer.h"
#include "Player.h"
#include "Game.h"
#include "Bullet.h"
#include "draw/WarDraw.h"
#include "draw/material.h"
#include "draw/OpenGLState.h"
#include "draw/mqoadapt.h"
#include "glsl.h"
#include "draw/ShadowMap.h"
#include "draw/ShaderBind.h"
#include "draw/effects.h"

extern "C"{
#include <clib/GL/multitex.h>
#include <clib/GL/gldraw.h>
#include <clib/cfloat.h>
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
Motion *Soldier::motions[2] = {NULL};

void Soldier::draw(WarDraw *wd){
	Soldier *p = this;
	double pixels;

	draw_healthbar(this, wd, health / maxhealth(), getHitRadius(), 0, 0);

	// Draw hook and tether before culling
	drawHookAndTether(wd);

	/* cull object */
	if(wd->vw->gc->cullFrustum(pos, getHitRadius()))
		return;
//	inf_pixels = pixels = .001 * fabs(glcullScale(&pt->pos, wd->pgc));
//	if(pixels < 2)
//		return;
	wd->lightdraws++;

	if(!initModel())
		return;

	// Interpolate motion poses.
	MotionPose mp[1];
	interpolate(mp);

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
	glPopMatrix();

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

void Soldier::drawHookAndTether(WarDraw *wd){
	if((hookshot || hooked || hookretract) && (!wd->shadowMap || wd->shadowMap->shadowLevel() == 0 || wd->shadowMap->shadowLevel() >= 3)){

		static OpenGLState::weak_ptr<bool> init;
		static Model *hookModel = NULL;
		if(!init){
			hookModel = LoadMQOModel(modPath() << "models/hook.mqo");
			CallCacheBitmap("tether.jpg", modPath() << "models/tether.jpg", NULL, NULL);
			init.create(*openGLState);
		}

		if(!hookModel)
			return;

		Mat4d mat;
		transform(mat);
		Vec3d src = mat.vp3(Vec3d(-0.0001, -0.00005, 0.0));
		Vec3d dest = hooked && hookedEntity ? hookedEntity->pos + hookedEntity->rot.trans(this->hookpos) : this->hookpos;
		Quatd direcRot = Quatd::direction(dest - src);

		// Draw hook head model
		double scale = 5e-7;
		Mat4d hookMat = direcRot.tomat4();
		hookMat.vec3(3) = dest;
		glPushMatrix();
		glMultMatrixd(hookMat);
		glScaled(scale, scale, scale);
		DrawMQOPose(hookModel, NULL);
		glPopMatrix();

		// Offset 75mm
		Vec3d delta = src - dest;
		if(75e-6 * 75e-6 < delta.slen())
			dest += delta.norm() * 75e-6;

		// Load texture for tether rope
		glPushAttrib(GL_LIGHTING_BIT | GL_TEXTURE_BIT | GL_POLYGON_BIT);
		const gltestp::TexCacheBind *tcb = gltestp::FindTexture("tether.jpg");
		if(tcb)
			glCallList(tcb->getList());

		// Draw tether rope
		glBegin(GL_QUAD_STRIP);
		for(int i = 0; i <= 5; i++){
			Vec3d circle = 0.00001 * Vec3d(sin(i * 2 * M_PI / 5), cos(i * 2 * M_PI / 5), 0);
			glNormal3dv(direcRot.trans(circle));
			glTexCoord2d((src - dest).len() / (0.0001 * M_PI), i / 5.);
			glVertex3dv(src + direcRot.trans(circle));
			glTexCoord2d(0, i / 5.);
			glVertex3dv(dest + direcRot.trans(circle));
		}
		glEnd();

		glPopAttrib();
	}
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


#endif

static double noise_pixel(int x, int y, int bit){
	struct random_sequence rs;
	initfull_rseq(&rs, x + (bit << 16), y);
	return drseq(&rs);
}

double perlin_noise_pixel(int x, int y, int bit){
	int ret = 0, i;
	double sum = 0., maxv = 0., f = 1.;
	double persistence = 0.5;
	for(i = 3; 0 <= i; i--){
		int cell = 1 << i;
		double a00, a01, a10, a11, fx, fy;
		a00 = noise_pixel(x / cell, y / cell, bit);
		a01 = noise_pixel(x / cell, y / cell + 1, bit);
		a10 = noise_pixel(x / cell + 1, y / cell, bit);
		a11 = noise_pixel(x / cell + 1, y / cell + 1, bit);
		fx = (double)(x % cell) / cell;
		fy = (double)(y % cell) / cell;
		sum += ((a00 * (1. - fx) + a10 * fx) * (1. - fy)
			+ (a01 * (1. - fx) + a11 * fx) * fy) * f;
		maxv += f;
		f *= persistence;
	}
	return sum / maxv;
}

void drawmuzzleflasha(const Vec3d &pos, const Vec3d &org, double rad, const Mat4d &irot){
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
	gldTranslate3dv(pos);
	glMultMatrixd(irot);
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

#if 0
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

#if 1
void Soldier::drawtra(WarDraw *wd){
	Soldier *p = this;

	if(!model || !motions[0])
		return;

	/* cull object */
	if(wd->vw->gc->cullFrustum(pos, getHitRadius()))
		return;
	double pixels = .002 * fabs(wd->vw->gc->scale(pos));
	if(pixels < 2)
		return;

#if 1
	if(3 < pixels && p->muzzle){

		// Interpolate motion poses.
		MotionPose mp[1];
		interpolate(mp);

		Vec3d pos;
		Quatd rot;
		model->getBonePos("rhand", mp[0], &pos, &rot);

		Mat4d mat;
		transform(mat);
		mat.vec3(0) *= -1;
		mat.vec3(2) *= -1;
		Mat4d gunmat = rot.tomat4();
		gunmat.vec3(3) = pos * modelScale;
		Mat4d totalMat = mat * gunmat;
		for(int i = 0; i < 2; i++)
			drawmuzzleflasha(totalMat.vp3(muzzleFlashOffset[i]), wd->vw->pos, muzzleFlashRadius[i], wd->vw->irot);
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
#endif

const double bloodSmokeLife = 1.;

static void bloodsmoke(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	if(dd->pgc->cullFrustum(pl->pos, pl->len))
		return;
	double pixels = .0002 * fabs(dd->pgc->scale(pl->pos));
	if(pixels < 1)
		return;

	GLubyte col[4] = {255,0,0,255}, colp[4] = {255,0,0,255}, cold[4] = {255,0,0,0};
	col[3] = 128 * pl->life / bloodSmokeLife;
	colp[3] = 255 * pl->life / bloodSmokeLife * (1. - 1. / (pixels - 2.));
//	BlendLight(col);
//	BlendLight(colp);
//	BlendLight(cold);

	double (*cuts)[2] = CircleCuts(10);
	RandomSequence rs((int)pl);
	glPushMatrix();
	gldTranslate3dv(pl->pos);
	glMultMatrixd(dd->invrot);
	double rad = (pl->len - .0005 * pl->life * pl->life);
	gldScaled(rad);
	glBegin(GL_TRIANGLE_FAN);
	glColor4ubv(col);
	glVertex3d(0., 0., 0.);
	glColor4ubv(cold);
	{
		double first = rs.nextd() + .5;
		glVertex3d(first * cuts[0][1], first * cuts[0][0], 0.);
		for(int i = 1; i < 10; i++){
			int k = i % 10;
			double r = rs.nextd() + .5;
			glVertex3d(r * cuts[k][1], r * cuts[k][0], 0.);
		}
		glVertex3d(first * cuts[0][1], first * cuts[0][0], 0.);
	}
	glEnd();
	glPopMatrix();

	if(pixels < 3)
		return;

	glPushMatrix();
	gldTranslate3dv(pl->pos);
	Mat4d mat;
	glGetDoublev(GL_MODELVIEW_MATRIX, mat);

	glBegin(GL_LINES);
	for(int i = 0; i < 8; i++){
		Vec3d velo;
		for(int j = 0; j < 3; j++)
			velo[j] = (rs.nextd() - .5) * .0015;
		Vec3d v0;
		for(int j = 0; j < 3; j++)
			v0[j] = (rs.nextd() - .5) * .0015;
		Vec3d v1 = v0 + velo * (bloodSmokeLife - pl->life);
		velo += pl->velo;
		Vec3d v2 = v1 + velo * -.03;
		v1 += velo * .03;
		glColor4ubv(col);
		glVertex3dv(v1);
		glColor4ubv(cold);
		glVertex3dv(v2);
	}
	glEnd();
	glPopMatrix();
}

void Soldier::bullethit(const Bullet *pb){
	if(w->getTeline3d()){
		Vec3d pos = pb->pos;
		Vec3d velo = pb->velo.norm() * 0.005 + this->velo;
		for(int i = 0; i < 3; i++)
			pos[i] += (drseq(&w->rs) - .5) * .0005;
		AddTelineCallback3D(w->getTeline3d(), pos, velo, .0010, quat_u, vec3_000, w->accel(pos, velo), bloodsmoke, NULL, 0, 1.);
	}
}

void Soldier::drawHUD(WarDraw *wd){
	char buf[128];
	Soldier *p = this;
	glPushMatrix();

	Player *player = game->player;
	glLoadIdentity();

	// If we have no Player, almost nothing can be drawn.
	if(player){
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

		if((player->mover == player->cockpitview || player->mover == player->freelook) && !controller){
			if(fmod(this->w->pl->gametime, 1.) < .5){
				glRasterPos3d(.0 - 8. * (sizeof"AI CONTROLLED"-1) / m, .0 - 10. / m, -0.);
				gldprintf("AI CONTROLLED");
			}
		}
		else if(arms[0]){
			// Display primary weapon name and ammunition count.
			glRasterPos3d(-left - 200. / m, 0/*bottom + 60. / m*/, -1);
			gldprintf("%s", arms[0]->classname());
			glRasterPos3d(-left - 200. / m, - 12. / m, -1);
			gldprintf("%d / %d", arms[0]->ammo, arms[0]->maxammo());
//			glRasterPos3d(-left - 200. / m, bottom + 40. / m, -1);
//			gldprintf("%c ???", this->weapon ? '>' : ' ');
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

		// Draw crosshair
		if(player->mover == player->cockpitview && (player->getChaseCamera() == 0 || player->getChaseCamera() == 3)){
			double precision = 2. * (arms[0] ? arms[0]->bulletVariance() / getFov() : 0.001);
			glBegin(GL_LINES);
			glVertex3d(-.15 - precision, 0., -0.);
			glVertex3d(     - precision, 0., -0.);
			glVertex3d( .15 + precision, 0., -0.);
			glVertex3d(       precision, 0., -0.);
			glVertex3d(0., -.15 - precision, -0.);
			glVertex3d(0.,      - precision, -0.);
			glVertex3d(0.,  .15 + precision, -0.);
			glVertex3d(0.,        precision, -0.);
			glEnd();
		}

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixd(projmat);
		glMatrixMode(GL_MODELVIEW);
	}
	glPopMatrix();
}

void Soldier::hookHitEffect(const otjEnumHitSphereParam &param){
	if(!game->isServer()){
		WarSpace *ws = *w;
		RandomSequence rs(clock());
		Vec3d accel = w->accel(*param.pos, Vec3d(0,0,0));
		int n = 12;

		// Add spark sprite
		{
			double angle = rs.nextd() * 2. * M_PI / 2.;
			AddTelineCallback3D(ws->tell, *param.pos,
				hookedEntity ? hookedEntity->velo + hookedEntity->omg.vp(hookedEntity->rot.trans(hookpos)) : vec3_000,
				.00075, Quatd(0, 0, sin(angle), cos(angle)),
				vec3_000, accel, sparkspritedraw, NULL, 0, .20 + rs.nextd() * .20);
		}

		// Add spark traces
		for(int j = 0; j < n; j++){
			Vec3d velo = -hookvelo.norm() * .015;
			for(int k = 0; k < 3; k++)
				velo[k] += .03 * (rs.nextd() - .5);
			AddTelineCallback3D(ws->tell, *param.pos, velo, .000025 + n * .00001, quat_u, vec3_000, accel, sparkdraw, NULL, TEL3_HEADFORWARD | TEL3_REFLECT, .20 + rs.nextd() * .20);
		}
	}
}

/// \brief Loads and initializes model and motions.
/// \returns True if initialized or already initialized, false if something fail to initialize.
///
/// This process is completely for display, so defined in this Sldier-draw.cpp.
bool Soldier::initModel(){
	static OpenGLState::weak_ptr<bool> init;

	if(!init){
		model = LoadMQOModel(modPath() << "models/Soldier.mqo");
		motions[0] = LoadMotion(modPath() << "models/Soldier_aim.mot");
		motions[1] = LoadMotion(modPath() << "models/Soldier_reload.mot");
		init.create(*openGLState);
	}

	return model && motions[0] && motions[1];
}

/// \brief Returns interpolated MotionPose based on the status of current object.
/// \param mp returning MotionPose is stored in the object pointed to by this pointer.
/// \returns True if succeeded, false if interpolation fails.
bool Soldier::interpolate(MotionPose *mp){
	if(!initModel())
		return false;

	// Interpolate motion poses.
	if(reloading)
		motions[1]->interpolate(mp[0], (reloadphase < 1.25 ? reloadphase / 1.25 : (2.5 - reloadphase) / 1.25) * 20.);
	else
		motions[0]->interpolate(mp[0], 10.);
	return true;
}

/// \brief Returns gun position and orientation.
/// \param ggp GetGunPosCommand EntityCommand object. The results are stored in the members of this object.
/// \returns True if succeeded, false if the position couldn't be determined.
///
/// Returned position and orientation are in the world coordinates.
bool Soldier::getGunPos(GetGunPosCommand &ggp){
	MotionPose mp[1];
	if((ggp.gunId == 0 || ggp.gunId == 1) && interpolate(mp)){
		Vec3d srcpos;
		Quatd srcrot; 
		Soldier::model->getBonePos(ggp.gunId == 0 ? "rhand" : "body", *mp, &srcpos, &srcrot);
		Mat4d mat;
		transform(mat);
		srcpos *= modelScale;
		if(ggp.gunId == 0){
			srcpos += srcrot.trans(Vec3d(-0.00015, 0.00015, 0.0));
			srcrot = Quatd(0, 1, 0, 0) * srcrot.rotate(M_PI / 2., 0, 1, 0);
		}
		else{
			srcpos += srcrot.trans(Vec3d(0, 0, -0.00025));
			srcrot = Quatd(0, 1, 0, 0) * srcrot;
			srcrot = srcrot.rotate(M_PI / 6., 0, 0, -1);
			srcrot = srcrot.rotate(M_PI / 2., -1, 0, 0);
			srcrot = srcrot.rotate(M_PI / 2., 0, 0, 1);
		}
		srcpos[0] *= -1;
		srcpos[2] *= -1;
		ggp.pos = mat.vp3(srcpos);
		ggp.rot = rot * srcrot;
		return true;
	}
	else
		return false;
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
		model = LoadMQOModel(modPath() << "models/m16.mqo");
		init.create(*openGLState);
	}
	if(!model)
		return;

	double scale = Soldier::getModelScale();

	// Retrieve gun id
	int gunId = 0;
	for(int i = 0; i < base->armsCount(); i++){
		if(base->armsGet(i) == this){
			gunId = i;
			break;
		}
	}

	// Send GetGunPosCommand to base Entity to query transformation for held weapons.
	GetGunPosCommand ggp(gunId);
	if(base->command(&ggp)){
		glPushMatrix();
		gldTranslate3dv(ggp.pos);
		gldMultQuat(ggp.rot);
		glScaled(-scale, scale, -scale);
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
		model = LoadMQOModel(modPath() << "models/m40.mqo");
		init.create(*openGLState);
	}
	if(!model)
		return;

	double scale = Soldier::getModelScale();

	// Retrieve gun id
	int gunId = 0;
	for(int i = 0; i < base->armsCount(); i++){
		if(base->armsGet(i) == this){
			gunId = i;
			break;
		}
	}

	// Send GetGunPosCommand to base Entity to query transformation for held weapons.
	GetGunPosCommand ggp(gunId);
	if(base->command(&ggp)){
		glPushMatrix();
		gldTranslate3dv(ggp.pos);
		gldMultQuat(ggp.rot);
		glScaled(-scale, scale, -scale);
		DrawMQOPose(model, NULL);
		glPopMatrix();
	}
}
