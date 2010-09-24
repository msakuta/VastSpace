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
//#include "glsl.h"
#include "../glstack.h"
#include "../material.h"
extern "C"{
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

#define BMPSRC_LINKAGE static
//#include "bmpsrc/bricks.h"
//#include "bmpsrc/bbrail.h"

//#include "sufsrc/scolony.h"
//#include "sufsrc/bridgetower.h"


#define numof(a) (sizeof(a)/sizeof*(a))

#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef ABS
#define ABS(a) ((a)<0?-(a):(a))
#endif


#define ISLAND3_RAD 3.25 /* outer radius */
#define ISLAND3_INRAD 3.2 /* inner radius */
#define ISLAND3_GRAD 3.24 /* inner glass radius */
#define ISLAND3_HALFLEN 16.
#define ISLAND3_FARMRAD 20.
#define MIRROR_LENGTH (ISLAND3_HALFLEN * 2.)
#define BRIDGE_HALFWID .01
#define BRIDGE_THICK .01


static void spacecolony_anim(struct coordsys *, double dt);
static int spacecolony_belongs(const struct coordsys *, const avec3_t pos, const struct coordsys *pos_cs);
static int spacecolony_rotation(const struct coordsys *, aquat_t retq, const avec3_t pos, const avec3_t pyr, const aquat_t srcq);

#if 0
struct coordsys_static g_spacecolony_s = {
	spacecolony_anim, /* void (*anim)(struct coordsys *, double dt); */
	NULL, /* postframe */
	NULL, /* endframe */
	NULL, /* void (*draw)(struct astrobj *, const struct viewer *); */
	NULL,
	NULL,
	spacecolony_belongs,
	spacecolony_rotation,
};
#endif

/*extern coordsys *g_spacecolony;*/
extern double sun_phase;
extern PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;

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

/*
static void lagrange1_anim(struct war_field *, double dt);
static void lagrange1_draw(struct war_field *, struct war_draw_data *);
static int lagrange1_pointhit(warf_t *w, const avec3_t *pos, const avec3_t *velo, double dt, const struct contact_info *);
static void lagrange1_accel(warf_t *w, avec3_t *dst, const avec3_t *srcpos, const avec3_t *srcvelo);
static double lagrange1_atmospheric_pressure(warf_t *w, const avec3_t *pos);
static double lagrange1_sonic_speed(warf_t *w, const avec3_t *pos);
static int lagrange1_spherehit(warf_t *w, const avec3_t *pos, double rad, struct contact_info *ci);
static int lagrange1_orientation(warf_t *w, amat3_t *dst, const avec3_t *pos);
*/
/*static struct war_field_static lagrange1_static = {
	lagrange1_anim,
	lagrange1_draw,
	lagrange1_pointhit,
	lagrange1_accel,
	lagrange1_atmospheric_pressure,
	lagrange1_sonic_speed,
	lagrange1_spherehit,
	lagrange1_orientation,
};*/

class Island3 : public Astrobj{
public:
	static const unsigned classid;
	typedef Astrobj st;
	Island3();
	virtual const char *classname()const{return "Island3";}
//	virtual void init(const char *path, CoordSys *root);
	virtual bool readFile(StellarContext &, int argc, char *argv[]);
	virtual bool belongs(const Vec3d &pos)const;
	virtual void anim(double dt);
	virtual void draw(const Viewer *);
};

const unsigned Island3::classid = registerClass("Island3", Serializable::Conster<Island3>);

Island3::Island3(){
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
	if(sun){
		Vec3d sunpos = parent->tocs(pos, sun);
		omg[1] = sqrt(.0098 / 3.25);
		CoordSys *top = findcspath("/");
		double phase = omg[1] * (!top || !top->toUniverse() ? 0. : top->toUniverse()->global_time);
		Quatd qrot = Quatd::direction(sunpos);
		Quatd qrot1 = qrot.rotate(-M_PI / 2., avec3_100);
		this->rot = qrot1.rotate(phase, avec3_010);
	}
}


struct randomness{
	GLubyte bias, range;
};

#define DETAILSIZE 64
static GLuint createdetail(unsigned long seed, GLuint level, GLuint base, const struct randomness (*ran)[3]){
	GLuint n;
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


void Island3::draw(const Viewer *vw){
	bool farmap = vw->zslice;
	GLcull *gc2 = vw->gclist[0];
	if(farmap)
		return;

	Vec3d pos = vw->cs->tocs(vec3_000, this);

	static GLuint walltex, walllist = 0, roadlist = 0, roadtex = 0, groundtex = 0, groundlist;
	static bool init = false;
	if(!init){
		suftexparam_t stp;
		init = 1;
	//	CacheTexture("bricks.bmp");
//		CacheMaterial("bricks.bmp");
		{
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
		}
	}


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
	Mat4d rot2, mat;
	int n, i, cutnum = 48 * 4, defleap = 2, leap = defleap;
	double brightness;
	double (*cuts)[2];
	int finecuts[48 * 4]; /* this buffer must be greater than cuts */

	double pixels = fabs(vw->gc->scale(pos)) * 32.;

	if(pixels < 1)
		return;
	else if(pixels < 50)
		cutnum /= 16;
	else if(pixels < 125)
		cutnum /= 8;
	else if(pixels < 250)
		cutnum /= 4;
	else if(pixels < 500)
		cutnum /= 2;

	cuts = CircleCuts(cutnum);

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

	rot2 = mat4_u;

//	brightness = Island3Light();
	brightness = 1;
	{
		GLfloat dif[4] = {1., 1., 1., 1.}, amb[4] = {1., 1., 1., 1.};
		Astrobj *sun = findBrightest(vec3_000);
		if(sun){
			Vec4<float> lightpos;
			glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
			glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
			Vec3d sunpos = parent->tocs(sun->pos, sun->parent);
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
			if((vw->pos[0] - pos[0]) * (vw->pos[0] - pos[0]) + (vw->pos[2] - pos[2]) * (vw->pos[2] - pos[2]) < 1.3 * 1.3){
				finecuts[i] = 1;
			}
			else
				finecuts[i] = 0;
		}
	}

//	glCallList(CacheMaterial("bricks.bmp"));
	Mat4d rot = vw->cs->tocsim(this);
	glMultMatrixd(rot);
	glGetDoublev(GL_MODELVIEW_MATRIX, mat);

//	glDisable(GL_LIGHTING);
	glColor4f(1,1,1,1);

	int northleap = -ISLAND3_HALFLEN - 3. * ISLAND3_RAD < vpos[1] && vpos[1] < -ISLAND3_HALFLEN + ISLAND3_RAD ? 1 : 2;

	/* hull draw */
	for(n = 0; n < 2; n++){
		Mat4d rot;
		const double hullrad = n ? ISLAND3_RAD : ISLAND3_INRAD;
		int m;
		if(n){
			GLfloat dif[4] = {.75, .75, .75, 1.}, amb[4] = {.2, .2, .2, 1.};
			glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
			glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
			glFrontFace(GL_CCW);
			glCallList(walllist);
		}
		else{
			GLfloat dif[4] = {.3, .3, .3, 1.}, amb[4] = {.7, .7, .7, 1.};
/*				VECSCALEIN(dif, brightness * .5 + .5);*/
/*			if(weather != wsnow){
				dif[0] = dif[2] = .1;
				amb[0] = amb[2] = .2;
			}
			VECSCALEIN(amb, brightness * .75 + .25);*/
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
/*				if(2 <= defleap){
				avec3_t pos;
				MAT4VP3(pos, rot, pos0[1]);
				if(i % defleap == 0 && (vw->pos[0] - pos[0]) * (vw->pos[0] - pos[0]) + (vw->pos[2] - pos[2]) * (vw->pos[2] - pos[2]) < .3 * .3){
					leap = defleap / 2;
				}
				else if(leap != defleap && (i) % defleap == 0)
					leap = defleap;
			}*/

			i1 = (i+leap) % cutnum;
/*				MAT4ROTY(rot2, rot, 2 * M_PI / cutnum);*/

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
						glMultiTexCoord2fARB(GL_TEXTURE1_ARB, st[0] * 16., st[1] * 16.);
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
						glMultiTexCoord2fARB(GL_TEXTURE1_ARB, st[0] * 16., st[1] * 16.);
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

}
