//#include "atmosphere.h"
#include "astrodraw.h"
#include "Universe.h"
//#include "player.h"
//#include "judge.h"
#include "coordsys.h"
#include "antiglut.h"
#include "galaxy_field.h"
#include "astro_star.h"
#include "stellar_file.h"
#define exit something_meanless
#include <windows.h>
#undef exit
extern "C"{
#include "bitmap.h"
#include <clib/amat3.h>
#include <clib/rseq.h>
#include <clib/timemeas.h>
#include <clib/gl/cull.h>
#include <clib/gl/gldraw.h>
#include <clib/gl/multitex.h>
#include <clib/suf/sufdraw.h>
#include <clib/colseq/cs2x.h>
#include <clib/cfloat.h>
#include <jpeglib.h>
}
#include <cpplib/gl/cullplus.h>
#include <gl/glext.h>
#include <stdio.h>
//#include <wingraph.h>
#include <assert.h>
#include <math.h>
#include <setjmp.h>
#include <strstream>
#include <sstream>
#include <iomanip>
#include <fstream>


#define numof(a) (sizeof(a)/sizeof*(a))
#define signof(a) ((a)<0?-1:0<(a)?1:0)
#define COLIST(c) COLOR32R(c), COLOR32G(c), COLOR32B(c), COLOR32A(c)
#define EPSILON 1e-7 // not sure
#define DRAWTEXTURESPHERE_PROFILE 0


#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef ABS
#define ABS(a) ((a)<0?-(a):(a))
#endif


int g_invert_hyperspace = 0;

void drawsuncolona(Astrobj *a, const Viewer *vw);
void drawpsphere(Astrobj *ps, const Viewer *vw, COLOR32 col);
void drawAtmosphere(const Astrobj *a, const Viewer *vw, const avec3_t sunpos, double thick, const GLfloat hor[4], const GLfloat dawn[4], GLfloat ret_horz[4], GLfloat ret_amb[4], int slices);
GLuint ProjectSphereJpg(const char *fname);

#if 0
void directrot(const double pos[3], const double base[3], amat4_t irot){
#if 1
	avec3_t dr, v;
	aquat_t q;
	amat4_t mat;
	double p;
	VECSUB(dr, pos, base);
	VECNORMIN(dr);

	/* half-angle formula of trigonometry replaces expensive tri-functions to square root */
	q[3] = sqrt((dr[2] + 1.) / 2.) /*cos(acos(dr[2]) / 2.)*/;

	VECVP(v, avec3_001, dr);
	p = sqrt(1. - q[3] * q[3]) / VECLEN(v);
	VECSCALE(q, v, p);
	quat2mat(mat, q);
	glMultMatrixd(mat);
	if(irot){
		MAT4CPY(irot, mat);
	}
#else
	double x = pos[0] - base[0], z = pos[2] - base[2], phi, theta;
	phi = atan2(x, z);
	theta = atan2((pos[1] - base[1]), sqrt(x * x + z * z));
	glRotated(phi * 360 / 2. / M_PI, 0., 1., 0.);
	glRotated(theta * 360 / 2. / M_PI, -1., 0., 0.);

	if(irot){
		amat4_t mat;
		MAT4ROTY(mat, mat4identity, -phi);
		MAT4ROTX(irot, mat, -theta);
	}
#endif
}


void rgb2hsv(double hsv[3], double r, double g, double b){
	double *ma, mi;
	ma = r < g ? &g : &r;
	ma = *ma < b ? &b : ma;
	mi = MIN(r, MIN(g, b));
	if(*ma == mi) hsv[0] = 0.; /* this value doesn't care */
	else if(ma == &r) hsv[0] = (g - b) / (*ma - mi) / 6.;
	else if(ma == &b) hsv[0] = (b - r) / (*ma - mi) / 6. + 1. / 3.;
	else hsv[0] = (r - g) / (*ma - mi) / 6. + 2. / 3.;
	hsv[0] -= floor(hsv[0]);
	hsv[1] = *ma ? (*ma - mi) / *ma : 1.;
	hsv[2] = *ma;
	assert(0. <= hsv[0] && hsv[0] <= 1.);
	assert(0. <= hsv[1] && hsv[1] <= 1.);
	assert(0. <= hsv[2] && hsv[2] <= 1.);
}

void hsv2rgb(double rgb[3], const double hsv[3]){
	int hi;
	double hue = hsv[0], sat = hsv[1], bri = hsv[2], f, p, q, t, r, g, b;
	hi = (int)floor(hue * 6.);
	f = hue * 6. - hi;
	p = bri * (1 - sat);
	q = bri * (1 - f * sat);
	t = bri * (1 - (1 - f) * sat);
	switch(hi){
		case 0: r = bri, g = t, b = q; break;
		case 1: r = q, g = bri, b = p; break;
		case 2: r = p, g = bri, b = t; break;
		case 3: r = p, g = q, b = bri; break;
		case 4: r = t, g = p, b = bri; break;
		case 5:
		case 6: r = bri, g = p, b = q; break;
		default: r = g = b = bri;
	}
	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
}

void doppler(double rgb[3], double r, double g, double b, double velo){
	double hsv[3];
	rgb2hsv(hsv, r, g, b);
	hsv[0] = hsv[0] + 1. * velo / LIGHT_SPEED;
	hsv[1] = 1. - (1. - hsv[1]) / (1. + fabs(velo) / LIGHT_SPEED);
	hsv[2] *= (hsv[0] < 0. ? 1. / (1. - hsv[0]) : 1. < hsv[0] ? 1. / hsv[0] : 1.);
	hsv[0] = rangein(hsv[0], 0., 1.);
	hsv2rgb(rgb, hsv);
}

#endif
void drawPoint(const Astrobj *a, const Viewer *p, const Vec3d &pspos, const Vec3d &sunp, const GLubyte color[4]){
	double sp;
	Vec3d tp;
	GLubyte lumi;
	Vec3d pos, ray;
	tp = pspos - p->pos;
/*	tocs(sunp, p->cs, sun.pos, sun.cs);*/
	ray = pspos - sunp;
	sp = tp.sp(ray) / tp.len() / ray.len();
	sp = (sp + 1.) / 2.;
	sp = 1. - (1. - sp * sp) * .9;
	lumi = (GLubyte)(255 * sp);
	glPushAttrib(GL_POINT_BIT);
	glDisable(GL_POINT_SMOOTH);
	glPointSize(2);
	glBegin(GL_POINTS);
	glColor3ub(lumi * color[0] / 256, lumi * color[1] / 256, lumi * color[2] / 256);
	pos = (pspos - p->pos).norm();
	glVertex3dv(pos);
	glEnd();
	glPopAttrib();
}

static void drawShadeSphere(Astrobj *ps, const Viewer *p, const Vec3d &sunpos, const GLubyte color[4], const GLubyte dark[4]){
	double (*cuts)[2];
	int i, n;
	Vec3d sunp, tp, pspos;
	Vec3d xh, yh;
/*	drawpsphere(a, COLOR32RGBA(191,191,191,255));*/
	double dist, as, cas, sas;
	double x, z, phi, theta;
	double zoom, spe;
	pspos = ps->calcPos(*p);
/*	tocs(sunp, p->cs, sun.pos, sun.cs);*/
	sunp = sunpos - p->pos;

	dist = (pspos - p->pos).len();
	tp = pspos - p->pos;

	x = pspos[0] - p->pos[0], z = pspos[2] - p->pos[2];

	if(dist < ps->rad)
		return;
	
	/* estimate roughly apparent size change caused by relativity effect */
	spe = (tp.sp(p->velo) / tp.len() / p->velolen - 1.) / 2.;
	zoom = p->velolen == 0. || !p->relative ? 1. : (1. + (LIGHT_SPEED / (LIGHT_SPEED - p->velolen) - 1.) * spe * spe);
	zoom *= fabs(p->gc->scale(pspos));
	if(ps->rad * /*gvp.m **/ zoom < 1e-3 /** 1e5 < dist*/)
		return;

#if 1
	if(ps->rad * /* gvp.m */ zoom < 2./*dist*/){
		drawPoint(ps, p, pspos, sunp, color);
		return;
	}
#endif
	as = asin(sas = ps->rad / dist);
	n = 8 + int(8 * sas);
	if(!n)
		return;
	cas = cos(as);
/*		n = (int)(16*(-(1. - sas) * (1. - sas) + 1.)) + 5;*/
/*		n = (int)(16*(-(as - M_PI / 4.) * (as - M_PI / 4.) / M_PI * 4. / M_PI * 4. + 1.)) + 5;*/
	phi = atan2(x, z);
	theta = atan2((pspos[1] - p->pos[1]), sqrt(x * x + z * z));
	cuts = CircleCuts(n * 2);

	glPushMatrix();
	glRotated(phi * 360 / 2. / M_PI, 0., 1., 0.);
	glRotated(theta * 360 / 2. / M_PI, -1., 0., 0.);
	{
		static const Vec3d xh0(1., 0., 0.), yh0(0., 1., 0.), nh0(0., 0., 1.), v1(0., 0., 1.), xh1(0., 1., 1.), yh1(1., 0., 1.);
		Mat4d mat, mat1;
		Vec3d delta, v, nh;
		double x, y, at;

/*		tocs(sunp, p->cs, sun.pos, sun.cs);*/
		tp = pspos - p->pos;

		mat = mat4_u;
		mat1 = mat.roty(phi);
		mat = mat1.rotx(-theta);
		xh = mat.vp3(xh0);
		yh = mat.vp3(yh0);
		delta = sunp - tp;
		x = xh.sp(sunp);
		y = yh.sp(sunp);
		at = atan2(y, x);
		glRotated(at * 360 / 2. / M_PI, 0., 0., 1.);

		/* cheaty trick to show the object having real sphere shape. */
		/*if(fmod(p->gametime, .5) < .45)*/{
			Vec3d plane, side;
			plane = sunp.vp(tp);
			side = plane.vp(delta).norm();
			tp += side * -ps->rad;
		}
	}

	{
	double sp, spa;
	Vec3d nsunp;
	nsunp = sunp - p->pos;
	nsunp *= -100;

	/* i really dont like to trace a ray here just for light source obscuring, but couldnt help. */
/*	if(jHitSphere(pspos, ps->rad, sunp, nsunp, 1.)){
		sp = spa = VECSP(sunp, tp) < 0. ? -1. : 1.;
	}
	else*/{
		sp = sunp.sp(tp) / sunp.len() / tp.len();
		spa = sp + .3 * (1. - sp * sp);
	}

	glColor4ubv(color);
	glBegin(GL_QUAD_STRIP);
	for(i = 0; i <= n; i++){
		glVertex3d(cuts[i][0] * sas * spa, cuts[i][1] * sas, cas);
		glVertex3d(cuts[i][0] * sas, cuts[i][1] * sas, cas);
	}
	glEnd();
	glBegin(GL_QUAD_STRIP);
	for(i = 0; i <= n; i++){
		glColor4ubv(dark);
		glVertex3d(cuts[i][0] * sas * sp, cuts[i][1] * sas, cas);
		glColor4ubv(color);
		glVertex3d(cuts[i][0] * sas * spa, cuts[i][1] * sas, cas);
	}
	glEnd();
	glBegin(GL_QUAD_STRIP);
	glColor4ubv(dark);
	for(i = 0; i <= n; i++){
		glVertex3d(-cuts[i][0] * sas, cuts[i][1] * sas, cas);
		glVertex3d(cuts[i][0] * sas * sp, cuts[i][1] * sas, cas);
	}
	glEnd();
	glPopMatrix();
	}
}



typedef struct normvertex_params{
	Mat4d mat;
	Mat4d texmat;
	const Viewer *vw;
	int texenable;
	int map;
	int detail;
} normvertex_params;

#if DRAWTEXTURESPHERE_PROFILE
static int normvertex_invokes = 0;
#endif

static void normvertexf(double x, double y, double z, normvertex_params *p, double ilen, int i, int j){
	Vec3d v, v1, vt;
	double theta, phi;
#if DRAWTEXTURESPHERE_PROFILE
	normvertex_invokes++;
#endif
	v[0] = x;
	v[1] = y;
	v[2] = z;
	v1 = p->mat.dvp3(v);
	glNormal3dv(v1);
	if(p->texenable){
		vt = p->texmat.dvp3(v);
		theta = vt[2];
		phi = vt[0];
	#if 0
		if(p->map < 0)
			p->map = vt[2] < 0. ? 0 : 2;
		if(p->map == 2){
			vt[0] *= -1;
		}
	#else
		if(p->map < 0)
			p->map = vt[2] < 0. ? (vt[0] / vt[2]) < -1. ? 3 : 1. < vt[0] / vt[2] ? 1 : 0 : -vt[0] / vt[2] < -1. ? 3 : 1. < -vt[0] / vt[2] ? 1 : 2;
		if(p->map == 1){
			vt[0] = vt[2];
			vt[2] = -phi;
		}
		else if(p->map == 2){
			vt[0] *= -1;
		}
		else if(p->map == 3){
			vt[0] = -vt[2];
			vt[2] = phi;
		}
	#endif
		vt[0] = p->map & 1 ? (3. - vt[0]) / 4. : (1. + vt[0]) / 4.;
/*		phi = 0. < theta ? vt[1] : -vt[1];*/
		vt[1] = ((!!(p->map & 1) + !!(p->map & 2)) & 1 ? (3. + vt[1]) / 4. : (1. + vt[1]) / 4.);
		glTexCoord2dv(vt);
		if(p->detail)
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB, GLfloat(phi * 256.), GLfloat(theta * 256.));
	}
/*	VECCPY(v1, v);*/
/*	phi = atan2(v1[1], v1[0]);
	theta = acos(v1[2] / sqrt(v1[0] * v1[0] + v1[1] * v1[1])) + M_PI / 2.;
	glTexCoord2d(phi / 2. / M_PI, theta / M_PI);*/
	if(p->vw->relative){
		v1 = p->mat * v;
		theta = v1.len();
		v = v1 * 1. / theta;
		v1 = p->vw->relrot * v;
		v1.scalein(theta);
	}
	else
		v1 = p->vw->relrot * (p->mat * v);
/*	VECNORMIN(v1);*/
/*	VECSCALEIN(v1, ilen);*/
	glVertex3dv(v1);
}


#define normvertex(x,y,z,p,ilen) normvertexf(x,y,z,p,ilen,i,j)
#define PROJTS 1024
#define PROJS (PROJTS/2)
#define PROJBITS 32 /* bit depth of projected texture */

bool drawTextureSphere(Astrobj *a, const Viewer *vw, const Vec3d &sunpos, const GLfloat mat_diffuse[4], const GLfloat mat_ambient[4], GLuint *ptexlist, const Mat4d *texmat, const char *texname){
	GLuint texlist = *ptexlist;
	double (*cuts)[2], (*finecuts)[2], (*ffinecuts)[2];
	double dist, tangent, scale, spe, zoom;
	int i, j, jstart, fine, texenable = texlist && texmat;
	normvertex_params params;
	Mat4d &mat = params.mat;
	Mat4d rot;
	Vec3d tp;

	params.vw = vw;
	params.texenable = texenable;
	params.detail = 0;

	const Vec3d apos = vw->cs->tocs(a->pos, a->parent);
/*	tocs(sunpos, vw->cs, sun.pos, sun.cs);*/
	scale = a->rad * vw->gc->scale(apos);

	VECSUB(tp, apos, vw->pos);
	spe = (VECSP(tp, vw->velo) / VECLEN(tp) / vw->velolen - 1.) / 2.;
	zoom = !vw->relative || vw->velolen == 0. ? 1. : LIGHT_SPEED / (LIGHT_SPEED - vw->velolen) /*(1. + (LIGHT_SPEED / (LIGHT_SPEED - vw->velolen) - 1.) * spe * spe)*/;
	scale *= zoom;
	if(0. < scale && scale < 5.){
		GLubyte color[4], dark[4];
		color[0] = GLubyte(mat_diffuse[0] * 255);
		color[1] = GLubyte(mat_diffuse[1] * 255);
		color[2] = GLubyte(mat_diffuse[2] * 255);
		color[3] = 255;
		dark[0] = GLubyte(mat_ambient[0] * 127);
		dark[1] = GLubyte(mat_ambient[1] * 127);
		dark[2] = GLubyte(mat_ambient[2] * 127);
		dark[3] = 255;
		drawShadeSphere(a, vw, sunpos, color, dark);
		return true;
	}

	do if(!texlist && texname){
		timemeas_t tm;
		TimeMeasStart(&tm);
		texlist = *ptexlist = ProjectSphereJpg(texname);
//		CmdPrintf("%s draw: %lg", texname, TimeMeasLap(&tm));
	} while(0);

	cuts = CircleCuts(32);
	finecuts = CircleCutsPartial(256, 9);

	dist = (apos - vw->pos).len();
	if(dist < a->rad)
		return true;

	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT);
/*	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(1);
	glDisable(GL_BLEND);
	glColor3ub(255,255,255);*/

	if(texenable){
		glCallList(texlist);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0., 0.);
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}
	else
		glDisable(GL_TEXTURE_2D);

	if(vw->detail){
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

/*	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glMultMatrixd(texmat);
	glMatrixMode(GL_MODELVIEW);*/

	{
		const GLfloat mat_specular[] = {0., 0., 0., 1.};
		const GLfloat mat_shininess[] = { 50.0 };
		const GLfloat color[] = {1.f, 1.f, 1.f, 1.f}, amb[] = {.1f, .1f, .1f, 1.f};

		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, texenable ? color : mat_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, texenable ? amb : mat_ambient);
		glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, color);
	}
/*		glPolygonMode(GL_BACK, GL_LINE);*/
/*	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);*/
/*	glEnable(GL_TEXTURE_2D);*/
	if(texenable)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glEnable(GL_NORMALIZE);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	{
		Mat4d irot;
		gldLookatMatrix(~irot, ~(apos - vw->pos));
		rot = irot.transpose();
		if(texenable){
			Mat4d rotm, rot2;
			rotm = vw->cs->tocsm(a->parent);
			rot2 = *texmat * rotm;
	/*		MAT4CPY(rot, irot);*/
			params.texmat = rot2 * irot;
		}
		mat = irot.translatein(0., 0., dist).scalein(-a->rad, -a->rad, -a->rad);
	}

	glPushMatrix();
	glLoadIdentity();

/*	MAT4CPY(params.texmat, texmat);*/

	tangent = asin(a->rad / dist);
/*	fine = M_PI / 3. < tangent;*/
	fine = M_PI * 7. / 16. < tangent ? 2 : M_PI / 3. < tangent ? 1 : 0;
	ffinecuts = CircleCutsPartial(2048, 9);

	{
		Vec4<GLfloat> light_position;
		light_position = (sunpos - apos).cast<GLfloat>();
		light_position[3] = 0.;
		glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	}

