/** \file
 * \brief Implementation of Island3 class.
 */
#include "island3.h"
#include "../astrodraw.h"
//#include "astro_star.h"
#include "../CoordSys.h"
#include "../Universe.h"
//#include "coordcnv.h"
#include "../player.h"
#include "../war.h"
//#include "bullet.h"
//#include "entity_p.h"
//#include "train.h"
//#include "warmap.h"
//#include "walk.h"
//#include "apache.h"
//#include "warutil.h"
#include "../glsl.h"
#include "../glstack.h"
#include "../material.h"
#include "../cmd.h"
extern "C"{
#include <clib/c.h>
#include <clib/gl/multitex.h>
#include <clib/amat3.h>
#include <clib/suf/suf.h>
#include <clib/suf/sufdraw.h>
#include <clib/suf/sufbin.h>
#include <clib/gl/gldraw.h>
#include <clib/gl/cull.h>
#include <clib/lzw/lzw.h>
#include <clib/timemeas.h>
}
#include <gl/glu.h>
#include <gl/glext.h>
#include <assert.h>


#define ISLAND3_RAD 3.25 /* outer radius */
#define ISLAND3_INRAD 3.2 /* inner radius */
#define ISLAND3_GRAD 3.24 /* inner glass radius */
#define ISLAND3_HALFLEN 16.
#define ISLAND3_FARMRAD 20.
#define MIRROR_LENGTH (ISLAND3_HALFLEN * 2.)
#define BRIDGE_HALFWID .01
#define BRIDGE_THICK .01


static int spacecolony_rotation(const struct coordsys *, aquat_t retq, const avec3_t pos, const avec3_t pyr, const aquat_t srcq);


/*
static void i3war_anim( *, double dt);
static void i3war_draw(struct war_field *, struct war_draw_data *);
static int i3war_pointhit(warf_t *w, const avec3_t *pos, const avec3_t *velo, double dt, const struct contact_info *);
static void i3war_accel(warf_t *w, avec3_t *dst, const avec3_t *srcpos, const avec3_t *srcvelo);
static double i3war_atmospheric_pressure(warf_t *w, const avec3_t *pos);
static double i3war_sonic_speed(warf_t *w, const avec3_t *pos);
static int i3war_spherehit(warf_t *w, const avec3_t *pos, double rad, struct contact_info *ci);
static int i3war_orientation(warf_t *w, amat3_t *dst, const avec3_t *pos);


static struct war_field_static i3war_static = {
	i3war_anim,
	WarPostFrame,
	WarEndFrame,
	i3war_draw,
	i3war_pointhit,
	i3war_accel,
	i3war_atmospheric_pressure,
	i3war_sonic_speed,
	i3war_spherehit,
	i3war_orientation,
	WarNearPlane,
	WarFarPlane,
};*/

/*warf_t i3warf = {
	&i3war_static,
};*/


class Island3 : public Astrobj{
public:
	static const unsigned classid;
	typedef Astrobj st;

	double sun_phase;

	Island3();
	virtual const char *classname()const{return "Island3";}
//	virtual void init(const char *path, CoordSys *root);
	virtual bool readFile(StellarContext &, int argc, char *argv[]);
	virtual bool belongs(const Vec3d &pos)const;
	virtual void anim(double dt);
	virtual void predraw(const Viewer *);
	virtual void draw(const Viewer *);
	virtual void drawtra(const Viewer *);

	static int &g_shader_enable;
protected:
	int getCutnum(const Viewer *vw)const;
	static GLuint walllist, walltex;
};

const unsigned Island3::classid = registerClass("Island3", Serializable::Conster<Island3>);
int &Island3::g_shader_enable = ::g_shader_enable;
GLuint Island3::walllist = 0;
GLuint Island3::walltex = 0;

Island3::Island3() : sun_phase(0.){
	absmag = 30.;
	rad = 100.;
	orbit_home = NULL;
	mass = 1e10;
	basecolor = 0xff8080;
	omg.clear();
}

bool Island3::readFile(StellarContext &sc, int argc, char *argv[]){
	return st::readFile(sc, argc, argv);
}

bool Island3::belongs(const Vec3d &lpos)const{
	double sdxy;
	sdxy = lpos[0] * lpos[0] + lpos[2] * lpos[2];
	return sdxy < ISLAND3_RAD * ISLAND3_RAD && -ISLAND3_HALFLEN - ISLAND3_INRAD < lpos[1] && lpos[1] < ISLAND3_HALFLEN;
}

void Island3::anim(double dt){
	Astrobj *sun = findBrightest();

	// Head toward sun
	if(sun){
		Vec3d sunpos = parent->tocs(pos, sun);
		omg[1] = sqrt(.0098 / 3.25);
		CoordSys *top = findcspath("/");
		double phase = omg[1] * (!top || !top->toUniverse() ? 0. : top->toUniverse()->global_time);
		Quatd qrot = Quatd::direction(sunpos);
		Quatd qrot1 = qrot.rotate(-M_PI / 2., avec3_100);
		this->rot = qrot1.rotate(phase, avec3_010);
	}

	// Calculate phase of the simulated sun.
	sun_phase += 2 * M_PI / 24. / 60. / 60. * dt;
}


struct randomness{
	GLubyte bias, range;
};

#define DETAILSIZE 64
static GLuint createdetail(unsigned long seed, int level, GLuint base, const struct randomness (*ran)[3]){
	int n;
	GLuint list;
	struct random_sequence rs;
	init_rseq(&rs, seed);
/*	list = base ? base : glGenLists(level);*/
	for(n = 0; n < level; n++){
		int i, j;
		int tsize = DETAILSIZE;
		GLubyte tbuf[DETAILSIZE][DETAILSIZE][3], tex[DETAILSIZE][DETAILSIZE][3];

		/* the higher you go, the less the ground details are */
		for(i = 0; i < tsize; i++) for(j = 0; j < tsize; j++){
			int buf[3] = {0};
			int k;
			for(k = 0; k < n*n+1; k++){
				buf[0] += (*ran)[0].bias + rseq(&rs) % (*ran)[0].range;
				buf[1] += (*ran)[1].bias + rseq(&rs) % (*ran)[1].range;
				buf[2] += (*ran)[2].bias + rseq(&rs) % (*ran)[2].range;
			}
			tbuf[i][j][0] = (GLubyte)(buf[0] / (n*n+1));
			tbuf[i][j][1] = (GLubyte)(buf[1] / (n*n+1));
			tbuf[i][j][2] = (GLubyte)(buf[2] / (n*n+1));
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
			if(tsize / 4 / 3 <= i % (tsize / 4) && i % (tsize / 4) < tsize / 4 * 2 / 3 && (tsize / 3) * 4.5 / 10 <= j % (tsize / 3) && j % (tsize / 3) < (tsize / 3) * 5.5 / 10){
				tex[i][j][0] = MIN(255, buf[0] / 9 / 2 + 191);
				tex[i][j][1] = MIN(255, buf[1] / 9 / 2 + 191);
				tex[i][j][2] = MIN(255, buf[2] / 9 / 2 + 191);
			}
			else{
				tex[i][j][0] = buf[0] / 9 / 2 + 31;
				tex[i][j][1] = buf[1] / 9 / 2 + 31;
				tex[i][j][2] = buf[2] / 9 / 2 + 31;
			}
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
			stp.env = GL_MODULATE;
			stp.mipmap = 1;
			stp.alphamap = 0;
			list = CacheSUFMTex("road.bmp", &stp, &stp);
		}

	}
	return list;
}


