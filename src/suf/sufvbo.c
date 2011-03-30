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
	GLdouble (**verts)[3] = NULL;
	GLdouble (**norms)[3] = NULL;
	GLdouble (**texcs)[3] = NULL;
	int i, n = 0;

	if(!suf)
		return NULL;
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
	ret->atris = (unsigned(*)[4])calloc(suf->na, sizeof *ret->atris);
	ret->natris = (int*)calloc(suf->na, sizeof *ret->natris);
	verts = (GLdouble**)calloc(suf->na, sizeof *verts);
	norms = (GLdouble**)calloc(suf->na, sizeof *norms);
	texcs = (GLdouble**)calloc(suf->na, sizeof *texcs);

	for(i = 0; i < suf->np; i++) if(suf->p[i]->t == suf_uvpoly || suf->p[i]->t == suf_uvshade){
		int j, k, kk;
		struct suf_uvpoly_t *uv = &suf->p[i]->uv;
		for(j = 0; j < uv->n; j++){
			int a = uv->atr;
			for(k = 0; k + 2 < uv->n; k++){
				for(kk = 0; kk < 3; kk++){
					int jj = kk == 0 ? 0 : k + kk;
					int n = ret->natris[a];
					verts[a] = (GLdouble (*)[3])realloc(verts[a], (n+1) * sizeof **verts);
					memcpy(verts[a][n], suf->v[uv->v[jj].p], sizeof *suf->v);
					norms[a] = (GLdouble (*)[3])realloc(norms[a], (n+1) * sizeof **norms);
					memcpy(norms[a][n], suf->v[uv->v[jj].n], sizeof *suf->v);
					texcs[a] = (GLdouble (*)[3])realloc(texcs[a], (n+1) * sizeof **texcs);
					texcs[a][n][0] = suf->v[uv->v[jj].t][0];
					texcs[a][n][1] = 1. - suf->v[uv->v[jj].t][1]; // upside down because of coordinate system difference.
					texcs[a][n][2] = suf->v[uv->v[jj].t][2];
					ret->natris[a]++;
				}
			}
		}
	}
	else{
		int j, k, kk;
		struct suf_poly_t *p = &suf->p[i]->p;
		for(j = 0; j < p->n; j++){
			int a = p->atr;
			for(k = 0; k + 2 < p->n; k++){
				for(kk = 0; kk < 3; kk++){
					int jj = kk == 0 ? 0 : k + kk;
					int n = ret->natris[a];
					verts[a] = (GLdouble (*)[3])realloc(verts[a], (n+1) * sizeof **verts);
					memcpy(verts[a][n], suf->v[p->v[jj][0]], sizeof *suf->v);
					norms[a] = (GLdouble (*)[3])realloc(norms[a], (n+1) * sizeof **norms);
					memcpy(norms[a][n], suf->v[p->v[jj][1]], sizeof *suf->v);
					texcs[a] = (GLdouble (*)[3])realloc(texcs[a], (n+1) * sizeof **texcs);
					memset(texcs[a][n], 0, sizeof *texcs[a]);
					ret->natris[a]++;
				}
			}
		}
	}

	for(i = 0; i < suf->na; i++){
		glGenBuffers(3, ret->atris[i]);

		/* Vertex array */
		glBindBuffer(GL_ARRAY_BUFFER, ret->atris[i][0]);
		glBufferData(GL_ARRAY_BUFFER, ret->natris[i] * sizeof(**verts), verts[i], GL_STATIC_DRAW);

		/* Normal array */
		glBindBuffer(GL_ARRAY_BUFFER, ret->atris[i][1]);
		glBufferData(GL_ARRAY_BUFFER, ret->natris[i] * sizeof(**norms), norms[i], GL_STATIC_DRAW);

		/* Texture coordinates array */
		glBindBuffer(GL_ARRAY_BUFFER, ret->atris[i][2]);
		glBufferData(GL_ARRAY_BUFFER, ret->natris[i] * sizeof(**texcs), texcs[i], GL_STATIC_DRAW);

		/* Vertex index array */
//		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[3]);
//		glBufferData(GL_ELEMENT_ARRAY_BUFFER, suf->np, face, GL_STATIC_DRAW);

		/* Now that buffers reside in video memory (hopefully!), we can free local memory for vertices. */
		free(verts[i]);
		free(norms[i]);
		free(texcs[i]);
	}

	free(verts);
	free(norms);
	free(texcs);

	return ret;
}

void DrawVBO(const VBO *vbo, unsigned long flags, suftex_t *tex){
	struct gldCache *c = NULL;
	if(!vbo || init_VBO <= 0)
		return;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	{
		int n = 0;
		int i;
		sufindex last = SUFINDEX_MAX, ai = SUFINDEX_MAX;
		for(i = 0; i < vbo->suf->na; i++){
			static const GLfloat defemi[4] = {0., 0., 0., 1.};
			struct suf_atr_t *atr = &vbo->suf->a[i];
			int j;

			/* Vertex array */
			glBindBuffer(GL_ARRAY_BUFFER, vbo->atris[i][0]);
			glVertexPointer(3, GL_DOUBLE, 0, (0));

			/* Normal array */
			glBindBuffer(GL_ARRAY_BUFFER, vbo->atris[i][1]);
			glNormalPointer(GL_DOUBLE, 0, (0));

			/* Texture coordinates array */
			glBindBuffer(GL_ARRAY_BUFFER, vbo->atris[i][2]);
			glTexCoordPointer(3, GL_DOUBLE, 0, (0));

			/* Vertex index array */
		//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[3]);

/*			if(ai == USHRT_MAX)
				glPushAttrib(GL_LIGHTING_BIT);*/
			ai = i;
			if(atr->valid & (SUF_DIF | SUF_COL))
				gldMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, atr->dif, NULL);
			if(atr->valid & (SUF_AMB | SUF_COL))
				gldMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, atr->amb, NULL);
			if(SUF_EMI)
				gldMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, atr->valid & SUF_EMI ? atr->emi : defemi, NULL);
			if(atr->valid & SUF_SPC)
				gldMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, atr->spc, NULL);
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
						{
							GLfloat envcolor[4] = {0., 0., 0., 1.};
							glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, envcolor);
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
			else{
/*				glBindTexture(GL_TEXTURE_2D, 0);
				glDisable(GL_TEXTURE_2D);
				if(glActiveTextureARB){
					glActiveTextureARB(GL_TEXTURE1_ARB);
					glDisable(GL_TEXTURE_2D);
					glActiveTextureARB(GL_TEXTURE0_ARB);
				}*/
			}

			glDrawArrays(GL_TRIANGLES, 0, vbo->natris[i]);
		}
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

}