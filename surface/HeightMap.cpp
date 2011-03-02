#include "WarMap.h"
#include "judge.h"
#include "astro.h"
#include "cmd.h"
#include "war.h"
extern "C"{
#include <clib/c.h>
#include <clib/stats.h>
#include <clib/timemeas.h>
#include <clib/avec3.h>
#include <clib/rseq.h>
#include <clib/zip/UnZip.h>
}
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <math.h>

#if 0
#define MAPSIZE 1024
#else
#define MAPSIZE 2048
#endif
#define TOPOSIZE 512
#define CACHEPIXEL 8
#if 0
#define HGTSIZE /*1201 */ 3601 
#endif

#define BUCKETS 509

static int SUBPIXEL = 1;
static int NOISEBITS = 1;
static double hgt_noise_base = .01;



#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef ABS
#define ABS(a) ((a)<0?-(a):(a))
#endif

#define signof(a) ((a)<0?-1:0<(a)?1:0)

static WarMap *OpenDatMap(const char *fname);

class HeightMap : public WarMap{
public:
	HeightMap();
	int getat(WarMapTile *wt, int x, int y);
	void size(int *x, int *y);
	double width();
	void levelterrain(int x0, int y0, int x1, int y1);
	int hitline(const double (*src)[3], const double (*dir)[3], double dt, double (*ret)[3]);
	const void *rawData()const{return value;}

	/* cache value is more precise and matched type to result */
	typedef struct mapcache{
		int x, y;
		struct mapcache *next;
		double value[CACHEPIXEL][CACHEPIXEL];
	} mapcache_t;

	mapcache_t *mc, *mcht[BUCKETS];
	int imc, nmc; /* Ring Buffer Pointer */
	short value[MAPSIZE][MAPSIZE];
};

int g_hgtoffset = 0;
double g_hgtroundoff = 0.;

HeightMap::HeightMap(){
	int i;
	imc = 0;
	nmc = 512;
	mc = (mapcache*)malloc(nmc * sizeof *mc);
	for(i = 0; i < nmc; i++){
		mc[i].x = mc[i].y = -1;
		mc[i].next = NULL;
	}
	for(i = 0; i < BUCKETS; i++)
		mcht[i] = NULL;
}

static WarMap *OpenAVMap(const char *fname){
	FILE *fp;
	HeightMap *ret = NULL;
	int i, j, m, n = 0;
	double wid;
	if(!strcmp(strrchr(fname, '.'), ".zip")){
		void *pv;
		unsigned long size;
		pv = ZipUnZip(fname, NULL, &size);
		if(!pv)
			return NULL;
		ret = new HeightMap();
		assert(size == sizeof ret->value);
		memcpy(ret->value, pv, sizeof ret->value);
		ZipFree(pv);
	}
	else{
		fp = fopen(fname, "rb");
		if(!fp)
			return NULL;
		fprintf(stderr, "file %s opened.\n", fname);
		ret = new HeightMap();
		fread(ret->value, sizeof ret->value, 1, fp);
		fclose(fp);
	}
	ret->nalt = 0;
	ret->alt = NULL;
	wid = ret->width();
#if 0
	for(i = 0; i < MAPSIZE; i++) for(j = 0; j < MAPSIZE; j++){
		extern struct astrobj earth;
		double rad = earth.rad /* EARTH_RAD */;
		double x = (j - MAPSIZE / 2.) * wid / MAPSIZE;
		double z = (i - MAPSIZE / 2.) * wid / MAPSIZE;
		if(!ret->value[i][j])
			ret->value[i][j] = -500;
		ret->value[i][j] -= g_hgtoffset + g_hgtroundoff * ((i - MAPSIZE / 2) * (i - MAPSIZE / 2) + (j - MAPSIZE / 2) * (j - MAPSIZE / 2)) / (MAPSIZE / 2);
		ret->value[i][j] += 1000 * (sqrt(rad * rad - x * x - z * z) - rad);
	}
#endif
/*	{
	struct random_sequence rs;
	init_rseq(&rs, 35298);
	for(i = 0; i < 64; i++){
		double x0, y0, r, w = 40000 / 360.;
		x0 = (drseq(&rs) - .5) * w;
		y0 = (drseq(&rs) - .5) * w;
		r = drseq(&rs) * 1.;
		for(i = 0; i < MAPSIZE; i++) for(j = 0; j < MAPSIZE; j++){
			double y = ((double)i / MAPSIZE - .5) * w, x = ((double)j / MAPSIZE - .5) * w;
			double sd, d, z, z1;
			sd = ((x - x0) * (x - x0) + (y - y0) * (y - y0)) / r / r;
			d = sqrt(sd);
			z = exp(-(d - 3.)*(d - 3.)) + exp(-(d + 3.)*(d + 3.)) * .5;
			z1 = exp(-sd / 16.) * 2. - 1.;
			z += z1 < 0 ? 0 : -z1;
			ret->value[i][j] += z * r * 500;
		}
	}
	}*/
	{
	size_t filesize = sizeof *ret;

	/* export raw */
	if(0){
		short *buf2;
		const short (*src)[MAPSIZE] = ret->value;
		char strbuf[256];
		buf2 = (short*)malloc(filesize);
		for(i = 0; i < MAPSIZE; i++) for(j = 0; j < MAPSIZE; j++){
			short srcp = src[i][j];
			buf2[i * MAPSIZE + j] = srcp < 0 ? 0 : srcp * 16;
		}
		strcpy(strbuf, fname);
		strcat(strbuf, ".raw");
		fp = fopen(strbuf, "wb");
		if(!fp)
			return ret;
		fwrite(buf2, MAPSIZE * MAPSIZE * sizeof *buf2, 1, fp);
		fclose(fp);
		free(buf2);
	}
	}

	return ret;
}