#if DRAWTEXTURESPHERE_PROFILE
	normvertex_invokes = 0;
	timemeas_t tm;
	TimeMeasStart(&tm);
#endif
	glBegin(GL_QUADS);
	jstart = int(tangent * 32 / 2 / M_PI);
	for(j = jstart; j < (fine ? 7 : 8); j++){
		double c, s, len1, len2;
		Vec3d v1, v;
		if(j == jstart){
			c = cos(tangent);
			s = sin(tangent);
		}
		else{
			c = cuts[j][1];
			s = cuts[j][0];
		}
		v[0] = c, v[1] = 0., v[2] = s;
		v1 = mat * v;
		len1 = 1. / VECLEN(v1);
		v[0] = cuts[j+1][1], v[2] = cuts[j+1][0];
		v1 = mat * v;
		len2 = 1. / VECLEN(v1);
		for(i = 0; i < 32; i++){
			int i2 = (i+1) % 32;
			params.map = -1;
			normvertex(cuts[i][0] * c, cuts[i][1] * c, s, &params, len1);
			normvertex(cuts[i2][0] * c, cuts[i2][1] * c, s, &params, len1);
			normvertex(cuts[i2][0] * cuts[j+1][1], cuts[i2][1] * cuts[j+1][1], cuts[j+1][0], &params, len2);
			normvertex(cuts[i][0] * cuts[j+1][1], cuts[i][1] * cuts[j+1][1], cuts[j+1][0], &params, len2);
		}
	}
	if(1 <= fine) for(i = 0; i < 32; i++) for(j = 0; j < (1 < fine ? 7 : 8); j++){
		int i2 = (i+1) % 32;
		double len1, len2;
		Vec3d v1, v;
		v[0] = finecuts[j][0], v[1] = 0., v[2] = finecuts[j][1];
		v1 = mat * v;
		len1 = 1. / VECLEN(v1);
		v[0] = finecuts[j+1][0], v[2] = finecuts[j+1][1];
		v1 = mat * v;
		len2 = 1. / VECLEN(v1);
		params.map = -1;
		normvertex(cuts[i][0] * finecuts[j][0], cuts[i][1] * finecuts[j][0], finecuts[j][1], &params, len1);
		normvertex(cuts[i][0] * finecuts[j+1][0], cuts[i][1] * finecuts[j+1][0], finecuts[j+1][1], &params, len2);
		normvertex(cuts[i2][0] * finecuts[j+1][0], cuts[i2][1] * finecuts[j+1][0], finecuts[j+1][1], &params, len2);
		normvertex(cuts[i2][0] * finecuts[j][0], cuts[i2][1] * finecuts[j][0], finecuts[j][1], &params, len1);
	}
	glEnd();
	if(2 <= fine){
		if(texenable){
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
		glBegin(GL_QUADS);
		for(i = 0; i < 32; i++) for(j = 0; j < 8; j++){
			int i2 = (i+1) % 32;
			double len1, len2;
			Vec3d v1, v;
			v[0] = ffinecuts[j][0], v[1] = 0., v[2] = ffinecuts[j][1];
			v1 = mat * v;
			len1 = 1. / VECLEN(v1);
			v[0] = ffinecuts[j+1][0], v[2] = ffinecuts[j+1][1];
			v1 = mat * v;
			len2 = 1. / VECLEN(v1);
			params.detail = 1;
			params.map = -1;
			normvertex(cuts[i][0] * ffinecuts[j][0], cuts[i][1] * ffinecuts[j][0], ffinecuts[j][1], &params, len1);
			normvertex(cuts[i][0] * ffinecuts[j+1][0], cuts[i][1] * ffinecuts[j+1][0], ffinecuts[j+1][1], &params, len2);
			normvertex(cuts[i2][0] * ffinecuts[j+1][0], cuts[i2][1] * ffinecuts[j+1][0], ffinecuts[j+1][1], &params, len2);
			normvertex(cuts[i2][0] * ffinecuts[j][0], cuts[i2][1] * ffinecuts[j][0], ffinecuts[j][1], &params, len1);
		}
		glEnd();
	}
#if DRAWTEXTURESPHERE_PROFILE
	printf("drawtexsphere[%d] %lg\n", normvertex_invokes, TimeMeasLap(&tm));
#endif
/*	glBegin(GL_POLYGON);
	j = 7;
	for(i = 0; i < 32; i++){
		normvertex(cuts[i][0] * cuts[j][1], cuts[i][1] * cuts[j][1], cuts[j][0], mat);
	}
	glEnd();*/

/*	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);*/

/*	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glColor4ubv(hud_color_options[hud_color]);
	glBegin(GL_LINES);
	{
		avec3_t src, dst;
		MAT4DVP3(dst, params.texmat, avec3_001);
		VECSCALEIN(dst, a->rad);
		glVertex3dv(avec3_000);
		glVertex3dv(dst);
	}
	glEnd();*/

	glPopAttrib();
	glPopMatrix();
	return texenable;
}

void drawSphere(const struct astrobj *a, const Viewer *vw, const avec3_t sunpos, GLfloat mat_diffuse[4], GLfloat mat_ambient[4]){
	GLuint zero = 0;
//	drawTextureSphere(a, vw, sunpos, mat_diffuse, mat_ambient, &zero, mat4identity, NULL);
}

void TexSphere::draw(const Viewer *vw){
	Astrobj *sun = findBrightest();
	Vec3d sunpos = sun ? vw->cs->tocs(sun->pos, sun->parent) : vec3_000;

	drawAtmosphere(this, vw, sunpos, atmodensity, atmohor, atmodawn, NULL, NULL, 32);
	if(sunAtmosphere(*vw)){
		Astrobj *brightest = findBrightest();
		if(brightest)
			drawsuncolona(brightest, vw);
	}
	if(!vw->gc->cullFrustum(calcPos(*vw), rad * 2.)){
		bool ret = drawTextureSphere(this, vw, sunpos,
			Vec4<GLfloat>(COLOR32R(basecolor) / 255.f, COLOR32R(basecolor) / 255.f, COLOR32B(basecolor) / 255.f, 1.f),
			Vec4<GLfloat>(COLOR32R(basecolor) / 511.f, COLOR32G(basecolor) / 511.f, COLOR32B(basecolor) / 511.f, 1.f), &texlist, &qrot.tomat4(), texname);
		if(!ret && texname){
			delete[] texname;
			texname = NULL;
		}
	}
	st::draw(vw);
}

struct atmo_dye_vertex_param{
	double redness;
	double isotropy;
	double air;
	double d;
	double pd;
	const Vec3d *sundir;
	const GLfloat *col, *dawn;
};

static void atmo_dye_vertex(struct atmo_dye_vertex_param &p, double x, double y, double z, double s, const Vec3d &base){
	const Vec3d &sundir = *p.sundir;
	double f;
	Vec3d v(x, y, z);
	double sp;
	double horizon, b;
	sp = base.sp(sundir);
	double vsp = v.sp(sundir);
	double nis = (1. - z) / 2.;
	b = /**(1. - sundir[2]) / 2. * */(sp + 1.) / 2. * (1. - nis) + (1. - sundir[2]) / 2. * nis;
	horizon = 1. * (1. - (1. - v[2]) * (1. - v[2])) / (1. - sundir[2] * 4.);
	horizon = 1. / (1. + fabs(horizon));
	f = p.redness * (vsp + 1. + p.isotropy) / (2. + p.isotropy) * horizon;
/*	g = f * horizon * horizon * horizon * horizon;*/
/*	f = redness * ((v[0] * sundir[0] + v[1] * sundir[1]) + v[2] * sundir[2] * isotropy + 1.) / (2. + isotropy);*/
	glColor4d(p.col[0] * b * (1. - f) + p.dawn[0] * f,
		p.col[1] * b * (1. - f) + p.dawn[1] * f,
		p.col[2] * b * (1. - f) + p.dawn[2] * f,
		p.col[3] * (1. + b) / 2. / (1. + (1. - s * s) * p.air) * (1. - f) + p.dawn[3] * f);
}

void drawAtmosphere(const Astrobj *a, const Viewer *vw, const avec3_t sunpos, double thick, const GLfloat hor[4], const GLfloat dawn[4], GLfloat ret_horz[4], GLfloat ret_amb[4], int slices){
	int hdiv = slices == 0 ? 16 : slices;
	int s, t;
	double (*hcuts)[2];
	double sdist, dist, as, cas, sas, height;
	double b;
	double redness, isotropy;
	Vec3d apos, delta, sundir;
	Vec3d spos;
	apos = vw->cs->tocs(a->pos, a->parent);
	delta = apos - vw->pos;
	sdist = delta.slen();

	/* too far */
	if(a->rad * a->rad / sdist < .01 * .01)
		return;

	dist = sqrt(sdist);
	/* buried in */
	if(dist < a->rad)
		dist = a->rad + EPSILON;
	as = asin(sas = a->rad / dist);
	cas = cos(as);
	height = dist - a->rad;
	hcuts = CircleCuts(hdiv);
	isotropy = thick / (thick / 5. + height);

	{
		double sp, cen;
		VECSUB(spos, sunpos, apos);
		sp = VECSP(spos, delta) / VECLEN(spos) / dist;
		b = (1. - sp) / 2.;
		cen = (sp - .0);
		redness = (1. / (1. + 500 * cen * cen * cen * cen) / (1. + MAX(0., dist - a->rad) / thick));
	}

	glPushMatrix();
	{
		Mat4d rot;
		gldLookatMatrix(~rot, ~(apos - vw->pos));
		glMultMatrixd(rot);
		sundir = rot.tdvp3(spos);
		sundir.normin();
	}
/*	glRotated(phi * 360 / 2. / M_PI, 0., 1., 0.);
	glRotated(theta * 360 / 2. / M_PI, -1., 0., 0.);*/
	struct atmo_dye_vertex_param param;
	double &pd = param.pd;
	double &d = param.d;
	double ss0 = sin(M_PI / 2. - as), sc0 = -cos(M_PI / 2. - as);
	pd = (thick * 16.) / (dist - a->rad);
	d = min(pd, 1.);
	param.redness = redness;
	param.isotropy = isotropy;
	param.air = height / thick / d;
	param.sundir = &sundir;
	param.col = hor;
	param.dawn = dawn;
	for(s = 0; s < 16; s++){
		double h[2][2];
		double dss[2];
		double sss, sss1, ss, ss1, f, f1;
		sss = 16. * (1. - pow(1. - s / 16., 1. / (dist - a->rad)));
		sss1 = 16. * (1. - pow(1. - (s+1) / 16., 1. / (dist - a->rad)));
		ss = 1. - pow(1. - s / 16., 2. + (1. - d));
		ss1 = 1. - pow(1. - (s+1) / 16., 2. + (1. - d));
		f = (1. + (16 - s) / 16.), f1 = (1. + (16 - (s + 1)) / 16.);
/*		if(128. < (15 - s) / 16. * (dist - a->rad) / thick / d)
			continue;*/
		h[0][1] = sin((M_PI - as) * (dss[0] = d * ss + (1. - d)));
		h[0][0] = -cos((M_PI - as) * (dss[0]));
		h[1][1] = sin((M_PI - as) * (dss[1] = d * ss1 + (1. - d)));
		h[1][0] = -cos((M_PI - as) * (dss[1]));
		for(t = 0; t < hdiv; t++){
			int t1 = (t + 1) % hdiv;
			double dawness = redness * (1. - (1. - h[0][0]) * (1. - h[0][0])) / (1. + -sundir[2] * 4.);
			Vec3d base[2];
			base[0] = Vec3d(hcuts[t][0] * ss0, hcuts[t][1] * ss0, sc0);
			base[1] = Vec3d(hcuts[t1][0] * ss0, hcuts[t1][1] * ss0, sc0);

			glBegin(GL_QUADS);
			atmo_dye_vertex(param, hcuts[t][0] * h[0][1], hcuts[t][1] * h[0][1], h[0][0], dss[0], base[0]);
			glVertex3d(hcuts[t][0] * h[0][1], hcuts[t][1] * h[0][1], h[0][0]);
			atmo_dye_vertex(param, hcuts[t1][0] * h[0][1], hcuts[t1][1] * h[0][1], h[0][0], dss[0], base[1]);
			glVertex3d(hcuts[t1][0] * h[0][1], hcuts[t1][1] * h[0][1], h[0][0]);
			atmo_dye_vertex(param, hcuts[t1][0] * h[1][1], hcuts[t1][1] * h[1][1], h[1][0], dss[1], base[1]);
			glVertex3d(hcuts[t1][0] * h[1][1], hcuts[t1][1] * h[1][1], h[1][0]);
			atmo_dye_vertex(param, hcuts[t][0] * h[1][1], hcuts[t][1] * h[1][1], h[1][0], dss[1], base[0]);
			glVertex3d(hcuts[t][0] * h[1][1], hcuts[t][1] * h[1][1], h[1][0]);
			glEnd();

			if(s == 0 && ret_amb){
				glGetFloatv(GL_CURRENT_COLOR, ret_amb);
			}
			if(s == 15 && ret_horz){
				glGetFloatv(GL_CURRENT_COLOR, ret_horz);
			}


#if 0 && !defined NDEBUG
			if(t % 2 == 0){
				glBegin(GL_LINES);
				glColor4ub(255,255,255,255);
				glVertex3d(hcuts[t][0] * h[0][1], hcuts[t][1] * h[0][1], h[0][0]);
				glVertex3d(hcuts[t1][0] * h[0][1], hcuts[t1][1] * h[0][1], h[0][0]);
	/*			glVertex3d(hcuts[t][0] * h[1][1], hcuts[t][1] * h[1][1], h[1][0]);*/
				glEnd();
			}
#endif
		}
	}
	glPopMatrix();
}

#define DETAILSIZE 64
static GLuint projcreatedetail(const char *name, const suftexparam_t *pstp){
	struct random_sequence rs;
	int i, j;
	int tsize = DETAILSIZE;
	static GLubyte tbuf[DETAILSIZE][DETAILSIZE][3], tex[DETAILSIZE][DETAILSIZE][3];
	static int init = 0;
	init_rseq(&rs, 124867);

	if(!init){
		init = 1;
		for(i = 0; i < tsize; i++) for(j = 0; j < tsize; j++){
			int buf[3] = {0};
			int k;
			for(k = 0; k < 1; k++){
				buf[0] += 192 + rseq(&rs) % 64;
				buf[1] += 192 + rseq(&rs) % 64;
				buf[2] += 192 + rseq(&rs) % 64;
			}
			tbuf[i][j][0] = buf[0] / (1);
			tbuf[i][j][1] = buf[1] / (1);
			tbuf[i][j][2] = buf[2] / (1);
		}

		/* average surrounding 8 texels to smooth */
		for(i = 0; i < tsize; i++) for(j = 0; j < tsize; j++){
			int k, l;
			int buf[3] = {0};
			for(k = -1; k <= 1; k++) for(l = -1; l <= 1; l++)/* if(k != 0 || l != 0)*/{
				int x = (i + k + DETAILSIZE) % DETAILSIZE, y = (j + l + DETAILSIZE) % DETAILSIZE;
				buf[0] += tbuf[x][y][0];
				buf[1] += tbuf[x][y][1];
				buf[2] += tbuf[x][y][2];
			}
			tex[i][j][0] = buf[0] / 9 / 2 + 127;
			tex[i][j][1] = buf[1] / 9 / 2 + 127;
			tex[i][j][2] = buf[2] / 9 / 2 + 127;
		}
		tex[0][0][0] = 
		tex[0][0][1] = 
		tex[0][0][2] = 255;
	}
	{
		struct BMIhack{
			BITMAPINFOHEADER bmiHeader;
			GLubyte buf[DETAILSIZE][DETAILSIZE][3];
		} bmi;
		suftexparam_t stp;
		bmi.bmiHeader.biSize = 0;
		bmi.bmiHeader.biSizeImage = sizeof bmi.buf;
		bmi.bmiHeader.biBitCount = 24;
		bmi.bmiHeader.biClrUsed = 0;
		bmi.bmiHeader.biClrImportant = 0;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biHeight = bmi.bmiHeader.biWidth = DETAILSIZE;
		memcpy(bmi.buf, tex, sizeof bmi.buf);
		stp.bmi = (BITMAPINFO*)&bmi;
		stp.flags = STP_ENV | STP_MAGFIL | STP_MIPMAP;
		stp.env = GL_MODULATE;
		stp.magfil = GL_LINEAR;
		stp.mipmap = 1;
		stp.alphamap = 0;
		return CacheSUFMTex(name, pstp, &stp);
	}
}

