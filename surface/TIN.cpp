/** \file
 * \brief Implementation of TIN class.
 */

#include "TIN.h"
#include "drawmap.h"

extern "C"{
#include <clib/gl/multitex.h>
}
#if _WIN32
#define exit something_meanless
#undef exit
#include <windows.h>
#endif
#include <gl/GL.h>
#include <GL/glext.h>
#include <fstream>

TIN::TIN(const char *fname) : vertices(vertexPredicate){
	std::ifstream is(fname);
	Triangle tri;
	int itri = 0;
	do{
		char buf[256];
		is.getline(buf, sizeof buf);
		int xyz[3];
		if(3 <= sscanf(buf, "%d %d %d", &xyz[0], &xyz[1], &xyz[2])){
			if(itri < 3){
				tri.vertices[itri] = Vec3i(xyz);
				if(++itri == 3){
					Vec3d v01 = (tri.vertices[1] - tri.vertices[0]).cast<double>();
					Vec3d v02 = (tri.vertices[2] - tri.vertices[0]).cast<double>();
					tri.normal = v01.vp(v02).norm();
					for(int i = 0; i < 3; i++){
						vertices[tri.vertices[i]].vset.insert((triangles.size()) * 3 + i);
						// We keep reference pointers to the vertex object from triangles,
						// although we could reach it by looking up map of vertices.
						// This is purely for execution time optimization.
						tri.vrefs[i] = &vertices[tri.vertices[i]];
					}
					triangles.push_back(tri);
					itri = 0;
				}
			}
		}
		else // Reset if an invalid or blank line is encountered, we only accept triangles
			itri = 0;
	} while(!is.eof());

	// The second pass calculates normals for each vertex
	for(Vertices::iterator it = vertices.begin(); it != vertices.end(); ++it){
		Vertex &v = it->second;
		VertexRefSet &vset = v.vset;

		// We need to accumulate all adjacent triangle's normals to obtain smooth-shaded normals.
		Vec3d vaccum(0,0,0);
		assert(vset.size() != 0); // Zero division case
		for(VertexRefSet::iterator kit = vset.begin(); kit != vset.end(); ++kit){
			Triangle &tri = triangles[*kit / 3];
			int edge = *kit % 3;
			Vec3d v01 = (tri.vertices[(edge + 1) % 3] - tri.vertices[edge]).cast<double>();
			Vec3d v02 = (tri.vertices[(edge + 2) % 3] - tri.vertices[edge]).cast<double>();
			vaccum += v01.vp(v02).norm();
		}

		// Could we possibly get zero vector for a summed normal vector, in which case zero division
		// protection is needed? I don't know.
		v.normal = vaccum.norm();
	}
}

double TIN::width(){
	return 10.;
}

void TIN::draw(){
	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT);
	if(glActiveTextureARB) for(int i = 0; i < 2; i++){
		glActiveTextureARB(i == 0 ? GL_TEXTURE0_ARB : GL_TEXTURE1_ARB);
		glCallList(generate_ground_texture());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
		glEnable(GL_TEXTURE_2D);
/*			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, i ? GL_MODULATE : GL_MODULATE);*/
	}
	else{
		glCallList(generate_ground_texture());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
	}

	glDisable(GL_COLOR_MATERIAL);
	glMaterialfv(GL_FRONT, GL_AMBIENT, Vec4f(0.2f, 0.2f, 0.2f, 1.f));
	glMaterialfv(GL_FRONT, GL_DIFFUSE, Vec4f(1.0f, 1.0f, 1.0f, 1.f));

	// Texture coordinates generation doesn't work with shadow mapping shader.
/*	glGetError();
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	GLenum err = glGetError();
	glTexGendv(GL_S, GL_OBJECT_PLANE, Vec4d(0,0,1,0));
	err = glGetError();
	glEnable(GL_TEXTURE_GEN_S);
	err = glGetError();
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGendv(GL_T, GL_OBJECT_PLANE, Vec4d(0,0,1,0));
	glEnable(GL_TEXTURE_GEN_T);*/

	glPushMatrix();
	glRotated(-90, 1, 0, 0); // Show the surface aligned on X-Z plane
	glScaled(10./1201, 10./1201, 0.5/1201);
	glTranslated(-1201/2, -1201/2, 0);
	glBegin(GL_TRIANGLES);
	for(int i = 0; i < triangles.size(); i++){
		Triangle &tri = triangles[i];
		glNormal3dv(tri.normal);
		for(int j = 0; j < 3; j++){
			glNormal3dv(tri.vrefs[j]->normal);
			glTexCoord2iv(tri.vertices[j]);
			glVertex3iv(tri.vertices[j]);
		}
	}
	glEnd();
	glPopMatrix();
	glPopAttrib();
}

TIN::~TIN(){
}

/// The predicate to order Vec3i objects to construct an ordered tree.
bool TIN::vertexPredicate(const Vec3i &a, const Vec3i &b){
	// In this application, we only need 2 components to identify equality, because
	// the z coordinate is uniquely derived from x and y coordinates.
	// But we leave the micro-optimization for later work.
	for(int i = 0; i < 3; i++){
		if(a[i] < b[i])
			return true;
		else if(b[i] < a[i])
			return false;
	}
	return false;
}