GLuint generate_road_texture(){
	static GLuint list = 0;
	static int init = 0;
	if(!init){
		struct randomness ran[3] = {{192, 64}, {192, 64}, {192, 64}};
#if 1
/*		list = glGenLists(1);*/
		list = createdetail(1, 1, list, &ran);
#else
		list = glGenLists(MIPMAPS * 2);
		createdetail(1, MIPMAPS, list, &ran);
		ran[0].bias = ran[1].bias = 0;
		ran[1].range = 32;
		ran[2].bias = 192;
		ran[2].range = 64;
		ran[0].bias = ran[1].bias = ran[2].bias = 128;
		ran[0].range = ran[1].range = ran[2].range = 128;
		createdetail(1, MIPMAPS, list + MIPMAPS, &ran);
#endif
		init = 1;
	}
	return list;
}

static GLuint cubetex = 0;
int g_reflesh_cubetex = 0;

#define CUBESIZE 512
#define SQRT2P2 (1.4142135623730950488016887242097/2.)

GLuint Island3MakeCubeMap(const Viewer *vw, const Astrobj *ignored, const Astrobj *sun){
	static const GLenum target[] = {
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	};
	static const Quatd dirs[] = {
		Quatd(SQRT2P2,0,SQRT2P2,0), /* {0,-SQRT2P2,0,SQRT2P2} */
		Quatd(SQRT2P2,0,0,SQRT2P2),
		Quatd(0,0,1,0), /* {0,0,0,1} */
		Quatd(-SQRT2P2,0,SQRT2P2,0), /*{0,SQRT2P2,0,SQRT2P2},*/
		Quatd(-SQRT2P2,0,0,SQRT2P2), /* ??? {0,-SQRT2P2,SQRT2P2,0},*/
		Quatd(-1,0,0,0), /* {0,1,0,0}, */
	};
	aquat_t omg0 = {0, 0, 1, 0};
	int i, g;
	Viewer v;
	struct viewport resvp = vw->vp;
	if(!g_reflesh_cubetex && cubetex)
		return cubetex;
	g_reflesh_cubetex = 0;

	v = *vw;
	v.detail = 0;
	v.fov = 1.;
	v.cs = vw->cs->findcspath("/");
	v.pos = v.cs->tocs(vw->pos, vw->cs);
	v.velo.clear();
	v.qrot = quat_u;
	v.relative = 0;
	v.relirot = mat4_u;
	v.relrot = mat4_u;
	v.velolen = 0.;
	v.vp.h = v.vp.m = v.vp.w = CUBESIZE;
	v.dynamic_range = 1e-4;
	glViewport(0,0,v.vp.w, v.vp.h);

	if(!cubetex){
		glGenTextures(1, &cubetex);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubetex);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
	GLcull g_glcull(v.fov, vec3_000, v.relirot, 1e-15, 1e+15);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, 0);
	GLpmatrix pmat;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-g_glcull.getNear(), g_glcull.getNear(), -g_glcull.getNear(), g_glcull.getNear(), g_glcull.getNear(), g_glcull.getFar());
	glMatrixMode (GL_MODELVIEW);
	for (i = 0; i < 6; ++i) {
		GLubyte buf[CUBESIZE][CUBESIZE][3];
		int mi = CUBESIZE;
		glPushMatrix();
		v.irot = (v.qrot = dirs[i]).cnj().tomat4();
		v.rot = dirs[i].tomat4();
		v.relrot = v.rot;
		v.relirot = v.irot;
		Mat4d proj;
		glGetDoublev(GL_PROJECTION_MATRIX, proj);
		v.trans = v.rot * proj;
		glLoadMatrixd(v.rot);
#if 0 && defined _DEBUG
		glClearColor(0,.5f,0,0);
#else
		glClearColor(0,0,0,0);
#endif
		glClear(GL_COLOR_BUFFER_BIT);
		drawstarback(&v, v.cs, NULL, sun);

		glBindTexture(GL_TEXTURE_CUBE_MAP, cubetex);
		glTexImage2D(target[i], 0, GL_RGB, mi, mi, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		g = glGetError();
		glCopyTexSubImage2D(target[i], 0, 0, 0, 0, 0, CUBESIZE, CUBESIZE);
		if(!g)
			g = glGetError();
		if(g){
			CmdPrintf((const char*)gluErrorString(g));
			glReadPixels(0, 0, CUBESIZE, CUBESIZE, GL_RGB, GL_UNSIGNED_BYTE, buf);
			glTexImage2D(target[i], 0, GL_RGB, mi, mi, 0,
				GL_RGB, GL_UNSIGNED_BYTE, buf);
		}
		glPopMatrix();
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	glPopAttrib();
	glViewport(0,0,vw->vp.w, vw->vp.h);
	return cubetex;
}

void Island3::predraw(const Viewer *vw){
	static bool cubeMapped = false;
	if(!cubeMapped){
		cubeMapped = true;
		Island3MakeCubeMap(vw, this, findBrightest());
	}
	st::predraw(vw);
}


static GLint invEyeMat3Loc, textureLoc, cubeEnvLoc, fracLoc, lightLoc;

GLuint Reflist(){
	static GLuint reflist = 0;
	if(reflist)
		return reflist + !!Island3::g_shader_enable * 2;
#if 1
	static const GLenum target[] = {
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	};
	static double genfunc[][4] = {
	{ 1.0, 0.0, 0.0, 0.0 },
	{ 0.0, 1.0, 0.0, 0.0 },
	{ 0.0, 0.0, 1.0, 0.0 },
	{ 0.0, 0.0, 0.0, 1.0 },
	};
	static GLuint texnames[6];
	reflist = glGenLists(4);
	glNewList(reflist + 2, GL_COMPILE);
	do{
		GLuint vtx, frg;
		GLuint shader;
//					GLint cubeTexLoc, nrmmapLoc;
		vtx = glCreateShader(GL_VERTEX_SHADER);
		frg = glCreateShader(GL_FRAGMENT_SHADER);
		if(!glsl_load_shader(vtx, "shaders/mirror.vs") || !glsl_load_shader(frg, "shaders/mirror.fs"))
			break;
		shader = glsl_register_program(vtx, frg);

		textureLoc = glGetUniformLocation(shader, "texture");
		cubeEnvLoc = glGetUniformLocation(shader, "envmap");
		invEyeMat3Loc = glGetUniformLocation(shader, "invEyeRot3x3");

		glUseProgram(shader);
//					glUniform1i(cubeTexLoc, cubetex);
//					glUniform1i(nrmmapLoc, 0);
		glUniform1i(textureLoc, 1);
		glUniform1i(cubeEnvLoc, 0);
	}while(0);
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubetex);
	glEnable(GL_TEXTURE_CUBE_MAP);
//	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);
	glEndList();

	glNewList(reflist + 3, GL_COMPILE);
	glUseProgram(0);
	glEndList();

	glNewList(reflist, GL_COMPILE);
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubetex);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
//	glDisable(GL_BLEND);
	glEndList();

	glNewList(reflist + 1, GL_COMPILE);
	glUseProgram(0);
	glEndList();