GLuint ProjectSphereMap(const char *name, const BITMAPINFO *raw){
	static int texinit = 0;
	GLuint tex;
	/*if(!texinit)*/{
		int ii, i, outsize, linebytes, linebytesp;
		long rawh = ABS(raw->bmiHeader.biHeight);
		BITMAPINFO *proj;
		RGBQUAD zero = {0,0,0,255};
		texinit = 1;
		proj = (BITMAPINFO*)malloc(outsize = sizeof(BITMAPINFOHEADER) + (PROJTS * PROJTS * PROJBITS + 31) / 32 * 4);
		memcpy(proj, raw, sizeof(BITMAPINFOHEADER));
		proj->bmiHeader.biWidth = proj->bmiHeader.biHeight = PROJTS;
		proj->bmiHeader.biBitCount = PROJBITS;
		proj->bmiHeader.biClrUsed = 0;
		proj->bmiHeader.biSizeImage = proj->bmiHeader.biWidth * proj->bmiHeader.biHeight * proj->bmiHeader.biBitCount / 8;
		linebytes = (raw->bmiHeader.biWidth * raw->bmiHeader.biBitCount + 31) / 32 * 4;
		linebytesp = (PROJTS * PROJBITS + 31) / 32 * 4;
		for(ii = 0; ii < 2; ii++) for(i = 0; i < PROJS; i++){
		int i1 = raw->bmiHeader.biHeight < 0 ? PROJS - i - 1 : i;
		int jj, j;
		double r;
		r = sqrt(1. - (double)(i - PROJS / 2) / (PROJS / 2) * (i - PROJS / 2) / (PROJS / 2));
		for(jj = 0; jj < 2; jj++) for(j = 0; j < PROJS; j++){
			int j1;
			RGBQUAD *dst;
			dst = (RGBQUAD*)&((unsigned char*)&proj->bmiColors[proj->bmiHeader.biClrUsed])[(i + ii * PROJS) * linebytesp + (jj * PROJS + j) * PROJBITS / 8];
			if(r){
				double dj = j * 2. / PROJS - 1.;
				j1 = r < fabs(dj) ? -1 : (int)(raw->bmiHeader.biWidth * fmod(asin(dj / r) / M_PI / 2. + .25 + (ii * 2 + jj) * .25, 1.));
			}
			else{
				*dst = zero;
				continue;
			}
			if(raw->bmiHeader.biBitCount == 4){
				const RGBQUAD *src;
				src = &raw->bmiColors[(j1 < 0 || raw->bmiHeader.biWidth <= j1 ? 0 : ((unsigned char *)&raw->bmiColors[raw->bmiHeader.biClrUsed])[i1 * rawh / PROJS * linebytes + j1 / 2] & (0x0f << j1 % 2 * 4) >> j1 % 2 * 4)];
				*dst = *src;
				dst->rgbReserved = 255;
			}
			else if(raw->bmiHeader.biBitCount == 24){
				const unsigned char *src;
				src = &((unsigned char *)&raw->bmiColors[raw->bmiHeader.biClrUsed])[i1 * rawh / PROJS * linebytes + j1 * 3];
/*				dst->rgbRed = src[2];
				dst->rgbGreen = src[1];
				dst->rgbBlue = src[0];
				dst->rgbReserved = 255;*/
				dst->rgbRed = src[0];
				dst->rgbGreen = src[1];
				dst->rgbBlue = src[2];
				dst->rgbReserved = 255;
			}
			else if(raw->bmiHeader.biBitCount == 32){
				const unsigned char *src;
				src = &((unsigned char *)&raw->bmiColors[raw->bmiHeader.biClrUsed])[i1 * rawh / PROJS * linebytes + j1 * 4];
/*				dst->rgbRed = src[0];
				dst->rgbGreen = src[1];
				dst->rgbBlue = src[2];
				dst->rgbReserved = 255;*/
				dst->rgbRed = src[2];
				dst->rgbGreen = src[1];
				dst->rgbBlue = src[0];
				dst->rgbReserved = 255;
			}
		}}
#if 0
		fp = fopen("projected.bmp", "wb");
		((char*)&fh.bfType)[0] = 'B';
		((char*)&fh.bfType)[1] = 'M';
		fh.bfSize = sizeof(BITMAPFILEHEADER) + outsize;
		fh.bfReserved1 = fh.bfReserved2 = 0;
		fh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + proj->bmiHeader.biClrUsed * sizeof(RGBQUAD);
		fwrite(&fh, 1, sizeof(BITMAPFILEHEADER), fp);
		fwrite(proj, 1, outsize, fp);
		fclose(fp);
#endif
#if 1
		{
			suftexparam_t stp;
			stp.flags = STP_ENV | STP_MAGFIL;
			stp.bmi = proj;
			stp.alphamap = 8;
			stp.env = GL_MODULATE;
			stp.magfil = GL_LINEAR;
			stp.mipmap = 0;
			tex = projcreatedetail(name, &stp);
		}
#elif 1
		{
			suftexparam_t stp, stp2;
			stp.bmi = proj;
			stp.alphamap = 0;
			stp.env = GL_MODULATE;
			stp.magfil = GL_LINEAR;
			stp.mipmap = 0;
			stp2.bmi = proj;
			stp2.alphamap = 0;
			stp2.env = GL_MODULATE;
			stp2.magfil = GL_LINEAR;
			stp2.mipmap = 0;
			tex = CacheSUFMTex(name, &stp, &stp2);
		}
#else
		tex = CacheSUFTex(name, proj, 1);
#endif
		{
			std::ostringstream bstr;
			const char *p;
//			FILE *fp;
			p = strrchr(name, '.');
			bstr << "cache/" << (p ? std::string(name).substr(0, p - name) : name) << "_proj.bmp";

			/* force overwrite */
			std::ofstream ofs(bstr.str().c_str(), std::ofstream::out | std::ofstream::binary);
			if(ofs/*fp = fopen(bstr.str().c_str(), "wb")*/){
				BITMAPFILEHEADER fh;
				((char*)&fh.bfType)[0] = 'B';
				((char*)&fh.bfType)[1] = 'M';
				fh.bfSize = sizeof(BITMAPFILEHEADER) + outsize;
				fh.bfReserved1 = fh.bfReserved2 = 0;
				fh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + proj->bmiHeader.biClrUsed * sizeof(RGBQUAD);
//				fwrite(&fh, 1, sizeof(BITMAPFILEHEADER), fp);
//				fwrite(proj, 1, outsize, fp);
				ofs.write(reinterpret_cast<char*>(&fh), sizeof fh);
				ofs.write(reinterpret_cast<char*>(proj), outsize);
				ofs.close();
//				fclose(fp);
			}

#if 0
			strcat(jpgfilename, ".jpg");
			{
				BITMAPDATA bmd;
				bmd.pBmp = (DWORD*)((unsigned char*)&proj->bmiColors[proj->bmiHeader.biClrUsed]);
				bmd.dwDataSize = proj->bmiHeader.biSizeImage;
				swap3byte(bmd.pBmp, bmd.dwDataSize);
				bmd.nWidth = proj->bmiHeader.biWidth;
				bmd.nHeight = proj->bmiHeader.biHeight;
				SaveJPEGData(jpgfilename, &bmd, 80);
			}
#endif

		}
		free(proj);
/*		free(raw);*/
	}
	return tex;
}
#if 1
struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

void my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}
GLuint ProjectSphereJpg(const char *fname){
		const struct suftexcache *stc;
		GLuint texlist = 0;
//		char outfilename[256], jpgfilename[256];
		const char *outfilename, *jpgfilename;
		const char *p;
//		FILE *fp;
		p = strrchr(fname, '.');
#ifdef _WIN32
		if(GetFileAttributes("cache") == -1)
			CreateDirectory("cache", NULL);
#else
		mkdir("cache");
#endif
		std::ostringstream bstr;
		bstr << "cache/" << (p ? std::string(fname).substr(0, p - fname) : fname) << "_proj.bmp";
		std::string bs = bstr.str();
		outfilename = bs.c_str();
		jpgfilename = fname;
/*		strcpy(outfilename, "cache\\");
		if(p){
			strncat(outfilename, fname, p - fname);
			strcat(&outfilename[sizeof "cache\\" - 1 + p - fname], "_proj");
			strcpy(jpgfilename, outfilename);
			strcat(outfilename, ".bmp");
		}
		else{
			strcat(outfilename, fname);
			strcat(outfilename, "_proj");
			strcpy(jpgfilename, outfilename);
		}
		strcpy(jpgfilename, fname);*/
		stc = FindTexCache(bstr.str().c_str()/*outfilename*/);
		if(stc){
			return stc->list;
		}
		else{
			BITMAPINFO *bmi;
			struct jpeg_decompress_struct cinfo;
			struct my_error_mgr jerr;
			FILE * infile;		/* source file */
//			BITMAPDATA bmd;
			WIN32_FILE_ATTRIBUTE_DATA fd, fd2;
			BOOL b;
			b = GetFileAttributesEx(outfilename, GetFileExInfoStandard, &fd);
			GetFileAttributesEx(jpgfilename, GetFileExInfoStandard, &fd2);
			if(b && 0 < CompareFileTime(&fd.ftLastWriteTime, &fd2.ftLastWriteTime) && (bmi = ReadBitmap(outfilename))){
#if 1
				suftexparam_t stp;
				stp.flags = STP_ENV | STP_MAGFIL | STP_ALPHA;
				stp.bmi = bmi;
				stp.alphamap = 8;
				stp.env = GL_MODULATE;
				stp.magfil = GL_LINEAR;
				stp.mipmap = 0;
/*				{
				timemeas_t tm;
				TimeMeasStart(&tm);*/
				texlist = projcreatedetail(outfilename, &stp);
/*				CmdPrintf("ProjectSphereJpg projcreatedetail: %lg", TimeMeasLap(&tm));
				}*/
#elif 1
				suftexparam_t stp, stp2;
				stp.bmi = bmi;
				stp.alphamap = 0;
				stp.env = GL_MODULATE;
				stp.magfil = GL_LINEAR;
				stp.mipmap = 0;
				stp2.bmi = bmi;
				stp2.alphamap = 0;
				stp2.env = GL_MODULATE;
				stp2.magfil = GL_LINEAR;
				stp2.mipmap = 0;
				texlist = CacheSUFMTex(outfilename, &stp, &stp2);
#else
				texlist = CacheSUFTex(outfilename, bmi, 1);
#endif
				LocalFree(bmi);
			}
			else if((infile = fopen(jpgfilename, "rb")) == NULL){
				fprintf(stderr, "can't open %s\n", jpgfilename);
				return 0;
			}
			else{
				JSAMPARRAY buffer;		/* Output row buffer */
				int row_stride;		/* physical row width in image buffer */
				int src_row_stride;
				cinfo.err = jpeg_std_error(&jerr.pub);
				jerr.pub.error_exit = my_error_exit;
				/* Establish the setjmp return context for my_error_exit to use. */
				if (setjmp(jerr.setjmp_buffer)) {
					/* If we get here, the JPEG code has signaled an error.
					 * We need to clean up the JPEG object, close the input file, and return.
					 */
					jpeg_destroy_decompress(&cinfo);
					fclose(infile);
					return 0;
				}
				jpeg_create_decompress(&cinfo);
				jpeg_stdio_src(&cinfo, infile);
				(void) jpeg_read_header(&cinfo, TRUE);
				(void) jpeg_start_decompress(&cinfo);
				row_stride = cinfo.output_width * 3/*cinfo.output_components*/;
				src_row_stride = cinfo.output_width * cinfo.output_components;
				bmi = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) + cinfo.output_width * cinfo.output_height * 3/*cinfo.output_components*/);
				bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				bmi->bmiHeader.biWidth = cinfo.output_width; 
				bmi->bmiHeader.biHeight = LONG(-cinfo.output_height);
				bmi->bmiHeader.biPlanes = 1;
				bmi->bmiHeader.biBitCount = 24;
				bmi->bmiHeader.biCompression = BI_RGB;
				bmi->bmiHeader.biSizeImage = cinfo.output_width * cinfo.output_height * 3/*cinfo.output_components*/;
				bmi->bmiHeader.biXPelsPerMeter = 0;
				bmi->bmiHeader.biYPelsPerMeter = 0;
				bmi->bmiHeader.biClrUsed = 0;
				bmi->bmiHeader.biClrImportant = 0;
				buffer = (*cinfo.mem->alloc_sarray)
					((j_common_ptr) &cinfo, JPOOL_IMAGE, src_row_stride, 1);
				while (cinfo.output_scanline < cinfo.output_height) {
					(void) jpeg_read_scanlines(&cinfo, buffer, 1);
//					memcpy(&((JSAMPLE*)bmi->bmiColors)[(cinfo.output_scanline-1) * row_stride], buffer[0], row_stride);
					unsigned j;
					if(cinfo.output_components == 3) for(j = 0; j < cinfo.output_width; j++){
						JSAMPLE *dst = &((JSAMPLE*)bmi->bmiColors)[(cinfo.output_scanline-1) * row_stride + j * cinfo.output_components];
						JSAMPLE *src = &buffer[0][j * cinfo.output_components];
						dst[0] = src[2];
						dst[1] = src[1];
						dst[2] = src[0];
					}
					else if(cinfo.output_components == 1) for(j = 0; j < cinfo.output_width; j++){
						JSAMPLE *dst = &((JSAMPLE*)bmi->bmiColors)[(cinfo.output_scanline-1) * row_stride + j * 3];
						JSAMPLE *src = &buffer[0][j * cinfo.output_components];
						dst[0] = src[0];
						dst[1] = src[0];
						dst[2] = src[0];
					}
				}
				(void) jpeg_finish_decompress(&cinfo);
				jpeg_destroy_decompress(&cinfo);
				fclose(infile);

/*				bmi = malloc(sizeof(BITMAPINFOHEADER) + bmd.dwDataSize);
				bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				bmi->bmiHeader.biWidth = bmd.nWidth; 
				bmi->bmiHeader.biHeight = -bmd.nHeight;
				bmi->bmiHeader.biPlanes = 1;
				bmi->bmiHeader.biBitCount = 32;
				bmi->bmiHeader.biCompression = BI_RGB;
				bmi->bmiHeader.biSizeImage = bmd.dwDataSize;
				bmi->bmiHeader.biXPelsPerMeter = 0;
				bmi->bmiHeader.biYPelsPerMeter = 0;
				bmi->bmiHeader.biClrUsed = 0;
				bmi->bmiHeader.biClrImportant = 0;
				memcpy(bmi->bmiColors, bmd.pBmp, bmd.dwDataSize);
				FreeBitmapData(&bmd);*/

				texlist = ProjectSphereMap(fname, bmi);

/*					bmi = ReadBitmap("earth.bmp");*/
/*					bmi = lzuc(lzw_earth, sizeof lzw_earth, NULL);*/
/*					texlist = ProjectSphereMap("earth.bmp", bmi);*/
				free(bmi);
			}
		}
		return texlist;
}
#endif
#if 0
#define ASTEROIDLIST 1

static void drawasteroid(const double org[3], const double pyr[3], double rad, unsigned long seed){
	int i, j;
#if ASTEROIDLIST
	static GLuint varies[8] = {0};
#else
	static struct asteroidcache{
		double pos[8][9][3], norm[8][9][3];
		double tpos[3], tnorm[3];
		double bpos[3], bnorm[3];
	} caches[8];
	static char varies[numof(caches)];
#endif
	if(!varies[seed %= numof(varies)]){
	double *pos[8][9]; /* indirect vector referencing to simplify normal generation */
#if ASTEROIDLIST
	double posv[8][7][3], norm[8][8][3];
	double tpos[3], tnorm[3];
	double bpos[3], bnorm[3];
	glNewList(varies[seed] = glGenLists(1), GL_COMPILE);
	{
		GLfloat dif[] = {.5, .475, .45, 1.}, amb[] = {.1, .1, .05, 1.};
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
	}
#else
	double (*posv)[8][3] = caches[seed].pos, (*norm)[8][3] = caches[seed].norm;
	double *tpos = caches[seed].tpos, *tnorm = caches[seed].tnorm;
	double *bpos = caches[seed].bpos, *bnorm = caches[seed].bnorm;
#endif
	for(i = 0; i < 8; i++){
		pos[i][0] = tpos;
		for(j = 1; j < 8; j++)
			pos[i][j] = posv[i][j-1];
		pos[i][8] = bpos;
	}
	for(i = 0; i < 8; i++) for(j = 1; j < 8; j++){
		double ci, cj, si, sj, ci1, cj1, si1, sj1;
		struct random_sequence rs;
		ci = cos(i * 2 * M_PI / 8);
		si = sin(i * 2 * M_PI / 8);
		cj = cos(j * M_PI / 8);
		sj = sin(j * M_PI / 8);
		ci1 = cos((i+1) * 2 * M_PI / 8);
		si1 = sin((i+1) * 2 * M_PI / 8);
		cj1 = cos((j+1) * M_PI / 8);
		sj1 = sin((j+1) * M_PI / 8);
		init_rseq(&rs, seed + i + j * 8);
		pos[i][j][0] = (drseq(&rs) + 1.) * ci * sj;
		pos[i][j][1] = (drseq(&rs) + 1.) * cj;
		pos[i][j][2] = (drseq(&rs) + 1.) * si * sj;
	}
	{
		struct random_sequence rs;
		init_rseq(&rs, seed + 1221);
		tpos[0] = 0.;
		tpos[1] = (drseq(&rs) + 1.);
		tpos[2] = 0.;
		bpos[0] = 0.;
		bpos[1] = -(drseq(&rs) + 1.);
		bpos[2] = 0.;
	}
	for(i = 0; i < 8; i++) for(j = 1; j < 8; j++){
		int i1 = (i+1)%8, j0 = j-1, j1 = (j+1)%9, in = (i-1+8)%8, jn = (j-1+9)%9;
		avec3_t dx, dy, vec;
		VECSUB(dx, pos[i1][j], pos[i][j]);
		VECSUB(dy, pos[i][j1], pos[i][j]);
		VECVP(vec, dx, dy);
		VECNORMIN(vec);
		VECCPY(norm[i][j0], vec);
		VECSUB(dy, pos[i][jn], pos[i][j]);
		VECVP(vec, dy, dx);
		VECNORMIN(vec);
		VECADDIN(norm[i][j0], vec);
		VECSUB(dx, pos[in][j], pos[i][j]);
		VECVP(vec, dx, dy);
		VECNORMIN(vec);
		VECADDIN(norm[i][j0], vec);
		VECSUB(dy, pos[i][j1], pos[i][j]);
		VECVP(vec, dy, dx);
		VECNORMIN(vec);
		VECADDIN(norm[i][j0], vec);
		VECNORMIN(norm[i][j0]);
/*		glBegin(GL_LINES);
		glVertex3dv(pos[i][j]);
		{
			avec3_t sum;
			VECADD(sum, pos[i][j], norm[i][j]);
			glVertex3dv(sum);
		}
		glEnd();
		*/
	}
	for(i = 0; i < 8; i++) for(j = 1; j < 7; j++){
		int i1 = (i+1)%8, j1 = (j+1)%9, j0 = j-1, j01 = j;
		glBegin(GL_TRIANGLE_STRIP);
		glNormal3dv(norm[i][j0]);
		glVertex3dv(pos[i][j]);
		glNormal3dv(norm[i1][j0]);
		glVertex3dv(pos[i1][j]);
		glNormal3dv(norm[i][j01]);
		glVertex3dv(pos[i][j1]);
		glNormal3dv(norm[i1][j01]);
		glVertex3dv(pos[i1][j1]);
		glEnd();
	}
	{
		VECNULL(bnorm);

		/* bottom cap */
		for(i = 0; i < 8; i++){
			avec3_t dx, dy, vec;
			int i1 = (i+1)%8;
			VECSUB(dx, pos[i][7], bpos);
			VECSUB(dy, pos[i1][7], bpos);
			VECVP(vec, dx, dy);
			VECNORMIN(vec);
			VECADDIN(bnorm, vec);
		}
		VECNORMIN(bnorm);
#if ASTEROIDLIST
		glBegin(GL_TRIANGLE_FAN);
		glNormal3dv(bnorm);
		glVertex3dv(bpos);
		for(i = 0; i < 9; i++){
			int i1 = i%8, j = 7, j1 = 8;
			glNormal3dv(norm[i1][6]);
			glVertex3dv(pos[i1][j]);
		}
		glEnd();
#endif
/*		glBegin(GL_LINES);
		glVertex3dv(pos[0][8]);
		{
			avec3_t sum;
			VECADD(sum, pos[0][8], bnorm);
			glVertex3dv(sum);
		}
		glEnd();*/

		VECNULL(tnorm);

		/* top cap */
		for(i = 0; i < 8; i++){
			avec3_t dx, dy, vec;
			int i1 = (i+1)%8;
			VECSUB(dx, pos[i][1], tpos);
			VECSUB(dy, pos[i1][1], tpos);
			VECVP(vec, dy, dx);
			VECNORMIN(vec);
			VECADDIN(tnorm, vec);
		}
		VECNORMIN(tnorm);
#if ASTEROIDLIST
		glBegin(GL_TRIANGLE_FAN);
		glNormal3dv(tnorm);
		glVertex3dv(tpos);
		for(i = 9; i; i--){
			int i1 = i%8, j = 0, j1 = 1;
			glNormal3dv(norm[i1][0]);
			glVertex3dv(pos[i1][j1]);
		}
		glEnd();
#endif
	}
#if ASTEROIDLIST
	glEndList();
#else
	varies[seed] = 1;
#endif
	}
	glPushMatrix();
	glTranslated(org[0], org[1], org[2]);
	{
		amat4_t rot;
		pyrmat(pyr, &rot);
		glMultMatrixd(rot);
	}
	glScaled(rad / 2, rad / 2, rad / 2);
	glCallList(varies[seed]);
	glPopMatrix();
}

