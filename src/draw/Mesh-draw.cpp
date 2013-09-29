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

void Mesh::draw(Flags flags, Cache *c)const{
	int i;
	Index last = INDEX_MAX, ai = INDEX_MAX;
	assert(this);
	for(i = 0; i < np; i++){
		Polygon *p = &this->p[i]->p;
		Attrib *atr = &this->a[p->atr];

		/* the effective use of bitfields determines which material commands are
		  only needed. */
		if(flags && (Emission | atr->valid) & flags && ai != p->atr){
			static const GLfloat defemi[4] = {0., 0., 0., 1.};
			ai = p->atr;
			if(atr->valid & flags & (Diffuse | Color))
				gldMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, atr->dif, c);
			if(atr->valid & flags & (Ambient | Color))
				gldMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, atr->amb, c);
			if(flags & Emission)
				gldMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, atr->valid & Emission ? atr->emi : defemi, c);
			if(atr->valid & flags & Specular)
				gldMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, atr->spc, c);
			if(atr->valid & flags & Shine)
				gldMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &atr->shininess, c);
			if(glIsEnabled(GL_TEXTURE_2D)){
				glBindTexture(GL_TEXTURE_2D, 0);
				glDisable(GL_TEXTURE_2D);
				if(c){
					c->texenabled = 0;
					c->valid &= ~Texture;
				}
			}
			if(glActiveTextureARB){
				glActiveTextureARB(GL_TEXTURE1_ARB);
				glDisable(GL_TEXTURE_2D);
				glActiveTextureARB(GL_TEXTURE0_ARB);
			}
		}

		polyDraw(flags, c, i, &last, NULL);
	}
}

void Mesh::gldMaterialfv(GLenum face, GLenum pname, const GLfloat *params, Cache *c){
	Flags flag;
	float *dst;
	size_t dstsize;
	if(!c){
		glMaterialfv(face, pname, params);
		return;
	}
	switch(pname){
		case GL_DIFFUSE:
			flag = Diffuse;
			dst = c->matdif;
			dstsize = sizeof c->matdif;
			break;
		case GL_AMBIENT:
			flag = Ambient;
			dst = c->matamb;
			dstsize = sizeof c->matamb;
			break;
		case GL_EMISSION:
			flag = Emission;
			dst = c->matemi;
			dstsize = sizeof c->matemi;
			break;
		case GL_SPECULAR:
			flag = Specular;
			dst = c->matspc;
			dstsize = sizeof c->matspc;
			break;
		case GL_SHININESS:
			flag = Shine;
			dst = &c->matshininess;
			dstsize = sizeof c->matshininess;
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

void Mesh::drawPoly(int i, Flags flags, Cache *c)const{
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
		if(atr->valid & flags & (Diffuse | Color))
			gldMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, atr->dif, c);
		if(atr->valid & flags & (Ambient | Color))
			gldMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, atr->amb, c);
		if(flags & Emission)
			gldMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, atr->valid & Emission ? atr->emi : defemi, c);
		if(atr->valid & flags & Specular)
			gldMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, atr->spc, c);
/*			glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);*/
	}
	glBegin(GL_POLYGON);
	if(this->p[i]->t == suf_uvpoly) for(j = 0; j < p->n; j++){
		if((i == 0 && j == 0 || last != this->p[i]->uv.v[j].nrm) && INDEX_MAX != this->p[i]->uv.v[j].nrm)
			glNormal3dv(this->v[last = this->p[i]->uv.v[j].nrm]);
		glVertex3dv(this->v[this->p[i]->uv.v[j].pos]);
	}
	else for(j = 0; j < p->n; j++){
		if((i == 0 && j == 0 || last != p->v[j].nrm) && USHRT_MAX != p->v[j].nrm)
			glNormal3dv(this->v[last = p->v[j].nrm]);
		glVertex3dv(this->v[p->v[j].pos]);
	}
	glEnd();
/*	if(ai != USHRT_MAX)
		glPopAttrib();*/
}

void Mesh::polyDraw(Flags flags, Cache *c, int i, Index *plast, const MeshTex *tex)const{
	int j;
	Primitive *pr = this->p[i];
	Polygon *p = &pr->p;
	const Attrib *a = &this->a[p->atr];
	glBegin(GL_POLYGON);
	if(this->p[i]->t == suf_uvpoly || this->p[i]->t == suf_uvshade){
		for(j = 0; j < p->n; j++){
			if((i == 0 && j == 0 || *plast != this->p[i]->uv.v[j].nrm) && INDEX_MAX != this->p[i]->uv.v[j].nrm)
				glNormal3dv(this->v[*plast = this->p[i]->uv.v[j].nrm]);
			if(flags & Texture && this->a[p->atr].colormap){
				glTexCoord2d(this->v[pr->uv.v[j].tex][0] / a->mapsize[2], 1. - this->v[pr->uv.v[j].tex][1] / a->mapsize[3]);
				if(glMultiTexCoord2dARB && flags & MultiTex && tex && 1 <= tex->n)
					glMultiTexCoord2dARB(GL_TEXTURE1_ARB, this->v[pr->uv.v[j].tex][0] / a->mapsize[2] * tex->a[p->atr].scale, (1. - this->v[pr->uv.v[j].tex][1] / a->mapsize[3]) * tex->a[p->atr].scale);
			}
			glVertex3dv(this->v[this->p[i]->uv.v[j].pos]);
		}
	}
	else for(j = 0; j < p->n; j++){
		if((i == 0 && j == 0 || *plast != p->v[j].nrm) && INDEX_MAX != p->v[j].nrm)
			glNormal3dv(this->v[*plast = p->v[j].nrm]);
		glVertex3dv(this->v[p->v[j].pos]);
	}
	glEnd();
}

static double textime = 0.;

void Mesh::decalDraw(Flags flags, Cache *c, const MeshTex *tex, Decal *sd, void *sdg)const{
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
		if(flags && (Emission | atr->valid) & flags && ai != p->atr){
			static const GLfloat defemi[4] = {0., 0., 0., 1.};
			ai = p->atr;
			if(atr->valid & flags & (Diffuse | Color))
				gldMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, atr->dif, c);
			if(atr->valid & flags & (Ambient | Color))
				gldMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, atr->amb, c);
			if(flags & Emission)
				gldMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, atr->valid & Emission ? atr->emi : defemi, c);
			if(atr->valid & flags & Specular)
				gldMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, atr->spc, c);
			if(atr->valid & flags & Shine)
				gldMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &atr->shininess, c);
			if(tex && flags & Texture){
				int mismatch = (!c || !(c->valid & Texture) || c->texture != tex->a[ai].list);

				// Execute user-provided callback function when an attribute is exiting.
				if(ai != last && last != INDEX_MAX && tex->a[last].onEndTexture)
					tex->a[last].onEndTexture(tex->a[last].onEndTextureData);

				// Execute user-provided callback function when texture is being initialized (even if texture is absent).
				if(tex->a[ai].onBeginTexture)
					tex->a[ai].onBeginTexture(tex->a[ai].onBeginTextureData);

				if(atr->valid & Texture){
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
							t = TimeMeasLap(&tm);
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
					c->valid |= Texture;
					c->texture = atr->valid & Texture ? tex->a[ai].list : 0;
				}
			}
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
	if(tex && flags & Texture && ai != INDEX_MAX && tex->a[ai].onEndTexture)
		tex->a[ai].onEndTexture(tex->a[ai].onEndTextureData);
}
