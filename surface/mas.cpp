/** \file
 * \brief mas.c - Map AScii, actually does drawing too */

#include "WarMap.h"
#include "drawmap.h"
#include "Player.h"
#include "astro.h"
#include "material.h"
#include "bitmap.h"
//#include "glw/glwindow.h"
extern "C"{
#include <clib/c.h>
#include <clib/avec4.h>
#include <clib/aquat.h>
#include <clib/amat4.h>
#include <clib/gl/cull.h>
#include <clib/lzw/lzw.h>
#include <clib/suf/sufdraw.h>
#include <clib/rseq.h>
#include <clib/timemeas.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/gl/multitex.h>
}
#define exit something_meanless
/*#include <GL/glut.h>*/
#ifdef _WIN32
#include <windows.h>
//#include <wingraph.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#undef exit
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define BMPSRC_LINKAGE static
//#include "bmpsrc/block.h"

#ifndef M_PI
#define M_PI 3.14159265358979
#endif

#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif

#ifndef ABS
#define ABS(a) ((a)<0?-(a):(a))
#endif

#define ROOT2 1.41421356

void interpolate_normal(WarMap *wm, double x, double y, double (*ret)[3]);

class Mas : public WarMap{
public:
	virtual int getat(wartile_t *, int x, int y);
	virtual void size(int *x, int *y);
	virtual double width();
	virtual void levelterrain(int x0, int y0, int x1, int y1);
//	virtual int linehit(const Vec3d &src, const Vec3d &dir, double dt, Vec3d &ret);
	int sx, sy;
	wartile_t *value;
	Mas(int sx, int sy) : sx(sx), sy(sy){
		value = new wartile_t[sx * sy];
	}
};

WarMap *OpenMas(const char *fname){
	FILE *fp;
	char buf[128];
	Mas *ret = NULL;
	fp = fopen(fname, "r");
	if(!fp)
		return NULL;
	fprintf(stderr, "file %s opened.\n", fname);
	fgets(buf, sizeof buf, fp);
	if(strcmp(buf, "#mas\n")){
		fclose(fp);
		return NULL;
	}
	fprintf(stderr, "mas signature found.\n");
	{
		int x, y, i, j;
		int unit;
		if(!fgets(buf, sizeof buf, fp) || 2 != sscanf(buf, "size %d %d\n", &x, &y)){
			fclose(fp);
			return NULL;
		}
		fprintf(stderr, "mas size is %d x %d.\n", x, y);
		if(!fgets(buf, sizeof buf, fp) || strncmp(buf, "unit ", 5) || strncmp(&buf[5], "m", 1)){
			fclose(fp);
			return NULL;
		}
		fprintf(stderr, "mas unit is meter.\n");
		ret = new Mas(x, y);
		for(j = 0; j < y; j++) for(i = 0; i < x; i++){
			fgets(buf, sizeof buf, fp);
			ret->value[i + j * x].height = 3e-5 * atof(buf);
		}
	}
	fclose(fp);
	return ret;
}

#define MAXPOINTS 512 /* i dont know. */


static void imagedata_openerror(const char *s){
  fprintf(stderr, "%s\n", s);
}

/* all we concern about drawing the data is the resolution. */
static void read_params(Mas *p, const char *buf)
{
  char temp[512];
  const char *str;
  int tempint,gindex;

/*  if (str = strstr(buf,"Width"), str!=NULL)
  {
    str+=10;
    sscanf(str,"%lf",&p->width);    
    return;
  }*/

	if (str = strstr(buf,"Div"), str!=NULL){
		int div;
		str+=10;
		sscanf(str,"%d",&div);    
		p->sy = p->sy = div;
		return;
	}

	return ;
}


int Mas::getat(wartile_t *wt, int x, int y){
	*wt = value[(sx <= x ? sx-1 : x) + (sy <= y ? sy-1 : y) * sx];
	return 1;
}

void Mas::size(int *x, int *y){
	*x = sx;
	*y = sy;
}

double Mas::width(){
	return 8.;
}

void Mas::levelterrain(int x0, int y0, int x1, int y1){
	int x, y;
	double avg = 0.;
	short v;
	x1++;
	y1++;
	for(x = x0; x <= x1; x++) for(y = y0; y <= y1; y++){
		wartile_t &t = value[(sx <= x ? sx-1 : x) + (sy <= y ? sy-1 : y) * sx];
		avg += t.height;
	}
	avg /= (x1 - x0 + 1) * (y1 - y0 + 1);
	v = avg / .001;
	for(x = x0; x <= x1; x++) for(y = y0; y <= y1; y++){
		value[(sx <= x ? sx-1 : x) + (sy <= y ? sy-1 : y) * sx].height = v;
	}
}


static void interpolate_normal(WarMap *wm, double x, double y, double (*ret)[3]){
}

#if 1

class MemoryMap : public WarMap{
	int getat(wartile_t *, int x, int y);
	void size(int *x, int *y);
	double width();
};

WarMap *CreateMemap(void){
	return new MemoryMap();
}

int MemoryMap::getat(wartile_t *wt, int x, int y){
	double hoy = 16. * 16. - ((x - 16.) * (x - 16.) + (y - 16.) * (y - 16.));
	wt->height = hoy > 0 ? (hoy) * 2e-4 : 0.;
	return 1;
}

void MemoryMap::size(int *x, int *y){
	*x = 32;
	*y = 32;
}

double MemoryMap::width(){
	return 4.;
}


class RepeatMap : public WarMap{
public:
	RepeatMap(WarMap *src, int xmin, int ymin, int xmax, int ymax);
	~RepeatMap();
	int getat(wartile_t *, int x, int y);
	void size(int *x, int *y);
	double width();
	Vec3d normal(double x, double y);
	void levelterrain(int x0, int y0, int x1, int y1);
protected:
	WarMap *src;
	int x0, y0, x1, y1;
	static const double rmap_falloff;
};

const double RepeatMap::rmap_falloff = .01;

RepeatMap::RepeatMap(WarMap *src, int xmin, int ymin, int xmax, int ymax) : src(src){
	if(!src)
		throw std::exception(__FILE__ ": Argument is NULL");
	nalt = 0;
	alt = NULL;
	x0 = xmin;
	y0 = ymin;
	x1 = xmax;
	y1 = ymax;
}

int RepeatMap::getat(wartile_t *wt, int x, int y){
	int xs, ys;
	int ret;
	int size;
	double cenx, ceny;
	double orgx, orgy;
	double cell;
	double xx, yy;
	double width;
	RepeatMap *m = this;
	src->size(&xs, &ys);
	if(x < m->x0 * xs || m->x1 * xs < x || y < m->y0 * ys || m->y1 * ys < y)
		return 0;
	size = MAX(xs, ys) * (m->x1 - m->x0);
	width = m->width();
	cell = width / size;
	cenx = 0./*orgx + map_width / 2.*/;
	ceny = 0./*orgy + map_width / 2.*/;
	orgx = -width / 2.;
	orgy = -width / 2.;
	xx = orgx + x * cell;
	yy = orgy + y * cell;
	if(-.02 < xx && xx < .02 || -.02 < yy && yy < .02){
		wt->height = 0.;
		return 1;
	}
	ret = src->getat(wt, (x - m->x0 * xs) % xs, (y - m->y0 * ys) % ys);
	wt->height -= (double)((xx - cenx) * (xx - cenx) + (yy - ceny) * (yy - ceny)) * rmap_falloff;
	wt->height *= (1. - .04 / (ABS(xx) + .02)) * (1. - .04 / (ABS(yy) + .02));
	return ret;
}

void RepeatMap::size(int *x, int *y){
	int xs, ys;
	src->size(&xs, &ys);
	*x = xs * (x1 - x0);
	*y = ys * (y1 - y0);
}

double RepeatMap::width(){
	return src->width();
}

Vec3d RepeatMap::normal(double x, double y){
	int xs, ys;
	src->size(&xs, &ys);
	return src->normal(fmod(x - x0 * xs, xs), fmod(y - y0 * ys, ys));
}

RepeatMap::~RepeatMap(){
	delete src;
}

void RepeatMap::levelterrain(int x0, int y0, int x1, int y1){
	src->levelterrain(x0, y0, x1, y1);
}
#endif


GLuint generate_ground_texture();

static void normalmap(WarMap *wm, int i, int j, int sx, int sy, double cell, Vec3d *ret){
	wartile_t t, tr, td, tl, tu;
	avec3_t total, nh, xh, zh;
	double height = 0;
	int count = 0, k = 0;
	wm->getat(&t, i, j);
	if(i < sx-1) wm->getat(&tr, i+1, j), k |= 1;
	if(j < sy-1) wm->getat(&td, i, j+1), k |= 2;
	if(0 < i) wm->getat(&tl, i-1, j), k |= 4;
	if(0 < j) wm->getat(&tu, i, j-1), k |= 8;
	VECNULL(total);
	if(!~(k | ~3)){
		count++;
		xh[0] = cell;
		xh[1] = tr.height - t.height;
		xh[2] = 0;
		zh[0] = 0;
		zh[1] = td.height - t.height;
		zh[2] = cell;
		VECVP(nh, zh, xh);
		VECNORMIN(nh);
		VECADDIN(total, nh);
	}
	if(!~(k | ~9)){
		count++;
		xh[0] = cell;
		xh[1] = tr.height - t.height;
		xh[2] = 0;
		zh[0] = 0;
		zh[1] = tu.height - t.height;
		zh[2] = -cell;
		VECVP(nh, xh, zh);
		VECNORMIN(nh);
		VECADDIN(total, nh);
	}
	if(!~(k | ~12)){
		count++;
		xh[0] = -cell;
		xh[1] = tl.height - t.height;
		xh[2] = 0;
		zh[0] = 0;
		zh[1] = tu.height - t.height;
		zh[2] = -cell;
		VECVP(nh, zh, xh);
		VECNORMIN(nh);
		VECADDIN(total, nh);
	}
	if(!~(k | ~6)){
		count++;
		xh[0] = -cell;
		xh[1] = tl.height - t.height;
		xh[2] = 0;
		zh[0] = 0;
		zh[1] = td.height - t.height;
		zh[2] = cell;
		VECVP(nh, xh, zh);
		VECNORMIN(nh);
		VECADDIN(total, nh);
	}
	if(count){
		VECSCALEIN(total, 1. / count);
		glNormal3dv(total);
		if(ret)
			VECCPY(*ret, total);
	}
	else if(ret)
		VECCPY(*ret, avec3_010);
}

