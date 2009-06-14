#ifndef SUFDRAW_H
#define SUFDRAW_H
#include "suf.h"
#ifdef _WIN32
#define exit something_meanless
#include <windows.h>
#undef exit
#endif
#include <GL/gl.h>

extern void DrawSUF(const suf_t*, unsigned long flags, struct gldCache *c);
extern void DrawSUFPoly(const suf_t*, int polygon_index, unsigned long flags, struct gldCache *c);
extern void ShadowSUF(const suf_t *, const double l[3], const double n[3], const double pos[3], const double pyr[3], double scale, const double (*pre_trans)[16]);
extern void ShadowDrawSUF(const suf_t *, unsigned long flags, const double light[3], const double normal[3], const double (*trans)[16]);

typedef struct sufdecal_s{
	void (*drawproc)(void*, struct gldCache *, void *global_var);
	void (*freeproc)(void*);
	int np;
	void *p[1];
} sufdecal_t;

typedef struct suftex_s{
	unsigned n;
	struct suftexlist{
		GLuint list; /* OpenGL list indices */
		GLuint tex[2];
		double scale;
	} a[1];
} suftex_t;

extern sufdecal_t *AllocSUFDecal(const suf_t *);
extern void FreeSUFDecal(suf_t *);
extern void AddSUFDecal(sufdecal_t *, sufindex ip, void *);
extern void DecalDrawSUF(const suf_t*, unsigned long flags, struct gldCache *c, const suftex_t *tex, sufdecal_t *, void *global_var);

/* the mechanism to avoid multiple list compilation of the same texture. */
struct suftexcache{
	char *name;
	GLuint list;
	GLuint tex[2]; /* texture names for each multitexture */
};

const struct suftexcache *FindTexCache(const char *name);

extern suftex_t *AllocSUFTex(const suf_t *);
extern suftex_t *AllocSUFTexScales(const suf_t *, const double *scales, int nscales, const char **override_textures, int ntexes);

typedef struct suftexparam_s{
	const BITMAPINFO *bmi;
	GLuint env;
	GLuint magfil;
	int mipmap, alphamap;
} suftexparam_t;

extern unsigned long CacheSUFTex(const char *name, const BITMAPINFO *bmi, int mipmap);
extern unsigned long CacheSUFMTex(const char *name, const suftexparam_t *tex1, const suftexparam_t *tex2);

#endif
