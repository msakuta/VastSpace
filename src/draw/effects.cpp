/** \file
 * \brief Implemetation of graphical effects.
 */
#include "draw/effects.h"
#include "bitmap.h"
#include "draw/material.h"
#include "draw/mqoadapt.h"
#include "tent3d.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/suf/sufbin.h>
#include <clib/suf/sufdraw.h>
#include <clib/suf/sufvbo.h>
#include <clib/GL/gldraw.h>
#include <clib/GL/multitex.h>
#include <clib/rseq.h>
}
#include <cpplib/RandomSequence.h>
#include <math.h>
#include <gl/glext.h>

void sparkdraw(const Teline3CallbackData *p, const Teline3DrawData *dd, void *private_data){
	double width = p->len;
	Vec3d start = p->pos;
	Vec3d end = p->pos;
	end += p->velo / 10. /** -(runlength / velolen < length ? runlength / velolen : length)*/;
	double pixels = fabs(dd->pgc->scale(p->pos)) * width;
	float alpha = MIN(1., p->life / .25);
	if(pixels < 2.){
		glBegin(GL_LINES);
		glColor4f(1., 1., .5, 0.);
		glVertex3dv(start);
		glColor4f(1., 1., .5, pixels / 2. * alpha);
		glVertex3dv(end);
		glEnd();
		return;
	}
	static GLuint texname = 0;
	static const GLfloat envcolor[4] = {.5,0,0,1};
	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
	if(!texname){
		suftexparam_t stp;
		stp.flags = STP_ENV | STP_MAGFIL | STP_MINFIL;
		stp.env = GL_MODULATE;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		texname = CallCacheBitmap5("textures/ricochet.bmp", "textures/ricochet.bmp", &stp, NULL, NULL);
	}
	glCallList(texname);
	glColor4f(1,1,1, alpha);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Add blend
	gldTextureBeam(dd->viewpoint, end, start, width);
	glPopAttrib();
}

void sparkspritedraw(const Teline3CallbackData *p, const Teline3DrawData *dd, void *private_data){
	double width = p->len;
	Vec3d start = p->pos;
	double pixels = fabs(dd->pgc->scale(p->pos)) * width;
	float alpha = MIN(1., p->life / .25);
	if(pixels < 2.){
		return;
	}
	static GLuint texname = 0;
	static const GLfloat envcolor[4] = {.5,0,0,1};
	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
	if(!texname){
		suftexparam_t stp;
		stp.flags = STP_ENV | STP_MAGFIL | STP_MINFIL;
		stp.env = GL_MODULATE;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		texname = CallCacheBitmap5("textures/spark.bmp", "textures/spark.bmp", &stp, NULL, NULL);
	}
	glCallList(texname);
	glColor4f(1,1,1, alpha);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Add blend
	glPushMatrix();
	gldTranslate3dv(p->pos);
	gldScaled(p->len);
	glMultMatrixd(dd->invrot);
	gldMultQuat(p->rot);
	glBegin(GL_QUADS);
	glTexCoord2i(0, 0); glVertex2i(-1,-1);
	glTexCoord2i(1, 0); glVertex2i( 1,-1);
	glTexCoord2i(1, 1); glVertex2i( 1, 1);
	glTexCoord2i(0, 1); glVertex2i(-1, 1);
	glEnd();
	glPopMatrix();
	glPopAttrib();
}

