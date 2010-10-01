#include "clib/cfloat.h"
#include "clib/suf/suf.h"
#include "clib/suf/sufdraw.h"
#include "clib/gl/gldraw.h"
#include "clib/gl/multitex.h"
#include "clib/timemeas.h"
#include <assert.h>
#include <limits.h>
/*#include <GL/glut.h>*/
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <stddef.h>
#include "clib/avec3.h"
#ifdef _WIN32
#define exit something_meanless
#include <windows.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979
#endif

#ifndef numof
#define numof(a) (sizeof(a)/sizeof*(a))
#endif

#ifndef ABS
#define ABS(a) ((a)<0?-(a):(a))
#endif

#define GLD_TEX 0x20


static polydraw(const suf_t *suf, unsigned long flags, struct gldCache *c, int i, sufindex *plast, const suftex_t *);



void DrawSUF(const suf_t *suf, unsigned long flags, struct gldCache *c){
	int i;
	sufindex last = SUFINDEX_MAX, ai = SUFINDEX_MAX;
	assert(suf);
	for(i = 0; i < suf->np; i++){
		struct suf_poly_t *p = &suf->p[i]->p;
		struct suf_atr_t *atr = &suf->a[p->atr];

		/* the effective use of bitfields determines which material commands are
		  only needed. */
		if(flags && (SUF_EMI | atr->valid) & flags && ai != p->atr){
			static const GLfloat defemi[4] = {0., 0., 0., 1.};
/*			if(ai == USHRT_MAX)
				glPushAttrib(GL_LIGHTING_BIT);*/
			ai = p->atr;
			if(atr->valid & flags & (SUF_DIF | SUF_COL))
				gldMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, atr->dif, c);
			if(atr->valid & flags & (SUF_AMB | SUF_COL))
				gldMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, atr->amb, c);
			if(flags & SUF_EMI)
				gldMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, atr->valid & SUF_EMI ? atr->emi : defemi, c);
			if(atr->valid & flags & SUF_SPC)
				gldMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, atr->spc, c);
			if(glIsEnabled(GL_TEXTURE_2D)){
				glBindTexture(GL_TEXTURE_2D, 0);
				glDisable(GL_TEXTURE_2D);
				if(c){
					c->texenabled = 0;
					c->valid &= ~GLD_TEX;
				}
			}
			if(glActiveTextureARB){
				glActiveTextureARB(GL_TEXTURE1_ARB);
				glDisable(GL_TEXTURE_2D);
				glActiveTextureARB(GL_TEXTURE0_ARB);
			}
/*			glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);*/
		}
/*		glBegin(GL_POLYGON);*/
		/* normal caching is more efficient! */
/*		if(3 < suf->p[i]->n){
			double *v0 = suf->v[suf->p[i]->v[0]], *v1 = suf->v[suf->p[i]->v[1]], *v2 = suf->v[suf->p[i]->v[2]];
			double v01[3], v02[3], vv[3];
			VECSUB(v01, v0, v1);
			VECSUB(v02, v0, v2);
			VECVP(vv, v01, v02);
			glNormal3dv(vv);
		}*/

		polydraw(suf, flags, c, i, &last, NULL);
/*
		if(suf->p[i]->t == suf_uvpoly || suf->p[i]->t == suf_uvshade) for(j = 0; j < p->n; j++){
			if((i == 0 && j == 0 || last != suf->p[i]->uv.v[j].n) && SUFINDEX_MAX != suf->p[i]->uv.v[j].n)
				glNormal3dv(suf->v[last = suf->p[i]->uv.v[j].n]);
			glVertex3dv(suf->v[suf->p[i]->uv.v[j].p]);
		}
		else for(j = 0; j < p->n; j++){
			if((i == 0 && j == 0 || last != p->v[j][1]) && USHRT_MAX != p->v[j][1])
				glNormal3dv(suf->v[last = p->v[j][1]]);
			glVertex3dv(suf->v[p->v[j][0]]);
		}
*/
/*		glEnd();*/
	}
/*	if(ai != USHRT_MAX)
		glPopAttrib();*/
}