#define RING_TRACKS 10

typedef struct ringVertexData{
	amat4_t matz;
	int relative;
	double arad;
	avec3_t spos;
	avec3_t apos;
	amat4_t matty;
	char shadow[RING_TRACKS+2][RING_CUTS];
} ringVertexData;

static void ringVertex3d(double x, double y, double z, COLOR32 col, ringVertexData *rvd, int n, int i){
	avec3_t rpos, pos, pp, ray;
	amat4_t imatz, id;

	pos[0] = (x); pos[1] = (y); pos[2] = (z);

	/* i really dont like to trace a ray here just for light source obscuring, but couldnt help. */
	if(rvd->shadow[n][i % RING_CUTS]){
		glColor4ub(COLOR32R(col) / 8, COLOR32G(col) / 8, COLOR32B(col) / 8, COLOR32A(col));
/*		glColor4ub(0, 0, 0, COLOR32A(col));*/
	}
	else{
		gldColor32(col);
	}

	if(rvd->relative){
		MAT4VP3(pp, rvd->matz, pos);
		VECNORMIN(pp);
		glVertex3dv(pp);
	}
	else{
		glVertex3d(x, y, z);
	}
}

void ring_draw(const Viewer *vw, const struct astrobj *a, const avec3_t sunpos, char start, char end, const amat4_t rotation, double thick, double minrad, double maxrad, double t, int relative){
	int i, inside = 0;
	double sp, dir;
	double width = (maxrad - minrad) / RING_TRACKS;
	struct random_sequence rs;
	avec3_t apos, spos, delta, light;
	amat4_t mat;
	if(0||start == end)
		return;

	tocs(apos, vw->cs, a->pos, a->cs);

	{
		avec3_t v;
		amat4_t mat2, rot;
/*		MAT4ROTX(mat2, mat4identity, pitch);*/
		tocsim(rot, vw->cs, a->cs);
		if(rotation)
			mat4mp(mat, rot, rotation);
		else
			MAT4CPY(mat, rot);
/*		mat4mp(mat, rot, mat2);*/
/*		glPushMatrix();
		glLoadIdentity();
		glRotated(10., 1., 0., 0.);
		glGetDoublev(GL_MODELVIEW_MATRIX, mat);
		glPopMatrix();*/
		MAT4VP3(v, mat, avec3_001);
		VECSUB(delta, vw->pos, apos);
		sp = VECSP(delta, v);
		dir = sqrt(VECSLEN(delta) - sp * sp) / a->rad; /* diameter can be calculated by sp */
	}

/*	tocs(spos, vw->cs, sun.pos, sun.cs);*/
	VECCPY(spos, sunpos);
	VECNORM(light, sunpos);

	init_rseq(&rs, (unsigned long)a + 123);

	if(!(minrad < dir && dir < maxrad && -thick < sp && sp < thick)){
		double rad;
		avec3_t xh, yh;
		ringVertexData rvd;
		double x, y, roll;
		COLOR32 ocol = COLOR32RGBA(255,255,255,0);
		int cutnum = RING_CUTS;
		double (*cuts)[2];
		int n, shade;
		cuts = CircleCuts(cutnum);
		MAT4VP3(xh, mat, avec3_100);
		x = VECSP(delta, xh);
		MAT4VP3(yh, mat, avec3_010);
		y = VECSP(delta, yh);
		roll = atan2(x, y);

		/* determine whether we are looking the ring from night side */
		{
			avec3_t das;
			VECSUB(das, spos, apos);
			shade = VECSP(delta, &rotation[8]) * VECSP(das, &rotation[8]) < 0.;
		}

		glPushMatrix();
		if(relative)
			glLoadIdentity();
		glScaled(1. / a->rad, 1. / a->rad, 1. / a->rad);
		glTranslated(-vw->pos[0], -vw->pos[1], -vw->pos[2]);

		glPushMatrix();
		glLoadIdentity();
		glTranslated(apos[0], apos[1], apos[2]);
		glMultMatrixd(mat);
/*		glRotated(deg_per_rad * pitch, 1., 0., 0.);*/
		glRotated(-deg_per_rad * roll, 0., 0., 1.);
		glScaled(a->rad, a->rad, a->rad);
		glGetDoublev(GL_MODELVIEW_MATRIX, rvd.matty);
		glPopMatrix();
		glMultMatrixd(rvd.matty);

		rvd.relative = relative;
		rvd.arad = a->rad;
		VECCPY(rvd.apos, apos);
		VECCPY(rvd.spos, spos);

		if(relative){
			glGetDoublev(GL_MODELVIEW_MATRIX, rvd.matz);
			glPopMatrix();
			glPushMatrix();
		}

		for(n = 0; n <= RING_TRACKS+1; n++){
			int i2;
			rad = minrad + (maxrad - minrad) * n / (RING_TRACKS+1);
			for(i = start; i <= end; i++) for(i2 = 0; i2 < 2; i2++){
				int i1 = (i2 ? cutnum - i : i) % cutnum;
				avec3_t rpos, pos, ray;
				pos[0] = cuts[i1][0] * rad;
				pos[1] = cuts[i1][1] * rad;
				pos[2] = 0.;
				MAT4VP3(rpos, rvd.matty, pos);
				VECSUB(ray, rpos, spos);
				rvd.shadow[n][i1] = jHitSphere(apos, a->rad, spos, ray, 1.);
			}
		}

		for(n = 0; n < RING_TRACKS+1; n++){
			double radn;
			COLOR32 col, ocold, cold;
			rad = minrad + (maxrad - minrad) * n / (RING_TRACKS+1);
			radn = minrad + (maxrad - minrad) * (n+1) / (RING_TRACKS+1);
			if(n < RING_TRACKS)
				col = (shade ? COLOR32RGBA(31,31,31,0) : COLOR32RGBA(127,127,127,0)) | COLOR32RGBA(0,0,0,rseq(&rs));
			else{
				col = shade ? COLOR32RGBA(31,31,31,0) : COLOR32RGBA(127,127,127,0);
				radn = maxrad;
			}
			glBegin(GL_QUAD_STRIP);
			for(i = start; i <= end; i++){
				int i1 = i % cutnum;
/*				gldColor32(ocol);*/
				ringVertex3d(cuts[i1][0] * rad, cuts[i1][1] * rad, 0., ocol, &rvd, n, i1);
/*				gldColor32(col);*/
				ringVertex3d(cuts[i1][0] * radn, cuts[i1][1] * radn, 0., col, &rvd, n+1, i1);
			}
			glEnd();
			glBegin(GL_QUAD_STRIP);
			for(i = cutnum - end; i <= cutnum - start; i++){
				int i1 = i % cutnum;
/*				gldColor32(ocol);*/
				ringVertex3d(cuts[i1][0] * rad, cuts[i1][1] * rad, 0., ocol, &rvd, n, i1);
/*				gldColor32(col);*/
				ringVertex3d(cuts[i1][0] * radn, cuts[i1][1] * radn, 0., col, &rvd, n+1, i1);
			}
			glEnd();
			ocol = col;
			rad = radn;
		}
		glPopMatrix();
	}
	else
		inside = 1;
	/* so close to the ring that its particles can be viewed */
	if(minrad < dir && dir < maxrad && -thick - .01 * (1<<6) < sp && sp < thick + .01 * (1<<6)){
		int inten, ointen;
		avec3_t localight;
		if(rotation){
			mat4tdvp3(localight, mat, light);
		}
		else
			VECCPY(localight, light);
		glPushMatrix();
		if(rotation)
			glMultMatrixd(rotation);
		{
			double rad;
			double f;
			for(rad = minrad; rad < maxrad;){
				double radn;
				radn = rad + width;
				inten = radn < maxrad ? rseq(&rs) % 256 : 0;
				if(dir < rad)
					break;
				ointen = inten;
				rad = radn;
			}

			/* linear interpolation */
			f = (dir - rad + width) / width;
			inten = (inten * f + ointen * (1 - f));
		}
		if(inside){
			double sign = sp / fabs(sp);
			double y, z;
			double (*cuts)[2];
			inside = 1;
			cuts = CircleCuts(16);
			y = sqrt(1 - sp * sp / thick / thick);
			z = sqrt(1 - y * y);
	/*		glRotated(deg_per_rad * pitch, 1., 0., 0.);*/
			{
				avec3_t v;
				GLubyte bri;
				glBegin(GL_QUAD_STRIP);
				for(i = 0; i <= 16; i++){
					int i1 = i % 16;
					v[0] = cuts[i1][0];
					v[1] = cuts[i1][1];
					v[2] = 0.;
					bri = 32 + (-VECSP(v, localight) + 1.) * .5 * 127;
					glColor4ub(bri,bri,bri,inten);
					glVertex3d(cuts[i1][0], cuts[i1][1], 0.);
					v[0] = cuts[i1][0] * z;
					v[1] = cuts[i1][1] * z;
					v[2] = sign * y;
					bri = 32 + (-VECSP(v, localight) + 1.) * .5 * 127;
					glColor4ub(bri,bri,bri,0);
		/*			glVertex3d(cuts[i1][0], cuts[i1][1], thick / sp - sign);*/
					glVertex3d(cuts[i1][0] * z, cuts[i1][1] * z, sign * y);
				}
				glEnd();
				glBegin(GL_TRIANGLE_FAN);
				v[0] = 0.;
				v[1] = 0.;
				v[2] = -sign;
				bri = 32 + (-VECSP(v, localight) + 1.) * .5 * 127;
				glColor4ub(bri,bri,bri, sp * sign / thick * inten / 2);
				glVertex3d(0., 0., -sign);
				for(i = 0; i <= 16; i++){
					int i1 = i % 16;
					v[0] = cuts[i1][0];
					v[1] = cuts[i1][1];
					v[2] = 0.;
					bri = 32 + (-VECSP(v, localight) + 1.) * .5 * 127;
					glColor4ub(bri,bri,bri,inten);
					glVertex3d(cuts[i1][0], cuts[i1][1], 0.);
				}
				glEnd();
			}
		}
		if(start == 0)
		{
			int i, count = inten / 4, total = 0;
			struct random_sequence rs;
/*			timemeas_t tm;*/
			const double destvelo = .01; /* destination velocity */
			const double width = .00002 * 1000. / gvp.m; /* line width changes by the viewport size */
			double plpos[3];
			GLfloat light_position[4] /*= { sunpos[0], sunpos[1], sunpos[2], 0.0 }*/;
			avec3_t nh0 = {0., 0., -1.}, nh;
			avec3_t vwpos;
			amat4_t mat;
			amat4_t proj;
			int level;
			mat4tdvp3(vwpos, rotation, delta);
/*			TimeMeasStart(&tm);*/
			{
				amat4_t modelmat, persmat;
				glGetDoublev(GL_MODELVIEW_MATRIX, modelmat);
				glGetDoublev(GL_PROJECTION_MATRIX, persmat);
				mat4mp(mat, persmat, modelmat);
			}
			MAT4VP3(nh, vw->irot, nh0);
			glPushMatrix();
/*			glLoadMatrixd(vw->rot);*/
		/*	glTranslated(-(*view)[0], -(*view)[1], -(*view)[2]);*/

			/* restore original projection matrix temporarily to make depth
			  buffer make sense. 'relative' projection frustum has near clipping
			  plane with so small z value that the precision drops so hard. */
			glGetDoublev(GL_PROJECTION_MATRIX, proj);
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);

			glPushAttrib(GL_DEPTH_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(1);
			LightOn(NULL);
			VECCPY(light_position, localight);
			light_position[3] = 0.;
			glLightfv(GL_LIGHT0, GL_POSITION, light_position);
			for(level = 6; level; level--){
				double cellsize, rocksize = (1<<level) * .0001;
				double rotspeed = .1 / level;
				double disp[3];
				double plpos[3];
				VECCPY(plpos, vw->pos);
				VECSADD(plpos, nh, -rocksize);
				init_rseq(&rs, level + 23342);
				cellsize = .01 * (1<<level);
				if(level == 6){
					glPushAttrib(GL_POINT_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
					glDisable(GL_LIGHTING);
					glDisable(GL_POINT_SMOOTH);
					glPointSize(2);
					glBegin(GL_POINTS);
				}
				for(i = level == 6 ? count * 64 : count; i; i--){
					double pos[3], pyr[3], x0, x1, z0, z1, base;
					unsigned long red;
					pos[0] = vwpos[0] + (drseq(&rs) - .5) * 2 * cellsize;
					pos[0] = /*pl.pos[0] +*/ +(floor(pos[0] / cellsize) * cellsize + cellsize / 2. - pos[0]);
					pos[1] = vwpos[1] + (drseq(&rs) - .5) * 2 * cellsize;
					pos[1] = floor(pos[1] / cellsize) * cellsize + cellsize / 2 - pos[1];
					pos[2] = vwpos[2] + (drseq(&rs) - .5) * 2 * cellsize;
					pos[2] = /*pl.pos[2] +*/ +(floor(pos[2] / cellsize) * cellsize + cellsize / 2. - pos[2]);
					pyr[0] = drseq(&rs) * rotspeed * t;
					pyr[1] = drseq(&rs) * rotspeed * t;
					pyr[2] = drseq(&rs) * rotspeed * t;
					red = rseq(&rs);
		/*			if((-1. < pos[0] && pos[0] < 1. && -1. < pos[2] && pos[2] < 1.
						? -.01 < pos[0] && pos[0] < .01
						: (pos[2] < -.01 || .01 < pos[2])))
						continue;*/
					if(pos[2] + vwpos[2] < -thick || thick < pos[2] + vwpos[2])
						continue;
			#if 1
					{
						avec4_t dst;
						avec4_t vec;
						VECCPY(vec, pos);
						VECSADD(vec, nh, rocksize / g_glcull.fov);
						MAT4VP3(dst, mat, vec);
						if(dst[2] < 0. || dst[0] < -dst[2] || dst[2] < dst[0] || dst[1] < -dst[2] || dst[2] < dst[1])
							continue;
					}
			#endif
					if(level == 6){
						int br;
						br = 128 * (1. - VECSP(pos, localight) / VECLEN(pos)) / 2.;
						br = br < 0 ? 0 : br;
						glColor3ub(br, br, br);
						glVertex3dv(pos);
					}
					else
						drawasteroid(pos, pyr, rocksize, i + level * count);
					total++;
				}
				if(level == 6){
					glEnd();
					glPopAttrib();
				}
			}
			glPopMatrix();
			LightOff();
			glPopAttrib();

			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadMatrixd(proj);
			glMatrixMode(GL_MODELVIEW);

/*			fprintf(stderr, "ring[%3d]: %lg sec\n", total, TimeMeasLap(&tm));*/
		}
		glPopMatrix();
	}
}

#if 0
#include <clib/colseq/cs2x.h>

extern GLubyte g_diffuse[3];

void colorcsxv(struct cs2x_vertex *csxv, const struct cs2x_vertex *g_csxv, int num, double x, double sp, double cen, double clearness, double dimfactor, double dimness, double height, int lightning, GLubyte *g_diffuse){
	int i;
			for(i = 0; i < num; i++){
#if 1
				int air = /*i != 0 &&*/ i != 7 && i != 8 && i != 10 && i != 4;
				double whiteness = i == 0 ? (1. - x) / (1. + 20. * (height * height)) + x : x;
				COLOR32 col = g_csxv[i].c;
				unsigned char r = COLOR32R(col), g = COLOR32G(col), b = COLOR32B(col), a = COLOR32A(col);
				long rr, rg, rb, ra;
				rr = /*clearness * (sp < 0 ? -sp * r / 2 : 0)*/ + dimfactor * (!air ? r : lightning ? g_diffuse[0] : 256 * (1. - whiteness) + r * whiteness) * (96 * cen * cen / 4 + 32) / 256;
				rr = MIN(255, MAX(0, rr));
				rg = clearness * (sp < 1. / 2. ? ((1. / 2. - sp + .1) / (3. / 2. + .1)) * g : g * .1 / (3. / 2. + .1)) + dimfactor * (!air ? g : lightning ? g_diffuse[0] : 256 * (1. - whiteness) + g * whiteness) * (96 * cen * cen / 4 + 32) / 256;
				rg = MIN(255, MAX(0, rg));
				rb = clearness * (sp < 2. / 3. ? ((2. / 3. - sp + .2) / (5. / 3. + .2)) * b : b * .2 / (5. / 3. + .2)) + dimfactor * (!air ? b : lightning ? g_diffuse[0] : 256 * (1. - whiteness) + b * whiteness) * (96 * cen * cen / 4 + 32) / 256;
				rb = MIN(255, MAX(0, rb));
				ra = clearness * (a != 255 ? (sp < 0 ? 1 : (1. - sp)) * a : 255) + dimfactor * (!air ? a : 255 * (dimness) + a * (1. - dimness));
				ra = MIN(255, MAX(0, ra));
				csxv[i].c = COLOR32RGBA(
					rr/*clearness * (sp < 0 ? -sp * COLOR32R(g_csxv[i].c) / 2 : 0) + dimfactor * (!air ? COLOR32R(g_csxv[i].c) : lightning ? g_diffuse[0] : 256 * (1. - whiteness) + COLOR32R(g_csxv[i].c) * whiteness) * (96 * cen * cen / 4 + 32) / 256*/,
					rg/*clearness * (sp < 1. / 2. ? ((1. / 2. - sp + .1) / (3. / 2. + .1)) * COLOR32G(g_csxv[i].c) : COLOR32G(g_csxv[i].c) * .1 / (3. / 2. + .1)) + dimfactor * (!air ? COLOR32R(g_csxv[i].c) : lightning ? g_diffuse[0] : 256 * (1. - whiteness) + COLOR32G(g_csxv[i].c) * whiteness) * (96 * cen * cen / 4 + 32) / 256*/,
					rb,
					ra);
/*				dummyfunc(&g_csxv[i].c);
				dummyfunc(&csxv[i].c);*/
#if 1
/*				printf("earth %d: %u->%ld %u->%ld %u->%ld %u->%ld\n", i, r, rr, g, rg, b, rb, a, ra);*/
#elif 1
				printf("earth %d: %lg %d %d %d %lg %d\n", i, sp, 1/*COLOR32R(g_csxv[i].c)*/, 1/*COLOR32A(g_csxv[i].c)*/, COLOR32A(csxv[i].c), clearness * (COLOR32A(csxv[i].c) != 255 ? (sp < 0 ? 1 : (1. - sp)) * COLOR32A(csxv[i].c) : 255), a);
#endif
#else
				csxv[i].c = COLOR32RGBA(
					(1. - csxv[i].x) * redness/*(1. / (1.2 + 150 * cen * cen * cen * cen))*/ * (i > 1 && i != 7 && i != 8 && i != 10 && i != 4 ? 256 : 128) / 256 + (sp < 0 ? -sp * COLOR32R(csxv[i].c) / 2 : 0),
					sp < 1. / 2. ? ((1. / 2. - sp + .1) / (3. / 2. + .1)) * COLOR32G(csxv[i].c) : COLOR32G(csxv[i].c) * .1 / (3. / 2. + .1),
					sp < 2. / 3. ? ((2. / 3. - sp + .2) / (5. / 3. + .2)) * COLOR32B(csxv[i].c) : COLOR32B(csxv[i].c) * .2 / (5. / 3. + .2),
					COLOR32A(csxv[i].c) != 255 ? (sp < 0 ? 1 : (1. - sp)) * COLOR32A(csxv[i].c) : 255);
#endif
			}
}
#endif
#endif
static int setstarcolor(GLubyte *pr, GLubyte *pg, GLubyte *pb, struct random_sequence *prs, double rvelo, double avelo){
	GLubyte r, g, b;
	int bri, hi;
	{
		double hue, sat, f, p, q, t;
		hue = drseq(prs);
		hue = (hue + drseq(prs)) * .5 + 1. * rvelo / LIGHT_SPEED;
		sat = 1. - (1. - (.25 + drseq(prs) * .5) /** (hue < 0. ? 1. / (1. - hue) : 1. < hue ? 1. / hue : 1.)*/) / (1. + avelo / LIGHT_SPEED);
		bri = int((127 + rseq(prs) % 128) * (hue < 0. ? 1. / (1. - hue) : 1. < hue ? 1. / hue : 1.));
		hue = rangein(hue, 0., 1.);
		hi = (int)floor(hue * 6.);
		f = hue * 6. - hi;
		p = bri * (1 - sat);
		q = bri * (1 - f * sat);
		t = bri * (1 - (1 - f) * sat);
		switch(hi){
			case 0: r = bri, g = t, b = q; break;
			case 1: r = q, g = bri, b = p; break;
			case 2: r = p, g = bri, b = t; break;
			case 3: r = p, g = q, b = bri; break;
			case 4: r = t, g = p, b = bri; break;
			case 5:
			case 6: r = bri, g = p, b = q; break;
			default: r = g = b = bri;
		}
	}
	*pr = r;
	*pg = g;
	*pb = b;
	return bri;
}
static avec3_t bpos;
static amat4_t brot, birot;
static double boffset;
static int btoofar;
static int binside;

#define STARNORMALIZE 1 /* stars are way too far to reach by light speed */

#if STARNORMALIZE
#if 1
static void STN(avec3_t r){
	if(0/*!blackhole_gen*/){
		VECNORMIN(r);
		return;
	}
	else{
	double rad, y;
	int back;
	back = !(0. < VECSP(r, bpos));
	if(!btoofar && !binside && !back){
		avec3_t r1;
		double f;
		MAT4DVP3(r1, brot, r);
		VECNORMIN(r1);
		rad = sqrt(r1[0] * r1[0] + r1[1] * r1[1]);
		if(back)
			rad = 2. - rad;
		y = (binside ? 0. : boffset * boffset / (boffset + rad)) + rad;
		f = back ? rad / y : y / rad;
		r1[0] *= f, r1[1] *= f;
/*		r1[2] = sqrt(1. - r1[0] * r1[0] - r1[1] * r1[1]);*/
		MAT4DVP3(r, birot, r1);
	}
	VECNORMIN(r);
	}
}
#else
#define STN(r) VECNORMIN(r)
#endif
#else
#define STN(r)
#endif

#define STARLIST 0

#ifdef NDEBUG
double g_star_num = 10;
int g_star_cells = 6;
double g_star_visible_thresh = .3;
double g_star_glow_thresh = .2;
#else
double g_star_num = 3;
int g_star_cells = 5;
double g_star_visible_thresh = .6;
double g_star_glow_thresh = .5;
#endif

int g_gs_slices = 256;
int g_gs_stacks = 128;
int g_multithread = 1;
int g_gs_always_fine = 0;

#if 0
GLubyte g_galaxy_field[FIELD][FIELD][FIELDZ][4];

static const GLubyte *negate_hyperspace(const GLubyte src[4], Viewer *vw, GLubyte buf[4]){
	{
		int k;
		if(g_invert_hyperspace) for(k = 0; k < 3; k++){
			buf[k] = (LIGHT_SPEED < vw->velolen ? LIGHT_SPEED / vw->velolen : 1.) * src[k];
		}
		else
			memcpy(buf, src, 3);
		buf[3] = MIN(src[3], src[3] * GALAXY_DR / vw->dynamic_range);
		return buf;
	}
}

static void gs_vertex_proc(double x, double y, double z){
	avec3_t r;
	double rad;
	int back;
	r[0] = x;
	r[1] = y;
	r[2] = z;
	back = !(0. < VECSP(r, bpos));
	if(!btoofar && !binside && !back){
		avec3_t r1;
		double f;
		MAT4DVP3(r1, brot, r);
		VECNORMIN(r1);
		rad = sqrt(r1[0] * r1[0] + r1[1] * r1[1]);
		if(back)
			rad = 2. - rad;
		y = (binside ? 0. : boffset * boffset / (boffset + rad)) + rad;
		f = back ? rad / y : y / rad;
		r1[0] *= f, r1[1] *= f;
/*		r1[2] = sqrt(1. - r1[0] * r1[0] - r1[1] * r1[1]);*/
		MAT4DVP3(r, birot, r1);
	}
	VECNORMIN(r);
	glVertex3dv(r);
}

static int draw_gs_proc(
	GLubyte (*colbuf)[HDIV][4],
	const GLubyte (*field)[FIELD][FIELDZ][4],
	const avec3_t *plpos,
	double rayinterval,
	int raysamples,
	int slices,
	int hdiv,
	double (*cuts)[2],
	double (*cutss)[2],
	volatile LONG *quitsignal
){
	int k, n;
	int xi, yi;
		for(xi = 0; xi < slices; xi++) for(yi = 0; yi <= hdiv; yi++){
			avec3_t dir;
			double sum[4] = {0};
			double divider0 = 0.;
			if(quitsignal && *quitsignal)
				return 1;
			dir[0] = cuts[xi][0] * cutss[yi][0], dir[1] = cuts[xi][1] * cutss[yi][0], dir[2] = cutss[yi][1];
			for(n = 0; n < raysamples; n++){
				avec3_t v0;
				int v1[3];
				double f = (n + 1) * rayinterval;
				VECSCALE(v0, dir, f);
				VECSADD(v0, *plpos, FIELD / GALAXY_EXTENT);
/*				VECSADD(v0, solarsystempos, 1. * FIELD);*/
				for(k = 0; k < 3; k++)
					v1[k] = floor(v0[k]);
				if(v1[0] <= -HFIELD && dir[0] < 0 || HFIELD-1 <= v1[0] && 0 < dir[0] || v1[1] <= -HFIELD && dir[1] < 0 || HFIELD-1 <= v1[1] && 0 < dir[1] || v1[2] <= -HFIELD && dir[2] < 0 || HFIELD-1 <= v1[2] && 0 < dir[2])
					break;
				if(-HFIELD < v1[0] && v1[0] < HFIELD-1 && -HFIELD < v1[1] && v1[1] < HFIELD-1 && -HFIELDZ < v1[2] && v1[2] < HFIELDZ-1){
					int xj, yj, zj;

					/* weigh closer samples to enable dark nebulae to obscure background starlights. */
					double divider1 = 1. / (1. + rayinterval * sum[3] / 512.);
					divider0 += divider1;
					for(k = 0; k < 4; k++){
						for(xj = 0; xj <= 1; xj++) for(yj = 0; yj <= 1; yj++) for(zj = 0; zj <= 1; zj++){
							sum[k] += (k == 3 ? 1. : divider1) * field[v1[0] + HFIELD + xj][v1[1] + HFIELD + yj][v1[2] + HFIELDZ + zj][k]
								* (xj ? v0[0] - v1[0] : 1. - (v0[0] - v1[0]))
								* (yj ? v0[1] - v1[1] : 1. - (v0[1] - v1[1]))
								* (zj ? v0[2] - v1[2] : 1. - (v0[2] - v1[2]));
						}
					}
	/*						colbuf[xi][yi][k] = field[v1[0] + 64][v1[1] + 64][v1[2] + 16][k] * (1. - (v0[0] - v1[0])) * (1. - (v0[1] - v1[1]))
						+ field[v1[0] + 1 + 64][v1[1] + 64][v1[2] + 16][k] * ((v0[0] - v1[0])) * (1. - (v0[1] - v1[1]))
						+ field[v1[0] + 1 + 64][v1[1] + 1 + 64][v1[2] + 16][k] * ((v0[0] - v1[0])) * ((v0[1] - v1[1]))
						+ field[v1[0] + 64][v1[1] + 64][v1[2] + 1 + 16][k] * (1. - (v0[0] - v1[0])) * ((v0[1] - v1[1]));*/
				}
	/*						else
					VEC4NULL(colbuf[xi][yi], cblack);*/
			}
			for(k = 0; k < 4; k++){
				double divider = (divider0 / 4. + 128.) / rayinterval;
				colbuf[xi][yi][k] = (sum[k] / divider < 256 ? sqrt(sum[k] / divider / 256.) * 255 : 255);
			}
		}
		return 0;
}

struct draw_gs_fine_thread_data{
	GLubyte (*field)[FIELD][FIELDZ][4];
	GLubyte (*colbuf)[HDIV][4];
	Viewer *vw;
	double (*cuts)[2], (*cutss)[2];
	avec3_t plpos;
	avec3_t *solarsystempos;
/*	int lod;*/
	volatile LONG *threaddone;
	volatile LONG threadstop;
	HANDLE drawEvent;
};

static DWORD WINAPI draw_gs_fine_thread(struct draw_gs_fine_thread_data *dat){
	while(WAIT_OBJECT_0 == WaitForSingleObject(dat->drawEvent, INFINITE)){
		GLubyte (*field)[FIELD][FIELDZ][4] = dat->field;
		GLubyte (*colbuf)[HDIV][4] = dat->colbuf;
		Viewer *vw = dat->vw;
		int k, n;
/*		volatile lod = dat->lod;*/
		double *solarsystempos = dat->solarsystempos;
		int slices = SLICES, hdiv = HDIV;
		int xi, yi;
		double (*cuts)[2], (*cutss)[2];
		cuts = dat->cuts;
		cutss = dat->cutss;
/*		if(lod)
			slices = SLICES * 2, hdiv = HDIV * 2;*/
recalc:
		if(draw_gs_proc(colbuf, field, dat->plpos, .5, 1024, slices, hdiv, cuts, cutss, &dat->threadstop)){
			InterlockedExchange(&dat->threadstop, 0);
			goto recalc;
		}
		InterlockedExchange(dat->threaddone, 1);
	}
	return 0;
}

static void perlin_noise(GLubyte (*field)[FIELD][FIELDZ][4], GLubyte (*work)[FIELD][FIELDZ][4], long seed){
	int octave;
	struct random_sequence rs;
	init_rseq(&rs, seed);
	for(octave = 0; (1 << octave) < FIELDZ; octave += 5){
		int cell = 1 << octave;
		int xi, yi, zi;
		int k;
		for(zi = 0; zi < FIELDZ / cell; zi++) for(xi = 0; xi < FIELD / cell; xi++) for(yi = 0; yi < FIELD / cell; yi++){
			int base;
			base = rseq(&rs) % 64 + 191;
			for(k = 0; k < 4; k++)
				work[xi][yi][zi][k] = /*rseq(&rs) % 32 +*/ base;
		}
		if(octave == 0)
			memcpy(field, work, FIELD * FIELD * FIELDZ * 4);
		else for(zi = 0; zi < FIELDZ; zi++) for(xi = 0; xi < FIELD; xi++) for(yi = 0; yi < FIELD; yi++){
			int xj, yj, zj;
			int sum[4] = {0};
			for(k = 0; k < 4; k++){
				for(xj = 0; xj <= 1; xj++) for(yj = 0; yj <= 1; yj++) for(zj = 0; zj <= 1; zj++){
					sum[k] += (double)work[xi / cell + xj][yi / cell + yj][zi / cell + zj][k]
					* (xj ? xi % cell : (cell - xi % cell - 1)) / (double)cell
					* (yj ? yi % cell : (cell - yi % cell - 1)) / (double)cell
					* (zj ? zi % cell : (cell - zi % cell - 1)) / (double)cell;
				}
				field[xi][yi][zi][k] = MIN(255, field[xi][yi][zi][k] + sum[k] / 2);
			}
		}
	}
}

static void perlin_noise_3d(GLubyte (*field)[FIELD][FIELDZ][4], long seed){
	int octave;
	int octaves = 6;
	struct random_sequence rs;
	double (*buf)[FIELD][FIELD][FIELDZ][4];
	int xi, yi, zi;
	int k;
	init_rseq(&rs, seed);
	buf = malloc(octaves * sizeof *buf);
	for(octave = 0; octave < octaves; octave ++){
		int cell = 1 << octave;
		int res = FIELD / cell;
		int zres = FIELDZ / cell;
		printf("octave %d\n", octave);
		for(xi = 0; xi < res; xi++) for(yi = 0; yi < res; yi++) for(zi = 0; zi < zres; zi++){
			int base;
			base = rseq(&rs) % 128;
			for(k = 0; k < 4; k++)
				buf[octave][xi][yi][zi][k] = (rseq(&rs) % 64 + base) / (double)octaves / (double)(octaves - octave);
		}
	}
	for(xi = 0; xi < FIELD; printf("xi %d\n", xi), xi++) for(yi = 0; yi < FIELD; yi++) for(zi = 0; zi < FIELD; zi++){
		int xj, yj, zj;
		double sum[4] = {0};
		for(k = 0; k < 4; k++) for(octave = 0; octave < octaves; octave ++){
			int cell = 1 << octave;
			int res = FIELD / cell;
			int zres = FIELDZ / cell;
			for(xj = 0; xj <= 1; xj++) for(yj = 0; yj <= 1; yj++) for(zj = 0; zj <= 1; zj++){
				sum[k] += (double)buf[octave][(xi / cell + xj) % res][(yi / cell + yj) % res][(zi / cell + zj) % zres][k]
				* (xj ? xi % cell : (cell - xi % cell - 1)) / (double)cell
				* (yj ? yi % cell : (cell - yi % cell - 1)) / (double)cell
				* (zj ? zi % cell : (cell - zi % cell - 1)) / (double)cell;
			}
			field[xi][yi][zi][k] = MIN(255, field[xi][yi][zi][k] + sum[k] / 2);
		}
	}
	free(buf);
}

static int ftimecmp(const char *file1, const char *file2){
	WIN32_FILE_ATTRIBUTE_DATA fd, fd2;
	BOOL b1, b2;

	b1 = GetFileAttributesEx(file1, GetFileExInfoStandard, &fd);
	b2 = GetFileAttributesEx(file2, GetFileExInfoStandard, &fd2);

	/* absent file is valued oldest */
	if(!b1 && !b2)
		return 0;
	if(!b1)
		return -1;
	if(!b2)
		return 1;
	return (int)CompareFileTime(&fd.ftLastWriteTime, &fd2.ftLastWriteTime);
}

static const avec3_t solarsystempos = {-0, -0, -.00};
int g_galaxy_field_cache = 1;

unsigned char galaxy_set_star_density(Viewer *vw, unsigned char c);

#define DARKNEBULA 16

static void draw_gs(struct coordsys *csys, Viewer *vw){
	static GLubyte (*field)[FIELD][FIELDZ][4] = g_galaxy_field;
	static int field_init = 0;
	static GLuint spheretex = 0;
	static GLuint spheretexs[4] = {0};
	if(!field_init){
		FILE *fp;
		FILE *ofp;
		timemeas_t tm;
		TimeMeasStart(&tm);
		field_init = 1;
		if(ftimecmp("ourgalaxy3.raw", "cache/ourgalaxyvol.raw") < 0){
			fp = fopen("cache/ourgalaxyvol.raw", "rb");
			fread(field, sizeof g_galaxy_field, 1, fp);
			fclose(fp);
		}
		else{
		struct random_sequence rs;
		int xi, yi, zi, zzi, xj, yj, zj;
		int k, n;
		int srcx, srcy;
		GLubyte (*field2)[FIELD][FIELDZ][4];
		GLfloat (*src)[4];
		GLfloat darknebula[DARKNEBULA][DARKNEBULA];
		field2 = malloc(sizeof g_galaxy_field);
#if 1
		fp = fopen("ourgalaxy3.raw", "rb");
		if(!fp)
			return;
		srcx = srcy = 512;
		src = (GLfloat*)malloc(srcx * srcy * sizeof *src);
		for(xi = 0; xi < srcx; xi++) for(yi = 0; yi < srcy; yi++){
			unsigned char c;
			fread(&c, 1, sizeof c, fp);
			src[xi * srcy + yi][2] = c / 256.f /*pow(c / 256.f, 2.)*/;
			fread(&c, 1, sizeof c, fp);
			src[xi * srcy + yi][1] = c / 256.f /*pow(c / 256.f, 2.)*/;
			fread(&c, 1, sizeof c, fp);
			src[xi * srcy + yi][0] = c / 256.f /*pow(c / 256.f, 2.)*/;
			src[xi * srcy + yi][3] = (src[xi * srcy + yi][0] + src[xi * srcy + yi][1] + src[xi * srcy + yi][2]) / 1.;
/*			c = src[xi * srcy + yi] * 255;
			fwrite(&c, 1, sizeof c, ofp);*/
		}
#else
		fp = fopen("ourgalaxy_model.dat", "rb");
		if(!fp)
			return;
		fread(&srcx, 1, sizeof srcx, fp);
		fread(&srcy, 1, sizeof srcy, fp);
		src = (GLfloat*)malloc(srcx * srcy * sizeof *src);
		for(xi = 0; xi < srcx; xi++) for(yi = 0; yi < srcy; yi++){
			unsigned char c;
			fread(&src[xi * srcy + yi], 1, sizeof *src, fp);
/*			c = src[xi * srcy + yi] * 255;
			fwrite(&c, 1, sizeof c, ofp);*/
		}
#endif
		CmdPrintf("draw_gs.load: %lg sec", TimeMeasLap(&tm));
		perlin_noise(field2, field, 3522526);
		fclose(fp);
		init_rseq(&rs, 35229);
/*		for(zzi = 0; zzi < 16; zzi++){
		int zzz = 1;
		char sbuf[64];
		sprintf(sbuf, "gal%d.raw", zi);
		ofp = fopen(sbuf, "wb");
		for(zi = 16 - zzi; zzi && zi <= 16 + zzi; zi += zzi * 2, zzz -= 2)*/
		CmdPrintf("draw_gs.noise: %lg sec", TimeMeasLap(&tm));
		for(xi = 0; xi < DARKNEBULA; xi++) for(yi = 0; yi < DARKNEBULA; yi++){
			darknebula[xi][yi] = (drseq(&rs) - .5) + (drseq(&rs) - .5);
		}
		CmdPrintf("draw_gs.nebula: %lg sec", TimeMeasLap(&tm));
		for(zi = 0; zi < FIELDZ; zi++){
		for(xi = 0; xi < FIELD; xi++) for(yi = 0; yi < FIELD; yi++){
			double z0;
			double sdz;
			double sd;
			double dxy, dz;
			double dellipse;
			double srcw;
			int xj, yj;
			int weather = ABS(zi - HFIELDZ) * 4;
			int weathercells = 0;
			z0 = 0.;
			if(xi / (FIELD / DARKNEBULA) < DARKNEBULA-1 && yi / (FIELD / DARKNEBULA) < DARKNEBULA-1) for(xj = 0; xj <= 1; xj++) for(yj = 0; yj <= 1; yj++){
				int cell = FIELD / DARKNEBULA;
				z0 += (darknebula[xi / cell + xj][yi / cell + yj]
					* (xj ? xi % cell : (cell - xi % cell - 1)) / (double)cell
					* (yj ? yi % cell : (cell - yi % cell - 1)) / (double)cell
				/*+ (drseq(&rs) - .5)*/) * FIELDZ * .10;
			}
			sdz = (zi + z0 - HFIELDZ) * (zi + z0 - HFIELDZ) / (double)(HFIELDZ * HFIELDZ);
			sd = ((double)(xi - HFIELD) * (xi - HFIELD) / (HFIELD * HFIELD) + (double)(yi - HFIELD) * (yi - HFIELD) / (HFIELD * HFIELD) + sdz);
			srcw = 0.;
			weather = ABS(zi - HFIELDZ) * 4;
			weathercells = 0;
/*			sdz *= drseq(&rs) * .5 + .5;*/
			dxy = sqrt((double)(xi - HFIELD) * (xi - HFIELD) / (HFIELD * HFIELD) + (double)(yi - HFIELD) * (yi - HFIELD) / (HFIELD * HFIELD));
			dz = sqrt(sdz);
			dellipse = sqrt((double)(xi - HFIELD) * (xi - HFIELD) / (HFIELD * HFIELD / 3 / 3) + (double)(yi - HFIELD) * (yi - HFIELD) / (HFIELD * HFIELD / 3 / 3) + sdz);
/*			double phase = atan2(xi - 64, yi - 64);
			double sss;
			sss = (sin(phase * 5. + dxy * 15.) + 1.) / 2.;
			field2[xi][yi][zi][k] = (sd < 1. ? drseq(&rs) * (1. - sd) : 0.) * (k == 2 ? 192 : 255) * (k == 3 ? 255 : sdz * 255);*/
			if(src[xi * srcx / FIELD * srcy + yi * srcy / FIELD][3] == 0.){
				memset(field2[xi][yi][zi], 0, sizeof (char[4]));
			}
			else if(1. < dxy || (1. - dxy - dz) < 0.){
				memset(field2[xi][yi][zi], 0, sizeof (char[4]));
			}
			else{
				for(k = 0; k < 4; k++){
	#if 1
					srcw = src[xi * srcx / FIELD * srcy + yi * srcy / FIELD][k] * 1;
					srcw = MIN(1., srcw);
	#else
					double sub = 0.;
					for(xj = -1; xj <= 1; xj++) for(yj = -1; yj <= 1; yj++) if(xj != 0 && yj != 0){
						int xk = (xi * srcx / FIELD + xj), yk = (yi * srcy / FIELD + yj);
						sub += 0 <= xk && xk < srcx && 0 <= yk && yk < srcy ? 1. - src[xk * srcy + yk] : 0.;
		/*				srcw += 0 <= xi + xj && xi + xj < srcx && 0 <= yi + yj && yi + yj < srcy ? src[(xi + xj) * srcx / FIELD * srcy + (yi + yj) * srcy / FIELD] : 0.;*/
						weathercells++;
					}
					srcw = (zzi == 0 ? src[xi * srcx / FIELD * srcy + yi * srcy / FIELD] : field2[xi][yi][zi+zzz][k] / 256.) - (weathercells ? sub / weathercells : 0);
					if(srcw < 0.)
						srcw = 0.;
	#endif
					field2[xi][yi][zi][k] = field2[xi][yi][zi][k] * ((/*(drseq(&rs) .5 + .5) **/ srcw) * (k == 3 ? ((1. - dxy - dz)) / 2 : (dz)));
				}
			}
			if(dellipse < 1.){
				field2[xi][yi][zi][0] = MIN(255, field2[xi][yi][zi][0] + 256 * (1. - dellipse));
				field2[xi][yi][zi][1] = MIN(255, field2[xi][yi][zi][1] + 256 * (1. - dellipse));
				field2[xi][yi][zi][2] = MIN(255, field2[xi][yi][zi][2] + 128 * (1. - dellipse));
				field2[xi][yi][zi][3] = MIN(255, field2[xi][yi][zi][3] + 128 * (1. - dellipse));
			}
/*			fwrite(&field2[xi][yi][zi], 1, 3, ofp);*/
		}
/*		fclose(ofp);*/
		}
#if 0
		for(zi = 1; zi < FIELDZ-1; zi++){
		char sbuf[64];
		for(n = 0; n < 1; n++){
			GLubyte (*srcf)[FIELD][FIELDZ][4] = n % 2 == 0 ? field2 : field;
			GLubyte (*dstf)[FIELD][FIELDZ][4] = n % 2 == 0 ? field : field2;
			for(xi = 1; xi < FIELD-1; xi++) for(yi = 1; yi < FIELD-1; yi++){
				int sum[4] = {0}, add[4] = {0};
				for(xj = -1; xj <= 1; xj++) for(yj = -1; yj <= 1; yj++) for(zj = -1; zj <= 1; zj++) for(k = 0; k < 4; k++){
					sum[k] += srcf[xi+xj][yi+yj][zi+zj][k];
					add[k]++;
				}
				for(k = 0; k < 4; k++)
					dstf[xi][yi][zi][k] = sum[k] / add[k]/*(3 * 3 * 3)*/;
			}
		}
/*		sprintf(sbuf, "galf%d.raw", zi);
		ofp = fopen(sbuf, "wb");
		for(xi = 0; xi < FIELD; xi++) for(yi = 0; yi < FIELD; yi++){
			fwrite(&(n % 2 == 0 ? field : field2)[xi][yi][zi], 1, 3, ofp);
		}
		fclose(ofp);*/
		}
#else
		memcpy(field, field2, sizeof g_galaxy_field);
#endif
		galaxy_set_star_density(vw, 64);
		if(g_galaxy_field_cache){
#ifdef _WIN32
			if(GetFileAttributes("cache") == -1)
				CreateDirectory("cache", NULL);
#else
			mkdir("cache");
#endif
			fp = fopen("cache/ourgalaxyvol.raw", "wb");
			fwrite(field, sizeof g_galaxy_field, 1, fp);
			fclose(fp);
		}
		free(field2);
		free(src);
		}
		glGenTextures(1, &spheretex);
		glBindTexture(GL_TEXTURE_2D, spheretex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SLICES, SLICES, 0, GL_RGB, GL_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		CmdPrintf("draw_gs: %lg sec", TimeMeasLap(&tm));
	}

	{
		int xi, yi;
		double (*cuts)[2], (*cutss)[2];
		GLubyte cblack[4] = {0};
		static GLubyte colbuf[SLICES][HDIV][4];
		static GLubyte finecolbuf[SLICES][HDIV][4];
		static avec3_t lastpos = {1e30};
		static int flesh = 0, firstload = 0, firstwrite = 0;
		static HANDLE ht;
		static DWORD tid = 0;
		static HANDLE drawEvent;
		static int threadrun = 0;
		static volatile LONG threaddone = 0;
		static Viewer svw;
		int recalc, reflesh, detail;
		int slices, hdiv;
		int firstloaded = 0;
		avec3_t plpos;
		FILE *fp;

		tocs(plpos, csys, vw->pos, vw->cs);
		reflesh = (GALAXY_EXTENT * GALAXY_EPSILON / FIELD) * (GALAXY_EXTENT * GALAXY_EPSILON / FIELD) < VECSDIST(lastpos, plpos);
		if(!firstload){
			firstload = 1;
			if(ftimecmp("cache/galaxy.bmp", "ourgalaxy3.raw") > 0 && (fp = fopen("cache/galaxy.bmp", "rb"))){
				BITMAPFILEHEADER fh;
				BITMAPINFOHEADER bi;
				fread(&fh, 1, sizeof(BITMAPFILEHEADER), fp);
				fread(&bi, 1, sizeof bi, fp);
				fread(finecolbuf, 1, bi.biSizeImage, fp);
				fclose(fp);
				reflesh = 0;
				firstloaded = firstwrite = 1;
				VECCPY(lastpos, plpos);
			}
		}
		flesh = (flesh << 1) | reflesh;
		detail = g_gs_always_fine || !(flesh & 0xfff);
		if(g_multithread){
			static struct draw_gs_fine_thread_data dat;
			if(!ht){
				dat.drawEvent = drawEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
				ht = CreateThread(NULL, 0, draw_gs_fine_thread, &dat, 0, &tid);
			}
			if(!threadrun && (flesh & 0x1fff) == 0x1000){
				dat.field = field;
				dat.colbuf = finecolbuf;
				dat.vw = &svw; svw = *vw;
				dat.threaddone = &threaddone;
				dat.threadstop = 0;
				VECCPY(dat.plpos, plpos);
				dat.solarsystempos = &solarsystempos;
				dat.cuts = CircleCuts(SLICES);
				dat.cutss = CircleCuts(HDIV * 2);
				threaddone = 0;
				SetEvent(dat.drawEvent);
				threadrun = 1;
			}
			if(threadrun && (flesh & 0x1fff) == 0x1000){
				VECCPY(dat.plpos, plpos);
				InterlockedExchange(&dat.threadstop, 1);
			}
			recalc = reflesh;
		}
		else if(g_gs_always_fine){
			recalc = reflesh;
			detail = 1;
		}
		else{
			recalc = ((flesh & 0x1fff) == 0x1000) || reflesh;
		}
		if(!detail)
			slices = SLICES1, hdiv = HDIV1;
		else
			slices = SLICES2, hdiv = HDIV2;
		if(firstloaded || threadrun && threaddone){
			extern int g_reflesh_cubetex;
			memcpy(colbuf, finecolbuf, sizeof colbuf);
			glBindTexture(GL_TEXTURE_2D, spheretex);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SLICES, HDIV, GL_RGBA, GL_UNSIGNED_BYTE, finecolbuf);
#if 1
			if(!firstwrite && ftimecmp("cache/galaxy.bmp", "ourgalaxy3.raw") < 0 && (fp = fopen("cache/galaxy.bmp", "wb"))){
				BITMAPFILEHEADER fh;
				BITMAPINFOHEADER bi;
				((char*)&fh.bfType)[0] = 'B';
				((char*)&fh.bfType)[1] = 'M';
				fh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof bi + SLICES * HDIV * 4;
				fh.bfReserved1 = fh.bfReserved2 = 0;
				fh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
				fwrite(&fh, 1, sizeof(BITMAPFILEHEADER), fp);
				bi.biSize = sizeof bi;
				bi.biWidth = SLICES;
				bi.biHeight = HDIV;
				bi.biPlanes = 1;
				bi.biBitCount = 32;
				bi.biCompression = BI_RGB;
				bi.biSizeImage = bi.biWidth * bi.biHeight * 4;
				bi.biXPelsPerMeter = bi.biYPelsPerMeter = 0;
				bi.biClrUsed = bi.biClrImportant = 0;
				fwrite(&bi, 1, sizeof bi, fp);
				fwrite(finecolbuf, 1, bi.biSizeImage, fp);
				fclose(fp);
				firstwrite = 1;
			}
#endif
			detail = 1;
			slices = SLICES2, hdiv = HDIV2;
			g_reflesh_cubetex = 1;
			threadrun = 0;
		}
		if(threadrun && !threaddone){
			detail = 0;
			slices = SLICES1, hdiv = HDIV1;
		}
		cuts = CircleCuts(slices);
		cutss = CircleCuts(hdiv * 2);
		if(!(threadrun && threaddone) && recalc){
#if 1
			if(detail){
				draw_gs_proc(colbuf, field, plpos, .5, 1024, SLICES, HDIV, CircleCuts(SLICES), CircleCuts(HDIV * 2), NULL);
				glBindTexture(GL_TEXTURE_2D, spheretex);
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SLICES, HDIV, GL_RGBA, GL_UNSIGNED_BYTE, colbuf);
			}
			else
				draw_gs_proc(colbuf, field, plpos, 4., 128, slices, hdiv, cuts, cutss, NULL);
#else
			for(xi = 0; xi < slices; xi++) for(yi = 0; yi <= hdiv; yi++){
				avec3_t v0;
				int v1[3];
				int k, n;
				double sum[4] = {0};
				for(n = 0; n < FIELDZ; n++){
					double f = (n + 1) * 2;
					v0[0] = cuts[xi][0] * cutss[yi][0] * f, v0[1] = cuts[xi][1] * cutss[yi][0] * f, v0[2] = cutss[yi][1] * f;
					VECSADD(v0, plpos, FIELD / GALAXY_EXTENT);
					VECSADD(v0, solarsystempos, 1. * FIELD);
					for(k = 0; k < 3; k++)
						v1[k] = floor(v0[k]);
					if(-HFIELD < v1[0] && v1[0] < HFIELD-1 && -HFIELD < v1[1] && v1[1] < HFIELD-1 && -HFIELDZ < v1[2] && v1[2] < HFIELDZ-1){
						int xj, yj, zj;
						for(k = 0; k < 4; k++){
							for(xj = 0; xj <= 1; xj++) for(yj = 0; yj <= 1; yj++) for(zj = 0; zj <= 1; zj++){
								sum[k] += field[v1[0] + HFIELD + xj][v1[1] + HFIELD + yj][v1[2] + HFIELDZ + zj][k]
									* (xj ? v0[0] - v1[0] : 1. - (v0[0] - v1[0]))
									* (yj ? v0[1] - v1[1] : 1. - (v0[1] - v1[1]))
									* (zj ? v0[2] - v1[2] : 1. - (v0[2] - v1[2]));
							}
						}
	/*						colbuf[xi][yi][k] = field[v1[0] + 64][v1[1] + 64][v1[2] + 16][k] * (1. - (v0[0] - v1[0])) * (1. - (v0[1] - v1[1]))
							+ field[v1[0] + 1 + 64][v1[1] + 64][v1[2] + 16][k] * ((v0[0] - v1[0])) * (1. - (v0[1] - v1[1]))
							+ field[v1[0] + 1 + 64][v1[1] + 1 + 64][v1[2] + 16][k] * ((v0[0] - v1[0])) * ((v0[1] - v1[1]))
							+ field[v1[0] + 64][v1[1] + 64][v1[2] + 1 + 16][k] * (1. - (v0[0] - v1[0])) * ((v0[1] - v1[1]));*/
					}
	/*						else
						VEC4NULL(colbuf[xi][yi], cblack);*/
				}
				for(k = 0; k < 4; k++){
					static const double divider = 4 * 32.;
					colbuf[xi][yi][k] = (sum[k] / divider < 256 ? sqrt(sum[k] / divider / 256.) * 255 : 255);
				}
			}
#endif
			VECCPY(lastpos, plpos);
		}
		if(detail){
			int slices2 = slices, hdiv2 = hdiv;
			glPushAttrib(GL_TEXTURE_BIT);
			glBindTexture(GL_TEXTURE_2D, spheretex);
			glEnable(GL_TEXTURE_2D);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glBegin(GL_QUADS);
			glColor4ub(255,255,255, MIN(255, 255 * GALAXY_DR / vw->dynamic_range));
			for(xi = 0; xi < slices2; xi++) for(yi = 0; yi < hdiv2; yi++){
				int xi1 = (xi + 1) % slices2, yi1 = yi + 1;
				glTexCoord2d((double)yi / slices2, (double)xi / slices2);
				gs_vertex_proc(cuts[xi][0] * cutss[yi][0], cuts[xi][1] * cutss[yi][0], cutss[yi][1]);
				glTexCoord2d((double)yi / slices2, (double)(xi+1) / slices2);
				gs_vertex_proc(cuts[xi1][0] * cutss[yi][0], cuts[xi1][1] * cutss[yi][0], cutss[yi][1]);
				glTexCoord2d((double)yi1 / slices2, (double)(xi+1) / slices2);
				gs_vertex_proc(cuts[xi1][0] * cutss[yi1][0], cuts[xi1][1] * cutss[yi1][0], cutss[yi1][1]);
				glTexCoord2d((double)yi1 / slices2, (double)xi / slices2);
				gs_vertex_proc(cuts[xi][0] * cutss[yi1][0], cuts[xi][1] * cutss[yi1][0], cutss[yi1][1]);
			}
			glEnd();
			glBindTexture(GL_TEXTURE_2D, 0);
			glPopAttrib();
		}
		else{
			glBegin(GL_QUADS);
			for(xi = 0; xi < slices; xi++) for(yi = 0; yi < hdiv; yi++){
				int xi1 = (xi + 1) % slices, yi1 = yi + 1;
				GLubyte buf[4];
	/*					avec3_t v0;
				int v1[3];
				v0[0] = cuts[xi][0] * cuts[yi][0] * 10, v0[1] = cuts[xi][1] * cuts[yi][0] * 10, v0[2] = cuts[yi][1] * 10;
				VECSADD(v0, vw->pos, 1e-4);
				VECCPY(v1, v0);
				if(-64 < v1[0] && v1[0] < 64 && -64 < v1[1] && v1[1] < 64 && -16 < v1[2] && v1[2] < 16)*/
				{
	/*						glColor4ubv(field[v1[0] + 64][v1[1] + 64][v1[2] + 16]);*/
					glColor4ubv(negate_hyperspace(colbuf[xi][yi], vw, buf));
					gs_vertex_proc(cuts[xi][0] * cutss[yi][0], cuts[xi][1] * cutss[yi][0], cutss[yi][1]);
					glColor4ubv(negate_hyperspace(colbuf[xi1][yi], vw, buf));
					gs_vertex_proc(cuts[xi1][0] * cutss[yi][0], cuts[xi1][1] * cutss[yi][0], cutss[yi][1]);
					glColor4ubv(negate_hyperspace(colbuf[xi1][yi1], vw, buf));
					gs_vertex_proc(cuts[xi1][0] * cutss[yi1][0], cuts[xi1][1] * cutss[yi1][0], cutss[yi1][1]);
					glColor4ubv(negate_hyperspace(colbuf[xi][yi1], vw, buf));
					gs_vertex_proc(cuts[xi][0] * cutss[yi1][0], cuts[xi][1] * cutss[yi1][0], cutss[yi1][1]);
				}
			}
			glEnd();
		}
	}
}