class TopoMap : public WarMap{
public:
	short value[TOPOSIZE][TOPOSIZE];

	int getat(WarMapTile *wt, int x, int y);
	void size(int *x, int *y);
	double width();
	void levelterrain(int x0, int y0, int x1, int y1);
	int hitline(const double (*src)[3], const double (*dir)[3], double dt, double (*ret)[3]);
} topomap_t;

static unsigned hashfuncxy(int x, int y){
/*	struct random_sequence rs;
	initfull_rseq(&rs, x, y);
	return rseq(&rs) % BUCKETS;*/
	return (0xa531b87e^x^(y<<16)) % BUCKETS;
}

static WarMap *OpenTopoMap(const char *fname){
	FILE *fp;
	TopoMap *ret = NULL;
	int i, j, m, n = 0;
	double w;
	if(!strcmp(strrchr(fname, '.'), ".zip")){
		void *pv;
		unsigned long size;
		pv = ZipUnZip(fname, NULL, &size);
		ret = new TopoMap();
		assert(size == sizeof ret->value);
		memcpy(ret->value, pv, sizeof ret->value);
		ZipFree(pv);
	}
	else{
		fp = fopen(fname, "rb");
		if(!fp)
			return NULL;
		fprintf(stderr, "file %s opened.\n", fname);
		ret = new TopoMap();
		fread(ret->value, sizeof ret->value, 1, fp);
		fclose(fp);
	}
	ret->nalt = 0;
	ret->alt = NULL;
/*	for(i = 0; i < MAPSIZE; i++) for(j = 0; j < MAPSIZE; j++)
		ret->value[i][j] -= g_hgtoffset + g_hgtroundoff * ((i - MAPSIZE / 2) * (i - MAPSIZE / 2) + (j - MAPSIZE / 2) * (j - MAPSIZE / 2)) / (MAPSIZE / 2);*/
	w = ret->width();
#if 0
	for(i = 0; i < TOPOSIZE; i++) for(j = 0; j < TOPOSIZE; j++){
		extern struct astrobj earth;
		double rad = earth.rad /* EARTH_RAD */;
		double x = (j - TOPOSIZE / 2.) * w / TOPOSIZE;
		double z = (i - TOPOSIZE / 2.) * w / TOPOSIZE;
		if(!ret->value[i][j])
			ret->value[i][j] = -500;
		ret->value[i][j] += 1000 * (sqrt(rad * rad - x * x - z * z) - rad);
	}
#endif
	return ret;
}

class HeightMap2 : public HeightMap{
public:
	short value2[MAPSIZE][MAPSIZE];
};

