/** \file
 * \brief Implementation of DrawTextureSphere and DrawTextureSpheroid classes.
 */
#include "DrawTextureSphere.h"
#include "astrodef.h"
#include "glsl.h"
#include "cmd.h"
#include "StaticInitializer.h"
#include "bitmap.h"
#undef exit
extern "C"{
#include <clib/timemeas.h>
#include <clib/gl/cull.h>
#include <clib/gl/gldraw.h>
#include <clib/gl/multitex.h>
#include <clib/cfloat.h>
}
#include <clib/stats.h>
#include <cpplib/gl/cullplus.h>
#include <gl/glu.h>
#include <gl/glext.h>
#include <fstream>

#define SQRT2P2 (1.4142135623730950488016887242097/2.)

const GLenum DrawTextureSphere::cubetarget[] = {
GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
GL_TEXTURE_CUBE_MAP_POSITIVE_X,
GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
};
const Quatd DrawTextureSphere::cubedirs[] = {
	Quatd(SQRT2P2,0,SQRT2P2,0), /* {0,-SQRT2P2,0,SQRT2P2} */
	Quatd(SQRT2P2,0,0,SQRT2P2),
	Quatd(0,0,1,0), /* {0,0,0,1} */
	Quatd(-SQRT2P2,0,SQRT2P2,0), /*{0,SQRT2P2,0,SQRT2P2},*/
	Quatd(-SQRT2P2,0,0,SQRT2P2), /* ??? {0,-SQRT2P2,SQRT2P2,0},*/
	Quatd(-1,0,0,0), /* {0,1,0,0}, */
};

static int g_sc = -1, g_cl = 1;
static double gpow = .5, gscale = 2.;

static void initar(){
	CvarAdd("g_sc", &g_sc, cvar_int);
	CvarAdd("g_cl", &g_cl, cvar_int);
	CvarAdd("gpow", &gpow, cvar_double);
	CvarAdd("gscale", &gscale, cvar_double);
}
static StaticInitializer s_initar(initar);

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



// Global ambient value for astronomical objects
float g_astro_ambient = .5f;
static void init_astro_ambient(){
	CvarAdd("g_astro_ambient", &g_astro_ambient, cvar_float);
}
static StaticInitializer ini(init_astro_ambient);



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
			Vec3d plane = sunp.vp(tp);
			Vec3d vp = plane.vp(delta);
			if(DBL_EPSILON < vp.slen()){
				Vec3d side = vp.norm();
				tp += side * -ps->rad;
			}
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
		double sunlen = sunp.len();
		double tplen = tp.len();
		if(sunlen <= DBL_EPSILON || tplen < DBL_EPSILON)
			return;
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
		timemeas_t tm;
		TimeMeasStart(&tm);
		PFNGLTEXIMAGE3DPROC glTexImage3D = (PFNGLTEXIMAGE3DPROC)wglGetProcAddress("glTexImage3D");
		if(GLenum err = glGetError())
			CmdPrint(dstring() << "ERR" << err << ": " << (const char*)gluErrorString(err));
		if(glTexImage3D){
			static const int NNOISE = 64;
			static GLubyte field[NNOISE][NNOISE][NNOISE][1];
		fprintf(stderr, "noise3DTexture: %g\n", TimeMeasLap(&tm));
			perlin_noise_3d(field, 48275);
		fprintf(stderr, "noise3DTexture: %g\n", TimeMeasLap(&tm));
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
		fprintf(stderr, "noise3DTexture: %g\n", TimeMeasLap(&tm));
	}
	return ret;
}



#define normvertex(x,y,z,p,ilen) normvertexf(x,y,z,p,ilen,i,j)