#endif
	return reflist + !!Island3::g_shader_enable * 2;
}


static const avec3_t pos0[] = {
	{0., 17., 0.},
	{3.25, 16., 0.},
	{3.25, 12., 0.},
	{3.25, 8., 0.},
	{3.25, 4., 0.},
	{3.25, .0, 0.},
	{3.25, -4., 0.},
	{3.25, -8., 0.},
	{3.25, -12., 0.},
	{3.25, -16., 0.},
	{0., -17., 0.},
};

int Island3::getCutnum(const Viewer *vw)const{
	int cutnum = 48 * 4;
	double pixels = fabs(vw->gc->scale(pos)) * 32.;

	if(pixels < 1)
		return cutnum;
	else if(pixels < 50)
		cutnum /= 16;
	else if(pixels < 125)
		cutnum /= 8;
	else if(pixels < 250)
		cutnum /= 4;
	else if(pixels < 500)
		cutnum /= 2;
	return cutnum;
}


void Island3::draw(const Viewer *vw){
	bool farmap = !!vw->zslice;
	GLcull *gc2 = vw->gclist[0];
	if(farmap)
		return;

	static bool multitex = false;
	if(!multitex){
		multitex = true;
		MultiTextureInit();
	}

	Vec3d pos = vw->cs->tocs(vec3_000, this);

	static GLuint roadlist = 0, roadtex = 0, groundtex = 0, groundlist, noisetex;
	static bool init = false;
	if(!init){
		init = 1;
		walllist = CallCacheBitmap("bricks.bmp", "models/bricks.bmp", NULL, NULL);
		if(const suftexcache *stc = FindTexCache("bricks.bmp"))
			walltex = stc->tex[0];
//			roadlist = generate_road_texture();
//			roadtex = FindTexCache("road.bmp")->tex[0];
		suftexparam_t stp;
		stp.flags = STP_WRAP_S | STP_WRAP_T | STP_MAGFIL | STP_MINFIL;
		stp.wraps = GL_MIRRORED_REPEAT;
		stp.wrapt = GL_MIRRORED_REPEAT;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		groundlist = CallCacheBitmap("grass.jpg", "textures/grass.jpg", &stp, NULL);
		if(const suftexcache *stc = FindTexCache("grass.jpg"))
			groundtex = stc->tex[0];
		CallCacheBitmap("noise.jpg", "textures/noise.jpg", &stp, NULL);
		if(const suftexcache *stc = FindTexCache("noise.jpg"))
			noisetex = stc->tex[0];
	}

	/* Shader is togglable on running */
	GLuint reflist = Reflist();

	static avec3_t norm0[] = {
		{0., 1., 0.},
		{1., 0., 0.},
		{1., 0., 0.},
		{1., 0., 0.},
		{1., 0., 0.},
		{1., 0., 0.},
		{1., 0., 0.},
		{1., 0., 0.},
		{1., 0., 0.},
		{1., 0., 0.},
		{0., -1., 0.},
	};
	const double sufrad = 3.25;
	int n, i;
	int finecuts[48 * 4]; /* this buffer must be greater than cuts */

//	double pixels = fabs(vw->gc->scale(pos)) * 32.;
	int cutnum = getCutnum(vw);

	double (*cuts)[2] = CircleCuts(cutnum);

	Vec3d vpos = tocs(vw->pos, vw->cs);

	GLmatrix glma;
	GLattrib glatt(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_TRANSFORM_BIT);

	if(farmap){
		glLoadMatrixd(vw->rot);
		Vec3d delta = pos - vw->pos;
		gldTranslate3dv(delta);
	}
	else
		glTranslated(pos[0], pos[1], pos[2]);

//	brightness = Island3Light();
	double brightness = (sin(sun_phase) + 1.) / 2.;
	{
		GLfloat dif[4] = {1., 1., 1., 1.}, amb[4] = {1., 1., 1., 1.};
		Astrobj *sun = findBrightest(vec3_000);
		if(sun){
			Vec4<float> lightpos;
			glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
			glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
			Vec3d sunpos = vw->cs->tocs(sun->pos, sun->parent);
			lightpos = sunpos.norm().cast<float>();
			lightpos[3] = 0.f;
			glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
		}
	}
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	glColor4ub(255,255,255,255);
	{
		GLfloat spe[4] = {0., 0., 0., 1.};
		glMaterialfv(GL_FRONT, GL_SPECULAR, spe);
	}

	int defleap = 2, leap = defleap;

	/* pre-calculate LOD information to reduce checking */
	{
		Mat4d rot2 = mat4_u;
		Mat4d trans = vw->cs->tocsm(this);
		trans.vec3(3) = vw->cs->tocs(vec3_000, this);
		for(i = 0; i < cutnum; i += defleap){
			rot2[0] = cuts[i][1];
			rot2[2] = -cuts[i][0];
			rot2[8] = cuts[i][0];
			rot2[10] = cuts[i][1];
			Vec3d pos = trans.vp3(rot2.vp3(pos0[1]));
			if((vw->pos[0] - pos[0]) * (vw->pos[0] - pos[0]) + (vw->pos[2] - pos[2]) * (vw->pos[2] - pos[2]) < .3 * .3){
				finecuts[i] = 1;
			}
			else
				finecuts[i] = 0;
		}
	}

	Mat4d rot2 = mat4_u;
	Mat4d rot = vw->cs->tocsim(this);
	glMultMatrixd(rot);
	Mat4d mat;
	glGetDoublev(GL_MODELVIEW_MATRIX, mat);
	glDisable(GL_BLEND);

	int northleap = -ISLAND3_HALFLEN - 3. * ISLAND3_RAD < vpos[1] && vpos[1] < -ISLAND3_HALFLEN + ISLAND3_RAD ? 1 : 2;

	/* hull draw */
	for(n = 0; n < 2; n++){
		Mat4d rot;
		const double hullrad = n ? ISLAND3_RAD : ISLAND3_INRAD;
		if(n){
			GLfloat dif[4] = {.75f, .75f, .75f, 1.}, amb[4] = {.2f, .2f, .2f, 1.};
			glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
			glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
			glFrontFace(GL_CCW);
			glCallList(walllist);
		}
		else{
			GLfloat dif[4] = {.3f, .3f, .3f, 1.}, amb[4] = {.7f, .7f, .7f, 1.};
			glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
			glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
			glFrontFace(GL_CW);
			glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT);
			glCallList(groundlist);
		}
		rot = mat4_u;
		for(int i = 0; i < cutnum; i += leap){
			int i1, j;
			Mat4d *prot[4];

			/* leap adjust for dynamic LOD */
			if(i % defleap || finecuts[i])
				leap = defleap / 2;
			else
				leap = defleap;

			i1 = (i+leap) % cutnum;

			/* set rotation matrices cyclically */
			prot[0] = prot[3] = &rot2;
			prot[1] = prot[2] = &rot;
			glBegin(GL_QUADS);

			/* northan hemisphere */
			for(j = 0; j < cutnum / 4 /*/ (i % (leap * 2) + 1)*/; j += defleap * northleap){
				int j1 = (j + defleap * northleap) % cutnum, k, i2 = /*i % (leap * 2) == 0 && !(j1 < cutnum / 8) ? (i+leap*2) % cutnum :*/ i1;
				Vec3d pos[4], pos1[2];
				rot2[0] = cuts[i2][1];
				rot2[2] = -cuts[i2][0];
				rot2[8] = cuts[i2][0];
				rot2[10] = cuts[i2][1];
				pos1[0][0] = hullrad * cuts[j][1];
				pos1[0][1] = -ISLAND3_HALFLEN - hullrad * cuts[j][0];
				pos1[0][2] = pos1[1][2] = 0.;
				pos1[1][0] = hullrad * cuts[j1][1];
				pos1[1][1] = -ISLAND3_HALFLEN - hullrad * cuts[j1][0];
				for(k = 0; k < 4; k++){
					pos[k] = prot[k]->dvp3(pos1[k/2]);
				}
				for(k = 0; k < 4; k++){
					Vec3d viewpos = mat.vp3(pos[k]);
					if(gc2->getFar() < -viewpos[2])
						break;
				}
				if(farmap ? 4 == k : 4 != k)
					continue;
				pos1[0][1] += ISLAND3_HALFLEN;
				pos1[0] /= hullrad;
				pos1[1][1] += ISLAND3_HALFLEN;
				pos1[1] /= hullrad;
				for(k = 0; k < 4; k++){
					double st[2];
					Vec3d norm = prot[k]->dvp3(pos1[k / 2]);
					if(n)
						glNormal3dv(norm);
					else{
						VECSCALEIN(norm, -1.);
						glNormal3dv(norm);
					}
					st[0] = pos1[k / 2][1];
					st[1] = (i % 16 + leap * !!(9 & (1<<k))) / 8.;
					glTexCoord2dv(st);
					if(glMultiTexCoord2fARB)
						glMultiTexCoord2fARB(GL_TEXTURE1_ARB, GLfloat(st[0] * 16.), GLfloat(st[1] * 16.));
					glVertex3dv(pos[k]);
				}
			}

			rot2[0] = cuts[i1][1];
			rot2[2] = -cuts[i1][0];
			rot2[8] = cuts[i1][0];
			rot2[10] = cuts[i1][1];

			for(j = 0; j < numof(pos0)-2; j++) if(i % (cutnum / 3) < cutnum / 6 || j < 1 || numof(pos0)-2 <= j){
				int k;
				Vec3d pos[4], pos001[2];
				pos001[0] = pos0[j];
				pos001[1] = pos0[j+1];
				if(0 < j && j < numof(pos0)-2){
					if(!n){
						pos001[0][0] = pos001[1][0] = ISLAND3_INRAD;
					}
					else{
						pos001[0][0] = pos001[1][0] = ISLAND3_RAD;
					}
				}
				else{
					if(j == 0)
						pos001[1][0] = hullrad/*ISLAND3_INRAD*/;
					else if(j == numof(pos0)-1)
						pos001[0][0] = ISLAND3_INRAD;
				}
				pos[0] = rot2.dvp3(pos001[0]);
				pos[1] = rot.dvp3(pos001[0]);
				pos[2] = rot.dvp3(pos001[1]);
				pos[3] = rot2.dvp3(pos001[1]);
				for(k = 0; k < 4; k++){
					Vec3d viewpos = mat.vp3(pos[k]);
					if(gc2->getFar() < -viewpos[2])
						break;
				}
				if(farmap ? 4 == k : 4 != k)
					continue;
				for(k = 0; k < 4; k++){
					double st[2];
					Vec3d norm = prot[k]->dvp3(norm0[j + k / 2]);
					if(n)
						glNormal3dv(norm);
					else{
						norm *= -1.;
						glNormal3dv(norm);
					}
					st[0] = pos0[j + k / 2][1];
					st[1] = (i % 16 + leap * !!(9 & (1<<k))) / 8.;
					glTexCoord2dv(st);
					if(glMultiTexCoord2fARB)
						glMultiTexCoord2fARB(GL_TEXTURE1_ARB, GLfloat(st[0] * 16.), GLfloat(st[1] * 16.));
					glVertex3dv(pos[k]);
				}
			}
			glEnd();
			rot = rot2;
		}
		if(!n){
			glPopAttrib();
//			shadow_draw(csint->w);
		}
	}

	/* seal at glass bondaries */
	{
		int n;
		Mat4d rot2 = mat4_u;
		for(n = 0; n < 2; n++){
			int m;
			Vec3d bpos0[2], norm;
			glFrontFace(n ? GL_CCW : GL_CW);
			bpos0[0][0] = ISLAND3_INRAD;
			bpos0[1][0] = ISLAND3_RAD;
			bpos0[0][1] = bpos0[1][1] = n ? -ISLAND3_HALFLEN : ISLAND3_HALFLEN;
			bpos0[0][2] = bpos0[1][2] = 0.;
			norm = (1 - n * 2) * rot2.vec3(2);
			glNormal3dv(norm);
			for(m = 0; m < 3; m++){
				int i;
				glBegin(GL_QUAD_STRIP);
				for(i = 0; i <= cutnum / 6; i += leap){
					int i1 = (i + m * cutnum / 3 + cutnum / 6) % cutnum;
					rot2[0] = cuts[i1][1];
					rot2[2] = -cuts[i1][0];
					rot2[8] = cuts[i1][0];
					rot2[10] = cuts[i1][1];
					if(i1 % defleap || finecuts[i1])
						leap = defleap / 2;
					else
						leap = defleap;
					Vec3d bpos = rot2.dvp3(bpos0[0]);
					glTexCoord2d(0., i);
					glVertex3dv(bpos);
					bpos = rot2.dvp3(bpos0[1]);
					glTexCoord2d(1., i);
					glVertex3dv(bpos);
				}
				glEnd();
			}
		}
	}

	/* walls between inner/outer cylinder */
	for(int i = 0; i < cutnum; i += cutnum / 6){
		int i1 = i;
		Mat4d rot = mat4_u;
		rot[0] = cuts[i1][1];
		rot[2] = -cuts[i1][0];
		rot[8] = cuts[i1][0];
		rot[10] = cuts[i1][1];
		glNormal3dv(i % (cutnum / 3) == 0 ? rot.vec3(2) : -rot.vec3(2));
		glBegin(GL_QUAD_STRIP);
		for(int j = 1; j < numof(pos0)-1; j++){
			Vec3d pos00 = pos0[j];
			pos00[0] = i % (cutnum / 3) == 0 ? ISLAND3_RAD : ISLAND3_INRAD;
			Vec3d pos = rot.vp3(pos00);
			glTexCoord3dv(pos00);
			glVertex3dv(pos);
			pos00[0] = i % (cutnum / 3) == 0 ? ISLAND3_INRAD : ISLAND3_RAD;
			pos = rot.vp3(pos00);
			glTexCoord3dv(pos00);
			glVertex3dv(pos);
		}
		glEnd();
	}
	leap = defleap;


	/* solar mirror wings */
	for(n = 0; n < 2; n++){
		if(n == 0){
			GLfloat dif[4] = {.010f, .01f, .02f, 1.}, amb[4] = {.005f, .005f, .01f, 1.}, black[4] = {0,0,0,1}, spc[4] = {.5, .5, .5, .55f};
			amb[1] = GLfloat((1. - brightness) * .005);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
			glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
			glMaterialfv(GL_FRONT, GL_EMISSION, black);
			glMaterialfv(GL_FRONT, GL_SPECULAR, spc);
			glPushAttrib(GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
			glCallList(reflist);
			glActiveTextureARB(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, noisetex);
			glActiveTextureARB(GL_TEXTURE0);
			Mat4d rot = vw->cs->tocsm(vw->cs->findcspath("/"));

			if(g_shader_enable){
				Mat4d rot2 = rot * vw->irot;
				Mat3<float> irot3 = rot2.tomat3().cast<float>();
				glUniformMatrix3fv(invEyeMat3Loc, 1, GL_FALSE, irot3);
//					glUniform1i(cubeEnvLoc, 0);
//					glUniform1f(fracLoc, g_shader_frac);
				glBindTexture(GL_TEXTURE_CUBE_MAP, cubetex);
			}
			else{
				glMatrixMode(GL_TEXTURE);
				glPushMatrix();
				glMultMatrixd(rot);
				glMultMatrixd(vw->irot);
				glMatrixMode(GL_MODELVIEW);
			}
			glFrontFace(GL_CW);
		}
		else{
			GLfloat dif[4] = {.75f, .75f, .75f, 1.}, amb[4] = {.2f, .2f, .2f, 1.}, spc[4] = {0., 0., 0., .15f};
			glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
			glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
			glMaterialfv(GL_FRONT, GL_SPECULAR, spc);
			glFrontFace(GL_CCW);
		}
		for(int m = 0; m < 3; m++){
			glPushMatrix();
			glRotated(m * 120. + 60., 0., 1., 0.);
			Vec3d joint(3.25 * cos(M_PI * 3. / 6. + M_PI / 6.), 16., 3.25 * sin(M_PI * 3. / 6. + M_PI / 6.));
			gldTranslate3dv(joint);
			glRotated(brightness * 30., -1., 0., 0.);
			gldTranslaten3dv(joint);
			glGetDoublev(GL_MODELVIEW_MATRIX, mat);
			glColor4ub(0,0,0,63);
			for(i = 0; i < 1; i += leap){
				int i1 = cutnum / 6;
				glBegin(GL_QUADS);
				glNormal3d(0., 0., n ? 1 : -1.);
				for(int j = 1; j < numof(pos0)-2; j++){
					Vec3d pos[4];
					int k;
					for(k = 0; k < 4; k++){
						pos[k][0] = ((6 & (1<<k) ? i : i1) - cutnum / 12) * ISLAND3_RAD * 2 / (cutnum / 6) / sqrt(3.);
						pos[k][1] = pos0[j+k/2][1];
						pos[k][2] = ISLAND3_RAD;
					}
					for(k = 0; k < 4; k++){
						Vec3d viewpos = mat.vp3(pos[k]);
						if(gc2->getFar() < -viewpos[2])
							break;
					}
					if(farmap ? 4 == k : 4 != k)
						continue;
					for(k = 0; k < 4; k++){
						glTexCoord2dv(pos[k]);
						glVertex3dv(pos[k]);
					}
				}
				glEnd();
			}

			glPopMatrix();
		}
		if(!n){
			glCallList(reflist + 1);
			glPopAttrib();
			glMatrixMode(GL_TEXTURE);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
		}
	}

}

