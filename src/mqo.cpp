/** \file
 * \brief Implementation of Metasequoia object loading functions and Model class.
 */
#include "mqo.h"
#include "ysdnmmot.h"
//#include "draw/material.h"
//#include <clib/avec3.h>
#include <clib/avec4.h>
#include <clib/c.h>
#include <clib/mathdef.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <stddef.h>


#ifdef _WIN32
#else
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

typedef struct{
	std::istream *is;
	size_t n;
	char *buf;
} FPOS;

static char *ftok(FPOS *fp){
	size_t i;
	int lc;
	char *s = fp->buf;
	for(i = 0; i < fp->n; i++){
		char c;
		fp->is->get(c);
		if(fp->is->eof() || isspace(c)){
			if(i == 0 || isspace(lc)){
				if(fp->is->eof()) return NULL;
				lc = c;
				continue;
			}
			*s = '\0';
			return fp->buf;
		}
		lc = c;
		*s++ = (char)c;
	}
	if(i == fp->n) /* overflow! */
		return NULL;
	return s;
}

static char *quotok(char **src){
	char inquote = 0;
	char *content = NULL, *ret = NULL;
	while(**src != '\0'){
		if(content && !inquote && isspace(**src)){
			*(*src)++ = '\0';
			return ret;
		}
		if(!inquote){
			if(**src == '"')
				inquote = '"';
			else if(**src == '(')
				inquote = ')';
			if(!content && inquote)
				content = ret = *src + 1;
		}
		else if(**src == inquote){
			*(*src)++ = '\0';
			return ret;
		}
		if(!content && !isspace(**src)){
			content = ret = *src;
		}
		(*src)++;
	}
	return ret;
}

static const Mesh::Attrib atr0 = {
	NULL, /* char *name; */
	0, /*unsigned valid; /* validity flag of following values */
	{1.F, 1.F, 1.F, 1.F}, /*float col[4]; /* clamped */
	{0.F, 0.F, 0.F, 1.F}, /*float tra[4];*/
	{1.F, 1.F, 1.F, 1.F}, /*float amb[4]; /* ambient */
	{0.F, 0.F, 0.F, 1.F}, /*float emi[4]; /* emission */
	{1.F, 1.F, 1.F, 1.F}, /*float dif[4]; /* diffuse */
	{0.F, 0.F, 0.F, 1.F}, /*float spc[4]; /* specular lighting */
	0.F, // float shininess
	NULL, /*char *colormap; /* texture color map name */
	{0, 0, 1, 1}, /*sufcoord mapsize[4];*/
	{0, 0, 1, 1}, /*sufcoord mapview[4];*/
	{0, 0, 1, 1}, /*sufcoord mapwind[4];*/
};