/*warmapdecal_t *AllocWarmapDecal(unsigned size){
	warmapdecal_t *ret;
	ret = malloc(offsetof(warmapdecal_t, p) + size * sizeof(warmapdecalelem_t));
	ret->drawproc = NULL;
	ret->freeproc = NULL;
	ret->actv = NULL;
	ret->frei = ret->p;
	ret->p[--size].next = NULL;
	do{
		size--;
		ret->p[size].next = &ret->p[size+1];
	} while(size);
	return ret;
}

void FreeWarmapDecal(warmapdecal_t *wmd){
	warmapdecalelem_t *p;
	if(wmd->freeproc) for(p = wmd->actv; p; p = p->next){
		wmd->freeproc(p);
	}
	free(wmd);
}

int AddWarmapDecal(warmapdecal_t *wmd, const double (*pos)[2], void *data){
	warmapdecalelem_t *p;
	if(wmd->frei){
		p = wmd->frei;
		wmd->frei = wmd->frei->next;
		p->next = wmd->actv;
		wmd->actv = p;
	}
	else{
		warmapdecalelem_t **pp;
		assert(wmd->actv);
		for(pp = &wmd->actv; (*pp)->next; pp = &(*pp)->next);
		p = *pp;
		p->next = wmd->actv;
		*pp = NULL;
		wmd->actv = p;
	}
	p->pos[0] = (*pos)[0];
	p->pos[1] = (*pos)[1];
	p->data = data;
	return 1;
}*/


struct randomness{
	GLubyte bias, range;
};

#define DETAILSIZE 64
static GLuint createdetail(unsigned long seed, GLuint level, GLuint base, const struct randomness (*ran)[3]){
#if 1
	suftexparam_t stp;
	stp.flags = STP_WRAP_S | STP_WRAP_T | STP_MAGFIL | STP_MINFIL;
	stp.wraps = GL_MIRRORED_REPEAT;
	stp.wrapt = GL_MIRRORED_REPEAT;
	stp.magfil = GL_LINEAR;
	stp.minfil = GL_LINEAR;
	return CallCacheBitmap("grass.jpg", "textures/grass.jpg", &stp, NULL);
#else

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
			tbuf[i][j][0] = buf[0] / (n*n+1);
			tbuf[i][j][1] = buf[1] / (n*n+1);
			tbuf[i][j][2] = buf[2] / (n*n+1);
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
#if 1
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
			stp.flags = STP_ENV | STP_MIPMAP;
			stp.bmi = (BITMAPINFO*)&bmi;
			stp.env = GL_MODULATE;
			stp.mipmap = 1;
			stp.alphamap = 0;
			list = CacheSUFMTex("grass.bmp", &stp, &stp);
		}
#else
		glNewList(list + n, GL_COMPILE);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		gluBuild2DMipmaps(GL_TEXTURE_2D, 3, DETAILSIZE, DETAILSIZE, GL_RGB, GL_UNSIGNED_BYTE, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
/*		{
			GLfloat ff[4] = {0., 1., 0., 1.};
			glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, ff);
		}*/
		glEnable(GL_TEXTURE_2D);
		glEndList();
#endif
	}
	return list;
#endif
}


GLuint generate_ground_texture(){
	static GLuint list = 0;
	static int init = 0;
	if(!init){
		struct randomness ran[3] = {{128, 64}, {128, 64}, {128, 64}};
/*		struct randomness ran[3] = {{15, 32}, {96, 64}, {15, 32}};*/
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



/* recursive call consumes the stack as hell, so we put the common variables here.
  make sure not to multiple threads to use the function at the same time. */
typedef struct drawmapnode{
	double x, z, h, gnd;
	double t[2][2];
/*	double n[3];*/
	int ix, iy; /* integral indices of map array, dont matter in intermidiate vertex */
	int valid, skip, inter, sealevel;
} dmn;

class DrawMap{
	static GLuint tex0, tex;
	static double g_map_lod_factor;
	static double detail_factor;
	int checkmap_maxlevel, checkmap_invokes;
	int underwater;
	int watersurface;
	int flatsurface;
	double drawmapt;
	Vec3d map_viewer;
	Vec4f mat_diffuse;
	Vec4f mat_ambient_color;
	static GLuint matlist;
	const dmn *last_dmn;
	int polys, drawmap_invokes, shadows;
	GLcull *g_pglcull;
	WarMap *wm;
	//static warmapdecal_t *gwmd = NULL;
	//void *gwmdg = NULL;
	int gsx, gsy;
	double g_cell;
	int g_scale;
	GLuint lasttex;
	double orgx, orgy;
	double map_width;
	short (*pwmdi)[2];
	short wmdi[2046][2]; ///< warmapdecal index cache
public:
	DrawMap(WarMap *wm, const Vec3d &pos, int detail, double t, GLcull *glc, /*warmapdecal_t *wmd, void *wmdg,*/ char **ptop, int *checked);
protected:
	void checkmap(char *top, int x, int y, int level);
	double drawmap_height(dmn *d, char *top, int sx, int sy, int x, int y, int level, int lastpos[2]);
	void drawmap_node(const dmn *d);
	void drawmap_face(dmn *d);
	void drawmap_in(char *top, int x, int y, int level);
};
extern avec3_t cp0[16];

GLuint DrawMap::tex0 = 0, DrawMap::tex = 0;
double DrawMap::g_map_lod_factor = 1.;
double DrawMap::detail_factor = 2.;
GLuint DrawMap::matlist = 0;

void drawmap(WarMap *wm, const Vec3d &pos, int detail, double t, GLcull *glc, /*warmapdecal_t *wmd, void *wmdg,*/ char **ptop, int *checked){
	try{
		DrawMap(wm, pos, detail, t, glc, ptop, checked);
	}
	catch(...){
		fprintf(stderr, __FILE__": ERROR!\n");
	}
}

#define EARTH_RAD 6378.
#define EPSILON .01

DrawMap::DrawMap(WarMap *wm, const Vec3d &pos, int detail, double t, GLcull *glc, /*warmapdecal_t *wmd, void *wmdg,*/ char **ptop, int *checked) :
	checkmap_maxlevel(0),
	checkmap_invokes(0),
	underwater(0),
	watersurface(0),
	flatsurface(0),
	drawmapt(0.),
	mat_diffuse(.75, .75, .75, 1.0),
	mat_ambient_color(1., 1., 1., 1.0),
	last_dmn(NULL),
	polys(0), drawmap_invokes(0), shadows(0),
	g_pglcull(NULL),
	wm(wm),
	gsx(0), gsy(0),
	g_scale(1),
	lasttex(0),
	orgx(-3601 * .03 / 2.), orgy(-3601 * .03 / 2.),
	map_width(3601 * .03)
{
	int i, j, sx, sy;
	Vec4f dif(1., 1., 1., 1.), amb(0.2, .2, 0.2, 1.);
/*	GLfloat dif[4] = {0.125, .5, 0.125, 1.}, amb[4] = {0., .2, 0., 1.};*/
#ifndef NDEBUG
	static volatile long running = 0;
#endif
	static int init = 0;
	static GLuint list;
	static char *top = NULL;
	int ex;
	int size;
#if MAPPROFILE
	timemeas_t tm2, tm;
	double checkmap_time, textime, decaltime = 0., drawtime;
#endif
	GLuint oldtex;

	if(!wm || !pos)
		return;

	glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&oldtex);

#if MAPPROFILE
	TimeMeasStart(&tm2);
#endif

#ifndef NDEBUG
	if(running){
		assert(0);
		return;
	}
	running = 1;
#endif

	map_viewer = pos;

#if 1

	wm->size(&sx, &sy);
	gsx = sx, gsy = sy;
	size = MAX(sx, sy);

	for(ex = 0; 1 << ex < size; ex++);

	if(!*ptop){
		int i;
		size_t vol = 0;

		/* volume of the pyramid buffer */
/*		TimeMeasStart(&tm);*/
		for(i = 0; i <= ex; i++)
			vol += ((1 << (2 * i)) + 7) / 8;
		*ptop = (char*)malloc(vol/*((4 << (2 * ex)) - 1) / 3*/);
		top = *ptop;
		*checked = 0;
		memset(top, 0, vol);
/*		printf("allc: %lg\n", TimeMeasLap(&tm));*/
	}

	/* its way faster to assign value at checkmap function,
	  for majority of the pyramid buffer is not even accessed. */
/*		TimeMeasStart(&tm);
	memset(top, 0, ((4 << (2 * ex)) - 1) / 3);
		printf("mset: %lg\n", TimeMeasLap(&tm));*/

	detail_factor = (glc ? 4. / (glc->getFov() + .5) : 3.) * g_map_lod_factor;
	checkmap_invokes = 0;
	map_width = wm->width();
	orgx = -map_width / 2.;
	orgy = -map_width / 2.;
	{
		wartile_t t;
		int x = (map_viewer[0] - orgx) * sx / map_width, y = (map_viewer[2] - orgy) * sy / map_width;
		if(0 <= x && x < sx && 0 <= y && y < sy){
			wm->getat(&t, x, y);
			checkmap_maxlevel = map_viewer[1] < t.height + 5. ? ex : (int)(ex / (1. + ((map_viewer[1] - (t.height + 5.)) / map_width)));
		}
		else
			checkmap_maxlevel = ex;
	}

#if MAPPROFILE
	TimeMeasStart(&tm);
#endif
	if(!*checked){
		*checked = 1;
		checkmap(top, 0, 0, 0);
	}
	checkmap_maxlevel = ex;
#if MAPPROFILE
	checkmap_time = TimeMeasLap(&tm);

	TimeMeasStart(&tm);
	drawmapt = fmod(t, 1.);
#endif
	underwater = pos[1] < sqrt(EARTH_RAD * EARTH_RAD - pos[0] * pos[0] - pos[2] * pos[2]) - EARTH_RAD;
	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT);
	if(detail){
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glColor4ub(255,0,0,255);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_TEXTURE_2D);
	}
	else{
		int i;
		GLfloat mat_specular[] = {0., 0., 0., 1.}/*{ 1., 1., .1, 1.0 }*/;
		Vec4f mat_diffuse(.75, .75, .75, 1.0);
		Vec4f mat_ambient_color(1., 1., 1., 1.0);
		GLfloat mat_shininess[] = { 0.0 };
		GLfloat color[] = {1., 1., 1., .8}, amb[] = {1., 1., 1., 1.};

/*		glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);*/
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_EMISSION, mat_specular);
/*		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_color);*/
		glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