WarMap *OpenHGTMap(const char *fname){
	FILE *fp;
	char buf[128];
	HeightMap *ret = NULL;
	int i, j, m, n = 0;
	int geotiff = 0;
	unsigned int hgtsize = 3601, filesize;
	short last = 0;
	if(!strcmp(strchr(fname, '.'), ".raw"))
		return OpenTopoMap(fname);
	if(!strcmp(strchr(fname, '.'), ".dat"))
		return OpenDatMap(fname);
	if(!strcmp(strchr(fname, '.'), ".av") || !strcmp(strrchr(fname, '.'), ".zip"))
		return OpenAVMap(fname);
	fp = fopen(fname, "rb");
	if(!fp)
		return NULL;
	if(!strcmp(strchr(fname, '.'), ".tif"))
		geotiff = 1;
	fprintf(stderr, "file %s opened.\n", fname);
	ret = new HeightMap2();
	fseek(fp, 0, SEEK_END);
	filesize = geotiff ? 3601 * 3601 * sizeof(short) : ftell(fp);
	fseek(fp, 0, SEEK_SET);
	hgtsize = geotiff ? 3601 : ::sqrt((double)(filesize / sizeof(short)));
#if 1
	{
	short *buf;
	buf = (short*)malloc(filesize);
	if(geotiff)
		fseek(fp, 8, SEEK_SET);
	fread(buf, filesize, 1, fp);

	/* byte order exchange */
	if(!geotiff) for(i = 0; i < filesize / sizeof*buf; i++){
		buf[i] = (((unsigned short)buf[i]) << 8) | (((unsigned short)buf[i]) >> 8);
		if(buf[i] < -1000)
			n++;
	}

	if(n){
		short *buf2;
		buf2 = (short*)malloc(filesize);
		for(m = 0; n; m++){
			short *src, *dst;
			printf("[%d]n: %d\n", m, n);
			n = 0;
			src = m % 2 ? buf2 : buf;
			dst = m % 2 ? buf : buf2;
			for(i = 0; i < hgtsize; i++) for(j = 0; j < hgtsize; j++) if(src[i * hgtsize + j] < -1000){
				int ii, jj, v = 0, nn = 0, ok = 0;
				for(ii = MAX(0, i - 3); ii <= MIN(hgtsize - 1, i + 3); ii++) for(jj = MAX(0, j - 3); jj <= MIN(hgtsize - 1, j + 3); jj++){
					if(src[ii * hgtsize + jj] >= -1000){
						int sd = (ii - i) * (ii - i) + (jj - j) * (jj - j);
						if(sd < 8){
							v += src[ii * hgtsize + jj] * (8 - sd);
							nn += (8 - sd);
							if(sd <= 2)
								ok = 1;
						}
					}
				}

				if(ok && nn)
					dst[i * hgtsize + j] = v / nn;
				n++;
			}
			else
				dst[i * hgtsize + j] = src[i * hgtsize + j];
		}
		if(m % 2){
			free(buf);
			buf = buf2;
		}
		else
			free(buf2);

		for(i = 0; i < hgtsize; i++) for(j = 0; j < hgtsize; j++) if(buf[i * hgtsize + j] < -1000){
			n++;
		}
	}

#if 1
	/* crop; not to resample */
	for(i = 0; i < MAPSIZE; i++){
		int i1 = i + 3601 - MAPSIZE;
		for(j = 0; j < MAPSIZE; j++){
			int id = MAPSIZE - j - 1, jd = MAPSIZE - i - 1;
			int j1;
			j1 = j + 3601 - MAPSIZE;
			ret->value[id][jd] = buf[(hgtsize <= j1 ? hgtsize - 1 : j1) + (hgtsize <= i1 ? hgtsize - 1 : i1) * hgtsize];
		}
	}
#else
	/* resampling by linear */
	for(i = 0; i < MAPSIZE; i++){
		int i1 = i * hgtsize / MAPSIZE;
		for(j = 0; j < MAPSIZE; j++){
			int id = MAPSIZE - j - 1, jd = i;
			int j1;
			double ifr, jfr;
			j1 = j * hgtsize / MAPSIZE;
			ifr = fmod((double)i * hgtsize / MAPSIZE, 1.);
			jfr = fmod((double)j * hgtsize / MAPSIZE, 1.);
			if(hgtsize <= j1 + 1 || hgtsize <= i1 + 1)
				ret->value[id][jd] = buf[j1 + i1 * hgtsize];
			else
				ret->value[id][jd] = (buf[j1 + i1 * hgtsize] * (1. - jfr) + buf[j1 + 1 + i1 * hgtsize] * jfr) * (1. - ifr)
					+ (buf[j1 + (i1 + 1) * hgtsize] * (1. - jfr) + buf[j1 + 1 + (i1 + 1) * hgtsize] * jfr) * ifr;
		}
	}
#endif

	free(buf);
	}
#elif
	{
	short (*buf)[HGTSIZE];
	buf = malloc(2 * sizeof *buf);
	for(i = 0; i < MAPSIZE; i++){
		int i1 = i * HGTSIZE / MAPSIZE;
		fseek(fp, (i * HGTSIZE / MAPSIZE) * HGTSIZE * sizeof(short), SEEK_SET);
		fread(buf[0], HGTSIZE, sizeof(short), fp);
		if(i1 + 1 < HGTSIZE)
			fread(buf[1], HGTSIZE, sizeof(short), fp);
		for(j = 0; j < HGTSIZE; j++){
			int k = 0;
			for(k = 0; k < 1 + (i1 + 1 < HGTSIZE); k++){
				buf[k][j] = (((unsigned short)buf[k][j]) << 8) | (((unsigned short)buf[k][j]) >> 8);
				if(buf[k][j] == -32768){
					n++;
					buf[k][j] = last;
				}
				else
					last = buf[k][j];
			}
		}
		for(j = 0; j < MAPSIZE; j++){
			int id = MAPSIZE - j - 1, jd = i;
			int j1;
			double ifr, jfr;
			j1 = j * HGTSIZE / MAPSIZE;
			ifr = fmod((double)i * HGTSIZE / MAPSIZE, 1.);
			jfr = fmod((double)j * HGTSIZE / MAPSIZE, 1.);
			if(HGTSIZE <= j1 + 1 || HGTSIZE <= i1 + 1)
				ret->value[id][jd] = buf[0][j1];
			else
				ret->value[id][jd] = (buf[0][j1] * (1. - jfr) + buf[0][j1+1] * jfr) * (1. - ifr)
					+ (buf[1][j1] * (1. - jfr) + buf[1][j1+1] * jfr) * ifr;
/*			ret->value[i][j] = ((unsigned short)ret->value[i][j] << 8) | ((unsigned short)ret->value[i][j] >> 8);*/
/*			if(ret->value[i][j] < -10000)
				n++;*/
/*				ret->value[i][j] = last;
			else
				last = ret->value[i][j];*/
		}
	}
	free(buf);
	}
#else
	for(i = 0; i < HGTSIZE; i++){
		fread(&ret->value[i], HGTSIZE, sizeof(*ret->value[i]), fp);
		for(j = 0; j < HGTSIZE; j++){
			ret->value[i][j] = ((unsigned short)ret->value[i][j] << 8) | ((unsigned short)ret->value[i][j] >> 8);
			if(ret->value[i][j] == -32768)
				ret->value[i][j] = last;
			else
				last = ret->value[i][j];
		}
	}
#endif
	fclose(fp);
	if(1){
/*	short dat[MAPSIZE][MAPSIZE];*/
	memcpy(&(*ret->value)[MAPSIZE * MAPSIZE], ret->value, sizeof ret->value);
	if(0&&n){
	for(m = 0; n; m++){
		short *src, *dst;
/*		printf("[%d]n: %d\n", m, n);*/
		n = 0;
		src = &(*ret->value)[m % 2 * MAPSIZE * MAPSIZE];
		dst = &(*ret->value)[(m + 1) % 2 * MAPSIZE * MAPSIZE];
		for(i = 0; i < MAPSIZE; i++) for(j = 0; j < MAPSIZE; j++) if(src[i * MAPSIZE + j] == -32768){
			int ii, jj, v = 0, nn = 0, ok = 0;
			for(ii = MAX(0, i - 3); ii <= MIN(MAPSIZE - 1, i + 3); ii++) for(jj = MAX(0, j - 3); jj <= MIN(MAPSIZE - 1, j + 3); jj++) if(src[ii * MAPSIZE + jj] != -32768){
				int sd = (ii - i) * (ii - i) + (jj - j) * (jj - j);
				if(sd < 8){
					v += src[ii * MAPSIZE + jj] * (8 - sd);
					nn += (8 - sd);
					if(sd <= 2)
						ok = 1;
				}
			}
			if(ok && nn)
				dst[i * MAPSIZE + j] = v / nn;
			n++;
		}
		else
			dst[i * MAPSIZE + j] = src[i * MAPSIZE + j];
	}
	if(m)
		memcpy(ret->value, &(*ret->value)[MAPSIZE * MAPSIZE], sizeof ret->value);
	}
	delete ret;
	ret = new HeightMap();
	{
		char *nfname;
		nfname = (char*)malloc(strlen(fname) + 1);
		strcpy(nfname, fname);
		strcpy(strchr(nfname, '.'), ".av");
		fp = fopen(nfname, "wb");
		free(nfname);
		fwrite(ret->value, sizeof ret->value, 1, fp);
		fclose(fp);
	}
	}
#if 0
	for(i = 0; i < MAPSIZE; i++) for(j = 0; j < MAPSIZE; j++){
		extern struct astrobj earth;
		double rad = earth.rad /* EARTH_RAD */;
		double x = (j - MAPSIZE / 2.) * 40000 / 360. / MAPSIZE;
		double z = (i - MAPSIZE / 2.) * 40000 / 360. / MAPSIZE;
		if(!ret->value[i][j])
			ret->value[i][j] = -500;
//		ret->value[i][j] -= g_hgtoffset + g_hgtroundoff * ((i - MAPSIZE / 2) * (i - MAPSIZE / 2) + (j - MAPSIZE / 2) * (j - MAPSIZE / 2)) / (MAPSIZE / 2);
		ret->value[i][j] += 1000 * (sqrt(rad * rad - x * x - z * z) - rad);
	}
#endif
	ret->alt = NULL;
	ret->nalt = 0;
	return ret;
}
double perlin_noise_pixel(int x0, int y0, int bits);

