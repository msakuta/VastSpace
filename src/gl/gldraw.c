#include "clib/gl/gldraw.h"
#include "clib/gl/multitex.h"
#include "clib/avec3.h"
#include "clib/aquatrot.h"
#include "clib/amat4.h"
/*#include <GL/glut.h>*/
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <limits.h>

#define GLD_DIF 0x01
#define GLD_AMB 0x02
#define GLD_EMI 0x04
#define GLD_SPC 0x08
#define GLD_SHI 0x10

PFNGLACTIVETEXTUREARBPROC glActiveTextureARB = NULL;
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB = NULL;
PFNGLMULTITEXCOORD1FARBPROC glMultiTexCoord1fARB = NULL;

void MultiTextureInit(){
	glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");
	glMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC)wglGetProcAddress("glMultiTexCoord2fARB");
	glMultiTexCoord1fARB = (PFNGLMULTITEXCOORD1FARBPROC)wglGetProcAddress("glMultiTexCoord1fARB");
}

void gldColor32(COLOR32 col){
	GLubyte ub[4];
	ub[0] = COLOR32R(col);
	ub[1] = COLOR32G(col);
	ub[2] = COLOR32B(col);
	ub[3] = COLOR32A(col);
	glColor4ubv(ub);
}

void gldTranslate3dv(const double v[3]){
	glTranslated(v[0], v[1], v[2]);
}

void gldTranslaten3dv(const double v[3]){
	glTranslated(-v[0], -v[1], -v[2]);
}

void gldScaled(double s){
	glScaled(s, s, s);
}

static void gldQuatToMatrix(amat4_t *mat, const aquat_t q){
	double *m = *mat;
	double x = q[0], y = q[1], z = q[2], w = q[3];
	m[0] = 1. - (2 * q[1] * q[1] + 2 * q[2] * q[2]); m[1] = 2 * q[0] * q[1] + 2 * q[2] * q[3]; m[2] = 2 * q[0] * q[2] - 2 * q[1] * q[3];
	m[4] = 2 * x * y - 2 * z * w; m[5] = 1. - (2 * x * x +  2 * z * z); m[6] = 2 * y * z + 2 * x * w;
	m[8] = 2 * x * z + 2 * y * w; m[9] = 2 * y * z - 2 * x * w; m[10] = 1. - (2 * x * x + 2 * y * y);
	m[3] = m[7] = m[11] = m[12] = m[13] = m[14] = 0.;
	m[15] = 1.;
}

void gldMultQuat(const double q[4]){
	amat4_t mat;
	gldQuatToMatrix(mat, q);
	glMultMatrixd(mat);
}

#if 0 /* Why was it left? */

static const unsigned char font8x10[] = {
255,255,255,255,255,255,255,255,255,255,255,255,247,255,255,255,
239,253,255,255,255,255,255,255,255,227,255,255,247,255,255,201,
239,253,239,227,251,241,247,235,221,253,225,251,247,247,255,219,
227,241,239,253,247,237,235,213,235,241,247,247,247,251,255,203,
237,237,239,243,247,237,237,213,247,237,251,247,247,251,239,219,
229,233,231,239,247,237,237,221,211,237,253,239,247,253,213,139,
235,245,233,243,227,237,222,255,237,237,225,247,247,251,251,175,
255,255,255,255,247,255,255,255,255,255,255,247,247,251,255,175,
255,255,255,255,247,255,255,255,255,255,255,251,247,247,255,159,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,227,255,255,207,255,255,255,255,255,
255,241,227,241,241,241,247,237,237,247,247,237,247,213,237,243,
255,237,237,239,237,239,247,241,237,247,247,235,247,213,237,237,
255,241,237,239,237,225,247,225,237,247,247,231,247,213,237,237,
255,253,237,239,237,237,225,237,227,247,247,235,247,213,237,237,
251,227,227,241,241,243,247,237,239,247,247,237,247,203,227,243,
247,255,239,255,253,255,247,241,239,255,255,239,247,255,255,255,
247,255,239,255,253,255,249,254,239,247,247,239,247,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
239,244,237,227,247,243,247,235,221,247,193,227,254,227,255,128,
239,233,237,253,247,237,235,213,221,247,223,239,253,251,255,255,
239,233,237,253,247,237,221,213,235,247,239,239,251,251,255,255,
227,237,227,243,247,237,221,213,247,247,247,239,247,251,255,255,
237,237,237,239,247,237,221,221,235,235,251,239,239,251,255,255,
237,237,237,239,247,237,221,221,221,221,253,239,223,251,221,255,
227,243,227,241,193,237,221,221,221,221,193,227,191,227,235,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,247,255,
227,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
221,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
160,237,227,243,231,225,239,243,237,227,231,237,225,221,237,243,
170,237,237,237,235,239,239,237,237,247,251,237,239,221,237,237,
162,225,237,237,237,239,239,237,237,247,251,235,239,221,233,237,
186,237,227,239,237,225,227,233,225,247,251,231,239,213,233,237,
166,245,237,239,237,239,239,239,237,247,251,235,239,213,229,237,
221,245,237,237,235,239,239,239,237,247,251,237,239,201,229,237,
227,249,227,243,231,225,225,243,237,227,225,237,239,221,237,243,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,239,255,255,255,255,
227,227,193,227,247,227,227,247,227,227,255,247,253,255,223,247,
221,247,223,221,247,221,221,247,221,253,247,247,243,193,231,255,
205,247,223,253,131,253,221,251,221,253,255,255,239,255,251,247,
213,247,225,227,183,195,195,251,227,225,255,255,223,255,253,247,
217,247,253,253,215,223,223,221,221,221,255,255,239,193,251,251,
221,231,221,221,231,223,223,221,221,221,247,247,243,255,231,221,
227,247,227,227,247,195,227,193,227,227,255,255,253,255,223,221,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,227,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,247,255,255,247,219,229,255,251,247,247,255,239,255,255,191,
255,255,255,235,227,213,219,255,247,251,213,247,247,255,243,223,
255,247,255,193,213,235,213,255,239,253,213,247,247,255,243,239,
255,247,255,235,243,247,231,255,239,253,227,193,255,193,255,247,
255,247,255,235,231,235,219,255,239,253,213,247,255,255,255,251,
255,247,235,193,213,213,219,247,239,253,213,247,255,255,255,253,
255,247,235,235,227,237,231,247,247,251,247,255,255,255,255,254,
255,255,255,255,247,255,255,255,251,247,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
255,190,190,190,190,190,190,190,190,190,190,190,190,190,190,190,
209,190,190,190,190,190,190,190,190,190,190,190,190,190,190,190,
213,190,190,190,190,190,190,190,190,190,190,190,190,190,190,190,
209,190,190,190,190,190,190,190,190,190,190,190,190,190,190,190,
215,190,190,190,190,190,190,190,190,190,190,190,190,190,190,190,
209,190,190,190,190,190,190,190,190,190,190,190,190,190,190,190,
255,190,190,190,190,190,190,190,190,190,190,190,190,190,190,190,
255,190,190,190,190,190,190,190,190,190,190,190,190,190,190,190,
255,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
255,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
255,128,190,190,190,190,190,190,190,190,190,190,190,190,190,190,
137,128,190,190,190,190,190,190,190,190,190,190,190,190,190,190,
171,128,190,190,190,190,190,190,190,190,190,190,190,190,190,190,
171,128,190,190,190,190,190,190,190,190,190,190,190,190,190,190,
255,128,190,190,190,190,190,190,190,190,190,190,190,190,190,190,
179,128,190,190,190,190,190,190,190,190,190,190,190,190,190,190,
171,128,190,190,190,190,190,190,190,190,190,190,190,190,190,190,
155,128,190,190,190,190,190,190,190,190,190,190,190,190,190,190,
};
static int bitmapfontinit = 0;
static unsigned char bitmapfont[256][10];