unsigned char galaxy_set_star_density(Viewer *vw, unsigned char c){
	extern struct coordsys *g_galaxysystem;
	struct coordsys *csys = g_galaxysystem;
	avec3_t v0;
	int v1[3];
	int i;
	tocs(v0, csys, vw->pos, vw->cs);
	VECSCALEIN(v0, FIELD / GALAXY_EXTENT);
	VECSADD(v0, solarsystempos, 1. * FIELD);
	v0[0] += FIELD / 2.;
	v0[1] += FIELD / 2.;
	v0[2] += FIELDZ / 2.;
	for(i = 0; i < 3; i++)
		v1[i] = floor(v0[i]);
/*	printf("stardensity(%lg,%lg,%lg)[%d,%d,%d]: %lg\n", v0[0], v0[1], v0[2], v1[0], v1[1], v1[2], );*/
	if(0 <= v1[0] && v1[0] < FIELD-1 && 0 <= v1[1] && v1[1] < FIELD-1 && 0 <= v1[2] && v1[2] < FIELDZ-1){
		int xj, yj, zj;
		for(xj = 0; xj <= 1; xj++) for(yj = 0; yj <= 1; yj++) for(zj = 0; zj <= 1; zj++){
			g_galaxy_field[v1[0] + xj][v1[1] + yj][v1[2] + zj][3] = c
				* (xj ? v0[0] - v1[0] : 1. - (v0[0] - v1[0]))
				* (yj ? v0[1] - v1[1] : 1. - (v0[1] - v1[1]))
				* (zj ? v0[2] - v1[2] : 1. - (v0[2] - v1[2]));
		}
		return c;
	}
	return (0 <= v1[0] && v1[0] < FIELD && 0 <= v1[1] && v1[1] < FIELD && 0 <= v1[2] && v1[2] < FIELDZ ? g_galaxy_field[v1[0]][v1[1]][v1[2]][3] = c : 0);
}