void DrawSUFPoly(const suf_t *suf, int i, unsigned long flags, struct gldCache *c){
	sufindex last = SUFINDEX_MAX, ai = SUFINDEX_MAX;
	int j;
	struct suf_poly_t *p = &suf->p[i]->p;
	struct suf_atr_t *atr;
	assert(suf && 0 <= i && i < suf->np);
	atr = &suf->a[p->atr];

	if(flags && atr->valid & flags){
		static const GLfloat defemi[4] = {0., 0., 0., 1.};
/*		if(ai == USHRT_MAX)
			glPushAttrib(GL_LIGHTING_BIT);*/
		ai = p->atr;
		if(atr->valid & flags & (SUF_DIF | SUF_COL))
			gldMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, atr->dif, c);
		if(atr->valid & flags & (SUF_AMB | SUF_COL))
			gldMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, atr->amb, c);
		if(flags & SUF_EMI)
			gldMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, atr->valid & SUF_EMI ? atr->emi : defemi, c);
		if(atr->valid & flags & SUF_SPC)
			gldMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, atr->spc, c);
/*			glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);*/
	}
	glBegin(GL_POLYGON);
	if(suf->p[i]->t == suf_uvpoly) for(j = 0; j < p->n; j++){
		if((i == 0 && j == 0 || last != suf->p[i]->uv.v[j].n) && SUFINDEX_MAX != suf->p[i]->uv.v[j].n)
			glNormal3dv(suf->v[last = suf->p[i]->uv.v[j].n]);
		glVertex3dv(suf->v[suf->p[i]->uv.v[j].p]);
	}
	else for(j = 0; j < p->n; j++){
		if((i == 0 && j == 0 || last != p->v[j][1]) && USHRT_MAX != p->v[j][1])
			glNormal3dv(suf->v[last = p->v[j][1]]);
		glVertex3dv(suf->v[p->v[j][0]]);
	}
	glEnd();
/*	if(ai != USHRT_MAX)
		glPopAttrib();*/
}

/* make sure to call this function prior to a model being shadowed, to enable
  race condition reduction. */
void ShadowSUF(const suf_t *suf, const double l[3], const double n[3], const double pos[3], const double pyr[3], double scale, const double (*pret)[16]){
	double nl = VECSP(l, n);
	GLdouble mat_shadow[16] = {
		1-n[0]*l[0]/nl,  -n[0]*l[1]/nl,  -n[0]*l[2]/nl, 0,
		 -n[1]*l[0]/nl, 1-n[1]*l[1]/nl,  -n[1]*l[2]/nl, 0,
		 -n[2]*l[0]/nl,  -n[2]*l[1]/nl, 1-n[2]*l[2]/nl, 0,
		 0, 0, 0, 1,
	};
	glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_LINE_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_LIGHTING);
	glEnable(GL_CULL_FACE);
	glColor4ub(0,0,0,255);
	glDepthMask(0); /* is it right to place here? */
	glPushMatrix();
	glMultMatrixd(mat_shadow);
	if(pos)
		glTranslated(pos[0], pos[1], pos[2]);
	if(scale != 1.) /* scaling can be a bit slower than comparition on software renderers */
		glScaled(scale, scale, scale);
	if(pyr){
		glRotated(-pyr[1] * 360 / 2. / M_PI, 0., 1., 0.);
		glRotated(pyr[0] * 360 / 2. / M_PI, 1., 0., 0.);
	}
	/* pre transformations for axis-converted suf models */
	if(pret)
		glMultMatrixd(*pret);
	DrawSUF(suf, 0, NULL);
/*	glDisable(GL_LINE_SMOOTH);
	glPolygonMode(GL_FRONT, GL_LINE);
	glColor3ub(255,255,255);
	DrawSUF(suf, 0);*/
	glPopMatrix();
	glPopAttrib();
}

void ShadowDrawSUF(const suf_t *suf, unsigned long flags, const double l[3], const double n[3], const double (*trans)[16]){
	ShadowSUF(suf, l, n, NULL, NULL, 1., trans);
	glPushMatrix();
	glMultMatrixd(*trans);
	DrawSUF(suf, flags, NULL);
	glPopMatrix();
}

sufdecal_t *AllocSUFDecal(const suf_t *suf){
	sufdecal_t *ret;
	ret = malloc(offsetof(sufdecal_t, p) + suf->np * sizeof *ret->p);
	ret->drawproc = NULL;
	ret->freeproc = NULL;
	ret->np = suf->np;
	memset(ret->p, 0, ret->np * sizeof *ret->p);
	return ret;
}

void FreeSUFDecal(sufdecal_t *sd){
	if(sd->freeproc){
		int i;
		for(i = 0; i < sd->np; i++)
			sd->freeproc(sd->p[i]);
	}
	free(sd);
}

void AddSUFDecal(sufdecal_t *sd, sufindex si, void *pv){
	if(sd->np <= si)
		return;
	if(sd->freeproc && sd->p[si])
		sd->freeproc(sd->p[si]);
	sd->p[si] = pv;
}