static void bitmapfont_build(){
	int n;
	if(bitmapfontinit)
		return;
	bitmapfontinit = 1;
	for(n = 0; n < 128; n++){
		int i;
		for(i = 0; i < 10; i++)
			bitmapfont[n][i] = ~font8x10[n % 16 + ((7 - n / 16) * 10 + i) * 16];
	}
}

void gldPutString(const char *s){
#if 0
	static int init = 0;
	static GLuint fonttex = 0;
	GLuint oldtex;
	glGetIntegerv(GL_TEXTURE_2D, &oldtex);
	if(!init){
		int y, x;
		GLubyte buf[80][128];
		init = 1;
		glGenTextures(1, &fonttex);
		glBindTexture(GL_TEXTURE_2D, fonttex);
		for(y = 0; y < 80; y++) for(x = 0; x < 128; x++){
			buf[y][x] = 0x1 & (font8x10[y * 16 + x / 8] >> (7 - (x) % 8)) ? 255 : 0;
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 128, 128, 0, GL_ALPHA, GL_BYTE, buf);
	}
	glBindTexture(GL_TEXTURE_2D, fonttex);
	glBindTexture(GL_TEXTURE_2D, oldtex);
#else
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	bitmapfont_build();
	while(*s){
		glBitmap(8, 10, 0, 0, 8, 0, bitmapfont[*s]);
		s++;
/*		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *s++);*/
	}
#endif
}

void gldPutStringN(const char *s, int n){
	int i = 0;
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	bitmapfont_build();
	while(*s && i++ < n){
		int j;
		unsigned char buf[10];
		for(j = 0; j < 10; j++)
			buf[j] = ~font8x10[*s % 16 + ((7 - *s / 16) * 10 + j) * 16];
		glBitmap(8, 10, 0, 0, 8, 0, buf);
		s++;
/*		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *s++);*/
	}
}

void gldPutStrokeString(const char *s){
/*	gldPutStrokeStringN(s, strlen(s));*/
}

void gldPutStrokeStringN(const char *s, int n){
/*	int i = 0;
	glPushMatrix();
	glScaled(1. / 104.76, 1. / 104.76, 1.);
	while(*s && i++ < n)
		glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, *s++);
	glPopMatrix();
	glTranslatef(n, 0.f, 0.f);*/
}

void gldPutTextureString(const char *s){
	gldPutTextureStringN(s, strlen(s));
}

#define GLDTW 128
#define GLDTH 80

