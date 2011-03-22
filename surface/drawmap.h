#ifndef DRAWMAP_H
#define DRAWMAP_H
#include "WarMap.h"
#include "Viewer.h"
#include <cpplib/vec3.h>
#include <cpplib/gl/cullplus.h>

/*typedef struct warmapdecalelem_s{
	double pos[2];
	struct warmapdecalelem_s *next;
	void *data;
} warmapdecalelem_t;

typedef struct warmapdecal_s{
	void (*drawproc)(warmapdecalelem_t *, warmap_t *, int x, int y, struct gldCache *, void *global_var);
	void (*freeproc)(warmapdecalelem_t *);
	warmapdecalelem_t *frei, *actv;
	warmapdecalelem_t p[1];
} warmapdecal_t;

warmapdecal_t *AllocWarmapDecal(unsigned size);
void FreeWarmapDecal(warmapdecal_t *wmd);
int AddWarmapDecal(warmapdecal_t *, const double (*pos)[2], void *data);
*/

class DrawMapCache;

void drawmap(WarMap *wm, const Viewer &vw, int, double t, GLcull *glc, /*warmapdecal_t *wmd, void *wmd_global,*/ char **ptop, int *checked, DrawMapCache *dmc);
//int DrawMiniMap(warmap_t *wm, const avec3_t pos, double scale, double angle);
//int mapcheck(struct player *p, warmap_t *wm);

/*
extern unsigned shadowtex[SHADOWS]; 
extern avec3_t shadoworg[SHADOWS];
extern double shadowscale[SHADOWS];
*/

extern double g_maprange;
extern int r_maprot;

#endif