static int GaussJordan(double value[], int count){
	double a;
	int i, j, k;
	double temp;
	for(i = 0; i < count; i++){
		a = value[i + i * count];
		if(fabs(a) < .0001)
			return 0;
		for(k = i; k < count; k++)
			value[k + i * count] = value[k + i * count] / a;
		for(j = 0; j < count; j++){
			if(j != i){
				temp = value[i + j * count];
				for(k = i; k < count; k++)
					value[k + j * count] -= temp * value[k + i * count];
			}
		}
	}
	return 1;
}

/* to allow the compiler to select fastest calling convention, static linkage is preferred. */
static double spline_cubic(const double a[4], double x){
	int i, j;
	double alpha[4], l[4], mu[4], z[4];
	double b[4], c[4], d[4];
	for(i = 1; i < 3; i++)
		alpha[i] = 3. * (a[i+1] - a[i]) - 3. * (a[i] - a[i-1]);
	l[0] = 1.;
	mu[0] = 0.;
	z[0] = 0.;
	for(i = 1; i < 3; i++){
		l[i] = 4. - mu[i-1];
		mu[i] = 1. / l[i];
		z[i] = (alpha[i] - z[i-1]) / l[i];
	}
	l[3] = 1.;
	z[3] = 0.;
	c[3] = 0.;
	for(j = 2; 0 <= j; j--){
		c[j] = z[j] - mu[j] * c[j+1];
		b[j] = a[j+1] - a[j] - (c[j+1] + 2. * c[j]) / 3.;
		d[j] = (c[j+1] - c[j]) / 3.;
	}
	return a[1] + b[1] * x + c[1] * x * x + d[1] * x * x * x;
}

