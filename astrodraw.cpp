/** \file
 * \brief Implementation of drawing methods of astronomical objects like Star and TexSphere.
 */
#include "astrodraw.h"
#include "draw/ring-draw.h"
#include "Universe.h"
#include "judge.h"
#include "CoordSys.h"
#include "antiglut.h"
#include "galaxy_field.h"
#include "astro_star.h"
#include "stellar_file.h"
#include "TexSphere.h"
#include "cmd.h"
#include "glsl.h"
#include "glstack.h"
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
#include <clib/c.h>
}
#include <clib/stats.h>
#include <cpplib/gl/cullplus.h>
#include <gl/glu.h>
#include <gl/glext.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <setjmp.h>
#include <strstream>
#include <sstream>
#include <iomanip>
#include <fstream>


#define COLIST(c) COLOR32R(c), COLOR32G(c), COLOR32B(c), COLOR32A(c)
#define EPSILON 1e-7 // not sure
#define DRAWTEXTURESPHERE_PROFILE 0
#define ICOSASPHERE_PROFILE 0
#define SQRT2P2 (1.4142135623730950488016887242097/2.)

static int g_sc = -1, g_cl = 1;
static double gpow = .5, gscale = 2.;

static void initar(){
	CvarAdd("g_sc", &g_sc, cvar_int);
	CvarAdd("g_cl", &g_cl, cvar_int);
	CvarAdd("gpow", &gpow, cvar_double);
	CvarAdd("gscale", &gscale, cvar_double);
}
static Initializator s_initar(initar);

struct drawIcosaSphereArg{
	int maxlevel;
	int culllevel;
	int polys;
	int invokes;
	int culltests;
	double radius;
	Vec3d org, view, viewdir, viewdelta;
	Vec3d torg, tview;
	Vec3d scales;
	Quatd qrot;
	Mat4d model, imodel;
};

static void drawIcosaSphereInt(int level, drawIcosaSphereArg *arg, const Vec3d &p0, const Vec3d &p1, const Vec3d &p2){

	/* Cull face */
	arg->invokes++;
	if(arg->culllevel <= level){
		const Vec3d pos[3] = {p0, p1, p2};
		arg->culltests++;
		Vec3d cen = p0 + p1 + p2;
		double f = arg->radius / (cen.len() / 3.);
		int i;
		Vec3d &tview(arg->tview);
		Vec3d &torg(arg->torg);
		for(i = 0; i < 3; i++)
			if(!jHitSphere(torg, arg->radius, tview, f * pos[i] + torg - tview, 1.))
				break;
		if(i == 3)
			return;
	}

	if(level <= 0){
		const Vec3d pos[3] = {p0, p1, p2};
		const Vec3d ipos[3] = {(arg->model.dvp3(p0)), (arg->model.dvp3(p1)), (arg->model.dvp3(p2))};
		static void (WINAPI *const glproc[3])(const GLdouble *) = {glNormal3dv, glTexCoord3dv, glVertex3dv};
		for(int n = 0; n < 3; n++)/* for(int m = 0; m < 3; m++)*/{
			Vec3d tpos = arg->model.dvp3(pos[n]);
			Vec3d rpos = arg->qrot.trans(tpos);
			Vec3d ipos = arg->imodel.dvp3(pos[n]);
			glNormal3dv(arg->qrot.trans(ipos));
			glTexCoord3dv(tpos);
			if(glMultiTexCoord3dvARB)
				glMultiTexCoord3dvARB(GL_TEXTURE1_ARB, rpos / arg->radius);
			glVertex3dv(rpos + arg->org);
		}
		arg->polys++;
		return;
	}

	Vec3d p01 = (p0 + p1).norm() * arg->radius;
	Vec3d p12 = (p1 + p2).norm() * arg->radius;
	Vec3d p20 = (p2 + p0).norm() * arg->radius;

	const Vec3d *faces[][3] = {
		{&p0, &p01, &p20},
		{&p01, &p1, &p12},
		{&p12, &p2, &p20},
		{&p01, &p12, &p20},
	};
	for(int i = 0; i < numof(faces); i++)
		drawIcosaSphereInt(level - 1, arg, *faces[i][0], *faces[i][1], *faces[i][2]);
}

static void gldIcosaSphere(drawIcosaSphereArg &arg){
	static const double vertices[][3] = {
		{ 0., 0., 1.}, // North pole
		{ 0.894427190999916,  0.000000000000000,  0.447213595499958},
		{ 0.276393202250021,  0.850650808352040,  0.447213595499958},
		{-0.723606797749979,  0.525731112119134,  0.447213595499958},
		{-0.723606797749979, -0.525731112119133,  0.447213595499958},
		{ 0.276393202250021, -0.850650808352040,  0.447213595499958},
		{ 0., 0., -1.}, // South pole
		{ 0.723606797749979,  0.525731112119134, -0.447213595499958},
		{-0.276393202250021,  0.850650808352040, -0.447213595499958},
		{-0.894427190999916,  0.000000000000000, -0.447213595499958},
		{-0.276393202250021, -0.850650808352040, -0.447213595499958},
		{ 0.723606797749979, -0.525731112119134, -0.447213595499958},
	};
	static const unsigned char tris[][3] = {
		{0,1,2}, // Following are 5 triangles around north pole, making pentagon.
		{0,2,3},
		{0,3,4},
		{0,4,5},
		{0,5,1},
		{6,8,7}, // Following are 5 triangles around south pole. Note that direction is opposite to north.
		{6,9,8},
		{6,10,9},
		{6,11,10},
		{6,7,11},
		{1,7,2}, // Following are 10 triangles around equator. Note that face direction is alternating per face.
		{7,8,2},
		{2,8,3},
		{8,9,3},
		{3,9,4},
		{9,10,4},
		{4,10,5},
		{10,11,5},
		{5,11,1},
		{11,7,1},
	};
	int i;
	glBegin(GL_TRIANGLES);
	for(i = 0; i < numof(tris); i++){
		drawIcosaSphereInt(arg.maxlevel, &arg, arg.radius * (Vec3d&)vertices[tris[i][0]], arg.radius * (Vec3d&)vertices[tris[i][1]], arg.radius * (Vec3d&)vertices[tris[i][2]]);
	}
	glEnd();
}

void drawIcosaSphere(const Vec3d &org, double radius, const Viewer &vw, const Vec3d &scales, const Quatd &qrot){
	drawIcosaSphereArg arg;
	arg.model = Mat4d(Vec3d(scales[0], 0, 0), Vec3d(0, scales[1], 0), Vec3d(0, 0, scales[2]));
	arg.imodel = Mat4d(Vec3d(1. / scales[0], 0, 0), Vec3d(0, 1. / scales[1], 0), Vec3d(0, 0, 1. / scales[2]));
	arg.scales = scales;
	arg.polys = arg.culltests = arg.invokes = 0;
	arg.radius = radius;
	Vec3d &tview = arg.tview;
	tview = arg.imodel.dvp3(qrot.cnj().trans(vw.pos));
	Vec3d &torg = arg.torg;
	torg = arg.imodel.dvp3(qrot.cnj().trans(org));
	Vec3d delta = torg - tview;
	if(delta.slen() < radius * radius)
		return;
	double pixels = arg.radius * fabs(vw.gc->scale(org));
	double fmaxlevel = 8 * exp(-(pow(delta.len() / arg.radius - 1., gpow)) * gscale);
	int maxlevel = int(fmaxlevel);
//	double maxlevel = 8 * (1. - log(wd->vw->pos.slen() - 1. + 1.));
//	double maxlevel = 8 * (1. - (1. - pow(wd->vw->pos.slen(), gpow)) * gscale);
	arg.maxlevel = g_sc < 0 ? maxlevel : g_sc;
	int pixelminlevel = int(min(sqrt(pixels * .1), 3));
	if(arg.maxlevel < pixelminlevel) arg.maxlevel = pixelminlevel;
	if(8 < arg.maxlevel) arg.maxlevel = 8;
	arg.culllevel = g_cl < 0 ? arg.maxlevel - 1 : g_cl;
	arg.org = org;
	arg.view = vw.pos;
	arg.viewdelta = org - vw.pos;
	arg.qrot = qrot;
//	Mat4d modelview, projection;
//	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
//	glGetDoublev(GL_PROJECTION_MATRIX, projection);
//	arg.modelview = modelview;
//	arg.trans = projection * modelview;
	gldIcosaSphere(arg);
#if ICOSASPHERE_PROFILE
	printf("%d,%g,%d/%15f,%d,%d,%g\n", arg.maxlevel, fmaxlevel, arg.polys, 20 * pow(4., arg.maxlevel), arg.culltests, arg.invokes, delta.len() / arg.radius);
#endif
}