/*		glLightfv(GL_LIGHT0, GL_AMBIENT, mat_ambient_color);*/
/*		glLightfv(GL_LIGHT0, GL_DIFFUSE, mat_diffuse);*/

		if(!matlist){
			glNewList(matlist = glGenLists(2), GL_COMPILE);
			mat_diffuse = dif;
			mat_ambient_color = amb;
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);
			glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
			glEnable(GL_CULL_FACE);
			glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
			glEnable(GL_COLOR_MATERIAL);
			glEndList();

			glNewList(matlist + 1, GL_COMPILE);
			{
				GLfloat mat_diffuse[] = { .25, .5, .75, 1.0 };
				GLfloat mat_ambient_color[] = { .25, .5, .75, 1.0 };
				glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
				glEnable(GL_COLOR_MATERIAL);
	/*			glDisable(GL_COLOR_MATERIAL);*/
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient_color);
				glDisable(GL_CULL_FACE);
			}
			glEndList();

/*			glGenTextures(1, &tex0);
			glBindTexture(GL_TEXTURE_2D, tex0);*/
			if(glActiveTextureARB) for(i = 0; i < 2; i++){
				glActiveTextureARB(i == 0 ? GL_TEXTURE0_ARB : GL_TEXTURE1_ARB);
				glCallList(generate_ground_texture());
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
				glEnable(GL_TEXTURE_2D);
	/*			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, i ? GL_MODULATE : GL_MODULATE);*/
			}
			else{
				glCallList(generate_ground_texture());
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
			}
			tex0 = FindTexCache("grass.jpg")->tex[0];
		}
#if 0
		if(!init){
			suftexparam_t stp, stp2;
			init = 1;
			stp.flags = STP_ENV | STP_MAGFIL | STP_MIPMAP;
			stp.env = GL_MODULATE;
			stp.magfil = GL_LINEAR;
			stp.mipmap = 0;
			stp.alphamap = 0;
#if 1
			CallCacheBitmap("gtarget.bmp", "gtarget.bmp", &stp, "gtarget.bmp");
			tex = FindTexCache("gtarget.bmp")->tex[0];
#else
			stp.bmi = ReadBitmap("gtarget.bmp")/*lzuc(lzw_bricks, sizeof lzw_bricks, NULL)*/;
			stp.env = GL_MODULATE;
			stp.magfil = GL_LINEAR;
			stp.mipmap = 0;
			stp.alphamap = 0;
			stp2.bmi = stp.bmi;
			stp2.env = GL_MODULATE;
			stp2.magfil = GL_NEAREST;
			stp2.mipmap = 0;
			stp2.alphamap = 0;
			if(stp.bmi){
				tex = CacheSUFMTex("gtarget.bmp", &stp, &stp2);
				LocalFree(stp.bmi);
				tex = FindTexCache("gtarget.bmp")->tex[0];
			}
#endif
		}
#endif
/*		tex = FindTexCache("bricks.bmp")->tex[0];*/

		if(glActiveTextureARB) for(i = 0; i < 2; i++){
			glActiveTextureARB(i == 0 ? GL_TEXTURE0_ARB : GL_TEXTURE1_ARB);
			glBindTexture(GL_TEXTURE_2D, tex0);
			glCallList(matlist);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
			glEnable(GL_TEXTURE_2D);
		}
		else{
			glBindTexture(GL_TEXTURE_2D, tex0);
			glCallList(matlist);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
			glEnable(GL_TEXTURE_2D);
		}
	}
/*	printf("map tex: %lg\n", textime = TimeMeasLap(&tm));*/

	g_pglcull = glc;
	polys = 0;
	lasttex = 0;
	drawmap_invokes = 0;
	shadows = 0;

#if 0
	if(wmd && sy <= 2048){
		double cell = map_width / sy;
		warmapdecalelem_t *we;
#if MAPPROFILE
		TimeMeasStart(&tm);
#endif
		memset(wmdi, 0, sizeof wmdi);
		for(we = wmd->actv; we; we = we->next){
			int iy = (we->pos[1] - orgy) / cell, ix = (we->pos[0] - orgx) / cell;
			if(0 <= iy && iy < sy){
				if(0 == wmdi[ix][1] || ix < wmdi[iy][0])
					wmdi[iy][0] = ix;
				if(wmdi[iy][1] < ix + 1)
					wmdi[iy][1] = ix + 1;
			}
		}
#if MAPPROFILE
		printf("map decal: %lg\n", decaltime = TimeMeasLap(&tm));
#endif
	}
#endif

	{
	amat4_t oldmat;
	int texdep;
	glMatrixMode(GL_TEXTURE);
	glGetDoublev(GL_TEXTURE_MATRIX, oldmat);
	glGetIntegerv(GL_TEXTURE_STACK_DEPTH, &texdep);
/*	glPushMatrix();*/
//	glScaled(100., 100., 100.);
	glMatrixMode(GL_MODELVIEW);

#if MAPPROFILE
	TimeMeasStart(&tm);
#endif
	glCallList(matlist);
	drawmap_in(top, 0, 0, 0);
	glPopAttrib();

	glMatrixMode(GL_TEXTURE);
	glLoadMatrixd(oldmat);
/*	glPopMatrix();*/
	glMatrixMode(GL_MODELVIEW);
	}

#if MAPPROFILE
	printf("map check[%d]: %lg, draw[%d/%d/%d]: %lg\n", checkmap_invokes, checkmap_time, polys, drawmap_invokes, sx * sy, drawtime = TimeMeasLap(&tm)/* - checkmap_time*/);
	printf("map shadows: %d\n", shadows);
#endif

//	glDeleteLists(matlist, 2);
#else
	if(!init){
		init = 1;
		glNewList(list = glGenLists(1), GL_COMPILE);
		wm->vft->size(wm, &sx, &sy);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
		glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
		glBegin(GL_QUADS);
		for(i = 0; i < sx-1; i++) for(j = 0; j < sy-1; j++){
			wartile_t t, tr, td, trd, tl, tu;
			avec3_t dx, dz, nh;
			wm->vft->getat(wm, &t, i, j);
			wm->vft->getat(wm, &tr, i+1, j);
			wm->vft->getat(wm, &td, i, j+1);
			wm->vft->getat(wm, &trd, i+1, j+1);
			dx[0] = .01, dx[1] = tr.height - t.height, dx[2] = 0;
			dz[0] = 0., dz[1] = td.height - t.height, dz[2] = .01;
			VECVP(nh, dz, dx);
			VECNORMIN(nh);
	/*		glNormal3dv(nh);*/
			normalmap(wm, i, j, sx, sy, .01, NULL);
			glVertex3d(i * .01, t.height, j * .01);
			normalmap(wm, i, j+1, sx, sy, .01, NULL);
			glVertex3d(i * .01, td.height, (j+1) * .01);
			normalmap(wm, i+1, j+1, sx, sy, .01, NULL);
			glVertex3d((i+1) * .01, trd.height, (j+1) * .01);
			normalmap(wm, i+1, j, sx, sy, .01, NULL);
			glVertex3d((i+1) * .01, tr.height, j * .01);
			if(i * .01 < pl.pos[0] && pl.pos[0] < i * .01 + .01 && j * .01 < pl.pos[2] && pl.pos[2] < j * .01 + .01
				&& 0. < pl.pos[1] && pl.pos[1] - pl.rad < t.height)
			{
				pl.pos[1] = t.height + pl.rad;
				pl.velo[1] = 0.;
			}
		}
		glEnd();
/*		glDisable(GL_LIGHTING);
		glDisable(GL_LINE_SMOOTH);
		glDepthMask(0);
		for(i = 0; i < sx-1; i++) for(j = 0; j < sy-1; j++){
			double pos[3];
			wartile_t t;
			wm->vft->getat(wm, &t, i, j);
			normalmap(wm, i+1, j, sx, sy, .01, &pos);
			glColor4ub(255,0,0,255);
			glBegin(GL_LINES);
			glVertex3d(i * .01, t.height, j * .01);
			glVertex3d(pos[0] * .01 + i * .01, t.height + pos[1] * .01, pos[2] * .01 + j * .01);
			glEnd();
		}
		glDepthMask(1);
		glEnable(GL_LIGHTING);*/
		glEndList();
	}
	glCallList(list);
#endif

#ifndef NDEBUG
	running = 0;
#endif

	glBindTexture(GL_TEXTURE_2D, oldtex);

#if MAPPROFILE
	printf("map total: %lg ; %lg\n", TimeMeasLap(&tm2), checkmap_time /*+ textime*/ + decaltime + drawtime);
#endif
}


void DrawMap::checkmap(char *top, int x, int y, int level){
	int size = 1 << level;
	int ind = x + y * size;
	double cell = map_width / size;
	double dx, dy;
	checkmap_invokes++;
	dx = orgx + (x + .5) * cell - map_viewer[0];
	dy = orgy + (y + .5) * cell - map_viewer[2];
	if(dx * dx + dy * dy < cell * cell * detail_factor * detail_factor){
		if(level < checkmap_maxlevel){
			char *sub = &top[(size * size + 7) / 8];
			top[ind / 8] |= 1 << (ind % 8);
			checkmap(sub, x * 2, y * 2, level + 1);
			checkmap(sub, x * 2 + 1, y * 2, level + 1);
			checkmap(sub, x * 2, y * 2 + 1, level + 1);
			checkmap(sub, x * 2 + 1, y * 2 + 1, level + 1);
		}
		else
			top[ind / 8] &= ~(1 << (ind % 8));
	}
	else
		top[ind / 8] &= ~(1 << (ind % 8));
}