static void spline_cubic_f(const double a[4], double f[4]){
	int i, j;
	double alpha[4], l[4], mu[4], z[4];
	double b[4], c[4], d[4];
	for(i = 1; i < 3; i++)
		alpha[i] = 3. * (a[i+1] - a[i]) - 3. * (a[i] - a[i-1]);
	l[0] = 1.;
	mu[0] = 0.;
	z[0] = 0.;
	for(i = 1; i < 3; i++){
		l[i] = 4. - mu[i-1];
		mu[i] = 1. / l[i];
		z[i] = (alpha[i] - z[i-1]) / l[i];
	}
	l[3] = 1.;
	z[3] = 0.;
	c[3] = 0.;
	for(j = 2; 0 <= j; j--){
		c[j] = z[j] - mu[j] * c[j+1];
		b[j] = a[j+1] - a[j] - (c[j+1] + 2. * c[j]) / 3.;
		d[j] = (c[j+1] - c[j]) / 3.;
	}
	f[0] = a[1];
	f[1] = b[1];
	f[2] = c[1];
	f[3] = d[1];
}


int HeightMap::getat(WarMapTile *wt, int x00, int y00){
	int x, y, x0 = x00/*MAPSIZE * SUBPIXEL - x00 - 1*/, y0 = y00/*MAPSIZE * SUBPIXEL - y00 - 1*/, xb = x0 / SUBPIXEL, yb = y0 / SUBPIXEL;
	if(SUBPIXEL <= 1){
/*		timemeas_t tm;
		static unsigned c = 0;
		TimeMeasStart(&tm);*/
		x = x0, y = y0;
		wt->height = .001 * value[(MAPSIZE <= x ? MAPSIZE-1 : x < 0 ? 0 : x)][(MAPSIZE <= y ? MAPSIZE-1 : y < 0 ? 0 : y)];
/*		c++;
		if(c % 10000 == 0)
			printf("time map (%lu)%lg\n", c, TimeMeasLap(&tm));*/
		return 1;
	}
	else{
		int xm = x0 % SUBPIXEL, ym = y0 % SUBPIXEL, i, j;
		double ret = 0.;
#if 1
		double ax[4], ay[4]/*, ax1[4], ay1[4]*/;
#	if 1
		int n;
/*		int c;
		static double timeint = 0.;
		static statistician_t stc;
		timemeas_t tm;
		c = GetStatsCount(&stc) % 10000 == 0;
		TimeMeasStart(&tm);
		if(c)
			printf("time0 %lg\n", TimeMeasLap(&tm));*/
		{
			mapcache_t *mc;
			n = nmc;
			for(mc = mcht[hashfuncxy(xb, yb)]; mc; mc = mc->next) if(mc->x == xb && mc->y == yb){
				ret = mc->value[xm][ym];
				n = mc - mc;
//				PutStats(&stc, 1);
				break;
			}
		}
/*		for(n = 0; n < m->nmc; n++) if(m->mc[n].x == xb && m->mc[n].y == yb){
			ret = m->mc[n].value[xm][ym];
			PutStats(&stc, 1);
			break;
		}*/
		if(n == nmc){
			int xx, yy;
			double fxy[4][4];
			double f[4];
			double t1;
			for(x = xb - 1, i = 0; i < 4; x++, i++){
				for(j = 0, y = yb - 1; j < 4; y++, j++)
					ay[j] = .001 * value[(MAPSIZE <= x ? MAPSIZE-1 : x < 0 ? 0 : x)][(MAPSIZE <= y ? MAPSIZE-1 : y < 0 ? 0 : y)];
				spline_cubic_f(ay, fxy[i]);
			}
			for(yy = 0; yy < SUBPIXEL; yy++){
				double dx = (double)yy / SUBPIXEL, dx2 = dx * dx, dx3 = dx2 * dx;
				for(x = xb - 1, i = 0; i < 4; x++, i++){
					ax[i] = fxy[i][0] + fxy[i][1] * dx + fxy[i][2] * dx2 + fxy[i][3] * dx3;
/*					for(j = 0, y = yb - 1; j < 4; y++, j++)
						ay[j] = .001 * m->value[(MAPSIZE <= x ? MAPSIZE-1 : x < 0 ? 0 : x)][(MAPSIZE <= y ? MAPSIZE-1 : y < 0 ? 0 : y)];
					ax[i] = spline_cubic(ay, (double)yy / CACHEPIXEL);*/
				}
				spline_cubic_f(ax, f);
				for(xx = 0; xx < SUBPIXEL; xx++){
					double x = (double)xx / SUBPIXEL;
					mc[imc].value[xx][yy] = f[0] + f[1] * x + f[2] * x * x + f[3] * x * x * x;
//					m->mc[m->imc].value[xx][yy] = (spline_quad(ax, (double)xx / CACHEPIXEL)/* + spline_quad(ax1, 1. - (double)xm / SUBPIXEL)) / 2.*/);
				}
			}
/*			t1 = TimeMeasLap(&tm);*/
			{
				mapcache_t **ppmc, *mc = &mc[imc];
				for(ppmc = &mcht[hashfuncxy(mc->x, mc->y)]; *ppmc; ppmc = &(*ppmc)->next) if((*ppmc)->x == mc->x && (*ppmc)->y == mc->y){
					*ppmc = mc->next;
					break;
				}
				ret = mc[imc].value[xm][ym];
				mc->x = xb;
				mc->y = yb;
				ppmc = &mcht[hashfuncxy(mc->x, mc->y)];
				mc->next = *ppmc;
				*ppmc = mc;
			}
			imc = (imc + 1) % nmc;
//			PutStats(&stc, 0);
/*			if(c)
				printf("time c %lg\n", TimeMeasLap(&tm) - t1);*/
		}
/*		stc.cnt++;
		timeint += TimeMeasLap(&tm);
		if(c){
			extern warf_t warf;
			if(1)
				printf("time all %s %lg\n", n != m->nmc ? "hit " : "miss", TimeMeasLap(&tm));
			printf("stat (%d)%lg %lg %lg %lg\n", GetStatsCount(&stc), GetStatsAvg(&stc), timeint, warf.realtime, timeint / warf.realtime);
		}*/
#	elif 1
		static int c;
		timemeas_t tm;
		TimeMeasStart(&tm);
		if(xm == 0 && ym == 0)
			ret = .001 * m->value[(MAPSIZE <= xb ? MAPSIZE-1 : xb < 0 ? 0 : xb)][(MAPSIZE <= yb ? MAPSIZE-1 : yb < 0 ? 0 : yb)];
		else{
			for(x = xb - 1, i = 0; i < 4; x++, i++){
				if(ym == 0)
					ax[i] = .001 * m->value[(MAPSIZE <= x ? MAPSIZE-1 : x < 0 ? 0 : x)][(MAPSIZE <= yb ? MAPSIZE-1 : yb < 0 ? 0 : yb)];
				else{
					for(j = 0, y = yb - 1; j < 4; y++, j++)
						ay[j] = .001 * m->value[(MAPSIZE <= x ? MAPSIZE-1 : x < 0 ? 0 : x)][(MAPSIZE <= y ? MAPSIZE-1 : y < 0 ? 0 : y)];
					ax[i] = spline_cubic(ay, (double)ym / SUBPIXEL);
				}
	/*			for(j = 0; j < 4; j++)
					ay1[j] = ay[3-j];
				ax[i] += spline_quad(ay1, 1. - (double)ym / SUBPIXEL);
				ax[i] /= 2.;
				ax1[3-i] = ax[i];*/
			}
			ret = (spline_cubic(ax, (double)xm / SUBPIXEL)/* + spline_quad(ax1, 1. - (double)xm / SUBPIXEL)) / 2.*/);
		}
		if(c % 10000 == 0)
			printf("time all %lg\n", TimeMeasLap(&tm));
		c++;
#	else
		for(x = xb - 1, i = 0, y = yb; x <= xb + 2; x++, i++){
			ax[i] = .001 * m->value[(MAPSIZE <= x ? MAPSIZE-1 : x < 0 ? 0 : x)][(MAPSIZE <= y ? MAPSIZE-1 : y < 0 ? 0 : y)];
		}
		ret = spline_quad(ax, (double)xm / SUBPIXEL)/* * spline_quad(ay, (double)ym / SUBPIXEL)*/;
#	endif
#else
		for(x = xb - 0; x <= xb + 1; x++) for(y = yb - 0; y <= yb + 1; y++){
			double xd, yd;
			double xf, yf;
/*			xd = 4 * SUBPIXEL - fabs((xm - (x - xb) * SUBPIXEL));
			xf = MAX(xd, 0) / SUBPIXEL / 8;
			yd = 4 * SUBPIXEL - fabs((ym - (y - yb) * SUBPIXEL));
			yf = MAX(yd, 0) / SUBPIXEL / 8;*/
			xf = (double)(x == xb ? ((SUBPIXEL - xm) * (SUBPIXEL - xm)) : (xm * xm)) / (((SUBPIXEL - xm) * (SUBPIXEL - xm)) + (xm * xm));
			yf = (double)(y == yb ? (SUBPIXEL - ym) * (SUBPIXEL - ym) : ym * ym) / ((SUBPIXEL - ym) * (SUBPIXEL - ym) + ym * ym);
/*			xf = (double)(x == xb ? SUBPIXEL - xm : xm);
			yf = (double)(y == yb ? SUBPIXEL - ym : ym);*/
			ret += .001 * m->value[(MAPSIZE <= x ? MAPSIZE-1 : x < 0 ? 0 : x)][(MAPSIZE <= y ? MAPSIZE-1 : y < 0 ? 0 : y)]
				* xf * yf;
		}
#endif
		if(NOISEBITS == 0){
			struct random_sequence rs;
			initfull_rseq(&rs, x0, y0);
			wt->height = ret/* / SUBPIXEL / SUBPIXEL */+ (drseq(&rs) - .5) * hgt_noise_base;
		}
		else{
			wt->height = ret/* / SUBPIXEL / SUBPIXEL */+ 
			(xm == 0 && ym == 0 ? 0 : perlin_noise_pixel(x0, y0, NOISEBITS) - .5) * hgt_noise_base;
		}
		return 1;
	}
}