void Island3::drawtra(const Viewer *vw){
	GLcull *gc = vw->gc, *gc2 = vw->gclist[0];
	double x0, srad;
	double pixels;

/*	for(csint = children; csint; csint = csint->next) if(csint->vft == &g_spacecolony_s){
		break;
	}
	if(!csint || !csint->w)
		return;*/
	Vec3d pos = vw->cs->tocs(avec3_000, this);
	if(gc->cullFrustum(pos, 50.))
		return;
	pixels = fabs(gc->scale(pos)) * 32.;
	if(pixels < 1)
		return;

	srad = vw->pos[0] * vw->pos[0] + vw->pos[2] * vw->pos[2];
	x0 = (srad < ISLAND3_INRAD * ISLAND3_INRAD ? ISLAND3_INRAD : ISLAND3_RAD) * sin(M_PI / 6.);
	if(vw->cs == this) for(int n = 0; n < 3; n++){
		amat4_t rot;
		avec3_t plpos;
		MAT4ROTY(rot, mat4identity, n * 2. * M_PI / 3.);
		MAT4DVP3(plpos, rot, vw->pos);
		if(-x0 < plpos[0] && plpos[0] < x0 && (plpos[2] < 0. || srad < ISLAND3_RAD * ISLAND3_RAD)){
			int i;
			double brightness, mirrorslope, rayslope;
			double as = .2, sas, cas;
			double (*cuts)[2];
			brightness = (sin(sun_phase) + 1.) / 2.;
			as = brightness * M_PI / 6.;
			sas = sin(as);
			cas = cos(as);
			mirrorslope = sas / cas;
			plpos[1] -= -cas * MIRROR_LENGTH + ISLAND3_HALFLEN;
			plpos[2] -= -sas * MIRROR_LENGTH - ISLAND3_RAD;
			if(plpos[2] < mirrorslope * plpos[1])
				continue;
			rayslope = tan(M_PI - 2. * (M_PI / 2. - as));
			if(plpos[1] * rayslope < plpos[2])
				continue;
			cuts = CircleCuts(10);

			glPushMatrix();
			glLoadMatrixd(vw->rot);
			glRotated(n * 120., 0., -1., 0.);
			glRotated(90. + 2. * brightness * 30., 1., 0., 0.);
			glBegin(GL_TRIANGLE_FAN);
			glColor4ub(255, 255, 255, (unsigned char)(brightness * 192.));
			glVertex3d(0., 0., 1.);
			glColor4ub(255, 255, 255, 0);
			for(i = 0; i <= 10; i++){
				int k = i % 10;
				glVertex3d(cuts[k][0] * sas, cuts[k][1] * sas, cas);
			}
			glEnd();
			glPopMatrix();
		}
	}

	// Glasses 
	bool farmap = !!vw->zslice;
	double brightness = (sin(sun_phase) + 1.) / 2.;
	int cutnum = getCutnum(vw);
	int defleap = 2, leap = defleap;
	double (*cuts)[2] = CircleCuts(cutnum);
	Mat4d rot2 = mat4_u;
	GLattrib gla(GL_TEXTURE_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LESS);
	glPushMatrix();
	Mat4d mat = vw->cs->tocsim(this);
	mat.vec3(3) = (vw->cs->tocs(vec3_000, this));
	glMultMatrixd(mat);
	glGetDoublev(GL_MODELVIEW_MATRIX, mat);
/*		glDepthFunc(vpos[0] * vpos[0] + vpos[2] * vpos[2] < ISLAND3_RAD * ISLAND3_RAD ? GL_LEQUAL : GL_LESS);*/
	{
//		GLfloat dif[4] = {.35, .4, .45, .5}, spc[4] = {.1, .1, .1, 1.};
//		VECSCALEIN(dif, brightness * .75 + .25);
//		glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
//		glMaterialfv(GL_FRONT, GL_AMBIENT, dif);
/*			glMaterialfv(GL_FRONT, GL_SPECULAR, spc);*/
//			glFrontFace(GL_CCW);
		glCallList(walllist);
		glEnable(GL_CULL_FACE);
	}
	for(int n = 0; n < 2; n++){
		if(n){
			glFrontFace(GL_CCW);
			if(g_shader_enable){
				glActiveTextureARB(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, walltex);
				glActiveTextureARB(GL_TEXTURE0);
				glCallList(Reflist());
			}
//			glUniform1i(cubeEnvLoc, 1);
			glColor4f(.3f, .3f, .3f, .3f);
		}
		else{
			glFrontFace(GL_CW);
			glColor4f(.5, .5, .5, .3f);
		}
		for(int m = 0; m < 3; m++){
			int i0 = (cutnum * m / 3 + cutnum / 6) % cutnum;
/*				glPushMatrix();*/
/*				glRotated(m * 120. + 60., 0., 1., 0.);*/
			glGetDoublev(GL_MODELVIEW_MATRIX, mat);
			Mat4d rot = mat4_u;
			rot[0] = cuts[i0][1];
			rot[2] = -cuts[i0][0];
			rot[8] = cuts[i0][0];
			rot[10] = cuts[i0][1];
			for(int i = 0; i < cutnum / 6; i += leap){
				int i1, j;
				if(2 <= defleap){
					Vec3d pos = rot.vp3(pos0[1]);
					if(i % defleap == 0 && (vw->pos[0] - pos[0]) * (vw->pos[0] - pos[0]) + (vw->pos[2] - pos[2]) * (vw->pos[2] - pos[2]) < .3 * .3){
						leap = defleap / 2;
					}
					else if(leap != defleap && (i) % defleap == 0)
						leap = defleap;
				}
				i1 = (i0+i+leap) % cutnum;
				rot2[0] = cuts[i1][1];
				rot2[2] = -cuts[i1][0];
				rot2[8] = cuts[i1][0];
				rot2[10] = cuts[i1][1];
/*					MAT4ROTY(rot2, rot, 2 * M_PI / cutnum);*/
				glBegin(GL_QUADS);
				for(j = 1; j < numof(pos0)-2; j++){
					int k;
					Vec3d pos[4];
					Vec3d pos00 = pos0[j], pos01 = pos0[j+1];
					if(0 < j && j < numof(pos0)-2){
						pos00[0] = !n ? ISLAND3_GRAD : ISLAND3_RAD;
						pos01[0] = !n ? ISLAND3_GRAD : ISLAND3_RAD;
					}
					pos[0] = rot2.vp3(pos00);
					pos[1] = rot.vp3(pos00);
					pos[2] = rot.vp3(pos01);
					pos[3] = rot2.vp3(pos01);
					for(k = 0; k < 4; k++){
						Vec3d viewpos = mat.vp3(pos[k]);
						if(gc2->getFar() < -viewpos[2])
							break;
					}
					if(farmap ? 4 == k : 4 != k)
						continue;
					for(k = 0; k < 4; k++){
						Vec3d norm;
						if(n){
							norm[0] = pos[k][0];
							norm[1] = 0.;
							norm[2] = pos[k][2];
						}
						else{
							norm[0] = -pos[k][0];
							norm[1] = 0.;
							norm[2] = -pos[k][2];
						}
						glNormal3dv(norm);
						glTexCoord2d(pos0[j + k / 2][1], (i % 8 + leap * !!(9 & (1<<k))) / 8.);
						glVertex3dv(pos[k]);
					}
				}
				glEnd();
				rot = rot2;
			}
/*				glPopMatrix();*/
		}
		if(n && g_shader_enable)
			glCallList(Reflist() + 1);

	}
	glPopMatrix();

#if 0
	/* Bridge tower navlights */
	if(vw->zslice == 0){
		GLubyte col[4] = {255,0,0,255};
		amat4_t rot2;
		double t;
		double (*cuts)[2];
		int cutnum = 48 * 4;
		cuts = CircleCuts(cutnum);
		t = fmod(csint->w->war_time, 10.);
		col[3] = t < 5 ? t / 5. * 255 : (10. - t) / 5. * 255;
		glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		for(m = 0; m < 3; m++){
			int j, i, n;
			MAT4IDENTITY(rot2);
			for(j = 1; j < numof(pos0)-1; j++) for(i = 1; i < 4; i++){
				int i1 = (i * cutnum / (4 * 6) + m * cutnum / 3 + cutnum / 6) % cutnum;
				avec3_t bpos, bpos0;
				rot2[0] = cuts[i1][1];
				rot2[2] = -cuts[i1][0];
				rot2[8] = cuts[i1][0];
				rot2[10] = cuts[i1][1];
				for(n = 0; n < 2; n++){
					bpos0[0] = ISLAND3_GRAD - .201;
					bpos0[1] = pos0[j][1] + (BRIDGE_HALFWID + .001) * (n * 2 - 1);
					bpos0[2] = 0.;
					mat4dvp3(bpos, rot2, bpos0);
		/*			if(glcullFar(bpos, .5, gc2))
						continue;*/
					gldSpriteGlow(bpos, .002, col, vw->irot);
				}
			}
		}
		glPopAttrib();
	}
#endif
}

