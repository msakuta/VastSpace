#include "TIN.h"
#if _WIN32
#define exit something_meanless
#undef exit
#include <windows.h>
#endif
#include <gl/GL.h>
#include <fstream>

TIN::TIN(const char *fname){
	std::ifstream is(fname);
	Triangle tri;
	int itri = 0;
	do{
		char buf[256];
		is.getline(buf, sizeof buf);
		int xyz[3];
		if(3 <= sscanf(buf, "%d %d %d", &xyz[0], &xyz[1], &xyz[2])){
			if(itri < 3){
				tri.vertices[itri] = Vec3i(xyz).cast<double>();
				if(++itri == 3){
					Vec3d v01 = tri.vertices[1] - tri.vertices[0];
					Vec3d v02 = tri.vertices[2] - tri.vertices[0];
					tri.normal = v01.vp(v02).norm();
					triangles.push_back(tri);
					itri = 0;
				}
			}
		}
		else // Reset if an invalid or blank line is encountered, we only accept triangles
			itri = 0;
	} while(!is.eof());
}

double TIN::width(){
	return 10.;
}

void TIN::draw(){
	glPushMatrix();
	glRotated(-90, 1, 0, 0); // Show the surface aligned on X-Z plane
	glScaled(10./1201, 10./1201, 1./1201);
	glBegin(GL_TRIANGLES);
	for(int i = 0; i < triangles.size(); i++){
		glNormal3dv(triangles[i].normal);
		for(int j = 0; j < 3; j++)
			glVertex3dv(triangles[i].vertices[j]);
	}
	glEnd();
	glPopMatrix();
}

TIN::~TIN(){
}
