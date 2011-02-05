#ifndef CLIB_SUF_SUFDRAW_H
#define CLIB_SUF_SUFDRAW_H
#include "suf.h"
#ifdef _WIN32
#define exit something_meanless
#include <windows.h>
#undef exit
#endif
#include <GL/gl.h>

#ifndef CLIB_SUFDRAW_EXPORT
#define CLIB_SUFDRAW_EXPORT
#endif

CLIB_SUFDRAW_EXPORT extern void DrawSUF(const suf_t*, unsigned long flags, struct gldCache *c);
CLIB_SUFDRAW_EXPORT extern void DrawSUFPoly(const suf_t*, int polygon_index, unsigned long flags, struct gldCache *c);
CLIB_SUFDRAW_EXPORT extern void ShadowSUF(const suf_t *, const double l[3], const double n[3], const double pos[3], const double pyr[3], double scale, const double (*pre_trans)[16]);
CLIB_SUFDRAW_EXPORT extern void ShadowDrawSUF(const suf_t *, unsigned long flags, const double light[3], const double normal[3], const double (*trans)[16]);

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

CLIB_SUFDRAW_EXPORT extern sufdecal_t *AllocSUFDecal(const suf_t *);
CLIB_SUFDRAW_EXPORT extern void FreeSUFDecal(sufdecal_t *);
CLIB_SUFDRAW_EXPORT extern void AddSUFDecal(sufdecal_t *, sufindex ip, void *);
CLIB_SUFDRAW_EXPORT extern void DecalDrawSUF(const suf_t*, unsigned long flags, struct gldCache *c, const suftex_t *tex, sufdecal_t *, void *global_var);

/// the mechanism to avoid multiple list compilation of the same texture.
struct suftexcache{
	char *name;
	GLuint list;
	GLuint tex[2]; ///< texture names for each multitexture
};

const struct suftexcache *FindTexCache(const char *name);

CLIB_SUFDRAW_EXPORT extern suftex_t *AllocSUFTex(const suf_t *);
CLIB_SUFDRAW_EXPORT extern suftex_t *AllocSUFTexScales(const suf_t *, const double *scales, int nscales, const char **override_textures, int ntexes);

#define STP_ALPHA      0x1   ///< Enable alpha channel in texture
#define STP_ALPHATEX   0x2   ///< Use texture as alpha map
#define STP_ALPHA_TEST 0x4   ///< Alpha test at 0.5A
#define STP_RGBA32     0x8   ///< Designates the input bitmap is 32bit RGBA. Use it with STP_ALPHA.
#define STP_MIPMAP     0x10  ///< Create mipmaps automatically
#define STP_MASKTEX    0x100 ///< Enable mask texture specified by bmiMask
#define STP_ENV        0x200 ///< Texture env mode
#define STP_MAGFIL     0x400
#define STP_MINFIL     0x800
#define STP_WRAP_S     0x1000
#define STP_WRAP_T     0x2000
#define STP_NORMALMAP  0x4000 ///< Create a normal map from supplied height map
#define STP_TRANSPARENTCOLOR  0x8000 ///< Specify transparent color by suftexparam_t::transparentColor.

/// Parameter set given to CacheSUFTex to indicate how images are loaded as textures.
typedef struct suftexparam_s{
	unsigned long flags; ///< Can contain STP_*. Only specified features are examined; unspecified fields can be uninitialized.
	const BITMAPINFO *bmi; ///< Color bitmap for the texture.
	const BITMAPINFO *bmiMask; ///< Bitmap for alpha channel.
	GLuint env; ///< Texture env mode, passed to glTexEnv.
	GLuint magfil, minfil, wraps, wrapt;
	int mipmap;
	int alphamap;
	sufcoord normalfactor; ///< Amplitude factor of normal map. Examined only if flags & STP_NORMALMAP is true.
	DWORD transparentColor; ///< The pixel that is equal to this color will be assigned 0 alpha value. If the bitmap is color-indexed, the index is compared with this value.
} suftexparam_t;

CLIB_SUFDRAW_EXPORT extern unsigned long CacheSUFTex(const char *name, const BITMAPINFO *bmi, int mipmap);
CLIB_SUFDRAW_EXPORT extern unsigned long CacheSUFMTex(const char *name, const suftexparam_t *tex1, const suftexparam_t *tex2);

#endif