static int chunk_material(Mesh *ret, FPOS *pfo){
	char *s;
	int n, i, j;
	Mesh::Attrib *patr;
	s = ftok(pfo);
	if(!s) return NULL;
	n = atoi(s);

	/* multiple material chunks leads to an error. */
	if(ret->a)
		return 0;

	ret->na = n;
	ret->a = (Mesh::Attrib*)malloc(n * sizeof *ret->a);

	while((s = ftok(pfo)) && s[0] != '{');

	for(i = 0; i < n;){
		char line[512], *cur;
		Mesh::Attrib *atr = &ret->a[i];
		double opa = 1.;
		pfo->is->getline(line, sizeof line);
		cur = line;
		s = quotok(&cur);
		if(!s || s[0] == '}')
			continue;
		if(s[0] == '\0')
			return 0;
		ret->a[i] = atr0;
		ret->a[i].name = (char*)malloc(strlen(s) + 1);
		strcpy(ret->a[i].name, s);
		while(s = quotok(&cur)){
			if(!strnicmp(s, "col(", sizeof "col(" - 1)){
				char *p = &s[sizeof"col("-1];
				for(j = 0; j < 3; j++){
					ret->a[i].col[j] = (float)strtod(p, &p);
				}
				opa = (float)strtod(p, &p); /* opaqueness and transparency is opposite notion */
				for(j = 0; j < 3; j++)
					atr->tra[j] = opa;
				if(atr) atr->valid |= Mesh::Color | Mesh::Trans;
			}
			else if(!strnicmp(s, "dif(", sizeof "dif(" - 1)){
				float dif = (float)atof(&s[sizeof"dif("-1]);
				for(j = 0; j < 3; j++)
					ret->a[i].dif[j] = ret->a[i].col[j] * dif;
				if(atr) atr->valid |= Mesh::Diffuse;
			}
			else if(!strnicmp(s, "amb(", sizeof "amb(" - 1)){
				float amb = (float)atof(&s[sizeof"amb("-1]);
				for(j = 0; j < 3; j++)
					ret->a[i].amb[j] = ret->a[i].col[j] * amb;
				if(atr) atr->valid |= Mesh::Ambient;
			}
			else if(!strnicmp(s, "emi(", sizeof "emi(" - 1)){
				float emi = (float)atof(&s[sizeof"emi("-1]);
				for(j = 0; j < 3; j++)
					ret->a[i].emi[j] = ret->a[i].col[j] * emi;
				if(atr) atr->valid |= Mesh::Emission;
			}
			else if(!strnicmp(s, "spc(", sizeof "spc(" - 1)){
				float spc = (float)atof(&s[sizeof"spc("-1]);
				for(j = 0; j < 3; j++)
					ret->a[i].spc[j] = /*ret->a[i].col[j] **/ spc;
				if(atr) atr->valid |= Mesh::Specular;
			}
			else if(!strnicmp(s, "power(", sizeof "power(" - 1)){
				float power = (float)atof(&s[sizeof"power("-1]);
				ret->a[i].shininess = power;
				if(atr && 0. < Vec3f(ret->a[i].spc).slen())
					atr->valid |= Mesh::Shine;
			}
			else if(!strnicmp(s, "tex(", sizeof "tex(" - 1)){
				char *p = &s[sizeof"tex("-1], *p2;
				p2 = quotok(&p);
				ret->a[i].colormap = (char*)malloc(strlen(p2) + 1);
				strcpy(ret->a[i].colormap, p2);
				if(atr) atr->valid |= Mesh::Texture;
			}
			else if(!strnicmp(s, "aplane(", sizeof "aplane(" - 1)){
				char *p = &s[sizeof"aplane("-1], *p2;
				p2 = quotok(&p);
				ret->a[i].colormap = (char*)malloc(strlen(p2) + 1);
				strcpy(ret->a[i].colormap, p2);
				if(atr) atr->valid |= Mesh::Texture;
			}
		}
		ret->a[i].dif[3] *= opa;
		ret->a[i].amb[3] *= opa;
		i++;
	}

	if(!(s = ftok(pfo)) || s[0] != '}')
		return 0;

	return 1;
}