double galaxy_get_star_density_pos(const avec3_t v0){
	int v1[3];
	int i;
	for(i = 0; i < 3; i++)
		v1[i] = floor(v0[i]);
	/* cubic linear interpolation is fairly slower, but this function is rarely called. */
	if(0 <= v1[0] && v1[0] < FIELD-1 && 0 <= v1[1] && v1[1] < FIELD-1 && 0 <= v1[2] && v1[2] < FIELDZ-1){
		int xj, yj, zj;
		double sum = 0;
		for(xj = 0; xj <= 1; xj++) for(yj = 0; yj <= 1; yj++) for(zj = 0; zj <= 1; zj++){
			sum += g_galaxy_field[v1[0] + xj][v1[1] + yj][v1[2] + zj][3]
				* (xj ? v0[0] - v1[0] : 1. - (v0[0] - v1[0]))
				* (yj ? v0[1] - v1[1] : 1. - (v0[1] - v1[1]))
				* (zj ? v0[2] - v1[2] : 1. - (v0[2] - v1[2]));
		}
		return sum / 256.;
	}
	return (0 <= v1[0] && v1[0] < FIELD && 0 <= v1[1] && v1[1] < FIELD && 0 <= v1[2] && v1[2] < FIELDZ ? g_galaxy_field[v1[0]][v1[1]][v1[2]][3] / 256. : 0.);
}