static void smokedraw_int(const Teline3CallbackData *p, COLOR32 col, float alpha){
	static GLuint list = 0;
	if(!list){
		suftexparam_t stp;
		stp.flags = STP_ENV | STP_ALPHA | STP_ALPHATEX | STP_MAGFIL | STP_MINFIL;
		stp.env = GL_MODULATE;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		list = CallCacheBitmap5("textures/smoke2.jpg", "textures/smoke2.jpg", &stp, NULL, NULL);
	}
	glCallList(list);
	glColor4f(COLOR32R(col) / 255., COLOR32G(col) / 255., COLOR32B(col) / 255., MIN(p->life * .25, 1.));
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, Vec4<float>(.5f, .5f, .5f, alpha));
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, Vec4<float>(.5f, .5f, .5f, alpha));
/*	glBegin(GL_QUADS);
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


void smokedraw(const Teline3CallbackData *p, const Teline3DrawData *dd, void *private_data){
	glPushMatrix();
	gldTranslate3dv(p->pos);
//	glMultMatrixd(dd->invrot);
	{
		Quatd rot;
		rot = dd->rot;
		Vec3d delta = dd->viewpoint - p->pos;
		Vec3d ldelta = rot.trans(delta);
		Quatd qirot = Quatd::direction(ldelta);
		Quatd qret = rot.cnj() * qirot;
		gldMultQuat(qret);
	}
	gldScaled(p->len);
	struct random_sequence rs;
	init_rseq(&rs, (long)p);
	glRotated(rseq(&rs) % 360, 0, 0, 1);
//	gldMultQuat(Quatd::direction(Vec3d(p->pos) - Vec3d(dd->viewpoint)));
	glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT);
	float alpha = MIN(p->life * .25, 1.);
	smokedraw_int(p, (COLOR32)private_data, alpha);
	glPopAttrib();
	glPopMatrix();
}

void smokedraw_swirl(const Teline3CallbackData *p, const Teline3DrawData *dd, void *private_data){
	const smokedraw_swirl_data *data = (const smokedraw_swirl_data*)private_data;
	glPushMatrix();
	gldTranslate3dv(p->pos);
//	glMultMatrixd(dd->invrot);
	{
		Quatd rot;
		rot = dd->rot;
		Vec3d delta = dd->viewpoint - p->pos;
		Vec3d ldelta = rot.trans(delta);
		Quatd qirot = Quatd::direction(ldelta);
		Quatd qret = rot.cnj() * qirot;
		struct random_sequence rs;
		init_rseq(&rs, (unsigned long)p);
		double randy = p->omg[2] * -p->life + 2. * M_PI * (drseq(&rs) - .5);
		gldMultQuat(qret * Quatd(0, 0, sin(randy), cos(randy)));
	}
	if(data->expand)
		gldScaled((2. - p->life) * p->len);
	else
		gldScaled(p->len);
	glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT);
	float alpha = MIN(p->life / 1.5, 1.);
	smokedraw_int(p, data->col, alpha);
	glPopAttrib();
	glPopMatrix();
}

void firesmokedraw(const Teline3CallbackData *p, const Teline3DrawData *dd, void *private_data){
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

static suf_t *sufs[5] = {NULL};
static suftex_t *suftexs[5] = {NULL};
//static VBO *vbo[5];
//static GLuint lists[numof(sufs)] = {0};
//static Model *model = NULL;

static void debrigib_init(){
	if(!sufs[0]){
		char buf[64];
		for(int i = 0; i < numof(sufs); i++){
			sprintf(buf, "models/debris%d.bin", i);
			sufs[i] = CallLoadSUF(buf);
			suftexs[i] = gltestp::AllocSUFTex(sufs[i]);
//			vbo[i] = CacheVBO(sufs[i]);
		}
//		model = LoadMQOModel("models/debris.mqo");
	}
}

void debrigib(const Teline3CallbackData *pl, const Teline3DrawData *dd, void *pv){
	if(dd->pgc && (dd->pgc->cullFrustum(pl->pos, .01) || (dd->pgc->scale(pl->pos) * .01) < 5))
		return;

	debrigib_init();
	glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
//	glBindTexture(GL_TEXTURE_2D, texname);
	glPushMatrix();
	gldTranslate3dv(pl->pos);
	gldMultQuat(pl->rot);
	gldScaled(.0001);
	struct random_sequence rs;
	initfull_rseq(&rs, 13230354, (unsigned long)pl);
	unsigned id = rseq(&rs) % numof(sufs);
//	glUseProgram(0);
//	if(!lists[id]){
//		glNewList(lists[id] = glGenLists(1), GL_COMPILE_AND_EXECUTE);
//		DrawSUF(sufs[id], SUF_ATR, NULL);
		DecalDrawSUF(sufs[id], SUF_ATR, NULL, suftexs[id], NULL, NULL);
//		DrawVBO(vbo[id], SUF_ATR, NULL);
//		DrawMQOPose(model, NULL);
//		glEndList();
//	}
//	else
//		glCallList(lists[id]);
	glPopMatrix();
	glPopAttrib();
}

// Multiple gibs drawn at once. Total number of polygons remains same, but switchings of context variables are
// reduced, therefore performance is gained compared to the one above.
void debrigib_multi(const Teline3CallbackData *pl, const Teline3DrawData *dd, void *pv){
	debrigib_init();
	glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
//	glBindTexture(GL_TEXTURE_2D, texname);
	for(int i = 0; i < (int)pv; i++){
		RandomSequence rs(pl + i);
		double t = 20. - pl->life;
		Vec3d pos = pl->pos
			+ .100 * Vec3d(rs.nextGauss(), rs.nextGauss(), rs.nextGauss()) // origin
			+ t * .025 * Vec3d(rs.nextGauss(), rs.nextGauss(), rs.nextGauss()); // linear motion
		if(dd->pgc && (dd->pgc->cullFrustum(pos, .01) || (dd->pgc->scale(pos) * .01) < 5) || pl->life < rs.nextd() * 2.)
			continue;
		glPushMatrix();
		gldTranslate3dv(pos);
		double angle = t * 2. * M_PI * rs.nextd() / 2.;
		Quatd rot = sin(angle) * Vec3d(rs.nextGauss(), rs.nextGauss(), rs.nextGauss());
		rot[3] = cos(angle);
		gldMultQuat(pl->rot * rot);
		gldScaled(.0001);
		unsigned id = rs.next() % numof(sufs);
		// Let's think out better way...
//		DrawVBO(vbo[id], SUF_ATR, NULL);
		glPopMatrix();
	}
	glPopAttrib();
}

GLuint muzzle_texture(){
	static GLuint texname = 0;
	static bool init = false;
	if(!init){
		init = true;
		suftexparam_t stp;
		stp.flags = STP_ENV | STP_MAGFIL | STP_MINFIL | STP_WRAP_S;
		stp.env = GL_MODULATE;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		stp.wraps = GL_CLAMP_TO_BORDER;
		texname = CallCacheBitmap5("textures/muzzle.bmp", "textures/muzzle.bmp", &stp, NULL, NULL);
	}
	return texname;
}

/// Extended version of gldTextureBeam with multitexturing.
void gldScrollTextureBeam(const Vec3d &view, const Vec3d &start, const Vec3d &end, double width, double offset){
	double dx, dy, dz, sx, sy, len;
	double pitch, yaw;
	Vec3d xh, yh;
	dy = end[1] - start[1];
	yaw = atan2(dx = end[0] - start[0], dz = end[2] - start[2]) + M_PI;
	pitch = -atan2(dy, sqrt(dx * dx + dz * dz)) + M_PI / 2;
	len = sqrt(dx * dx + dy * dy + dz * dz);
	xh[0] = -cos(yaw);
	xh[1] = 0;
	xh[2] = sin(yaw);
	yh[0] = cos(pitch) * sin(yaw);
	yh[1] = sin(pitch);
	yh[2] = cos(pitch) * (cos(yaw));
	Vec3d diff = view - start;
	glPushMatrix();
	glTranslated(start[0], start[1], start[2]);
	sy = diff.sp(yh);
	sx = diff.sp(xh);
	glRotated(yaw * 360 / 2. / M_PI, 0., 1., 0.);
	glRotated(pitch * 360 / 2. / M_PI, -1., 0., 0.);
	glRotated(atan2(sx, sy) * 360 / 2 / M_PI, 0., -1., 0.);
	glScaled(width, len, 1.);
	glBegin(GL_QUADS);
	glMultiTexCoord2dARB(GL_TEXTURE1_ARB, 0., offset + 0.); glTexCoord2d(0., 0.); glVertex3d(-1., 0., 0.);
	glMultiTexCoord2dARB(GL_TEXTURE1_ARB, 0., offset + 1.); glTexCoord2d(0., 1.); glVertex3d(-1., 1., 0.);
	glMultiTexCoord2dARB(GL_TEXTURE1_ARB, 1., offset + 1.); glTexCoord2d(1., 1.); glVertex3d( 1., 1., 0.);
	glMultiTexCoord2dARB(GL_TEXTURE1_ARB, 1., offset + 0.); glTexCoord2d(1., 0.); glVertex3d( 1., 0., 0.);
	glEnd();
	glPopMatrix();
}

void drawmuzzleflash4(const Vec3d &pos, const Mat4d &rot, double rad, const Mat4d &irot, struct random_sequence *prs, const Vec3d &viewer){
	int i;
	glPushMatrix();
	gldTranslate3dv(pos);
/*	glRotated(deg_per_rad * (*pyr)[1], 0., -1., 0.);
	glRotated(deg_per_rad * (*pyr)[0], 1., 0., 0.);
	glRotated(deg_per_rad * (*pyr)[2], 0., 0., -1.);*/
	glMultMatrixd(rot);
	glScaled(rad, rad, rad);
	for(i = 0; i < 4; i++){
		Vec3d pz;
		glRotated(90, 0., 0., 1.);
		glBegin(GL_TRIANGLE_FAN);
		glColor4ub(255,255,31,255);
		pz[0] = drseq(prs) * .25 + .5;
		pz[1] = drseq(prs) * .3 - .15;
		pz[2] = -drseq(prs) * .25;
		glVertex3dv(pz);
		glVertex3d(0., 0., 0.);
		glColor4ub(255,0,0,0);
		pz[0] = drseq(prs) * .25 + .5;
		pz[1] = drseq(prs) * .15 + .15;
		pz[2] = -drseq(prs) * .25;
		glVertex3dv(pz);
		pz[0] = drseq(prs) * .25 + .75;
		pz[1] = drseq(prs) * .3 - .15;
		pz[2] = -drseq(prs) * .15 - .25;
		glVertex3dv(pz);
		pz[0] = drseq(prs) * .25 + .5;
		pz[1] = -drseq(prs) * .15 - .15;
		pz[2] = -drseq(prs) * .25;
		glVertex3dv(pz);
		glColor4ub(255,255,31,255);
		glVertex3d(0., 0., 0.);
		glEnd();
	}
	glPopMatrix();