void HeightMap::size(int *x, int *y){
	*x = MAPSIZE * SUBPIXEL;
	*y = MAPSIZE * SUBPIXEL;
}

double HeightMap::width(){
	return 40000 / 360. /*3601. * .03*/ * MAPSIZE / 3601. * .809;
}

int HeightMap::hitline(const double (*src)[3], const double (*dir)[3], double dt, double (*ret)[3]){
	int ix, iy, x0, y0, x1, y1, dx, dy, sx, sy, delta, i;
	double width, xcell, ycell, minray;
	width = this->width();
	this->size(&sx, &sy);
	xcell = width / sx;
	ycell = width / sy;
	x0 = (*src)[0] / xcell;
	y0 = (*src)[2] / ycell;
	x1 = ((*src)[0] + (*dir)[0] * dt) / xcell;
	y1 = ((*src)[2] + (*dir)[2] * dt) / ycell;
	dx = signof(x1 - x0);
	dy = signof(y1 - y0);

	minray = MIN((*src)[1], (*src)[1] + (*dir)[1] * dt);

	/* because cells must be checked inclusively, this many cells */
	delta = dx * (x1 - x0) + dy * (y1 - y0) + 1;
	ix = x0;
	iy = y0;
	for(i = 0; i < delta; i++){
		double x, y, t;
		int j, k;
		avec3_t p[4], n, p01, p02, p0s, pi;
		unsigned short vi[3] = {0, 1, 3};

		do{
		if(ix < 0 || sx - 1 <= ix || iy < 0 || sy - 1 <= iy);
		else{
			double maxcell = 0.;

			for(j = 0; j < 2; j++) for(k = 0; k < 2; k++){
				WarMapTile a;
				getat(&a, ix + j, iy + k);
				p[j + 2 * k][0] = xcell * (ix + j);
				p[j + 2 * k][1] = a.height;
				p[j + 2 * k][2] = ycell * (iy + k);
				if(maxcell < minray)
					minray = a.height;
			}

			if(maxcell < minray)
				break;

			VECSUB(p01, p[1], p[0]);
			VECSUB(p02, p[2], p[0]);
			VECSUB(p0s, *src, p[0]);
			VECVP(n, p02, p01);
			t = -VECSP(p0s, n) / VECSP(*dir, n);
			VECSCALE(pi, *dir, t);
			VECADDIN(pi, *src);
			if(0. < t && t < dt && p[0][0] <= pi[0] && p[0][2] <= pi[2] && pi[0] + pi[2] < p[2][0] + p[2][2]){
				if(ret)
					VECCPY(*ret, pi);
				return 1;
			}
			VECSUB(p01, p[2], p[3]);
			VECSUB(p02, p[1], p[3]);
			VECSUB(p0s, *src, p[3]);
			VECVP(n, p02, p01);
			t = -VECSP(p0s, n) / VECSP(*dir, n);
			VECSCALE(pi, *dir, t);
			VECADDIN(pi, *src);
			if(0. < t && t < dt && p[0][0] <= pi[0] && p[0][1] <= pi[2] && pi[0] < p[3][0] && pi[2] < p[3][2] && p[2][0] + p[2][2] <= pi[0] + pi[2]){
				if(ret)
					VECCPY(*ret, pi);
				return 1;
			}
/*			if(jHitPolygon(p, vi, 3, *src, *dir, 0., dt, &t, *ret, NULL))
				return 1;

			vi[1] = 3; vi[2] = 2;
			if(jHitPolygon(p, vi, 3, *src, *dir, 0., dt, &t, *ret, NULL))
				return 1;*/
		}
		} while(0);

		/* we hit the end */
		if(ix == x1 && iy == y1)
			break;

		/* determine which adjacent cell is to be checked next. */
		if(!dy)
			ix += dx;
		x = ((0 < dy ? (iy + 1) : iy) * ycell - (*src)[2]) * (*dir)[0] / (*dir)[2] + (*src)[0];
		if(0 < dx ? (ix + 1) * xcell < x : dx < 0 ? x < ix * xcell : 0){
			ix += dx;
		}
		else iy += dy;
	}
	return 0;
}