#if 0
static void Island3DrawGrass(const Viewer *vw){
	const Mat4d &rotmat = vw->rot;
	const Vec3d &view = vw->pos;
	const Mat4d &inv_rot = vw->irot;
	int i, count = 1000, total = 0;
	struct random_sequence rs;
	timemeas_t tm;
	const double destvelo = .01; /* destination velocity */
	const double width = .00002 * 1000. / vw->vp.m; /* line width changes by the viewport size */
	double plpos[3];
	avec3_t ortpos;
	avec3_t nh0 = {0., 0., -1.}, nh;
	amat4_t mat;
	int level;
	int tris = 0, fan = 0;
	Universe *univ = findcspath("/")->toUniverse();
	double t = univ ? univ->astro_time : 0.;
/*	GLfloat rotaxisf[16] = {
		1., 0., 0., 0.,
		0., 0., -1., 0.,
		0., 1., 0., 0.,
		0., 0., 0., 1.,
	};
	GLfloat irotaxisf[16];*/
/*	if(!grassing)
		return;*/

	Mat3d ort3 = this->rotation(pos, Vec3d(0,0,0), rot).tomat4().tomat3();
//	cs->w->vft->orientation(cs->w, &ort3, &vw->pos);
	Mat3d iort3 = ort3.transpose();
	Mat4<float> rotaxisf = ort3.cast<float>();
	Mat4<float> irotaxisf = iort3.cast<float>();

	{
		double phase;
		phase = atan2(vw->pos[0], vw->pos[2]);
		MATVP(ortpos, iort3, vw->pos);
		ortpos[0] += phase * 3.2;
		ortpos[1] += ISLAND3_INRAD;
	}

/*	TimeMeasStart(&tm);*/
	glPushMatrix();
	glLoadMatrixd(rotmat);
	glMultMatrixf(rotaxisf);
	{
		amat4_t modelmat, persmat, imodelmat;
		glGetDoublev(GL_MODELVIEW_MATRIX, modelmat);
		glGetDoublev(GL_PROJECTION_MATRIX, persmat);
		persmat[14] = 0.;
		mat4mp(mat, persmat, modelmat);
		MAT4TRANSPOSE(imodelmat, modelmat);
		MAT4DVP3(nh, imodelmat, nh0);
	}
/*	MAT4VP3(nh, *inv_rot, nh0);*/
/*	glTranslated(-(*view)[0], -(*view)[1], -(*view)[2]);*/
	glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
/*	glDisable(GL_LIGHTING);*/
	glEnable(GL_NORMALIZE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(1);
	for(level = 5; level; level--){
		double cellsize;
		double disp[3];
		if(0. + .05 * (1<<level) < ortpos[1])
			break;
		init_rseq(&rs, level + 23342);
		cellsize = .005 * (1<<level);
		count = level == 5 ? 50 : 100;
		for(i = count; i; i--){
			double pos[3], lpos[3], dst[3], x0, x1, z0, z1, base;
			double phase;
			double phase2;
			unsigned long red;

			pos[0] = ortpos[0] + (drseq(&rs) - .5) * 2 * cellsize;
			pos[0] = /*pl.pos[0] +*/ +(floor(pos[0] / cellsize) * cellsize + cellsize / 2. - pos[0]);
			pos[1] = .0001 + (.00003 + drseq(&rs) * .00003) * (4. + (1<<level));
			pos[2] = ortpos[2] + (drseq(&rs) - .5) * 2 * cellsize;
			pos[2] = /*pl.pos[2] +*/ +(floor(pos[2] / cellsize) * cellsize + cellsize / 2. - pos[2]);
			x0 = (.00001 + (drseq(&rs) - .5) * .00005) * (1+level);
			x1 = (.00001 + (drseq(&rs) - .5) * .00005) * (1+level);
			z0 = (.00001 + (drseq(&rs) - .5) * .00005) * (1+level);
			z1 = (.00001 + (drseq(&rs) - .5) * .00005) * (1+level);
			red = rseq(&rs);
/*			if((-1. < pos[0] && pos[0] < 1. && -1. < pos[2] && pos[2] < 1.
				? -.01 < pos[0] && pos[0] < .01
				: (pos[2] < -.01 || .01 < pos[2])))
				continue;*/
			base = 0. - ortpos[1];
			pos[1] += base;
	#if 0
			if(glcullFrustum(&pos, .005, &g_glcull))
				continue;
	#elif 1
			if(pos[2] + ortpos[2] < -ISLAND3_HALFLEN || ISLAND3_HALFLEN < pos[2] + ortpos[2])
				continue;
			{
				avec4_t vec;
				double d, period = ISLAND3_INRAD * M_PI * 2. / 3.;
				d = pos[0] + ortpos[0] + ISLAND3_INRAD * M_PI / 6.;
				d /= period;
				if(.5 < d - floor(d))
					continue;
				VECCPY(dst, pos);
				VECSADD(dst, nh, .003 / vw->gc->getFov());
				MAT4VP3(vec, mat, dst);
				if(vec[2] < 0. || vec[0] < -vec[2] || vec[2] < vec[0] || vec[1] < -vec[2] || vec[2] < vec[1])
					continue;
			}
	#else
			VECSUB(dst, pos, pl.pos);
	/*			VECSADD(dst, velo, -.08);*/
			if(VECSP(pos, nh) < -0.005)
				continue;
	#endif
			{
				double apos[3];
				VECADD(apos, pos, vw->pos);
				phase = (apos[0] + apos[2]) / .00241 + t / .2;
				phase2 = 1.232123 * ((apos[0] - apos[2]) / .005 + t / .2);
				disp[0] = pos[0] + (pos[1] - base) * .15 * (sin(phase) + sin(phase2));
				disp[1] = pos[1];
				disp[2] = pos[2] + (pos[1] - base) * .15 * (cos(phase) + cos(phase2));
			}
			if(level == 5){
				struct random_sequence rs;
				double org[2][3];
				int j, c;
				init_rseq(&rs, i);
				memcpy(org[0], pos, sizeof org[0]);
				org[0][1] = base;
				memcpy(org[1], pos, sizeof org[0]);
				org[1][0] += x1 * 2;
				org[1][1] = base;
				org[1][2] += z1 * 2;
				frexp(1e2 * VECLEN(org[0]), &c);
				c = 4 - c;
				if(4 < c) c = 4;
				if(0 <= c){
					GLubyte col[4] = {128, 0, 0, 255};
					avec3_t norm;
					if(tris || fan){
						tris = fan = 0;
						glEnd();
					}
					VECSCALE(norm, pos, -1.);
					glNormal3dv(norm);
					glBegin(GL_QUADS);
					for(j = 0; j < 4; j++){
//						glColor4ubv(BlendLight(col));
//						drawtree(org, 4, 4 - c, &rs);
					}
					glEnd();
				}
			}
#define flowermod 47
			if(red % flowermod < 3){
				int j, k;
				double (*cuts)[2];
				GLdouble v[3][3] = {
					{0., 0.0001, 0.},
					{.0001, .00015, .00005},
					{.0001, .00015, -.00005},
				};
				GLubyte cc[4], oc[4];
				if(tris){
					tris = 0;
					glEnd();
				}
				if(!fan){
					glBegin(GL_TRIANGLE_FAN);
				}
				if(red % flowermod == 0)
					cc[0] = 191, cc[1] = 128, cc[2] = 0, cc[3] = 255, oc[0] = 255, oc[1] = 255, oc[2] = 0, oc[3] = 255;
				else if(red % flowermod == 1)
					cc[0] = 128, cc[1] = 0, cc[2] = 191, cc[3] = 255, oc[0] = 255, oc[1] = 0, oc[2] = 255, oc[3] = 255;
				else if(red % flowermod == 2)
					cc[0] = 128, cc[1] = 128, cc[2] = 191, cc[3] = 255, oc[0] = 255, oc[1] = 255, oc[2] = 255, oc[3] = 255;
//				glColor4ubv(BlendLight(cc));
				glVertex3dv(disp);
				cuts = CircleCuts(5);
//				glColor4ubv(BlendLight(oc));
				for(j = 0; j <= 5; j++){
					k = j % 5;
					glVertex3d(disp[0] + .0001 * cuts[k][0], disp[1] + .00003, disp[2] + .0001 * cuts[k][1]);
				}
				glEnd();
			}
			{
				double disp2[3];
				if(fan){
					fan = 0;
					glEnd();
				}
/*				if(!tris){
					tris = 1;
					glBegin(GL_TRIANGLES);
				}*/

				disp2[0] = pos[0] + (pos[1] - base) * .15 * (sin(phase + .1 * M_PI) + sin(phase2 + .1 * M_PI)) / 2.;
				disp2[1] = (pos[1] + base) / 2.;
				disp2[2] = pos[2] + (pos[1] - base) * .15 * (cos(phase + .1 * M_PI) + cos(phase2 + .1 * M_PI)) / 2.;

				glBegin(GL_TRIANGLES);
				glColor4ub(red % 50 + 31,255,0,255);
				glVertex3d(disp[0], disp[1], disp[2]);
				glColor4ub(red % 50 + 31,191,0,255);
				glVertex3d(disp2[0] + x0 / 2., disp2[1], disp2[2] + z0 / 2.);
				glVertex3d(disp2[0] + x1 / 2., disp2[1], disp2[2] + z1 / 2.);
				glEnd();

				glBegin(GL_QUADS);
				glVertex3d(disp2[0] + x0 / 2., disp2[1], disp2[2] + z0 / 2.);
				glVertex3d(disp2[0] + x1 / 2., disp2[1], disp2[2] + z1 / 2.);
				glColor4ub(red % 50 + 31,127,0,255);
				glVertex3d(pos[0] + x1, base, pos[2] + z1);
				glVertex3d(pos[0] + x0, base, pos[2] + z0);
				glEnd();
			}
			if(red % 2423 < 513){
				avec3_t v0, v1, v2, v01, v02, vp;
				glBegin(GL_TRIANGLES);
				v0[0] = disp[0] - z0, v0[1] = disp[1], v0[2] = disp[2] + x0;
				v1[0] = pos[0] + x0, v1[1] = base, v1[2] = pos[2] + z0;
				v2[0] = pos[0] + x1, v2[1] = base, v2[2] = pos[2] + z1;
				VECSUB(v01, v1, v0);
				VECSUB(v02, v2, v0);
				VECVP(vp, v01, v02);
				glNormal3dv(vp);
				glColor4ub(red % 50 + 31,255,0,255);
				glVertex3dv(v0);
				glColor4ub(red % 50 + 31,127,0,255);
				glVertex3dv(v1);
				glVertex3dv(v2);
				glColor4ub(red % 50 + 31,255,0,255);
				glVertex3d(disp[0] + z0, disp[1], disp[2] - x0);
				glColor4ub(red % 50 + 31,127,0,255);
				glVertex3d(pos[0] + x0, base, pos[2] + z0);
				glVertex3d(pos[0] + x1, base, pos[2] + z1);
				glEnd();
			}
			total++;
		}
	}
	if(tris || fan)
		glEnd();
	glPopAttrib();
	glPopMatrix();
/*	fprintf(stderr, "grass[%3d]: %lg sec\n", total, TimeMeasLap(&tm));*/
}
#endif