static polydraw(const suf_t *suf, unsigned long flags, struct gldCache *c, int i, sufindex *plast, const suftex_t *tex){
	int j;
	union suf_prim_t *pr = suf->p[i];
	struct suf_poly_t *p = &pr->p;
	const struct suf_atr_t *a = &suf->a[p->atr];
	glBegin(GL_POLYGON);
	if(suf->p[i]->t == suf_uvpoly || suf->p[i]->t == suf_uvshade){
		for(j = 0; j < p->n; j++){
			if((i == 0 && j == 0 || *plast != suf->p[i]->uv.v[j].n) && SUFINDEX_MAX != suf->p[i]->uv.v[j].n)
				glNormal3dv(suf->v[*plast = suf->p[i]->uv.v[j].n]);
			if(flags & SUF_TEX && suf->a[p->atr].colormap){
				glTexCoord2d(suf->v[pr->uv.v[j].t][0] / a->mapsize[2], 1. - suf->v[pr->uv.v[j].t][1] / a->mapsize[3]);
				if(glMultiTexCoord2dARB && flags & SUF_MTX && tex && 1 <= tex->n)
					glMultiTexCoord2dARB(GL_TEXTURE1_ARB, suf->v[pr->uv.v[j].t][0] / a->mapsize[2] * tex->a[p->atr].scale, (1. - suf->v[pr->uv.v[j].t][1] / a->mapsize[3]) * tex->a[p->atr].scale);
			}
			glVertex3dv(suf->v[suf->p[i]->uv.v[j].p]);
		}
	}
	else for(j = 0; j < p->n; j++){
		if((i == 0 && j == 0 || *plast != p->v[j][1]) && USHRT_MAX != p->v[j][1])
			glNormal3dv(suf->v[*plast = p->v[j][1]]);
		glVertex3dv(suf->v[p->v[j][0]]);
	}
	glEnd();
}

static double textime = 0.;

void EndFrameSUF(void){
/*	printf("texture time total: %lg\n", textime);*/
	textime = 0.;
}

void DecalDrawSUF(const suf_t *suf, unsigned long flags, struct gldCache *c, const suftex_t *tex, sufdecal_t *sd, void *sdg){
	int i;
	unsigned k;
	sufindex last = SUFINDEX_MAX, ai = SUFINDEX_MAX;
	assert(suf);
	for(i = k = 0; i < suf->np; i++){
		struct suf_poly_t *p = &suf->p[i]->p;
		struct suf_atr_t *atr = &suf->a[p->atr];

		/* the effective use of bitfields determines which material commands are
		  only needed. */
		if(flags && (SUF_EMI | atr->valid) & flags && ai != p->atr){
			static const GLfloat defemi[4] = {0., 0., 0., 1.};
/*			if(ai == USHRT_MAX)
				glPushAttrib(GL_LIGHTING_BIT);*/
			ai = p->atr;
			if(atr->valid & flags & (SUF_DIF | SUF_COL))
				gldMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, atr->dif, c);
			if(atr->valid & flags & (SUF_AMB | SUF_COL))
				gldMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, atr->amb, c);
			if(flags & SUF_EMI)
				gldMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, atr->valid & SUF_EMI ? atr->emi : defemi, c);
			if(atr->valid & flags & SUF_SPC)
				gldMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, atr->spc, c);
			if(tex && flags & SUF_TEX){
				int mismatch = (!c || !(c->valid & GLD_TEX) || c->texture != tex->a[ai].list);
				if(atr->valid & SUF_TEX){
					if(mismatch){
						if(tex->a[ai].list == 0){
							glDisable(GL_TEXTURE_2D);
							if(c)
								c->texenabled = 0;
						}
						else{
							timemeas_t tm;
							double t;
							TimeMeasStart(&tm);
							glCallList(tex->a[ai].list);
/*							if(glActiveTextureARB){
								glActiveTextureARB(GL_TEXTURE0_ARB);
								glBindTexture(GL_TEXTURE_2D, tex->a[ai].tex[0]);
								glEnable(GL_TEXTURE_2D);
								glActiveTextureARB(GL_TEXTURE1_ARB);
								if(tex->a[ai].tex[1]){
									glBindTexture(GL_TEXTURE_2D, tex->a[ai].tex[1]);
									glEnable(GL_TEXTURE_2D);
								}
								else
									glDisable(GL_TEXTURE_2D);
								glActiveTextureARB(GL_TEXTURE0_ARB);
							}
							else{
								glBindTexture(GL_TEXTURE_2D, tex->a[ai].tex[0]);
								glEnable(GL_TEXTURE_2D);
							}*/
							t = TimeMeasLap(&tm);
/*							printf("[%d]%s %lg\n", tex->a[ai].list, atr->colormap, t);*/
							textime += t;
							if(c)
								c->texenabled = 1;
						}
					}
					else if(!c->texenabled){
						glEnable(GL_TEXTURE_2D);
						c->texenabled = 1;
					}
				}
				else{
					if(mismatch){
						glDisable(GL_TEXTURE_2D);
						if(glActiveTextureARB){
							glActiveTextureARB(GL_TEXTURE1);
							glDisable(GL_TEXTURE_2D);
							glActiveTextureARB(GL_TEXTURE0);
						}
						if(c)
							c->texenabled = 0;
					}
				}
				if(c && mismatch){
					c->valid |= GLD_TEX;
					c->texture = atr->valid & SUF_TEX ? tex->a[ai].list : 0;
				}
			}