/*	{
		struct gldBeamsData bd;
		amat4_t mat;
		avec3_t nh, nh0 = {0., 0., -.002}, v;
		pyrmat(*pyr, &mat);
		MAT4VP3(nh, mat, nh0);
		bd.cc = 0;
		bd.solid = 0;
		gldBeams(&bd, *viewer, *pos, rad * .2, COLOR32RGBA(255,255,31,255));
		VECADD(v, *pos, nh);
		gldBeams(&bd, *viewer, v, rad * .4, COLOR32RGBA(255,127,0,255));
		VECADDIN(v, nh);
		gldBeams(&bd, *viewer, v, rad * .01, COLOR32RGBA(255,127,0,255));
	}*/
/*	{
		static int init = 0;
		static GLuint list;
		struct gldBeamsData bd;
		amat4_t mat;
		avec3_t nh, nh0 = {0., 0., -.002}, v;
		if(!init){
			GLbyte buf[4][4] = {
				{255,255,255,0},
				{255,255,255,64},
				{255,255,255,191},
				{255,255,255,255}
			};
			glNewList(list = glGenLists(1), GL_COMPILE);
			glEnable(GL_TEXTURE_1D);
			glDisable(GL_BLEND);
			glTexImage1D(GL_TEXTURE_1D, 0, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
			glEndList();
			init = 1;
		}
		glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT);
		glCallList(list);
		pyrmat(*pyr, &mat);
		MAT4VP3(nh, mat, nh0);
		bd.cc = 0;
		bd.solid = 0;
			glEnable(GL_TEXTURE_1D);
		gldBeams(&bd, *viewer, *pos, rad * .2, COLOR32RGBA(255,255,31,255));
		VECADD(v, *pos, nh);
		gldBeams(&bd, *viewer, v, rad * .4, COLOR32RGBA(255,127,0,255));
		VECADDIN(v, nh);
		gldBeams(&bd, *viewer, v, rad * .01, COLOR32RGBA(255,127,0,255));
		glPopAttrib();
	}*/
}




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
