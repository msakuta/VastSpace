/** \file
 * \brief Implementation of Mesh and MeshTex classes' graphics.
 */
#include "Mesh.h"
extern "C"{
#include "clib/c.h"
#include "clib/cfloat.h"
#include "clib/gl/gldraw.h"
#include "clib/gl/multitex.h"
#include "clib/timemeas.h"
#include "clib/avec3.h"
}
#include <assert.h>
#include <limits.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <stddef.h>

#define GLD_TEX 0x20

#define GLD_DIF 0x01
#define GLD_AMB 0x02
#define GLD_EMI 0x04
#define GLD_SPC 0x08
#define GLD_SHI 0x10


void Mesh::draw(unsigned long flags, Cache *c)const{
	int i;
	Index last = INDEX_MAX, ai = INDEX_MAX;
	assert(this);
	for(i = 0; i < np; i++){
		Polygon *p = &this->p[i]->p;
		Attrib *atr = &this->a[p->atr];

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

		polyDraw(flags, c, i, &last, NULL);
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

void Mesh::gldMaterialfv(GLenum face, GLenum pname, const GLfloat *params, Cache *c){
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

void Mesh::drawPoly(int i, unsigned long flags, Cache *c)const{
	Index last = INDEX_MAX, ai = INDEX_MAX;
	int j;
	Polygon *p = &this->p[i]->p;
	Attrib *atr;
	assert(this && 0 <= i && i < this->np);
	atr = &this->a[p->atr];

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
	if(this->p[i]->t == suf_uvpoly) for(j = 0; j < p->n; j++){
		if((i == 0 && j == 0 || last != this->p[i]->uv.v[j].n) && INDEX_MAX != this->p[i]->uv.v[j].n)
			glNormal3dv(this->v[last = this->p[i]->uv.v[j].n]);
		glVertex3dv(this->v[this->p[i]->uv.v[j].p]);
	}
	else for(j = 0; j < p->n; j++){
		if((i == 0 && j == 0 || last != p->v[j][1]) && USHRT_MAX != p->v[j][1])
			glNormal3dv(this->v[last = p->v[j][1]]);
		glVertex3dv(this->v[p->v[j][0]]);
	}
	glEnd();
/*	if(ai != USHRT_MAX)
		glPopAttrib();*/
}

void Mesh::polyDraw(unsigned long flags, Cache *c, int i, Index *plast, const MeshTex *tex)const{
	int j;
	Primitive *pr = this->p[i];
	Polygon *p = &pr->p;
	const Attrib *a = &this->a[p->atr];
	glBegin(GL_POLYGON);
	if(this->p[i]->t == suf_uvpoly || this->p[i]->t == suf_uvshade){
		for(j = 0; j < p->n; j++){
			if((i == 0 && j == 0 || *plast != this->p[i]->uv.v[j].n) && INDEX_MAX != this->p[i]->uv.v[j].n)
				glNormal3dv(this->v[*plast = this->p[i]->uv.v[j].n]);
			if(flags & SUF_TEX && this->a[p->atr].colormap){
				glTexCoord2d(this->v[pr->uv.v[j].t][0] / a->mapsize[2], 1. - this->v[pr->uv.v[j].t][1] / a->mapsize[3]);
				if(glMultiTexCoord2dARB && flags & SUF_MTX && tex && 1 <= tex->n)
					glMultiTexCoord2dARB(GL_TEXTURE1_ARB, this->v[pr->uv.v[j].t][0] / a->mapsize[2] * tex->a[p->atr].scale, (1. - this->v[pr->uv.v[j].t][1] / a->mapsize[3]) * tex->a[p->atr].scale);
			}
			glVertex3dv(this->v[this->p[i]->uv.v[j].p]);
		}
	}
	else for(j = 0; j < p->n; j++){
		if((i == 0 && j == 0 || *plast != p->v[j][1]) && USHRT_MAX != p->v[j][1])
			glNormal3dv(this->v[*plast = p->v[j][1]]);
		glVertex3dv(this->v[p->v[j][0]]);
	}
	glEnd();
}

static double textime = 0.;

void Mesh::decalDraw(unsigned long flags, Cache *c, const MeshTex *tex, Decal *sd, void *sdg)const{
	int i;
	unsigned k;
	Index last = INDEX_MAX;
	Index ai = INDEX_MAX;
	assert(this);
	for(i = k = 0; i < this->np; i++){
		Polygon *p = &this->p[i]->p;
		Attrib *atr = &this->a[p->atr];

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

				// Execute user-provided callback function when an attribute is exiting.
				if(ai != last && last != INDEX_MAX && tex->a[last].onEndTexture)
					tex->a[last].onEndTexture(tex->a[last].onEndTextureData);

				// Execute user-provided callback function when texture is being initialized (even if texture is absent).
				if(tex->a[ai].onBeginTexture)
					tex->a[ai].onBeginTexture(tex->a[ai].onBeginTextureData);

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
					
					// Execute user-provided callback function when texture initialization is complete (even if texture is absent).
					if(tex->a[ai].onInitedTexture)
						tex->a[ai].onInitedTexture(tex->a[ai].onInitedTextureData);
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
/*			glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);*/
		}

		if(sd && sd->drawproc && i < sd->np && sd->p[i]){
			glDepthMask(GL_FALSE);

			polyDraw(flags, c, i, &last, tex);

			sd->drawproc(sd->p[i], c, sdg);

			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			glDepthMask(GL_TRUE);
			polyDraw(flags, c, i, &last, tex);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}
		else
			polyDraw(flags, c, i, &last, tex);
		last = ai;
	}

	// Execute user-provided callback function when exiting.
	if(tex && flags & SUF_TEX && ai != INDEX_MAX && tex->a[ai].onEndTexture)
		tex->a[ai].onEndTexture(tex->a[ai].onEndTextureData);
/*	if(ai != USHRT_MAX)
		glPopAttrib();*/
}
