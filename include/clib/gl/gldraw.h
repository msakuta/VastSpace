#ifndef CLIB_GL_GLDRAW_H
#define CLIB_GL_GLDRAW_H
#if _WIN32
#define exit something_meanless
/*#include <GL/glut.h>*/
#undef exit
#include <windows.h>
#endif
#include <GL/gl.h>
#include "../colseq/color.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#define deg_per_rad (360./2./M_PI)

struct gldCache{
	unsigned long valid;
	float matdif[4];
	float matamb[4];
	float matemi[4];
	float matspc[4];
	GLuint texture;
	int texenabled;
};


extern double (*CircleCuts(int n))[2];
extern double (*CircleCutsPartial(int n, int nmax))[2];
extern size_t CircleCutsMemory(void);
extern void gldColor32(COLOR32);
extern void gldTranslate3dv(const double[3]);
extern void gldTranslaten3dv(const double[3]);
extern void gldScaled(double);
extern void gldMultQuat(const double q[4]);
extern void gldPutString(const char *s);
extern void gldPutStringN(const char *s, int n);
extern void gldPutStrokeString(const char *s);
extern void gldPutStrokeStringN(const char *s, int n);
extern void gldPutTextureString(const char *s);
extern void gldPutTextureStringN(const char *s, int n);
extern void gldprintf(const char *format, ...);
extern void gldMarker(const double pos[3]);
extern void gldPolyChar(char c);
extern void gldPolyString(const char *buf);
extern void gldPolyPrintf(const char *format, ...);
extern void gldMaterialfv(GLenum face, GLenum pname, const GLfloat *param, struct gldCache *c);
extern void gldLookatMatrix(double (*mat)[16], const double (*position)[3]);
extern void gldLookatMatrix3(double (*mat)[9], const double (*position)[3]);
extern void gldBegin(void);
extern void gldEnd(void);
extern void gldPoint(const double pos[3], double rad);
extern void gldGlow(double phi, double theta, double arcsin, const GLubyte color[4]);
extern void gldSpriteGlow(const double pos[3], double radius, const GLubyte color[4], const double inverse_rotation[16]);
extern void gldTextureGlow(const double pos[3], double radius, const GLubyte color[4], const double inverse_rotation[16]);
extern void gldPseudoSphere(const double pos[3], double radius, const GLubyte color[4]);
extern void gldBeam(const double view[3], const double start[3], const double end[3], double width);
extern void gldTextureBeam(const double view[3], const double start[3], const double end[3], double width);
extern void gldGradBeam(const double view[3], const double start[3], const double end[3], double width, COLOR32 endcolor);

struct gldBeamsData{
	double pos[4], a[4], b[4];
	int cc;
	COLOR32 col;
	int solid;
	int tex2d;
};

extern void gldBeams(struct gldBeamsData *bd, const double view[3], const double pos[3], double width, COLOR32 endcolor);

#endif
