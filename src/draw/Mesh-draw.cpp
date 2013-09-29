/** \file
 * \brief Implementation of Mesh and MeshTex classes' graphics.
 */
#include "Mesh.h"
#include "Cmd.h"
#include "StaticInitializer.h"
extern "C"{
#include "clib/c.h"
#include "clib/cfloat.h"
#include "clib/gl/gldraw.h"
#include "clib/gl/multitex.h"
#include "clib/timemeas.h"
#include "clib/avec3.h"
}
#include "cpplib/Vec3.h"
#include <assert.h>
#include <limits.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <stddef.h>
#include <map>

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

			beginMaterial(atr, flags, c);

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
	if(this->p[i]->t == ET_UVPolygon) for(j = 0; j < p->n; j++){
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
	if(this->p[i]->t == ET_UVPolygon || this->p[i]->t == ET_UVShade){
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

#if defined(WIN32)
static PFNGLGENBUFFERSPROC glGenBuffers;
static PFNGLISBUFFERPROC glIsBuffer;
static PFNGLBINDBUFFERPROC glBindBuffer;
static PFNGLBUFFERDATAPROC glBufferData;
static PFNGLBUFFERSUBDATAPROC glBufferSubData;
static PFNGLMAPBUFFERPROC glMapBuffer;
static PFNGLUNMAPBUFFERPROC glUnmapBuffer;
static PFNGLDELETEBUFFERSPROC glDeleteBuffers;
static bool initBuffers(){
	static bool init = false;
	static bool result = false;
	if(!init){
		init = true;
		result = !(!(glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers"))
		|| !(glIsBuffer = (PFNGLISBUFFERPROC)wglGetProcAddress("glIsBuffer"))
		|| !(glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer"))
		|| !(glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData"))
		|| !(glBufferSubData = (PFNGLBUFFERSUBDATAPROC)wglGetProcAddress("glBufferSubData"))
		|| !(glMapBuffer = (PFNGLMAPBUFFERPROC)wglGetProcAddress("glMapBuffer"))
		|| !(glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)wglGetProcAddress("glUnmapBuffer"))
		|| !(glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers")));
	}
	return result;
}
#else
static bool initBuffers(){return -1;}
#endif

static double textime = 0.;
static int r_vbomesh = 1;

static void stinitfunc(){
	CvarAdd("r_vbomesh", &r_vbomesh, cvar_int);
}

static StaticInitializer stinit(stinitfunc);

void Mesh::beginMaterial(Attrib *atr, Flags flags, Cache *c)const{
	static const GLfloat defemi[4] = {0., 0., 0., 1.};
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
}

void Mesh::beginTexture(Attrib *atr, bool mismatch, const MeshTex *tex, int ai, Cache *c)const{
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

void Mesh::decalDraw(Flags flags, Cache *c, const MeshTex *tex, Decal *sd, void *sdg)const{
	// Check if we can use vertex buffer objects.
	if(r_vbomesh && initBuffers()){
		if(buf.size() == 0)
			compileVertexBuffers();

		for(int a = 0; a < buf.size(); ++a){
			BufferSet &bufs = buf[a];
			if(bufs.count == 0)
				continue;

			Attrib *atr = &this->a[a];

			beginMaterial(atr, flags, c);

			if(tex && flags & Texture){
				bool mismatch = (!c || !(c->valid & Texture) || c->texture != tex->a[a].list);

				// Execute user-provided callback function when texture is being initialized (even if texture is absent).
				if(tex->a[a].onBeginTexture)
					tex->a[a].onBeginTexture(tex->a[a].onBeginTextureData);

				beginTexture(atr, mismatch, tex, a, c);
			}

			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_NORMAL_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);

			/* Vertex array */
			glBindBuffer(GL_ARRAY_BUFFER, bufs.pos);
			glVertexPointer(3, GL_DOUBLE, 0, (0));

			/* Normal array */
			glBindBuffer(GL_ARRAY_BUFFER, bufs.nrm);
			glNormalPointer(GL_DOUBLE, 0, (0));

			/* Texture coordinates array */
			glBindBuffer(GL_ARRAY_BUFFER, bufs.tex);
			glTexCoordPointer(3, GL_DOUBLE, 0, (0));

			// Index array
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufs.ind);

			glDrawElements(GL_TRIANGLES, bufs.count, GL_UNSIGNED_INT, 0);
	//		glDrawArrays(GL_TRIANGLES, 0, vertices.size());

			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);

			// Execute user-provided callback function when exiting.
			if(tex && flags & Texture && tex->a[a].onEndTexture)
				tex->a[a].onEndTexture(tex->a[a].onEndTextureData);
		}
		return;
	}

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
			ai = p->atr;

			beginMaterial(atr, flags, c);

			if(tex && flags & Texture){
				bool mismatch = (!c || !(c->valid & Texture) || c->texture != tex->a[ai].list);

				// Execute user-provided callback function when an attribute is exiting.
				if(ai != last && last != INDEX_MAX && tex->a[last].onEndTexture)
					tex->a[last].onEndTexture(tex->a[last].onEndTextureData);

				// Execute user-provided callback function when texture is being initialized (even if texture is absent).
				if(tex->a[ai].onBeginTexture)
					tex->a[ai].onBeginTexture(tex->a[ai].onBeginTextureData);

				beginTexture(atr, mismatch, tex, ai, c);
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


typedef std::vector<Vec3d> VecList;
typedef std::vector<GLuint> IndList;

struct TempVertex;
typedef std::map<TempVertex, GLuint> VertexIndMap;

/// Set of data buffers that should be inserted during VBO compilation.
struct BufferData{
	VertexIndMap mapVertices;
	VecList plainVertices;
	VecList plainNormals;
	VecList plainTextures;
	IndList indices;
};

/// Local operator override for Vec3d comparison.
static bool operator<(const Vec3d &a, const Vec3d &b){
	for(int i = 0; i < 3; i++){
		if(a[i] < b[i])
			return true;
		else if(b[i] < a[i])
			return false;
	}
	return false;
}

/// Temporary vertex object to use as a key to vertex map.
struct TempVertex{

	Vec3d pos;
	Vec3d nrm;
	Vec3d tex;

	TempVertex(const Mesh &mesh, const Mesh::Polygon::Vertex &v) : pos(mesh.v[v.pos]), nrm(mesh.v[v.nrm]), tex(0,0,0){}
	TempVertex(const Mesh &mesh, const Mesh::UVPolygon::Vertex &v) : pos(mesh.v[v.pos]), nrm(mesh.v[v.nrm]), tex(mesh.v[v.tex]){}

	bool operator<(const TempVertex &o)const{
		if(pos < o.pos)
			return true;
		else if(o.pos < pos)
			return false;
		else if(nrm < o.nrm)
			return true;
		else if(o.nrm < nrm)
			return false;
		else if(tex < o.tex)
			return true;
		else
			return false;
	}

	// Template function to insert a vertex for either a Polygon or an UVPolygon.
	template<typename T>
	static void insert(BufferData &bd, T &vtx, const Mesh &mesh){
		TempVertex tv(mesh, vtx);
		VertexIndMap::iterator it = bd.mapVertices.find(tv);
		if(it != bd.mapVertices.end())
			bd.indices.push_back(it->second);
		else{
			bd.mapVertices[tv] = bd.plainVertices.size();
			bd.indices.push_back(bd.plainVertices.size());
			bd.plainVertices.push_back(tv.pos);
			bd.plainNormals.push_back(tv.nrm);
			bd.plainTextures.push_back(tv.tex);
		}
	}
};

/// Compile vertex buffer objects for each attributes.
void Mesh::compileVertexBuffers()const{
	// Note that we need to accumulate primitives for each attributes, because
	// it's costly to switch attributes between primitives.
	for(int a = 0; a < na; ++a){
		BufferData bd;

		for(int i = 0; i < np; ++i){
			Primitive *pr = p[i];

			// Skip primitives having not concerned attributes
			if(pr->p.atr != a)
				continue;

			for(int j = 0; j < pr->p.n - 2; j++){
				if(pr->t == ET_Polygon){
					TempVertex::insert(bd, pr->p.v[0], *this);
					for(int k = 0; k < 2; k++)
						TempVertex::insert(bd, pr->p.v[j+k+1], *this);
				}
				else{
					TempVertex::insert(bd, pr->uv.v[0], *this);
					for(int k = 0; k < 2; k++)
						TempVertex::insert(bd, pr->uv.v[j+k+1], *this);
				}
			}
		}

		// There could be an attribute without a primitive if a model consists
		// of multiple meshes.
		if(bd.indices.size() == 0){
			BufferSet bufs;
			bufs.count = 0;
			this->buf.push_back(bufs);
			continue;
		}

		// Allocate buffer objects only if we're sure that there are at least one primitive.
		GLuint bs[4];
		glGenBuffers(4, bs);
		BufferSet bufs;
		bufs.pos = bs[0];
		bufs.nrm = bs[1];
		bufs.tex = bs[2];
		bufs.ind = bs[3];
		bufs.count = bd.indices.size();

		/* Vertex array */
		glBindBuffer(GL_ARRAY_BUFFER, bufs.pos);
		glBufferData(GL_ARRAY_BUFFER, bd.plainVertices.size() * sizeof(bd.plainVertices[0]), &bd.plainVertices.front(), GL_STATIC_DRAW);

		/* Normal array */
		glBindBuffer(GL_ARRAY_BUFFER, bufs.nrm);
		glBufferData(GL_ARRAY_BUFFER, bd.plainNormals.size() * sizeof(bd.plainNormals[0]), &bd.plainNormals.front(), GL_STATIC_DRAW);

		/* Texture coordinates array */
		glBindBuffer(GL_ARRAY_BUFFER, bufs.tex);
		glBufferData(GL_ARRAY_BUFFER, bd.plainTextures.size() * sizeof(bd.plainTextures[0]), &bd.plainTextures.front(), GL_STATIC_DRAW);

		// Index array
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufs.ind);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, bd.indices.size() * sizeof(bd.indices[0]), &bd.indices.front(), GL_STATIC_DRAW);

		this->buf.push_back(bufs);
	}
}