void gldPutTextureStringN(const char *s, int n){
	static int init = 0;
	static GLuint fonttex = 0;
	GLuint oldtex;
	int i;
	glGetIntegerv(GL_TEXTURE_2D, &oldtex);
	if(!init){
		int y, x;
		GLubyte buf[GLDTW][GLDTW][2];
		init = 1;
		glGenTextures(1, &fonttex);
		glBindTexture(GL_TEXTURE_2D, fonttex);
		for(y = 0; y < GLDTW - GLDTH; y++) for(x = 0; x < GLDTW; x++){
			buf[y][x][0] = buf[y][x][1] = 127;
		}
		for(y = 0; y < GLDTH; y++) for(x = 0; x < GLDTW; x++){
			buf[y + GLDTW - GLDTH][x][0] = 255; buf[y + GLDTW - GLDTH][x][1] = (0x1 & (font8x10[y * 16 + x / 8] >> (7 - (x) % 8)) ? 0 : 255);
		}
		glTexImage2D(GL_TEXTURE_2D, 0, 2, GLDTW, GLDTW, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, buf);
	}
	glBindTexture(GL_TEXTURE_2D, fonttex);
	glPushMatrix();
	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR/*/GL_NEAREST*/);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR/*/GL_NEAREST*/);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	{
		GLfloat envc[4] = {0,0,0,0};
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, envc);
	}
	glBegin(GL_QUADS);
	for(i = 0; *s && i < n; i++){
		glTexCoord2d(*s % 16 * 8. / GLDTW, (12 - *s / 16 - .1) * 10. / GLDTW); glVertex2i(i,0);
		glTexCoord2d((*s % 16 + 1) * 8. / GLDTW, (12 - *s / 16 - .1) * 10. / GLDTW); glVertex2i(i+1,0);
		glTexCoord2d((*s % 16 + 1) * 8. / GLDTW, (12 - *s / 16 + .7) * 10. / GLDTW); glVertex2i(i+1,1);
		glTexCoord2d(*s % 16 * 8. / GLDTW, (12 - *s / 16 + .7) * 10. / GLDTW); glVertex2i(i,1);
		s++;
	}
	glEnd();
/*	glLoadIdentity();
	glBegin(GL_QUADS);
		glTexCoord2d(0, 0); glVertex2i(0,0);
		glTexCoord2d(1, 0); glVertex2i(+1,0);
		glTexCoord2d(1, 1); glVertex2i(+1,1);
		glTexCoord2d(0, 1); glVertex2i(0,1);
	glEnd();*/
	glPopAttrib();
	glPopMatrix();
	glTranslatef(n, 0.f, 0.f);
	glBindTexture(GL_TEXTURE_2D, oldtex);
}

void gldprintf(const char *f, ...){
	static char buf[512]; /* it's not safe but unlikely to be reached */
	va_list ap;
	va_start(ap, f);

	/* Unluckily, snprintf is not part of the standard. */
#ifdef _WIN32
	_vsnprintf(buf, sizeof buf, f, ap);
#else
	vsprintf(buf, f, ap);
#endif

	gldPutString(buf);
	va_end(ap);
}

#endif

void gldMarker(const double pos[3]){
/*	if(pos)
		glRasterPos3dv(pos);
	glutBitmapCharacter(GLUT_BITMAP_8_BY_13, 'o');*/
}

void gldPolyChar(char c){
#if 0
	glutStrokeCharacter(GLUT_STROKE_ROMAN, c);
#else
	switch(c){
		case '.':
			glBegin(GL_POINTS);
			glVertex2d(.0, -.8);
			glEnd();
			break;
		case '-':
			glBegin(GL_LINES);
			glVertex2d(-.8, .0); glVertex2d(.8, .0);
			glEnd();
			break;
		case '0':
			glBegin(GL_LINES);
			glVertex2d(.0, -.8); glVertex2d(.6, -.5);
			glVertex2d(.6, -.5); glVertex2d(.6, .5);
			glVertex2d(.6, .5); glVertex2d(.0, .8);
			glVertex2d(.0, .8); glVertex2d(-.6, .5);
			glVertex2d(-.6, .5); glVertex2d(-.6, -.5);
			glVertex2d(-.6, -.5); glVertex2d(.0, -.8);
			glEnd();
			break;
		case '1':
			glBegin(GL_LINE_STRIP);
			glVertex2d(.0, -.8);
			glVertex2d(.0, .8);
			glVertex2d(-.2, .6);
			glEnd();
			break;
		case '2':
			glBegin(GL_LINE_STRIP);
			glVertex2d(-.8, .5);
			glVertex2d(-.5, .8);
			glVertex2d(.5, .8);
			glVertex2d(.8, .5);
			glVertex2d(.8, .3);
			glVertex2d(-.8, -.4);
			glVertex2d(-.8, -.8);
			glVertex2d(.8, -.8);
			glEnd();
			break;
		case '3':
			glBegin(GL_LINE_STRIP);
			glVertex2d(-.6, .6);
			glVertex2d(-.4, .8);
			glVertex2d(.4, .8);
			glVertex2d(.6, .6);
			glVertex2d(.6, .2);
			glVertex2d(.3, .0);
			glVertex2d(.6, -.2);
			glVertex2d(.6, -.6);
			glVertex2d(.4, -.8);
			glVertex2d(-.4, -.8);
			glVertex2d(-.6, -.6);
			glEnd();
			glBegin(GL_LINES);
			glVertex2d(.3, .0);
			glVertex2d(.0, .0);
			glEnd();
			break;
		case '4':
			glBegin(GL_LINE_STRIP);
			glVertex2d(.0, -.8);
			glVertex2d(.0, .8);
			glVertex2d(-.6, .0);
			glVertex2d(.6, .0);
			glEnd();
			break;
		case '5':
			glBegin(GL_LINE_STRIP);
			glVertex2d(.6, .8);
			glVertex2d(-.6, .8);
			glVertex2d(-.6, .0);
			glVertex2d(.4, .0);
			glVertex2d(.6, -.2);
			glVertex2d(.6, -.6);
			glVertex2d(.4, -.8);
			glVertex2d(-.6, -.8);
			glEnd();
			break;
		case '6':
			glBegin(GL_LINE_STRIP);
			glVertex2d(.8, .6);
			glVertex2d(.6, .8);
			glVertex2d(-.6, .8);
			glVertex2d(-.8, .6);
			glVertex2d(-.8, -.6);
			glVertex2d(-.6, -.8);
			glVertex2d(.6, -.8);
			glVertex2d(.8, -.6);
			glVertex2d(.8, -.2);
			glVertex2d(.6, .0);
			glVertex2d(-.8, .0);
			glEnd();
			break;
		case '7':
			glBegin(GL_LINE_STRIP);
			glVertex2d(-.7, .8);
			glVertex2d(.7, .8);
			glVertex2d(.0, -.8);
			glEnd();
			break;
		case '8':
			glBegin(GL_LINE_LOOP);
			glVertex2d(-.6, .6);
			glVertex2d(-.4, .8);
			glVertex2d(.4, .8);
			glVertex2d(.6, .6);
			glVertex2d(.6, .2);
			glVertex2d(.4, .0);
			glVertex2d(.6, -.2);
			glVertex2d(.6, -.6);
			glVertex2d(.4, -.8);
			glVertex2d(-.4, -.8);
			glVertex2d(-.6, -.6);
			glVertex2d(-.6, -.2);
			glVertex2d(-.4, .0);
			glVertex2d(-.6, .2);
			glEnd();
			glBegin(GL_LINES);
			glVertex2d(.4, .0);
			glVertex2d(-.4, .0);
			glEnd();
			break;
		case '9':
			glBegin(GL_LINE_STRIP);
			glVertex2d(.6, .2);
			glVertex2d(.4, .0);
			glVertex2d(-.4, .0);
			glVertex2d(-.6, .2);
			glVertex2d(-.6, .6);
			glVertex2d(-.4, .8);
			glVertex2d(.4, .8);
			glVertex2d(.6, .6);
			glVertex2d(.6, -.6);
			glVertex2d(.4, -.8);
			glVertex2d(-.6, -.8);
			glEnd();
			break;
		case 'M':
			glBegin(GL_LINE_STRIP);
			glVertex2d(-.8, -.8);
			glVertex2d(-.8, .8);
			glVertex2d(.0, .0);
			glVertex2d(.8, .8);
			glVertex2d(.8, -.8);
			glEnd();
			break;
	}
#endif
}