static int chunk_object(Mesh *ret, FPOS *pfo, Mesh::Coord scale, struct Bone ***bones, int num){
	char *s, *name = NULL;
	int n, i, j;
	Mesh::Attrib *patr;
	char line[512], *cur;
	Mesh::Index atr = (Mesh::Index)-1;
	int shading = 0;
	double facet = 0.;
	struct Bone *bone = NULL;
	int mirror = 0, mirror_axis = 0, mirrors = 0;
	int mirrornv[3];

	pfo->is->getline(line, sizeof line);
	s = line;
	if(!s) return NULL;
	cur = s;
	s = quotok(&cur);
	name = (char*)malloc(strlen(s) + 1);
	strcpy(name, s);

	if(bones && *bones){
//		(*bones)[num] = malloc(sizeof(struct Bone));
		bone = (*bones)[num];
//		VECNULL(bone->joint);
//		bone->depth = 0;
//		bone->parent = NULL;
	}

	/* forward until vertex chunk */
	while((pfo->is->getline(line, sizeof line), !pfo->is->eof()) && !(cur = line, (s = quotok(&cur)) && !stricmp(s, "vertex"))){
		if(!stricmp(s, "shading"))
			shading = atoi(quotok(&cur));
		else if(!stricmp(s, "facet"))
			facet = atof(quotok(&cur));
		else if(!stricmp(s, "depth")){
			if(bone){
				bone->depth = atoi(quotok(&cur));
				if(num){
					int j;
					for(j = num - 1; 0 <= j; j--){
						if((*bones)[j]->depth < bone->depth){
							bone->parent = (*bones)[j];
							bone->nextSibling = (*bones)[j]->children;
							(*bones)[j]->children = bone;
							break;
						}
					}
				}
				else
					bone->parent = NULL;
			}
		}
		else if(!stricmp(s, "mirror")){
			mirror = atoi(quotok(&cur));
		}
		else if(!stricmp(s, "mirror_axis")){
			mirror_axis = atoi(quotok(&cur));

			// There could be multiple mirrors for a object, so we count it for allocating space.
			mirrors = 0;
			for(int m = 0; m < 3; m++) if(mirror_axis & (1 << m))
				mirrors++;
		}
	}

	if(!s)
		return 0;

	/* multiple vertex chunks leads to an error. */
	if(ret->v)
		return 0;

	s = quotok(&cur);
	ret->nv = n = atoi(s);
	// Mirroring generates twice as many vertices for each mirror, i.e. 2 power count of mirrors.
	ret->v = (Mesh::Coord(*)[3])malloc(n * (mirror ? 1 << mirrors : 1) * sizeof *ret->v);

	i = 0;
	while(i < n && (pfo->is->getline(line, sizeof line), s = line, !pfo->is->eof())){
		if(!s)
			return NULL;
		if(*s == '{')
			break;
		cur = line;
		for(j = 0; j < 3 && (s = quotok(&cur)); j++)
			ret->v[i][j] = (Mesh::Coord)atof(s) * (scale);
		i++;
	}

	if(mirror){
		for(int m = 0; m < 3; m++){
			// Check for each axis if it's flagged for mirroring.
			if(mirror_axis & (1 << m)){
				// Mirrored vertices have simply negated coordinate along axis perpendicular to the mirror.
				for(i = 0; i < n; i++){
					VECCPY(ret->v[i+n], ret->v[i]);
					ret->v[i+n][m] *= -1;
				}
				mirrornv[m] = n;
				n = ret->nv *= 2;
			}
		}
	}

	/* forward until face chunk */
	while((pfo->is->getline(line, sizeof line), !pfo->is->eof()) && !(cur = line, (s = quotok(&cur)) && !stricmp(s, "face")));

	/* multiple face chunks leads to an error. */
	if(ret->p)
		return 0;

	s = quotok(&cur);
	ret->np = n = atoi(s);
	// Mirroring generates twice as many vertices for each mirror, i.e. 2 power count of mirrors.
	ret->p = (Mesh::Primitive**)malloc(n * (mirror ? 1 << mirrors : 1) * sizeof *ret->p);

	i = 0;
	while(i <= n && (pfo->is->getline(line, sizeof line), s = line, !pfo->is->eof())){
		int polys;
		Mesh::ElemType type = Mesh::suf_poly;
		Mesh::Index verts[4], norms[4], uvs[4]; /* vertex count is capped at 4 in Metasequoia */
		if(!s)
			return NULL;
		if(*s == '{')
			break;
		else if(strchr(s, '}'))
			break;
		cur = line;
		s = quotok(&cur);
		polys = atoi(s);

		// Polygons with two vertices (edges) are treated as bone segments.
		if(polys == 2){
			 while(s = quotok(&cur)) if(!strnicmp(s, "V(", sizeof"V("-1)){
				char *p = &s[sizeof"V("-1];

				if(bone){
					int vind[2];
					avec3_t org;
					double sd;
					vind[0] = atoi(p);
					quotok(&p);
					vind[1] = atoi(p);

					// Set to measure distance from parent bonee's joint position.
					if(bone->parent){
						VECCPY(org, bone->parent->joint);
					}
					else{
						VECNULL(org);
					}

					// Choose the farther vertex of the edge as the bone joint, since the user of Metasequoia has no
					// way to figure out which is earlier in the data structure.
					if(VECSDIST(org, ret->v[vind[0]]) < VECSDIST(org, ret->v[vind[1]])){
						VECCPY(bone->joint, ret->v[vind[1]]);
					}
					else{
						VECCPY(bone->joint, ret->v[vind[0]]);
					}
				}
			}

			// Exclude this polygon (line) from drawing
			n--;
			ret->np--;
			continue;
		}
		else while(s = quotok(&cur)){

			/* cache vertex indices temporarily until presence of UV map is determined. */
			if(!strnicmp(s, "V(", sizeof"V("-1)){
				char *p = &s[sizeof"V("-1];

				/* face direction conversion */
				for(j = 0; j < polys; j++)
					verts[polys - j - 1] = (Mesh::Coord)strtol(p, &p, 10);

				/* Metasequoia never writes normal vector directions as part of data file,
				so we always need to calculate them, regardless of whether smoothshading is
				enabled or not.
				For drawing performance, we choose to calculate normal vectors here, not
				rendering or loading time of actual use. Sure we could add another polygon
				type to inform renderer to calculate them on runtime, but I prefer leaving
				runtime routine simpler (thus hopefully faster).
				The following code fragment is just the same as standard suf.c.*/
				{
					unsigned short a, b, c, vvi;
					double vv[3];
					int i = polys;

					/* there's an occasion that three consequative vertices lie on a line,
						so we need to find the valid triplet from all possible cases.
						note that polygon normal is calculated the same direction,
						as long as the polygon is convex. */
					for(a = 0; a < i; a++) for(b = a+1; b < i; b++) for(c = b+1; c < i; c++){
						double *v0, *v1, *v2;
						double v01[3], v02[3];
						v0 = ret->v[verts[a]];
						v1 = ret->v[verts[b]];
						v2 = ret->v[verts[c]];
						VECSUB(v01, v0, v1);
						VECSUB(v02, v0, v2);
						VECVP(vv, v01, v02);
						if(vv[0] != 0. || vv[1] != 0. || vv[2] != 0.)
							goto abc_break;
					}
	abc_break:

					/* polygon on a line */
					if(a == i){
						Mesh::Coord nullvec[3];
						VECNULL(nullvec);
						vvi = ret->add_vertex(nullvec);
						for(j = 0; j < i; j++)
							norms[j] = vvi;
						break;
					}

					/* this normalization isn't necessary, but
						it would improve memory space efficiency, for
						normal vectors tend to point the same direction
						with different length. */
					VECNORMIN(vv);

					vvi = ret->add_vertex(vv);
					for(j = 0; j < i; j++)
						norms[j] = vvi;
				}
			}
			else if(!strnicmp(s, "M(", sizeof"M("-1)){
				atr = (Mesh::Index)atoi(&s[sizeof"M("-1]);
				assert(atr < 30000);
			}
			else if(!strnicmp(s, "UV(", sizeof"UV("-1)){
				char *q = &s[sizeof"UV("-1];
				Mesh::Coord uv[3];
				type = Mesh::suf_uvpoly;
				for(j = 0; j < polys; j++){
					uv[0] = strtod(q, &q);
					uv[1] = strtod(q, &q);
					uv[2] = 0.;
					uvs[polys - j - 1] = ret->add_vertex(uv);
				}
			}
		}

		if(atr == (Mesh::Index)-1){
			fprintf(stderr, "Error: Material is not specified! poly %d\n", i);
			return 0;
		}

		if(type == Mesh::suf_uvpoly){
			struct Mesh::UVPolygon *p;
			type = Mesh::suf_uvpoly;
			ret->p[i] = (Mesh::Primitive*)malloc(offsetof(Mesh::UVPolygon, v) + polys * sizeof(struct Mesh::UVPolygon::suf_uvpoly_vertex));
			p = &ret->p[i]->uv;
			p->t = Mesh::suf_uvpoly;
			p->n = polys;
			p->atr = atr;
			for(j = 0; j < polys; j++){
				p->v[j].p = verts[j];
				p->v[j].n = norms[j];
				p->v[j].t = uvs[j];
			}
		}
		else{
			Mesh::Polygon *p;
			ret->p[i] = (Mesh::Primitive*)malloc(offsetof(Mesh::UVPolygon, v) + polys * sizeof(Mesh::Index[2]));
			p = &ret->p[i]->p;
			p->t = Mesh::suf_poly;
			p->n = polys;
			p->atr = atr;
			assert(p->atr < 30000);
			for(j = 0; j < polys; j++){
				p->v[j][0] = verts[j];
				p->v[j][1] = norms[j];
			}
		}

		i++;
	}

	for(int m = 0; m < 3; m++) if(mirror_axis & (1 << m)){
		n = ret->np;
		for(i = 0; i < n; i++){
			if(ret->p[i]->t == Mesh::suf_uvpoly){
				Mesh::UVPolygon *p, *p0 = &ret->p[i]->uv;
				ret->p[i+n] = (Mesh::Primitive*)malloc(offsetof(Mesh::UVPolygon, v) + p0->n * sizeof(Mesh::UVPolygon::suf_uvpoly_vertex));
				p = &ret->p[i+n]->uv;
				p->t = Mesh::suf_uvpoly;
				p->n = p0->n;
				p->atr = p0->atr;
				for(j = 0; j < p0->n; j++){
					avec3_t vecn;
					// Flip face direction because it's mirrored.
					p->v[j].p = p0->v[p0->n-j-1].p + mirrornv[m];
					VECCPY(vecn, ret->v[p0->v[p0->n-j-1].n]);
					// Normal vector's coordinate along mirror axis must be negated just like position vector.
					vecn[m] *= -1;
					p->v[j].n = ret->add_vertex(vecn);
					p->v[j].t = p0->v[p0->n-j-1].t; // Keep texture coord to the same.
				}
			}
			else{
				Mesh::Polygon *p, *p0 = &ret->p[i]->p;
				ret->p[i+n] = (Mesh::Primitive*)malloc(offsetof(Mesh::Polygon, v) + p0->n * sizeof(Mesh::Index[2]));
				p = &ret->p[i+n]->p;
				p->t = Mesh::suf_poly;
				p->n = p0->n;
				p->atr = p0->atr;
				for(j = 0; j < p0->n; j++){
					avec3_t vecn;
					// Flip face direction because it's mirrored.
					p->v[j][0] = p0->v[p0->n-j-1][0] + mirrornv[m];
					VECCPY(vecn, ret->v[p0->v[p0->n-j-1][1]]);
					// Normal vector's coordinate along mirror axis must be negated just like position vector.
					vecn[m] *= -1;
					p->v[j][1] = ret->add_vertex(vecn);
				}
			}
		}
		ret->np *= 2;
	}

	if(shading){
		double cf;
		cf = cos(facet / deg_per_rad);
		for(i = 0; i < ret->np; i++) for(j = 0; j < ret->p[i]->uv.n; j++){
			int k, l, is = 0;
			Mesh::Index *shares[32];
			Mesh::Index *vertex = ret->p[i]->t == Mesh::suf_poly ? ret->p[i]->p.v[j] : &ret->p[i]->uv.v[j].p;
			avec3_t norm;
			for(k = 0; k < ret->np; k++) for(l = 0; l < ret->p[k]->uv.n; l++){
				Mesh::Index *vertex2 = ret->p[k]->t == Mesh::suf_poly ? ret->p[k]->p.v[l] : &ret->p[k]->uv.v[l].p;
				if(vertex[0] == vertex2[0] && cf < VECSP(ret->v[vertex2[1]], ret->v[vertex[1]])){
					if(is == numof(shares))
						break;
					shares[is++] = vertex2;
				}
			}
			for(k = 0; k < is; k++) if(shares[k][1] != vertex[1])
				break;
			if(k == is)
				continue;
			VECNULL(norm);
			for(k = 0; k < is; k++)
				VECADDIN(norm, ret->v[shares[k][1]]);
			VECNORMIN(norm);
			l = ret->add_vertex(norm);
			for(k = 0; k < is; k++)
				shares[k][1] = l;
		}
	}

	if(!(s = ftok(pfo)) || s[0] != '}')
		return 0;

	return 1;
}