void HeightMap::levelterrain(int x00, int y00, int x10, int y10){
	int x, y, x0 = x00/*MAPSIZE - y10 - 2*/, y0 = y00/*MAPSIZE - x10 - 2*/, x1 = x10/*MAPSIZE - y00 - 2*/, y1 = y10/*MAPSIZE - x00 - 2*/;
	double avg = 0.;
	short v;
	if(1 < SUBPIXEL)
		return;
	x1++;
	y1++;
	for(x = x0; x <= x1; x++) for(y = y0; y <= y1; y++){
		WarMapTile t;
		getat(&t, x, y);
		avg += t.height;
	}
	avg /= (x1 - x0 + 1) * (y1 - y0 + 1);
	v = avg / .001;
	for(x = x0; x <= x1; x++) for(y = y0; y <= y1; y++){
		value[(MAPSIZE <= x ? MAPSIZE-1 : x)][(MAPSIZE <= y ? MAPSIZE-1 : y)] = v;
	}
}








int TopoMap::getat(WarMapTile *wt, int x0, int y0){
	int x = TOPOSIZE - x0 - 1;
	int y = y0/*TOPOSIZE - y0 - 1*/;
	wt->height = .001 * value[(TOPOSIZE <= y ? TOPOSIZE-1 : y < 0 ? 0 : (TOPOSIZE - y - 1))][(TOPOSIZE <= x ? TOPOSIZE-1 : x < 0 ? 0 : x)];
	return 1;
}