double DrawMap::drawmap_height(dmn *d, char *top, int sx, int sy, int x, int y, int level, int lastpos[2]){
	wartile_t wt;
	double ret;
	int size = 1 << level;
	int scale = 1 << (checkmap_maxlevel - level);
	int sscale = scale * 2;
	double mcell = map_width / (1 << checkmap_maxlevel);
	double cell = map_width / size;
	double tcell = cell * 5.;
	double t = 0/*wm ? 0 : drawmapt*/;
	d->inter = 0;
	d->sealevel = -1;
	if(wm == NULL){
		ret = 0;
/*		glNormal3dv(avec3_010);*/
	}
	else if(level == 0 || x == 0 || y == 0 || sx <= x-1 || sy <= y-1 || (x + y) % 2 == 0){
		wm->getat(&wt, x * scale, y * scale);
/*		normalmap(wm, x * scale, y * scale, sx, sy, mcell, d->n);*/
		ret = wt.height;
	}
	else{
		int ssize = 1 << (level - 1);
		char *sup = &top[-(ssize * ssize + 7) / 8];
		int sind, sind2;
		int xx, yy, xx2, yy2;
		xx = x / 2 + x % 2 - 1;
		yy = y / 2 + y % 2 - 1;
		xx2 = x / 2;
		yy2 = y / 2;
		sind = xx + yy * ssize;
		sind2 = xx2 + yy2 * ssize;
		if(!(xx < 0 || sx <= xx * sscale || yy < 0 || sy <= yy * sscale) &&
			!(xx2 < 0 || sx <= xx2 * sscale || yy2 < 0 || sy <= yy2 * sscale) &&
			(!(sup[sind / 8] & (1 << (sind % 8))) || !(sup[sind2 / 8] & (1 << (sind2 % 8)))))
		{
			double h0, h1, n0[3], n1[3];
			wm->getat(&wt, x / 2 * sscale, y / 2 * sscale);
			h0 = wt.height;
			wm->getat(&wt, (x + 1) / 2 * sscale, (y + 1) / 2 * sscale);
			h1 = wt.height;
			ret = h1;
/*			normalmap(wm, x / 2 * sscale, y / 2 * sscale, sx, sy, mcell, &n0);*/
/*			normalmap(wm, (x + 1) / 2 * sscale, (y + 1) / 2 * sscale, sx, sy, mcell, d->n);*/
/*			VECADDIN(n0, n1);
			VECSCALEIN(n0, 1. / 2.);
			glNormal3dv(n0);*/
			x = (x + 1) / 2 * 2;
			y = (y + 1) / 2 * 2;
		}
		else{
			wm->getat(&wt, x * scale, y * scale);
			ret = wt.height;
/*			normalmap(wm, x * scale, y * scale, sx, sy, mcell, d->n);*/
		}
	}
	if(x == lastpos[0] && y == lastpos[1]){
		*d = *last_dmn;
		d->valid = 0;
		d->skip = 1;
		return 0.;
	}

	d->ix = x * scale;
	d->iy = y * scale;

	if(glMultiTexCoord2fARB){
		/*glMultiTexCoord2fARB*/(GL_TEXTURE0_ARB, d->t[0][0] = t + x * tcell, d->t[0][1] = t + y * tcell);
		if(checkmap_maxlevel == level)
			/*glMultiTexCoord2fARB*/(GL_TEXTURE1_ARB, d->t[1][0] = (t + x * tcell) * 13.78, d->t[1][1] = (t + y * tcell) * 13.78);
		else
			/*glMultiTexCoord2fARB*/(GL_TEXTURE1_ARB, d->t[1][0] = 0, d->t[1][1] = 0);
	}
	else{
		d->t[0][0] = t + x * cell;
		d->t[0][1] = t + y * cell;
	}

	/*glVertex3d*/(d->x = orgx + x * cell, ret, d->z = orgy + y * cell);

	/* drawmapnode is always be used at least once, so caching ground falloff here
	  is expected to be effective */
#if 0
	{
		extern struct astrobj earth;
		double rad = earth.rad /* EARTH_RAD */;
		d->gnd = sqrt(rad * rad - d->x * d->x - d->z * d->z) - rad;
		d->sealevel = ret < d->gnd ? -1 : 1/*d->gnd < ret ? 1 : 0*/;
	}
#endif

	lastpos[0] = x;
	lastpos[1] = y;
	last_dmn = d;
	d->h = ret;
	d->valid = 1;
	d->skip = 0;
	return ret;
}

static int interm(dmn *d0, dmn *d1, dmn *d01){
	double t;
	double h;
	d01->skip = 0;
	if(d1->h == d0->h)
		return d01->valid = 0;
/*	if(d0->h < d0->gnd && d0->gnd <= d1->h)
		d0->sealevel = -1, d1->sealevel = 1;
	else if(d0->gnd <= d0->h && d1->h < d1->gnd)
		d0->sealevel = 1, d1->sealevel = -1;*/
	if(d0->sealevel * d1->sealevel < 0);
	else return d01->valid = 0;
	h = d0->h - d0->gnd;
	t = -h / ((d1->h - d1->gnd) - h);
/*	if(t < 0. || 1. <= t)
		return d01->valid = 0;*/
	*d01 = *d0;
	d01->x += t * (d1->x - d0->x);
	d01->z += t * (d1->z - d0->z);
	d01->t[0][0] += t * (d1->t[0][0] - d0->t[0][0]);
	d01->t[0][1] += t * (d1->t[0][1] - d0->t[0][1]);
	d01->t[1][0] += t * (d1->t[1][0] - d0->t[1][0]);
	d01->t[1][1] += t * (d1->t[1][1] - d0->t[1][1]);
/*	d01->n[0] += t * (d1->n[0] - d0->n[0]);
	d01->n[1] += t * (d1->n[1] - d0->n[1]);
	d01->n[2] += t * (d1->n[2] - d0->n[2]);*/
/*	d01->h = grd;*/
	d01->gnd = d01->h = t * d0->gnd + (1. - t) * d1->gnd;
	d01->inter = 1;
	d01->sealevel = 0;
	return d01->valid = 2;
}

double perlin_noise_pixel(int x, int y, int bit);

void DrawMap::drawmap_node(const dmn *d){
	double t;
	Vec3d normal;
	double grd; /* ground level */
/*	if(d->skip)
		return;*/
	grd = d->gnd;
	if(!d->valid || (watersurface ? 0 < d->sealevel/*grd + drawmap_cellsize * EPSILON < d->h*/ : d->valid == 1 && (underwater ? grd <= d->h : d->h < grd)))
		return;
	if(last_dmn == d)
		return;
	last_dmn = d;
	t = watersurface ? drawmapt : 0;
	if(glMultiTexCoord2fARB){
		glMultiTexCoord2fARB(GL_TEXTURE0_ARB, d->t[0][0] + t, d->t[0][1] + t);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB, d->t[1][0] + t * 13.78, d->t[1][1] + t * 13.78);
	}
    else
		glTexCoord2d(d->t[0][0] + t, d->t[0][1] + t);
	if(watersurface)
		glNormal3d(0, 1, 0);
	else if(d->valid == 1)
		normalmap(wm, d->ix, d->iy, gsx, gsy, map_width / gsx, &normal);
	else
		normalmap(wm, (d->x - orgx) / g_cell, (d->z - orgy) / g_cell, gsx, gsy, g_cell, &normal);
/*	glNormal3dv(d->n);*/
	if(flatsurface){
		glColor3f(.5, .5, .5);
	}
	else if(!watersurface){
		double h = d->h - grd;
		double r;
		struct random_sequence rs;
		initfull_rseq(&rs, d->ix, d->iy);
		r = (drseq(&rs) - .5) / 1.5;
/*		r = perlin_noise_pixel(d->ix, d->iy, 2) * 2;*/
		if(0. <= h){
			double browness = .3 / ((h) / .01 + 1.) * .25 + .75;
			double whiteness = /*weather == wsnow ? 1. :*/ h < 1. ? 0. : 1. - 1. / (h * .3 + .7);
			browness = rangein(MAX(browness, 1. - pow(normal[1], 8.) + r), 0., 1.);
/*			if(weather == wsnow)
				glColor3ub(255 * whiteness, 255 * whiteness, 255 * whiteness);
			else*/ if(whiteness)
				glColor3ub((1. - whiteness) * 191 * browness + 255 * whiteness, (1. - whiteness) * (255 - 191 * browness) + 255 * whiteness, (1. - whiteness) * 31 + 255 * whiteness);
			else{
				double sandiness = pow(normal[1], 8.) * (0. <= h && h < .010 ? (.010 - h) / .010 : 0.);
				extern double foilage_level(int x, int y);
				double foilage;
				foilage = 0/*foilage_level(d->ix - gsx / 2, d->iy - gsy / 2)*/;
				glColor3ub(
					(1. - foilage) * (230 * sandiness + (1. - sandiness) * 127 * browness),
					(1. - foilage / 2.) * (230 * sandiness + (1. - sandiness) * (255 - 127 * browness)),
					(1. - foilage) * (120 * sandiness + (1. - sandiness) * 31));
			}
		}
		else{
			double browness = 1. / (-(h) * 2.718281828459 / .05 + 1.);
			browness = rangein(browness + r, 0, 1);
			glColor3ub(127 * browness, 127 * browness, 127 * (browness - .5) * (browness - .5));
		}
	}
	else{
		double h = -(d->h - grd), f = 1. / (h / .01 + 1.);
		glColor3f(.75 * f + .25, .5 * f + .5, .25 * f + .75);
	}
	glVertex3d(d->x, watersurface && !d->inter ? grd : d->h, d->z);
}

#define glCallList
void DrawMap::drawmap_face(dmn *d){
	double h;
	int i, first = 1;
	watersurface = 0;
	last_dmn = NULL;
#if 1
	for(i = 0; i < 4; i++) if(!d[i].skip){
		if(first){
			h = d[i].h;
			first = 0;
		}
		else if(d[i].h != h)
			break;
	}
	glCallList(matlist);
	if(0 && i == 4 /*(d[0].skip || 0. == d[0].h) && (d[1].skip || 0. == d[1].h) && (d[2].skip || 0. == d[2].h) && (d[3].skip || 0. == d[3].h)*/){
		if(lasttex != tex){
/*					glCallList(tex);*/
			glBindTexture(GL_TEXTURE_2D, tex);
/*			if(glActiveTextureARB){
				glActiveTextureARB(GL_TEXTURE1_ARB);
				glDisable(GL_TEXTURE_2D);
				glActiveTextureARB(GL_TEXTURE0_ARB);
			}*/
			lasttex = tex;
		}
		flatsurface = 1;
		for(i = 0; i < 4; i++){
			d[i].t[0][0] *= .1;
			d[i].t[0][1] *= .1;
			d[i].t[1][0] *= .1;
			d[i].t[1][1] *= .1;
		}
/*				glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glEnable(GL_TEXTURE);*/
	}
	else if(lasttex != tex0){
/*				timemeas_t tm;
		TimeMeasStart(&tm);*/
/*				glCallList(tex0);*/
		glBindTexture(GL_TEXTURE_2D, tex0);
/*				printf("tex %lg\n", TimeMeasLap(&tm));*/
/*		if(glActiveTextureARB){
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}*/
		lasttex = tex0;
		flatsurface = 0;
/*				glBindTexture(GL_TEXTURE_2D, tex0);*/
	}
#endif
			glBegin(GL_POLYGON);
			drawmap_node(&d[0]);
			drawmap_node(&d[1]);
			drawmap_node(&d[2]);
			drawmap_node(&d[3]);
			glEnd();
			if(d[0].sealevel < 0/*(d[0].skip || d[0].h < 0.) && (d[1].skip || d[1].h < 0.) && (d[2].skip || d[2].h < 0.) && (d[3].skip || d[3].h < 0.)*/){
				watersurface = 1;
				last_dmn = NULL;
				glCallList(matlist + 1);
				glBegin(GL_POLYGON);
				drawmap_node(&d[0]);
				drawmap_node(&d[1]);
				drawmap_node(&d[2]);
				drawmap_node(&d[3]);
				glEnd();
			}
}