/* the argument must be either full or relative path to existing file,
  for its path is used to search attribute files. */
Mesh *LoadMQO_SUF(const char *fname){
	FILE *fp;
	Mesh *ret;
	char buf[128], *s = NULL, *name = NULL;
	Mesh::Index atr = USHRT_MAX; /* current attribute index */
	FPOS fo;

	fp = fopen(fname, "r");
	if(!fp)
		return NULL;

	std::ifstream is(fname);
	fo.is = &is;
	fo.n = sizeof buf, fo.buf = buf;

	/* checking signatures */
	if(!(s = ftok(&fo)) || stricmp(s, "Metasequoia")) return NULL;
	if(!(s = ftok(&fo)) || stricmp(s, "Document")) return NULL;
	if(!(s = ftok(&fo)) || stricmp(s, "Format")) return NULL;
	if(!(s = ftok(&fo)) || stricmp(s, "Text")) return NULL;
	if(!(s = ftok(&fo)) || stricmp(s, "Ver")) return NULL;
	if(!(s = ftok(&fo)) || stricmp(s, "1.0")) return NULL; /* fixed version validation */

	ret = (Mesh*)malloc(sizeof *ret);
	ret->nv = 0;
	ret->v = NULL;
	ret->na = 0;
	ret->a = NULL;
	ret->np = 0;
	ret->p = NULL;

	do{
		int bracestack = 0;
		s = ftok(&fo);
		if(!s) break;
		if(!stricmp(s, "material")){
			if(!chunk_material(ret, &fo))
				return NULL;
		}
		else if(!stricmp(s, "object")){
			if(!chunk_object(ret, &fo, 1., NULL, 1))
				return NULL;
		}
		else while((s = ftok(&fo))){
			if(s[0] == '{')
				bracestack++;
			else if(s[0] == '}' && !--bracestack)
				break;
		}
	} while(s);

	fclose(fp);

	return ret;
}

