/** \file
 * \brief Implementation of TIN class.
 */

#include "TIN.h"
#include "drawmap.h"
#include "glw/GLWchart.h"

extern "C"{
#include <clib/mathdef.h>
#include <clib/gl/multitex.h>
#include <clib/timemeas.h>
}

#include <cpplib/mat2.h>

#if _WIN32
#define exit something_meanless
#undef exit
#include <windows.h>
#endif
#include <gl/GL.h>
#include <GL/glext.h>
#include <fstream>

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

TIN::TIN(const char *fname) : vertices(vertexPredicate){
	std::ifstream is(fname);
	if(is.fail())
		return;
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

int TIN::getat(WarMapTile *return_info, int x, int y){
	Vec2i v(x, y);
	for(Triangles::iterator it = triangles.begin(); it != triangles.end(); ++it){
		Vec2i v01 = (it->vertices[1] - it->vertices[0]);
		Vec2i v12 = (it->vertices[2] - it->vertices[1]);
		Vec2i v20 = (it->vertices[0] - it->vertices[2]);
		int vp0 = v01.vp(v - Vec2i(it->vertices[0]));
		int vp1 = v12.vp(v - Vec2i(it->vertices[1]));
		int vp2 = v20.vp(v - Vec2i(it->vertices[2]));
		if(vp0 <= 0 && vp1 <= 0 && vp2 <= 0 || 0 <= vp0 && 0 <= vp1 && 0 <= vp2){
//			return_info->height = it->vertices[0][2];
			Vec2d v0 = (v - Vec2i(it->vertices[0])).cast<double>();
			Mat2d mat(v01.cast<double>(), -v20.cast<double>());
			Mat2d imat = mat.inverse();
			double sp01 = imat.vec2(0).sp(v0);
			double sp02 = imat.vec2(1).sp(v0);
			return_info->height =
				  (it->vertices[1][2] - it->vertices[0][2]) * sp01
				+ (it->vertices[2][2] - it->vertices[0][2]) * sp02
				+ it->vertices[0][2];
			return 1;
		}
	}
	return 0;
}

void TIN::size(int *x, int *y){
	*x = 1024;
	*y = 1024;
}

double TIN::width(){
	static const double latscale = 10000. / 180.; // Latitude scaling
	return latscale;
}

void TIN::draw(){
	// If there's nothing to draw, probably loading the TIN from a file failed.
	if(vertices.empty() || triangles.empty())
		return;

	timemeas_t tm;
	TimeMeasStart(&tm);

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

	glPushMatrix();
	glRotated(-90, 1, 0, 0); // Show the surface aligned on X-Z plane
	static const int tileSize = 1024;
	static const double latscale = 10000. / 180. / tileSize; // Latitude scaling
	static const double longscale = latscale * cos(35 / deg_per_rad); // Longitude scaling
	glScaled(-longscale, latscale, 0.001 / 4.);
	glTranslated(-(tileSize - 1) / 2., -(tileSize - 1) / 2., 0); // Offset center

	// Make the front face reversed because the data itself are reversed.
	glFrontFace(GL_CW);

	if(0 < initBuffers()){
		static GLuint bufs[4];
		static bool init = false;
		if(!init){
			glGenBuffers(4, bufs);

			typedef std::map<Vec3i, GLuint, bool (*)(const Vec3i &, const Vec3i &)> VertexIndMap;
			VertexIndMap mapVertices(vertexPredicate);
			std::vector<Vec3i> plainVertices;
			std::vector<Vec3d> plainNormals;
			for(Vertices::iterator it = vertices.begin(); it != vertices.end(); ++it){
				mapVertices[it->first] = plainVertices.size();
				plainVertices.push_back(it->first);
				plainNormals.push_back(it->second.normal);
			}
//			std::copy(vertices.begin(), vertices.end(), plainVertices);
//			for(Vertices::iterator it = vertices.begin(); it != vertices.end(); ++it)
//				plainVertices.push_back(it->first);
			std::vector<GLuint> indices;
			for(Triangles::iterator it = triangles.begin(); it != triangles.end(); ++it){
				for(int j = 0; j < 3; j++){
					VertexIndMap::iterator it2 = mapVertices.find(it->vertices[j]);
					if(it2 != mapVertices.end()){
						indices.push_back(it2->second);
					}
					else{
						assert(0);
					}
				}
			}


			/* Vertex array */
			glBindBuffer(GL_ARRAY_BUFFER, bufs[0]);
			glBufferData(GL_ARRAY_BUFFER, plainVertices.size() * sizeof(plainVertices[0]), &plainVertices.front(), GL_STATIC_DRAW);

			/* Normal array */
			glBindBuffer(GL_ARRAY_BUFFER, bufs[1]);
			glBufferData(GL_ARRAY_BUFFER, plainNormals.size() * sizeof(plainNormals[0]), &plainNormals.front(), GL_STATIC_DRAW);

			/* Texture coordinates array */
			glBindBuffer(GL_ARRAY_BUFFER, bufs[2]);
			glBufferData(GL_ARRAY_BUFFER, plainVertices.size() * sizeof(plainVertices[0]), &plainVertices.front(), GL_STATIC_DRAW);

			// Index array
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufs[3]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), &indices.front(), GL_STATIC_DRAW);
  
			init = true;
		}

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		/* Vertex array */
		glBindBuffer(GL_ARRAY_BUFFER, bufs[0]);
		glVertexPointer(3, GL_INT, 0, (0));

		/* Normal array */
		glBindBuffer(GL_ARRAY_BUFFER, bufs[1]);
		glNormalPointer(GL_DOUBLE, 0, (0));

		/* Texture coordinates array */
		glBindBuffer(GL_ARRAY_BUFFER, bufs[2]);
		glTexCoordPointer(3, GL_INT, 0, (0));

		// Index array
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufs[3]);

		glDrawElements(GL_TRIANGLES, triangles.size() * 3, GL_UNSIGNED_INT, 0);
//		glDrawArrays(GL_TRIANGLES, 0, vertices.size());

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	else{

		// Display lists do not work well with tens of thousands of primitives,
		// the vertex buffer objects cope with them better.
	/*	static GLuint list = 0;
		if(list){
			glCallList(list);
			return;
		}
		else{

		list = glGenLists(1);
		glNewList(list, GL_COMPILE_AND_EXECUTE);*/
		
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
	/*	glEndList();
		}*/

	}

	glPopMatrix();
	glPopAttrib();

	GLWchart::addSampleToCharts("tintime", TimeMeasLap(&tm));

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