int g_invert_hyperspace = 0;

void drawsuncolona(Astrobj *a, const Viewer *vw);
void drawpsphere(Astrobj *ps, const Viewer *vw, COLOR32 col);
void drawAtmosphere(const Astrobj *a, const Viewer *vw, const avec3_t sunpos, double thick, const GLfloat hor[4], const GLfloat dawn[4], GLfloat ret_horz[4], GLfloat ret_amb[4], int slices);
static GLuint ProjectSphereJpg(const char *fname);
GLuint ProjectSphereCubeJpg(const char *fname, int flags);

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

// Global ambient value for astronomical objects
float g_astro_ambient = .5f;
static void init_astro_ambient(){
	CvarAdd("g_astro_ambient", &g_astro_ambient, cvar_float);
}
static Initializator ini(init_astro_ambient);



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
	double zoom/*, spe*/;
	pspos = ps->calcPos(*p);
/*	tocs(sunp, p->cs, sun.pos, sun.cs);*/
	sunp = sunpos - p->pos;

	dist = (pspos - p->pos).len();
	tp = pspos - p->pos;

	x = pspos[0] - p->pos[0], z = pspos[2] - p->pos[2];

	if(dist < ps->rad)
		return;
	
	/* estimate roughly apparent size change caused by relativity effect */
//	spe = (tp.sp(p->velo) / tp.len() / p->velolen - 1.) / 2.;
//	zoom = p->velolen == 0. || !p->relative ? 1. : (1. + (LIGHT_SPEED / (LIGHT_SPEED - p->velolen) - 1.) * spe * spe);
	zoom = fabs(p->gc->scale(pspos));
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
	Mat4d cloudmat;
	const Viewer *vw;
	int texenable;
	int detail;
} normvertex_params;

#if DRAWTEXTURESPHERE_PROFILE
static int normvertex_invokes = 0;
#endif

static void normvertexf(double x, double y, double z, normvertex_params *p, double ilen, int i, int j){
#if DRAWTEXTURESPHERE_PROFILE
	normvertex_invokes++;
#endif
	Vec3d v(x, y, z);
	glNormal3dv(p->mat.dvp3(v));
	if(p->texenable){
		Vec3d vt = p->texmat.dvp3(v);
		glTexCoord3dv(vt);
		if(p->detail)
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB, GLfloat(vt[0] * 256.), GLfloat(vt[2] * 256.));
		glMultiTexCoord3dvARB(GL_TEXTURE2_ARB, p->cloudmat.dvp3(v));
	}
	Vec3d v1;
	if(p->vw->relative){
		v1 = p->mat * v;
		double theta = v1.len();
		v = v1 * 1. / theta;
		v1 = p->vw->relrot * v;
		v1.scalein(theta);
	}
	else
		v1 = p->vw->relrot * (p->mat * v);
	glVertex3dv(v1);
}

inline double noise3D(int x, int y, int z, int k){
	return RandomSequence(459794526 * x + 165885793 * y + 316878923 * z + 456843661, k).nextd();
}

template<int NNOISE, int COMPS>
static void perlin_noise_3d(GLubyte (*field)[NNOISE][NNOISE][COMPS], long seed, int octaves = 4){
//	CStatistician sta;
	for(int xi = 0; xi < NNOISE; xi++) for(int yi = 0; yi < NNOISE; yi++) for(int zi = 0; zi < NNOISE; zi++){
		for(int k = 0; k < COMPS; k++){
			double sum = 0.;
			double f = 1.;
			double amp = 0.;
			for(int octave = 0; octave < octaves; octave ++){
				int xj, yj, zj;
				int cell = 1 << octave;
				int divides = NNOISE / cell;
				int res = NNOISE / cell;
				int zres = NNOISE / cell;
				if(octave == 0)
					sum += f * noise3D(xi, yi, zi, k);
				else for(xj = 0; xj <= 1; xj++) for(yj = 0; yj <= 1; yj++) for(zj = 0; zj <= 1; zj++){
					sum += f * noise3D((xi / cell + xj) % divides, (yi / cell + yj) % divides, (zi / cell + zj) % divides, k)
					* (xj ? xi % cell : cell - xi % cell) / cell
					* (yj ? yi % cell : cell - yi % cell) / cell
					* (zj ? zi % cell : cell - zi % cell) / cell;
				}
				amp += f;
				f *= 2.;
			}
			field[xi][yi][zi][k] = MIN(255, int(field[xi][yi][zi][k] + sum / amp * 255));
//			sta.put(double(field[xi][yi][zi][k]));
		}
	}
//	CmdPrint(sta.getAvg());
//	CmdPrint(sta.getDev());
}

static GLuint noise3DTexture(){
	static bool init = false;
	static GLuint ret = 0;
	if(!init){
		init = true;
		PFNGLTEXIMAGE3DPROC glTexImage3D = (PFNGLTEXIMAGE3DPROC)wglGetProcAddress("glTexImage3D");
		if(GLenum err = glGetError())
			CmdPrint(dstring() << "ERR" << err << ": " << (const char*)gluErrorString(err));
		if(glTexImage3D){
			static const int NNOISE = 64;
			static GLubyte field[NNOISE][NNOISE][NNOISE][1];
			perlin_noise_3d(field, 48275);
			glGenTextures(1, &ret);
			glBindTexture(GL_TEXTURE_3D, ret);
			glTexImage3D(GL_TEXTURE_3D, 0, GL_ALPHA, NNOISE, NNOISE, NNOISE, 0, GL_ALPHA, GL_UNSIGNED_BYTE, field);
			if(GLenum err = glGetError())
				CmdPrint(dstring() << "ERR" << err << ": " << (const char*)gluErrorString(err));
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
		}
	}
	return ret;
}



#define normvertex(x,y,z,p,ilen) normvertexf(x,y,z,p,ilen,i,j)
#define PROJTS 1024
#define PROJS (PROJTS/2)
#define PROJBITS 32 /* bit depth of projected texture */

/// drawTextureSphere flags
enum DTS{
	DTS_ADD = 1<<0,
	DTS_NODETAIL = 1<<1,
	DTS_ALPHA = 1<<2,
	DTS_NORMALMAP = 1<<3,
	DTS_NOGLOBE = 1<<4,
	DTS_LIGHTING = 1<<5
};

/// \brief A class that draw textured sphere.
///
/// Arguments on how to draw the sphere are passed via member functions.
/// The caller must invoke draw() method to get it working.
class DrawTextureSphere{
protected:
	Astrobj *a;
	const Viewer *vw;
	const Vec3d sunpos;
	const Vec3d apos;
	Vec4f m_mat_diffuse;
	Vec4f m_mat_ambient;
	GLuint *ptexlist;
	Mat4d m_texmat;
	const char *m_texname;
	const std::vector<TexSphere::Texture> *m_textures;
	double m_rad;
	int m_flags;
	GLuint m_shader;
	bool m_drawint;
	int m_ncuts;
	int m_nfinecuts;
	int m_nffinecuts;
	AstroRing *m_ring;
	double m_ringmin, m_ringmax;
	Quatd m_cloudRotation;
	void useShader();
public:
	typedef DrawTextureSphere tt;
	tt(Astrobj *a, const Viewer *vw, const Vec3d &sunpos) : a(a), vw(vw), sunpos(sunpos)
		, apos(vw->cs->tocs(a->pos, a->parent))
		, m_mat_diffuse(1,1,1,1), m_mat_ambient(.5, .5, .5, 1.)
		, ptexlist(NULL)
		, m_texmat(mat4_u)
		, m_textures(NULL)
		, m_rad(0), m_flags(0), m_shader(0), m_drawint(false)
		, m_ncuts(32), m_nfinecuts(256), m_nffinecuts(2048)
		, m_ring(NULL)
		, m_ringmin(0.), m_ringmax(0.)
		, m_cloudRotation(quat_u){}
	tt &astro(TexSphere *a){this->a = a; return *this;}
	tt &viewer(const Viewer *vw){this->vw = vw; return *this;}
	tt &mat_diffuse(const Vec4f &a){this->m_mat_diffuse = a; return *this;}
	tt &mat_ambient(const Vec4f &a){this->m_mat_ambient = a; return *this;}
	tt &texlist(GLuint *a){ptexlist = a; return *this;}
	tt &texmat(const Mat4d &a){m_texmat = a; return *this;}
	tt &texname(const char *a){m_texname = a; return *this;}
	tt &textures(const std::vector<TexSphere::Texture> &atextures){m_textures = &atextures; return *this;}
	tt &rad(double a){m_rad = a; return *this;}
	tt &flags(int a){m_flags = a; return *this;}
	tt &shader(GLuint a){m_shader = a; return *this;}
	tt &drawint(bool a){m_drawint = a; return *this;}
	tt &ncuts(int a){m_ncuts = a; m_nfinecuts = a * 8; m_nffinecuts = a * 64; return *this;}
	tt &ring(AstroRing *a){m_ring = a; return *this;}
	tt &ringRange(double ringmin, double ringmax){m_ringmin = ringmin; m_ringmax = ringmax; return *this;}
	tt &cloudRotation(const Quatd &acloudrot){m_cloudRotation = acloudrot; return *this;}
	virtual bool draw();
};