void DrawMap::drawmap_in(char *top, int x, int y, int level){
	int size = 1 << level;
	double cell = map_width / size;
	dmn d[4];
	wartile_t wt;
	int scale = 1 << (checkmap_maxlevel - level);
	int ind = x + y * size;

	drawmap_invokes++;

	{
		double cellmax = cell * 1.41421356;
		double sp;
		wm->getat(&wt, x * scale, y * scale);
		Vec3d pos(orgx + (x + .5) * cell,
			wt.height,
			orgy + (y + .5) * cell);
#if 1
		Vec3d delta = pos - g_pglcull->getViewpoint();
		sp = g_pglcull->getViewdir().sp(delta);
		if(g_pglcull->getFar() + cellmax < sp)
			return;
		if(sp < g_pglcull->getNear() - cellmax)
			return;
#else
		if(glcullFar(&pos, cellmax, g_pglcull) || glcullNear(&pos, cellmax, g_pglcull))
			return;
#endif
	}

	if(top[ind / 8] & (1 << (ind % 8)) && level < checkmap_maxlevel){
		char *sub = &top[(size * size + 7) / 8];
		drawmap_in(sub, x * 2, y * 2, level + 1);
		drawmap_in(sub, x * 2 + 1, y * 2, level + 1);
		drawmap_in(sub, x * 2, y * 2 + 1, level + 1);
		drawmap_in(sub, x * 2 + 1, y * 2 + 1, level + 1);
	}
	else{
		int scale = 1 << (checkmap_maxlevel - level);
		int sx, sy;
		double mcell = map_width / (1 << checkmap_maxlevel);
		double h, minh = 0.;
		int pos[2] = {-1};
		struct warmap_alt *alt = NULL;

		polys++;

		wm->size(&sx, &sy);

		g_scale = 1 << (checkmap_maxlevel - level);
		g_cell = cell;

/*		if(sx <= (x+1) * scale || sy <= (y+1) * scale)
			return;*/

		/*
		if(checkmap_maxlevel == level)
			glEnable(GL_TEXTURE_2D);
		else
			glDisable(GL_TEXTURE_2D);*/

/*		glCallList(matlist);*/
/*		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_color);
		glEnable(GL_CULL_FACE);*/
		last_dmn = NULL;

/*		wm->vft->getat(wm, &wt, x * scale, y * scale);*/
	/*	glVertex3d*/(x * cell, h = drawmap_height(&d[0], top, sx, sy, x, y, level, pos), y * cell);
		if(h < minh)
			minh = h;

/*		normalmap(wm, x * scale, (y+1) * scale, sx, sy, mcell, NULL);
		wm->vft->getat(wm, &wt, x * scale, (y+1) * scale);*/
/*		glTexCoord2d(x * cell, (y+1) * cell);*/
	/*	glVertex3d*/(x * cell, h = drawmap_height(&d[1], top, sx, sy, x, y+1, level, pos), (y+1) * cell);
		if(h < minh)
			minh = h;

/*		normalmap(wm, (x+1) * scale, (y+1) * scale, sx, sy, mcell, NULL);
		wm->vft->getat(wm, &wt, (x+1) * scale, (y+1) * scale);*/
/*		glTexCoord2d((x+1) * cell, (y+1) * cell);*/
	/*	glVertex3d*/((x+1) * cell, h = drawmap_height(&d[2], top, sx, sy, x+1, y+1, level, pos), (y+1) * cell);
		if(h < minh)
			minh = h;

/*		normalmap(wm, (x+1) * scale, y * scale, sx, sy, mcell, NULL);
		wm->vft->getat(wm, &wt, (x+1) * scale, y * scale);*/
/*		glTexCoord2d((x+1) * cell, y * cell);*/
	/*	glVertex3d*/((x+1) * cell, h = drawmap_height(&d[3], top, sx, sy, x+1, y, level, pos), y * cell);
		if(h < minh)
			minh = h;

		/* alternate terrains needs not to be planar, so culling is not done here. */
#if 0
		{
			int i;
			for(i = 0; i < wm->nalt; i++) if(wm->alt[i].x0 <= x && x <= wm->alt[i].x1 && wm->alt[i].y0 <= y && y <= wm->alt[i].y1){
				alt = &wm->alt[i];
				break;
			}
		}
#endif

		{
			int i;
			double pos[3] = {0.};
			double mcell = cell;

			/* calculate center of gravity first */
			for(i = 0; i < 4; i++){
				pos[0] += d[i].x / 4.;
				if(underwater ? d[i].sealevel < 0 : 0 <= d[i].sealevel)
					pos[1] += d[i].h / 4.;
				else
					pos[1] += d[i].gnd / 4.;
				pos[2] += d[i].z / 4.;
			}

			/* determine real cell size used for culling */
			for(i = 0; i < 4; i++){
				double h;
				if(mcell < ABS(d[i].x - pos[0]))
					mcell = ABS(d[i].x - pos[0]);
				if(mcell < ABS(d[i].z - pos[2]))
					mcell = ABS(d[i].z - pos[2]);
				h = underwater && d[i].sealevel < 0 || !underwater && 0 <= d[i].sealevel ? d[i].h : d[i].gnd;
				if(mcell < ABS(h - pos[1]))
					mcell = ABS(h - pos[1]);
				if(underwater && d[i].h < d[i].gnd && mcell < ABS(d[i].gnd - pos[1]) * 2.)
					mcell = ABS(d[i].gnd - pos[1]) * 2.;
			}
			if(!underwater && d[0].sealevel < 0)
				mcell *= 2.;

//			if(glcullFrustum(pos, mcell, g_pglcull))
//				return;
		}

		{
		int sign = 0;
		int i, j;
		int first = 1, flat = 0;
		double h;
		dmn d01, d12, d23, d30, d02, d13;
		sign |= interm(&d[0], &d[1], &d01);
		sign |= interm(&d[0], &d[2], &d02)/*/interm(&d[1], &d[3], &d13)*/;
		sign |= interm(&d[1], &d[2], &d12);
		sign |= interm(&d[2], &d[3], &d23);
		sign |= interm(&d[3], &d[0], &d30);

		for(i = 0; i < 4; i++) if(!d[i].skip){
			if(first){
				h = d[i].h;
				first = 0;
			}
			else if(d[i].h != h){
				flat = 1;
				break;
			}
		}

		/* if all nodes have the same sign, excluding skipped one. */
		if(!sign)
		/*if(j == 4)*/
		/*if((d[0].skip || 0. <= d[0].h) && (d[1].skip || 0. <= d[1].h) && (d[2].skip || 0. <= d[2].h) && (d[3].skip || 0. <= d[3].h))*/
		/*if((d[1].skip || 0. < d[1].h * sign) && (d[2].skip || 0. < d[2].h * sign) && (d[3].skip || 0. < d[3].h * sign))*/
		{
			extern int r_drawshadow;
			extern int r_numshadows;
			extern double sun_yaw;
			extern double sun_pitch;
			int decal = 0, n;
//			warmapdecalelem_t *we;
/*			if(gwmd && pwmdi[y][1] && pwmdi[y][0] <= x && x < pwmdi[y][1]) for(we = gwmd->actv; we; we = we->next) if(d[0].x < we->pos[0] && we->pos[0] < d[2].x && d[0].z < we->pos[1] && we->pos[1] < d[1].z){
				if(!decal){
					decal = 1;
					glDepthMask(0);
					if(alt){
						glPushMatrix();
						glTranslated(d[0].x, d[0].h, d[0].z);
						glScaled(cell, 1, cell);
						alt->altfunc(wm, x, y, alt->hint);
						glPopMatrix();
					}
					else
						drawmap_face(d);
					glDepthMask(1);
				}
				gwmd->drawproc(we, gwm, x * scale, y * scale, NULL, gwmdg);
			}*/

#if 0
			/* projected texture shadow */
			if(r_drawshadow && 0. < sun_pitch){
				amat4_t mat;
				static const amat4_t rotaxis = {
					1,0,0,0,
					0,0,-1,0,
					0,1,0,0,
					0,0,0,1,
				};
				for(n = 0; n < r_numshadows; n++) if(shadowscale[n] && x % 1 == 0 && y % 1 == 0 && scale == 1){
					double pyr[3];
/*					amat4_t mat, imat;
					amat3_t mat3, imat3;*/
					pyr[0] = sun_pitch;
					pyr[1] = sun_yaw;
					pyr[2] = 0.;
/*					quat2imat(mat, sun_light);*/
					pyrimat(pyr, &mat);
					break;
				}
				for(n = 0; n < r_numshadows; n++) if(shadowscale[n] && x % 1 == 0 && y % 1 == 0 && scale == 1){
	/*				extern GLuint shadowtex;
					extern avec3_t shadoworg;
					extern double shadowscale;*/
					extern aquat_t sun_light;
					static GLuint pre = 0;
					avec4_t genfunc[3] = {
						{1.,0,0},
						{0,0,1.},
						{0,1.,0},
	/*					{.5/shadowscale,0,0},
						{0,0,.5/shadowscale},
						{0,.5/shadowscale,0},*/
					};
					avec3_t vertices[4];
					double aabb[2][2];
					amat4_t oldmat, mat1;
					GLuint oldtex;
					int i, j;

	/*				genfunc[0][0] = genfunc[0][1] = .5/shadowscale * sqrt(2.) / 2.;*/

					if(!pre){
						glNewList(pre = glGenLists(1), GL_COMPILE);
						(0 ? glDisable : glEnable)(GL_TEXTURE_2D);
						glDisable(GL_LIGHTING);
						glEnable(GL_BLEND);
						glDisable(GL_CULL_FACE);
						glDepthMask(GL_FALSE);
						glColor4ub(0,0,0,127);
						glAlphaFunc(GL_GREATER, .0);
						glEnable(GL_ALPHA_TEST);
						glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

						glDisable(GL_TEXTURE_GEN_S);
						glDisable(GL_TEXTURE_GEN_T);
						glDisable(GL_TEXTURE_GEN_R);
						glEndList();
					}

					if(!decal){
						decal = 1;
						glDepthMask(0);
						if(alt){
							glPushMatrix();
							glTranslated(d[0].x, d[0].h, d[0].z);
							glScaled(cell, 1, cell);
							alt->altfunc(wm, x, y, alt->hint);
							glPopMatrix();
						}
						else
							drawmap_face(d);
						glDepthMask(1);
					}
					shadows++;

					glPushAttrib(GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
					if(glActiveTextureARB){
						glActiveTextureARB(GL_TEXTURE1_ARB);
						glDisable(GL_TEXTURE_2D);
						glActiveTextureARB(GL_TEXTURE0_ARB);
						glEnable(GL_TEXTURE_2D);
					}
					glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldtex);
					glBindTexture(GL_TEXTURE_2D, shadowtex[n]);

	#undef glCallList
					glCallList(pre);
	#define glCallList

					glMatrixMode(GL_TEXTURE);
/*					glGetIntegerv(GL_TEXTURE_STACK_DEPTH, &i);*/
					glGetDoublev(GL_TEXTURE_MATRIX, &oldmat);
	#if 1
	/*				glPushMatrix();*/
	/*				glRotated(90, 1, 0, 0);*/
					glLoadIdentity();
					glTranslated(.5, .5, 0.);
/*					glScaled(.5 / shadowscale[n], .5 / shadowscale[n], .5 / shadowscale[n]);*/
/*					glTranslated(shadoworg[n][0], shadoworg[n][2], shadoworg[n][1]);*/

					glMultMatrixd(mat);
/*					glMultMatrixd(rotaxis);*/

	/*				glTranslated(-.5 / shadowscale[n] * shadoworg[n][0] + .5, -.5 / shadowscale[n] * shadoworg[n][2] + .5, -.5 / shadowscale[n] * shadoworg[n][1]);*/
					glScaled(.5 / shadowscale[n], .5 / shadowscale[n], .5 / shadowscale[n]);
					glTranslated(-shadoworg[n][0], -shadoworg[n][1], -shadoworg[n][2]);
					glGetDoublev(GL_TEXTURE_MATRIX, &mat1);
					glLoadIdentity();
	#endif
					for(i = 0; i < 4; i++){
						avec3_t texc0;
						texc0[0] = d[i].x, texc0[1] = d[i].sealevel < 0 ? d[i].gnd : d[i].h, texc0[2] = d[i].z;
						mat4vp3(vertices[i], mat1, texc0);
					}
					aabb[0][0] = aabb[1][0] = 1.;
					aabb[0][1] = aabb[1][1] = 0.;
					for(i = 0; i < 4; i++) for(j = 0; j < 2; j++){
						if(vertices[i][j] < aabb[j][0])
							aabb[j][0] = vertices[i][j];
						if(aabb[0][1] < vertices[i][j])
							aabb[j][1] = vertices[i][j];
					}
/*					for(i = 0; i < 4; i++) if(0. < vertices[i][0] && vertices[i][0] < 1. && 0. < vertices[i][1] && vertices[i][1] < 1.)
						break;*/
					if(1. <= aabb[0][0] || aabb[0][1] <= 0. || 1. <= aabb[1][0] || aabb[1][1] <= 0. /*i == 4 && (0. < (vertices[0][0] + 2.) * (vertices[1][0] - 2.)/* || 0. < (vertices[0][1]) * (vertices[1][1] - 2.))*/){
						glMatrixMode(GL_MODELVIEW);
						glBindTexture(GL_TEXTURE_2D, oldtex);
						glPopAttrib();
						continue;
					}
	/*
					for(i = 0; i < 3; i++)
						VECSCALEIN(genfunc[i], .5 / shadowscale[n]);
					glTexGendv(GL_S, GL_OBJECT_PLANE, genfunc[0]);
					glTexGendv(GL_T, GL_OBJECT_PLANE, genfunc[1]);
					glTexGendv(GL_R, GL_OBJECT_PLANE, genfunc[2]);

						glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
						glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
						glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);*/

	/*				glTexGendv(GL_S, GL_EYE_PLANE, genfunc[0]);
					glTexGendv(GL_T, GL_EYE_PLANE, genfunc[1]);
					glTexGendv(GL_R, GL_EYE_PLANE, genfunc[2]);

						glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
						glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
						glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);*/

	/*					glEnable(GL_TEXTURE_GEN_S);
						glEnable(GL_TEXTURE_GEN_T);
						glEnable(GL_TEXTURE_GEN_R);*/

	/*				glTranslated(-.5 / shadowscale * shadoworg[0] + .5, -.5 / shadowscale * shadoworg[2] + .5, -.5 / shadowscale * shadoworg[1]);
					glScaled(.5 / shadowscale, .5 / shadowscale, .5 / shadowscale);*/

	#if 1
					glBegin(GL_POLYGON);
	#define texfunc(i) {\
/*						avec3_t texc, texc0;\
						texc0[0] = d[i].x, texc0[1] = d[i].h, texc0[2] = d[i].z;*/\
	/*					VECSCALEIN(texc0, .5 / shadowscale[n]);*/\
/*						mat4vp3(texc, mat, texc0);*/\
						glTexCoord2dv(vertices[i]);\
	/*					printf("t%d %lg %lg %lg\n", i, texc[0], texc[1], texc[2]);*/\
					}
					texfunc(0); glVertex3d(d[0].x, d[0].sealevel < 0 ? d[0].gnd : d[0].h, d[0].z);
					texfunc(1); glVertex3d(d[1].x, d[1].sealevel < 0 ? d[1].gnd : d[1].h, d[1].z);
					texfunc(2); glVertex3d(d[2].x, d[2].sealevel < 0 ? d[2].gnd : d[2].h, d[2].z);
					texfunc(3); glVertex3d(d[3].x, d[3].sealevel < 0 ? d[3].gnd : d[3].h, d[3].z);
	#define glTexCoord2d
	/*				glTexCoord2d(0.,0.); glVertex3d(d[0].x,d[0].h,d[0].z);
					glTexCoord2d(1.,0.); glVertex3d(d[1].x,d[1].h,d[1].z);
					glTexCoord2d(1.,1.); glVertex3d(d[2].x,d[2].h,d[2].z);
					glTexCoord2d(0.,1.); glVertex3d(d[3].x,d[3].h,d[3].z);*/
	#undef glTexCoord2d
					glEnd();
	#endif

	/*				glPopMatrix();*/
	/*				glLoadMatrixd(oldmat);*/
					glMatrixMode(GL_MODELVIEW);

					glBindTexture(GL_TEXTURE_2D, oldtex);
					glPopAttrib();
				}
			}
#endif

/*			if(decal){
				glColorMask(0, 0, 0, 0);
			}
			if(alt){
				glPushMatrix();
				glTranslated(d[0].x, d[0].h, d[0].z);
				glScaled(cell, 1, cell);
				alt->altfunc(wm, x, y, alt->hint);
				glPopMatrix();
			}
			else*/
				drawmap_face(d);
/*			if(decal){
				extern int nightvision;
				if(nightvision)
					glColorMask(0, 1, 0, 1);
				else
					glColorMask(1, 1, 1, 1);
			}*/
		}
		else/* if(alt){
			glPushMatrix();
			glTranslated(d[0].x, d[0].h, d[0].z);
			glScaled(cell, 1, cell);
			alt->altfunc(wm, x, y, alt->hint);
			glPopMatrix();
		}
		else*/{
			/*if(0. <= d[0].h || 0. <= d[1].h || 0. <= d[2].h)*/{
				watersurface = 0;
				last_dmn = NULL;
				glCallList(matlist);
/*				if(flat && lasttex != tex0){
					glBindTexture(GL_TEXTURE_2D, tex0);
					lasttex = tex0;
				}
				else{
					glBindTexture(GL_TEXTURE_2D, tex);
					lasttex = tex;
				}*/
				glBegin(GL_POLYGON);
#if 1
				drawmap_node(&d[0]);
				drawmap_node(&d01);
				drawmap_node(&d[1]);
				drawmap_node(&d12);
				drawmap_node(&d[2]);
				drawmap_node(&d02);
#else
				drawmap_node(&d[1]);
				drawmap_node(&d12);
				drawmap_node(&d[2]);
				drawmap_node(&d23);
				drawmap_node(&d[3]);
				drawmap_node(&d13);
#endif
				glEnd();
			/*	if(!d[0].skip && d[0].h <= 0.)*/{
				watersurface = 1;
				last_dmn = NULL;
				glCallList(matlist + 1);
				glBegin(GL_POLYGON);
#if 1
				drawmap_node(&d[0]);
				drawmap_node(&d01);
				drawmap_node(&d[1]);
				drawmap_node(&d12);
				drawmap_node(&d[2]);
				drawmap_node(&d02);
#else
				drawmap_node(&d[1]);
				drawmap_node(&d12);
				drawmap_node(&d[2]);
				drawmap_node(&d23);
				drawmap_node(&d[3]);
				drawmap_node(&d13);
#endif
				glEnd();
				}
			}
			/*if(0. <= d[2].h || 0. <= d[3].h || 0. <= d[0].h)*/{
				watersurface = 0;
				last_dmn = NULL;
				glCallList(matlist);
				glBegin(GL_POLYGON);
#if 1
				drawmap_node(&d[d[2].skip ? 1 : 2]);
				drawmap_node(&d23);
				drawmap_node(&d[3]);
				drawmap_node(&d30);
				drawmap_node(&d[0]);
				drawmap_node(&d02);
#else
				drawmap_node(&d[3]);
				drawmap_node(&d30);
				drawmap_node(&d[0]);
				drawmap_node(&d01);
				drawmap_node(&d[1]);
				drawmap_node(&d13);
#endif
				glEnd();
				watersurface = 1;
				last_dmn = NULL;
				glCallList(matlist + 1);
				glBegin(GL_POLYGON);
#if 1
/*				if(d[2].h <= 0.)*/
					drawmap_node(&d[d[2].skip ? 1 : 2]);
/*				if(d23.valid)*/
					drawmap_node(&d23);
/*				if(d[3].h <= 0.)*/
					drawmap_node(&d[3]);
/*				if(d30.valid)*/
					drawmap_node(&d30);
/*				if(d[0].h <= 0.)*/
					drawmap_node(&d[0]);
/*				if(d02.valid)*/
					drawmap_node(&d02);
#else
				drawmap_node(&d[3]);
				drawmap_node(&d30);
				drawmap_node(&d[0]);
				drawmap_node(&d01);
				drawmap_node(&d[1]);
				drawmap_node(&d13);
#endif
				glEnd();
			}
		}
		}

#if 0
		if(minh < 0.){
			glCallList(matlist + 1);
/*			GLfloat mat_diffuse[] = { .15, .15, .75, 1.0 };
			GLfloat mat_ambient_color[] = { .5, .5, 1., 1.0 };
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
			glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_color);
			glDisable(GL_CULL_FACE);*/
			glBegin(GL_POLYGON);
			drawmap_height(&d[0], top, NULL, sx, sy, x, y, level, pos);
			drawmap_height(&d[1], top, NULL, sx, sy, x, y+1, level, pos);
			drawmap_height(&d[2], top, NULL, sx, sy, x+1, y+1, level, pos);
			drawmap_height(&d[3], top, NULL, sx, sy, x+1, y, level, pos);
			drawmap_node(&d[0]);
			drawmap_node(&d[1]);
			drawmap_node(&d[2]);
			drawmap_node(&d[3]);
			glEnd();
		}
#endif
	}
}
#undef glCallList