void DrawTextureSphere::useShader(){
	static GLuint noise3tex = noise3DTexture();
	GLuint shader = m_shader;
	double dist = (apos - vw->pos).len();
	if(shader && g_shader_enable){
		/// A class that prevents redundant uniform location lookups for every frame, in conjunction with std::map.
		struct Locs{
			GLint textureLoc;
			GLint noise3DLoc;
			GLint invEyeMat3Loc;
			GLint timeLoc;
			GLint heightLoc;
			GLint exposureLoc;
			void getLocs(GLuint shader){
				textureLoc = glGetUniformLocation(shader, "texture");
				noise3DLoc = glGetUniformLocation(shader, "noise3D");
				invEyeMat3Loc = glGetUniformLocation(shader, "invEyeRot3x3");
				timeLoc = glGetUniformLocation(shader, "time");
				heightLoc = glGetUniformLocation(shader, "height");
				exposureLoc = glGetUniformLocation(shader, "exposure");
			}
		};
		static std::map<GLuint, Locs> locmap;
		bool newLocs = locmap.find(shader) == locmap.end();
		Locs &locs = locmap[shader];

		if(newLocs)
			locs.getLocs(shader);

		glUseProgram(shader);

		if(0 <= locs.textureLoc && ptexlist){
			glUniform1i(locs.textureLoc, 0);
		}
		if(0 <= locs.noise3DLoc && ptexlist){
			glActiveTextureARB(GL_TEXTURE2_ARB);
			glUniform1i(locs.noise3DLoc, 2);
			glBindTexture(GL_TEXTURE_3D, noise3tex);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
		if(0 <= locs.invEyeMat3Loc){
			Mat4d rot2 = /*a->rot.tomat4() **/ vw->irot;
			Mat3<float> irot3 = rot2.tomat3().cast<float>();
			glUniformMatrix3fv(locs.invEyeMat3Loc, 1, GL_FALSE, irot3);
		}
		if(0 <= locs.timeLoc){
			glUniform1f(locs.timeLoc, GLfloat(vw->viewtime));
		}
		if(0 <= locs.heightLoc){
			glUniform1f(locs.heightLoc, GLfloat(max(0, dist - a->rad)));
		}
		if(0 <= locs.exposureLoc){
			extern double r_dynamic_range;
			glUniform1f(locs.exposureLoc, r_dynamic_range);
		}

		TexSphere::TextureIterator it;
		int i;
		if(m_textures) for(it = m_textures->begin(), i = 3; it != m_textures->end(); it++, i++){
			const TexSphere::Texture &tex = *it;
//			if(tex.shaderLoc == -2)
				tex.shaderLoc = glGetUniformLocation(shader, tex.uniformname);
			if(0 <= tex.shaderLoc){
				if(1&&!tex.list){
					tex.list = ProjectSphereCubeJpg(tex.filename, TexSphere::DTS_NODETAIL | tex.flags);
				}
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
		if(!(flags & TexSphere::DTS_NOGLOBE)){
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
		timemeas_t tm;
		TimeMeasStart(&tm);
//		texlist = *ptexlist = ProjectSphereJpg(texname);
		*ptexlist = ProjectSphereCubeJpg(texname, flags);
		CmdPrintf("DrawTextureSphere::draw(\"%s\") projection: %lg", texname, TimeMeasLap(&tm));
	} while(0);

	double (*cuts)[2] = CircleCuts(m_ncuts);
	double (*finecuts)[2] = CircleCutsPartial(m_nfinecuts, 9);

	double dist = (apos - vw->pos).len();
	if(dist < DBL_EPSILON)
		return false;

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

	if(m_flags & TexSphere::DTS_LIGHTING){
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

	if(flags & TexSphere::DTS_ADD){
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

	if(m_flags & TexSphere::DTS_LIGHTING){
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




#define DETAILSIZE 64
#define PROJTS 1024
#define PROJS (PROJTS/2)
#define PROJBITS 32 /* bit depth of projected texture */

static int PROJC = 512;
static void Init_PROJC(){
	CvarAdd("g_proj_size", &PROJC, cvar_int);
}
static StaticInitializer init_PROJC(Init_PROJC);

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

GLuint DrawTextureSphere::ProjectSphereCube(const char *name, const BITMAPINFO *raw, BITMAPINFO *cacheload[6], unsigned flags){
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
		int bits = flags & TexSphere::DTS_ALPHA ? 8 : PROJBITS;

		if(!cacheload){
			proj = (BITMAPINFO*)malloc(outsize = sizeof(BITMAPINFOHEADER) + (flags & TexSphere::DTS_ALPHA ? 256 * sizeof(RGBQUAD) : 0) + (PROJC * PROJC * bits + 31) / 32 * 4);
			memcpy(proj, raw, sizeof(BITMAPINFOHEADER));
			proj->bmiHeader.biWidth = proj->bmiHeader.biHeight = PROJC;
			proj->bmiHeader.biBitCount = bits;
			proj->bmiHeader.biClrUsed = flags & TexSphere::DTS_ALPHA ? 256 : 0;
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
					if(flags & TexSphere::DTS_ALPHA){
						double accum = 0.;
						for(int ii = 0; ii < 2; ii++) for(int jj = 0; jj < 2; jj++)
							accum += (jj ? fj1 : 1. - fj1) * (ii ? fi1 : 1. - fi1) * ((unsigned char *)&raw->bmiColors[raw->bmiHeader.biClrUsed])[(i1 + ii) % rawh * linebytes + (j1 + jj) % raww];
						*dst8 = (GLubyte)(accum);
					}
					else if(flags & TexSphere::DTS_NORMALMAP){
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
						// There could be the case 8 bit deep bitmap doesn't have a color samples table,
						// e.g. returned object of ReadJpeg() when a monochrome JPEG is opened.
						// In that case, we assume the single 8 bit channel as intensity value.
						if(raw->bmiHeader.biClrUsed){
							double accum[3] = {0}; // accumulator
							for(int ii = 0; ii < 2; ii++) for(int jj = 0; jj < 2; jj++) for(int c = 0; c < 3; c++)
								accum[c] += (jj ? fj1 : 1. - fj1) * (ii ? fi1 : 1. - fi1) * ((unsigned char *)&raw->bmiColors[((unsigned char *)&raw->bmiColors[raw->bmiHeader.biClrUsed])[(i1 + ii) % rawh * linebytes + (j1 + jj) % raww]])[c];
							dst->rgbRed = (GLubyte)accum[0];
							dst->rgbGreen = (GLubyte)accum[1];
							dst->rgbBlue = (GLubyte)accum[2];
						}
						else{
							double accum = 0; // accumulator
							for(int ii = 0; ii < 2; ii++) for(int jj = 0; jj < 2; jj++)
								accum += (jj ? fj1 : 1. - fj1) * (ii ? fi1 : 1. - fi1) * ((unsigned char *)&raw->bmiColors[raw->bmiHeader.biClrUsed])[(i1 + ii) % rawh * linebytes + (j1 + jj) % raww];
							dst->rgbRed = dst->rgbGreen = dst->rgbBlue = (GLubyte)accum;
						}
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
				glTexImage2D(cubetarget[nn], 0, flags & TexSphere::DTS_ALPHA ? GL_ALPHA : GL_RGBA,
					PROJC, PROJC, 0, flags & TexSphere::DTS_ALPHA ? GL_ALPHA : GL_RGBA, GL_UNSIGNED_BYTE, &proj->bmiColors[proj->bmiHeader.biClrUsed]);

			}
			else{
				glTexImage2D(cubetarget[nn], 0, flags & TexSphere::DTS_ALPHA ? GL_ALPHA : cacheload[nn]->bmiHeader.biBitCount / 8, cacheload[nn]->bmiHeader.biWidth, cacheload[nn]->bmiHeader.biHeight, 0,
					cacheload[nn]->bmiHeader.biBitCount == 32 ? GL_RGBA : cacheload[nn]->bmiHeader.biBitCount == 24 ? GL_RGB : cacheload[nn]->bmiHeader.biBitCount == 8 ? flags & TexSphere::DTS_ALPHA ? GL_ALPHA : GL_LUMINANCE : (assert(0), 0),
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

		if(glActiveTextureARB && !(flags & TexSphere::DTS_NODETAIL)){
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


GLuint DrawTextureSphere::ProjectSphereCubeJpg(const char *fname, int flags){
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