void DrawTextureSphere::useShader(){
	static GLuint noise3tex = noise3DTexture();
	GLuint shader = m_shader;
	double dist = (apos - vw->pos).len();
	if(shader && g_shader_enable){
		glUseProgram(shader);
		GLint textureLoc = glGetUniformLocation(shader, "texture");
		if(0 <= textureLoc && ptexlist){
			glUniform1i(textureLoc, 0);
		}
		GLint noise3DLoc = glGetUniformLocation(shader, "noise3D");
		if(0 <= noise3DLoc && ptexlist){
			glActiveTextureARB(GL_TEXTURE2_ARB);
			glUniform1i(noise3DLoc, 2);
			glBindTexture(GL_TEXTURE_3D, noise3tex);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
		GLint invEyeMat3Loc = glGetUniformLocation(shader, "invEyeRot3x3");
		if(0 <= invEyeMat3Loc){
			Mat4d rot2 = /*a->rot.tomat4() **/ vw->irot;
			Mat3<float> irot3 = rot2.tomat3().cast<float>();
			glUniformMatrix3fv(invEyeMat3Loc, 1, GL_FALSE, irot3);
		}
		GLint timeLoc = glGetUniformLocation(shader, "time");
		if(0 <= timeLoc){
			glUniform1f(timeLoc, GLfloat(vw->viewtime));
		}
		GLint heightLoc = glGetUniformLocation(shader, "height");
		if(0 <= heightLoc){
			glUniform1f(heightLoc, GLfloat(max(0, dist - a->rad)));
		}
		TexSphere::TextureIterator it;
		int i;
		if(m_textures) for(it = m_textures->begin(), i = 3; it != m_textures->end(); it++, i++){
			const TexSphere::Texture &tex = *it;
//			if(tex.shaderLoc == -2)
				tex.shaderLoc = glGetUniformLocation(shader, tex.uniformname);
			if(0 <= tex.shaderLoc){
				if(!tex.list)
					tex.list = ProjectSphereCubeJpg(tex.filename, m_flags | DTS_NODETAIL | (tex.normalmap ? DTS_NORMALMAP : 0));
				glActiveTextureARB(GL_TEXTURE0_ARB + i);
				glCallList(tex.list);
				glUniform1i(tex.shaderLoc, i);
				glMatrixMode(GL_TEXTURE);
				glLoadIdentity();
				glMatrixMode(GL_MODELVIEW);
				glActiveTextureARB(GL_TEXTURE0_ARB);
			}
		}
	}
}

/// Draws a sphere textured
bool DrawTextureSphere::draw(){
	const Vec4f &mat_diffuse = m_mat_diffuse;
	const Vec4f &mat_ambient = m_mat_ambient;
	const Mat4d &texmat = m_texmat;
//	GLuint texlist = *ptexlist;
	const char *texname = m_texname;
	double &rad = m_rad;
	int flags = m_flags;
	GLuint shader = m_shader;

	int i, j, jstart, fine;
	bool texenable = ptexlist && *ptexlist && texmat;
	normvertex_params params;
	Mat4d rot;
	
	if(rad == 0.)
		rad = a->rad;

	params.vw = vw;
	params.texenable = texenable;
	params.detail = 0;

	double scale = rad * vw->gc->scale(apos);

	Vec3d tp = apos - vw->pos;
	double zoom = !vw->relative || vw->velolen == 0. ? 1. : LIGHT_SPEED / (LIGHT_SPEED - vw->velolen) /*(1. + (LIGHT_SPEED / (LIGHT_SPEED - vw->velolen) - 1.) * spe * spe)*/;
	scale *= zoom;
	if(0. < scale && scale < 10.){
		if(!(flags & DTS_NOGLOBE)){
			GLubyte color[4], dark[4];
			color[0] = GLubyte(mat_diffuse[0] * 255);
			color[1] = GLubyte(mat_diffuse[1] * 255);
			color[2] = GLubyte(mat_diffuse[2] * 255);
			color[3] = 255;
			dark[0] = GLubyte(mat_ambient[0] * 127);
			dark[1] = GLubyte(mat_ambient[1] * 127);
			dark[2] = GLubyte(mat_ambient[2] * 127);
			for(i = 0; i < 3; i++)
				dark[i] = GLubyte(g_astro_ambient * g_astro_ambient * 255);
			dark[3] = 255;
			drawShadeSphere(a, vw, sunpos, color, dark);
		}
		return true;
	}

	// Allocate surface texture
	do if(ptexlist && !*ptexlist && texname){
//		timemeas_t tm;
//		TimeMeasStart(&tm);
//		texlist = *ptexlist = ProjectSphereJpg(texname);
		*ptexlist = ProjectSphereCubeJpg(texname, flags);
//		CmdPrintf("%s draw: %lg", texname, TimeMeasLap(&tm));
	} while(0);

	double (*cuts)[2] = CircleCuts(m_ncuts);
	double (*finecuts)[2] = CircleCutsPartial(m_nfinecuts, 9);

	double dist = (apos - vw->pos).len();
	if(m_drawint ^ (dist < rad))
		return true;

	if(rad / dist < 1. / vw->vp.m)
		return texenable;

	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT);
/*	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(1);
	glDisable(GL_BLEND);
	glColor3ub(255,255,255);*/

	if(texenable){
		glCallList(*ptexlist);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0., 0.);
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}
	else
		glDisable(GL_TEXTURE_2D);

	useShader();

	if(vw->detail){
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

/*	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glMultMatrixd(texmat);
	glMatrixMode(GL_MODELVIEW);*/

	// The model view matrix must be initialized before lighting is set up.
	glPushMatrix();
	glLoadIdentity();

	if(m_flags & DTS_LIGHTING){
		const GLfloat mat_specular[] = {0., 0., 0., 1.};
		const GLfloat mat_shininess[] = { 50.0 };
		const GLfloat color[] = {1.f, 1.f, 1.f, 1.f}, amb[] = {g_astro_ambient, g_astro_ambient, g_astro_ambient, 1.f};

		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, texenable ? color : mat_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, texenable ? amb : mat_ambient);
		glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, color);

		Vec4<GLfloat> light_position = (sunpos - apos).normin().cast<GLfloat>();
		light_position[3] = 0.;
		glLightfv(GL_LIGHT0, GL_POSITION, light_position);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glEnable(GL_NORMALIZE);
	}
	else{
		glColor4fv(mat_diffuse);
		glDisable(GL_LIGHTING);
	}

	if(flags & DTS_ADD){
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
	}
/*		glPolygonMode(GL_BACK, GL_LINE);*/
/*	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);*/
/*	glEnable(GL_TEXTURE_2D);*/
//	if(texenable)
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glEnable(GL_CULL_FACE);
	glFrontFace(m_drawint ? GL_CW : GL_CCW);

	{
//		Mat4d irot;
		Quatd qirot = Quatd::direction(apos - vw->pos);
		AstroRing *ring = m_ring;
		if(ring && m_shader){
			Quatd qrot = vw->cs->tocsq(a->parent).cnj()/* * Quatd::direction(a->omg)*/;
			Vec3d ringnorm = a->rot.trans(vec3_001);
			ring->ring_setsphereshadow(*vw, m_ringmin, m_ringmax, ringnorm, m_shader);
		}

//		gldLookatMatrix(~irot, ~(apos - vw->pos));
//		irot = qirot.tomat4();
		rot = qirot.cnj().tomat4();
		if(texenable){
//			Mat4d rotm = vw->cs->tocsm(a->parent);
//			Mat4d rot2 = texmat * rotm;
			Quatd qrotvw = vw->cs->tocsq(a->parent);
			Quatd qrotm = qrotvw * qirot;
			params.texmat = texmat * qrotm.tomat4();
			params.cloudmat = (m_cloudRotation.cnj() * qrotm).tomat4();
/*			glActiveTextureARB(GL_TEXTURE3_ARB);
			glMatrixMode(GL_TEXTURE);
			glLoadMatrixd(params.texmat);
			glMatrixMode(GL_MODELVIEW);
			glActiveTextureARB(GL_TEXTURE0_ARB);*/
		}
		params.mat = qirot.tomat4().translatein(0., 0., dist).scalein(-rad, -rad, -rad);
	}

	double tangent = rad < dist ? asin(rad / dist) : M_PI / 2.;