double galaxy_get_star_density(Viewer *vw){
	extern struct coordsys *g_galaxysystem;
	struct coordsys *csys = g_galaxysystem;
	avec3_t v0;
	tocs(v0, csys, vw->pos, vw->cs);
	VECSCALEIN(v0, FIELD / GALAXY_EXTENT);
	VECSADD(v0, solarsystempos, 1. * FIELD);
	v0[0] += FIELD / 2.;
	v0[1] += FIELD / 2.;
	v0[2] += FIELDZ / 2.;
/*	printf("stardensity(%lg,%lg,%lg)[%d,%d,%d]: %lg\n", v0[0], v0[1], v0[2], v1[0], v1[1], v1[2], );*/
	return galaxy_get_star_density_pos(v0);
}
#endif

#define TEXSIZE 64
static GLuint drawStarTexture(){
	static GLubyte bits[TEXSIZE][TEXSIZE][2];
	int i, j;
	GLuint texname;
	glGenTextures(1, &texname);
	glBindTexture(GL_TEXTURE_2D, texname);
	for(i = 0; i < TEXSIZE; i++) for(j = 0; j < TEXSIZE; j++){
		int di = i - TEXSIZE / 2, dj = j - TEXSIZE / 2;
		int sdist = di * di + dj * dj;
		bits[i][j][0] = /*255;*/
		bits[i][j][1] = GLubyte(TEXSIZE * TEXSIZE / 2 / 2 <= sdist ? 0 : 255 - 255 * pow((double)(di * di + dj * dj) / (TEXSIZE * TEXSIZE / 2 / 2), 1. / 8.));
	}
	glTexImage2D(GL_TEXTURE_2D, 0, 2, TEXSIZE, TEXSIZE, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, bits);
	return texname;
}

void drawstarback(const Viewer *vw, const CoordSys *csys, const Astrobj *pe, const Astrobj *sun){
	static int init = 0;
	static GLuint listBright, listDark;
	double height;
	const Mat4d &irot = vw->irot;
	static int invokes = 0;
	int drawnstars = 0;
	timemeas_t tm;
	double velolen;
	Vec3d velo;
	Astrobj *blackhole = NULL;

	TimeMeasStart(&tm);

	velo = csys->tocsv(vw->velo, vw->pos, vw->cs);
	velolen = velo.len();

	/* calculate height from the planet's surface */
	if(pe){
		Vec3d epos;
		epos = vw->cs->tocs(pe->pos, pe->parent);
		height = VECDIST(epos, vw->pos) - pe->rad;
	}
	else
		height = 1e10;

	invokes++;
	{
		Mat4d mat, mat2;
/*		avec3_t bpos1, bpos0 = {0., 0., 1e5}, vpos;*/
		glPushMatrix();
/*		tocsm(mat2, vw->cs, csys);
		MAT4TRANSPOSE(mat, mat2);*/
		mat = csys->tocsm(vw->cs);
/*		tocsim(mat, vw->cs, csys);*/
		glMultMatrixd(mat);
	}
/*	{
		int i;
		extern struct astrobj **astrobjs;
		extern int nastrobjs;
		for(i = nastrobjs-1; 0 <= i; i--) if(astrobjs[i]->flags & AO_BLACKHOLE){
			blackhole = astrobjs[i];
			break;
		}
	}

	if(blackhole){
		avec3_t bpos1;
		tocs(bpos1, vw->cs, blackhole->pos, blackhole->cs);
		VECSUBIN(bpos1, vw->pos);
		VECCPY(bpos, bpos1);
	}


	bdist = VECLEN(bpos);
	btoofar = 0;
	if(!blackhole || 1e5 < bdist / blackhole->rad){
		tangent = 0.;
		boffset = 0.;
		btoofar = 1;
	}
	else if(blackhole->rad < bdist){
		tangent = asin(blackhole->rad / bdist);
		boffset = tan(tangent);
		binside = 0;
	}
	else
		binside = 1;

	{
		amat4_t mat, mat2;
		gldLookatMatrix(mat, bpos);
		tocsim(mat2, csys, vw->cs);
		mat4mp(birot, mat2, mat);
		MAT4TRANSPOSE(brot, birot);*/
/*		glPushMatrix();
		directrot(bpos, avec3_000, mat);
		glPopMatrix();
		MAT4CPY(birot, mat);*/
//	}
/*	MAT4TRANSPOSE(brot, birot);*/

/*		glBegin(GL_LINES);
		glColor4ub(0,255,0,255);
		for(i = 0; i < 128; i++){
			int j;
			for(j = 0; j < 128; j++){
				glVertex3d(i - 64 - vw->pos[0] * 1e-4, j - 64 - vw->pos[1] * 1e-4, -16 - vw->pos[2] * 1e-4);
				glVertex3d(i - 64 - vw->pos[0] * 1e-4, j - 64 - vw->pos[1] * 1e-4,  16 - vw->pos[2] * 1e-4);
			}
		}
		for(i = 0; i < 128; i++){
			int j;
			for(j = 0; j < 32; j++){
				glVertex3d(i - 64 - vw->pos[0] * 1e-4,  64 - vw->pos[1] * 1e-4, j - 16 - vw->pos[2] * 1e-4);
				glVertex3d(i - 64 - vw->pos[0] * 1e-4, -64 - vw->pos[1] * 1e-4, j - 16 - vw->pos[2] * 1e-4);
				glVertex3d( 64 - vw->pos[0] * 1e-4, i - 64 - vw->pos[1] * 1e-4, j - 16 - vw->pos[2] * 1e-4);
				glVertex3d(-64 - vw->pos[0] * 1e-4, i - 64 - vw->pos[1] * 1e-4, j - 16 - vw->pos[2] * 1e-4);
			}
		}
		glEnd();*/

/**		glBegin(GL_LINES);
		{
			int xi, yi, zi;
			for(xi = 0; xi < 128; xi++) for(yi = 0; yi < 128; yi++) for(zi = 0; zi < 32; zi++){
				glColor4ub(xi * 256 / 128, yi * 256 / 128, zi * 256 / 32, 255);
				glVertex3d(xi - 64 - vw->pos[0] * 1e-4, yi - 64 - vw->pos[1] * 1e-4, zi - 16 - vw->pos[2] * 1e-4 + .25);
				glVertex3d(xi - 64 - vw->pos[0] * 1e-4, yi - 64 - vw->pos[1] * 1e-4, zi - 16 - vw->pos[2] * 1e-4 - .25);
			}
		}
		glEnd();*/

//		draw_gs(csys, vw);

#if STARLIST
	if(!init)
#endif
	{
		int i;
		struct random_sequence rs;
		static struct cs2x *cs = NULL;
		static struct cs2x_vertex_ex{
			double pos[3];
			COLOR32 c;
		} *ver;
//		FILE *fp;
		double velo;
		init_rseq(&rs, 1);
		velo = vw->relative ? vw->velolen : 0.;
#if STARLIST
		init = 1;
		listBright = glGenLists(2);
		listDark = listBright + 1;

		glNewList(listBright, GL_COMPILE);
#endif

#if 1
#define GLOWTHRESH g_star_glow_thresh
#define VISIBLETHRESH g_star_visible_thresh
#define NUMSTARS g_star_num
#define NUMCELLS g_star_cells
#ifdef NDEBUG
/*#define NUMSTARS 10
#define NUMGLOWSTARS 4500
#define NUMCELLS 6*/
/*#define GLOWTHRESH 3.
#define VISIBLETHRESH .2*/
#else
/*#define NUMSTARS 10
#define NUMGLOWSTARS 2
#define NUMCELLS 5*/
/*#define GLOWTHRESH 6.
#define VISIBLETHRESH .5*/
#endif
		{
		Mat4d relmat;
		int pointstart = 0, glowstart = 0, nearest = 0, cen[3], gx = 0, gy = 0, gz, frameinvokes = 0;
		const double cellsize = 1e13;
		double radiusfactor = .03 * 1. / (1. + 10. / height) * 512. / vw->vp.m;
//		GLcull glc;
		Vec3d gpos;
		Vec3d plpos, npos;
		GLubyte nearest_color[4];
		Vec3d v0;
/*		int v1[3];*/

#if 0
		if(pe){
			double sp;
			avec3_t sunp, earthp, vpos;
			tocs(vpos, vw->cs, vw->pos, vw->cs);
			tocs(sunp, vw->cs, sun->pos, sun->cs);
			tocs(earthp, vw->cs, pe->pos, pe->cs);
			VECSUBIN(sunp, vpos);
			VECSUBIN(earthp, vpos);
			sp = (.5 + VECSP(sunp, earthp) / VECLEN(sunp) / VECLEN(earthp));
			if(sp < 0) sp = 0.;
			else if(1. < sp) sp = 1.;
			radiusfactor = .03 * ((1. - sp) / (1. + 10. / height) + sp) * 512. / gvp.m;
		}
		else{
			radiusfactor = .03 * (2.) * 512. / gvp.m;
		}
#endif

		if(vw->cs == csys){
			plpos = vw->pos / cellsize;
		}
		else{
			plpos = csys->tocs(vw->pos, vw->cs) / cellsize;
		}

		for(i = 0; i < 3; i++){
			cen[i] = (int)floor(plpos[i] + .5);
		}

		glPushMatrix();
		glLoadIdentity();
//		glcullInit(&glc, g_glcull.fov, avec3_000, mat4identity, g_glcull.znear, g_glcull.zfar);
		glPopMatrix();

/*		tocs(v0, csys, vw->pos, vw->cs);
		VECSCALEIN(v0, FIELD / GALAXY_EXTENT);*/
		v0 = vec3_000 * 1. * FIELD;
		v0[0] += FIELD / 2.;
		v0[1] += FIELD / 2.;
		v0[2] += FIELDZ / 2.;
/*		VECCPY(v1, v0);*/
/*		printf("stardensity(%lg,%lg,%lg)[%d,%d,%d]: %lg\n", v0[0], v0[1], v0[2], v1[0], v1[1], v1[2], (0 <= v1[0] && v1[0] < FIELD && 0 <= v1[1] && v1[1] < FIELD && 0 <= v1[2] && v1[2] < FIELDZ ? g_galaxy_field[v1[0]][v1[1]][v1[2]][3] / 256. : 0.));*/

		glPushAttrib(GL_TEXTURE_BIT | GL_POINT_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT);

		for(gx = cen[0] - NUMCELLS; gx <= cen[0] + NUMCELLS; gx++)
		for(gy = cen[1] - NUMCELLS; gy <= cen[1] + NUMCELLS; gy++)
		for(gz = cen[2] - NUMCELLS; gz <= cen[2] + NUMCELLS; gz++){
		int numstars;
		avec3_t v01;
		init_rseq(&rs, (gx + (gy << 5) + (gz << 10)) ^ 0x8f93ab98);
		VECCPY(v01, v0);
		v01[0] = gx * cellsize;
		v01[1] = gy * cellsize;
		v01[2] = gz * cellsize;
/*		VECSCALEIN(v01, FIELD / GALAXY_EXTENT);
		VECADDIN(v01, v0);
		numstars = drseq(&rs) * NUMSTARS * galaxy_get_star_density_pos(v01);*/
		numstars = int(NUMSTARS / (1. + .01 * gz * gz + .01 * (gx + 10) * (gx + 10) + .1 * gy * gy));
		for(i = 0; i < numstars; i++){
			double pos[3], rvelo;
			double radius = radiusfactor;
			GLubyte r, g, b;
			int bri, current_nearest = 0;
			pos[0] = drseq(&rs);
			pos[1] = drseq(&rs); /* cover entire sky */
			pos[2] = drseq(&rs);

		/*	if(vw->cs == &solarsystem)*/{
				double cellsize = 1.;
				pos[0] += gx - .5 - plpos[0];
/*				pos[0] = floor(pos[0] / cellsize) * cellsize + cellsize / 2. - pos[0];*/
				pos[1] += gy - .5 - plpos[1];
/*				pos[1] = floor(pos[1] / cellsize) * cellsize + cellsize / 2. - pos[1];*/
				pos[2] += gz - .5 - plpos[2];
/*				pos[2] = floor(pos[2] / cellsize) * cellsize + cellsize / 2. - pos[2];*/
/*				VECSADD(pos, vw->pos, -1e-14);*/
			}
			radius /= 1. + VECLEN(pos);

#if 1
/*			if(glcullFrustum(&pos, 0., &g_glcull)){
				int j;
				for(j = 0; j < 4; j++)
					rseq(&rs);
			}*/
#else
			{
				double hsv[3];
				r = 127 + rseq(&rs) % 128;
				g = 127 + rseq(&rs) % 128;
				b = 127 + rseq(&rs) % 128;
				rgb2hsv(hsv, r / 256., g / 256., b / 256.);
				bri = hsv[2] * 255;
			}
#endif

			if(VECSLEN(pos) < 1e-6 * 1e-6){
				VECSCALE(npos, pos, 1e14);
				current_nearest = nearest = 1;
				radius /= VECLEN(pos) * 1e6;
			}

			STN(pos);
			rvelo = vw->relative ? VECSP(pos, vw->velo) : 0.;
			frameinvokes++;
			if(GLOWTHRESH < radius * vw->vp.m / vw->fov/* && 1. < gvp.m * radiusfactor * .2*/){
				static GLuint texname = 0, list;
				if(pointstart){
					glEnd();
					pointstart = 0;
				}
				if(!glowstart){
					Mat4d csrot;
					glowstart = 1;
					if(!texname){
						texname = drawStarTexture();
					/*	glNewList(list = glGenLists(1), GL_COMPILE);
						glPushAttrib(GL_TEXTURE_BIT | GL_PIXEL_MODE_BIT);
						glBindTexture(GL_TEXTURE_2D, texname);
						glPixelTransferf(GL_RED_BIAS, color[0] / 256.F);
						glPixelTransferf(GL_GREEN_BIAS, color[1] / 256.F);
						glPixelTransferf(GL_BLUE_BIAS, color[2] / 256.F);
						glEndList();*/
					}
					else{
						glBindTexture(GL_TEXTURE_2D, texname);
					}
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
						GL_NEAREST);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
						GL_NEAREST);
					glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
					glEnable(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D, texname);
					glPushMatrix();
					glGetDoublev(GL_MODELVIEW_MATRIX, relmat);
					csrot = vw->cs->tocsim(csys);
					relmat = vw->relrot * csrot;
				}
				{
					static const GLfloat col[4] = {1., 1., 1., 1.};
					Vec3d local;
					local = relmat.vp3(pos);
					local.normin();
/*					if(!current_nearest && glcullFrustum(local, radius, &glc)){
						int j;
						for(j = 0; j < 4; j++)
							rseq(&rs);
						continue;
					}*/
					bri = setstarcolor(&r, &g, &b, &rs, rvelo, velo);
					radius *= bri / 256.;
					drawnstars++;
/*					col[0] = r / 256.F;
					col[1] = g / 256.F;
					col[2] = b / 256.F;
					col[3] = 1.F;*/
/*					if(g_invert_hyperspace && LIGHT_SPEED < velolen){
						VECSCALEIN(col, GLfloat(LIGHT_SPEED / velolen));
						r *= LIGHT_SPEED / velolen;
						g *= LIGHT_SPEED / velolen;
						b *= LIGHT_SPEED / velolen;
					}*/
					if(current_nearest){
						nearest_color[0] = r;
						nearest_color[1] = g;
						nearest_color[2] = b;
						nearest_color[3] = 255;
					}
					glColor4ub(r, g, b, 255);
					glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, col);
/*					glPushMatrix();*/
/*					glTranslated(pos[0], pos[1], pos[2]);
					glMultMatrixd(irot);
					glScaled(radius, radius, radius);*/
//					printf("%lg %lg %lg\n", local[0], local[1], local[2]);
					glLoadIdentity();
					gldTranslate3dv(local);
					glScaled(radius, radius, radius);
					glBegin(GL_QUADS);
					glTexCoord2i(0, 0); glVertex2i(-1, -1);
					glTexCoord2i(1, 0); glVertex2i( 1, -1);
					glTexCoord2i(1, 1); glVertex2i( 1,  1);
					glTexCoord2i(0, 1); glVertex2i(-1,  1);
					glEnd();
/*					glPopMatrix();*/
				}
/*				gldTextureGlow(pos, .03 * bri / 256, rgb, vw->irot);*/
			}
			else if(VISIBLETHRESH < radius * vw->vp.m / vw->fov){
				double f = radius * vw->vp.m / vw->fov  / GLOWTHRESH;
				if(glowstart){
					glPopMatrix();
					glDisable(GL_TEXTURE_2D);
					glowstart = 0;
				}
				if(!pointstart){
					glBegin(GL_POINTS);
					pointstart = 1;
				}
				bri = setstarcolor(&r, &g, &b, &rs, rvelo, velo);
				if(g_invert_hyperspace && LIGHT_SPEED < vw->velolen){
					r *= LIGHT_SPEED / vw->velolen;
					g *= LIGHT_SPEED / vw->velolen;
					b *= LIGHT_SPEED / vw->velolen;
				}
				glColor4ub(r, g, b, 255 * f * f);
				glVertex3dv(pos);
			}
		}
		}
		if(glowstart){
			glPopMatrix();
		}
		if(pointstart)
			glEnd();
		glPopAttrib();