int mapcheck(Player *p, WarMap *wm){
	int sx, sy;
	int size;
	double map_width;
	double orgx, orgy;
	double cell;
	double abcd;
	Vec3d n;
	if(!wm)
		return 0;
	wm->size(&sx, &sy);
	size = MAX(sx, sy);
	map_width = wm->width();
	orgx = -map_width / 2.;
	orgy = -map_width / 2.;
	cell = map_width / size;
	Vec3d plpos = p->getpos();
/*	if(plpos[0] < orgx || orgx + (sx-1) * cell < plpos[0] || plpos[2] < orgy || orgy + (sy-1) * cell < plpos[2])
		return p->floortouch |= 0;*/
	abcd = wm->height(plpos[0], plpos[2], &n);
	if(plpos[1] < abcd + p->rad){
		Vec3d norm;
		norm[0] = n[0];
		norm[1] = n[2];
		norm[2] = n[1];
		norm.normin();
		double f = -p->getvelo().sp(norm) * 1.2; /* reflectance ?? */
		if(0. < f)
			p->setvelo(p->getvelo() + norm * f);
		p->setpos(Vec3d(plpos[0], abcd + p->rad, plpos[2]));
//		p->floortouch |= 3;

		/* auto-standup */
		double g_dt = .01;
		Quatd plrot = p->getrot();
		Vec3d xh = plrot.trans(vec3_100);
		Vec3d zh = plrot.trans(vec3_001);
		double sp = -vec3_010.sp(xh);
		Vec3d omega = zh * sp * g_dt;
		p->setrot(plrot.quatrotquat(omega));
	}
	return /*p->floortouch |= p->pos[1] < abcd + p->rad + FEETD ? 1 :*/ 0;
}