/*	fine = M_PI / 3. < tangent;*/
	fine = 0/*DTS_NODETAIL & flags*/ ? 0 : M_PI * 7. / 16. < tangent ? 2 : M_PI / 3. < tangent ? 1 : 0;
	double (*ffinecuts)[2] = CircleCutsPartial(m_nffinecuts, 9);

#if DRAWTEXTURESPHERE_PROFILE
	normvertex_invokes = 0;
	timemeas_t tm;
	TimeMeasStart(&tm);
#endif
	Mat4d &mat = params.mat;
	glBegin(GL_QUADS);
	jstart = int(tangent * m_ncuts / 2 / M_PI);
	for(j = jstart; j < m_ncuts / 4 - !!fine; j++){
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
		len1 = 1. / v1.len();
		v[0] = cuts[j+1][1], v[2] = cuts[j+1][0];
		v1 = mat * v;
		len2 = 1. / v1.len();
		for(i = 0; i < m_ncuts; i++){
			int i2 = (i+1) % m_ncuts;
			normvertex(cuts[i][0] * c, cuts[i][1] * c, s, &params, len1);
			normvertex(cuts[i2][0] * c, cuts[i2][1] * c, s, &params, len1);
			normvertex(cuts[i2][0] * cuts[j+1][1], cuts[i2][1] * cuts[j+1][1], cuts[j+1][0], &params, len2);
			normvertex(cuts[i][0] * cuts[j+1][1], cuts[i][1] * cuts[j+1][1], cuts[j+1][0], &params, len2);
		}
	}
	if(1 <= fine) for(i = 0; i < m_ncuts; i++) for(j = (1 < fine); j < m_nfinecuts / m_ncuts; j++){
		int i2 = (i+1) % m_ncuts;
		double len1, len2;
		Vec3d v1, v;
		v[0] = finecuts[j][0], v[1] = 0., v[2] = finecuts[j][1];
		v1 = mat * v;
		len1 = 1. / VECLEN(v1);
		v[0] = finecuts[j+1][0], v[2] = finecuts[j+1][1];
		v1 = mat * v;
		len2 = 1. / VECLEN(v1);
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
		for(i = 0; i < m_ncuts; i++) for(j = 0; j < m_nffinecuts / m_nfinecuts; j++){
			int i2 = (i+1) % m_ncuts;
			double len1, len2;
			Vec3d v1, v;
			v[0] = ffinecuts[j][0], v[1] = 0., v[2] = ffinecuts[j][1];
			v1 = mat * v;
			len1 = 1. / VECLEN(v1);
			v[0] = ffinecuts[j+1][0], v[2] = ffinecuts[j+1][1];
			v1 = mat * v;
			len2 = 1. / VECLEN(v1);
			params.detail = 1;
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

	if(shader && g_shader_enable)
		glUseProgram(0);

	glPopAttrib();
	glPopMatrix();
	return texenable;
}

/// \brief A class that draw textured sphere.
///
/// Arguments on how to draw the sphere are passed via member functions.
/// The caller must invoke draw() method to get it working.
class DrawTextureSpheroid : public DrawTextureSphere{
public:
	typedef DrawTextureSphere st;
	typedef DrawTextureSpheroid tt;
	double m_oblateness;
	tt(TexSphere *a, const Viewer *vw, const Vec3d &sunpos) : st(a, vw, sunpos)
		, m_oblateness(0.){}
	tt &oblateness(double a){m_oblateness = a; return *this;}
	virtual bool draw();
};

bool DrawTextureSpheroid::draw(){
	const Vec4f &mat_diffuse = m_mat_diffuse;
	const Vec4f &mat_ambient = m_mat_ambient;
	const Mat4d &texmat = m_texmat;
	GLuint texlist = *ptexlist;
	const char *texname = m_texname;
	double &rad = m_rad;
	int flags = m_flags;
	GLuint shader = m_shader;
//	const Quatd *texrot;
	double &oblateness = m_oblateness;
	AstroRing *ring = m_ring;
	double ringminrad = m_ringmin;
	double ringmaxrad = m_ringmax;

	double scale, zoom;
	bool texenable = !!texlist;
	normvertex_params params;
	Mat4d &mat = params.mat;
	Mat4d rot;

	params.vw = vw;
	params.texenable = texenable;
	params.detail = 0;

/*	tocs(sunpos, vw->cs, sun.pos, sun.cs);*/
	scale = a->rad * vw->gc->scale(apos);

	Vec3d tp = apos - vw->pos;
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
//		timemeas_t tm;
//		TimeMeasStart(&tm);
		texlist = *ptexlist = ProjectSphereCubeJpg(texname, 0);
//		CmdPrintf("%s draw: %lg", texname, TimeMeasLap(&tm));
	} while(0);

	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT);
/*	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(1);
	glDisable(GL_BLEND);
	glColor3ub(255,255,255);*/
	glDisable(GL_BLEND);

	if(texenable){
		glCallList(texlist);
	}
	else
		glDisable(GL_TEXTURE_2D);

	if(vw->detail){
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	Quatd qrot = vw->cs->tocsq(a->parent).cnj() * a->rot;
	Vec3d pos(vw->cs->tocs(a->pos, a->parent));

	if(m_flags & DTS_LIGHTING){
		const GLfloat mat_specular[] = {0., 0., 0., 1.};
		const GLfloat mat_shininess[] = { 50.0 };
		const GLfloat color[] = {1.f, 1.f, 1.f, 1.f}, amb[] = {g_astro_ambient, g_astro_ambient, g_astro_ambient, 1.f};

		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, texenable ? color : mat_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, texenable ? amb : mat_ambient);
		glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, color);
		glEnable(GL_NORMALIZE);
		glEnable(GL_CULL_FACE);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

		glLightfv(GL_LIGHT0, GL_POSITION, Vec4<float>(/*qrot.cnj().trans*/(sunpos - pos).cast<float>()));
	}

	// Temporarily create dummy Viewer (in other words, latch) with position of zero vector, to avoid cancellation errors.
	// It does not remove subtraction, but many OpenGL implementation treats matrices with float, so using double inside CPU would help.
	Viewer avw = *vw;
	GLcull gc = GLcull(vw->gc->getFov(), Vec3d(0,0,0), vw->gc->getInvrot(), vw->gc->getNear(), vw->gc->getFar());
	avw.pos = Vec3d(0,0,0);
	avw.gc = &gc;
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glMultMatrixd(texmat);
	glRotatef(90, 1, 0, 0);
	glMatrixMode(GL_MODELVIEW);

	useShader();

	if(ring && m_shader)
		ring->ring_setsphereshadow(*vw, ringminrad, ringmaxrad, vw->rot.dvp3(qrot.trans(vec3_001)), m_shader);

	drawIcosaSphere(pos - vw->pos, rad, avw, Vec3d(1., 1., 1. - oblateness), qrot);

	if(ring && m_shader)
		ring->ring_setsphereshadow_end();
	if(shader && g_shader_enable)
		glUseProgram(0);

	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glPopAttrib();

	return texenable;
}

void drawSphere(const struct astrobj *a, const Viewer *vw, const avec3_t sunpos, GLfloat mat_diffuse[4], GLfloat mat_ambient[4]){
	GLuint zero = 0;
//	drawTextureSphere(a, vw, sunpos, mat_diffuse, mat_ambient, &zero, mat4identity, NULL);
}

static int g_tscuts = 32;
static int g_cloud = 1;
static void g_tscuts_init(){CvarAdd("g_tscuts", &g_tscuts, cvar_int); CvarAdd("g_cloud", &g_cloud, cvar_int);}
static Initializator s_tscuts(g_tscuts_init);

