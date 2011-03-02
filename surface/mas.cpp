/** \file
 * \brief mas.c - Implementation of class Mas and bunch of other similar classes. */

#include "WarMap.h"
extern "C"{
#include <clib/c.h>
#include <clib/rseq.h>
#include <clib/timemeas.h>
}
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <exception>


class Mas : public WarMap{
public:
	virtual int getat(WarMapTile *, int x, int y);
	virtual void size(int *x, int *y);
	virtual double width();
	virtual void levelterrain(int x0, int y0, int x1, int y1);
//	virtual int linehit(const Vec3d &src, const Vec3d &dir, double dt, Vec3d &ret);
	int sx, sy;
	WarMapTile *value;
	Mas(int sx, int sy) : sx(sx), sy(sy){
		value = new WarMapTile[sx * sy];
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


int Mas::getat(WarMapTile *wt, int x, int y){
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
		WarMapTile &t = value[(sx <= x ? sx-1 : x) + (sy <= y ? sy-1 : y) * sx];
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
	int getat(WarMapTile *, int x, int y);
	void size(int *x, int *y);
	double width();
};

WarMap *CreateMemap(void){
	return new MemoryMap();
}

int MemoryMap::getat(WarMapTile *wt, int x, int y){
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
	int getat(WarMapTile *, int x, int y);
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

int RepeatMap::getat(WarMapTile *wt, int x, int y){
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