#if 0
void altterrain_gravel(WarMap *wm, int x, int y, void *hint){
	double dx = (x + 1) % 2 * .5, dy = y % 2 * .5;
	glPushAttrib(GL_TEXTURE_BIT);
	glBindTexture(GL_TEXTURE_2D, tex);
	glColor4ub(255,255,255,255);
	
	glBegin(GL_QUADS);
	glNormal3dv(avec3_010);
	if(glMultiTexCoord2fARB) glMultiTexCoord2fARB(GL_TEXTURE1, dx, dy); glTexCoord2d(dx, dy); glVertex3d(0,0,0);
	if(glMultiTexCoord2fARB) glMultiTexCoord2fARB(GL_TEXTURE1, dx, dy+.5); glTexCoord2d(dx, dy+.5); glVertex3d(0,0,1);
	if(glMultiTexCoord2fARB) glMultiTexCoord2fARB(GL_TEXTURE1, dx+.5, dy+.5); glTexCoord2d(dx + .5, dy+.5); glVertex3d(1,0,1);
	if(glMultiTexCoord2fARB) glMultiTexCoord2fARB(GL_TEXTURE1, dx+.5, dy); glTexCoord2d(dx + .5, dy); glVertex3d(1,0,0);
	glEnd();
	glPopAttrib();
}