void gldPolyString(const char *buf){
	int j;
	for(j = 0; buf[j]; j++){
		gldPolyChar(buf[j]);
		glTranslated(2., 0., 0.);
	}
}

void gldPolyPrintf(const char *format, ...){
	static char buf[256]; /* it's not safe but unlikely to be reached */
	va_list ap;
	va_start(ap, format);
	vsprintf(buf, format, ap);
	gldPolyString(buf);
	va_end(ap);
}




void gldMaterialfv(GLenum face, GLenum pname, const GLfloat *params, struct gldCache *c){
	unsigned long flag;
	float *dst;
	size_t dstsize;
	if(!c){
		glMaterialfv(face, pname, params);
		return;
	}
	switch(pname){
		case GL_DIFFUSE:
			flag = GLD_DIF;
			dst = c->matdif;
			dstsize = sizeof c->matdif;
			break;
		case GL_AMBIENT:
			flag = GLD_AMB;
			dst = c->matamb;
			dstsize = sizeof c->matamb;
			break;
		case GL_EMISSION:
			flag = GLD_EMI;
			dst = c->matemi;
			dstsize = sizeof c->matemi;
			break;
		case GL_SPECULAR:
			flag = GLD_SPC;
			dst = c->matspc;
			dstsize = sizeof c->matspc;
			break;
		default:
			dst = NULL;
	}
	if(dst){
		if(c->valid & flag && !memcmp(params, dst, dstsize))
			return;
		c->valid |= flag;
		memcpy(dst, params, dstsize);
	}
	glMaterialfv(face, pname, params);
}



void gldLookatMatrix(double (*mat)[16], const double (*pos)[3]){
	avec3_t dr, v;
	aquat_t q;
	double p;
	VECNORM(dr, *pos);

	/* half-angle formula of trigonometry replaces expensive tri-functions to square root */
	q[3] = sqrt((dr[2] + 1.) / 2.) /*cos(acos(dr[2]) / 2.)*/;

	VECVP(v, avec3_001, dr);
	p = sqrt(1. - q[3] * q[3]) / VECLEN(v);
	VECSCALE(q, v, p);
	quat2mat(*mat, q);
}




/* the purpose of start/end functions are that glPushAttrib and glPopAttrib can be 
  loading heavy in some machines (typically mine) so storing attributes in application
  memory space is expected to improve speed. */
/* generally having variables global causes problems in multithreaded environment, but
  OpenGL state machine is also thread-critical from the first place. */
static int gldbegun = 0;
static int gl_point_smooth = 0, old_gl_point_smooth = 0;
static GLfloat gl_pointsize = 1, old_gl_pointsize = 1;

void gldBegin(void){
	gldbegun++;
	old_gl_point_smooth = gl_point_smooth = glIsEnabled(GL_POINT_SMOOTH);
	glGetFloatv(GL_POINT_SIZE, &old_gl_pointsize);
	gl_pointsize = old_gl_pointsize;
}

void gldEnd(void){
	assert(gldbegun);
	gldbegun--;
	if(gl_point_smooth != old_gl_point_smooth)
		(old_gl_point_smooth ? glEnable : glDisable)(GL_POINT_SMOOTH);
	if(gl_pointsize != old_gl_pointsize)
		glPointSize(old_gl_pointsize);
}