/// 
/// \param tex_callback Texture allocator function.
/// \param tex_callback_data Pointer to buffer that texture objects resides.
int LoadMQO_Scale(std::istream &is, Mesh ***pret, char ***pname, Mesh::Coord scale, struct Bone ***bones, MQOTextureCallback *tex_callback, MeshTex ***tex_callback_data){
	char buf[128], *s = NULL, *name = NULL;
	Mesh **ret = NULL;
	Mesh *sufatr = NULL;
	Mesh::Index atr = USHRT_MAX; /* current attribute index */
	FPOS fo;
	int num = 0;

	if(is.eof() || !is.good())
		return 0;

	if(bones)
		*bones = NULL;
	if(tex_callback_data)
		*tex_callback_data = NULL;

	fo.is = &is;
	fo.n = sizeof buf, fo.buf = buf;

	/* checking signatures */
	if(!(s = ftok(&fo)) || stricmp(s, "Metasequoia")) return NULL;
	if(!(s = ftok(&fo)) || stricmp(s, "Document")) return NULL;
	if(!(s = ftok(&fo)) || stricmp(s, "Format")) return NULL;
	if(!(s = ftok(&fo)) || stricmp(s, "Text")) return NULL;
	if(!(s = ftok(&fo)) || stricmp(s, "Ver")) return NULL;
	if(!(s = ftok(&fo)) || stricmp(s, "1.0")) return NULL; /* fixed version validation */

	if(tex_callback_data){
		*tex_callback_data = (MeshTex**)malloc(num * sizeof *tex_callback_data);
	}

	do{
		int bracestack = 0;
		s = ftok(&fo);
		if(!s) break;
		if(!stricmp(s, "material")){
			if(!sufatr){
				sufatr = (Mesh*)malloc(sizeof *sufatr);
				sufatr->nv = 0;
				sufatr->v = NULL;
				sufatr->na = 0;
				sufatr->a = NULL;
				sufatr->np = 0;
				sufatr->p = NULL;
			}
			if(!chunk_material(sufatr, &fo))
				return NULL;
		}
		else if(!stricmp(s, "object")){
			if(!sufatr){
				float col[4] = {1,1,1,1}, zero[4] = {0};
				sufatr = (Mesh*)malloc(sizeof *sufatr);
				sufatr->nv = 0;
				sufatr->v = NULL;
				sufatr->na = 1;
				sufatr->a = (Mesh::Attrib*)malloc(sizeof *sufatr->a);
				sufatr->np = 0;
				sufatr->p = NULL;

				VEC4CPY(sufatr->a[0].amb, col);
				VEC4CPY(sufatr->a[0].dif, col);
				VEC4CPY(sufatr->a[0].emi, zero);
				sufatr->a[0].name = (char*)malloc(strlen("default")+1);
				strcpy(sufatr->a[0].name, "default");
				sufatr->a[0].colormap = NULL;
				VEC4CPY(sufatr->a[0].spc, zero);
				VEC4CPY(sufatr->a[0].tra, zero);
				sufatr->a[0].valid = Mesh::Color | Mesh::Ambient;
			}
			ret = (Mesh**)realloc(ret, (num + 1) * sizeof *ret);
			ret[num] = (Mesh*)malloc(sizeof *ret[num]);
			ret[num]->nv = 0;
			ret[num]->v = NULL;
			ret[num]->na = sufatr->na;
			ret[num]->a = sufatr->a;
			ret[num]->np = 0;
			ret[num]->p = NULL;
			s = ftok(&fo);
			if(s && pname){
				*pname = (char**)realloc(*pname, (num + 1) * sizeof **pname);
				(*pname)[num] = (char*)malloc(strlen(s) - 1);
				strncpy((*pname)[num], &s[1], strlen(s) - 1);
				(*pname)[num][strlen(s) - 2] = '\0';
			}
			if(bones){
				struct Bone *bone;
//				Bone *newbones = new Bone*[num+1];
				*bones = (Bone**)realloc(*bones, (num + 1) * sizeof **bones);
//				(*bones)[num] = (Bone*)malloc(sizeof ***bones);
				(*bones)[num] = new Bone;
				bone = (*bones)[num];
				bone->depth = 0;
				VECNULL(bone->joint);
//				bone->name = (char*)malloc(strlen(s) - 1);
//				strncpy(bone->name, &s[1], strlen(s) - 1);
//				bone->name[strlen(s) - 2] = '\0';
				s[strlen(s)-1] = '\0';
				bone->name = &s[1];
				bone->suf = ret[num];
				bone->suftex = NULL;
				bone->parent = NULL;
				bone->children = NULL;
				bone->nextSibling = NULL;
			}
			if(!chunk_object(ret[num], &fo, scale, bones, num))
				return NULL;
			if(tex_callback && tex_callback_data){
				*tex_callback_data = (MeshTex**)realloc(*tex_callback_data, (num + 1) * sizeof **tex_callback_data);
				(*tex_callback)(ret[num], &(*tex_callback_data)[num]);
/*				CacheSUFMaterials(ret[num]);
				(*texes)[num] = gltestp::AllocSUFTex(ret[num]);*/
				if(bones){
					(*bones)[num]->suftex = (*tex_callback_data)[num];
				}
			}
			num++;
		}
		else while((s = ftok(&fo))){
			if(s[0] == '{')
				bracestack++;
			else if(s[0] == '}' && !--bracestack)
				break;
		}
	} while(s);

	*pret = ret;
	return num;
}