/*			glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);*/
		}

		if(sd && sd->drawproc && i < sd->np && sd->p[i]){
			glDepthMask(GL_FALSE);

			polydraw(suf, flags, c, i, &last, tex);

			sd->drawproc(sd->p[i], c, sdg);

			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			glDepthMask(GL_TRUE);
			polydraw(suf, flags, c, i, &last, tex);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}
		else
			polydraw(suf, flags, c, i, &last, tex);
	}
/*	if(ai != USHRT_MAX)
		glPopAttrib();*/
}

typedef struct suftexcache stc_t;
static stc_t *gstc = NULL;
static unsigned nstc = 0;

void listtex(void){
#if 0
	GLboolean r;
	int i, j;
	for(i = 0; i < nstc; i++) for(j = 0; j < numof(gstc[i].tex); j++) if(glIsTexture(gstc[i].tex[j])){
		glAreTexturesResident(1, &gstc[i].tex[j], &r);
		printf("texture[%d](%d) \"%s\" is %s\n", gstc[i].tex[j], gstc[i].list, gstc[i].name, r ? "resident" : "absent");
		r = 0;
	}
#endif
}

/* if we have already compiled the texture into a list, reuse it */
const stc_t *FindTexCache(const char *name){
	unsigned j;
	for(j = 0; j < nstc; j++) if(!strcmp(name, gstc[j].name)){
		return &gstc[j];
	}
	return NULL;
}


static void cachemtex(const suftexparam_t *stp){
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, stp->flags & STP_WRAP_S ? stp->wraps : GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, stp->flags & STP_WRAP_T ? stp->wrapt : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, stp->flags & STP_MAGFIL ? stp->magfil : GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, stp->flags & STP_MINFIL ? stp->minfil : stp->flags & STP_MIPMAP ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, stp->flags & STP_ENV ? stp->env : GL_MODULATE);
	if(stp->flags & STP_ALPHA_TEST /*stp->mipmap & 0x80*/){
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, .5f);
	}
	else
		glDisable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
}