void TopoMap::size(int *x, int *y){
	*x = TOPOSIZE;
	*y = TOPOSIZE;
}

double TopoMap::width(){
	return 10920.17606387812129689614840028/*237.04*/;
}

int TopoMap::hitline(const double (*src)[3], const double (*dir)[3], double dt, double (*ret)[3]){
	return 0;
}

void TopoMap::levelterrain(int x0, int y0, int x1, int y1){
	int x, y;
	double avg = 0.;
	short v;
	x1++;
	y1++;
	for(x = x0; x <= x1; x++) for(y = y0; y <= y1; y++){
		WarMapTile t;
		getat(&t, x, y);
		avg += t.height;
	}
	avg /= (x1 - x0 + 1) * (y1 - y0 + 1);
	v = avg / .001;
	for(x = x0; x <= x1; x++) for(y = y0; y <= y1; y++){
		value[(TOPOSIZE <= x ? TOPOSIZE-1 : x)][(TOPOSIZE <= y ? TOPOSIZE-1 : y)] = v;
	}
}






class DatMap : public HeightMap{
	double width();
};


double DatMap::width(){
	return /*10920.17606387812129689614840028/*/200;
}



#define DATWID 1440
#define DATHEI 720

static WarMap *OpenDatMap(const char *fname){
	FILE *fp;
	TopoMap *ret = NULL;
	int i, j, m, n = 0;
	{
		fp = fopen(fname, "r");
		if(!fp)
			return NULL;
		fprintf(stderr, "file %s opened.\n", fname);
		ret = new TopoMap();
	}
	ret->nalt = 0;
	ret->alt = NULL;
/*	for(i = 0; i < MAPSIZE; i++) for(j = 0; j < MAPSIZE; j++)
		ret->value[i][j] -= g_hgtoffset + g_hgtroundoff * ((i - MAPSIZE / 2) * (i - MAPSIZE / 2) + (j - MAPSIZE / 2) * (j - MAPSIZE / 2)) / (MAPSIZE / 2);*/
#if 0
	for(i = 0; i < DATHEI; i++) for(j = 0; j < DATWID; j++){
		extern struct astrobj earth;
		double rad = earth.rad /* EARTH_RAD */;
		double x = (j - TOPOSIZE / 2.) * 237.04 / TOPOSIZE;
		double z = (i - TOPOSIZE / 2.) * 237.04 / TOPOSIZE;
		double d;
		fscanf(fp, "%lf", &d);
		if(i < TOPOSIZE && j < TOPOSIZE){
			ret->value[i][j] = d + 3000;
			if(!ret->value[i][j])
				ret->value[i][j] = -500;
			ret->value[i][j] += 1000 * (sqrt(rad * rad - x * x - z * z) - rad);
		}
	}
#endif
	{
		double x0, y0, w = 200.;
		x0 = 0, y0 = 0.;
		for(i = 0; i < TOPOSIZE; i++) for(j = 0; j < TOPOSIZE; j++){
			double y = ((double)i / TOPOSIZE - .5) * w, x = ((double)j / TOPOSIZE - .5) * w;
			double sd, d, z, z1;
			sd = (x - x0) * (x - x0) + (y - y0) * (y - y0);
			d = sqrt(sd);
			z = exp(-(d - 3.)*(d - 3.)) + exp(-(d + 3.)*(d + 3.)) * .5;
			z1 = exp(-sd / 16.) * 2. - 1.;
			z += z1 < 0 ? 0 : z1;
			ret->value[i][j] += z * 5000;
		}
	}
	fclose(fp);
	return ret;
}










void WarMap::altterrain(int x0, int y0, int x1, int y1, void altfunc(WarMap *wm, int x, int y, void *hint), void *hint){
	int x, y;
	double avg = 0.;
	short v;
	struct warmap_alt *p;
	alt = (warmap_alt*)realloc(alt, (nalt + 1) * sizeof *alt);
	p = &alt[nalt++];
	p->x0 = x0;
	p->y0 = y0;
	p->x1 = x1;
	p->y1 = y1;
	p->altfunc = altfunc;
	p->hint = hint;
}


/* calculate max value returned by perlin noise */
/*static int vrc_noisebits(int *v){
	int i;
	double persistence = 0.5;
	double f = 1.;
	hgt_noise_base = 0.;
	for(i = 0; i < *v; i++){
		hgt_noise_base += f;
		f *= persistence;
	}
}*/

static int vrc_subpixel(int *v){
	if(*v < 0)
		*v = 0;
	else if(CACHEPIXEL < *v)
		*v = CACHEPIXEL;
	return 0;
}

void HgtCmdInit(){
	CvarAddVRC("g_map_subpixel", &SUBPIXEL, cvar_int, (int(*)(void*))vrc_subpixel);
	CvarAdd("g_map_noise_bits", &NOISEBITS, cvar_int);
	CvarAdd("g_map_noise_amp", &hgt_noise_base, cvar_double);
/*	CvarAddVRC("g_map_noise_bits", &NOISEBITS, cvar_int, vrc_noisebits);*/
}
