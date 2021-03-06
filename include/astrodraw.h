#ifndef ASTRODRAW_H
#define ASTRODRAW_H
#include "astro.h"
#include "Viewer.h"
#include <clib/colseq/color.h>
#include <cpplib/RandomSequence.h>
#include <cpplib/quat.h>
#ifdef _WIN32
#define exit something_meanless
#include <windows.h>
#undef exit
#endif
#include <GL/gl.h>
/*#include <GL/glut.h>*/

void drawIcosaSphere(const Vec3d &org, double radius, const Viewer &vw, const Vec3d &scales = Vec3d(1,1,1), const Quatd &qrot = quat_u);

/*
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
extern double g_star_num;
extern int g_star_cells;
extern double g_star_visible_thresh;
extern double g_star_glow_thresh;
extern int g_invert_hyperspace;*/

EXPORT void drawstarback(const Viewer *vw, const CoordSys *csys, const Astrobj *pe, const Astrobj *sun);


#endif