static GLuint cachetex(const suftexparam_t *stp){
	GLuint ret;
	const BITMAPINFO *bmi = stp->bmi;
	GLint env = stp->env;
	unsigned char *head;
	int alpha = stp->flags & STP_ALPHA;
	int mipmap = stp->flags & STP_MIPMAP;
#if 1
	GLubyte (*tex)[3], (*tex4)[4];

	head = &((unsigned char*)bmi)[offsetof(BITMAPINFO, bmiColors) + bmi->bmiHeader.biClrUsed * sizeof(RGBQUAD)];

	switch(bmi->bmiHeader.biBitCount){
	case 4:
	{
		int x, y;
		int cols = bmi->bmiHeader.biClrUsed ? bmi->bmiHeader.biClrUsed : 16;
		const unsigned char *src = (const unsigned char*)&bmi->bmiColors[cols];
		if(alpha){
			tex4 = malloc(bmi->bmiHeader.biWidth * bmi->bmiHeader.biHeight * sizeof*tex4);
			for(y = 0; y < bmi->bmiHeader.biHeight; y++){
				unsigned char *buf = head + (bmi->bmiHeader.biWidth * bmi->bmiHeader.biBitCount + 31) / 32 * 4 * y;
				for(x = 0; x < bmi->bmiHeader.biWidth; x++){
					int pos = x + y * bmi->bmiHeader.biWidth, idx = 0xf & (buf[x / 2] >> ((x+1) % 2 * 4));
					tex4[pos][0] = bmi->bmiColors[idx].rgbRed;
					tex4[pos][1] = bmi->bmiColors[idx].rgbGreen;
					tex4[pos][2] = bmi->bmiColors[idx].rgbBlue;
					tex4[pos][3] = stp->mipmap & 0x80 ? idx ? 255 : 0 : 128 == bmi->bmiColors[idx].rgbRed && 128 == bmi->bmiColors[idx].rgbGreen && 128 == bmi->bmiColors[idx].rgbBlue ? 0 : 255;
				}
			}
		}
		else{
			tex = malloc(bmi->bmiHeader.biWidth * bmi->bmiHeader.biHeight * sizeof*tex);
			for(y = 0; y < bmi->bmiHeader.biHeight; y++){
				unsigned char *buf = head + (bmi->bmiHeader.biWidth * bmi->bmiHeader.biBitCount + 31) / 32 * 4 * y;
				for(x = 0; x < bmi->bmiHeader.biWidth; x++){
					int pos = x + y * bmi->bmiHeader.biWidth, idx = 0xf & (buf[x / 2] >> ((x+1) % 2 * 4));
					tex[pos][0] = bmi->bmiColors[idx].rgbRed;
					tex[pos][1] = bmi->bmiColors[idx].rgbGreen;
					tex[pos][2] = bmi->bmiColors[idx].rgbBlue;
				}
			}
		}
		break;
	}
	case 8:
	{
		int x, y;
		int cols = bmi->bmiHeader.biClrUsed;
		const unsigned char *src = (const unsigned char*)&bmi->bmiColors[cols];
		if(alpha){
			tex4 = malloc(bmi->bmiHeader.biWidth * bmi->bmiHeader.biHeight * sizeof*tex4);
			for(y = 0; y < bmi->bmiHeader.biHeight; y++) for(x = 0; x < bmi->bmiHeader.biWidth; x++){
				int pos = x + y * bmi->bmiHeader.biWidth, idx = src[pos];
				tex4[pos][0] = bmi->bmiColors[idx].rgbRed;
				tex4[pos][1] = bmi->bmiColors[idx].rgbGreen;
				tex4[pos][2] = bmi->bmiColors[idx].rgbBlue;
				tex4[pos][3] = 128 == bmi->bmiColors[idx].rgbRed && 128 == bmi->bmiColors[idx].rgbGreen && 128 == bmi->bmiColors[idx].rgbBlue ? 0 : 255;
			}
		}
		else{
			int w = bmi->bmiHeader.biWidth, h = bmi->bmiHeader.biHeight, wb = (bmi->bmiHeader.biWidth + 3) / 4 * 4;
			tex = malloc(w * h * sizeof*tex);
			if(stp->flags & STP_NORMALMAP) for(y = 0; y < h; y++) for(x = 0; x < w; x++){
				int pos = x + y * w;
				tex[pos][0] = (unsigned char)rangein(127 + (double)stp->normalfactor * (src[x + y * wb] - src[(x - 1 + w) % w + y * wb]) / 4, 0, 255);
				tex[pos][1] = (unsigned char)rangein(127 + stp->normalfactor * (src[x + y * wb] - src[x + (y - 1 + h) % h * wb]) / 4, 0, 255);
				tex[pos][2] = 255;
			}
			else  for(y = 0; y < h; y++) for(x = 0; x < w; x++){
				int pos = x + y * w, idx = src[x + y * wb];
				tex[pos][0] = bmi->bmiColors[idx].rgbRed;
				tex[pos][1] = bmi->bmiColors[idx].rgbGreen;
				tex[pos][2] = bmi->bmiColors[idx].rgbBlue;
			}
		}
		break;
	}
	case 24:
	{
		int x, y;
		int cols = bmi->bmiHeader.biClrUsed;
		const unsigned char (*src)[3] = (const unsigned char(*)[3])&bmi->bmiColors[cols];
		if(alpha){
			int alphatex = stp->flags & STP_ALPHATEX && stp->bmiMask->bmiHeader.biBitCount == 8 && stp->bmi->bmiHeader.biWidth == stp->bmiMask->bmiHeader.biWidth && stp->bmi->bmiHeader.biHeight == stp->bmiMask->bmiHeader.biHeight;
			BYTE *alphasrc;
			if(alphatex)
				alphasrc = (BYTE*)&stp->bmiMask->bmiColors[stp->bmiMask->bmiHeader.biClrUsed];
			tex4 = malloc(bmi->bmiHeader.biWidth * bmi->bmiHeader.biHeight * sizeof*tex4);
			for(y = 0; y < bmi->bmiHeader.biHeight; y++) for(x = 0; x < bmi->bmiHeader.biWidth; x++){
				int pos = x + y * bmi->bmiHeader.biWidth;
				tex4[pos][0] = src[pos][2];
				tex4[pos][1] = src[pos][1];
				tex4[pos][2] = src[pos][0];
				tex4[pos][3] = alphatex ? alphasrc[pos] : 255;
			}
		}
		else{
			tex = malloc(bmi->bmiHeader.biWidth * bmi->bmiHeader.biHeight * sizeof*tex);
			for(y = 0; y < bmi->bmiHeader.biHeight; y++) for(x = 0; x < bmi->bmiHeader.biWidth; x++){
				int pos = x + y * bmi->bmiHeader.biWidth;
				tex[pos][0] = src[pos][2];
				tex[pos][1] = src[pos][1];
				tex[pos][2] = src[pos][0];
			}
		}
		break;
	}
	case 32:
	{
		int x, y, h = ABS(bmi->bmiHeader.biHeight);
		int cols = bmi->bmiHeader.biClrUsed;
		const unsigned char (*src)[4] = (const unsigned char(*)[4])&bmi->bmiColors[cols], *mask;
		if(stp->alphamap & STP_MASKTEX)
			mask = (const unsigned char*)&stp->bmiMask->bmiColors[stp->bmiMask->bmiHeader.biClrUsed];
		if((stp->flags & (STP_ALPHA | STP_RGBA32)) == (STP_ALPHA | STP_RGBA32)){
			tex4 = (unsigned char(*)[4])src;
		}
		else if(alpha /*stp->alphamap & (3 | STP_MASKTEX)*/){
			tex4 = malloc(bmi->bmiHeader.biWidth * h * sizeof*tex4);
			for(y = 0; y < h; y++) for(x = 0; x < bmi->bmiHeader.biWidth; x++){
				int pos = x + y * bmi->bmiHeader.biWidth;
				int pos1 = x + (bmi->bmiHeader.biHeight < 0 ? h - y - 1 : y) * bmi->bmiHeader.biWidth;
				tex4[pos1][0] = src[pos][2];
				tex4[pos1][1] = src[pos][1];
				tex4[pos1][2] = src[pos][0];
				tex4[pos1][3] = stp->alphamap & STP_MASKTEX ?
					((mask[x / 8 + (stp->bmiMask->bmiHeader.biWidth + 31) / 32 * 4 * (bmi->bmiHeader.biHeight < 0 ? h - y - 1 : y)] >> (7 - x % 8)) & 1) * 127 + 127 :
					stp->alphamap == 2 ? src[pos][3] : 255;
			}
		}
		else{
			tex = malloc(bmi->bmiHeader.biWidth * h * sizeof*tex);
			for(y = 0; y < h; y++) for(x = 0; x < bmi->bmiHeader.biWidth; x++){
				int pos = x + y * bmi->bmiHeader.biWidth;
				int pos1 = x + (bmi->bmiHeader.biHeight < 0 ? h - y - 1 : y) * bmi->bmiHeader.biWidth;
				tex[pos1][0] = src[pos][2];
				tex[pos1][1] = src[pos][1];
				tex[pos1][2] = src[pos][0];
			}
		}
		break;
	}
	default:
		return 0;
	}
	glGenTextures(1, &ret);
	glBindTexture(GL_TEXTURE_2D, ret);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	if(mipmap)
		gluBuild2DMipmaps(GL_TEXTURE_2D, alpha ? GL_RGBA : GL_RGB, bmi->bmiHeader.biWidth, ABS(bmi->bmiHeader.biHeight),
			alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, alpha ? (void*)tex4 : (void*)tex);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, alpha ? GL_RGBA : GL_RGB, bmi->bmiHeader.biWidth, ABS(bmi->bmiHeader.biHeight), 0,
			alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, alpha ? (void*)tex4 : (void*)tex);
	if(!(bmi->bmiHeader.biBitCount == 32 && (stp->flags & (STP_ALPHA | STP_RGBA32)) == (STP_ALPHA | STP_RGBA32)))
		free(stp->flags & STP_ALPHA ? (void*)tex4 : (void*)tex);