void altterrain_runway(WarMap *wm, int x, int y, void *hint){
	static GLuint tex = 0;
	double dx = (x + 1) % 2 * .5, dy = y % 4 * .25;
	glPushAttrib(GL_TEXTURE_BIT);
	if(!tex){
		suftexparam_t stp, stp2;
		glGenTextures(1, &tex);
		stp.flags = STP_ENV | STP_MAGFIL;
		stp.bmi = ReadBitmap("grunway.bmp")/*lzuc(lzw_bricks, sizeof lzw_bricks, NULL)*/;
		stp.env = GL_MODULATE;
		stp.magfil = GL_LINEAR;
		stp.mipmap = 0;
		stp.alphamap = 0;
		stp2.flags = STP_ENV | STP_MAGFIL;
		stp2.bmi = stp.bmi;
		stp2.env = GL_MODULATE;
		stp2.magfil = GL_NEAREST;
		stp2.mipmap = 0;
		if(stp.bmi){
			tex = CacheSUFMTex("grunway.bmp", &stp, NULL);
			LocalFree((void*)stp.bmi);
/*			tex = FindTexCache("grunway.bmp")->tex[0];*/
		}
	}
/*	glBindTexture(GL_TEXTURE_2D, tex);*/
	glCallList(tex);
	glColor4ub(255,255,255,255);
	
	glBegin(GL_QUADS);
	glNormal3dv(avec3_010);
	if(glMultiTexCoord2fARB) glMultiTexCoord2fARB(GL_TEXTURE1, dx, dy); glTexCoord2d(dx, dy); glVertex3d(0,0,0);
	if(glMultiTexCoord2fARB) glMultiTexCoord2fARB(GL_TEXTURE1, dx, dy+.25); glTexCoord2d(dx, dy+.25); glVertex3d(0,0,1);
	if(glMultiTexCoord2fARB) glMultiTexCoord2fARB(GL_TEXTURE1, dx+.5, dy+.25); glTexCoord2d(dx + .5, dy+.25); glVertex3d(1,0,1);
	if(glMultiTexCoord2fARB) glMultiTexCoord2fARB(GL_TEXTURE1, dx+.5, dy); glTexCoord2d(dx + .5, dy); glVertex3d(1,0,0);
	glEnd();
	glPopAttrib();
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


double foilage_level(int x, int y){
	int intcell = 8;
	int bits = 8;
	double f[2][2], g;
	x += 2048;
	y += 2048;
	f[0][0] = perlin_noise_pixel(x / intcell, y / intcell, bits);
	f[0][1] = perlin_noise_pixel(x / intcell, (y) / intcell+1, bits);
	f[1][0] = perlin_noise_pixel((x) / intcell+1, y / intcell, bits);
	f[1][1] = perlin_noise_pixel((x) / intcell+1, (y) / intcell+1, bits);
	g = ((intcell - y % intcell) * ((intcell - x % intcell) * f[0][0] + x % intcell * f[1][0]) / intcell
		+ y % intcell * ((intcell - x % intcell) * f[0][1] + x % intcell * f[1][1]) / intcell) / intcell;
	return rangein(g * 4. - 2., 0., 1.);
}

int r_maprot;
char map_srcfilename[256];

int DrawMiniMap(WarMap *wm, const avec3_t pos, double scale, double angle){
	static GLuint tex = 0;
	double width = 1.;
	char outfilename[256] = "cache/map.bmp";
	if(wm)
		width = wm->width();
	if(!tex && wm){
		char binfilename[MAX_PATH];
#ifdef _WIN32
		BITMAPINFO *bmi;
//		BITMAPDATA bmd;
		WIN32_FILE_ATTRIBUTE_DATA fd, fd2, fd3;
		BOOL b;
		if(GetFileAttributes("cache") == -1)
			CreateDirectory("cache", NULL);
#else
		mkdir("cache");
#endif
		GetModuleFileName(NULL, binfilename, sizeof binfilename);
		b = GetFileAttributesEx(outfilename, GetFileExInfoStandard, &fd);
		GetFileAttributesEx(map_srcfilename, GetFileExInfoStandard, &fd2);
		GetFileAttributesEx(binfilename, GetFileExInfoStandard, &fd3);
		if(b && 0 < CompareFileTime(&fd.ftLastWriteTime, &fd2.ftLastWriteTime)/* && 0 < CompareFileTime(&fd.ftLastWriteTime, &fd3.ftLastWriteTime)*/ && (bmi = ReadBitmap(outfilename))){
			suftexparam_t stp;
			const struct suftexcache *stc;
			bmi->bmiHeader.biHeight = ABS(bmi->bmiHeader.biHeight);
			stp.flags = STP_ENV | STP_MAGFIL;
			stp.bmi = bmi;
			stp.alphamap = 0;
			stp.env = GL_MODULATE;
			stp.magfil = GL_LINEAR;
			stp.mipmap = 0;
			CacheSUFMTex(outfilename, &stp, NULL);
			LocalFree(bmi);
			stc = FindTexCache(outfilename);
			tex = stc->tex[0];
		}
	}
	if(!tex && wm){
		int xs, ys;
		int mx, my;
		int x, y;
		double *dbuf, mi = 1000., ma = 0.;
		double cell;
		double orgx, orgy;
		BITMAPINFO *bmi;
		GLubyte (*bbuf)[3];
		GLint texSize;
		size_t outsize;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
		texSize = MIN(texSize, 2048);
		wm->size(&xs, &ys);
		mx = MIN(xs, texSize);
		my = MIN(ys, texSize);
		cell = width / mx;
		orgx = -width / 2.;
		orgy = -width / 2.;
		dbuf = (double*)malloc(mx * my * sizeof(double));
		for(y = 0; y < my; y++) for(x = 0; x < mx; x++){
			wartile_t wt;
			double h;
			wm->getat(&wt, x * xs / mx, y * ys / my);
			dbuf[x + y * mx] = h = /*MAX*/(0, wt.height);
			if(h < mi)
				mi = h;
			if(ma < h)
				ma = h;
		}
		bmi = (BITMAPINFO*)malloc(outsize = sizeof(BITMAPINFOHEADER) + mx * my * 3 * sizeof(GLubyte));
		bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi->bmiHeader.biWidth = mx; 
		bmi->bmiHeader.biHeight = -my;
		bmi->bmiHeader.biPlanes = 1;
		bmi->bmiHeader.biBitCount = 24;
		bmi->bmiHeader.biCompression = BI_RGB;
		bmi->bmiHeader.biSizeImage = mx * my * 3 * sizeof(GLubyte);
		bmi->bmiHeader.biXPelsPerMeter = 0;
		bmi->bmiHeader.biYPelsPerMeter = 0;
		bmi->bmiHeader.biClrUsed = 0;
		bmi->bmiHeader.biClrImportant = 0;
/*					bmi = ReadBitmap("earth.bmp");*/
/*					bmi = lzuc(lzw_earth, sizeof lzw_earth, NULL);*/
/*					texlist = ProjectSphereMap("earth.bmp", bmi);*/
		bbuf = (GLubyte(*)[3])&(&bmi->bmiHeader)[1]/*(GLubyte(*)[3])malloc(mx * my * 3 * sizeof(GLubyte))*/;
#define SQRT2P2 (1.41421356/2.)
#define glColor3ub(r,g,b) (bbuf[x + y * mx][0] = r, bbuf[x + y * mx][1] = g, bbuf[x + y * mx][2] = b)
#define glColor3f(r,g,b) (bbuf[x + y * mx][0] = (r) * 255, bbuf[x + y * mx][1] = (g) * 255, bbuf[x + y * mx][2] = (b) * 255)
		for(y = 0; y < my; y++) for(x = 0; x < mx; x++){
			double h = dbuf[x + y * my];
			double r;
			struct random_sequence rs;
			Vec3d normal(0,1,0), light(SQRT2P2, SQRT2P2, 0);
#if 0
			{
				extern struct astrobj earth;
				double dx = x * cell + orgx, dz = y * cell + orgx;
				double rad = earth.rad /* EARTH_RAD */;
				h -= sqrt(rad * rad - dx * dx - dz * dz) - rad;
			}
#endif
			normalmap(wm, x * xs / mx, y * ys / my, xs, ys, width / xs, &normal);
			initfull_rseq(&rs, x, y);
			r = 0./*(drseq(&rs) - .5) / 1.5*/;
			if(0. <= h){
				extern double perlin_noise_pixel(int x, int y, int bit);
				double browness = .3 / ((h) / .01 + 1.);
				double whiteness = /*weather == wsnow ? 1. : */h < 1. ? 0. : 1. - 1. / (h * .1 + .9);
				browness = rangein(MAX(browness, 1. - pow(normal[1], 8.) + r), 0., 1.);
/*				if(weather == wsnow)
					glColor3ub(255 * whiteness, 255 * whiteness, 255 * whiteness);
				else if(whiteness)
					glColor3ub((1. - whiteness) * 127 * browness + 255 * whiteness, (1. - whiteness) * (255 - 127 * browness) + 255 * whiteness, (1. - whiteness) * 31 + 255 * whiteness);
				else*/{
					double sandiness = pow(normal[1], 8.) * (0. <= h && h < .010 ? (.010 - h) / .010 : 0.);
					double sp;
					double foilage;
					GLfloat aaa[3];
					foilage = foilage_level(x * xs / mx - xs / 2, y * ys / my - ys / 2);
					aaa[0] = 230 * sandiness + (1. - sandiness) * 127 * browness;
					aaa[1] = 230 * sandiness + (1. - sandiness) * (255 - 127 * browness);
					aaa[2] = 120 * sandiness + (1. - sandiness) * 31;
					aaa[0] *= 1. - foilage;
					aaa[1] *= 1. - foilage / 2.;
					aaa[2] *= 1. - foilage;
					sp = VECSP(normal, light);
					sp = (sp + 1.) / 2.;
					VECSCALEIN(aaa, .5 + .5 * h / ma);
					VECSCALEIN(aaa, sp);
					glColor3ub(aaa[0], aaa[1], aaa[2]);
				}
			}
			else{
				double f = 1. / (-(h) / .01 + 1.);
				glColor3f(.75 * f + .25, .5 * f + .5, .25 * f + .75);
	/*			double browness = 1. / (-(h) * 2.718281828459 / .05 + 1.);
				browness = rangein(browness + r, 0, 1);
				glColor3ub(127 * browness, 127 * browness, 127 * (browness - .5) * (browness - .5));*/
			}
#undef glColor3ub
#undef glColor3f
/*			bbuf[x + y * mx][0] = (GLubyte)(255 * (dbuf[x + y * my] - mi) / (ma - mi));
			bbuf[x + y * mx][1] = 2 * (GLubyte)(255 * (dbuf[x + y * my] - mi) / (ma - mi));
			bbuf[x + y * mx][2] = (GLubyte)(255 * (dbuf[x + y * my] - mi) / (ma - mi)) / 2;*/
		}

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mx, my, 0,
			GL_RGB, GL_UNSIGNED_BYTE, bbuf);

/*		glNewList(tex = glGenLists(1), GL_COMPILE);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);*/
		glEndList();

		{
			FILE *fp;
			if(fp = fopen(outfilename, "wb")){
				BITMAPFILEHEADER fh;
				((char*)&fh.bfType)[0] = 'B';
				((char*)&fh.bfType)[1] = 'M';
				fh.bfSize = sizeof(BITMAPFILEHEADER) + outsize;
				fh.bfReserved1 = fh.bfReserved2 = 0;
				fh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmi->bmiHeader.biClrUsed * sizeof(RGBQUAD);
				fwrite(&fh, 1, sizeof(BITMAPFILEHEADER), fp);
				for(y = 0; y < my; y++) for(x = 0; x < mx; x++){
					bbuf[x + y * mx][0] ^= bbuf[x + y * mx][2];
					bbuf[x + y * mx][2] ^= bbuf[x + y * mx][0];
					bbuf[x + y * mx][0] ^= bbuf[x + y * mx][2];
				}
				fwrite(bmi, 1, outsize, fp);
				fclose(fp);
			}
		}

		free(dbuf);
		free(bmi);
	}
	if(tex && wm){
		double cell, x0, y0, yaw, tc[4][2];
		cell = scale / width /* g_far / width*/;
		x0 = (pos[0] + width / 2.) / width;
		y0 = (pos[2] + width / 2.) / width;
/*		x = x0 - cell;
		y = y0 - cell;
		x1 = x + 2 * cell;
		y1 = y + 2 * cell;*/
		if(r_maprot){
			int i;
			cell *= sqrt(2.);
			tc[0][0] = cell * cos(angle + M_PI / 4.);
			tc[2][0] = -tc[0][0];
			tc[0][1] = cell * sin(angle + M_PI / 4.);
			tc[2][1] = -tc[0][1];
			tc[1][0] = cell * cos(angle + M_PI * 3 / 4.);
			tc[3][0] = -tc[1][0];
			tc[1][1] = cell * sin(angle + M_PI * 3 / 4.);
			tc[3][1] = -tc[1][1];
			for(i = 0; i < 4; i++){
				tc[i][0] += x0;
				tc[i][1] += y0;
			}
		}
		else{
			angle = 0.;
			tc[0][0] = tc[3][0] = x0 + cell;
			tc[1][0] = tc[2][0] = x0 - cell;
			tc[0][1] = tc[1][1] = y0 + cell;
			tc[2][1] = tc[3][1] = y0 - cell;
		}
/*		projection((glPushMatrix(), glLoadIdentity(), glOrtho(-1., 1., -1., 1., -1, 1)));*/
		glPushMatrix();
/*		glLoadIdentity();
		glTranslated(1. - MIN(gvp.h, gvp.w) / (double)gvp.w, 1. - MIN(gvp.h, gvp.w) / (double)gvp.h, 0.);
		glScaled(MIN(gvp.h, gvp.w) / (double)gvp.w, MIN(gvp.h, gvp.w) / (double)gvp.h, 1.);
		glTranslated(0., 0., 0.);*/
/*		MiniMapPos();*/

		glPushAttrib(GL_TEXTURE_BIT);
		glBindTexture(GL_TEXTURE_2D, tex);
/*		glCallList(tex);*/
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glEnable(GL_TEXTURE_2D);
//		glDisable(GL_CULL_FACE);
		glBegin(GL_QUADS);
		glTexCoord2dv(tc[0]); glVertex2d(1., -1.);
		glTexCoord2dv(tc[1]); glVertex2d(-1., -1.);
		glTexCoord2dv(tc[2]); glVertex2d(-1., 1.);
		glTexCoord2dv(tc[3]); glVertex2d(1., 1.);
		glEnd();
		glPopAttrib();


/*		glRasterPos3d(-1., -1., 0.);
		gldprintf("range %lg km", g_maprange);*/

		/* center indicator */
/*		glBegin(GL_LINES);
		glVertex2d(-1., 0.);
		glVertex2d(1., 0.);
		glVertex2d(0., -1.);
		glVertex2d(0., 1.);
		glEnd();*/

		/* compass */
		glTranslated(.7, .7, 0.);
		glRotated(deg_per_rad * angle + 180, 0., 0., 1.);

		glBegin(GL_LINE_STRIP);
		glVertex2d(-.05, .1);
		glVertex2d(.05, .1);
		glVertex2d(.0, .15);
		glVertex2d(.0, -.1);
		glEnd();

		glBegin(GL_LINES);
		glVertex2d(-.1, 0.);
		glVertex2d(.1, 0.);
		glEnd();

/*		yaw = pl.pyr[1] + (pl.chase ? pl.chase->pyr[1] : 0.);
		glBegin(GL_LINES);
		glVertex2d(.5, .5);
		glVertex2d(.5 - .2 * cos(yaw), .5 + .2 * sin(yaw));
		glVertex2d(.5, .4);
		glVertex2d(.5, .6);
		glVertex2d(.4, .5);
		glVertex2d(.6, .5);
		glEnd();*/
		glPopMatrix();
/*		projection(glPopMatrix());*/
		glBindTexture(GL_TEXTURE_2D, 0);
		return 1;
	}
	return 0;
}