void gldPoint(const double pos[3], double rad){
	int xywh[4], m;
	GLfloat size;
	glGetIntegerv(GL_VIEWPORT, xywh);
	m = xywh[2] < xywh[3] ? xywh[3] : xywh[2];
	size = rad * m / 2.;
	if(2 < size){
/*		if(!gl_point_smooth){
			gl_point_smooth = 1;
			glEnable(GL_POINT_SMOOTH);
		}*/
	}
	else{
/*		if(!size){
			GLfloat col[4];
			glPushAttrib(GL_POINT_BIT | GL_COLOR_BUFFER_BIT);
			glGetFloatv(GL_CURRENT_COLOR, col);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			col[3] *= rad * m / 2.;
			glColor4fv(col);
		}
		else*/
		if(!gldbegun || gl_point_smooth){
			gl_point_smooth = 0;
			glDisable(GL_POINT_SMOOTH);
		}
	}
	if(!size) size = 1;
	if(!gldbegun || gl_pointsize != size)
		glPointSize(gl_pointsize = size);
	glBegin(GL_POINTS);
	glVertex3dv(pos);
	glEnd();
}

void gldGlow(double phi, double theta, double as, const GLubyte color[4]){
	int i;
	double sas, cas;
	double (*cuts)[2];
	GLubyte edgecolor[4];
	sas = sin(as);
	cas = cos(as);
	cuts = CircleCuts(10);

	glPushMatrix();
	glRotated(phi * 360 / 2. / M_PI, 0., 1., 0.);
	glRotated(theta * 360 / 2. / M_PI, -1., 0., 0.);
	glBegin(GL_TRIANGLE_FAN);
	glColor4ubv(color);
	glVertex3d(0., 0., 1.);
	edgecolor[0] = color[0];
	edgecolor[1] = color[1];
	edgecolor[2] = color[2];
	edgecolor[3] = 0;
	glColor4ubv(edgecolor);
	for(i = 0; i <= 10; i++){
		int k = i % 10;
		glVertex3d(cuts[k][0] * sas, cuts[k][1] * sas, cas);
	}
	glEnd();
	glPopMatrix();
}

void gldSpriteGlow(const double pos[3], double radius, const GLubyte color[4], const double irot[16]){
	int i;
	double (*cuts)[2];
	GLubyte edgecolor[4];
	cuts = CircleCuts(10);
	glPushMatrix();
	glTranslated(pos[0], pos[1], pos[2]);
	if(irot)
		glMultMatrixd(irot);
	glScaled(radius, radius, radius);
	glBegin(GL_TRIANGLE_FAN);
	glColor4ubv(color);
	glVertex3d(0., 0., 0.);
	edgecolor[0] = color[0];
	edgecolor[1] = color[1];
	edgecolor[2] = color[2];
	edgecolor[3] = 0;
	glColor4ubv(edgecolor);
	for(i = 0; i <= 10; i++){
		int k = i % 10;
		glVertex3d(cuts[k][1], cuts[k][0], 0.);
	}
	glEnd();
	glPopMatrix();
}

#define TEXSIZE 256
void gldTextureGlow(const double pos[3], double radius, const GLubyte color[4], const double irot[16]){
	static int init = 0;
	static GLuint list, texname;
	GLfloat col[4];
	if(!init){
		GLubyte tex[TEXSIZE][TEXSIZE][2];
		int i, j;
		for(i = 0; i < TEXSIZE; i++) for(j = 0; j < TEXSIZE; j++){
			int di = i - TEXSIZE / 2, dj = j - TEXSIZE / 2;
			int sdist = di * di + dj * dj;
			tex[i][j][0] = 255;
			tex[i][j][1] = TEXSIZE * TEXSIZE / 2 / 2 <= sdist ? 0 : 255 - 255 * pow((double)(di * di + dj * dj) / (TEXSIZE * TEXSIZE / 2 / 2), 1. / 8.);
		}
		glGenTextures(1, &texname);
		glBindTexture(GL_TEXTURE_2D, texname);
		glTexImage2D(GL_TEXTURE_2D, 0, 2, TEXSIZE, TEXSIZE, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, tex);
		glNewList(list = glGenLists(1), GL_COMPILE);
		glPushAttrib(GL_TEXTURE_BIT | GL_PIXEL_MODE_BIT);
		glBindTexture(GL_TEXTURE_2D, texname);
	/*	glPixelTransferf(GL_RED_BIAS, color[0] / 256.F);
		glPixelTransferf(GL_GREEN_BIAS, color[1] / 256.F);
		glPixelTransferf(GL_BLUE_BIAS, color[2] / 256.F);*/
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
			GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
			GL_NEAREST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glEndList();
		init = 1;
	}
	glCallList(list);
	col[0] = color[0] / 256.F;
	col[1] = color[1] / 256.F;
	col[2] = color[2] / 256.F;
	col[3] = color[3] / 256.F;
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, col);
	glPushMatrix();
	glTranslated(pos[0], pos[1], pos[2]);
	glMultMatrixd(irot);
	glScaled(radius, radius, radius);
	glBegin(GL_QUADS);
	glTexCoord2d(0., 0.); glVertex2d(-1., -1.);
	glTexCoord2d(1., 0.); glVertex2d( 1., -1.);
	glTexCoord2d(1., 1.); glVertex2d( 1.,  1.);
	glTexCoord2d(0., 1.); glVertex2d(-1.,  1.);
	glEnd();
	glPopMatrix();
	glPopAttrib();
}