#endif

#if STARLIST
		glEndList();
#endif

		if(nearest){
			gldPseudoSphere(npos, 1e5, nearest_color);
		}
		}
	}

#if STARLIST
	/* don't ever think about constellations' collapsing if you are so close to the
	  earth. but aberration takes effect if your velocity is so high. */
#if 0 /* old aberration by vector arithmetics */
	if(1e5 < height || LIGHT_SPEED / 100. < pl.velolen){
		const double c = LIGHT_SPEED;
		int i;
		struct random_sequence rs;
		double velo;
		double velo, gamma;

		/* Lorentz transformation */
		velo = VECLEN(pl.velo);
		gamma = sqrt(1 - velo * velo / c / c);

		init_rseq(&rs, 1);

		glBegin(GL_POINTS);
		for(i = 0; i < 500; i++){
			{
				GLubyte r, g, b;
				r = 127 + rseq(&rs) % 128;
				g = 127 + rseq(&rs) % 128;
				b = 127 + rseq(&rs) % 128;
				glColor3ub(r, g, b);
			}
			{
				double r[3], v[3], dist, vlen, theta;
				r[0] = 10. * (drseq(&rs) * 2. - 1.);
				r[1] = drseq(&rs) * 2. - 1.;
				r[2] = drseq(&rs) * 2. - 1.;
				if(velo < LIGHT_SPEED){
					double sp, s1, s2;
					VECNORMIN(r);
					VECSCALE(v, r, c * gamma);
					sp = VECSP(pl.velo, r);
					s1 = 1 + sp / (c * (1. + gamma));
					VECSADD(v, pl.velo, s1);
					s2 = 1. / (c + VECSP(pl.velo, r));
					VECSCALE(v, v, s2);
					glVertex3dv(v);
				}
			}
		}
		glEnd();
	}
#else
	else if(1){
		glCallList(listBright);
		glCallList(listDark);
	}
#endif
	else
		glCallList(listBright);
#endif

	glPopMatrix();

/*	printf("stars[%d]: %lg\n", drawnstars, TimeMeasLap(&tm));*/
#if 0
	if(!vw->relative/*(pl.mover == warp_move || pl.mover == player_tour || pl.chase && pl.chase->vft->is_warping && pl.chase->vft->is_warping(pl.chase, pl.cs->w))*/ && LIGHT_SPEED * LIGHT_SPEED < VECSLEN(velo)){
		int i, count = 1250;
		struct random_sequence rs;
		static double raintime = 0.;
/*		timemeas_t tm;*/
		double cellsize = 1e7;
		double speedfactor;
		const double width = .00002 * 1000. / gvp.m; /* line width changes by the viewport size */
		double epos[3];
		double basebright;
		const double levelfactor = 1.5e3;
		int level = 0;
		Vec3d nh0 = {0., 0., -1.}, nh, plpos, localvelo;
		amat3_t ort3, iort3;
		amat4_t irot, rot;
		GLubyte col[4] = {255, 255, 255, 255};
		static GLuint texname = 0;
		static const GLfloat envcolor[4] = {0,0,0,0};

		init_rseq(&rs, 2413531);
/*		count = raintime / dt < .1 ? 500 * (1. - raintime / dt / .1) * (1. - raintime / dt / .1) : 100;*/
/*		TimeMeasStart(&tm);*/
		tocsm(rot, csys, vw->cs);
		{
			Vec3d gpos;
			gpos = csys->tocs(vw->pos, vw->cs);
//			mat4dvp3(plpos, rot, gpos);
			plpos = gpos;
		}
#if 0
		tocsv(velo, csys, /*pl.chase ? pl.chase->velo :*/ vw->velo, vw->pos, vw->cs);
#elif 1
		VECCPY(localvelo, velo);
#else
		mat4dvp3(localvelo, rot, velo);
#endif
		velolen = VECLEN(localvelo);
		mat4dvp3(nh, vw->irot, nh0);
		glPushMatrix();
		glLoadMatrixd(vw->rot);
		glMultMatrixd(rot);
		glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		if(!texname){
			GLubyte texbits[64][64][2];
			int i, j;
			for(i = 0; i < 64; i++) for(j = 0; j < 64; j++){
				int a, r;
				r = (32 * 32 - (i - 32) * (i - 32) - (j - 32) * (j - 32)) * 255 / 32 / 32;
				texbits[i][j][0] = 256. * sqrt(MIN(255, MAX(0, r)) / 256.)/*256. * sqrt((255 - MIN(255, MAX(0, r))) / 256.)*/;
				r = (32 * 32 - (i - 32) * (i - 32) - (j - 32) * (j - 32)) * 255 / 32 / 32;
				texbits[i][j][1] = 256. * sqrt(MIN(255, MAX(0, r)) / 256.);
			}
			glGenTextures(1, &texname);
			glBindTexture(GL_TEXTURE_2D, texname);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, 64, 64, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, texbits);
		}
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, envcolor);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glBindTexture(GL_TEXTURE_2D, texname);
		glDepthMask(0);
		/*glTranslated*//*(plpos[0] = floor(pl.pos[0] / cellsize) * cellsize, plpos[1] = floor(pl.pos[1] / cellsize) * cellsize, plpos[2] = floor(pl.pos[2] / cellsize) * cellsize);*/

		if(velolen < LIGHT_SPEED * levelfactor);
		else if(velolen < LIGHT_SPEED * levelfactor * levelfactor){
			level = 1;
			cellsize *= levelfactor;
		}
		else{
			level = 2;
			cellsize *= levelfactor * levelfactor;
		}
		speedfactor = velolen / LIGHT_SPEED / (level == 0 ? 1. : level == 1 ? levelfactor : levelfactor * levelfactor);
		basebright = .5 / (.998 + .002 * speedfactor) * (1. - 2. / (speedfactor + 1.));

		VECSCALEIN(localvelo, .025 * (velolen + cellsize / 100. / .025) / velolen);

		for(i = count; i; i--){
			double pos[3], dst[3];
			int red, bright;
			pos[0] = plpos[0] + (drseq(&rs) - .5) * 2 * cellsize;
			pos[0] = floor(pos[0] / cellsize) * cellsize + cellsize / 2. - pos[0];
			pos[1] = plpos[1] + (drseq(&rs) - .5) * 2 * cellsize;
			pos[1] = floor(pos[1] / cellsize) * cellsize + cellsize / 2. - pos[1];
			pos[2] = plpos[2] + (drseq(&rs) - .5) * 2 * cellsize;
			pos[2] = floor(pos[2] / cellsize) * cellsize + cellsize / 2. - pos[2];
/*			pos[0] = pl.pos[0] + (drseq(&rs) - .5) * 2 * cellsize;
			pos[0] = pl.pos[0] + floor(pos[0] / cellsize) * cellsize + cellsize / 2. - pos[0];
			pos[1] = pl.pos[1] + (drseq(&rs) - .5) * 2 * cellsize + warf.war_time * destvelo;
			pos[1] = pl.pos[1] + floor(pos[1] / cellsize) * cellsize + cellsize / 2. - pos[1];
			pos[2] = pl.pos[2] + (drseq(&rs) - .5) * 2 * cellsize;
			pos[2] = pl.pos[2] + floor(pos[2] / cellsize) * cellsize + cellsize / 2. - pos[2];*/
			red = rseq(&rs) % 256;
			VECCPY(dst, pos);
			VECSADD(dst, localvelo, 1);
			VECSADD(pos, localvelo, -1);
			bright = (255 - VECLEN(pos) * 255 / (cellsize / 2.)) * basebright;
			if(0 < bright){
				glColor4ub(level == 0 ? red : 255, level == 1 ? red : 255, level == 2 ? red : 255, bright);
				gldTextureBeam(avec3_000, pos, dst, cellsize / 100. / (1. + speedfactor / levelfactor));
/*				glBegin(GL_LINES);
				glVertex3dv(pos);
				glVertex3dv(dst);
				glEnd();*/
			}
/*			gldGradBeam(avec3_000, pos, dst, width, COLOR32RGBA(0,0,255,0));*/
		}
		glPopAttrib();
		glPopMatrix();
/*		fprintf(stderr, "starrain: %lg\n", raintime = TimeMeasLap(&tm));*/
	}
#endif
}




























void Star::predraw(const Viewer *){
	flags &= ~AO_DRAWAIRCOLONA;
}

void Star::draw(const Viewer *vw){
	COLOR32 col = COLOR32RGBA(255,255,255,255);
	Vec3d gvelo;
	gvelo = parent->tocsv(vw->velo, vw->pos, vw->cs);
	if(LIGHT_SPEED * LIGHT_SPEED < VECSLEN(gvelo)){
		extern int g_invert_hyperspace;
		double velolen;
		velolen = VECLEN(gvelo);
		if(g_invert_hyperspace)
			col = COLOR32SCALE(col, LIGHT_SPEED / velolen * 256) & COLOR32RGBA(255,255,255,0) | COLOR32RGBA(0,0,0,COLOR32A(col));
	}
	drawpsphere(this, vw, col);
	if(/*!((struct sun*)a)->aircolona !(flags & AO_DRAWAIRCOLONA)*/1){
		Astrobj *abest = NULL;
		Vec3d epos, spos;

		/* astrobjs are sorted and the nearest solid celestial body can be
		  easily found without distance examination. */
		CoordSys *eics = findeisystem();
		if(eics){
			bool drawhere = true;
			for(AOList::iterator i = eics->aorder.begin(); i != eics->aorder.end(); i++){
				abest = (*i)->toAstrobj();
				if(abest && abest->sunAtmosphere(*vw)){
					drawhere = false;
					break;
				}
			}
			if(drawhere){
				drawsuncolona(this, vw);
			}
		}
	}
	st::draw(vw);
}

/* show flat disk that has same apparent radius as if it were a sphere. */
void drawpsphere(Astrobj *ps, const Viewer *vw, COLOR32 col){
	int i;
	Vec3d plpos, pspos, delta;
	double sdist, scale;
	plpos = vw->cs->tocs(vw->pos, vw->cs);
	pspos = vw->cs->tocs(ps->pos, ps->parent);
	delta = pspos - plpos;
#if 0
	if(glcullFrustum(&pspos/*&delta*/, ps->rad, &g_glcull))
		return;
#endif
	sdist = (pspos - plpos).slen();
	scale = ps->rad * 1/*glcullScale(pspos, &g_glcull)*/;
	if(scale * scale < .1 * .1)
		return;
	else if(scale * scale < 2. * 2.){
/*		dist = sqrt(sdist);
		glGetIntegerv(GL_VIEWPORT, vp);*/
		glPushAttrib(GL_POINT_BIT);
/*		glEnable(GL_POINT_SMOOTH);*/
		glPointSize(float(scale * 2.));
		glColor4ub(COLIST(col));
		glBegin(GL_POINTS);
/*		glVertex3d((pspos[0] - plpos[0]) / dist, (pspos[1] - plpos[1]) / dist, (pspos[2] - plpos[2]) / dist);*/
		glVertex3dv(delta);
		glEnd();
		glPopAttrib();
	}
	else if(ps->rad * ps->rad < sdist){
		int n;
		double dist, as, cas, sas;
		double (*cuts)[2];
		double x = pspos[0] - plpos[0], z = pspos[2] - plpos[2], phi, theta;
		dist = sqrt(sdist);
		as = asin(sas = ps->rad / dist);
		cas = cos(as);
		{
			double x = sas - .5;
			n = (int)(32*(-x * x / .5 / .5+ 1.)) + 8;
		}
/*		n = (int)(16*(-(1. - sas) * (1. - sas) + 1.)) + 5;*/
/*		n = (int)(16*(-(as - M_PI / 4.) * (as - M_PI / 4.) / M_PI * 4. / M_PI * 4. + 1.)) + 5;*/
		phi = atan2(x, z);
		theta = atan2((pspos[1] - plpos[1]), sqrt(x * x + z * z));
		cuts = CircleCuts(n);
		glPushMatrix();
		glRotated(phi * 360 / 2. / M_PI, 0., 1., 0.);
		glRotated(theta * 360 / 2. / M_PI, -1., 0., 0.);
		glColor4ub(COLIST(col));
		glBegin(GL_POLYGON);
		for(i = 0; i < n; i++)
			glVertex3d(cuts[i][0] * sas, cuts[i][1] * sas, cas);
		glEnd();
		glPopMatrix();
	}
}

void drawsuncolona(Astrobj *a, const Viewer *vw){
	int i;
	GLubyte white[4] = {255, 255, 255, 255};
	double height;
	double sdist;
	double dist, as, cas, sas;
	double (*cuts)[2];
	Vec3d vpos, epos, spos;
	double x, z, phi, theta, f;
	Astrobj *abest = NULL;

	/* astrobjs are sorted and the nearest celestial body with atmosphere can be
	  easily found without distance examination. */
	CoordSys *eics = a->findeisystem();
	for(Astrobj::AOList::iterator i = eics->aorder.begin(); i != eics->aorder.end(); i++){
		Astrobj *a = (*i)->toAstrobj();
		if(a && a->flags & AO_ATMOSPHERE){
			abest = a;
			break;
		}
	}
	spos = a->calcPos(*vw);
	vpos = vw->pos;
	if(abest){
		epos = abest->calcPos(*vw);
		height = (epos - vpos).len() - abest->rad;
	}
	else{
		epos = Vec3d(1e5,1e5,1e5);
		height = 1e10;
	}

	x = spos[0] - vpos[0], z = spos[2] - vpos[2];
	sdist = VECSDIST(spos, vpos);
	dist = sqrt(sdist);

	{
/*		avec3_t de, ds;
		double h = 1. / (1. + height / 10.), sp;
		VECSUB(de, epos, vpos);
		VECSUB(ds, spos, vpos);*/
		f = 1./* - calcredness(vw, abest->rad, de, ds) / 256.*//*1. - h + h / MAX(1., 1.0 + sp)*/;
		white[1] *= 1. - (1. - f) * .5;
		white[2] *= f;
	}

	/* if neither sun nor planet with atmosphere like earth is near you, the sun's colona shrinks. */
	{
		static const double c = .8, d = .1, e = .08;
		static int normalized = 0;
		static double normalizer;
		double sp, brightness;
		avec3_t dv, spos1;
		amat4_t mat, mat2;
		if(!normalized)
			normalizer = 1. / (c/* + d / (1. + SUN_DISTANCE)*/), normalized = 1;
/*		pyrmat(vw->pyr, &mat);*/
		VECSUB(dv, vw->pos, spos);
		mat4dvp3(spos1, vw->rot, dv);
		sp = -spos1[2];
		brightness = pow(100, -a->absmag / 5.);
		as = M_PI * f * normalizer * (c / (1. + dist / a->rad / 5.) + d / (1. + height / 3.) + e * (sp * sp / VECSLEN(dv))) / (1. + dist / brightness * 1e-11);
		MAT4IDENTITY(mat);
		VECNORMIN(spos1);
		VECCPY(&mat[8], spos1);
		VECVP(&mat[4], &mat[8], avec3_001);
		if(VECSLEN(&mat[4]) == 0.)
			VECCPY(&mat[4], avec3_010);
		else
			VECNORMIN(&mat[4]);
		VECVP(&mat[0], &mat[4], &mat[8]);
		sp = 100. * (1. - mat[10]) * (1. - mat[10]) - 7.;
		if(sp < 1.)
			sp = 1.;
		VECSCALEIN(&mat[0], sp);
		sp /= 100.;
		if(sp < 1.)
			sp = 1.;
		VECSCALEIN(&mat[4], sp);
		glDisable(GL_CULL_FACE);
		glPushMatrix();
		glLoadIdentity();
/*		glMultMatrixd(mat);*/
/*		mat4mp(mat2, vw->rot, mat);*/
		VECSCALEIN(spos1, -1);
		gldSpriteGlow(spos1, as / M_PI, white, mat);
		glPopMatrix();
	}

	/* the sun is well too far that colona doesn't show up in a pixel. */
/*	if(as < M_PI / 300.)
		return;*/

	white[3] = 127 + (int)(127 / (1. + height));
	{
		double rgb[3];
		avec3_t pos;
		VECSUB(pos, spos, vpos);
		VECSCALEIN(pos, 1. / dist);
		if(vw->relative){
/*			doppler(rgb, white[0] / 256., white[1] / 256., white[2] / 256., VECSP(pos, vw->velo));
			VECSCALE(white, rgb, 255);*/
		}
		else if(g_invert_hyperspace && LIGHT_SPEED < vw->velolen)
			VECSCALEIN(white, LIGHT_SPEED / vw->velolen);
		gldGlow(atan2(x, z), atan2(spos[1] - vpos[1], sqrt(x * x + z * z)), as, white);
	}

	a->flags |= AO_DRAWAIRCOLONA;
}


void Universe::draw(const Viewer *vw){
	drawstarback(vw, this, NULL, NULL);
	st::draw(vw);
}