int LoadMQO(std::istream &is, Mesh ***pret, char ***pname, struct Bone ***bones){
	return LoadMQO_Scale(is, pret, pname, 1., bones);
}


struct Model *LoadMQOModelSource(std::istream &is, double scale, MQOTextureCallback *tex_callback){
	struct Model *ret;
	ret = (struct Model*)malloc(sizeof *ret);
	ret->n = LoadMQO_Scale(is, &ret->sufs, NULL, scale, &ret->bones, tex_callback, &ret->tex);
	if(!ret->n){
		free(ret);
		return NULL;
	}
	return ret;
}

struct Model *LoadMQOModel(const char *fname, double scale, MQOTextureCallback *tex_callback){
	std::ifstream ifs(fname);
	return LoadMQOModelSource(static_cast<std::istream&>(ifs), scale, tex_callback);
}


bool Model::getBonePos(const char *boneName, const ysdnmv_t &var, Vec3d *pos, Quatd *rot)const{
	for(int i = 0; i < this->n; i++) if(bones[i]->depth == 0)
		if(getBonePosInt(boneName, var, bones[i], Vec3d(0,0,0), Quatd(0,0,0,1), pos, rot))
			return true;
	return false;
}


bool Model::getBonePosInt(const char *boneName, const ysdnmv_t &v0, const Bone *bone, const Vec3d &spos, const Quatd &srot, Vec3d *pos, Quatd *rot)const{
	Vec3d apos = spos;
	Quatd arot = srot;
	for(const ysdnmv_t *v = &v0; v; v = v->next){
		ysdnm_bone_var *bonevar = v->bonevar;
		int bones = min(v->bones, this->n);
		for(int i = 0; i < bones; i++) if(!strcmp(bonevar[i].name, bone->name)){
			apos += arot.trans(bone->joint);
			apos += arot.trans(v->bonevar[i].pos);
			arot *= bonevar[i].rot;
			apos -= arot.trans(bone->joint);
		}
	}
	if(!strcmp(boneName, bone->name)){
		if(pos)
			*pos = apos + arot.trans(bone->joint);
		if(rot)
			*rot = arot;
		return true;
	}
	for(const Bone *nextbone = bone->children; nextbone; nextbone = nextbone->nextSibling){
		if(nextbone->suf){
			if(getBonePosInt(boneName, v0, nextbone, apos, arot, pos, rot))
				return true;
		}
	}
	return false;
}





