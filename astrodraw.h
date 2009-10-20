#ifndef ASTRODRAW_H
#define ASTRODRAW_H
#include "astro.h"
#include "viewer.h"
#include <clib/colseq/color.h>
#define exit something_meanless
#include <windows.h>
#undef exit
#include <GL/gl.h>
/*#include <GL/glut.h>*/

#define LIGHT_SPEED 299792.458 /* km/s */
#define RING_CUTS 64





extern struct glcull g_glcull;

void directrot(const double pos[3], const double base[3], amat4_t irot);
void rgb2hsv(double hsv[3], double r, double g, double b);
void hsv2rgb(double rgb[3], const double hsv[3]);
void doppler(double rgb[3], double r, double g, double b, double velo);
void drawPoint(const struct astrobj *a, const Viewer *p, const avec3_t pspos, const avec3_t sunp, const GLubyte color[4]);
void drawpsphere(Astrobj *ps, const Viewer *, COLOR32 col);
void drawShadeSphere(const struct astrobj *ps, const Viewer *p, const avec3_t sunpos, const GLubyte color[4], const GLubyte dark[4]);
void drawTextureSphere(const struct astrobj *a, const Viewer *vw, const avec3_t sunpos, GLfloat mat_diffuse[4], GLfloat mat_ambient[4], GLuint *texlist, const amat4_t texmat, const char *texname);
void drawSphere(const struct astrobj *a, const Viewer *vw, const avec3_t sunpos, GLfloat mat_diffuse[4], GLfloat mat_ambient[4]);

GLuint ProjectSphereMap(const char *name, const BITMAPINFO*);
GLuint ProjectSphereJpg(const char *fname);

void LightOn(void);
void LightOff(void);

void ring_draw(const Viewer *vw, const struct astrobj *a, const avec3_t sunpos, char start, char end, const amat4_t rotation, double thick, double minrad, double maxrad, double t, int relative);

extern double g_star_num;
extern int g_star_cells;
extern double g_star_visible_thresh;
extern double g_star_glow_thresh;
extern int g_invert_hyperspace;

void drawstarback(Viewer *vw, const CoordSys *cs, const Astrobj *landing_planet, const Astrobj *lighting_sun);

#endif
