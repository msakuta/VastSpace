/** \file
 * \brief Implementation of Mesh and MeshTex classes.
 */
#include "Mesh.h"
#include "clib/avec3.h"
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef Mesh::Coord Coord;
typedef Mesh::Index Index;


Mesh::Attrib *Mesh::find_atr(const char *name){
	int i;
	for(i = 0; i < na; i++){
		if(a[i].name && !strcmp(a[i].name, name))
			return &a[i];
	}
	return NULL;
}

Index Mesh::add_vertex(const Vec &v){
	int i;
	for(i = 0; i < this->nv; i++){
		Coord *va = this->v[i];
		if(va[0] == v[0] && va[1] == v[1] && va[2] == v[2])
			return (Index)i;
	}
	if(INDEX_MAX - 1 <= this->nv)
		return INDEX_MAX;
	this->v = (Vec*)realloc(this->v, ++this->nv * sizeof *this->v);
	VECCPY(this->v[i], v);
	return (Index)i;
}

Mesh::Primitive **Mesh::add_poly(){
	this->p = (Primitive**)realloc(this->p, (this->np + 1) * sizeof *this->p);
	this->p[this->np] = (Primitive*)malloc(sizeof(Primitive));
/*	suf->p[suf->np]->n = 0;*/
	return &this->p[this->np++];
}

void Mesh::add_polyvert(Polygon **p, Index i, Index j){
	int n = *p ? (*p)->n + 1 : 1;
	assert((*p)->t == ET_Polygon || (*p)->t == ET_Shade);
	*p = (Polygon*)realloc(*p, offsetof(Polygon, v) + n * sizeof (*p)->v);
	(*p)->v[n-1].pos = i;
	(*p)->v[n-1].nrm = j;
	(*p)->n = n;
}

void Mesh::add_uvpolyvert(UVPolygon **uv, Index i, Index j, Index k){
	int n = *uv ? (*uv)->n + 1 : 1;
	assert((*uv)->t == ET_UVPolygon || (*uv)->t == ET_UVShade);
	*uv = (UVPolygon*)realloc(*uv, offsetof(UVPolygon, v) + n * sizeof (*uv)->v);
	(*uv)->v[n-1].pos = i;
	(*uv)->v[n-1].nrm = j;
	(*uv)->v[n-1].tex = k;
	(*uv)->n = n;
}

Mesh::~Mesh(){
	if(v) free(v);
	if(a){
		int i;
		for(i = 0; i < na; i++)
			free(a[i].name);
		free(a);
	}
	if(p){
		int i;
		for(i = 0; i < np; i++)
			free(p[i]);
		free(p);
	}
};
