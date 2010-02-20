#include "clib/suf/sufvbo.h"
#include "clib/gl/multitex.h"
#include "clib/gl/gldraw.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#define GLD_TEX 0x20

#if defined(WIN32)
static PFNGLGENBUFFERSPROC glGenBuffers;
static PFNGLISBUFFERPROC glIsBuffer;
static PFNGLBINDBUFFERPROC glBindBuffer;
static PFNGLBUFFERDATAPROC glBufferData;
static PFNGLBUFFERSUBDATAPROC glBufferSubData;
static PFNGLMAPBUFFERPROC glMapBuffer;
static PFNGLUNMAPBUFFERPROC glUnmapBuffer;
static PFNGLDELETEBUFFERSPROC glDeleteBuffers;
static int initBuffers(){
	return (!(glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers"))
		|| !(glIsBuffer = (PFNGLISBUFFERPROC)wglGetProcAddress("glIsBuffer"))
		|| !(glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer"))
		|| !(glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData"))
		|| !(glBufferSubData = (PFNGLBUFFERSUBDATAPROC)wglGetProcAddress("glBufferSubData"))
		|| !(glMapBuffer = (PFNGLMAPBUFFERPROC)wglGetProcAddress("glMapBuffer"))
		|| !(glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)wglGetProcAddress("glUnmapBuffer"))
		|| !(glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers")))
		? -1 : 1;
}
#else
static int initBuffers(){return -1;}
#endif
static int init_VBO = 0;

VBO *CacheVBO(suf_t *suf){
	VBO *ret;
	GLdouble (*vert)[3] = NULL;
	GLdouble (*norm)[3] = NULL;
	GLdouble (*texc)[3] = NULL;
	int i, n = 0;

	if(init_VBO < 0)
		return NULL;
	if(!init_VBO){
		init_VBO = initBuffers();
		if(init_VBO <= 0){
		fprintf(stderr, "VBO not supported!\n");
			return NULL;
		}
	}
	ret = malloc(sizeof *ret);

	ret->suf = suf;
	ret->np = 0;
	ret->indices = NULL;

	for(i = 0; i < suf->np; i++) if(suf->p[i]->t == suf_uvpoly || suf->p[i]->t == suf_uvshade){
		int j;
		struct suf_uvpoly_t *uv = &suf->p[i]->uv;
		for(j = 0; j < uv->n; j++){
			vert = (GLdouble (*)[3])realloc(vert, (n+1) * sizeof *vert);
			memcpy(vert[n], suf->v[uv->v[j].p], sizeof *suf->v);
			norm = (GLdouble (*)[3])realloc(norm, (n+1) * sizeof *norm);
			memcpy(norm[n], suf->v[uv->v[j].n], sizeof *suf->v);
			texc = (GLdouble (*)[3])realloc(texc, (n+1) * sizeof *norm);
			texc[n][0] = suf->v[uv->v[j].t][0];
			texc[n][1] = 1. - suf->v[uv->v[j].t][1]; // upside down because of coordinate system difference.
			texc[n][2] = suf->v[uv->v[j].t][2];
			n++;
		}
		ret->indices = (GLushort *)realloc(ret->indices, ++ret->np * sizeof *ret->indices);
		ret->indices[ret->np-1] = uv->n;
	}
	else{
		int j;
		struct suf_poly_t *p = &suf->p[i]->p;
		for(j = 0; j < p->n; j++){
			vert = (GLdouble (*)[3])realloc(vert, (n+1) * sizeof *vert);
			memcpy(vert[n], suf->v[p->v[j][0]], sizeof *suf->v);
			norm = (GLdouble (*)[3])realloc(norm, (n+1) * sizeof *norm);
			memcpy(norm[n], suf->v[p->v[j][1]], sizeof *suf->v);
			texc = (GLdouble (*)[3])realloc(texc, (n+1) * sizeof *norm);
			memset(texc[n], 0, sizeof *texc);
			n++;
		}
		ret->indices = (GLushort *)realloc(ret->indices, ++ret->np * sizeof *ret->indices);
		ret->indices[ret->np-1] = p->n;
	}

	glGenBuffers(4, ret->buffers);

	/* Vertex array */
	glBindBuffer(GL_ARRAY_BUFFER, ret->buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, n * sizeof(*vert), vert, GL_STATIC_DRAW);

	/* Normal array */
	glBindBuffer(GL_ARRAY_BUFFER, ret->buffers[1]);
	glBufferData(GL_ARRAY_BUFFER, n * sizeof(*norm), norm, GL_STATIC_DRAW);

	/* Texture coordinates array */
	glBindBuffer(GL_ARRAY_BUFFER, ret->buffers[2]);
	glBufferData(GL_ARRAY_BUFFER, n * sizeof(*texc), texc, GL_STATIC_DRAW);

	/* Vertex index array */
//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[3]);
//	glBufferData(GL_ELEMENT_ARRAY_BUFFER, suf->np, face, GL_STATIC_DRAW);

	/* Now that buffers reside in video memory (hopefully!), we can free local memory for vertices. */
	free(vert);
	free(norm);
	free(texc);

	return ret;
}

void DrawVBO(const VBO *vbo, unsigned long flags, suftex_t *tex){
	struct gldCache *c = NULL;
	if(!vbo || init_VBO <= 0)
		return;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	/* Vertex array */
	glBindBuffer(GL_ARRAY_BUFFER, vbo->buffers[0]);
	glVertexPointer(3, GL_DOUBLE, 0, (0));

	/* Normal array */
	glBindBuffer(GL_ARRAY_BUFFER, vbo->buffers[1]);
	glNormalPointer(GL_DOUBLE, 0, (0));

	/* Texture coordinates array */
	glBindBuffer(GL_ARRAY_BUFFER, vbo->buffers[2]);
	glTexCoordPointer(3, GL_DOUBLE, 0, (0));

	/* Vertex index array */
//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[3]);

	/* Materials must be accumulated yet. */
	{
		int n = 0;
		int i;
		sufindex last = SUFINDEX_MAX, ai = SUFINDEX_MAX;
		for(i = 0; i < vbo->np; i++){
			struct suf_poly_t *p = &vbo->suf->p[i]->p;
			struct suf_atr_t *atr = &vbo->suf->a[p->atr];

			/* the effective use of bitfields determines which material commands are
			  only needed. */
			if((SUF_EMI | atr->valid) && ai != p->atr){
				static const GLfloat defemi[4] = {0., 0., 0., 1.};
	/*			if(ai == USHRT_MAX)
					glPushAttrib(GL_LIGHTING_BIT);*/
				ai = p->atr;
				if(atr->valid & (SUF_DIF | SUF_COL))
					gldMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, atr->dif, NULL);
				if(atr->valid & (SUF_AMB | SUF_COL))
					gldMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, atr->amb, NULL);
				if(SUF_EMI)
					gldMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, atr->valid & SUF_EMI ? atr->emi : defemi, NULL);
				if(atr->valid & SUF_SPC)
					gldMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, atr->spc, NULL);
				if(glIsEnabled(GL_TEXTURE_2D)){
					glBindTexture(GL_TEXTURE_2D, 0);
					glDisable(GL_TEXTURE_2D);
				}
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
	/*							timemeas_t tm;
								double t;
								TimeMeasStart(&tm);*/
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
	//							t = TimeMeasLap(&tm);
	/*							printf("[%d]%s %lg\n", tex->a[ai].list, atr->colormap, t);*/
	/*							textime += t;*/
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
				else if(glActiveTextureARB){
					glActiveTextureARB(GL_TEXTURE1_ARB);
					glDisable(GL_TEXTURE_2D);
					glActiveTextureARB(GL_TEXTURE0_ARB);
				}
	/*			glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);*/
			}
			glDrawArrays(GL_POLYGON, n, vbo->indices[i]);
			n += vbo->indices[i];
		}
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

}