void gldPseudoSphere(const double pos[3], double rad, const GLubyte color[4]){
	int i;
	double sdist;
	sdist = VECSLEN(pos);
	if(10000. < sdist / rad / rad){
		double dist;
		int vp[4];
		dist = sqrt(sdist);
		glGetIntegerv(GL_VIEWPORT, vp);
		glPushAttrib(GL_POINT_BIT);
		glPointSize(vp[2] * rad / dist);
		glColor4ubv(color);
		glBegin(GL_POINTS);
		glVertex3d(pos[0] / dist, pos[1] / dist, pos[2] / dist);
		glEnd();
		glPopAttrib();
	}
	else if(rad * rad < sdist){
		int n;
		double dist, as, cas, sas;
		double (*cuts)[2];
		double x = pos[0], z = pos[2], phi, theta;
		dist = sqrt(sdist);
		as = asin(sas = rad / dist);
		cas = cos(as);
		n = (int)(16*(-(as - M_PI / 4.) * (as - M_PI / 4.) / M_PI * 4. / M_PI * 4. + 1.)) + 5;
		phi = atan2(x, z);
		theta = atan2(pos[1], sqrt(x * x + z * z));
		cuts = CircleCuts(n);
		glPushMatrix();
		glRotated(phi * 360 / 2. / M_PI, 0., 1., 0.);
		glRotated(theta * 360 / 2. / M_PI, -1., 0., 0.);
		glColor4ubv(color);
		glBegin(GL_POLYGON);
		for(i = 0; i < n; i++)
			glVertex3d(cuts[i][0] * sas, cuts[i][1] * sas, cas);
		glEnd();
		glPopMatrix();
	}
}

void gldBeam(const double view[3], const double start[3], const double end[3], double width){
	double dx, dy, dz, sx, sy, len;
	double pitch, yaw;
	avec3_t diff, xh, yh;
	dy = end[1] - start[1];
	yaw = atan2(dx = end[0] - start[0], dz = end[2] - start[2]) + M_PI;
	pitch = -atan2(dy, sqrt(dx * dx + dz * dz)) + M_PI / 2;
	len = sqrt(dx * dx + dy * dy + dz * dz);
	xh[0] = -cos(yaw);
	xh[1] = 0;
	xh[2] = sin(yaw);
	yh[0] = cos(pitch) * sin(yaw);
	yh[1] = sin(pitch);
	yh[2] = cos(pitch) * (cos(yaw));
	VECSUB(diff, view, start);
/*
	glBegin(GL_LINES);
	glVertex3dv(start);
	glVertex3dv(end);
	glEnd();
*/
	glPushMatrix();
	glTranslated(start[0], start[1], start[2]);
/*
	glBegin(GL_LINES);
	glVertex3d(0,0,0);
	glVertex3dv(xh);
	glVertex3d(0,0,0);
	glVertex3dv(yh);
	glEnd();
*/
	sy = VECSP(diff, yh);
	sx = VECSP(diff, xh);
	glRotated(yaw * 360 / 2. / M_PI, 0., 1., 0.);
	glRotated(pitch * 360 / 2. / M_PI, -1., 0., 0.);
	glRotated(atan2(sx, sy) * 360 / 2 / M_PI, 0., -1., 0.);
	glScaled(width, len, 1.);
	{
		GLint cc[4], oc[4];
		glGetIntegerv(GL_CURRENT_COLOR, cc);
		memcpy(oc, cc, sizeof oc);
		oc[3] = INT_MIN;
		glBegin(GL_QUAD_STRIP);
		glColor4iv(oc);
		glVertex3d(-1., 0., 0.);
		glVertex3d(-1., 1., 0.);
		glColor4iv(cc);
		glVertex3d( 0., 0., 0.);
		glVertex3d( 0., 1., 0.);
		glColor4iv(oc);
		glVertex3d( 1., 0., 0.);
		glVertex3d( 1., 1., 0.);
		glEnd();
	}
	glPopMatrix();
}

void gldTextureBeam(const double view[3], const double start[3], const double end[3], double width){
	double dx, dy, dz, sx, sy, len;
	double pitch, yaw;
	avec3_t diff, xh, yh;
	dy = end[1] - start[1];
	yaw = atan2(dx = end[0] - start[0], dz = end[2] - start[2]) + M_PI;
	pitch = -atan2(dy, sqrt(dx * dx + dz * dz)) + M_PI / 2;
	len = sqrt(dx * dx + dy * dy + dz * dz);
	xh[0] = -cos(yaw);
	xh[1] = 0;
	xh[2] = sin(yaw);
	yh[0] = cos(pitch) * sin(yaw);
	yh[1] = sin(pitch);
	yh[2] = cos(pitch) * (cos(yaw));
	VECSUB(diff, view, start);
	glPushMatrix();
	glTranslated(start[0], start[1], start[2]);
	sy = VECSP(diff, yh);
	sx = VECSP(diff, xh);
	glRotated(yaw * 360 / 2. / M_PI, 0., 1., 0.);
	glRotated(pitch * 360 / 2. / M_PI, -1., 0., 0.);
	glRotated(atan2(sx, sy) * 360 / 2 / M_PI, 0., -1., 0.);
	glScaled(width, len, 1.);
	glBegin(GL_QUADS);
	glTexCoord2d(0., 0.); glVertex3d(-1., 0., 0.);
	glTexCoord2d(0., 1.); glVertex3d(-1., 1., 0.);
	glTexCoord2d(1., 1.); glVertex3d( 1., 1., 0.);
	glTexCoord2d(1., 0.); glVertex3d( 1., 0., 0.);
	glEnd();
	glPopMatrix();
}