void TexSphere::draw(const Viewer *vw){
	if(1)
		return;
	if(vw->zslice != 2)
		return;
	Astrobj *sun = findBrightest();
	Vec3d sunpos = sun ? vw->cs->tocs(sun->pos, sun->parent) : vec3_000;
	Quatd ringrot;
	int ringdrawn = 8;
	bool drawring = 0. < ringthick && !vw->gc->cullFrustum(calcPos(*vw), rad * ringmax * 1.1);
	double sunar = sun ? sun->rad / (parent->tocs(sun->pos, sun->parent) - pos).len() : .01;

	Vec3d pos = vw->cs->tocs(this->pos, this->parent);
	if(drawring){
		double dist = (vw->pos - pos).len();
		double ratio = this->rad / dist;
		// Avoid domain error
		double theta = ratio < 1. ? acos(ratio) : 0.;
		ringdrawn = dist / (this->rad * ringmin) < 1. ? 0 : int(theta * RING_CUTS / 2. / M_PI + 1);
		ringrot = Quatd::direction(omg);
		astroRing.ring_draw(*vw, this, sunpos, ringdrawn, RING_CUTS / 2, ringrot, ringthick, ringmin, ringmax, 0., oblateness, ringtexname, ringbacktexname, sunar);
	}

	drawAtmosphere(this, vw, sunpos, atmodensity, atmohor, atmodawn, NULL, NULL, 32);
	if(sunAtmosphere(*vw)){
		Astrobj *brightest = findBrightest();
		if(brightest)
			drawsuncolona(brightest, vw);
	}
	if(!vw->gc->cullFrustum(calcPos(*vw), rad * 2.)){
		if(g_shader_enable){
			if(!shaderGiveup && !shader && vertexShaderName.size() && fragmentShaderName.size()){
				GLuint *shaders = NULL;
				try{
					std::vector<GLuint> shaders(vertexShaderName.size() + fragmentShaderName.size());
					int j = 0;
					for(unsigned i = 0; i < vertexShaderName.size(); i++)
						if(!glsl_load_shader(shaders[j++] = glCreateShader(GL_VERTEX_SHADER), vertexShaderName[i]))
							throw 1;
					for(unsigned i = 0; i < fragmentShaderName.size(); i++)
						if(!glsl_load_shader(shaders[j++] = glCreateShader(GL_FRAGMENT_SHADER), fragmentShaderName[i]))
							throw 2;
					shader = glsl_register_program(&shaders.front(), vertexShaderName.size() + fragmentShaderName.size());
					if(!shader)
						throw 3;
				}
				catch(int){
					CmdPrint(cpplib::dstring() << "Shader compile error for " << getpath() << " globe.");
					shaderGiveup = true;
				}
			}

			if(!cloudShaderGiveup && !cloudShader && cloudVertexShaderName.size() && cloudFragmentShaderName.size()){
				try{
					std::vector<GLuint> shaders(cloudVertexShaderName.size() + cloudFragmentShaderName.size());
					int j = 0;
					for(unsigned i = 0; i < cloudVertexShaderName.size(); i++)
						if(!glsl_load_shader(shaders[j++] = glCreateShader(GL_VERTEX_SHADER), cloudVertexShaderName[i]))
							throw 1;
					for(unsigned i = 0; i < cloudFragmentShaderName.size(); i++)
						if(!glsl_load_shader(shaders[j++] = glCreateShader(GL_FRAGMENT_SHADER), cloudFragmentShaderName[i]))
							throw 2;
					cloudShader = glsl_register_program(&shaders.front(), shaders.size());
					if(!shader)
						throw 3;
				}
				catch(int){
					CmdPrint(cpplib::dstring() << "Shader compile error for " << getpath() << " cloud.");
					cloudShaderGiveup = true;
				}
			}
		}
		double fcloudHeight = cloudHeight == 0. ? rad * 1.001 : cloudHeight + rad;
		bool underCloud = (pos - vw->pos).len() < fcloudHeight;
		if(oblateness != 0.){
			DrawTextureSpheroid cloudDraw = DrawTextureSpheroid(this, vw, sunpos);
			if(g_cloud && (cloudtexname.len() || cloudtexlist)){
				cloudDraw
				.oblateness(oblateness)
				.flags(DTS_LIGHTING)
				.texlist(&cloudtexlist)
				.texmat(cloudRotation().cnj().tomat4())
				.texname(cloudtexname)
				.textures(textures)
				.shader(cloudShader)
				.rad(fcloudHeight)
				.flags(DTS_ALPHA | DTS_NODETAIL | DTS_NOGLOBE)
				.drawint(true)
				.ncuts(g_tscuts);
				if(underCloud){
					bool ret = cloudDraw.draw();
					if(!ret && *cloudtexname){
						cloudtexname = "";
					}
				}
			}
			bool ret = DrawTextureSpheroid(this, vw, sunpos)
				.oblateness(oblateness)
				.flags(DTS_LIGHTING)
				.mat_diffuse(basecolor)
				.mat_ambient(basecolor / 2.)
				.texlist(&texlist)
				.texmat(mat4_u)
				.textures(textures)
				.shader(shader)
				.texname(texname)
				.rad(rad)
				.ring(&astroRing)
				.ringRange(ringmin, ringmax)
				.cloudRotation(cloudRotation())
				.draw();
			if(!ret && *texname){
				texname = "";
			}
			if(g_cloud && (cloudtexname.len() || cloudtexlist) && !underCloud){
				bool ret = cloudDraw.drawint(false).draw();
				if(!ret && *cloudtexname){
					cloudtexname = "";
				}
			}
		}
		else{
			DrawTextureSphere cloudDraw = DrawTextureSphere(this, vw, sunpos);
			if(g_cloud && (cloudtexname.len() || cloudtexlist)){
				cloudDraw
				.flags(DTS_LIGHTING)
				.texlist(&cloudtexlist)
				.texmat(cloudRotation().cnj().tomat4())
				.texname(cloudtexname)
				.textures(textures)
				.shader(cloudShader)
				.rad(fcloudHeight)
				.flags(DTS_ALPHA | DTS_NODETAIL | DTS_NOGLOBE)
				.drawint(true)
				.ncuts(g_tscuts);
				if(underCloud){
					bool ret = cloudDraw.draw();
					if(!ret && *cloudtexname){
						cloudtexname = "";
					}
				}
			}
			bool ret = DrawTextureSphere(this, vw, sunpos)
				.flags(DTS_LIGHTING)
				.mat_diffuse(basecolor)
				.mat_ambient(basecolor / 2.f)
				.texlist(&texlist).texmat(rot.cnj().tomat4()).texname(texname).shader(shader)
				.textures(textures)
				.ncuts(g_tscuts)
				.ring(&astroRing)
				.cloudRotation(cloudRotation())
				.draw();
			if(!ret && *texname){
				texname = "";
			}
			if(g_cloud && (cloudtexname.len() || cloudtexlist) && !underCloud){
				bool ret = cloudDraw.drawint(false).draw();
				if(!ret && *cloudtexname){
					cloudtexname = "";
				}
			}
		}
	}
	st::draw(vw);
	if(drawring && ringdrawn)
		astroRing.ring_draw(*vw, this, sunpos, 0, ringdrawn, ringrot, ringthick, ringmin, ringmax, 0., oblateness, NULL, NULL, sunar);
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
	// Don't call me in the first place!
	if(thick == 0.) return;
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

static int PROJC = 512;
static void Init_PROJC(){
	CvarAdd("g_proj_size", &PROJC, cvar_int);
}
static Initializator init_PROJC(Init_PROJC);

static const GLenum cubetarget[] = {
GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
GL_TEXTURE_CUBE_MAP_POSITIVE_X,
GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
};
static const Quatd cubedirs[] = {
	Quatd(SQRT2P2,0,SQRT2P2,0), /* {0,-SQRT2P2,0,SQRT2P2} */
	Quatd(SQRT2P2,0,0,SQRT2P2),
	Quatd(0,0,1,0), /* {0,0,0,1} */
	Quatd(-SQRT2P2,0,SQRT2P2,0), /*{0,SQRT2P2,0,SQRT2P2},*/
	Quatd(-SQRT2P2,0,0,SQRT2P2), /* ??? {0,-SQRT2P2,SQRT2P2,0},*/
	Quatd(-1,0,0,0), /* {0,1,0,0}, */
};

static cpplib::dstring pathProjection(const char *name, int nn){
	const char *p = strrchr(name, '.');
	cpplib::dstring dstr;
	dstr << "cache/";
	if(p)
		dstr.strncat(name, p - name);
	else
		dstr.strcat(name);
	dstr << "_proj" << nn << ".bmp";
	return dstr;
}

GLuint ProjectSphereCube(const char *name, const BITMAPINFO *raw, BITMAPINFO *cacheload[6], unsigned flags){
	static int texinit = 0;
	GLuint tex, texname;
//	int srcheight = !raw ? 0 : raw->bmiHeader.biHeight < 0 ? -raw->bmiHeader.biHeight : raw->bmiHeader.biHeight;
//	int srcwidth = !raw ? 0 : raw->bmiHeader.biWidth;
	glGenTextures(1, &texname);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texname);
	/*if(!texinit)*/
	{
		int outsize, linebytes, linebytesp;
		long rawh = !raw ? 0 : ABS(raw->bmiHeader.biHeight);
		long raww = !raw ? 0 : raw->bmiHeader.biWidth;
		BITMAPINFO *proj = NULL;
		RGBQUAD zero = {0,0,0,255};
		texinit = 1;
		int bits = flags & DTS_ALPHA ? 8 : PROJBITS;

		if(!cacheload){
			proj = (BITMAPINFO*)malloc(outsize = sizeof(BITMAPINFOHEADER) + (flags & DTS_ALPHA ? 256 * sizeof(RGBQUAD) : 0) + (PROJC * PROJC * bits + 31) / 32 * 4);
			memcpy(proj, raw, sizeof(BITMAPINFOHEADER));
			proj->bmiHeader.biWidth = proj->bmiHeader.biHeight = PROJC;
			proj->bmiHeader.biBitCount = bits;
			proj->bmiHeader.biClrUsed = flags & DTS_ALPHA ? 256 : 0;
			proj->bmiHeader.biSizeImage = proj->bmiHeader.biWidth * proj->bmiHeader.biHeight * proj->bmiHeader.biBitCount / 8;
			linebytes = (raw->bmiHeader.biWidth * raw->bmiHeader.biBitCount + 31) / 32 * 4;
			linebytesp = (PROJC * bits + 31) / 32 * 4;
			for(unsigned i = 0; i < proj->bmiHeader.biClrUsed; i++)
				proj->bmiColors[i].rgbBlue = proj->bmiColors[i].rgbGreen = proj->bmiColors[i].rgbRed = proj->bmiColors[i].rgbReserved = i;
		}

		for(int nn = 0; nn < 6; nn++){
			if(!cacheload) for(int i = 0; i < PROJC; i++) for(int j = 0; j < PROJC; j++){
				RGBQUAD *dst;
				dst = (RGBQUAD*)&((unsigned char*)&proj->bmiColors[proj->bmiHeader.biClrUsed])[(i) * linebytesp + (j) * bits / 8];
				GLubyte *dst8 = (GLubyte*)&((unsigned char*)&proj->bmiColors[proj->bmiHeader.biClrUsed])[(i) * linebytesp + (j) * bits / 8];
	/*			if(r){
					double dj = j * 2. / PROJS - 1.;
					j1 = r < fabs(dj) ? -1 : (int)(raw->bmiHeader.biWidth * fmod(asin(dj / r) / M_PI / 2. + .25 + (ii * 2 + jj) * .25, 1.));
				}
				else{
					*dst = zero;
					continue;
				}*/
				Vec3d epos = cubedirs[nn].cnj().trans(Vec3d(j / (PROJC / 2.) - 1., i / (PROJC / 2.) - 1., -1));
				double lon = -atan2(epos[0], -(epos[2]));
				double lat = atan2(epos[1], sqrt(epos[0] * epos[0] + epos[2] * epos[2])) + M_PI / 2.;
//					double lon = -atan2(epos[0], -(epos[1]));
//					double lat = atan2(epos[2], sqrt(epos[0] * epos[0] + epos[1] * epos[1]));
				double dj1 = (raww-1) * (lon / (2. * M_PI) - floor(lon / (2. * M_PI)));
				double di1 = (rawh-1) * (lat / (M_PI) - floor(lat / (M_PI)));
				double fj1 = dj1 - floor(dj1); // fractional part
				double fi1 = di1 - floor(di1);
				int i1 = (int(floor(di1)) + rawh) % rawh;
				int j1 = (int(floor(dj1)) + raww) % raww;

				if(raw->bmiHeader.biBitCount == 4){ // untested
					double accum[3] = {0}; // accumulator
					for(int ii = 0; ii < 2; ii++) for(int jj = 0; jj < 2; jj++) for(int c = 0; c < 3; c++)
						accum[c] += (jj ? fj1 : 1. - fj1) * (ii ? fi1 : 1. - fi1) * ((unsigned char *)&raw->bmiColors[((unsigned char *)&raw->bmiColors[raw->bmiHeader.biClrUsed])[(i1 + ii) % rawh * linebytes + (j1 + jj) % raww]])[c];
					dst->rgbRed = (GLubyte)accum[0];
					dst->rgbGreen = (GLubyte)accum[1];
					dst->rgbBlue = (GLubyte)accum[2];
					dst->rgbReserved = 255;
				}
				else if(raw->bmiHeader.biBitCount == 8){
					if(flags & DTS_ALPHA){
						double accum = 0.;
						for(int ii = 0; ii < 2; ii++) for(int jj = 0; jj < 2; jj++)
							accum += (jj ? fj1 : 1. - fj1) * (ii ? fi1 : 1. - fi1) * ((unsigned char *)&raw->bmiColors[raw->bmiHeader.biClrUsed])[(i1 + ii) % rawh * linebytes + (j1 + jj) % raww];
						*dst8 = (GLubyte)(accum);
					}
					else if(flags & DTS_NORMALMAP){
						double accum[3] = {0.};
						for(int ii = 0; ii < 2; ii++) for(int jj = 0; jj < 2; jj++){
							const unsigned char *src = ((unsigned char *)&raw->bmiColors[raw->bmiHeader.biClrUsed]);
							int y = (i1 + ii) % rawh;
							int x = (j1 + jj) % raww;
							int wb = linebytes, w = raww, h = rawh;
							double f = (jj ? fj1 : 1. - fj1) * (ii ? fi1 : 1. - fi1);
							accum[0] += f * (unsigned char)rangein(127 + (double)1. * (src[x + y * wb] - src[(x - 1 + w) % w + y * wb]) / 4, 0, 255);
							accum[1] += f * (unsigned char)rangein(127 + 1. * (src[x + y * wb] - src[x + (y - 1 + h) % h * wb]) / 4, 0, 255);
							accum[2] = 255;
						}
						dst->rgbRed = (GLubyte)accum[0];
						dst->rgbGreen = (GLubyte)accum[1];
						dst->rgbBlue = (GLubyte)accum[2];
						dst->rgbReserved = 255;
					}
					else{
						double accum[3] = {0}; // accumulator
						for(int ii = 0; ii < 2; ii++) for(int jj = 0; jj < 2; jj++) for(int c = 0; c < 3; c++)
							accum[c] += (jj ? fj1 : 1. - fj1) * (ii ? fi1 : 1. - fi1) * ((unsigned char *)&raw->bmiColors[((unsigned char *)&raw->bmiColors[raw->bmiHeader.biClrUsed])[(i1 + ii) % rawh * linebytes + (j1 + jj) % raww]])[c];
						dst->rgbRed = (GLubyte)accum[0];
						dst->rgbGreen = (GLubyte)accum[1];
						dst->rgbBlue = (GLubyte)accum[2];
						dst->rgbReserved = 255;
					}
				}
				else if(raw->bmiHeader.biBitCount == 24){
					double accum[3] = {0}; // accumulator
					for(int ii = 0; ii < 2; ii++) for(int jj = 0; jj < 2; jj++) for(int c = 0; c < 3; c++)
						accum[c] += (jj ? fj1 : 1. - fj1) * (ii ? fi1 : 1. - fi1) * ((unsigned char *)&raw->bmiColors[raw->bmiHeader.biClrUsed])[(i1 + ii) % rawh * linebytes + (j1 + jj) % raww * 3 + c];
					dst->rgbRed = (GLubyte)accum[0];
					dst->rgbGreen = (GLubyte)accum[1];
					dst->rgbBlue = (GLubyte)accum[2];
					dst->rgbReserved = 255;
				}
				else if(raw->bmiHeader.biBitCount == 32){ // untested
//					const unsigned char *src;
					double accum[4] = {0}; // accumulator
					for(int ii = 0; ii < 2; ii++) for(int jj = 0; jj < 2; jj++) for(int c = 0; c < 4; c++)
						accum[c] += (jj ? fj1 : 1. - fj1) * (ii ? fi1 : 1. - fi1) * ((unsigned char *)&raw->bmiColors[raw->bmiHeader.biClrUsed])[(i1 + ii) % rawh * linebytes + (j1 + jj) % raww * 4 + c];
//					src = &((unsigned char *)&raw->bmiColors[raw->bmiHeader.biClrUsed])[i1 * linebytes + j1 * 4];
					dst->rgbRed = (GLubyte)accum[0];
					dst->rgbGreen = (GLubyte)accum[1];
					dst->rgbBlue = (GLubyte)accum[2];
					dst->rgbReserved = (GLubyte)accum[3];
				}
			}

			if(!cacheload){
//				std::ostringstream bstr;
				const char *p;
	//			FILE *fp;
				cpplib::dstring dstr = pathProjection(name, nn);
//				bstr << "cache/" << (p ? std::string(name).substr(0, p - name) : name) << "_proj" << nn << ".bmp";

				// Create directories to make path available
//				std::string sstr = bstr.str();
//				const char *cstr = sstr.c_str();
				const char *cstr = dstr;
				p = &cstr[sizeof "cache"-1];
				while(p = strchr(p+1, '/')){
					cpplib::dstring dirpath = cpplib::dstring().strncat(cstr, p - cstr);
#ifdef _WIN32
					if(GetFileAttributes(dirpath) == -1)
						CreateDirectory(dirpath, NULL);
#else
					mkdir(dirpath);
#endif
				}

				/* force overwrite */
				std::ofstream ofs(dstr/*bstr.str().c_str()*/, std::ofstream::out | std::ofstream::binary);
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
				glTexImage2D(cubetarget[nn], 0, flags & DTS_ALPHA ? GL_ALPHA : GL_RGBA, PROJC, PROJC, 0, flags & DTS_ALPHA ? GL_ALPHA : GL_RGBA, GL_UNSIGNED_BYTE, &proj->bmiColors[proj->bmiHeader.biClrUsed]);

			}
			else{
				glTexImage2D(cubetarget[nn], 0, flags & DTS_ALPHA ? GL_ALPHA : cacheload[nn]->bmiHeader.biBitCount / 8, cacheload[nn]->bmiHeader.biWidth, cacheload[nn]->bmiHeader.biHeight, 0,
					cacheload[nn]->bmiHeader.biBitCount == 32 ? GL_RGBA : cacheload[nn]->bmiHeader.biBitCount == 24 ? GL_RGB : cacheload[nn]->bmiHeader.biBitCount == 8 ? flags & DTS_ALPHA ? GL_ALPHA : GL_LUMINANCE : (assert(0), 0),
					GL_UNSIGNED_BYTE, &cacheload[nn]->bmiColors[cacheload[nn]->bmiHeader.biClrUsed]);
			}
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		static bool init = false;
		static GLuint detailname = 0;

		if(!init){
			struct random_sequence rs;
			int tsize = DETAILSIZE;
			static GLubyte tbuf[DETAILSIZE][DETAILSIZE][3], tex[DETAILSIZE][DETAILSIZE][3];
			init_rseq(&rs, 124867);
			init = true;
			for(int i = 0; i < tsize; i++) for(int j = 0; j < tsize; j++){
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
			for(int i = 0; i < tsize; i++) for(int j = 0; j < tsize; j++){
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
			glGenTextures(1, &detailname);
			glBindTexture(GL_TEXTURE_2D, detailname);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tsize, tsize, 0, GL_RGB, GL_UNSIGNED_BYTE, tex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}

		glNewList(tex = glGenLists(1), GL_COMPILE);
		glBindTexture(GL_TEXTURE_CUBE_MAP, texname);
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_CUBE_MAP);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glMaterialfv(GL_FRONT, GL_AMBIENT, Vec4<float>(.5,.5,.5,1.));
//		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, Vec4<float>(.5,.5,.5,1.));

		if(glActiveTextureARB && !(flags & DTS_NODETAIL)){
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glBindTexture(GL_TEXTURE_2D, detailname);
			glEnable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}

		glEndList();
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

		free(proj);
/*		free(raw);*/
	}
	return tex;
}

#if 1


GLuint ProjectSphereCubeJpg(const char *fname, int flags){
		GLuint texlist = 0;
		const char *jpgfilename;
		const char *p;
		p = strrchr(fname, '.');
#ifdef _WIN32
		if(GetFileAttributes("cache") == -1)
			CreateDirectory("cache", NULL);
#else
		mkdir("cache");
#endif
		jpgfilename = fname;
		{
			BITMAPINFO *bmis[6];
			WIN32_FILE_ATTRIBUTE_DATA fd, fd2;
			GetFileAttributesEx(jpgfilename, GetFileExInfoStandard, &fd2);
			int i;
			bool ok = true;
			for(i = 0; i < 6; i++){
				cpplib::dstring outfilename = pathProjection(fname, i);

				if(!GetFileAttributesEx(outfilename, GetFileExInfoStandard, &fd))
					goto heterogeneous;
				if(!(0 < CompareFileTime(&fd.ftLastWriteTime, &fd2.ftLastWriteTime)))
					goto heterogeneous;
				if(!(bmis[i] = ReadBitmap(outfilename))){
					i++;
					goto heterogeneous;
				}
				// If header is different, they cannot be concatenated to produce a cube map.
				if(0 < i && memcmp(&bmis[0]->bmiHeader, &bmis[i]->bmiHeader, sizeof bmis[0]->bmiHeader)){
					i++;
heterogeneous:
					// Free all loaded images so far.
					for(int j = 0; j < i; j++)
						LocalFree(bmis[j]);
					ok = false;
					break;
				}
			}
			if(ok){
				texlist = ProjectSphereCube(fname, NULL, bmis, flags);
				for(int i = 0; i < 6; i++)
					LocalFree(bmis[i]);
			}
			else
			{
				void (*freeproc)(BITMAPINFO*);
				BITMAPINFO *bmi = ReadJpeg(jpgfilename, &freeproc);
				if(!bmi)
					return 0;
				texlist = ProjectSphereCube(fname, bmi, NULL, flags);
				freeproc(bmi);
			}
		}
		return texlist;
}

#endif


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

static int setstarcolor(GLubyte *pr, GLubyte *pg, GLubyte *pb, struct random_sequence *prs, double rvelo, double avelo){
	GLubyte r, g, b;
	int bri, hi;
	{
		double hue, sat;
		hue = drseq(prs);
		hue = (hue + drseq(prs)) * .5 + 1. * rvelo / LIGHT_SPEED;
		sat = 1. - (1. - (.25 + drseq(prs) * .5) /** (hue < 0. ? 1. / (1. - hue) : 1. < hue ? 1. / hue : 1.)*/) / (1. + avelo / LIGHT_SPEED);
		bri = int((127 + rseq(prs) % 128) * (hue < 0. ? 1. / (1. - hue) : 1. < hue ? 1. / hue : 1.));
		hue = rangein(hue, 0., 1.);
		hi = (int)floor(hue * 6.);
		GLubyte f = GLubyte(hue * 6. - hi);
		GLubyte p = GLubyte(bri * (1 - sat));
		GLubyte q = GLubyte(bri * (1 - f * sat));
		GLubyte t = GLubyte(bri * (1 - (1 - f) * sat));
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
		height = (epos - vw->pos).len() - pe->rad;
	}
	else
		height = 1e10;

	invokes++;
	{
		Mat4d mat;
		glPushMatrix();
		mat = csys->tocsm(vw->cs);
		glMultMatrixd(mat);
	}

	static GLuint backimg = 0;
	if(!backimg)
		backimg = ProjectSphereCubeJpg("mwpan2_Merc_2000x1200.jpg", 0);
	{
		glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
		glCallList(backimg);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor4f(.25, .25, .25, 1.);
//		glColor4f(.5, .5, .5, 1.);
		for(int i = 0; i < numof(cubedirs); i++){
			glPushMatrix();
			gldMultQuat(cubedirs[i]);
			glMatrixMode(GL_TEXTURE);
			glPushMatrix();
			gldMultQuat(cubedirs[i]);
			glBegin(GL_QUADS);
			glTexCoord3i(-1,-1,-1); glVertex3i(-1, -1, -1);
			glTexCoord3i( 1,-1,-1); glVertex3i( 1, -1, -1);
			glTexCoord3i( 1, 1,-1); glVertex3i( 1,  1, -1);
			glTexCoord3i(-1, 1,-1); glVertex3i(-1,  1, -1);
			glEnd();
/*			int in[4];
			glGetIntegerv(GL_ATTRIB_STACK_DEPTH, &in[0]);
			glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &in[1]);
			glGetIntegerv(GL_PROJECTION_STACK_DEPTH, &in[2]);
			glGetIntegerv(GL_TEXTURE_STACK_DEPTH, &in[3]);
			printf("amp %d,%d,%d,%d\n", in[0], in[1], in[2], in[3]);*/
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}
		glPopAttrib();
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
		numstars = 0*int(NUMSTARS / (1. + .01 * gz * gz + .01 * (gx + 10) * (gx + 10) + .1 * gy * gy));
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
					r = GLubyte(r * LIGHT_SPEED / vw->velolen);
					g = GLubyte(g * LIGHT_SPEED / vw->velolen);
					b = GLubyte(b * LIGHT_SPEED / vw->velolen);
				}
				glColor4f(r / 255.f, g / 255.f, b / 255.f, GLfloat(255 * f * f));
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
#if 1
	if(!vw->relative/*(pl.mover == warp_move || pl.mover == player_tour || pl.chase && pl.chase->vft->is_warping && pl.chase->vft->is_warping(pl.chase, pl.cs->w))*/ && LIGHT_SPEED * LIGHT_SPEED < velo.slen()){
		int i, count = 1250;
		RandomSequence rs(2413531);
		static double raintime = 0.;
		double cellsize = 1e7;
		double speedfactor;
		const double width = .00002 * 1000. / vw->vp.m; /* line width changes by the viewport size */
		double basebright;
		const double levelfactor = 1.5e3;
		int level = 0;
		const Vec3d nh0(0., 0., -1.);
		Mat4d irot, rot;
		GLubyte col[4] = {255, 255, 255, 255};
		static GLuint texname = 0;
		static const GLfloat envcolor[4] = {0,0,0,0};

/*		count = raintime / dt < .1 ? 500 * (1. - raintime / dt / .1) * (1. - raintime / dt / .1) : 100;*/
/*		TimeMeasStart(&tm);*/
		rot = csys->tocsm(vw->cs);
		Vec3d plpos = csys->tocs(vw->pos, vw->cs);
#if 0
		tocsv(velo, csys, /*pl.chase ? pl.chase->velo :*/ vw->velo, vw->pos, vw->cs);
#elif 1
		Vec3d localvelo = velo;
#else
		mat4dvp3(localvelo, rot, velo);
#endif
		velolen = localvelo.len();
		Vec3d nh = vw->irot.dvp3(nh0);
		glPushMatrix();
		glLoadMatrixd(vw->rot);
		glMultMatrixd(rot);
		glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		if(!texname){
			GLubyte texbits[64][64][2];
			int i, j;
			for(i = 0; i < 64; i++) for(j = 0; j < 64; j++){
				int r;
				r = (32 * 32 - (i - 32) * (i - 32) - (j - 32) * (j - 32)) * 255 / 32 / 32;
				texbits[i][j][0] = GLubyte(256. * sqrt(MIN(255, MAX(0, r)) / 256.)/*256. * sqrt((255 - MIN(255, MAX(0, r))) / 256.)*/);
				r = (32 * 32 - (i - 32) * (i - 32) - (j - 32) * (j - 32)) * 255 / 32 / 32;
				texbits[i][j][1] = GLubyte(256. * sqrt(MIN(255, MAX(0, r)) / 256.));
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

		localvelo *= .025 * (velolen + cellsize / 100. / .025) / velolen;

		for(i = count; i; i--){
			Vec3d pos, dst;
			GLfloat red;
			pos[0] = plpos[0] + (drseq(&rs) - .5) * 2 * cellsize;
			pos[0] = floor(pos[0] / cellsize) * cellsize + cellsize / 2. - pos[0];
			pos[1] = plpos[1] + (drseq(&rs) - .5) * 2 * cellsize;
			pos[1] = floor(pos[1] / cellsize) * cellsize + cellsize / 2. - pos[1];
			pos[2] = plpos[2] + (drseq(&rs) - .5) * 2 * cellsize;
			pos[2] = floor(pos[2] / cellsize) * cellsize + cellsize / 2. - pos[2];
			red = GLfloat(rs.nextd());
			dst = pos;
			dst += localvelo;
			pos -= localvelo;
			double bright = (255 - pos.len() * 255 / (cellsize / 2.)) * basebright;
			if(1. / 255. < bright){
				glColor4f(level == 0 ? red : 1.f, level == 1 ? red : 1.f, level == 2 ? red : 1.f, GLfloat(bright / 255.));
				gldTextureBeam(avec3_000, pos, dst, cellsize / 100. / (1. + speedfactor / levelfactor));
			}
		}
		glPopAttrib();
		glPopMatrix();
	}
#endif
}




























void Star::predraw(const Viewer *){
	flags &= ~AO_DRAWAIRCOLONA;
}

void Star::draw(const Viewer *vw){
	if(vw->zslice != 2)
		return;
	COLOR32 col = COLOR32RGBA(255,255,255,255);
	Vec3d gvelo = parent->tocsv(vw->velo, vw->pos, vw->cs);
	if(LIGHT_SPEED * LIGHT_SPEED < gvelo.slen()){
		extern int g_invert_hyperspace;
		double velolen = gvelo.len();
		if(g_invert_hyperspace)
			col = COLOR32SCALE(col, LIGHT_SPEED / velolen * 256) & COLOR32RGBA(255,255,255,0) | COLOR32RGBA(0,0,0,COLOR32A(col));
	}

/*	Vec3d spos = vw->cs->tocs(pos, this) - vw->pos;
	double dist = spos.len();
	double apparentSize = rad / dist / vw->fov * vw->vp.m;
	if(apparentSize < 1){
		gldSpriteGlow(spos, dist * vw->fov / vw->vp.m * 10, (spectralRGB(this->spect) * 255).cast<GLubyte>(), vw->irot);
		return;
	}*/

//	drawpsphere(this, vw, col);
	DrawTextureSphere(this, vw, vec3_000)
		.draw();
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
	double height;
	double sdist;
	double dist, as;
	Vec3d vpos, epos, spos;
	double x, z;
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
		height = max(.001, (epos - vpos).len() - abest->rad);
	}
	else{
		epos = Vec3d(1e5,1e5,1e5);
		height = 1e10;
	}

	x = spos[0] - vpos[0], z = spos[2] - vpos[2];
	sdist = VECSDIST(spos, vpos);
	dist = sqrt(sdist);

	Vec4<GLubyte> col;
	{
		Vec4<GLubyte> white(255, 255, 255, 255);
		double f = 1./* - calcredness(vw, abest->rad, de, ds) / 256.*//*1. - h + h / MAX(1., 1.0 + sp)*/;
		white[1] = GLubyte(white[1] * 1. - (1. - f) * .5);
		white[2] = GLubyte(white[2] * f);
		if(a->classname() == Star::classRegister.id){
			col = Vec4f(255. * Star::spectralRGB(((Star*)a)->spect)).cast<GLubyte>();
			col[3] = 255;
		}
		else
			col = white;
	}

	GLattrib gla(GL_COLOR_BUFFER_BIT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	double asfactor = 1. / (1. + dist / a->rad * 1e-3);
	if(asfactor < vw->fov / vw->vp.m * 20.){
		as = M_PI * vw->fov / vw->vp.m * 2.;
		Vec3d pos = (spos - vpos) / dist;
		gldTextureGlow(pos, as, col, vw->irot);
		return;
	}
	/* if neither sun nor planet with atmosphere like earth is near you, the sun's colona shrinks. */
	else{
		static const double c = .8, d = .1, e = .08;
		static int normalized = 0;
		static double normalizer;
		if(!normalized)
			normalizer = 1. / c, normalized = 1;
		Vec3d dv = vw->pos - spos;
		Vec3d spos1 = vw->rot.dvp3(dv);
		double sp = -spos1[2];
		double brightness = pow(100, -a->absmag / 5.);
		as = M_PI * normalizer * (c / (1. + dist / a->rad / 5.) + d / (1. + height / 3.) + e * (sp * sp / dv.slen())) * asfactor;
		Mat4d mat = mat4_u;
		spos1.normin();
		mat.vec3(2) = spos1;
		mat.vec3(1) = mat.vec3(2).vp(vec3_001);
		if(mat.vec3(1).slen() == 0.)
			mat.vec3(1) = vec3_010;
		else
			mat.vec3(1).normin();
		mat.vec3(0) = mat.vec3(1).vp(mat.vec3(2));
		sp = 100. * (1. - mat[10]) * (1. - mat[10]) - 7.;
		if(sp < 1.)
			sp = 1.;
		mat.vec3(0) *= sp;
		sp /= 100.;
		if(sp < 1.)
			sp = 1.;
		mat.vec3(1) *= sp;
		glDisable(GL_CULL_FACE);
		glPushMatrix();
		glLoadIdentity();
		spos1 *= -1;
		gldSpriteGlow(spos1, as / M_PI, col, mat);
		glPopMatrix();
	}

	/* the sun is well too far that colona doesn't show up in a pixel. */
/*	if(as < M_PI / 300.)
		return;*/

	col[3] = 127 + (int)(127 / (1. + height));
	{
		Vec3d pos = (spos - vpos) / dist;
		if(vw->relative){
		}
		else if(g_invert_hyperspace && LIGHT_SPEED < vw->velolen)
			col *= GLubyte(LIGHT_SPEED / vw->velolen);
		gldGlow(atan2(x, z), atan2(spos[1] - vpos[1], sqrt(x * x + z * z)), as, col);
	}

	a->flags |= AO_DRAWAIRCOLONA;
}


void Universe::draw(const Viewer *vw){
	if(vw->zslice == 2)
		drawstarback(vw, this, NULL, NULL);
	st::draw(vw);
}