#else
	{
		int i;
		static const GLenum target[3] = {GL_PIXEL_MAP_I_TO_B, GL_PIXEL_MAP_I_TO_G, GL_PIXEL_MAP_I_TO_R};
		GLushort *map;
		map = malloc(cols * sizeof*map);
		for(i = 0; i < 3; i++){
			int j;
			for(j = 0; j < cols; j++)
				map[j] = (GLushort)(*(unsigned long*)&bmi->bmiColors[j] >> i * 8 << 8);
			glPixelMapusv(target[i], cols, map);
		}
		free(map);
	}
	glGenTextures(1, &ret);
	glBindTexture(GL_TEXTURE_2D, ret);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB4, bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight, 0,
		GL_COLOR_INDEX, GL_UNSIGNED_BYTE, &bmi->bmiColors[cols]);
/*	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight,
		GL_COLOR_INDEX, GL_UNSIGNED_BYTE, &bmi->bmiColors[cols]);*/
#endif
/*	cachemtex(stp);*/
	return ret;
}

unsigned long CacheSUFMTex(const char *name, const suftexparam_t *tex1, const suftexparam_t *tex2){

	/* allocate cache list space */
	gstc = realloc(gstc, ++nstc * sizeof *gstc);
	gstc[nstc-1].name = malloc(strlen(name)+1);
	strcpy(gstc[nstc-1].name, name);

/*	cachetex(bmi);*/
	glPushAttrib(GL_TEXTURE_BIT);
/*	glGetIntegerv(GL_TEXTURE_2D_BINDING, &prevtex);*/
	if(!strcmp(name, "Mapping1.bmp")){
		gstc[nstc-1].list = 0;
		return 0;
	}
/*	glNewList(gstc[nstc-1].list = glGenLists(1), GL_COMPILE);*/
	gstc[nstc-1].tex[1] = 0;
	if(!glActiveTextureARB)
		gstc[nstc-1].tex[0] = cachetex(tex1);
	else{
		if(tex2){
/*			glActiveTextureARB(GL_TEXTURE0_ARB);*/
			gstc[nstc-1].tex[0] = cachetex(tex1);
/*			glActiveTextureARB(GL_TEXTURE1_ARB);*/
/*			if(tex1->bmi == tex2->bmi){
				gstc[nstc-1].tex[1] = gstc[nstc-1].tex[0];
				glBindTexture(GL_TEXTURE_2D, gstc[nstc-1].tex[1] = gstc[nstc-1].tex[0]);
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			}
			else*/
				gstc[nstc-1].tex[1] = cachetex(tex2);
/*			glActiveTextureARB(GL_TEXTURE0_ARB);*/
		}
		else{
/*			glActiveTextureARB(GL_TEXTURE0_ARB);*/
			gstc[nstc-1].tex[0] = cachetex(tex1);
/*			glActiveTextureARB(GL_TEXTURE1_ARB);
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE0_ARB);*/
		}
	}
/*	glBindTexture(GL_TEXTURE_2D, prevtex);*/
	glPopAttrib();
/*	glEndList();*/


	glNewList(gstc[nstc-1].list = glGenLists(1), GL_COMPILE);
	if(!glActiveTextureARB){
		cachemtex(tex1);
		glBindTexture(GL_TEXTURE_2D, gstc[nstc-1].tex[0]);
	}
	else{
		if(tex2){
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_2D, gstc[nstc-1].tex[0]);
			cachemtex(tex1);
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glBindTexture(GL_TEXTURE_2D, gstc[nstc-1].tex[1]);
			cachemtex(tex2);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
		else{
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_2D, gstc[nstc-1].tex[0]);
			cachemtex(tex1);
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
	}
	glEndList();
#if 0
	glNewList(gstc[nstc-1].list = glGenLists(1), GL_COMPILE);
	if(!glActiveTextureARB){
		glBindTexture(GL_TEXTURE_2D, gstc[nstc-1].tex[0]);
		cachemtex(tex1);
	}
	else{
		if(tex2){
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_2D, gstc[nstc-1].tex[0]);
			cachemtex(tex1);
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glBindTexture(GL_TEXTURE_2D, gstc[nstc-1].tex[1]);
			cachemtex(tex2);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
		else{
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_2D, gstc[nstc-1].tex[0]);
			cachemtex(tex1);
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
	}
	glEndList();
#endif

	return gstc[nstc-1].list;
}