void gldGradBeam(const double view[3], const double start[3], const double end[3], double width, COLOR32 endc){
	double dx, dy, dz, sx, sy, len;
	double pitch, yaw;
	avec3_t diff, xh, yh;
	static int invokes = 0;
	invokes++;
	dy = end[1] - start[1];
	yaw = atan2(dx = end[0] - start[0], dz = end[2] - start[2]) + M_PI;
	pitch = -atan2(dy, sqrt(dx * dx + dz * dz)) + M_PI / 2;
	len = sqrt(dx * dx + dy * dy + dz * dz);
	xh[0] = -cos(yaw);
	xh[1] = 0;
	xh[2] = sin(yaw);
	yh[0] = cos(pitch) * sin(yaw);
	yh[1] = sin(pitch);
	yh[2] = cos(pitch) * (cos(yaw));
	VECSUB(diff, view, start);
/*
	glBegin(GL_LINES);
	glVertex3dv(start);
	glVertex3dv(end);
	glEnd();
*/
	glPushMatrix();
	glTranslated(start[0], start[1], start[2]);
/*
	glBegin(GL_LINES);
	glVertex3d(0,0,0);
	glVertex3dv(xh);
	glVertex3d(0,0,0);
	glVertex3dv(yh);
	glEnd();
*/
	sy = VECSP(diff, yh);
	sx = VECSP(diff, xh);
	glRotated(yaw * 360 / 2. / M_PI, 0., 1., 0.);
	glRotated(pitch * 360 / 2. / M_PI, -1., 0., 0.);
	glRotated(atan2(sx, sy) * 360 / 2 / M_PI, 0., -1., 0.);
	glScaled(width, len, 1.);
	{
		GLint cc[4], oc[4];
		GLuint eoc = endc & ~COLOR32RGBA(0,0,0,255);
		glGetIntegerv(GL_CURRENT_COLOR, cc);
		memcpy(oc, cc, sizeof oc);
		oc[3] = INT_MIN;
		glBegin(GL_QUAD_STRIP);
		glColor4iv(oc);
		glVertex3d(-1., 0., 0.);
		gldColor32(eoc);
		glVertex3d(-1., 1., 0.);
		glColor4iv(cc);
		glVertex3d( 0., 0., 0.);
		gldColor32(endc);
		glVertex3d( 0., 1., 0.);
		glColor4iv(oc);
		glVertex3d( 1., 0., 0.);
		gldColor32(eoc);
		glVertex3d( 1., 1., 0.);
		glEnd();
	}
	glPopMatrix();
}

void gldBeams(struct gldBeamsData *bd, const double view[3], const double end[3], double width, COLOR32 endc){
	double dx, dy, dz, sx, sy, len;
	double pitch, yaw;
	avec3_t diff, xh, yh;
/*
	glBegin(GL_LINES);
	glVertex3dv(start);
	glVertex3dv(end);
	glEnd();
*/
	glPushMatrix();
	glLoadIdentity();
	glTranslated(end[0], end[1], end[2]);
/*
	glBegin(GL_LINES);
	glVertex3d(0,0,0);
	glVertex3dv(xh);
	glVertex3d(0,0,0);
	glVertex3dv(yh);
	glEnd();
*/
	if(bd->cc){
		double *start = bd->pos;
		dy = end[1] - start[1];
		yaw = atan2(dx = end[0] - start[0], dz = end[2] - start[2]) + M_PI;
		pitch = -atan2(dy, sqrt(dx * dx + dz * dz)) + M_PI / 2;
		len = sqrt(dx * dx + dy * dy + dz * dz);
		xh[0] = -cos(yaw);
		xh[1] = 0;
		xh[2] = sin(yaw);
		yh[0] = cos(pitch) * sin(yaw);
		yh[1] = sin(pitch);
		yh[2] = cos(pitch) * (cos(yaw));
		VECSUB(diff, view, start);
		sy = VECSP(diff, yh);
		sx = VECSP(diff, xh);
		glRotated(yaw * 360 / 2. / M_PI, 0., 1., 0.);
		glRotated(pitch * 360 / 2. / M_PI, -1., 0., 0.);
		glRotated(atan2(sx, sy) * 360 / 2 / M_PI, 0., -1., 0.);
		glScaled(width, len, 1.);
	}
	{
		GLdouble mat[16];
		avec3_t nullvec3 = {0}, onevec3 = {0., 1., 0.};
		glGetDoublev(GL_MODELVIEW_MATRIX, mat);
		if(!bd->cc){
			avec3_t st;
			MAT4VP3(st, mat, nullvec3);
			VECCPY(bd->pos, st);
			bd->a[0] = width; /* temporarily put the width value here */
		}
		else{
			COLOR32 cc, oc;
			GLuint eoc = bd->solid ? endc | COLOR32RGBA(0,0,0,255) : endc & ~COLOR32RGBA(0,0,0,255);
			avec3_t ea = {-1., 0., 0.}, eb = {1., 0., 0.};
			avec3_t a, b;
			avec3_t et;
			MAT4VP3(a, mat, ea);
			MAT4VP3(b, mat, eb);
			VECCPY(et, end);
			cc = bd->col;
			oc = bd->solid ? cc | COLOR32RGBA(0,0,0,255) : cc & ~COLOR32RGBA(0,0,0,255);
			glPopMatrix();
			glPushMatrix();
#if 0
			glBegin(GL_LINES);
			glColor3ub(0, 255, 0);
			glVertex4dv(bd->pos);
			glVertex4dv(et);
			if(1 < bd->cc){
				glVertex4dv(bd->pos);
				glVertex4dv(bd->a);
				glVertex4dv(bd->pos);
				glVertex4dv(bd->b);
			}
			glEnd();
#endif
			glBegin(GL_TRIANGLE_STRIP);
			gldColor32(oc);
			if(bd->tex2d)
				glTexCoord2d(bd->tex2d, 0.);
			else
				glTexCoord1d(1.);
			if(1 < bd->cc)
				glVertex3dv(bd->a);
			else{
				avec3_t sa = {-1., -1., 0.};
				avec3_t vt;
				sa[0] *= bd->a[0] / width;
				MAT4VP3(vt, mat, sa);
				glVertex3dv(vt);
			}
			gldColor32(eoc);
			if(bd->tex2d)
				glTexCoord2d(0., 0.);
			glVertex3dv(a);
			gldColor32(cc);
			if(bd->tex2d)
				glTexCoord2d(bd->tex2d, .5);
			else
				glTexCoord1d(0.);
			glVertex3dv(bd->pos);
			gldColor32(endc);
			if(bd->tex2d)
				glTexCoord2d(0., .5);
			glVertex3dv(et);
			gldColor32(oc);
			if(bd->tex2d)
				glTexCoord2d(bd->tex2d, 1.);
			else
				glTexCoord1d(1.);
			if(1 < bd->cc)
				glVertex3dv(bd->b);
			else{
				avec3_t sb = {1., -1., 0.};
				avec3_t vt;
				sb[0] *= bd->a[0] / width;
				MAT4VP3(vt, mat, sb);
				glVertex3dv(vt);
			}
			gldColor32(eoc);
			if(bd->tex2d)
				glTexCoord2d(0., 1.);
			glVertex3dv(b);
			glEnd();
			VECCPY(bd->a, a);
			VECCPY(bd->b, b);
			VECCPY(bd->pos, et);
		}
		bd->cc++;
	}
	glPopMatrix();
	bd->col = endc;
}