bool Model::getBonePos(const char *boneName, const MotionPose &var, Vec3d *pos, Quatd *rot)const{
	for(int i = 0; i < this->n; i++) if(bones[i]->depth == 0)
		if(getBonePosInt(boneName, var, bones[i], Vec3d(0,0,0), Quatd(0,0,0,1), pos, rot))
			return true;
	return false;
}


bool Model::getBonePosInt(const char *boneName, const MotionPose &v0, const Bone *bone, const Vec3d &spos, const Quatd &srot, Vec3d *pos, Quatd *rot)const{
	Vec3d apos = spos;
	Quatd arot = srot;
	for(const MotionPose *v = &v0; v; v = v->next){
		MotionPose::const_iterator it = v->nodes.find(bone->name);
//		for(int i = 0; i < bones; i++) if(!strcmp(bonevar[i].name, bone->name)){
		if(it != v->nodes.end()){
			apos += arot.trans(bone->joint);
			apos += arot.trans(it->second.pos);
			arot *= it->second.rot;
			apos -= arot.trans(bone->joint);
		}
	}
	if(!strcmp(boneName, bone->name)){
		if(pos)
			*pos = apos + arot.trans(bone->joint);
		if(rot)
			*rot = arot;
		return true;
	}
	for(const Bone *nextbone = bone->children; nextbone; nextbone = nextbone->nextSibling){
		if(nextbone->suf){
			if(getBonePosInt(boneName, v0, nextbone, apos, arot, pos, rot))
				return true;
		}
	}
	return false;
}