unsigned long CacheSUFTex(const char *name, const BITMAPINFO *bmi, int mipmap){
	suftexparam_t stp;
	if(!bmi)
		return 0;
	stp.bmi = bmi;
	stp.flags = STP_ENV | STP_ALPHA | (mipmap ? STP_MIPMAP : 0);
	stp.env = GL_MODULATE;
	stp.mipmap = mipmap;
	stp.magfil = GL_NEAREST;
	stp.alphamap = 1;
	return CacheSUFMTex(name, &stp, NULL);
}

suftex_t *AllocSUFTex(const suf_t *suf){
	return AllocSUFTexScales(suf, NULL, 0, NULL, 0);
}

suftex_t *AllocSUFTexScales(const suf_t *suf, const double *scales, int nscales, const char **texes, int ntexes){
	suftex_t *ret;
	int i, n, k;
/*	for(i = n = 0; i < suf->na; i++) if(suf->a[i].colormap)
		n++;*/
	n = suf->na;
	ret = (suftex_t*)malloc(offsetof(suftex_t, a) + n * sizeof *ret->a);
	ret->n = n;
	for(i = k = 0; i < n; i++){
		unsigned j;
		const char *name = texes && i < ntexes && texes[i] ? texes[i] : suf->a[i].colormap;
		k = i;
		if(!(suf->a[i].valid & SUF_TEX) || !name){
			ret->a[k].list = 0;
			ret->a[k].tex[0] = 0;
			ret->a[k].tex[1] = 0;
			continue;
		}

		/* if we have already compiled the texture into a list, reuse it */
		for(j = 0; j < nstc; j++) if(!strcmp(name, gstc[j].name)){
			ret->a[k].list = gstc[j].list;
			ret->a[k].tex[0] = gstc[j].tex[0];
			ret->a[k].tex[1] = gstc[j].tex[1];
			ret->a[k].scale = 1.;
			break;
		}

		/* otherwise, compile it */
		if(j == nstc){
			HANDLE hbm;
			BITMAP bm;
			BITMAPINFOHEADER dbmi;
			FILE *fp;
			LPVOID pbuf;
			LONG lines, liens;
			HDC hdc;
			int err;

			/* allocate cache list space */
			gstc = realloc(gstc, ++nstc * sizeof *gstc);
			gstc[nstc-1].name = malloc(strlen(name)+1);
			strcpy(gstc[nstc-1].name, name);

			fp = fopen(suf->a[i].colormap, "rb");
			if(!fp){
				ret->a[k].list = gstc[nstc-1].list = 0;
				ret->a[k].tex[0] = ret->a[k].tex[1] = gstc[nstc-1].tex[0] = gstc[nstc-1].tex[0] = 0;
				continue;
			}
/*			glNewList(ret->a[k].list = gstc[nstc-1].list = glGenLists(1), GL_COMPILE);*/
			hbm = LoadImage(0, suf->a[i].colormap, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE/* | LR_CREATEDIBSECTION*/);
			GetObject(hbm, sizeof bm, &bm);
			pbuf = malloc(bm.bmWidth * bm.bmHeight * 4);
			lines = bm.bmHeight;
			dbmi.biSize = sizeof dbmi;
			dbmi.biWidth = bm.bmWidth;
			dbmi.biHeight = bm.bmHeight;
			dbmi.biPlanes = 1;
			dbmi.biBitCount = 24;
			dbmi.biCompression = BI_RGB;
			dbmi.biSizeImage = 0;
			dbmi.biClrUsed = dbmi.biClrImportant = 0;
			hdc = GetDC(GetDesktopWindow());
			if(lines != (liens = GetDIBits(hdc, hbm, 0, bm.bmHeight, pbuf, (LPBITMAPINFO)&dbmi, DIB_RGB_COLORS))){
/*				glEndList();
				glDeleteLists(ret->a[k].list, 1);*/
				ret->a[k].list = 0;
			}
			ReleaseDC(GetDesktopWindow(), hdc);
			if(lines == liens){
				GLint align;
				GLboolean swapbyte;
				glGenTextures(1, ret->a[k].tex);
				gstc[nstc-1].tex[0] = ret->a[k].tex[0];
				gstc[nstc-1].tex[1] = ret->a[k].tex[1] = 0;
				glBindTexture(GL_TEXTURE_2D, ret->a[k].tex[0]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &align);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
				glGetBooleanv(GL_UNPACK_SWAP_BYTES, &swapbyte);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
				err = gluBuild2DMipmaps(GL_TEXTURE_2D, 3, dbmi.biWidth, dbmi.biHeight, GL_RGB, GL_UNSIGNED_BYTE, pbuf);
				glPixelStorei(GL_UNPACK_ALIGNMENT, align);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, swapbyte);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

				glNewList(ret->a[k].list = gstc[nstc-1].list = glGenLists(1), GL_COMPILE);
				glBindTexture(GL_TEXTURE_2D, ret->a[k].tex[0]);
				glEnable(GL_TEXTURE_2D);
				if(glActiveTextureARB){
					glActiveTextureARB(GL_TEXTURE1_ARB);
					glDisable(GL_TEXTURE_2D);
					glActiveTextureARB(GL_TEXTURE0_ARB);
				}
				glEndList();
			}
			fclose(fp);
			DeleteObject(hbm);
			free(pbuf);
			if(k < nscales)
				ret->a[k].scale = scales[k];
		}
	}
	return ret;
}