#define MAX_CIRCUT_ARRAY 256

/* Circle cuts
    Binary Tree vs. Pointer Array
  Binary Tree is fairly fast, but it gets slower when plenty of entries are added by O(log(N)).
  For very frequent usage, array indexing can be faster I think.
  But arrays can grow really large in memory, which is expected to be somewhat sparse.
  So I combined those algorithms.
  Entries less than MAX_CIRCUT_ARRAY is indexed in the array, otherwise searched from the binary tree.
 */
typedef struct circut{
	int n, m;
	struct circut *lo, *hi;
	double d[1][2];
} circut;
static circut **circut_array = NULL, *circut_root = NULL;
static int circut_num = 0;

double (*CircleCuts(int n))[2]{
	circut **node;
	if(n <= 0) return NULL;
	if(n < MAX_CIRCUT_ARRAY){
		if(circut_num <= n){
			circut_array = realloc(circut_array, (n + 1) * sizeof *circut_array);
			memset(&circut_array[circut_num], 0, (&circut_array[n + 1] - &circut_array[circut_num]) * sizeof *circut_array);
			circut_num = n + 1;
		}
		node = &circut_array[n];
	}
	else for(node = &circut_root; *node && (*node)->n != n; node = n < (*node)->n ? &(*node)->hi : &(*node)->lo);
	if(!*node){
		int i;
		*node = malloc(offsetof(circut, d) + n * sizeof *(*node)->d);
		(*node)->n = (*node)->m = n;
		(*node)->hi = (*node)->lo = NULL;
		for(i = 0; i < n; i++){
			(*node)->d[i][0] = sin(i * 2. * M_PI / n);
			(*node)->d[i][1] = cos(i * 2. * M_PI / n);
		}
	}
	else if((*node)->m < n){
		int i;
		*node = realloc(*node, offsetof(circut, d) + n * sizeof *(*node)->d);
		(*node)->n = (*node)->m = n;
/*		(*node)->hi = (*node)->lo = NULL;*/
		for(i = 0; i < n; i++){
			(*node)->d[i][0] = sin(i * 2. * M_PI / n);
			(*node)->d[i][1] = cos(i * 2. * M_PI / n);
		}
	}
	return (*node)->d;
}

double (*CircleCutsPartial(int n, int nmax))[2]{
	static circut *root = NULL;
	circut **node;
	if(n <= 0 || n < nmax) return NULL;
	if(n < MAX_CIRCUT_ARRAY){
		if(circut_num <= n){
			circut_array = realloc(circut_array, (n + 1) * sizeof *circut_array);
			memset(&circut_array[circut_num], 0, (&circut_array[n + 1] - &circut_array[circut_num]) * sizeof *circut_array);
			circut_num = n + 1;
		}
		node = &circut_array[n];
	}
	else for(node = &circut_root; *node && (*node)->n != n; node = n < (*node)->n ? &(*node)->hi : &(*node)->lo);
	if(!*node){
		int i;
		*node = malloc(offsetof(circut, d) + nmax * sizeof *(*node)->d);
		(*node)->n = n;
		(*node)->m = nmax;
		(*node)->hi = (*node)->lo = NULL;
		for(i = 0; i < nmax; i++){
			(*node)->d[i][0] = sin(i * 2. * M_PI / n);
			(*node)->d[i][1] = cos(i * 2. * M_PI / n);
		}
	}
	else if((*node)->m < nmax){
		int i;
		*node = realloc(*node, offsetof(circut, d) + nmax * sizeof *(*node)->d);
		(*node)->n = n;
		(*node)->m = nmax;
/*		(*node)->hi = (*node)->lo = NULL;*/
		for(i = 0; i < nmax; i++){
			(*node)->d[i][0] = sin(i * 2. * M_PI / n);
			(*node)->d[i][1] = cos(i * 2. * M_PI / n);
		}
	}
	return (*node)->d;
}

static size_t circuts_memory(circut *root){
	size_t ret;
	if(!root)
		return 0;
	ret = offsetof(circut, d) + root->m * sizeof *root->d;
	ret += circuts_memory(root->lo);
	ret += circuts_memory(root->hi);
	return ret;
}

size_t CircleCutsMemory(void){
	int i;
	size_t ret;
	circut **node;
	ret = circut_num * sizeof *circut_array;
	for(i = 0; i < circut_num; i++) if(circut_array[i])
		ret += offsetof(circut, d) + circut_array[i]->m * sizeof *circut_array[i]->d;
	ret += circuts_memory(circut_root);
	return ret;
}
